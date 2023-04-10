// Copyright 2020 The libgav1 Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/film_grain.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

#include "src/dsp/common.h"
#include "src/dsp/constants.h"
#include "src/dsp/dsp.h"
#include "src/dsp/film_grain_common.h"
#include "src/utils/array_2d.h"
#include "src/utils/blocking_counter.h"
#include "src/utils/common.h"
#include "src/utils/compiler_attributes.h"
#include "src/utils/constants.h"
#include "src/utils/logging.h"
#include "src/utils/threadpool.h"

namespace libgav1 {

namespace {

// The kGaussianSequence array contains random samples from a Gaussian
// distribution with zero mean and standard deviation of about 512 clipped to
// the range of [-2048, 2047] (representable by a signed integer using 12 bits
// of precision) and rounded to the nearest multiple of 4.
//
// Note: It is important that every element in the kGaussianSequence array be
// less than 2040, so that RightShiftWithRounding(kGaussianSequence[i], 4) is
// less than 128 for bitdepth=8 (GrainType=int8_t).
constexpr int16_t kGaussianSequence[/*2048*/] = {
    56,    568,   -180,  172,   124,   -84,   172,   -64,   -900,  24,   820,
    224,   1248,  996,   272,   -8,    -916,  -388,  -732,  -104,  -188, 800,
    112,   -652,  -320,  -376,  140,   -252,  492,   -168,  44,    -788, 588,
    -584,  500,   -228,  12,    680,   272,   -476,  972,   -100,  652,  368,
    432,   -196,  -720,  -192,  1000,  -332,  652,   -136,  -552,  -604, -4,
    192,   -220,  -136,  1000,  -52,   372,   -96,   -624,  124,   -24,  396,
    540,   -12,   -104,  640,   464,   244,   -208,  -84,   368,   -528, -740,
    248,   -968,  -848,  608,   376,   -60,   -292,  -40,   -156,  252,  -292,
    248,   224,   -280,  400,   -244,  244,   -60,   76,    -80,   212,  532,
    340,   128,   -36,   824,   -352,  -60,   -264,  -96,   -612,  416,  -704,
    220,   -204,  640,   -160,  1220,  -408,  900,   336,   20,    -336, -96,
    -792,  304,   48,    -28,   -1232, -1172, -448,  104,   -292,  -520, 244,
    60,    -948,  0,     -708,  268,   108,   356,   -548,  488,   -344, -136,
    488,   -196,  -224,  656,   -236,  -1128, 60,    4,     140,   276,  -676,
    -376,  168,   -108,  464,   8,     564,   64,    240,   308,   -300, -400,
    -456,  -136,  56,    120,   -408,  -116,  436,   504,   -232,  328,  844,
    -164,  -84,   784,   -168,  232,   -224,  348,   -376,  128,   568,  96,
    -1244, -288,  276,   848,   832,   -360,  656,   464,   -384,  -332, -356,
    728,   -388,  160,   -192,  468,   296,   224,   140,   -776,  -100, 280,
    4,     196,   44,    -36,   -648,  932,   16,    1428,  28,    528,  808,
    772,   20,    268,   88,    -332,  -284,  124,   -384,  -448,  208,  -228,
    -1044, -328,  660,   380,   -148,  -300,  588,   240,   540,   28,   136,
    -88,   -436,  256,   296,   -1000, 1400,  0,     -48,   1056,  -136, 264,
    -528,  -1108, 632,   -484,  -592,  -344,  796,   124,   -668,  -768, 388,
    1296,  -232,  -188,  -200,  -288,  -4,    308,   100,   -168,  256,  -500,
    204,   -508,  648,   -136,  372,   -272,  -120,  -1004, -552,  -548, -384,
    548,   -296,  428,   -108,  -8,    -912,  -324,  -224,  -88,   -112, -220,
    -100,  996,   -796,  548,   360,   -216,  180,   428,   -200,  -212, 148,
    96,    148,   284,   216,   -412,  -320,  120,   -300,  -384,  -604, -572,
    -332,  -8,    -180,  -176,  696,   116,   -88,   628,   76,    44,   -516,
    240,   -208,  -40,   100,   -592,  344,   -308,  -452,  -228,  20,   916,
    -1752, -136,  -340,  -804,  140,   40,    512,   340,   248,   184,  -492,
    896,   -156,  932,   -628,  328,   -688,  -448,  -616,  -752,  -100, 560,
    -1020, 180,   -800,  -64,   76,    576,   1068,  396,   660,   552,  -108,
    -28,   320,   -628,  312,   -92,   -92,   -472,  268,   16,    560,  516,
    -672,  -52,   492,   -100,  260,   384,   284,   292,   304,   -148, 88,
    -152,  1012,  1064,  -228,  164,   -376,  -684,  592,   -392,  156,  196,
    -524,  -64,   -884,  160,   -176,  636,   648,   404,   -396,  -436, 864,
    424,   -728,  988,   -604,  904,   -592,  296,   -224,  536,   -176, -920,
    436,   -48,   1176,  -884,  416,   -776,  -824,  -884,  524,   -548, -564,
    -68,   -164,  -96,   692,   364,   -692,  -1012, -68,   260,   -480, 876,
    -1116, 452,   -332,  -352,  892,   -1088, 1220,  -676,  12,    -292, 244,
    496,   372,   -32,   280,   200,   112,   -440,  -96,   24,    -644, -184,
    56,    -432,  224,   -980,  272,   -260,  144,   -436,  420,   356,  364,
    -528,  76,    172,   -744,  -368,  404,   -752,  -416,  684,   -688, 72,
    540,   416,   92,    444,   480,   -72,   -1416, 164,   -1172, -68,  24,
    424,   264,   1040,  128,   -912,  -524,  -356,  64,    876,   -12,  4,
    -88,   532,   272,   -524,  320,   276,   -508,  940,   24,    -400, -120,
    756,   60,    236,   -412,  100,   376,   -484,  400,   -100,  -740, -108,
    -260,  328,   -268,  224,   -200,  -416,  184,   -604,  -564,  -20,  296,
    60,    892,   -888,  60,    164,   68,    -760,  216,   -296,  904,  -336,
    -28,   404,   -356,  -568,  -208,  -1480, -512,  296,   328,   -360, -164,
    -1560, -776,  1156,  -428,  164,   -504,  -112,  120,   -216,  -148, -264,
    308,   32,    64,    -72,   72,    116,   176,   -64,   -272,  460,  -536,
    -784,  -280,  348,   108,   -752,  -132,  524,   -540,  -776,  116,  -296,
    -1196, -288,  -560,  1040,  -472,  116,   -848,  -1116, 116,   636,  696,
    284,   -176,  1016,  204,   -864,  -648,  -248,  356,   972,   -584, -204,
    264,   880,   528,   -24,   -184,  116,   448,   -144,  828,   524,  212,
    -212,  52,    12,    200,   268,   -488,  -404,  -880,  824,   -672, -40,
    908,   -248,  500,   716,   -576,  492,   -576,  16,    720,   -108, 384,
    124,   344,   280,   576,   -500,  252,   104,   -308,  196,   -188, -8,
    1268,  296,   1032,  -1196, 436,   316,   372,   -432,  -200,  -660, 704,
    -224,  596,   -132,  268,   32,    -452,  884,   104,   -1008, 424,  -1348,
    -280,  4,     -1168, 368,   476,   696,   300,   -8,    24,    180,  -592,
    -196,  388,   304,   500,   724,   -160,  244,   -84,   272,   -256, -420,
    320,   208,   -144,  -156,  156,   364,   452,   28,    540,   316,  220,
    -644,  -248,  464,   72,    360,   32,    -388,  496,   -680,  -48,  208,
    -116,  -408,  60,    -604,  -392,  548,   -840,  784,   -460,  656,  -544,
    -388,  -264,  908,   -800,  -628,  -612,  -568,  572,   -220,  164,  288,
    -16,   -308,  308,   -112,  -636,  -760,  280,   -668,  432,   364,  240,
    -196,  604,   340,   384,   196,   592,   -44,   -500,  432,   -580, -132,
    636,   -76,   392,   4,     -412,  540,   508,   328,   -356,  -36,  16,
    -220,  -64,   -248,  -60,   24,    -192,  368,   1040,  92,    -24,  -1044,
    -32,   40,    104,   148,   192,   -136,  -520,  56,    -816,  -224, 732,
    392,   356,   212,   -80,   -424,  -1008, -324,  588,   -1496, 576,  460,
    -816,  -848,  56,    -580,  -92,   -1372, -112,  -496,  200,   364,  52,
    -140,  48,    -48,   -60,   84,    72,    40,    132,   -356,  -268, -104,
    -284,  -404,  732,   -520,  164,   -304,  -540,  120,   328,   -76,  -460,
    756,   388,   588,   236,   -436,  -72,   -176,  -404,  -316,  -148, 716,
    -604,  404,   -72,   -88,   -888,  -68,   944,   88,    -220,  -344, 960,
    472,   460,   -232,  704,   120,   832,   -228,  692,   -508,  132,  -476,
    844,   -748,  -364,  -44,   1116,  -1104, -1056, 76,    428,   552,  -692,
    60,    356,   96,    -384,  -188,  -612,  -576,  736,   508,   892,  352,
    -1132, 504,   -24,   -352,  324,   332,   -600,  -312,  292,   508,  -144,
    -8,    484,   48,    284,   -260,  -240,  256,   -100,  -292,  -204, -44,
    472,   -204,  908,   -188,  -1000, -256,  92,    1164,  -392,  564,  356,
    652,   -28,   -884,  256,   484,   -192,  760,   -176,  376,   -524, -452,
    -436,  860,   -736,  212,   124,   504,   -476,  468,   76,    -472, 552,
    -692,  -944,  -620,  740,   -240,  400,   132,   20,    192,   -196, 264,
    -668,  -1012, -60,   296,   -316,  -828,  76,    -156,  284,   -768, -448,
    -832,  148,   248,   652,   616,   1236,  288,   -328,  -400,  -124, 588,
    220,   520,   -696,  1032,  768,   -740,  -92,   -272,  296,   448,  -464,
    412,   -200,  392,   440,   -200,  264,   -152,  -260,  320,   1032, 216,
    320,   -8,    -64,   156,   -1016, 1084,  1172,  536,   484,   -432, 132,
    372,   -52,   -256,  84,    116,   -352,  48,    116,   304,   -384, 412,
    924,   -300,  528,   628,   180,   648,   44,    -980,  -220,  1320, 48,
    332,   748,   524,   -268,  -720,  540,   -276,  564,   -344,  -208, -196,
    436,   896,   88,    -392,  132,   80,    -964,  -288,  568,   56,   -48,
    -456,  888,   8,     552,   -156,  -292,  948,   288,   128,   -716, -292,
    1192,  -152,  876,   352,   -600,  -260,  -812,  -468,  -28,   -120, -32,
    -44,   1284,  496,   192,   464,   312,   -76,   -516,  -380,  -456, -1012,
    -48,   308,   -156,  36,    492,   -156,  -808,  188,   1652,  68,   -120,
    -116,  316,   160,   -140,  352,   808,   -416,  592,   316,   -480, 56,
    528,   -204,  -568,  372,   -232,  752,   -344,  744,   -4,    324,  -416,
    -600,  768,   268,   -248,  -88,   -132,  -420,  -432,  80,    -288, 404,
    -316,  -1216, -588,  520,   -108,  92,    -320,  368,   -480,  -216, -92,
    1688,  -300,  180,   1020,  -176,  820,   -68,   -228,  -260,  436,  -904,
    20,    40,    -508,  440,   -736,  312,   332,   204,   760,   -372, 728,
    96,    -20,   -632,  -520,  -560,  336,   1076,  -64,   -532,  776,  584,
    192,   396,   -728,  -520,  276,   -188,  80,    -52,   -612,  -252, -48,
    648,   212,   -688,  228,   -52,   -260,  428,   -412,  -272,  -404, 180,
    816,   -796,  48,    152,   484,   -88,   -216,  988,   696,   188,  -528,
    648,   -116,  -180,  316,   476,   12,    -564,  96,    476,   -252, -364,
    -376,  -392,  556,   -256,  -576,  260,   -352,  120,   -16,   -136, -260,
    -492,  72,    556,   660,   580,   616,   772,   436,   424,   -32,  -324,
    -1268, 416,   -324,  -80,   920,   160,   228,   724,   32,    -516, 64,
    384,   68,    -128,  136,   240,   248,   -204,  -68,   252,   -932, -120,
    -480,  -628,  -84,   192,   852,   -404,  -288,  -132,  204,   100,  168,
    -68,   -196,  -868,  460,   1080,  380,   -80,   244,   0,     484,  -888,
    64,    184,   352,   600,   460,   164,   604,   -196,  320,   -64,  588,
    -184,  228,   12,    372,   48,    -848,  -344,  224,   208,   -200, 484,
    128,   -20,   272,   -468,  -840,  384,   256,   -720,  -520,  -464, -580,
    112,   -120,  644,   -356,  -208,  -608,  -528,  704,   560,   -424, 392,
    828,   40,    84,    200,   -152,  0,     -144,  584,   280,   -120, 80,
    -556,  -972,  -196,  -472,  724,   80,    168,   -32,   88,    160,  -688,
    0,     160,   356,   372,   -776,  740,   -128,  676,   -248,  -480, 4,
    -364,  96,    544,   232,   -1032, 956,   236,   356,   20,    -40,  300,
    24,    -676,  -596,  132,   1120,  -104,  532,   -1096, 568,   648,  444,
    508,   380,   188,   -376,  -604,  1488,  424,   24,    756,   -220, -192,
    716,   120,   920,   688,   168,   44,    -460,  568,   284,   1144, 1160,
    600,   424,   888,   656,   -356,  -320,  220,   316,   -176,  -724, -188,
    -816,  -628,  -348,  -228,  -380,  1012,  -452,  -660,  736,   928,  404,
    -696,  -72,   -268,  -892,  128,   184,   -344,  -780,  360,   336,  400,
    344,   428,   548,   -112,  136,   -228,  -216,  -820,  -516,  340,  92,
    -136,  116,   -300,  376,   -244,  100,   -316,  -520,  -284,  -12,  824,
    164,   -548,  -180,  -128,  116,   -924,  -828,  268,   -368,  -580, 620,
    192,   160,   0,     -1676, 1068,  424,   -56,   -360,  468,   -156, 720,
    288,   -528,  556,   -364,  548,   -148,  504,   316,   152,   -648, -620,
    -684,  -24,   -376,  -384,  -108,  -920,  -1032, 768,   180,   -264, -508,
    -1268, -260,  -60,   300,   -240,  988,   724,   -376,  -576,  -212, -736,
    556,   192,   1092,  -620,  -880,  376,   -56,   -4,    -216,  -32,  836,
    268,   396,   1332,  864,   -600,  100,   56,    -412,  -92,   356,  180,
    884,   -468,  -436,  292,   -388,  -804,  -704,  -840,  368,   -348, 140,
    -724,  1536,  940,   372,   112,   -372,  436,   -480,  1136,  296,  -32,
    -228,  132,   -48,   -220,  868,   -1016, -60,   -1044, -464,  328,  916,
    244,   12,    -736,  -296,  360,   468,   -376,  -108,  -92,   788,  368,
    -56,   544,   400,   -672,  -420,  728,   16,    320,   44,    -284, -380,
    -796,  488,   132,   204,   -596,  -372,  88,    -152,  -908,  -636, -572,
    -624,  -116,  -692,  -200,  -56,   276,   -88,   484,   -324,  948,  864,
    1000,  -456,  -184,  -276,  292,   -296,  156,   676,   320,   160,  908,
    -84,   -1236, -288,  -116,  260,   -372,  -644,  732,   -756,  -96,  84,
    344,   -520,  348,   -688,  240,   -84,   216,   -1044, -136,  -676, -396,
    -1500, 960,   -40,   176,   168,   1516,  420,   -504,  -344,  -364, -360,
    1216,  -940,  -380,  -212,  252,   -660,  -708,  484,   -444,  -152, 928,
    -120,  1112,  476,   -260,  560,   -148,  -344,  108,   -196,  228,  -288,
    504,   560,   -328,  -88,   288,   -1008, 460,   -228,  468,   -836, -196,
    76,    388,   232,   412,   -1168, -716,  -644,  756,   -172,  -356, -504,
    116,   432,   528,   48,    476,   -168,  -608,  448,   160,   -532, -272,
    28,    -676,  -12,   828,   980,   456,   520,   104,   -104,  256,  -344,
    -4,    -28,   -368,  -52,   -524,  -572,  -556,  -200,  768,   1124, -208,
    -512,  176,   232,   248,   -148,  -888,  604,   -600,  -304,  804,  -156,
    -212,  488,   -192,  -804,  -256,  368,   -360,  -916,  -328,  228,  -240,
    -448,  -472,  856,   -556,  -364,  572,   -12,   -156,  -368,  -340, 432,
    252,   -752,  -152,  288,   268,   -580,  -848,  -592,  108,   -76,  244,
    312,   -716,  592,   -80,   436,   360,   4,     -248,  160,   516,  584,
    732,   44,    -468,  -280,  -292,  -156,  -588,  28,    308,   912,  24,
    124,   156,   180,   -252,  944,   -924,  -772,  -520,  -428,  -624, 300,
    -212,  -1144, 32,    -724,  800,   -1128, -212,  -1288, -848,  180,  -416,
    440,   192,   -576,  -792,  -76,   -1080, 80,    -532,  -352,  -132, 380,
    -820,  148,   1112,  128,   164,   456,   700,   -924,  144,   -668, -384,
    648,   -832,  508,   552,   -52,   -100,  -656,  208,   -568,  748,  -88,
    680,   232,   300,   192,   -408,  -1012, -152,  -252,  -268,  272,  -876,
    -664,  -648,  -332,  -136,  16,    12,    1152,  -28,   332,   -536, 320,
    -672,  -460,  -316,  532,   -260,  228,   -40,   1052,  -816,  180,  88,
    -496,  -556,  -672,  -368,  428,   92,    356,   404,   -408,  252,  196,
    -176,  -556,  792,   268,   32,    372,   40,    96,    -332,  328,  120,
    372,   -900,  -40,   472,   -264,  -592,  952,   128,   656,   112,  664,
    -232,  420,   4,     -344,  -464,  556,   244,   -416,  -32,   252,  0,
    -412,  188,   -696,  508,   -476,  324,   -1096, 656,   -312,  560,  264,
    -136,  304,   160,   -64,   -580,  248,   336,   -720,  560,   -348, -288,
    -276,  -196,  -500,  852,   -544,  -236,  -1128, -992,  -776,  116,  56,
    52,    860,   884,   212,   -12,   168,   1020,  512,   -552,  924,  -148,
    716,   188,   164,   -340,  -520,  -184,  880,   -152,  -680,  -208, -1156,
    -300,  -528,  -472,  364,   100,   -744,  -1056, -32,   540,   280,  144,
    -676,  -32,   -232,  -280,  -224,  96,    568,   -76,   172,   148,  148,
    104,   32,    -296,  -32,   788,   -80,   32,    -16,   280,   288,  944,
    428,   -484};
static_assert(sizeof(kGaussianSequence) / sizeof(kGaussianSequence[0]) == 2048,
              "");

// The number of rows in a contiguous group computed by a single worker thread
// before checking for the next available group.
constexpr int kFrameChunkHeight = 8;

// |width| and |height| refer to the plane, not the frame, meaning any
// subsampling should be applied by the caller.
template <typename Pixel>
inline void CopyImagePlane(const uint8_t* source_plane, ptrdiff_t source_stride,
                           int width, int height, uint8_t* dest_plane,
                           ptrdiff_t dest_stride) {
  // If it's the same buffer there's nothing to do.
  if (source_plane == dest_plane) return;

  int y = 0;
  do {
    memcpy(dest_plane, source_plane, width * sizeof(Pixel));
    source_plane += source_stride;
    dest_plane += dest_stride;
  } while (++y < height);
}

}  // namespace

template <int bitdepth>
FilmGrain<bitdepth>::FilmGrain(const FilmGrainParams& params,
                               bool is_monochrome,
                               bool color_matrix_is_identity, int subsampling_x,
                               int subsampling_y, int width, int height,
                               ThreadPool* thread_pool)
    : params_(params),
      is_monochrome_(is_monochrome),
      color_matrix_is_identity_(color_matrix_is_identity),
      subsampling_x_(subsampling_x),
      subsampling_y_(subsampling_y),
      width_(width),
      height_(height),
      template_uv_width_((subsampling_x != 0) ? kMinChromaWidth
                                              : kMaxChromaWidth),
      template_uv_height_((subsampling_y != 0) ? kMinChromaHeight
                                               : kMaxChromaHeight),
      thread_pool_(thread_pool) {}

template <int bitdepth>
bool FilmGrain<bitdepth>::Init() {
  // Section 7.18.3.3. Generate grain process.
  const dsp::Dsp& dsp = *dsp::GetDspTable(bitdepth);
  // If params_.num_y_points is 0, luma_grain_ will never be read, so we don't
  // need to generate it.
  const bool use_luma = params_.num_y_points > 0;
  if (use_luma) {
    GenerateLumaGrain(params_, luma_grain_);
    // If params_.auto_regression_coeff_lag is 0, the filter is the identity
    // filter and therefore can be skipped.
    if (params_.auto_regression_coeff_lag > 0) {
      dsp.film_grain
          .luma_auto_regression[params_.auto_regression_coeff_lag - 1](
              params_, luma_grain_);
    }
  } else {
    // Have AddressSanitizer warn if luma_grain_ is used.
    ASAN_POISON_MEMORY_REGION(luma_grain_, sizeof(luma_grain_));
  }
  if (!is_monochrome_) {
    GenerateChromaGrains(params_, template_uv_width_, template_uv_height_,
                         u_grain_, v_grain_);
    if (params_.auto_regression_coeff_lag > 0 || use_luma) {
      dsp.film_grain.chroma_auto_regression[static_cast<int>(
          use_luma)][params_.auto_regression_coeff_lag](
          params_, luma_grain_, subsampling_x_, subsampling_y_, u_grain_,
          v_grain_);
    }
  }

  // Section 7.18.3.4. Scaling lookup initialization process.

  // Initialize scaling_lut_y_. If params_.num_y_points > 0, scaling_lut_y_
  // is used for the Y plane. If params_.chroma_scaling_from_luma is true,
  // scaling_lut_u_ and scaling_lut_v_ are the same as scaling_lut_y_ and are
  // set up as aliases. So we need to initialize scaling_lut_y_ under these
  // two conditions.
  //
  // Note: Although it does not seem to make sense, there are test vectors
  // with chroma_scaling_from_luma=true and params_.num_y_points=0.
#if LIBGAV1_MSAN
  // Quiet film grain / md5 msan warnings.
  memset(scaling_lut_y_, 0, sizeof(scaling_lut_y_));
#endif
  if (use_luma || params_.chroma_scaling_from_luma) {
    dsp.film_grain.initialize_scaling_lut(
        params_.num_y_points, params_.point_y_value, params_.point_y_scaling,
        scaling_lut_y_, kScalingLutLength);
  } else {
    ASAN_POISON_MEMORY_REGION(scaling_lut_y_, sizeof(scaling_lut_y_));
  }
  if (!is_monochrome_) {
    if (params_.chroma_scaling_from_luma) {
      scaling_lut_u_ = scaling_lut_y_;
      scaling_lut_v_ = scaling_lut_y_;
    } else if (params_.num_u_points > 0 || params_.num_v_points > 0) {
      const size_t buffer_size =
          kScalingLutLength * (static_cast<int>(params_.num_u_points > 0) +
                               static_cast<int>(params_.num_v_points > 0));
      scaling_lut_chroma_buffer_.reset(new (std::nothrow) int16_t[buffer_size]);
      if (scaling_lut_chroma_buffer_ == nullptr) return false;

      int16_t* buffer = scaling_lut_chroma_buffer_.get();
#if LIBGAV1_MSAN
      // Quiet film grain / md5 msan warnings.
      memset(buffer, 0, buffer_size * 2);
#endif
      if (params_.num_u_points > 0) {
        scaling_lut_u_ = buffer;
        dsp.film_grain.initialize_scaling_lut(
            params_.num_u_points, params_.point_u_value,
            params_.point_u_scaling, scaling_lut_u_, kScalingLutLength);
        buffer += kScalingLutLength;
      }
      if (params_.num_v_points > 0) {
        scaling_lut_v_ = buffer;
        dsp.film_grain.initialize_scaling_lut(
            params_.num_v_points, params_.point_v_value,
            params_.point_v_scaling, scaling_lut_v_, kScalingLutLength);
      }
    }
  }
  return true;
}

template <int bitdepth>
void FilmGrain<bitdepth>::GenerateLumaGrain(const FilmGrainParams& params,
                                            GrainType* luma_grain) {
  // If params.num_y_points is equal to 0, Section 7.18.3.3 specifies we set
  // the luma_grain array to all zeros. But the Note at the end of Section
  // 7.18.3.3 says luma_grain "will never be read in this case". So we don't
  // call GenerateLumaGrain if params.num_y_points is equal to 0.
  assert(params.num_y_points > 0);
  const int shift = kBitdepth12 - bitdepth + params.grain_scale_shift;
  uint16_t seed = params.grain_seed;
  GrainType* luma_grain_row = luma_grain;
  for (int y = 0; y < kLumaHeight; ++y) {
    for (int x = 0; x < kLumaWidth; ++x) {
      luma_grain_row[x] = RightShiftWithRounding(
          kGaussianSequence[GetFilmGrainRandomNumber(11, &seed)], shift);
    }
    luma_grain_row += kLumaWidth;
  }
}

template <int bitdepth>
void FilmGrain<bitdepth>::GenerateChromaGrains(const FilmGrainParams& params,
                                               int chroma_width,
                                               int chroma_height,
                                               GrainType* u_grain,
                                               GrainType* v_grain) {
  const int shift = kBitdepth12 - bitdepth + params.grain_scale_shift;
  if (params.num_u_points == 0 && !params.chroma_scaling_from_luma) {
    memset(u_grain, 0, chroma_height * chroma_width * sizeof(*u_grain));
  } else {
    uint16_t seed = params.grain_seed ^ 0xb524;
    GrainType* u_grain_row = u_grain;
    assert(chroma_width > 0);
    assert(chroma_height > 0);
    int y = 0;
    do {
      int x = 0;
      do {
        u_grain_row[x] = RightShiftWithRounding(
            kGaussianSequence[GetFilmGrainRandomNumber(11, &seed)], shift);
      } while (++x < chroma_width);

      u_grain_row += chroma_width;
    } while (++y < chroma_height);
  }
  if (params.num_v_points == 0 && !params.chroma_scaling_from_luma) {
    memset(v_grain, 0, chroma_height * chroma_width * sizeof(*v_grain));
  } else {
    GrainType* v_grain_row = v_grain;
    uint16_t seed = params.grain_seed ^ 0x49d8;
    int y = 0;
    do {
      int x = 0;
      do {
        v_grain_row[x] = RightShiftWithRounding(
            kGaussianSequence[GetFilmGrainRandomNumber(11, &seed)], shift);
      } while (++x < chroma_width);

      v_grain_row += chroma_width;
    } while (++y < chroma_height);
  }
}

template <int bitdepth>
bool FilmGrain<bitdepth>::AllocateNoiseStripes() {
  const int half_height = DivideBy2(height_ + 1);
  assert(half_height > 0);
  // ceil(half_height / 16.0)
  const int max_luma_num = DivideBy16(half_height + 15);
  constexpr int kNoiseStripeHeight = 34;
  size_t noise_buffer_size = kNoiseStripePadding;
  if (params_.num_y_points > 0) {
    noise_buffer_size += max_luma_num * kNoiseStripeHeight * width_;
  }
  if (!is_monochrome_) {
    noise_buffer_size += 2 * max_luma_num *
                         (kNoiseStripeHeight >> subsampling_y_) *
                         SubsampledValue(width_, subsampling_x_);
  }
  noise_buffer_.reset(new (std::nothrow) GrainType[noise_buffer_size]);
  if (noise_buffer_ == nullptr) return false;
  GrainType* noise_buffer = noise_buffer_.get();
  if (params_.num_y_points > 0) {
    noise_stripes_[kPlaneY].Reset(max_luma_num, kNoiseStripeHeight * width_,
                                  noise_buffer);
    noise_buffer += max_luma_num * kNoiseStripeHeight * width_;
  }
  if (!is_monochrome_) {
    noise_stripes_[kPlaneU].Reset(max_luma_num,
                                  (kNoiseStripeHeight >> subsampling_y_) *
                                      SubsampledValue(width_, subsampling_x_),
                                  noise_buffer);
    noise_buffer += max_luma_num * (kNoiseStripeHeight >> subsampling_y_) *
                    SubsampledValue(width_, subsampling_x_);
    noise_stripes_[kPlaneV].Reset(max_luma_num,
                                  (kNoiseStripeHeight >> subsampling_y_) *
                                      SubsampledValue(width_, subsampling_x_),
                                  noise_buffer);
  }
  return true;
}

template <int bitdepth>
bool FilmGrain<bitdepth>::AllocateNoiseImage() {
  // When LIBGAV1_MSAN is enabled, zero initialize to quiet optimized film grain
  // msan warnings.
  constexpr bool zero_initialize = LIBGAV1_MSAN == 1;
  if (params_.num_y_points > 0 &&
      !noise_image_[kPlaneY].Reset(height_, width_ + kNoiseImagePadding,
                                   zero_initialize)) {
    return false;
  }
  if (!is_monochrome_) {
    if (!noise_image_[kPlaneU].Reset(
            (height_ + subsampling_y_) >> subsampling_y_,
            ((width_ + subsampling_x_) >> subsampling_x_) + kNoiseImagePadding,
            zero_initialize)) {
      return false;
    }
    if (!noise_image_[kPlaneV].Reset(
            (height_ + subsampling_y_) >> subsampling_y_,
            ((width_ + subsampling_x_) >> subsampling_x_) + kNoiseImagePadding,
            zero_initialize)) {
      return false;
    }
  }
  return true;
}

// Uses |overlap_flag| to skip rows that are covered by the overlap computation.
template <int bitdepth>
void FilmGrain<bitdepth>::ConstructNoiseImage(
    const Array2DView<GrainType>* noise_stripes, int width, int height,
    int subsampling_x, int subsampling_y, int stripe_start_offset,
    Array2D<GrainType>* noise_image) {
  const int plane_width = (width + subsampling_x) >> subsampling_x;
  const int plane_height = (height + subsampling_y) >> subsampling_y;
  const int stripe_height = 32 >> subsampling_y;
  const int stripe_mask = stripe_height - 1;
  int y = 0;
  // |luma_num| = y >> (5 - |subsampling_y|). Hence |luma_num| == 0 for all y up
  // to either 16 or 32.
  const GrainType* first_noise_stripe = (*noise_stripes)[0];
  do {
    memcpy((*noise_image)[y], first_noise_stripe + y * plane_width,
           plane_width * sizeof(first_noise_stripe[0]));
  } while (++y < std::min(stripe_height, plane_height));
  // End special iterations for luma_num == 0.

  int luma_num = 1;
  for (; y < (plane_height & ~stripe_mask); ++luma_num, y += stripe_height) {
    const GrainType* noise_stripe = (*noise_stripes)[luma_num];
    int i = stripe_start_offset;
    do {
      memcpy((*noise_image)[y + i], noise_stripe + i * plane_width,
             plane_width * sizeof(noise_stripe[0]));
    } while (++i < stripe_height);
  }

  // If there is a partial stripe, copy any rows beyond the overlap rows.
  const int remaining_height = plane_height - y;
  if (remaining_height > stripe_start_offset) {
    assert(luma_num < noise_stripes->rows());
    const GrainType* noise_stripe = (*noise_stripes)[luma_num];
    int i = stripe_start_offset;
    do {
      memcpy((*noise_image)[y + i], noise_stripe + i * plane_width,
             plane_width * sizeof(noise_stripe[0]));
    } while (++i < remaining_height);
  }
}

template <int bitdepth>
void FilmGrain<bitdepth>::BlendNoiseChromaWorker(
    const dsp::Dsp& dsp, const Plane* planes, int num_planes,
    std::atomic<int>* job_counter, int min_value, int max_chroma,
    const uint8_t* source_plane_y, ptrdiff_t source_stride_y,
    const uint8_t* source_plane_u, const uint8_t* source_plane_v,
    ptrdiff_t source_stride_uv, uint8_t* dest_plane_u, uint8_t* dest_plane_v,
    ptrdiff_t dest_stride_uv) {
  assert(num_planes > 0);
  const int full_jobs_per_plane = height_ / kFrameChunkHeight;
  const int remainder_job_height = height_ & (kFrameChunkHeight - 1);
  const int total_full_jobs = full_jobs_per_plane * num_planes;
  // If the frame height is not a multiple of kFrameChunkHeight, one job with
  // a smaller number of rows is necessary at the end of each plane.
  const int total_jobs =
      total_full_jobs + ((remainder_job_height == 0) ? 0 : num_planes);
  int job_index;
  // Each job corresponds to a slice of kFrameChunkHeight rows in the luma
  // plane. dsp->blend_noise_chroma handles subsampling.
  // This loop body handles a slice of one plane or the other, depending on
  // which are active. That way, threads working on consecutive jobs will keep
  // the same region of luma source in working memory.
  while ((job_index = job_counter->fetch_add(1, std::memory_order_relaxed)) <
         total_jobs) {
    const Plane plane = planes[job_index % num_planes];
    const int slice_index = job_index / num_planes;
    const int start_height = slice_index * kFrameChunkHeight;
    const int job_height = std::min(height_ - start_height, kFrameChunkHeight);

    const auto* source_cursor_y = reinterpret_cast<const Pixel*>(
        source_plane_y + start_height * source_stride_y);
    const int16_t* scaling_lut_uv;
    const uint8_t* source_plane_uv;
    uint8_t* dest_plane_uv;

    if (plane == kPlaneU) {
      scaling_lut_uv = scaling_lut_u_;
      source_plane_uv = source_plane_u;
      dest_plane_uv = dest_plane_u;
    } else {
      assert(plane == kPlaneV);
      scaling_lut_uv = scaling_lut_v_;
      source_plane_uv = source_plane_v;
      dest_plane_uv = dest_plane_v;
    }
    const auto* source_cursor_uv = reinterpret_cast<const Pixel*>(
        source_plane_uv + (start_height >> subsampling_y_) * source_stride_uv);
    auto* dest_cursor_uv = reinterpret_cast<Pixel*>(
        dest_plane_uv + (start_height >> subsampling_y_) * dest_stride_uv);
    dsp.film_grain.blend_noise_chroma[params_.chroma_scaling_from_luma](
        plane, params_, noise_image_, min_value, max_chroma, width_, job_height,
        start_height, subsampling_x_, subsampling_y_, scaling_lut_uv,
        source_cursor_y, source_stride_y, source_cursor_uv, source_stride_uv,
        dest_cursor_uv, dest_stride_uv);
  }
}

template <int bitdepth>
void FilmGrain<bitdepth>::BlendNoiseLumaWorker(
    const dsp::Dsp& dsp, std::atomic<int>* job_counter, int min_value,
    int max_luma, const uint8_t* source_plane_y, ptrdiff_t source_stride_y,
    uint8_t* dest_plane_y, ptrdiff_t dest_stride_y) {
  const int total_full_jobs = height_ / kFrameChunkHeight;
  const int remainder_job_height = height_ & (kFrameChunkHeight - 1);
  const int total_jobs =
      total_full_jobs + static_cast<int>(remainder_job_height > 0);
  int job_index;
  // Each job is some number of rows in a plane.
  while ((job_index = job_counter->fetch_add(1, std::memory_order_relaxed)) <
         total_jobs) {
    const int start_height = job_index * kFrameChunkHeight;
    const int job_height = std::min(height_ - start_height, kFrameChunkHeight);

    const auto* source_cursor_y = reinterpret_cast<const Pixel*>(
        source_plane_y + start_height * source_stride_y);
    auto* dest_cursor_y =
        reinterpret_cast<Pixel*>(dest_plane_y + start_height * dest_stride_y);
    dsp.film_grain.blend_noise_luma(
        noise_image_, min_value, max_luma, params_.chroma_scaling, width_,
        job_height, start_height, scaling_lut_y_, source_cursor_y,
        source_stride_y, dest_cursor_y, dest_stride_y);
  }
}

template <int bitdepth>
bool FilmGrain<bitdepth>::AddNoise(
    const uint8_t* source_plane_y, ptrdiff_t source_stride_y,
    const uint8_t* source_plane_u, const uint8_t* source_plane_v,
    ptrdiff_t source_stride_uv, uint8_t* dest_plane_y, ptrdiff_t dest_stride_y,
    uint8_t* dest_plane_u, uint8_t* dest_plane_v, ptrdiff_t dest_stride_uv) {
  if (!Init()) {
    LIBGAV1_DLOG(ERROR, "Init() failed.");
    return false;
  }
  if (!AllocateNoiseStripes()) {
    LIBGAV1_DLOG(ERROR, "AllocateNoiseStripes() failed.");
    return false;
  }

  const dsp::Dsp& dsp = *dsp::GetDspTable(bitdepth);
  const bool use_luma = params_.num_y_points > 0;

  // Construct noise stripes.
  if (use_luma) {
    // The luma plane is never subsampled.
    dsp.film_grain
        .construct_noise_stripes[static_cast<int>(params_.overlap_flag)](
            luma_grain_, params_.grain_seed, width_, height_,
            /*subsampling_x=*/0, /*subsampling_y=*/0, &noise_stripes_[kPlaneY]);
  }
  if (!is_monochrome_) {
    dsp.film_grain
        .construct_noise_stripes[static_cast<int>(params_.overlap_flag)](
            u_grain_, params_.grain_seed, width_, height_, subsampling_x_,
            subsampling_y_, &noise_stripes_[kPlaneU]);
    dsp.film_grain
        .construct_noise_stripes[static_cast<int>(params_.overlap_flag)](
            v_grain_, params_.grain_seed, width_, height_, subsampling_x_,
            subsampling_y_, &noise_stripes_[kPlaneV]);
  }

  if (!AllocateNoiseImage()) {
    LIBGAV1_DLOG(ERROR, "AllocateNoiseImage() failed.");
    return false;
  }

  // Construct noise image.
  if (use_luma) {
    ConstructNoiseImage(
        &noise_stripes_[kPlaneY], width_, height_, /*subsampling_x=*/0,
        /*subsampling_y=*/0, static_cast<int>(params_.overlap_flag) << 1,
        &noise_image_[kPlaneY]);
    if (params_.overlap_flag) {
      dsp.film_grain.construct_noise_image_overlap(
          &noise_stripes_[kPlaneY], width_, height_, /*subsampling_x=*/0,
          /*subsampling_y=*/0, &noise_image_[kPlaneY]);
    }
  }
  if (!is_monochrome_) {
    ConstructNoiseImage(&noise_stripes_[kPlaneU], width_, height_,
                        subsampling_x_, subsampling_y_,
                        static_cast<int>(params_.overlap_flag)
                            << (1 - subsampling_y_),
                        &noise_image_[kPlaneU]);
    ConstructNoiseImage(&noise_stripes_[kPlaneV], width_, height_,
                        subsampling_x_, subsampling_y_,
                        static_cast<int>(params_.overlap_flag)
                            << (1 - subsampling_y_),
                        &noise_image_[kPlaneV]);
    if (params_.overlap_flag) {
      dsp.film_grain.construct_noise_image_overlap(
          &noise_stripes_[kPlaneU], width_, height_, subsampling_x_,
          subsampling_y_, &noise_image_[kPlaneU]);
      dsp.film_grain.construct_noise_image_overlap(
          &noise_stripes_[kPlaneV], width_, height_, subsampling_x_,
          subsampling_y_, &noise_image_[kPlaneV]);
    }
  }

  // Blend noise image.
  int min_value;
  int max_luma;
  int max_chroma;
  if (params_.clip_to_restricted_range) {
    min_value = 16 << (bitdepth - kBitdepth8);
    max_luma = 235 << (bitdepth - kBitdepth8);
    if (color_matrix_is_identity_) {
      max_chroma = max_luma;
    } else {
      max_chroma = 240 << (bitdepth - kBitdepth8);
    }
  } else {
    min_value = 0;
    max_luma = (256 << (bitdepth - kBitdepth8)) - 1;
    max_chroma = max_luma;
  }

  // Handle all chroma planes first because luma source may be altered in place.
  if (!is_monochrome_) {
    // This is done in a strange way but Vector can't be passed by copy to the
    // lambda capture that spawns the thread.
    Plane planes_to_blend[2];
    int num_planes = 0;
    if (params_.chroma_scaling_from_luma) {
      // Both noise planes are computed from the luma scaling lookup table.
      planes_to_blend[num_planes++] = kPlaneU;
      planes_to_blend[num_planes++] = kPlaneV;
    } else {
      const int height_uv = SubsampledValue(height_, subsampling_y_);
      const int width_uv = SubsampledValue(width_, subsampling_x_);

      // Noise is applied according to a lookup table defined by pieceiwse
      // linear "points." If the lookup table is empty, that corresponds to
      // outputting zero noise.
      if (params_.num_u_points == 0) {
        CopyImagePlane<Pixel>(source_plane_u, source_stride_uv, width_uv,
                              height_uv, dest_plane_u, dest_stride_uv);
      } else {
        planes_to_blend[num_planes++] = kPlaneU;
      }
      if (params_.num_v_points == 0) {
        CopyImagePlane<Pixel>(source_plane_v, source_stride_uv, width_uv,
                              height_uv, dest_plane_v, dest_stride_uv);
      } else {
        planes_to_blend[num_planes++] = kPlaneV;
      }
    }
    if (thread_pool_ != nullptr && num_planes > 0) {
      const int num_workers = thread_pool_->num_threads();
      BlockingCounter pending_workers(num_workers);
      std::atomic<int> job_counter(0);
      for (int i = 0; i < num_workers; ++i) {
        thread_pool_->Schedule([this, dsp, &pending_workers, &planes_to_blend,
                                num_planes, &job_counter, min_value, max_chroma,
                                source_plane_y, source_stride_y, source_plane_u,
                                source_plane_v, source_stride_uv, dest_plane_u,
                                dest_plane_v, dest_stride_uv]() {
          BlendNoiseChromaWorker(dsp, planes_to_blend, num_planes, &job_counter,
                                 min_value, max_chroma, source_plane_y,
                                 source_stride_y, source_plane_u,
                                 source_plane_v, source_stride_uv, dest_plane_u,
                                 dest_plane_v, dest_stride_uv);
          pending_workers.Decrement();
        });
      }
      BlendNoiseChromaWorker(
          dsp, planes_to_blend, num_planes, &job_counter, min_value, max_chroma,
          source_plane_y, source_stride_y, source_plane_u, source_plane_v,
          source_stride_uv, dest_plane_u, dest_plane_v, dest_stride_uv);

      pending_workers.Wait();
    } else {
      // Single threaded.
      if (params_.num_u_points > 0 || params_.chroma_scaling_from_luma) {
        dsp.film_grain.blend_noise_chroma[params_.chroma_scaling_from_luma](
            kPlaneU, params_, noise_image_, min_value, max_chroma, width_,
            height_, /*start_height=*/0, subsampling_x_, subsampling_y_,
            scaling_lut_u_, source_plane_y, source_stride_y, source_plane_u,
            source_stride_uv, dest_plane_u, dest_stride_uv);
      }
      if (params_.num_v_points > 0 || params_.chroma_scaling_from_luma) {
        dsp.film_grain.blend_noise_chroma[params_.chroma_scaling_from_luma](
            kPlaneV, params_, noise_image_, min_value, max_chroma, width_,
            height_, /*start_height=*/0, subsampling_x_, subsampling_y_,
            scaling_lut_v_, source_plane_y, source_stride_y, source_plane_v,
            source_stride_uv, dest_plane_v, dest_stride_uv);
      }
    }
  }
  if (use_luma) {
    if (thread_pool_ != nullptr) {
      const int num_workers = thread_pool_->num_threads();
      BlockingCounter pending_workers(num_workers);
      std::atomic<int> job_counter(0);
      for (int i = 0; i < num_workers; ++i) {
        thread_pool_->Schedule(
            [this, dsp, &pending_workers, &job_counter, min_value, max_luma,
             source_plane_y, source_stride_y, dest_plane_y, dest_stride_y]() {
              BlendNoiseLumaWorker(dsp, &job_counter, min_value, max_luma,
                                   source_plane_y, source_stride_y,
                                   dest_plane_y, dest_stride_y);
              pending_workers.Decrement();
            });
      }

      BlendNoiseLumaWorker(dsp, &job_counter, min_value, max_luma,
                           source_plane_y, source_stride_y, dest_plane_y,
                           dest_stride_y);
      pending_workers.Wait();
    } else {
      dsp.film_grain.blend_noise_luma(
          noise_image_, min_value, max_luma, params_.chroma_scaling, width_,
          height_, /*start_height=*/0, scaling_lut_y_, source_plane_y,
          source_stride_y, dest_plane_y, dest_stride_y);
    }
  } else {
    CopyImagePlane<Pixel>(source_plane_y, source_stride_y, width_, height_,
                          dest_plane_y, dest_stride_y);
  }

  return true;
}

// Explicit instantiations.
template class FilmGrain<kBitdepth8>;
#if LIBGAV1_MAX_BITDEPTH >= 10
template class FilmGrain<kBitdepth10>;
#endif
#if LIBGAV1_MAX_BITDEPTH == 12
template class FilmGrain<kBitdepth12>;
#endif

}  // namespace libgav1
