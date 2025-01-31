/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "get_bytes_private.h"

#include "comm/bluetooth_analytics.h"
#include "drivers/flash.h"
#include "drivers/rtc.h"
#include "flash_region/flash_region.h"
#include "kernel/events.h"
#include "kernel/pbl_malloc.h"
#include "services/common/comm_session/session_send_buffer.h"
#include "services/common/get_bytes/get_bytes_storage.h"
#include "services/common/system_task.h"
#include "services/normal/filesystem/pfs.h"
#include "system/hexdump.h"
#include "system/logging.h"
#include "util/math.h"
#include "util/net.h"

#include <bluetooth/analytics.h>
#include <bluetooth/conn_event_stats.h>
#include <os/tick.h>

#include "portmacro.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

// Internal state used by the protocol handler.
typedef struct {
  CommSession *session;
  GetBytesObjectType object_type;
  uint8_t transaction_id;
  uint32_t num_bytes;
  bool sent_header;
  GetBytesStorage storage;
  TickType_t start_ticks;
  SlaveConnEventStats conn_event_stats;
} GetBytesState;


// ----------------------------------------------------------------------------------------
// Private globals
static bool s_get_bytes_in_progress = false;


static void prv_put_status_event(DebugInfoEventState state) {
  PebbleEvent event = {
    .type = PEBBLE_GATHER_DEBUG_INFO_EVENT,
    .debug_info = {
      .source = DebugInfoSourceGetBytes,
      .state = state,
    },
  };
  event_put(&event);
}

// -----------------------------------------------------------------------------------------------
static bool prv_protocol_send_err_response(CommSession *session, int8_t transaction_id,
                                           GetBytesInfoErrorCode result) {
  const GetBytesRspObjectInfo rsp = (const GetBytesRspObjectInfo) {
    .hdr.cmd_id = GET_BYTES_CMD_OBJECT_INFO,
    .hdr.transaction_id = transaction_id,
    .error_code = result,
    .num_bytes = htonl(0),
  };

  bool success = comm_session_send_data(session,
                                        GET_BYTES_ENDPOINT_ID,
                                        (const uint8_t *) &rsp,
                                        sizeof(rsp),
                                        COMM_SESSION_DEFAULT_TIMEOUT);
  if (!success) {
    PBL_LOG(LOG_LEVEL_ERROR, "GET_BYTES: aborted");
    s_get_bytes_in_progress = false;
    return false;
  }

  if (result != GET_BYTES_ALREADY_IN_PROGRESS) {
    s_get_bytes_in_progress = false;
  }

  prv_put_status_event(DebugInfoStateFinished);

  return true;
}

static void prv_gather_and_record_stats(GetBytesState *state) {
  uint32_t elapsed_time_ms = ticks_to_milliseconds(rtc_get_ticks() - state->start_ticks);
  uint32_t bytes_per_sec = ((state->num_bytes * MS_PER_SECOND) / elapsed_time_ms);
  PBL_LOG(LOG_LEVEL_DEBUG, "GET_BYTES: Done sending data. Pushed %"PRIu32" bytes/sec",
          bytes_per_sec);
  bluetooth_analytics_handle_get_bytes_stats(
    state->object_type, state->num_bytes, elapsed_time_ms, &state->conn_event_stats);
}

// -----------------------------------------------------------------------------------------------
static void prv_protocol_send_next_chunk(void* raw_state) {
  GetBytesState* state = (GetBytesState*)raw_state;

  // -------------------------------------------------------------------------------------------
  // Did we fetch the total size yet? If not, walk through all the chunks getting the total size
  if (state->num_bytes == 0) {
    uint32_t size;
    // read the size of object from storage
    GetBytesInfoErrorCode rv = gb_storage_get_size(&state->storage, &size);
    if (rv != GET_BYTES_OK) {
      prv_protocol_send_err_response(state->session, state->transaction_id, rv);
      goto cleanup;
    }
    state->num_bytes = size;
    PBL_LOG(LOG_LEVEL_DEBUG, "GET_BYTES: total bytes: %ld", state->num_bytes);
  }

  // -------------------------------------------------------------------------------------------
  // Send next chunk out
  uint32_t remaining_bytes = state->num_bytes - state->storage.current_offset;
  uint32_t packet_len;

  uint32_t data_len = 0;
  if (state->sent_header) {
    uint32_t max_buf_len = comm_session_send_buffer_get_max_payload_length(state->session);
    if (max_buf_len == 0) {
      // Session disconnected in the mean time
      packet_len = 0;
    } else {
      data_len = MIN(remaining_bytes, max_buf_len - sizeof(GetBytesRspObjectData));
      packet_len = data_len + sizeof(GetBytesRspObjectData);
    }
  } else {
    packet_len = sizeof(GetBytesRspObjectInfo);
  }

  SendBuffer *sb;
  if (packet_len == 0 ||
      !(sb = comm_session_send_buffer_begin_write(state->session, GET_BYTES_ENDPOINT_ID, packet_len,
                                                  COMM_SESSION_DEFAULT_TIMEOUT))) {
    // If timeout, try again
    // MT: What if the session got disconnected?
    system_task_add_callback(prv_protocol_send_next_chunk, state);
    return;
  }

  if (state->sent_header) {
    // Send next chunk of data
    GetBytesRspObjectData* rsp = (GetBytesRspObjectData *) kernel_zalloc_check(packet_len);
    *rsp = (GetBytesRspObjectData) {
      .hdr.cmd_id = GET_BYTES_CMD_OBJECT_DATA,
      .hdr.transaction_id = state->transaction_id,
      .byte_offset = htonl(state->storage.current_offset),
    };

    // read the next chunk from storage
    gb_storage_read_next_chunk(&state->storage, rsp->data, data_len);

    comm_session_send_buffer_write(sb, (const uint8_t *) rsp, packet_len);
    kernel_free(rsp);
    PBL_LOG(LOG_LEVEL_DEBUG, "GET_BYTES: sending next %d bytes. %d remaining", (int)data_len,
            (int)(remaining_bytes-data_len));
  } else {
    // Send image info response
    const GetBytesRspObjectInfo rsp = (const GetBytesRspObjectInfo) {
      .hdr.cmd_id = GET_BYTES_CMD_OBJECT_INFO,
      .hdr.transaction_id = state->transaction_id,
      .error_code = htonl(GET_BYTES_OK),
      .num_bytes  = htonl(state->num_bytes),
    };
    comm_session_send_buffer_write(sb, (const uint8_t *) &rsp, sizeof(rsp));
    state->sent_header = true;
  }
  comm_session_send_buffer_end_write(sb);

  if (state->storage.current_offset >= state->num_bytes) {
    prv_gather_and_record_stats(state);

    // If all done, mark the image as "read" and free up our state structure
    gb_storage_cleanup(&state->storage, true /* successful  */);
    kernel_free(state);

    s_get_bytes_in_progress = false;
    prv_put_status_event(DebugInfoStateFinished);
    comm_session_set_responsiveness(state->session, BtConsumerPpGetBytes, ResponseTimeMax, 0);
    return;
  } else {
    comm_session_set_responsiveness(state->session, BtConsumerPpGetBytes, ResponseTimeMin,
                                    MIN_LATENCY_MODE_TIMEOUT_CD_SECS);
  }

  system_task_add_callback(prv_protocol_send_next_chunk, state);
  return;

cleanup:
  gb_storage_cleanup(&state->storage, false /* unsuccessful  */);
  kernel_free(state);
}

// -----------------------------------------------------------------------------------------------

//! Sets up the storage for the given GetBytes command. Will return whether the command
//! was successful.
bool prv_setup_state_for_command(GetBytesCmd cmd, GetBytesState *state,
                                 const uint8_t* data, uint32_t len) {
  GetBytesStorageInfo info = { 0 };

  switch (cmd) {
    case GET_BYTES_CMD_GET_NEW_COREDUMP:
      info.only_get_new_coredump = true;
      /* FALLTHRU */
    case GET_BYTES_CMD_GET_COREDUMP:
      state->object_type = GetBytesObjectCoredump;
      return gb_storage_setup(&state->storage, state->object_type, &info);

// if we are on a release build, don't allow the user to retrieve files or read the flash
#ifndef RELEASE
    case GET_BYTES_CMD_GET_FILE: {
      state->object_type = GetBytesObjectFile;
      // copy the filename
      GetBytesFileHeader *hdr = (GetBytesFileHeader *)data;
      if ((hdr->filename_len + sizeof(GetBytesFileHeader) + 1) < len) {
        PBL_LOG(LOG_LEVEL_ERROR, "Filename len does not match message length %d",
                hdr->filename_len);
        return false;
      }
      if (hdr->filename[hdr->filename_len] != '\0') {
        PBL_LOG(LOG_LEVEL_ERROR, "Non NULL terminated filename");
        return false;
      }
      info.filename = hdr->filename;
      bool rv = gb_storage_setup(&state->storage, state->object_type, &info);
      return rv;
    }
    case GET_BYTES_CMD_GET_FLASH: {
      state->object_type = GetBytesObjectFlash;
      GetBytesFlashHeader *hdr = (GetBytesFlashHeader *)data;
      info.flash_start_addr = ntohl(hdr->start_addr);
      info.flash_len = ntohl(hdr->len);
      PBL_LOG(LOG_LEVEL_DEBUG, "Fetching %d bytes starting at %d", (int)info.flash_len,
              (int)info.flash_start_addr);
      bool rv = gb_storage_setup(&state->storage, state->object_type, &info);
      return rv;
    }
#endif
    default:
      // NYI
      return false;
  }
}

void get_bytes_protocol_msg_callback(CommSession *session, const uint8_t* msg_data,
                                    uint32_t msg_len) {
  // at least have a cmd and a transaction_id
  if (msg_len < 2) {
    PBL_LOG(LOG_LEVEL_ERROR, "Invalid length %"PRIu32, msg_len);
    prv_protocol_send_err_response(session, 0 /*transaction_id*/,
                                         GET_BYTES_MALFORMED_COMMAND);
    return;
  }

  GetBytesHeader *hdr = (GetBytesHeader *)msg_data;
  switch (hdr->cmd_id) {
    case GET_BYTES_CMD_GET_COREDUMP:
    case GET_BYTES_CMD_GET_FILE:
    case GET_BYTES_CMD_GET_FLASH:
    case GET_BYTES_CMD_GET_NEW_COREDUMP:
      break;
    default:
      PBL_LOG(LOG_LEVEL_ERROR, "first byte can't be %u", hdr->cmd_id);
      prv_protocol_send_err_response(session, hdr->transaction_id,
                                         GET_BYTES_MALFORMED_COMMAND);
      return;
  }

  if (s_get_bytes_in_progress == true) {
    PBL_LOG(LOG_LEVEL_ERROR, "already in progress.");
    prv_protocol_send_err_response(session, hdr->transaction_id,
                                         GET_BYTES_ALREADY_IN_PROGRESS);
    return;
  }

  GetBytesState* state = kernel_zalloc_check(sizeof(*state));
  *state = (GetBytesState) {
    .session = session,
    .transaction_id = hdr->transaction_id,
  };

  // let the other types of pushes setup the state.
  if (!prv_setup_state_for_command(hdr->cmd_id, state, msg_data, msg_len)) {
    kernel_free(state);
    prv_protocol_send_err_response(session, hdr->transaction_id, GET_BYTES_MALFORMED_COMMAND);
    return;
  }

  s_get_bytes_in_progress = true;
  prv_put_status_event(DebugInfoStateStarted);
  state->start_ticks = rtc_get_ticks();
  bt_driver_analytics_get_conn_event_stats(&state->conn_event_stats);

  prv_protocol_send_next_chunk(state);
}
