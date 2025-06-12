/****************************************************************
 * @file stk_alps_lib.h
 *
 * stk_alps_lib algorithm.
 *
 * Copyright (c) 2016-2022 Sensortek Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - sensortek Technologies, Inc.
****************************************************************/

#ifndef _STK_ALPS_LIB_H_
#define _STK_ALPS_LIB_H_

// #include <linux/types.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct FloatParameter {
    float ParameterF;
//    float ParameterR;
    float ParameterG;
//    float ParameterB;
//    float ParameterC;
} FloatParameter;

typedef struct IntParameter
{
    int32_t ParameterF;
//    int32_t ParameterR;
    int32_t ParameterG;
//    int32_t ParameterB;
//    int32_t ParameterC;
} IntParameter;

typedef struct ChannelData
{
    uint32_t ChannelF;
//    uint32_t ChannelR;
    uint32_t ChannelG;
//    uint32_t ChannelB;
//    uint32_t ChannelC;
} ChannelData;

typedef struct ChannelDataSet
{
    uint32_t* ChannelF;
//    uint32_t* ChannelR;
    uint32_t* ChannelG;
//    uint32_t* ChannelB;
//    uint32_t* ChannelC;
    uint16_t Size;
} ChannelDataSet;

typedef struct CNode
{
    FloatParameter Alpha;
    FloatParameter Beta;
    FloatParameter Bias;
    ChannelData HighTHD;
    ChannelData LowTHD;
    uint16_t Brightness;
} CNode;

typedef struct AlgoParam
{
    CNode DisplayNode[1];
    float luxBase;
    float luxCode[8];
    float DebouncePct;
    uint8_t DebounceCount;
} AlgoParam;

typedef enum AlgoStatus
{
    ALGO_OK = 0x00,
    ALGO_ERROR = 0x01,
    ALGO_INVALID_DATA = 0x02,
    ALGO_INVALID_PARAMETER = 0x03,
    ALGO_INVALID_OPERATION = 0x04,
    ALGO_OVER_THRESHOLD = 0x05,
    ALGO_UNDER_THRESHOLD = 0x06
} AlgoStatus;

#ifdef __cplusplus
extern "C"
{
#endif
    uint32_t STK_getVersion(void);
    uint32_t STK_getBuildDate(void);
    void STK_initAlgo(AlgoParam *info_k);
    void STK_deInitAlgo(void);
    void STK_resetAlgo(void);
    float STK_calcAmbientLuxbyFIFO(ChannelDataSet* fifo, uint16_t brightness, bool enableFilter);
    float STK_calcAmbientLuxbyMaxMin(ChannelData* max, ChannelData* min, uint16_t brightness, bool enableFilter);
    AlgoStatus STK_calcAmbientInfobyFIFO(ChannelDataSet* fifo, ChannelData* ambient, uint16_t brightness, bool enableFilter);
    AlgoStatus STK_calcAmbientInfobyMaxMin(ChannelData* max, ChannelData* min, ChannelData* ambient, uint16_t brightness, bool enableFilter);
    AlgoStatus STK_calcAmbientInfobyFIFOSet(ChannelDataSet* fifo, uint8_t sectionSize, uint8_t dataSize, ChannelData* ambient, uint16_t brightness, bool enableFilter);
    AlgoStatus STK_calcAmbientInfobyMaxMinSet(ChannelData* max, ChannelData* min, ChannelData* ambient, uint16_t brightness, bool enableFilter);
    void STK_extrFeaturebyFIFOSet(ChannelDataSet* fifo, uint8_t sectionSize, uint8_t dataSize, ChannelData* max, ChannelData* min);
    void STK_outputDebugInfo(ChannelDataSet* fifo, ChannelData* max, ChannelData* min, ChannelData* mid);

#ifdef __cplusplus
}
#endif
#endif
