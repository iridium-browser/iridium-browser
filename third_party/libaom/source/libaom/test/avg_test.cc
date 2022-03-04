/*
 *  Copyright (c) 2019, Alliance for Open Media. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdlib.h>
#include <ostream>
#include <tuple>

#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

#include "config/aom_dsp_rtcd.h"

#include "test/acm_random.h"
#include "test/register_state_check.h"
#include "test/util.h"

namespace {

using libaom_test::ACMRandom;

template <typename Pixel>
class AverageTestBase : public ::testing::Test {
 public:
  AverageTestBase(int width, int height)
      : width_(width), height_(height), source_data_(NULL), source_stride_(0),
        bit_depth_(8) {}

  virtual void TearDown() {
    aom_free(source_data_);
    source_data_ = NULL;
  }

 protected:
  // Handle blocks up to 4 blocks 64x64 with stride up to 128
  static const int kDataAlignment = 16;
  static const int kDataBlockSize = 64 * 128;

  virtual void SetUp() {
    source_data_ = static_cast<Pixel *>(
        aom_memalign(kDataAlignment, kDataBlockSize * sizeof(source_data_[0])));
    ASSERT_TRUE(source_data_ != NULL);
    source_stride_ = (width_ + 31) & ~31;
    bit_depth_ = 8;
    rnd_.Reset(ACMRandom::DeterministicSeed());
  }

  // Sum Pixels
  static unsigned int ReferenceAverage8x8(const Pixel *source, int pitch) {
    unsigned int average = 0;
    for (int h = 0; h < 8; ++h) {
      for (int w = 0; w < 8; ++w) average += source[h * pitch + w];
    }
    return (average + 32) >> 6;
  }

  static unsigned int ReferenceAverage4x4(const Pixel *source, int pitch) {
    unsigned int average = 0;
    for (int h = 0; h < 4; ++h) {
      for (int w = 0; w < 4; ++w) average += source[h * pitch + w];
    }
    return (average + 8) >> 4;
  }

  void FillConstant(Pixel fill_constant) {
    for (int i = 0; i < width_ * height_; ++i) {
      source_data_[i] = fill_constant;
    }
  }

  void FillRandom() {
    for (int i = 0; i < width_ * height_; ++i) {
      source_data_[i] = rnd_.Rand16() & ((1 << bit_depth_) - 1);
    }
  }

  int width_, height_;
  Pixel *source_data_;
  int source_stride_;
  int bit_depth_;

  ACMRandom rnd_;
};
typedef unsigned int (*AverageFunction)(const uint8_t *s, int pitch);

// Arguments: width, height, pitch, block size, avg function.
typedef std::tuple<int, int, int, int, AverageFunction> AvgFunc;

class AverageTest : public AverageTestBase<uint8_t>,
                    public ::testing::WithParamInterface<AvgFunc> {
 public:
  AverageTest() : AverageTestBase(GET_PARAM(0), GET_PARAM(1)) {}

 protected:
  void CheckAverages() {
    const int block_size = GET_PARAM(3);
    unsigned int expected = 0;
    if (block_size == 8) {
      expected =
          ReferenceAverage8x8(source_data_ + GET_PARAM(2), source_stride_);
    } else if (block_size == 4) {
      expected =
          ReferenceAverage4x4(source_data_ + GET_PARAM(2), source_stride_);
    }

    unsigned int actual;
    API_REGISTER_STATE_CHECK(
        actual = GET_PARAM(4)(source_data_ + GET_PARAM(2), source_stride_));

    EXPECT_EQ(expected, actual);
  }
};

TEST_P(AverageTest, MinValue) {
  FillConstant(0);
  CheckAverages();
}

TEST_P(AverageTest, MaxValue) {
  FillConstant(255);
  CheckAverages();
}

TEST_P(AverageTest, Random) {
  // The reference frame, but not the source frame, may be unaligned for
  // certain types of searches.
  for (int i = 0; i < 1000; i++) {
    FillRandom();
    CheckAverages();
  }
}

typedef void (*IntProRowFunc)(int16_t hbuf[16], uint8_t const *ref,
                              const int ref_stride, const int height);

// Params: height, asm function, c function.
typedef std::tuple<int, IntProRowFunc, IntProRowFunc> IntProRowParam;

class IntProRowTest : public AverageTestBase<uint8_t>,
                      public ::testing::WithParamInterface<IntProRowParam> {
 public:
  IntProRowTest()
      : AverageTestBase(16, GET_PARAM(0)), hbuf_asm_(NULL), hbuf_c_(NULL) {
    asm_func_ = GET_PARAM(1);
    c_func_ = GET_PARAM(2);
  }

 protected:
  virtual void SetUp() {
    source_data_ = static_cast<uint8_t *>(
        aom_memalign(kDataAlignment, kDataBlockSize * sizeof(source_data_[0])));
    ASSERT_TRUE(source_data_ != NULL);

    hbuf_asm_ = static_cast<int16_t *>(
        aom_memalign(kDataAlignment, sizeof(*hbuf_asm_) * 16));
    hbuf_c_ = static_cast<int16_t *>(
        aom_memalign(kDataAlignment, sizeof(*hbuf_c_) * 16));
  }

  virtual void TearDown() {
    aom_free(source_data_);
    source_data_ = NULL;
    aom_free(hbuf_c_);
    hbuf_c_ = NULL;
    aom_free(hbuf_asm_);
    hbuf_asm_ = NULL;
  }

  void RunComparison() {
    API_REGISTER_STATE_CHECK(c_func_(hbuf_c_, source_data_, 0, height_));
    API_REGISTER_STATE_CHECK(asm_func_(hbuf_asm_, source_data_, 0, height_));
    EXPECT_EQ(0, memcmp(hbuf_c_, hbuf_asm_, sizeof(*hbuf_c_) * 16))
        << "Output mismatch\n";
  }

  void RunSpeedTest() {
    const int numIter = 5000000;
    printf("Height = %d number of iteration is %d \n", height_, numIter);
    aom_usec_timer c_timer_;
    aom_usec_timer_start(&c_timer_);
    for (int i = 0; i < numIter; i++) {
      c_func_(hbuf_c_, source_data_, 0, height_);
    }
    aom_usec_timer_mark(&c_timer_);

    aom_usec_timer asm_timer_;
    aom_usec_timer_start(&asm_timer_);

    for (int i = 0; i < numIter; i++) {
      asm_func_(hbuf_asm_, source_data_, 0, height_);
    }
    aom_usec_timer_mark(&asm_timer_);

    const int c_sum_time = static_cast<int>(aom_usec_timer_elapsed(&c_timer_));
    const int asm_sum_time =
        static_cast<int>(aom_usec_timer_elapsed(&asm_timer_));

    printf("c_time = %d \t simd_time = %d \t Gain = %4.2f \n", c_sum_time,
           asm_sum_time,
           (static_cast<float>(c_sum_time) / static_cast<float>(asm_sum_time)));

    EXPECT_EQ(0, memcmp(hbuf_c_, hbuf_asm_, sizeof(*hbuf_c_) * 16))
        << "Output mismatch\n";
  }

 private:
  IntProRowFunc asm_func_;
  IntProRowFunc c_func_;
  int16_t *hbuf_asm_;
  int16_t *hbuf_c_;
};
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(IntProRowTest);

typedef int16_t (*IntProColFunc)(uint8_t const *ref, const int width);

// Params: width, asm function, c function.
typedef std::tuple<int, IntProColFunc, IntProColFunc> IntProColParam;

class IntProColTest : public AverageTestBase<uint8_t>,
                      public ::testing::WithParamInterface<IntProColParam> {
 public:
  IntProColTest() : AverageTestBase(GET_PARAM(0), 1), sum_asm_(0), sum_c_(0) {
    asm_func_ = GET_PARAM(1);
    c_func_ = GET_PARAM(2);
  }

 protected:
  void RunComparison() {
    API_REGISTER_STATE_CHECK(sum_c_ = c_func_(source_data_, width_));
    API_REGISTER_STATE_CHECK(sum_asm_ = asm_func_(source_data_, width_));
    EXPECT_EQ(sum_c_, sum_asm_) << "Output mismatch";
  }
  void RunSpeedTest() {
    const int numIter = 5000000;
    printf("Width = %d number of iteration is %d \n", width_, numIter);
    aom_usec_timer c_timer_;
    aom_usec_timer_start(&c_timer_);
    for (int i = 0; i < numIter; i++) {
      sum_c_ = c_func_(source_data_, width_);
    }
    aom_usec_timer_mark(&c_timer_);

    aom_usec_timer asm_timer_;
    aom_usec_timer_start(&asm_timer_);

    for (int i = 0; i < numIter; i++) {
      sum_asm_ = asm_func_(source_data_, width_);
    }
    aom_usec_timer_mark(&asm_timer_);

    const int c_sum_time = static_cast<int>(aom_usec_timer_elapsed(&c_timer_));
    const int asm_sum_time =
        static_cast<int>(aom_usec_timer_elapsed(&asm_timer_));

    printf("c_time = %d \t simd_time = %d \t Gain = %4.2f \n", c_sum_time,
           asm_sum_time,
           (static_cast<float>(c_sum_time) / static_cast<float>(asm_sum_time)));

    EXPECT_EQ(sum_c_, sum_asm_) << "Output mismatch \n";
  }

 private:
  IntProColFunc asm_func_;
  IntProColFunc c_func_;
  int16_t sum_asm_;
  int16_t sum_c_;
};
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(IntProColTest);

TEST_P(IntProRowTest, MinValue) {
  FillConstant(0);
  RunComparison();
}

TEST_P(IntProRowTest, MaxValue) {
  FillConstant(255);
  RunComparison();
}

TEST_P(IntProRowTest, Random) {
  FillRandom();
  RunComparison();
}

TEST_P(IntProRowTest, DISABLED_Speed) {
  FillRandom();
  RunSpeedTest();
}

TEST_P(IntProColTest, MinValue) {
  FillConstant(0);
  RunComparison();
}

TEST_P(IntProColTest, MaxValue) {
  FillConstant(255);
  RunComparison();
}

TEST_P(IntProColTest, Random) {
  FillRandom();
  RunComparison();
}

TEST_P(IntProColTest, DISABLED_Speed) {
  FillRandom();
  RunSpeedTest();
}
class VectorVarTestBase : public ::testing::Test {
 public:
  explicit VectorVarTestBase(int bwl) { m_bwl = bwl; }
  VectorVarTestBase() {}
  ~VectorVarTestBase() {}

 protected:
  static const int kDataAlignment = 16;

  virtual void SetUp() {
    width = 4 << m_bwl;

    ref_vector = static_cast<int16_t *>(
        aom_memalign(kDataAlignment, width * sizeof(ref_vector[0])));
    ASSERT_TRUE(ref_vector != NULL);
    src_vector = static_cast<int16_t *>(
        aom_memalign(kDataAlignment, width * sizeof(src_vector[0])));
    ASSERT_TRUE(src_vector != NULL);

    rnd_.Reset(ACMRandom::DeterministicSeed());
  }
  virtual void TearDown() {
    aom_free(ref_vector);
    ref_vector = NULL;
    aom_free(src_vector);
    src_vector = NULL;
  }

  void FillConstant(int16_t fill_constant_ref, int16_t fill_constant_src) {
    for (int i = 0; i < width; ++i) {
      ref_vector[i] = fill_constant_ref;
      src_vector[i] = fill_constant_src;
    }
  }

  void FillRandom() {
    for (int i = 0; i < width; ++i) {
      ref_vector[i] =
          rnd_.Rand16() % max_range;  // acc. aom_vector_var_c brief.
      src_vector[i] = rnd_.Rand16() % max_range;
    }
  }

  int width;
  int m_bwl;
  int16_t *ref_vector;
  int16_t *src_vector;
  ACMRandom rnd_;

  static const int max_range = 510;
  static const int num_random_cmp = 50;
};

typedef int (*VectorVarFunc)(const int16_t *ref, const int16_t *src,
                             const int bwl);

typedef std::tuple<int, VectorVarFunc, VectorVarFunc> VecVarFunc;

class VectorVarTest : public VectorVarTestBase,
                      public ::testing::WithParamInterface<VecVarFunc> {
 public:
  VectorVarTest()
      : VectorVarTestBase(GET_PARAM(0)), c_func(GET_PARAM(1)),
        simd_func(GET_PARAM(2)) {}

 protected:
  int calcVarC() { return c_func(ref_vector, src_vector, m_bwl); }
  int calcVarSIMD() { return simd_func(ref_vector, src_vector, m_bwl); }

  VectorVarFunc c_func;
  VectorVarFunc simd_func;
};
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VectorVarTest);

TEST_P(VectorVarTest, MaxVar) {
  FillConstant(0, max_range);
  int c_var = calcVarC();
  int simd_var = calcVarSIMD();
  ASSERT_EQ(c_var, simd_var);
}
TEST_P(VectorVarTest, MaxVarRev) {
  FillConstant(max_range, 0);
  int c_var = calcVarC();
  int simd_var = calcVarSIMD();
  ASSERT_EQ(c_var, simd_var);
}
TEST_P(VectorVarTest, ZeroDiff) {
  FillConstant(0, 0);
  int c_var = calcVarC();
  int simd_var = calcVarSIMD();
  ASSERT_EQ(c_var, simd_var);
}
TEST_P(VectorVarTest, ZeroDiff2) {
  FillConstant(max_range, max_range);
  int c_var = calcVarC();
  int simd_var = calcVarSIMD();
  ASSERT_EQ(c_var, simd_var);
}
TEST_P(VectorVarTest, Constant) {
  FillConstant(30, 90);
  int c_var = calcVarC();
  int simd_var = calcVarSIMD();
  ASSERT_EQ(c_var, simd_var);
}
TEST_P(VectorVarTest, Random) {
  for (size_t i = 0; i < num_random_cmp; i++) {
    FillRandom();
    int c_var = calcVarC();
    int simd_var = calcVarSIMD();
    ASSERT_EQ(c_var, simd_var);
  }
}
TEST_P(VectorVarTest, DISABLED_Speed) {
  FillRandom();
  const int numIter = 50000;
  printf("Width = %d number of iteration is %d \n", width, numIter);

  int sum_c_var = 0;
  int c_var = 0;

  aom_usec_timer c_timer_;
  aom_usec_timer_start(&c_timer_);
  for (size_t i = 0; i < numIter; i++) {
    c_var = calcVarC();
    sum_c_var += c_var;
  }
  aom_usec_timer_mark(&c_timer_);

  int simd_var = 0;
  int sum_simd_var = 0;
  aom_usec_timer simd_timer_;
  aom_usec_timer_start(&simd_timer_);
  for (size_t i = 0; i < numIter; i++) {
    simd_var = calcVarSIMD();
    sum_simd_var += simd_var;
  }
  aom_usec_timer_mark(&simd_timer_);

  const int c_sum_time = static_cast<int>(aom_usec_timer_elapsed(&c_timer_));
  const int simd_sum_time =
      static_cast<int>(aom_usec_timer_elapsed(&simd_timer_));

  printf("c_time = %d \t simd_time = %d \t Gain = %4.2f \n", c_sum_time,
         simd_sum_time,
         (static_cast<float>(c_sum_time) / static_cast<float>(simd_sum_time)));

  EXPECT_EQ(c_var, simd_var) << "Output mismatch \n";
  EXPECT_EQ(sum_c_var, sum_simd_var) << "Output mismatch \n";
}

using std::make_tuple;

INSTANTIATE_TEST_SUITE_P(
    C, AverageTest,
    ::testing::Values(make_tuple(16, 16, 1, 8, &aom_avg_8x8_c),
                      make_tuple(16, 16, 1, 4, &aom_avg_4x4_c)));

#if HAVE_SSE2
INSTANTIATE_TEST_SUITE_P(
    SSE2, AverageTest,
    ::testing::Values(make_tuple(16, 16, 0, 8, &aom_avg_8x8_sse2),
                      make_tuple(16, 16, 5, 8, &aom_avg_8x8_sse2),
                      make_tuple(32, 32, 15, 8, &aom_avg_8x8_sse2),
                      make_tuple(16, 16, 0, 4, &aom_avg_4x4_sse2),
                      make_tuple(16, 16, 5, 4, &aom_avg_4x4_sse2),
                      make_tuple(32, 32, 15, 4, &aom_avg_4x4_sse2)));

INSTANTIATE_TEST_SUITE_P(
    SSE2, IntProRowTest,
    ::testing::Values(make_tuple(16, &aom_int_pro_row_sse2, &aom_int_pro_row_c),
                      make_tuple(32, &aom_int_pro_row_sse2, &aom_int_pro_row_c),
                      make_tuple(64, &aom_int_pro_row_sse2, &aom_int_pro_row_c),
                      make_tuple(128, &aom_int_pro_row_sse2,
                                 &aom_int_pro_row_c)));

INSTANTIATE_TEST_SUITE_P(
    SSE2, IntProColTest,
    ::testing::Values(make_tuple(16, &aom_int_pro_col_sse2, &aom_int_pro_col_c),
                      make_tuple(32, &aom_int_pro_col_sse2, &aom_int_pro_col_c),
                      make_tuple(64, &aom_int_pro_col_sse2, &aom_int_pro_col_c),
                      make_tuple(128, &aom_int_pro_col_sse2,
                                 &aom_int_pro_col_c)));
#endif

#if HAVE_NEON
INSTANTIATE_TEST_SUITE_P(
    NEON, AverageTest,
    ::testing::Values(make_tuple(16, 16, 0, 8, &aom_avg_8x8_neon),
                      make_tuple(16, 16, 5, 8, &aom_avg_8x8_neon),
                      make_tuple(32, 32, 15, 8, &aom_avg_8x8_neon),
                      make_tuple(16, 16, 0, 4, &aom_avg_4x4_neon),
                      make_tuple(16, 16, 5, 4, &aom_avg_4x4_neon),
                      make_tuple(32, 32, 15, 4, &aom_avg_4x4_neon)));
INSTANTIATE_TEST_SUITE_P(
    NEON, IntProRowTest,
    ::testing::Values(make_tuple(16, &aom_int_pro_row_neon, &aom_int_pro_row_c),
                      make_tuple(32, &aom_int_pro_row_neon, &aom_int_pro_row_c),
                      make_tuple(64, &aom_int_pro_row_neon, &aom_int_pro_row_c),
                      make_tuple(128, &aom_int_pro_row_neon,
                                 &aom_int_pro_row_c)));

INSTANTIATE_TEST_SUITE_P(
    NEON, IntProColTest,
    ::testing::Values(make_tuple(16, &aom_int_pro_col_neon, &aom_int_pro_col_c),
                      make_tuple(32, &aom_int_pro_col_neon, &aom_int_pro_col_c),
                      make_tuple(64, &aom_int_pro_col_neon, &aom_int_pro_col_c),
                      make_tuple(128, &aom_int_pro_col_neon,
                                 &aom_int_pro_col_c)));
#endif

typedef int (*SatdFunc)(const tran_low_t *coeffs, int length);
typedef int (*SatdLpFunc)(const int16_t *coeffs, int length);

template <typename SatdFuncType>
struct SatdTestParam {
  SatdTestParam(int s, SatdFuncType f1, SatdFuncType f2)
      : satd_size(s), func_ref(f1), func_simd(f2) {}
  friend std::ostream &operator<<(std::ostream &os,
                                  const SatdTestParam<SatdFuncType> &param) {
    return os << "satd_size: " << param.satd_size;
  }
  int satd_size;
  SatdFuncType func_ref;
  SatdFuncType func_simd;
};

template <typename CoeffType, typename SatdFuncType>
class SatdTestBase
    : public ::testing::Test,
      public ::testing::WithParamInterface<SatdTestParam<SatdFuncType>> {
 protected:
  explicit SatdTestBase(const SatdTestParam<SatdFuncType> &func_param) {
    satd_size_ = func_param.satd_size;
    satd_func_ref_ = func_param.func_ref;
    satd_func_simd_ = func_param.func_simd;
  }
  virtual void SetUp() {
    rnd_.Reset(ACMRandom::DeterministicSeed());
    src_ = reinterpret_cast<CoeffType *>(
        aom_memalign(32, sizeof(*src_) * satd_size_));
    ASSERT_TRUE(src_ != NULL);
  }
  virtual void TearDown() { aom_free(src_); }
  void FillConstant(const CoeffType val) {
    for (int i = 0; i < satd_size_; ++i) src_[i] = val;
  }
  void FillRandom() {
    for (int i = 0; i < satd_size_; ++i) {
      src_[i] = static_cast<int16_t>(rnd_.Rand16());
    }
  }
  void Check(int expected) {
    int total_ref;
    API_REGISTER_STATE_CHECK(total_ref = satd_func_ref_(src_, satd_size_));
    EXPECT_EQ(expected, total_ref);

    int total_simd;
    API_REGISTER_STATE_CHECK(total_simd = satd_func_simd_(src_, satd_size_));
    EXPECT_EQ(expected, total_simd);
  }
  void RunComparison() {
    int total_ref;
    API_REGISTER_STATE_CHECK(total_ref = satd_func_ref_(src_, satd_size_));

    int total_simd;
    API_REGISTER_STATE_CHECK(total_simd = satd_func_simd_(src_, satd_size_));

    EXPECT_EQ(total_ref, total_simd);
  }
  void RunSpeedTest() {
    const int numIter = 500000;
    printf("size = %d number of iteration is %d \n", satd_size_, numIter);

    int total_ref;
    aom_usec_timer c_timer_;
    aom_usec_timer_start(&c_timer_);
    for (int i = 0; i < numIter; i++) {
      total_ref = satd_func_ref_(src_, satd_size_);
    }
    aom_usec_timer_mark(&c_timer_);

    int total_simd;
    aom_usec_timer simd_timer_;
    aom_usec_timer_start(&simd_timer_);

    for (int i = 0; i < numIter; i++) {
      total_simd = satd_func_simd_(src_, satd_size_);
    }
    aom_usec_timer_mark(&simd_timer_);

    const int c_sum_time = static_cast<int>(aom_usec_timer_elapsed(&c_timer_));
    const int simd_sum_time =
        static_cast<int>(aom_usec_timer_elapsed(&simd_timer_));

    printf(
        "c_time = %d \t simd_time = %d \t Gain = %4.2f \n", c_sum_time,
        simd_sum_time,
        (static_cast<float>(c_sum_time) / static_cast<float>(simd_sum_time)));

    EXPECT_EQ(total_ref, total_simd) << "Output mismatch \n";
  }
  int satd_size_;

 private:
  CoeffType *src_;
  SatdFuncType satd_func_ref_;
  SatdFuncType satd_func_simd_;
  ACMRandom rnd_;
};

class SatdTest : public SatdTestBase<tran_low_t, SatdFunc> {
 public:
  SatdTest() : SatdTestBase(GetParam()) {}
};

TEST_P(SatdTest, MinValue) {
  const int kMin = -32640;
  const int expected = -kMin * satd_size_;
  FillConstant(kMin);
  Check(expected);
}
TEST_P(SatdTest, MaxValue) {
  const int kMax = 32640;
  const int expected = kMax * satd_size_;
  FillConstant(kMax);
  Check(expected);
}
TEST_P(SatdTest, Random) {
  int expected;
  switch (satd_size_) {
    case 16: expected = 205298; break;
    case 64: expected = 1113950; break;
    case 256: expected = 4268415; break;
    case 1024: expected = 16954082; break;
    default:
      FAIL() << "Invalid satd size (" << satd_size_
             << ") valid: 16/64/256/1024";
  }
  FillRandom();
  Check(expected);
}
TEST_P(SatdTest, Match) {
  FillRandom();
  RunComparison();
}
TEST_P(SatdTest, DISABLED_Speed) {
  FillRandom();
  RunSpeedTest();
}
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SatdTest);

INSTANTIATE_TEST_SUITE_P(
    C, SatdTest,
    ::testing::Values(SatdTestParam<SatdFunc>(16, &aom_satd_c, &aom_satd_c),
                      SatdTestParam<SatdFunc>(64, &aom_satd_c, &aom_satd_c),
                      SatdTestParam<SatdFunc>(256, &aom_satd_c, &aom_satd_c),
                      SatdTestParam<SatdFunc>(1024, &aom_satd_c, &aom_satd_c)));

#if HAVE_NEON
INSTANTIATE_TEST_SUITE_P(
    NEON, SatdTest,
    ::testing::Values(SatdTestParam<SatdFunc>(16, &aom_satd_c, &aom_satd_neon),
                      SatdTestParam<SatdFunc>(64, &aom_satd_c, &aom_satd_neon),
                      SatdTestParam<SatdFunc>(256, &aom_satd_c, &aom_satd_neon),
                      SatdTestParam<SatdFunc>(1024, &aom_satd_c,
                                              &aom_satd_neon)));
INSTANTIATE_TEST_SUITE_P(
    NEON, VectorVarTest,
    ::testing::Values(make_tuple(2, &aom_vector_var_c, &aom_vector_var_neon),
                      make_tuple(3, &aom_vector_var_c, &aom_vector_var_neon),
                      make_tuple(4, &aom_vector_var_c, &aom_vector_var_neon),
                      make_tuple(5, &aom_vector_var_c, &aom_vector_var_neon)));
#endif

#if HAVE_AVX2
INSTANTIATE_TEST_SUITE_P(
    AVX2, SatdTest,
    ::testing::Values(SatdTestParam<SatdFunc>(16, &aom_satd_c, &aom_satd_avx2),
                      SatdTestParam<SatdFunc>(64, &aom_satd_c, &aom_satd_avx2),
                      SatdTestParam<SatdFunc>(256, &aom_satd_c, &aom_satd_avx2),
                      SatdTestParam<SatdFunc>(1024, &aom_satd_c,
                                              &aom_satd_avx2)));
#endif

#if HAVE_SSE2
INSTANTIATE_TEST_SUITE_P(
    SSE2, SatdTest,
    ::testing::Values(SatdTestParam<SatdFunc>(16, &aom_satd_c, &aom_satd_sse2),
                      SatdTestParam<SatdFunc>(64, &aom_satd_c, &aom_satd_sse2),
                      SatdTestParam<SatdFunc>(256, &aom_satd_c, &aom_satd_sse2),
                      SatdTestParam<SatdFunc>(1024, &aom_satd_c,
                                              &aom_satd_sse2)));
#endif

class SatdLpTest : public SatdTestBase<int16_t, SatdLpFunc> {
 public:
  SatdLpTest() : SatdTestBase(GetParam()) {}
};

TEST_P(SatdLpTest, MinValue) {
  const int kMin = -32640;
  const int expected = -kMin * satd_size_;
  FillConstant(kMin);
  Check(expected);
}
TEST_P(SatdLpTest, MaxValue) {
  const int kMax = 32640;
  const int expected = kMax * satd_size_;
  FillConstant(kMax);
  Check(expected);
}
TEST_P(SatdLpTest, Random) {
  int expected;
  switch (satd_size_) {
    case 16: expected = 205298; break;
    case 64: expected = 1113950; break;
    case 256: expected = 4268415; break;
    case 1024: expected = 16954082; break;
    default:
      FAIL() << "Invalid satd size (" << satd_size_
             << ") valid: 16/64/256/1024";
  }
  FillRandom();
  Check(expected);
}
TEST_P(SatdLpTest, Match) {
  FillRandom();
  RunComparison();
}
TEST_P(SatdLpTest, DISABLED_Speed) {
  FillRandom();
  RunSpeedTest();
}
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SatdLpTest);

// Add the following c test to avoid gtest uninitialized warning.
INSTANTIATE_TEST_SUITE_P(
    C, SatdLpTest,
    ::testing::Values(
        SatdTestParam<SatdLpFunc>(16, &aom_satd_lp_c, &aom_satd_lp_c),
        SatdTestParam<SatdLpFunc>(64, &aom_satd_lp_c, &aom_satd_lp_c),
        SatdTestParam<SatdLpFunc>(256, &aom_satd_lp_c, &aom_satd_lp_c),
        SatdTestParam<SatdLpFunc>(1024, &aom_satd_lp_c, &aom_satd_lp_c)));

#if HAVE_NEON
INSTANTIATE_TEST_SUITE_P(
    NEON, SatdLpTest,
    ::testing::Values(
        SatdTestParam<SatdLpFunc>(16, &aom_satd_lp_c, &aom_satd_lp_neon),
        SatdTestParam<SatdLpFunc>(64, &aom_satd_lp_c, &aom_satd_lp_neon),
        SatdTestParam<SatdLpFunc>(256, &aom_satd_lp_c, &aom_satd_lp_neon),
        SatdTestParam<SatdLpFunc>(1024, &aom_satd_lp_c, &aom_satd_lp_neon)));
#endif

#if HAVE_AVX2
INSTANTIATE_TEST_SUITE_P(
    AVX2, SatdLpTest,
    ::testing::Values(
        SatdTestParam<SatdLpFunc>(16, &aom_satd_lp_c, &aom_satd_lp_avx2),
        SatdTestParam<SatdLpFunc>(64, &aom_satd_lp_c, &aom_satd_lp_avx2),
        SatdTestParam<SatdLpFunc>(256, &aom_satd_lp_c, &aom_satd_lp_avx2),
        SatdTestParam<SatdLpFunc>(1024, &aom_satd_lp_c, &aom_satd_lp_avx2)));
#endif

#if HAVE_SSE2
INSTANTIATE_TEST_SUITE_P(
    SSE2, SatdLpTest,
    ::testing::Values(
        SatdTestParam<SatdLpFunc>(16, &aom_satd_lp_c, &aom_satd_lp_sse2),
        SatdTestParam<SatdLpFunc>(64, &aom_satd_lp_c, &aom_satd_lp_sse2),
        SatdTestParam<SatdLpFunc>(256, &aom_satd_lp_c, &aom_satd_lp_sse2),
        SatdTestParam<SatdLpFunc>(1024, &aom_satd_lp_c, &aom_satd_lp_sse2)));
#endif

}  // namespace
