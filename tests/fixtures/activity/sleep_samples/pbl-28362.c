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




// ----------------------------------------------------------------
// Sample captured at: 2015-10-26 08:20:00 local, 2015-10-26 12:20:00 GMT
// This sample show should 0 minutes of total sleep.
// But, the algorithm things the time from 8:36am to 9:50am (offset 135 to 209)
// is sleep time. Not sure how to reliably distinguish this from real sleep because there
// is very little activity, and it looks like just sleep.
AlgDlsMinuteData *activity_sample_2015_10_26_08_20_00(int *len) {
  // The unit tests parse the //> TEST_.* lines below for test values
  //> TEST_NAME pbl_28362
  //> TEST_VERSION 2
  //> TEST_TOTAL 0
  //> TEST_TOTAL_MIN 0
  //> TEST_TOTAL_MAX 80         // Fudged: see note above
  //> TEST_DEEP -1
  //> TEST_DEEP_MIN -1
  //> TEST_DEEP_MAX -1
  //> TEST_START_AT -1
  //> TEST_START_AT_MIN -1
  //> TEST_START_AT_MAX -1
  //> TEST_END_AT -1
  //> TEST_END_AT_MIN -1
  //> TEST_END_AT_MAX -1
  //> TEST_CUR_STATE_ELAPSED -1
  //> TEST_CUR_STATE_ELAPSED_MIN -1
  //> TEST_CUR_STATE_ELAPSED_MAX -1
  //> TEST_IN_SLEEP 0
  //> TEST_IN_SLEEP_MIN 0
  //> TEST_IN_SLEEP_MAX 0
  //> TEST_IN_DEEP_SLEEP 0
  //> TEST_IN_DEEP_SLEEP_MIN 0
  //> TEST_IN_DEEP_SLEEP_MAX 0
  //> TEST_WEIGHT 1.0

  // list of: {steps, orientation, vmc, light}
  static AlgDlsMinuteData samples[] = {
    // 0: Local time: 06:21:00 AM
    { 0, 0x75, 12639, 206},
    { 20, 0x69, 7709, 196},
    { 38, 0x6a, 13010, 197},
    { 35, 0x68, 15803, 197},
    { 40, 0x68, 17934, 197},
    { 50, 0x78, 15895, 215},
    { 43, 0x64, 12642, 164},
    { 0, 0x43, 7805, 181},
    { 16, 0x43, 9946, 185},
    { 0, 0x33, 6711, 179},
    { 0, 0x42, 7635, 176},
    { 10, 0x33, 11773, 148},
    { 16, 0x34, 8994, 145},
    { 0, 0x44, 5644, 148},
    { 0, 0x34, 2970, 146},
    // 15: Local time: 06:36:00 AM
    { 0, 0x34, 278, 153},
    { 8, 0x45, 2440, 151},
    { 0, 0x35, 2634, 145},
    { 0, 0x55, 1934, 147},
    { 9, 0x45, 3178, 137},
    { 0, 0x35, 1280, 156},
    { 0, 0x45, 562, 150},
    { 0, 0x45, 967, 154},
    { 0, 0x54, 1369, 148},
    { 0, 0x65, 1386, 143},
    { 0, 0x65, 562, 147},
    { 0, 0x75, 812, 147},
    { 0, 0x45, 1238, 145},
    { 10, 0x45, 3315, 152},
    { 46, 0x42, 7199, 158},
    // 30: Local time: 06:51:00 AM
    { 109, 0x41, 7346, 148},
    { 116, 0x43, 4685, 148},
    { 114, 0x43, 4665, 150},
    { 71, 0x44, 4451, 160},
    { 61, 0x43, 4611, 151},
    { 95, 0x43, 3438, 154},
    { 3, 0x44, 1490, 150},
    { 0, 0x44, 96, 149},
    { 0, 0x44, 1085, 148},
    { 0, 0x54, 0, 145},
    { 8, 0x43, 1247, 154},
    { 15, 0x24, 3806, 151},
    { 0, 0x44, 2168, 161},
    { 0, 0x44, 101, 153},
    { 0, 0x45, 5281, 167},
    // 45: Local time: 07:06:00 AM
    { 0, 0x45, 3107, 193},
    { 0, 0x65, 3033, 213},
    { 12, 0x65, 1175, 214},
    { 0, 0x65, 68, 214},
    { 0, 0x55, 419, 192},
    { 0, 0x65, 612, 213},
    { 0, 0x65, 779, 208},
    { 11, 0x65, 839, 206},
    { 0, 0x65, 481, 187},
    { 0, 0x65, 364, 211},
    { 0, 0x75, 96, 214},
    { 0, 0x65, 456, 213},
    { 0, 0x65, 246, 212},
    { 0, 0x46, 736, 189},
    { 8, 0x46, 896, 191},
    // 60: Local time: 07:21:00 AM
    { 21, 0x55, 1330, 214},
    { 0, 0x65, 258, 211},
    { 0, 0x65, 153, 214},
    { 0, 0x76, 1602, 214},
    { 0, 0x65, 1216, 212},
    { 0, 0x65, 308, 213},
    { 0, 0x65, 235, 212},
    { 0, 0x65, 65, 212},
    { 0, 0x75, 189, 211},
    { 0, 0x65, 53, 212},
    { 0, 0x65, 20, 212},
    { 0, 0x65, 29, 212},
    { 0, 0x66, 1154, 213},
    { 0, 0x66, 259, 214},
    { 0, 0x66, 87, 210},
    // 75: Local time: 07:36:00 AM
    { 0, 0x66, 56, 212},
    { 0, 0x57, 3813, 193},
    { 9, 0x65, 4973, 212},
    { 0, 0x46, 8622, 161},
    { 26, 0x42, 5735, 186},
    { 79, 0x42, 8500, 184},
    { 113, 0x42, 5296, 177},
    { 120, 0x33, 5074, 175},
    { 120, 0x32, 5317, 172},
    { 119, 0x33, 5103, 152},
    { 77, 0x22, 3209, 177},
    { 120, 0x33, 4881, 178},
    { 119, 0x33, 4971, 177},
    { 29, 0x33, 1244, 165},
    { 15, 0x43, 3238, 163},
    // 90: Local time: 07:51:00 AM
    { 29, 0x42, 8421, 189},
    { 0, 0x45, 3352, 188},
    { 0, 0x45, 2240, 184},
    { 0, 0x45, 1442, 183},
    { 0, 0x45, 1068, 186},
    { 0, 0x45, 3272, 187},
    { 0, 0x46, 4770, 185},
    { 0, 0x46, 2758, 186},
    { 0, 0x45, 108, 186},
    { 37, 0x46, 3250, 182},
    { 12, 0x46, 3285, 187},
    { 0, 0x45, 101, 191},
    { 0, 0x35, 1349, 192},
    { 0, 0x44, 1337, 196},
    { 0, 0x44, 1, 191},
    // 105: Local time: 08:06:00 AM
    { 0, 0x44, 55, 195},
    { 0, 0x44, 5, 195},
    { 0, 0x44, 6, 195},
    { 0, 0x44, 122, 196},
    { 0, 0x44, 6, 199},
    { 0, 0x44, 101, 196},
    { 0, 0x44, 242, 193},
    { 0, 0x44, 1447, 205},
    { 13, 0x46, 7986, 182},
    { 91, 0x34, 4088, 189},
    { 80, 0x34, 7867, 182},
    { 111, 0x33, 4520, 167},
    { 103, 0x33, 4080, 182},
    { 107, 0x33, 3964, 189},
    { 59, 0x33, 2847, 156},
    // 120: Local time: 08:21:00 AM
    { 32, 0x33, 1550, 162},
    { 12, 0x44, 3255, 159},
    { 0, 0x44, 1421, 157},
    { 0, 0x44, 2686, 167},
    { 0, 0x34, 2992, 162},
    { 23, 0x34, 1756, 161},
    { 0, 0x45, 3381, 186},
    { 90, 0x35, 1495, 187},
    { 90, 0x35, 1590, 187},
    { 99, 0x35, 1741, 180},
    { 72, 0x43, 3314, 201},
    { 97, 0x43, 4832, 177},
    { 28, 0x46, 3322, 172},
    { 0, 0x8b, 0, 172},
    { 0, 0x8b, 0, 175},

    // 135: Local time: 08:36:00 AM
    // Plugging in 10 minutes of missing data from a reboot here
    { 0, 0x0, 0, 0},
    { 0, 0x0, 0, 0},
    { 0, 0x0, 0, 0},
    { 0, 0x0, 0, 0},
    { 0, 0x0, 0, 0},
    { 0, 0x0, 0, 0},
    { 0, 0x0, 0, 0},
    { 0, 0x0, 0, 0},
    { 0, 0x0, 0, 0},
    { 0, 0x0, 0, 0},

    // 145 + 0: Local time: 08:46:00 AM
    { 0, 0x7a, 502, 196},
    { 0, 0x7b, 449, 206},
    { 0, 0x8b, 0, 206},
    { 0, 0x8b, 617, 205},
    { 0, 0x8b, 0, 202},
    { 0, 0x8b, 0, 205},
    { 0, 0x8b, 0, 205},
    { 0, 0x8b, 0, 205},
    { 0, 0x8b, 89, 207},
    { 0, 0x8b, 0, 207},
    { 0, 0x8b, 0, 208},
    { 0, 0x8b, 0, 207},
    { 0, 0x8b, 0, 208},
    { 0, 0x8b, 0, 208},
    { 0, 0x8b, 0, 207},
    // 160: Local time: 09:01:00 AM
    { 0, 0x8b, 0, 207},
    { 0, 0x8b, 0, 208},
    { 0, 0x8b, 0, 207},
    { 0, 0x8b, 32, 207},
    { 0, 0x8b, 0, 207},
    { 0, 0x8b, 0, 207},
    { 0, 0x8b, 1094, 221},
    { 0, 0x8a, 0, 166},
    { 0, 0x8b, 1736, 216},
    { 0, 0x8b, 629, 224},
    { 0, 0x8b, 0, 224},
    { 0, 0x8b, 0, 225},
    { 0, 0x8b, 0, 225},
    { 0, 0x8b, 3, 229},
    { 0, 0x8b, 0, 229},
    // 175: Local time: 09:16:00 AM
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 229},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    // 190: Local time: 09:31:00 AM
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    // 205: Local time: 09:46:00 AM
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 228},
    { 0, 0x8b, 0, 227},
    { 0, 0x8c, 5150, 210},
    { 0, 0x8f, 1058, 197},
    { 0, 0x65, 2866, 201},
    { 0, 0x74, 412, 205},
    { 0, 0x74, 258, 207},
    { 0, 0x74, 228, 210},
    { 0, 0x74, 1414, 191},
    { 0, 0x64, 309, 211},
    { 0, 0x73, 82, 213},
    { 0, 0x74, 483, 208},
    { 8, 0x74, 1755, 209},
    // 220: Local time: 10:01:00 AM
    { 0, 0x62, 3121, 209},
    { 48, 0x26, 3257, 182},
    { 39, 0x52, 6876, 207},
    { 15, 0x31, 8605, 184},
    { 8, 0x43, 1880, 178},
    { 0, 0x43, 3, 174},
    { 0, 0x32, 1134, 176},
    { 0, 0x32, 418, 173},
    { 0, 0x53, 1767, 184},
    { 0, 0x33, 2175, 181},
    { 0, 0x23, 1559, 181},
    { 0, 0x23, 1658, 180},
    { 0, 0x33, 1443, 181},
    { 0, 0x13, 629, 187},
    { 0, 0x53, 373, 180},
    // 235: Local time: 10:16:00 AM
    { 0, 0x54, 154, 182},
    { 0, 0x53, 1469, 183},
    { 0, 0x53, 2073, 186},
    { 0, 0x63, 1, 185},
    { 0, 0x63, 196, 191},
    { 0, 0x63, 0, 192},
    { 0, 0x63, 1, 191},
    { 0, 0x63, 0, 191},
    { 0, 0x54, 2484, 180},
    { 0, 0x54, 1896, 179},
    { 7, 0x44, 772, 176},
    { 0, 0x44, 266, 181},
    { 0, 0x64, 1672, 183},
    { 0, 0x54, 29, 183},
    { 0, 0x54, 0, 179},
    // 250: Local time: 10:31:00 AM
    { 0, 0x54, 0, 183},
    { 0, 0x44, 196, 191},
    { 0, 0x43, 910, 188},
    { 0, 0x43, 13, 185},
    { 0, 0x44, 56, 179},
    { 9, 0x63, 1731, 190},
    { 0, 0x53, 2419, 180},
    { 0, 0x54, 1741, 182},
    { 0, 0x53, 2018, 154},
    { 0, 0x34, 1641, 166},
    { 0, 0x54, 1443, 182},
    { 0, 0x54, 1295, 180},
    { 0, 0x53, 3694, 183},
    { 0, 0x53, 1732, 168},
    { 0, 0x53, 4424, 183},
    // 265: Local time: 10:46:00 AM
    { 0, 0x53, 1457, 181},
    { 0, 0x43, 2114, 181},
    { 0, 0x33, 1901, 175},
    { 0, 0x43, 1025, 184},
    { 0, 0x52, 1097, 192},
    { 0, 0x63, 2350, 190},
    { 0, 0x62, 357, 182},
    { 0, 0x43, 905, 181},
    { 0, 0x43, 722, 182},
    { 8, 0x43, 6573, 185},
    { 0, 0x53, 4572, 191},
    { 0, 0x63, 1115, 193},
    { 0, 0x63, 1595, 183},
    { 0, 0x53, 454, 180},
    { 0, 0x53, 1710, 191},
    // 280: Local time: 11:01:00 AM
    { 12, 0x53, 3534, 191},
    { 0, 0x63, 1822, 190},
    { 0, 0x65, 946, 182},
    { 8, 0x46, 2015, 179},
    { 0, 0x0, 0, 180},
    { 0, 0x43, 2037, 194},
    { 45, 0x13, 3383, 191},
    { 0, 0x54, 3383, 184},
    { 7, 0x64, 2056, 185},
    { 0, 0x53, 104, 181},
    { 0, 0x53, 0, 184},
    { 0, 0x53, 0, 185},
    { 0, 0x12, 3131, 152},
    { 0, 0x13, 2028, 173},
    { 0, 0x64, 2343, 183},
    // 295: Local time: 11:15:00 AM
    { 0, 0x64, 1842, 176},
    { 0, 0x65, 1973, 172},
    { 0, 0x53, 2180, 198},
    { 9, 0x52, 9963, 193},
    { 0, 0x33, 2087, 184},
    { 0, 0x43, 1235, 183},
    { 0, 0x53, 247, 185},
    { 0, 0x53, 3202, 183},
    { 0, 0x53, 2569, 186},
    { 0, 0x53, 1904, 180},
    { 0, 0x53, 3305, 177},
    { 0, 0x24, 1486, 169},
    { 0, 0x23, 1846, 185},
    { 0, 0x53, 48, 185},
    { 0, 0x53, 99, 181},
    // 310: Local time: 11:30:00 AM
    { 0, 0x53, 899, 173},
    { 0, 0x11, 240, 175},
    { 0, 0x3, 1435, 171},
    { 0, 0x14, 855, 193},
    { 0, 0x33, 1930, 171},
    { 0, 0x33, 17, 184},
    { 0, 0x32, 2288, 191},
    { 12, 0x45, 2911, 178},
    { 0, 0x62, 318, 175},
    { 0, 0x42, 1128, 175},
    { 0, 0x32, 607, 178},
    { 0, 0x42, 633, 183},
    { 0, 0x62, 328, 185},
    { 0, 0x52, 641, 180},
    { 0, 0x52, 628, 179},
    // 325: Local time: 11:45:00 AM
    { 0, 0x52, 240, 188},
    { 0, 0x52, 323, 178},
    { 0, 0x52, 486, 180},
    { 0, 0x52, 989, 172},
    { 0, 0x42, 1710, 172},
    { 0, 0x42, 1574, 178},
    { 0, 0x52, 6, 177},
    { 0, 0x52, 533, 173},
    { 0, 0x52, 27, 173},
    { 0, 0x52, 509, 171},
    { 0, 0x52, 232, 173},
    { 0, 0x44, 1756, 181},
    { 0, 0x52, 1698, 174},
    { 0, 0x52, 70, 188},
    { 0, 0x53, 808, 173},
  };
  *len = ARRAY_LENGTH(samples);
  return samples;
}

