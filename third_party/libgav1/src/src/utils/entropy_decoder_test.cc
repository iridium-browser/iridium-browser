// Copyright 2021 The libgav1 Authors
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

#include "src/utils/entropy_decoder.h"

#include <cstdint>
#include <cstdio>

#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"

namespace libgav1 {
namespace {

#include "src/utils/entropy_decoder_test_data.inc"

class EntropyDecoderTest : public testing::Test {
 protected:
  // If compile_time is true, tests
  //     bool EntropyDecoder::ReadSymbol(uint16_t* cdf).
  // Otherwise, tests
  //     int EntropyDecoder::ReadSymbol(uint16_t* cdf, int symbol_count)
  // with symbol_count=2.
  template <bool compile_time>
  void TestReadSymbolBoolean(int num_runs);

  // For N = 3..16 (except 15):
  //     template <bool compile_time>
  //     void TestReadSymbolN(int num_runs);
  //
  // If compile_time is true, tests
  //     int EntropyDecoder::ReadSymbol<N>(uint16_t* const cdf).
  // Otherwise, tests
  //     int EntropyDecoder::ReadSymbol(uint16_t* cdf, int symbol_count)
  // with symbol_count=N.
  //
  // NOTE: symbol_count=15 is not tested because AV1 does not use it.
  template <bool compile_time>
  void TestReadSymbol3(int num_runs);

  template <bool compile_time>
  void TestReadSymbol4(int num_runs);

  template <bool compile_time>
  void TestReadSymbol5(int num_runs);

  template <bool compile_time>
  void TestReadSymbol6(int num_runs);

  template <bool compile_time>
  void TestReadSymbol7(int num_runs);

  template <bool compile_time>
  void TestReadSymbol8(int num_runs);

  template <bool compile_time>
  void TestReadSymbol9(int num_runs);

  template <bool compile_time>
  void TestReadSymbol10(int num_runs);

  template <bool compile_time>
  void TestReadSymbol11(int num_runs);

  template <bool compile_time>
  void TestReadSymbol12(int num_runs);

  template <bool compile_time>
  void TestReadSymbol13(int num_runs);

  template <bool compile_time>
  void TestReadSymbol14(int num_runs);

  template <bool compile_time>
  void TestReadSymbol16(int num_runs);
};

template <bool compile_time>
void EntropyDecoderTest::TestReadSymbolBoolean(int num_runs) {
  static constexpr int kSymbols[4][4] = {{0, 0, 1, 1},  //
                                         {0, 1, 1, 0},  //
                                         {1, 0, 1, 0},  //
                                         {1, 0, 0, 1}};
  absl::Duration elapsed_time;
  bool symbols[1024 * 4 * 4];
  for (int run = 0; run < num_runs; ++run) {
    EntropyDecoder reader(kBytesTestReadSymbolBoolean,
                          kNumBytesTestReadSymbolBoolean,
                          /*allow_update_cdf=*/true);
    uint16_t cdf[4][3] = {
        {16384, 0, 0},
        {32768 - 8386, 0, 0},
        {32768 - 24312, 0, 0},
        {16384, 0, 0},
    };
    const absl::Time start = absl::Now();
    int index = 0;
    for (int i = 0; i < 1024; ++i) {
      for (int j = 0; j < 4; ++j) {
        for (int k = 0; k < 4; ++k) {  // NOLINT(modernize-loop-convert)
          if (compile_time) {
            symbols[index++] = reader.ReadSymbol(cdf[k]);
          } else {
            symbols[index++] = reader.ReadSymbol(cdf[k], 2) != 0;
          }
        }
      }
    }
    elapsed_time += absl::Now() - start;
  }
  if (compile_time) {
    printf("TestReadSymbolBooleanCompileTime(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  } else {
    printf("TestReadSymbolBoolean(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }

  int index = 0;
  for (int i = 0; i < 1024; ++i) {
    for (int j = 0; j < 4; ++j) {  // NOLINT(modernize-loop-convert)
      for (int k = 0; k < 4; ++k) {
        ASSERT_EQ(symbols[index++], kSymbols[j][k]);
      }
    }
  }
}

template <bool compile_time>
void EntropyDecoderTest::TestReadSymbol3(int num_runs) {
  static constexpr int kSymbols[6][4] = {{0, 2, 1, 2},  //
                                         {1, 1, 2, 1},  //
                                         {2, 0, 0, 0},  //
                                         {0, 2, 0, 2},  //
                                         {1, 2, 1, 0},  //
                                         {2, 1, 1, 0}};
  absl::Duration elapsed_time;
  int symbols[1024 * 6 * 4];
  for (int run = 0; run < num_runs; ++run) {
    EntropyDecoder reader(kBytesTestReadSymbol3, kNumBytesTestReadSymbol3,
                          /*allow_update_cdf=*/true);
    uint16_t cdf[4][4] = {
        // pdf: 1/3, 1/3, 1/3
        {32768 - 10923, 32768 - 21845, 0, 0},
        // pdf: 1/6, 2/6, 3/6
        {32768 - 5461, 32768 - 16384, 0, 0},
        // pdf: 2/6, 3/6, 1/6
        {32768 - 10923, 32768 - 27307, 0, 0},
        // pdf: 3/6, 1/6, 2/6
        {32768 - 16384, 32768 - 21845, 0, 0},
    };
    const absl::Time start = absl::Now();
    int index = 0;
    for (int i = 0; i < 1024; ++i) {
      for (int j = 0; j < 6; ++j) {
        for (int k = 0; k < 4; ++k) {  // NOLINT(modernize-loop-convert)
          if (compile_time) {
            symbols[index++] = reader.ReadSymbol<3>(cdf[k]);
          } else {
            symbols[index++] = reader.ReadSymbol(cdf[k], 3);
          }
        }
      }
    }
    elapsed_time += absl::Now() - start;
  }
  if (compile_time) {
    printf("TestReadSymbol3CompileTime(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  } else {
    printf("TestReadSymbol3(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }

  int index = 0;
  for (int i = 0; i < 1024; ++i) {
    for (int j = 0; j < 6; ++j) {  // NOLINT(modernize-loop-convert)
      for (int k = 0; k < 4; ++k) {
        ASSERT_EQ(symbols[index++], kSymbols[j][k]);
      }
    }
  }
}

template <bool compile_time>
void EntropyDecoderTest::TestReadSymbol4(int num_runs) {
  static constexpr int kSymbols[8][4] = {{0, 0, 3, 3},  //
                                         {0, 0, 2, 2},  //
                                         {1, 1, 0, 0},  //
                                         {1, 2, 1, 1},  //
                                         {2, 2, 3, 2},  //
                                         {2, 3, 2, 1},  //
                                         {3, 3, 0, 0},  //
                                         {3, 3, 1, 1}};
  absl::Duration elapsed_time;
  int symbols[1024 * 8 * 4];
  for (int run = 0; run < num_runs; ++run) {
    EntropyDecoder reader(kBytesTestReadSymbol4, kNumBytesTestReadSymbol4,
                          /*allow_update_cdf=*/true);
    uint16_t cdf[4][5] = {
        // pdf: 1/4, 1/4, 1/4, 1/4
        {32768 - 8192, 32768 - 16384, 32768 - 24576, 0, 0},
        // pdf: 2/8, 1/8, 2/8, 3/8
        {32768 - 8192, 32768 - 12288, 32768 - 20480, 0, 0},
        // pdf: 1/4, 1/4, 1/4, 1/4
        {32768 - 8192, 32768 - 16384, 32768 - 24576, 0, 0},
        // pdf: 2/8, 3/8, 2/8, 1/8
        {32768 - 8192, 32768 - 20480, 32768 - 28672, 0, 0},
    };
    const absl::Time start = absl::Now();
    int index = 0;
    for (int i = 0; i < 1024; ++i) {
      for (int j = 0; j < 8; ++j) {
        for (int k = 0; k < 4; ++k) {  // NOLINT(modernize-loop-convert)
          if (compile_time) {
            symbols[index++] = reader.ReadSymbol<4>(cdf[k]);
          } else {
            symbols[index++] = reader.ReadSymbol(cdf[k], 4);
          }
        }
      }
    }
    elapsed_time += absl::Now() - start;
  }
  if (compile_time) {
    printf("TestReadSymbol4CompileTime(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  } else {
    printf("TestReadSymbol4(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }

  int index = 0;
  for (int i = 0; i < 1024; ++i) {
    for (int j = 0; j < 8; ++j) {  // NOLINT(modernize-loop-convert)
      for (int k = 0; k < 4; ++k) {
        ASSERT_EQ(symbols[index++], kSymbols[j][k]);
      }
    }
  }
}

template <bool compile_time>
void EntropyDecoderTest::TestReadSymbol5(int num_runs) {
  static constexpr int kSymbols[10][4] = {{0, 0, 4, 4},  //
                                          {0, 1, 3, 3},  //
                                          {1, 2, 2, 2},  //
                                          {1, 3, 1, 1},  //
                                          {2, 4, 0, 0},  //
                                          {2, 0, 4, 3},  //
                                          {3, 1, 3, 2},  //
                                          {3, 2, 2, 1},  //
                                          {4, 3, 1, 2},  //
                                          {4, 0, 4, 2}};
  absl::Duration elapsed_time;
  int symbols[320 * 10 * 4];
  for (int run = 0; run < num_runs; ++run) {
    EntropyDecoder reader(kBytesTestReadSymbol5, kNumBytesTestReadSymbol5,
                          /*allow_update_cdf=*/true);
    uint16_t cdf[4][6] = {
        // pdf: 1/5, 1/5, 1/5, 1/5, 1/5
        {32768 - 6554, 32768 - 13107, 32768 - 19661, 32768 - 26214, 0, 0},
        // pdf: 3/10, 2/10, 2/10, 2/10, 1/10
        {32768 - 9830, 32768 - 16384, 32768 - 22938, 32768 - 29491, 0, 0},
        // pdf: 1/10, 2/10, 2/10, 2/10, 3/10
        {32768 - 3277, 32768 - 9830, 32768 - 16384, 32768 - 22938, 0, 0},
        // pdf: 1/10, 2/10, 4/10, 2/10, 1/10
        {32768 - 3277, 32768 - 9830, 32768 - 22938, 32768 - 29491, 0, 0},
    };
    const absl::Time start = absl::Now();
    int index = 0;
    for (int i = 0; i < 320; ++i) {
      for (int j = 0; j < 10; ++j) {
        for (int k = 0; k < 4; ++k) {  // NOLINT(modernize-loop-convert)
          if (compile_time) {
            symbols[index++] = reader.ReadSymbol<5>(cdf[k]);
          } else {
            symbols[index++] = reader.ReadSymbol(cdf[k], 5);
          }
        }
      }
    }
    elapsed_time += absl::Now() - start;
  }
  if (compile_time) {
    printf("TestReadSymbol5CompileTime(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  } else {
    printf("TestReadSymbol5(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }

  int index = 0;
  for (int i = 0; i < 320; ++i) {
    for (int j = 0; j < 10; ++j) {  // NOLINT(modernize-loop-convert)
      for (int k = 0; k < 4; ++k) {
        ASSERT_EQ(symbols[index++], kSymbols[j][k]);
      }
    }
  }
}

template <bool compile_time>
void EntropyDecoderTest::TestReadSymbol6(int num_runs) {
  static constexpr int kSymbols[12][4] = {{0, 0, 5, 5},  //
                                          {0, 1, 4, 4},  //
                                          {1, 2, 3, 3},  //
                                          {1, 3, 2, 2},  //
                                          {2, 4, 1, 1},  //
                                          {2, 5, 0, 0},  //
                                          {3, 0, 5, 4},  //
                                          {3, 1, 4, 3},  //
                                          {4, 2, 3, 2},  //
                                          {4, 3, 2, 1},  //
                                          {5, 4, 1, 3},  //
                                          {5, 0, 5, 2}};
  absl::Duration elapsed_time;
  int symbols[256 * 12 * 4];
  for (int run = 0; run < num_runs; ++run) {
    EntropyDecoder reader(kBytesTestReadSymbol6, kNumBytesTestReadSymbol6,
                          /*allow_update_cdf=*/true);
    uint16_t cdf[4][7] = {
        // pmf: 1/6, 1/6, 1/6, 1/6, 1/6, 1/6
        {32768 - 5461, 32768 - 10923, 32768 - 16384, 32768 - 21845,
         32768 - 27307, 0, 0},
        // pmf: 3/12, 2/12, 2/12, 2/12, 2/12, 1/12
        {32768 - 8192, 32768 - 13653, 32768 - 19115, 32768 - 24576,
         32768 - 30037, 0, 0},
        // pmf: 1/12, 2/12, 2/12, 2/12, 2/12, 3/12
        {32768 - 2731, 32768 - 8192, 32768 - 13653, 32768 - 19115,
         32768 - 24576, 0, 0},
        // pmf: 1/12, 2/12, 3/12, 3/12, 2/12, 1/12
        {32768 - 2731, 32768 - 8192, 32768 - 16384, 32768 - 24576,
         32768 - 30037, 0, 0},
    };
    const absl::Time start = absl::Now();
    int index = 0;
    for (int i = 0; i < 256; ++i) {
      for (int j = 0; j < 12; ++j) {
        for (int k = 0; k < 4; ++k) {  // NOLINT(modernize-loop-convert)
          if (compile_time) {
            symbols[index++] = reader.ReadSymbol<6>(cdf[k]);
          } else {
            symbols[index++] = reader.ReadSymbol(cdf[k], 6);
          }
        }
      }
    }
    elapsed_time += absl::Now() - start;
  }
  if (compile_time) {
    printf("TestReadSymbol6CompileTime(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  } else {
    printf("TestReadSymbol6(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }

  int index = 0;
  for (int i = 0; i < 256; ++i) {
    for (int j = 0; j < 12; ++j) {  // NOLINT(modernize-loop-convert)
      for (int k = 0; k < 4; ++k) {
        ASSERT_EQ(symbols[index++], kSymbols[j][k]);
      }
    }
  }
}

template <bool compile_time>
void EntropyDecoderTest::TestReadSymbol7(int num_runs) {
  static constexpr int kSymbols[14][4] = {{0, 4, 6, 3},  //
                                          {1, 5, 5, 2},  //
                                          {2, 6, 4, 1},  //
                                          {3, 0, 3, 0},  //
                                          {4, 1, 2, 6},  //
                                          {5, 2, 1, 5},  //
                                          {6, 3, 0, 4},  //
                                          {0, 0, 6, 5},  //
                                          {2, 1, 4, 3},  //
                                          {4, 3, 6, 1},  //
                                          {6, 5, 2, 4},  //
                                          {1, 0, 5, 2},  //
                                          {3, 2, 3, 2},  //
                                          {5, 4, 5, 3}};
  absl::Duration elapsed_time;
  int symbols[1024 * 14 * 4];
  for (int run = 0; run < num_runs; ++run) {
    EntropyDecoder reader(kBytesTestReadSymbol7, kNumBytesTestReadSymbol7,
                          /*allow_update_cdf=*/true);
    uint16_t cdf[4][8] = {
        // pdf: 1/7, 1/7, 1/7, 1/7, 1/7, 1/7, 1/7
        {32768 - 4681, 32768 - 9362, 32768 - 14043, 32768 - 18725,
         32768 - 23406, 32768 - 28087, 0, 0},
        // pdf: 3/14, 2/14, 2/14, 2/14, 2/14, 2/14, 1/14
        {32768 - 7022, 32768 - 11703, 32768 - 16384, 32768 - 21065,
         32768 - 25746, 32768 - 30427, 0, 0},
        // pdf: 1/14, 1/14, 2/14, 2/14, 2/14, 3/14, 3/14
        {32768 - 2341, 32768 - 4681, 32768 - 9362, 32768 - 14043, 32768 - 18725,
         32768 - 25746, 0, 0},
        // pdf: 1/14, 2/14, 3/14, 3/14, 2/14, 2/14, 1/14
        {32768 - 2341, 32768 - 7022, 32768 - 14043, 32768 - 21065,
         32768 - 25746, 32768 - 30427, 0, 0},
    };
    const absl::Time start = absl::Now();
    int index = 0;
    for (int i = 0; i < 1024; ++i) {
      for (int j = 0; j < 14; ++j) {
        for (int k = 0; k < 4; ++k) {  // NOLINT(modernize-loop-convert)
          if (compile_time) {
            symbols[index++] = reader.ReadSymbol<7>(cdf[k]);
          } else {
            symbols[index++] = reader.ReadSymbol(cdf[k], 7);
          }
        }
      }
    }
    elapsed_time += absl::Now() - start;
  }
  if (compile_time) {
    printf("TestReadSymbol7CompileTime(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  } else {
    printf("TestReadSymbol7(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }

  int index = 0;
  for (int i = 0; i < 1024; ++i) {
    for (int j = 0; j < 14; ++j) {  // NOLINT(modernize-loop-convert)
      for (int k = 0; k < 4; ++k) {
        ASSERT_EQ(symbols[index++], kSymbols[j][k]);
      }
    }
  }
}

template <bool compile_time>
void EntropyDecoderTest::TestReadSymbol8(int num_runs) {
  static constexpr int kSymbols[16][4] = {{0, 4, 7, 3},  //
                                          {1, 5, 6, 2},  //
                                          {2, 6, 5, 1},  //
                                          {3, 7, 4, 0},  //
                                          {4, 0, 3, 7},  //
                                          {5, 1, 2, 6},  //
                                          {6, 2, 1, 5},  //
                                          {7, 3, 0, 4},  //
                                          {0, 0, 6, 5},  //
                                          {2, 1, 4, 3},  //
                                          {4, 3, 6, 4},  //
                                          {6, 5, 2, 2},  //
                                          {1, 0, 7, 3},  //
                                          {3, 2, 5, 5},  //
                                          {5, 4, 7, 2},  //
                                          {7, 6, 3, 4}};
  absl::Duration elapsed_time;
  int symbols[1024 * 16 * 4];
  for (int run = 0; run < num_runs; ++run) {
    EntropyDecoder reader(kBytesTestReadSymbol8, kNumBytesTestReadSymbol8,
                          /*allow_update_cdf=*/true);
    uint16_t cdf[4][9] = {
        // pdf: 1/8, 1/8, 1/8, 1/8, 1/8, 1/8, 1/8, 1/8
        {32768 - 4096, 32768 - 8192, 32768 - 12288, 32768 - 16384,
         32768 - 20480, 32768 - 24576, 32768 - 28672, 0, 0},
        // pdf: 3/16, 2/16, 2/16, 2/16, 2/16, 2/16, 2/16, 1/16
        {32768 - 6144, 32768 - 10240, 32768 - 14336, 32768 - 18432,
         32768 - 22528, 32768 - 26624, 32768 - 30720, 0, 0},
        // pdf: 1/16, 1/16, 2/16, 2/16, 2/16, 2/16, 3/16, 3/16
        {32768 - 2048, 32768 - 4096, 32768 - 8192, 32768 - 12288, 32768 - 16384,
         32768 - 20480, 32768 - 26624, 0, 0},
        // pdf: 1/16, 1/16, 3/16, 3/16, 3/16, 3/16, 1/16, 1/16
        {32768 - 2048, 32768 - 4096, 32768 - 10240, 32768 - 16384,
         32768 - 22528, 32768 - 28672, 32768 - 30720, 0, 0},
    };
    const absl::Time start = absl::Now();
    int index = 0;
    for (int i = 0; i < 1024; ++i) {
      for (int j = 0; j < 16; ++j) {
        for (int k = 0; k < 4; ++k) {  // NOLINT(modernize-loop-convert)
          if (compile_time) {
            symbols[index++] = reader.ReadSymbol<8>(cdf[k]);
          } else {
            symbols[index++] = reader.ReadSymbol(cdf[k], 8);
          }
        }
      }
    }
    elapsed_time += absl::Now() - start;
  }
  if (compile_time) {
    printf("TestReadSymbol8CompileTime(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  } else {
    printf("TestReadSymbol8(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }

  int index = 0;
  for (int i = 0; i < 1024; ++i) {
    for (int j = 0; j < 16; ++j) {  // NOLINT(modernize-loop-convert)
      for (int k = 0; k < 4; ++k) {
        ASSERT_EQ(symbols[index++], kSymbols[j][k]);
      }
    }
  }
}

template <bool compile_time>
void EntropyDecoderTest::TestReadSymbol9(int num_runs) {
  static constexpr int kSymbols[18][4] = {{0, 4, 8, 3},  //
                                          {1, 5, 7, 2},  //
                                          {2, 6, 6, 1},  //
                                          {3, 7, 5, 0},  //
                                          {4, 8, 4, 8},  //
                                          {5, 0, 3, 7},  //
                                          {6, 1, 2, 6},  //
                                          {7, 2, 1, 5},  //
                                          {8, 3, 0, 4},  //
                                          {0, 0, 8, 7},  //
                                          {2, 1, 6, 5},  //
                                          {4, 3, 4, 3},  //
                                          {6, 5, 2, 1},  //
                                          {8, 7, 7, 6},  //
                                          {1, 0, 5, 4},  //
                                          {3, 2, 3, 2},  //
                                          {5, 4, 1, 4},  //
                                          {7, 6, 8, 4}};
  absl::Duration elapsed_time;
  int symbols[128 * 18 * 4];
  for (int run = 0; run < num_runs; ++run) {
    EntropyDecoder reader(kBytesTestReadSymbol9, kNumBytesTestReadSymbol9,
                          /*allow_update_cdf=*/true);
    uint16_t cdf[4][10] = {
        // pmf: 1/9, 1/9, 1/9, 1/9, 1/9, 1/9, 1/9, 1/9, 1/9
        {32768 - 3641, 32768 - 7282, 32768 - 10923, 32768 - 14564,
         32768 - 18204, 32768 - 21845, 32768 - 25486, 32768 - 29127, 0, 0},
        // pmf: 3/18, 2/18, 2/18, 2/18, 2/18, 2/18, 2/18, 2/18, 1/18
        {32768 - 5461, 32768 - 9102, 32768 - 12743, 32768 - 16384,
         32768 - 20025, 32768 - 23666, 32768 - 27307, 32768 - 30948, 0, 0},
        // pmf: 1/18, 2/18, 2/18, 2/18, 2/18, 2/18, 2/18, 2/18, 3/18
        {32768 - 1820, 32768 - 5461, 32768 - 9102, 32768 - 12743, 32768 - 16384,
         32768 - 20025, 32768 - 23666, 32768 - 27307, 0, 0},
        // pmf: 1/18, 2/18, 2/18, 2/18, 4/18, 2/18, 2/18, 2/18, 1/18
        {32768 - 1820, 32768 - 5461, 32768 - 9102, 32768 - 12743, 32768 - 20025,
         32768 - 23666, 32768 - 27307, 32768 - 30948, 0, 0},
    };
    const absl::Time start = absl::Now();
    int index = 0;
    for (int i = 0; i < 128; ++i) {
      for (int j = 0; j < 18; ++j) {
        for (int k = 0; k < 4; ++k) {  // NOLINT(modernize-loop-convert)
          if (compile_time) {
            symbols[index++] = reader.ReadSymbol<9>(cdf[k]);
          } else {
            symbols[index++] = reader.ReadSymbol(cdf[k], 9);
          }
        }
      }
    }
    elapsed_time += absl::Now() - start;
  }
  if (compile_time) {
    printf("TestReadSymbol9CompileTime(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  } else {
    printf("TestReadSymbol9(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }

  int index = 0;
  for (int i = 0; i < 128; ++i) {
    for (int j = 0; j < 18; ++j) {  // NOLINT(modernize-loop-convert)
      for (int k = 0; k < 4; ++k) {
        ASSERT_EQ(symbols[index++], kSymbols[j][k]);
      }
    }
  }
}

template <bool compile_time>
void EntropyDecoderTest::TestReadSymbol10(int num_runs) {
  static constexpr int kSymbols[20][4] = {{0, 5, 9, 4},  //
                                          {1, 6, 8, 3},  //
                                          {2, 7, 7, 2},  //
                                          {3, 8, 6, 1},  //
                                          {4, 9, 5, 0},  //
                                          {5, 0, 4, 9},  //
                                          {6, 1, 3, 8},  //
                                          {7, 2, 2, 7},  //
                                          {8, 3, 1, 6},  //
                                          {9, 4, 0, 5},  //
                                          {0, 0, 9, 7},  //
                                          {2, 1, 8, 5},  //
                                          {4, 3, 6, 3},  //
                                          {6, 5, 4, 1},  //
                                          {8, 7, 2, 8},  //
                                          {1, 0, 9, 6},  //
                                          {3, 2, 7, 4},  //
                                          {5, 4, 5, 2},  //
                                          {7, 6, 3, 5},  //
                                          {9, 8, 1, 4}};
  absl::Duration elapsed_time;
  int symbols[96 * 20 * 4];
  for (int run = 0; run < num_runs; ++run) {
    EntropyDecoder reader(kBytesTestReadSymbol10, kNumBytesTestReadSymbol10,
                          /*allow_update_cdf=*/true);
    uint16_t cdf[4][11] = {
        // pmf: 1/10, 1/10, 1/10, 1/10, 1/10, 1/10, 1/10, 1/10, 1/10, 1/10
        {32768 - 3277, 32768 - 6554, 32768 - 9830, 32768 - 13107, 32768 - 16384,
         32768 - 19661, 32768 - 22938, 32768 - 26214, 32768 - 29491, 0, 0},
        // pmf: 3/20, 2/20, 2/20, 2/20, 2/20, 2/20, 2/20, 2/20, 2/20, 1/20
        {32768 - 4915, 32768 - 8192, 32768 - 11469, 32768 - 14746,
         32768 - 18022, 32768 - 21299, 32768 - 24576, 32768 - 27853,
         32768 - 31130, 0, 0},
        // pmf: 1/20, 2/20, 2/20, 2/20, 2/20, 2/20, 2/20, 2/20, 2/20, 3/20
        {32768 - 1638, 32768 - 4915, 32768 - 8192, 32768 - 11469, 32768 - 14746,
         32768 - 18022, 32768 - 21299, 32768 - 24576, 32768 - 27853, 0, 0},
        // pmf: 1/20, 2/20, 2/20, 2/20, 3/20, 3/20, 2/20, 2/20, 2/20, 1/20
        {32768 - 1638, 32768 - 4915, 32768 - 8192, 32768 - 11469, 32768 - 16384,
         32768 - 21299, 32768 - 24576, 32768 - 27853, 32768 - 31130, 0, 0},
    };
    const absl::Time start = absl::Now();
    int index = 0;
    for (int i = 0; i < 96; ++i) {
      for (int j = 0; j < 20; ++j) {
        for (int k = 0; k < 4; ++k) {  // NOLINT(modernize-loop-convert)
          if (compile_time) {
            symbols[index++] = reader.ReadSymbol<10>(cdf[k]);
          } else {
            symbols[index++] = reader.ReadSymbol(cdf[k], 10);
          }
        }
      }
    }
    elapsed_time += absl::Now() - start;
  }
  if (compile_time) {
    printf("TestReadSymbol10CompileTime(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  } else {
    printf("TestReadSymbol10(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }

  int index = 0;
  for (int i = 0; i < 96; ++i) {
    for (int j = 0; j < 20; ++j) {  // NOLINT(modernize-loop-convert)
      for (int k = 0; k < 4; ++k) {
        ASSERT_EQ(symbols[index++], kSymbols[j][k]);
      }
    }
  }
}

template <bool compile_time>
void EntropyDecoderTest::TestReadSymbol11(int num_runs) {
  static constexpr int kSymbols[22][4] = {{0, 6, 10, 5},   //
                                          {1, 7, 9, 4},    //
                                          {2, 8, 8, 3},    //
                                          {3, 9, 7, 2},    //
                                          {4, 10, 6, 1},   //
                                          {5, 0, 5, 0},    //
                                          {6, 1, 4, 10},   //
                                          {7, 2, 3, 9},    //
                                          {8, 3, 2, 8},    //
                                          {9, 4, 1, 7},    //
                                          {10, 5, 0, 6},   //
                                          {0, 0, 10, 9},   //
                                          {2, 1, 8, 7},    //
                                          {4, 3, 6, 5},    //
                                          {6, 5, 4, 3},    //
                                          {8, 7, 2, 1},    //
                                          {10, 9, 10, 8},  //
                                          {1, 0, 9, 6},    //
                                          {3, 2, 7, 4},    //
                                          {5, 4, 5, 2},    //
                                          {7, 6, 3, 5},    //
                                          {9, 8, 1, 5}};
  absl::Duration elapsed_time;
  int symbols[96 * 22 * 4];
  for (int run = 0; run < num_runs; ++run) {
    EntropyDecoder reader(kBytesTestReadSymbol11, kNumBytesTestReadSymbol11,
                          /*allow_update_cdf=*/true);
    uint16_t cdf[4][12] = {
        // pmf: 1/11, 1/11, 1/11, 1/11, 1/11, 1/11, 1/11, 1/11, 1/11, 1/11, 1/11
        {32768 - 2979, 32768 - 5958, 32768 - 8937, 32768 - 11916, 32768 - 14895,
         32768 - 17873, 32768 - 20852, 32768 - 23831, 32768 - 26810,
         32768 - 29789, 0, 0},
        // pmf: 3/22, 2/22, 2/22, 2/22, 2/22, 2/22, 2/22, 2/22, 2/22, 2/22, 1/22
        {32768 - 4468, 32768 - 7447, 32768 - 10426, 32768 - 13405,
         32768 - 16384, 32768 - 19363, 32768 - 22342, 32768 - 25321,
         32768 - 28300, 32768 - 31279, 0, 0},
        // pmf: 1/22, 2/22, 2/22, 2/22, 2/22, 2/22, 2/22, 2/22, 2/22, 2/22, 3/22
        {32768 - 1489, 32768 - 4468, 32768 - 7447, 32768 - 10426, 32768 - 13405,
         32768 - 16384, 32768 - 19363, 32768 - 22342, 32768 - 25321,
         32768 - 28300, 0, 0},
        // pmf: 1/22, 2/22, 2/22, 2/22, 2/22, 4/22, 2/22, 2/22, 2/22, 2/22, 1/22
        {32768 - 1489, 32768 - 4468, 32768 - 7447, 32768 - 10426, 32768 - 13405,
         32768 - 19363, 32768 - 22342, 32768 - 25321, 32768 - 28300,
         32768 - 31279, 0, 0},
    };
    const absl::Time start = absl::Now();
    int index = 0;
    for (int i = 0; i < 96; ++i) {
      for (int j = 0; j < 22; ++j) {
        for (int k = 0; k < 4; ++k) {  // NOLINT(modernize-loop-convert)
          if (compile_time) {
            symbols[index++] = reader.ReadSymbol<11>(cdf[k]);
          } else {
            symbols[index++] = reader.ReadSymbol(cdf[k], 11);
          }
        }
      }
    }
    elapsed_time += absl::Now() - start;
  }
  if (compile_time) {
    printf("TestReadSymbol11CompileTime(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  } else {
    printf("TestReadSymbol11(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }

  int index = 0;
  for (int i = 0; i < 96; ++i) {
    for (int j = 0; j < 22; ++j) {  // NOLINT(modernize-loop-convert)
      for (int k = 0; k < 4; ++k) {
        ASSERT_EQ(symbols[index++], kSymbols[j][k]);
      }
    }
  }
}

template <bool compile_time>
void EntropyDecoderTest::TestReadSymbol12(int num_runs) {
  static constexpr int kSymbols[24][4] = {{0, 6, 11, 5},   //
                                          {1, 7, 10, 4},   //
                                          {2, 8, 9, 3},    //
                                          {3, 9, 8, 2},    //
                                          {4, 10, 7, 1},   //
                                          {5, 11, 6, 0},   //
                                          {6, 0, 5, 11},   //
                                          {7, 1, 4, 10},   //
                                          {8, 2, 3, 9},    //
                                          {9, 3, 2, 8},    //
                                          {10, 4, 1, 7},   //
                                          {11, 5, 0, 6},   //
                                          {0, 0, 11, 9},   //
                                          {2, 1, 10, 7},   //
                                          {4, 3, 8, 5},    //
                                          {6, 5, 6, 3},    //
                                          {8, 7, 4, 1},    //
                                          {10, 9, 2, 10},  //
                                          {1, 0, 11, 8},   //
                                          {3, 2, 9, 6},    //
                                          {5, 4, 7, 4},    //
                                          {7, 6, 5, 2},    //
                                          {9, 8, 3, 6},    //
                                          {11, 10, 1, 5}};
  absl::Duration elapsed_time;
  int symbols[80 * 24 * 4];
  for (int run = 0; run < num_runs; ++run) {
    EntropyDecoder reader(kBytesTestReadSymbol12, kNumBytesTestReadSymbol12,
                          /*allow_update_cdf=*/true);
    uint16_t cdf[4][13] = {
        // pmf: 1/12, 1/12, 1/12, 1/12, 1/12, 1/12, 1/12, 1/12, 1/12, 1/12,
        // 1/12,
        // 1/12
        {32768 - 2731, 32768 - 5461, 32768 - 8192, 32768 - 10923, 32768 - 13653,
         32768 - 16384, 32768 - 19115, 32768 - 21845, 32768 - 24576,
         32768 - 27307, 32768 - 30037, 0, 0},
        // pmf: 3/24, 2/24, 2/24, 2/24, 2/24, 2/24, 2/24, 2/24, 2/24, 2/24,
        // 2/24,
        // 1/24
        {32768 - 4096, 32768 - 6827, 32768 - 9557, 32768 - 12288, 32768 - 15019,
         32768 - 17749, 32768 - 20480, 32768 - 23211, 32768 - 25941,
         32768 - 28672, 32768 - 31403, 0, 0},
        // pmf: 1/24, 2/24, 2/24, 2/24, 2/24, 2/24, 2/24, 2/24, 2/24, 2/24,
        // 2/24,
        // 3/24
        {32768 - 1365, 32768 - 4096, 32768 - 6827, 32768 - 9557, 32768 - 12288,
         32768 - 15019, 32768 - 17749, 32768 - 20480, 32768 - 23211,
         32768 - 25941, 32768 - 28672, 0, 0},
        // pmf: 1/24, 2/24, 2/24, 2/24, 2/24, 3/24, 3/24, 2/24, 2/24, 2/24,
        // 2/24,
        // 1/24
        {32768 - 1365, 32768 - 4096, 32768 - 6827, 32768 - 9557, 32768 - 12288,
         32768 - 16384, 32768 - 20480, 32768 - 23211, 32768 - 25941,
         32768 - 28672, 32768 - 31403, 0, 0},
    };
    const absl::Time start = absl::Now();
    int index = 0;
    for (int i = 0; i < 80; ++i) {
      for (int j = 0; j < 24; ++j) {
        for (int k = 0; k < 4; ++k) {  // NOLINT(modernize-loop-convert)
          if (compile_time) {
            symbols[index++] = reader.ReadSymbol<12>(cdf[k]);
          } else {
            symbols[index++] = reader.ReadSymbol(cdf[k], 12);
          }
        }
      }
    }
    elapsed_time += absl::Now() - start;
  }
  if (compile_time) {
    printf("TestReadSymbol12CompileTime(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  } else {
    printf("TestReadSymbol12(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }

  int index = 0;
  for (int i = 0; i < 80; ++i) {
    for (int j = 0; j < 24; ++j) {  // NOLINT(modernize-loop-convert)
      for (int k = 0; k < 4; ++k) {
        ASSERT_EQ(symbols[index++], kSymbols[j][k]);
      }
    }
  }
}

template <bool compile_time>
void EntropyDecoderTest::TestReadSymbol13(int num_runs) {
  static constexpr int kSymbols[26][4] = {{0, 6, 12, 5},     //
                                          {1, 7, 11, 4},     //
                                          {2, 8, 10, 3},     //
                                          {3, 9, 9, 2},      //
                                          {4, 10, 8, 1},     //
                                          {5, 11, 7, 0},     //
                                          {6, 12, 6, 12},    //
                                          {7, 0, 5, 11},     //
                                          {8, 1, 4, 10},     //
                                          {9, 2, 3, 9},      //
                                          {10, 3, 2, 8},     //
                                          {11, 4, 1, 7},     //
                                          {12, 5, 0, 6},     //
                                          {0, 0, 12, 11},    //
                                          {2, 1, 10, 9},     //
                                          {4, 3, 8, 7},      //
                                          {6, 5, 6, 5},      //
                                          {8, 7, 4, 3},      //
                                          {10, 9, 2, 1},     //
                                          {12, 11, 12, 10},  //
                                          {1, 0, 11, 8},     //
                                          {3, 2, 9, 6},      //
                                          {5, 4, 7, 4},      //
                                          {7, 6, 5, 2},      //
                                          {9, 8, 3, 6},      //
                                          {11, 10, 1, 6}};
  absl::Duration elapsed_time;
  int symbols[64 * 26 * 4];
  for (int run = 0; run < num_runs; ++run) {
    EntropyDecoder reader(kBytesTestReadSymbol13, kNumBytesTestReadSymbol13,
                          /*allow_update_cdf=*/true);
    uint16_t cdf[4][14] = {
        // pmf: 1/13, 1/13, 1/13, 1/13, 1/13, 1/13, 1/13, 1/13, 1/13, 1/13,
        // 1/13, 1/13, 1/13
        {32768 - 2521, 32768 - 5041, 32768 - 7562, 32768 - 10082, 32768 - 12603,
         32768 - 15124, 32768 - 17644, 32768 - 20165, 32768 - 22686,
         32768 - 25206, 32768 - 27727, 32768 - 30247, 0, 0},
        // pmf: 3/26, 2/26, 2/26, 2/26, 2/26, 2/26, 2/26, 2/26, 2/26, 2/26,
        // 2/26, 2/26, 1/26
        {32768 - 3781, 32768 - 6302, 32768 - 8822, 32768 - 11343, 32768 - 13863,
         32768 - 16384, 32768 - 18905, 32768 - 21425, 32768 - 23946,
         32768 - 26466, 32768 - 28987, 32768 - 31508, 0, 0},
        // pmf: 1/26, 2/26, 2/26, 2/26, 2/26, 2/26, 2/26, 2/26, 2/26, 2/26,
        // 2/26, 2/26, 3/26
        {32768 - 1260, 32768 - 3781, 32768 - 6302, 32768 - 8822, 32768 - 11343,
         32768 - 13863, 32768 - 16384, 32768 - 18905, 32768 - 21425,
         32768 - 23946, 32768 - 26466, 32768 - 28987, 0, 0},
        // pmf: 1/26, 2/26, 2/26, 2/26, 2/26, 2/26, 4/26, 2/26, 2/26, 2/26,
        // 2/26, 2/26, 1/26
        {32768 - 1260, 32768 - 3781, 32768 - 6302, 32768 - 8822, 32768 - 11343,
         32768 - 13863, 32768 - 18905, 32768 - 21425, 32768 - 23946,
         32768 - 26466, 32768 - 28987, 32768 - 31508, 0, 0},
    };
    const absl::Time start = absl::Now();
    int index = 0;
    for (int i = 0; i < 64; ++i) {
      for (int j = 0; j < 26; ++j) {
        for (int k = 0; k < 4; ++k) {  // NOLINT(modernize-loop-convert)
          if (compile_time) {
            symbols[index++] = reader.ReadSymbol<13>(cdf[k]);
          } else {
            symbols[index++] = reader.ReadSymbol(cdf[k], 13);
          }
        }
      }
    }
    elapsed_time += absl::Now() - start;
  }
  if (compile_time) {
    printf("TestReadSymbol13CompileTime(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  } else {
    printf("TestReadSymbol13(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }

  int index = 0;
  for (int i = 0; i < 64; ++i) {
    for (int j = 0; j < 26; ++j) {  // NOLINT(modernize-loop-convert)
      for (int k = 0; k < 4; ++k) {
        ASSERT_EQ(symbols[index++], kSymbols[j][k]);
      }
    }
  }
}

template <bool compile_time>
void EntropyDecoderTest::TestReadSymbol14(int num_runs) {
  static constexpr int kSymbols[28][4] = {{0, 7, 13, 6},    //
                                          {1, 8, 12, 5},    //
                                          {2, 9, 11, 4},    //
                                          {3, 10, 10, 3},   //
                                          {4, 11, 9, 2},    //
                                          {5, 12, 8, 1},    //
                                          {6, 13, 7, 0},    //
                                          {7, 0, 6, 13},    //
                                          {8, 1, 5, 12},    //
                                          {9, 2, 4, 11},    //
                                          {10, 3, 3, 10},   //
                                          {11, 4, 2, 9},    //
                                          {12, 5, 1, 8},    //
                                          {13, 6, 0, 7},    //
                                          {0, 0, 13, 11},   //
                                          {2, 1, 12, 9},    //
                                          {4, 3, 10, 7},    //
                                          {6, 5, 8, 5},     //
                                          {8, 7, 6, 3},     //
                                          {10, 9, 4, 1},    //
                                          {12, 11, 2, 12},  //
                                          {1, 0, 13, 10},   //
                                          {3, 2, 11, 8},    //
                                          {5, 4, 9, 6},     //
                                          {7, 6, 7, 4},     //
                                          {9, 8, 5, 2},     //
                                          {11, 10, 3, 7},   //
                                          {13, 12, 1, 6}};
  absl::Duration elapsed_time;
  int symbols[64 * 28 * 4];
  for (int run = 0; run < num_runs; ++run) {
    EntropyDecoder reader(kBytesTestReadSymbol14, kNumBytesTestReadSymbol14,
                          /*allow_update_cdf=*/true);
    uint16_t cdf[4][15] = {
        // pmf: 1/14, 1/14, 1/14, 1/14, 1/14, 1/14, 1/14, 1/14, 1/14, 1/14,
        // 1/14, 1/14, 1/14, 1/14
        {32768 - 2341, 32768 - 4681, 32768 - 7022, 32768 - 9362, 32768 - 11703,
         32768 - 14043, 32768 - 16384, 32768 - 18725, 32768 - 21065,
         32768 - 23406, 32768 - 25746, 32768 - 28087, 32768 - 30427, 0, 0},
        // pmf: 3/28, 2/28, 2/28, 2/28, 2/28, 2/28, 2/28, 2/28, 2/28, 2/28,
        // 2/28, 2/28, 2/28, 1/28
        {32768 - 3511, 32768 - 5851, 32768 - 8192, 32768 - 10533, 32768 - 12873,
         32768 - 15214, 32768 - 17554, 32768 - 19895, 32768 - 22235,
         32768 - 24576, 32768 - 26917, 32768 - 29257, 32768 - 31598, 0, 0},
        // pmf: 1/28, 2/28, 2/28, 2/28, 2/28, 2/28, 2/28, 2/28, 2/28, 2/28,
        // 2/28, 2/28, 2/28, 3/28
        {32768 - 1170, 32768 - 3511, 32768 - 5851, 32768 - 8192, 32768 - 10533,
         32768 - 12873, 32768 - 15214, 32768 - 17554, 32768 - 19895,
         32768 - 22235, 32768 - 24576, 32768 - 26917, 32768 - 29257, 0, 0},
        // pmf: 1/28, 2/28, 2/28, 2/28, 2/28, 2/28, 3/28, 3/28, 2/28, 2/28,
        // 2/28, 2/28, 2/28, 1/28
        {32768 - 1170, 32768 - 3511, 32768 - 5851, 32768 - 8192, 32768 - 10533,
         32768 - 12873, 32768 - 16384, 32768 - 19895, 32768 - 22235,
         32768 - 24576, 32768 - 26917, 32768 - 29257, 32768 - 31598, 0, 0},
    };
    const absl::Time start = absl::Now();
    int index = 0;
    for (int i = 0; i < 64; ++i) {
      for (int j = 0; j < 28; ++j) {
        for (int k = 0; k < 4; ++k) {  // NOLINT(modernize-loop-convert)
          if (compile_time) {
            symbols[index++] = reader.ReadSymbol<14>(cdf[k]);
          } else {
            symbols[index++] = reader.ReadSymbol(cdf[k], 14);
          }
        }
      }
    }
    elapsed_time += absl::Now() - start;
  }
  if (compile_time) {
    printf("TestReadSymbol14CompileTime(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  } else {
    printf("TestReadSymbol14(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }

  int index = 0;
  for (int i = 0; i < 64; ++i) {
    for (int j = 0; j < 28; ++j) {  // NOLINT(modernize-loop-convert)
      for (int k = 0; k < 4; ++k) {
        ASSERT_EQ(symbols[index++], kSymbols[j][k]);
      }
    }
  }
}

template <bool compile_time>
void EntropyDecoderTest::TestReadSymbol16(int num_runs) {
  static constexpr int kSymbols[32][4] = {{0, 8, 15, 7},    //
                                          {1, 9, 14, 6},    //
                                          {2, 10, 13, 5},   //
                                          {3, 11, 12, 4},   //
                                          {4, 12, 11, 3},   //
                                          {5, 13, 10, 2},   //
                                          {6, 14, 9, 1},    //
                                          {7, 15, 8, 0},    //
                                          {8, 0, 7, 15},    //
                                          {9, 1, 6, 14},    //
                                          {10, 2, 5, 13},   //
                                          {11, 3, 4, 12},   //
                                          {12, 4, 3, 11},   //
                                          {13, 5, 2, 10},   //
                                          {14, 6, 1, 9},    //
                                          {15, 7, 0, 8},    //
                                          {0, 0, 15, 13},   //
                                          {2, 1, 14, 11},   //
                                          {4, 3, 12, 9},    //
                                          {6, 5, 10, 7},    //
                                          {8, 7, 8, 5},     //
                                          {10, 9, 6, 3},    //
                                          {12, 11, 4, 1},   //
                                          {14, 13, 2, 14},  //
                                          {1, 0, 15, 12},   //
                                          {3, 2, 13, 10},   //
                                          {5, 4, 11, 8},    //
                                          {7, 6, 9, 6},     //
                                          {9, 8, 7, 4},     //
                                          {11, 10, 5, 2},   //
                                          {13, 12, 3, 8},   //
                                          {15, 14, 1, 7}};
  absl::Duration elapsed_time;
  int symbols[48 * 32 * 4];
  for (int run = 0; run < num_runs; ++run) {
    EntropyDecoder reader(kBytesTestReadSymbol16, kNumBytesTestReadSymbol16,
                          /*allow_update_cdf=*/true);
    uint16_t cdf[4][17] = {
        // pmf: 1/16, 1/16, 1/16, 1/16, 1/16, 1/16, 1/16, 1/16, 1/16, 1/16,
        // 1/16, 1/16, 1/16, 1/16, 1/16, 1/16
        {32768 - 2048, 32768 - 4096, 32768 - 6144, 32768 - 8192, 32768 - 10240,
         32768 - 12288, 32768 - 14336, 32768 - 16384, 32768 - 18432,
         32768 - 20480, 32768 - 22528, 32768 - 24576, 32768 - 26624,
         32768 - 28672, 32768 - 30720, 0, 0},
        // pmf: 3/32, 2/32, 2/32, 2/32, 2/32, 2/32, 2/32, 2/32, 2/32, 2/32,
        // 2/32, 2/32, 2/32, 2/32, 2/32, 1/32
        {32768 - 3072, 32768 - 5120, 32768 - 7168, 32768 - 9216, 32768 - 11264,
         32768 - 13312, 32768 - 15360, 32768 - 17408, 32768 - 19456,
         32768 - 21504, 32768 - 23552, 32768 - 25600, 32768 - 27648,
         32768 - 29696, 32768 - 31744, 0, 0},
        // pmf: 1/32, 2/32, 2/32, 2/32, 2/32, 2/32, 2/32, 2/32, 2/32, 2/32,
        // 2/32, 2/32, 2/32, 2/32, 2/32, 3/32
        {32768 - 1024, 32768 - 3072, 32768 - 5120, 32768 - 7168, 32768 - 9216,
         32768 - 11264, 32768 - 13312, 32768 - 15360, 32768 - 17408,
         32768 - 19456, 32768 - 21504, 32768 - 23552, 32768 - 25600,
         32768 - 27648, 32768 - 29696, 0, 0},
        // pmf: 1/32, 2/32, 2/32, 2/32, 2/32, 2/32, 2/32, 3/32, 3/32, 2/32,
        // 2/32, 2/32, 2/32, 2/32, 2/32, 1/32
        {32768 - 1024, 32768 - 3072, 32768 - 5120, 32768 - 7168, 32768 - 9216,
         32768 - 11264, 32768 - 13312, 32768 - 16384, 32768 - 19456,
         32768 - 21504, 32768 - 23552, 32768 - 25600, 32768 - 27648,
         32768 - 29696, 32768 - 31744, 0, 0},
    };
    const absl::Time start = absl::Now();
    int index = 0;
    for (int i = 0; i < 48; ++i) {
      for (int j = 0; j < 32; ++j) {
        for (int k = 0; k < 4; ++k) {  // NOLINT(modernize-loop-convert)
          if (compile_time) {
            symbols[index++] = reader.ReadSymbol<16>(cdf[k]);
          } else {
            symbols[index++] = reader.ReadSymbol(cdf[k], 16);
          }
        }
      }
    }
    elapsed_time += absl::Now() - start;
  }
  if (compile_time) {
    printf("TestReadSymbol16CompileTime(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  } else {
    printf("TestReadSymbol16(%d): %5d us\n", num_runs,
           static_cast<int>(absl::ToInt64Microseconds(elapsed_time)));
  }

  int index = 0;
  for (int i = 0; i < 48; ++i) {
    for (int j = 0; j < 32; ++j) {  // NOLINT(modernize-loop-convert)
      for (int k = 0; k < 4; ++k) {
        ASSERT_EQ(symbols[index++], kSymbols[j][k]);
      }
    }
  }
}

TEST_F(EntropyDecoderTest, ReadSymbolBoolean) {
  TestReadSymbolBoolean</*compile_time=*/false>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbolBooleanCompileTime) {
  TestReadSymbolBoolean</*compile_time=*/true>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol3) {
  TestReadSymbol3</*compile_time=*/false>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol3CompileTime) {
  TestReadSymbol3</*compile_time=*/true>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol4) {
  TestReadSymbol4</*compile_time=*/false>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol4CompileTime) {
  TestReadSymbol4</*compile_time=*/true>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol5) {
  TestReadSymbol5</*compile_time=*/false>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol5CompileTime) {
  TestReadSymbol5</*compile_time=*/true>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol6) {
  TestReadSymbol6</*compile_time=*/false>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol6CompileTime) {
  TestReadSymbol6</*compile_time=*/true>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol7) {
  TestReadSymbol7</*compile_time=*/false>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol7CompileTime) {
  TestReadSymbol7</*compile_time=*/true>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol8) {
  TestReadSymbol8</*compile_time=*/false>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol8CompileTime) {
  TestReadSymbol8</*compile_time=*/true>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol9) {
  TestReadSymbol9</*compile_time=*/false>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol9CompileTime) {
  TestReadSymbol9</*compile_time=*/true>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol10) {
  TestReadSymbol10</*compile_time=*/false>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol10CompileTime) {
  TestReadSymbol10</*compile_time=*/true>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol11) {
  TestReadSymbol11</*compile_time=*/false>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol11CompileTime) {
  TestReadSymbol11</*compile_time=*/true>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol12) {
  TestReadSymbol12</*compile_time=*/false>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol12CompileTime) {
  TestReadSymbol12</*compile_time=*/true>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol13) {
  TestReadSymbol13</*compile_time=*/false>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol13CompileTime) {
  TestReadSymbol13</*compile_time=*/true>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol14) {
  TestReadSymbol14</*compile_time=*/false>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol14CompileTime) {
  TestReadSymbol14</*compile_time=*/true>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol16) {
  TestReadSymbol16</*compile_time=*/false>(1);
}

TEST_F(EntropyDecoderTest, ReadSymbol16CompileTime) {
  TestReadSymbol16</*compile_time=*/true>(1);
}

TEST_F(EntropyDecoderTest, DISABLED_Speed) {
  // compile_time=true is only tested for those symbol_count values that have
  // an instantiation of the EntropyDecoder::ReadSymbol<symbol_count> template
  // method.
  TestReadSymbolBoolean</*compile_time=*/false>(10000);
  TestReadSymbolBoolean</*compile_time=*/true>(10000);
  TestReadSymbol3</*compile_time=*/false>(5000);
  TestReadSymbol3</*compile_time=*/true>(5000);
  TestReadSymbol4</*compile_time=*/false>(2000);
  TestReadSymbol4</*compile_time=*/true>(2000);
  TestReadSymbol5</*compile_time=*/false>(5000);
  TestReadSymbol5</*compile_time=*/true>(5000);
  TestReadSymbol6</*compile_time=*/false>(5000);
  TestReadSymbol6</*compile_time=*/true>(5000);
  TestReadSymbol7</*compile_time=*/false>(1000);
  TestReadSymbol7</*compile_time=*/true>(1000);
  TestReadSymbol8</*compile_time=*/false>(1000);
  TestReadSymbol8</*compile_time=*/true>(1000);
  TestReadSymbol9</*compile_time=*/false>(5000);
  TestReadSymbol9</*compile_time=*/true>(5000);
  TestReadSymbol10</*compile_time=*/false>(5000);
  TestReadSymbol10</*compile_time=*/true>(5000);
  TestReadSymbol11</*compile_time=*/false>(5000);
  TestReadSymbol11</*compile_time=*/true>(5000);
  TestReadSymbol12</*compile_time=*/false>(5000);
  TestReadSymbol12</*compile_time=*/true>(5000);
  TestReadSymbol13</*compile_time=*/false>(5000);
  TestReadSymbol13</*compile_time=*/true>(5000);
  TestReadSymbol14</*compile_time=*/false>(5000);
  TestReadSymbol14</*compile_time=*/true>(5000);
  TestReadSymbol16</*compile_time=*/false>(5000);
  TestReadSymbol16</*compile_time=*/true>(5000);
}

}  // namespace
}  // namespace libgav1
