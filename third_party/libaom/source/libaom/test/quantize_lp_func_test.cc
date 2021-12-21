/*
 * Copyright (c) 2021, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <tuple>

#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

#include "config/aom_config.h"
#include "config/aom_dsp_rtcd.h"
#include "config/av1_rtcd.h"

#include "aom/aom_codec.h"
#include "aom_ports/aom_timer.h"
#include "av1/encoder/encoder.h"
#include "av1/common/scan.h"
#include "test/acm_random.h"
#include "test/register_state_check.h"
#include "test/util.h"

namespace {
using libaom_test::ACMRandom;

#define QUAN_LP_PARAM_LIST                                                 \
  const int16_t *coeff_ptr, intptr_t n_coeffs, const int16_t *round_ptr,   \
      const int16_t *quant_ptr, int16_t *qcoeff_ptr, int16_t *dqcoeff_ptr, \
      const int16_t *dequant_ptr, uint16_t *eob_ptr, const int16_t *scan,  \
      const int16_t *iscan

typedef void (*QuantizeFunc)(QUAN_LP_PARAM_LIST);

using std::tuple;
typedef tuple<QuantizeFunc, QuantizeFunc, TX_SIZE, aom_bit_depth_t>
    QuantizeParam;

typedef struct {
  QUANTS quant;
  Dequants dequant;
} QuanTable;

const int kTestNum = 1000;

template <typename CoeffType>
class QuantizeTestBase : public ::testing::TestWithParam<QuantizeParam> {
 protected:
  QuantizeTestBase()
      : quant_ref_(GET_PARAM(0)), quant_(GET_PARAM(1)), tx_size_(GET_PARAM(2)),
        bd_(GET_PARAM(3)) {}

  virtual ~QuantizeTestBase() {}

  virtual void SetUp() {
    qtab_ = reinterpret_cast<QuanTable *>(aom_memalign(32, sizeof(*qtab_)));
    const int n_coeffs = coeff_num();
    coeff_ = reinterpret_cast<CoeffType *>(
        aom_memalign(32, 6 * n_coeffs * sizeof(CoeffType)));
    InitQuantizer();
  }

  virtual void TearDown() {
    aom_free(qtab_);
    qtab_ = NULL;
    aom_free(coeff_);
    coeff_ = NULL;
  }

  void InitQuantizer() {
    av1_build_quantizer(bd_, 0, 0, 0, 0, 0, &qtab_->quant, &qtab_->dequant);
  }

  virtual void RunQuantizeFunc(const CoeffType *coeff_ptr, intptr_t n_coeffs,
                               const int16_t *round_ptr,
                               const int16_t *quant_ptr, CoeffType *qcoeff_ptr,
                               CoeffType *qcoeff_ref_ptr,
                               CoeffType *dqcoeff_ptr,
                               CoeffType *dqcoeff_ref_ptr,
                               const int16_t *dequant_ptr,
                               uint16_t *eob_ref_ptr, uint16_t *eob_ptr,
                               const int16_t *scan, const int16_t *iscan) = 0;

  void QuantizeRun(bool is_loop, int q = 0, int test_num = 1) {
    CoeffType *coeff_ptr = coeff_;
    const intptr_t n_coeffs = coeff_num();

    CoeffType *qcoeff_ref = coeff_ptr + n_coeffs;
    CoeffType *dqcoeff_ref = qcoeff_ref + n_coeffs;

    CoeffType *qcoeff = dqcoeff_ref + n_coeffs;
    CoeffType *dqcoeff = qcoeff + n_coeffs;
    uint16_t *eob = (uint16_t *)(dqcoeff + n_coeffs);

    // Testing uses 2-D DCT scan order table
    const SCAN_ORDER *const sc = get_default_scan(tx_size_, DCT_DCT);

    // Testing uses luminance quantization table
    const int16_t *round = 0;
    const int16_t *quant = 0;
    round = qtab_->quant.y_round_fp[q];
    quant = qtab_->quant.y_quant_fp[q];

    const int16_t *dequant = qtab_->dequant.y_dequant_QTX[q];

    for (int i = 0; i < test_num; ++i) {
      if (is_loop) FillCoeffRandom();

      memset(qcoeff_ref, 0, 5 * n_coeffs * sizeof(*qcoeff_ref));

      RunQuantizeFunc(coeff_ptr, n_coeffs, round, quant, qcoeff, qcoeff_ref,
                      dqcoeff, dqcoeff_ref, dequant, &eob[0], &eob[1], sc->scan,
                      sc->iscan);

      quant_ref_(coeff_ptr, n_coeffs, round, quant, qcoeff_ref, dqcoeff_ref,
                 dequant, &eob[0], sc->scan, sc->iscan);

      API_REGISTER_STATE_CHECK(quant_(coeff_ptr, n_coeffs, round, quant, qcoeff,
                                      dqcoeff, dequant, &eob[1], sc->scan,
                                      sc->iscan));

      for (int j = 0; j < n_coeffs; ++j) {
        ASSERT_EQ(qcoeff_ref[j], qcoeff[j])
            << "Q mismatch on test: " << i << " at position: " << j
            << " Q: " << q << " coeff: " << coeff_ptr[j];
      }

      for (int j = 0; j < n_coeffs; ++j) {
        ASSERT_EQ(dqcoeff_ref[j], dqcoeff[j])
            << "Dq mismatch on test: " << i << " at position: " << j
            << " Q: " << q << " coeff: " << coeff_ptr[j];
      }

      ASSERT_EQ(eob[0], eob[1])
          << "eobs mismatch on test: " << i << " Q: " << q;
    }
  }

  void CompareResults(const CoeffType *buf_ref, const CoeffType *buf, int size,
                      const char *text, int q, int number) {
    int i;
    for (i = 0; i < size; ++i) {
      ASSERT_EQ(buf_ref[i], buf[i]) << text << " mismatch on test: " << number
                                    << " at position: " << i << " Q: " << q;
    }
  }

  int coeff_num() const { return av1_get_max_eob(tx_size_); }

  void FillCoeff(CoeffType c) {
    const int n_coeffs = coeff_num();
    for (int i = 0; i < n_coeffs; ++i) {
      coeff_[i] = c;
    }
  }

  void FillCoeffRandom() {
    const int n_coeffs = coeff_num();
    FillCoeffZero();
    int num = rnd_.Rand16() % n_coeffs;
    for (int i = 0; i < num; ++i) {
      coeff_[i] = GetRandomCoeff();
    }
  }

  void FillCoeffRandomRows(int num) {
    FillCoeffZero();
    for (int i = 0; i < num; ++i) {
      coeff_[i] = GetRandomCoeff();
    }
  }

  void FillCoeffZero() { FillCoeff(0); }

  void FillCoeffConstant() {
    CoeffType c = GetRandomCoeff();
    FillCoeff(c);
  }

  void FillDcOnly() {
    FillCoeffZero();
    coeff_[0] = GetRandomCoeff();
  }

  void FillDcLargeNegative() {
    FillCoeffZero();
    // Generate a qcoeff which contains 512/-512 (0x0100/0xFE00) to catch issues
    // like BUG=883 where the constant being compared was incorrectly
    // initialized.
    coeff_[0] = -8191;
  }

  CoeffType GetRandomCoeff() {
    CoeffType coeff;
    if (bd_ == AOM_BITS_8) {
      coeff =
          clamp(static_cast<int16_t>(rnd_.Rand16()), INT16_MIN + 1, INT16_MAX);
    } else {
      CoeffType min = -(1 << (7 + bd_));
      CoeffType max = -min - 1;
      coeff = clamp(static_cast<CoeffType>(rnd_.Rand31()), min, max);
    }
    return coeff;
  }

  ACMRandom rnd_;
  QuanTable *qtab_;
  CoeffType *coeff_;
  QuantizeFunc quant_ref_;
  QuantizeFunc quant_;
  TX_SIZE tx_size_;
  aom_bit_depth_t bd_;
};

class FullPrecisionQuantizeLpTest : public QuantizeTestBase<int16_t> {
  void RunQuantizeFunc(const int16_t *coeff_ptr, intptr_t n_coeffs,
                       const int16_t *round_ptr, const int16_t *quant_ptr,
                       int16_t *qcoeff_ptr, int16_t *qcoeff_ref_ptr,
                       int16_t *dqcoeff_ptr, int16_t *dqcoeff_ref_ptr,
                       const int16_t *dequant_ptr, uint16_t *eob_ref_ptr,
                       uint16_t *eob_ptr, const int16_t *scan,
                       const int16_t *iscan) override {
    quant_ref_(coeff_ptr, n_coeffs, round_ptr, quant_ptr, qcoeff_ref_ptr,
               dqcoeff_ref_ptr, dequant_ptr, eob_ref_ptr, scan, iscan);

    API_REGISTER_STATE_CHECK(quant_(coeff_ptr, n_coeffs, round_ptr, quant_ptr,
                                    qcoeff_ptr, dqcoeff_ptr, dequant_ptr,
                                    eob_ptr, scan, iscan));
  }
};
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(FullPrecisionQuantizeLpTest);

TEST_P(FullPrecisionQuantizeLpTest, ZeroInput) {
  FillCoeffZero();
  QuantizeRun(false);
}

TEST_P(FullPrecisionQuantizeLpTest, LargeNegativeInput) {
  FillDcLargeNegative();
  QuantizeRun(false, 0, 1);
}

TEST_P(FullPrecisionQuantizeLpTest, DcOnlyInput) {
  FillDcOnly();
  QuantizeRun(false, 0, 1);
}

TEST_P(FullPrecisionQuantizeLpTest, RandomInput) {
  QuantizeRun(true, 0, kTestNum);
}

TEST_P(FullPrecisionQuantizeLpTest, MultipleQ) {
  for (int q = 0; q < QINDEX_RANGE; ++q) {
    QuantizeRun(true, q, kTestNum);
  }
}

// Force the coeff to be half the value of the dequant.  This exposes a
// mismatch found in av1_quantize_fp_sse2().
TEST_P(FullPrecisionQuantizeLpTest, CoeffHalfDequant) {
  FillCoeff(16);
  QuantizeRun(false, 25, 1);
}

TEST_P(FullPrecisionQuantizeLpTest, DISABLED_Speed) {
  int16_t *coeff_ptr = coeff_;
  const intptr_t n_coeffs = coeff_num();

  int16_t *qcoeff_ref = coeff_ptr + n_coeffs;
  int16_t *dqcoeff_ref = qcoeff_ref + n_coeffs;

  int16_t *qcoeff = dqcoeff_ref + n_coeffs;
  int16_t *dqcoeff = qcoeff + n_coeffs;
  uint16_t *eob = (uint16_t *)(dqcoeff + n_coeffs);

  // Testing uses 2-D DCT scan order table
  const SCAN_ORDER *const sc = get_default_scan(tx_size_, DCT_DCT);

  // Testing uses luminance quantization table
  const int q = 22;
  const int16_t *round_fp = qtab_->quant.y_round_fp[q];
  const int16_t *quant_fp = qtab_->quant.y_quant_fp[q];
  const int16_t *dequant = qtab_->dequant.y_dequant_QTX[q];
  const int kNumTests = 5000000;
  aom_usec_timer timer, simd_timer;
  int rows = tx_size_high[tx_size_];
  int cols = tx_size_wide[tx_size_];
  rows = AOMMIN(32, rows);
  cols = AOMMIN(32, cols);
  for (int cnt = 0; cnt <= rows; cnt++) {
    FillCoeffRandomRows(cnt * cols);

    aom_usec_timer_start(&timer);
    for (int n = 0; n < kNumTests; ++n) {
      quant_ref_(coeff_ptr, n_coeffs, round_fp, quant_fp, qcoeff, dqcoeff,
                 dequant, eob, sc->scan, sc->iscan);
    }
    aom_usec_timer_mark(&timer);

    aom_usec_timer_start(&simd_timer);
    for (int n = 0; n < kNumTests; ++n) {
      quant_(coeff_ptr, n_coeffs, round_fp, quant_fp, qcoeff, dqcoeff, dequant,
             eob, sc->scan, sc->iscan);
    }
    aom_usec_timer_mark(&simd_timer);

    const int elapsed_time = static_cast<int>(aom_usec_timer_elapsed(&timer));
    const int simd_elapsed_time =
        static_cast<int>(aom_usec_timer_elapsed(&simd_timer));
    printf("c_time = %d \t simd_time = %d \t Gain = %f \n", elapsed_time,
           simd_elapsed_time, ((float)elapsed_time / simd_elapsed_time));
  }
}

using std::make_tuple;

#if HAVE_AVX2
const QuantizeParam kQParamArrayAVX2[] = {
  // av1_quantize_lp is only called in nonrd_pickmode.c, and is used for 16X16,
  // 8X8, and 4X4.
  make_tuple(&av1_quantize_lp_c, &av1_quantize_lp_avx2,
             static_cast<TX_SIZE>(TX_16X16), AOM_BITS_8),
  make_tuple(&av1_quantize_lp_c, &av1_quantize_lp_avx2,
             static_cast<TX_SIZE>(TX_8X8), AOM_BITS_8),
  make_tuple(&av1_quantize_lp_c, &av1_quantize_lp_avx2,
             static_cast<TX_SIZE>(TX_4X4), AOM_BITS_8)
};

INSTANTIATE_TEST_SUITE_P(AVX2, FullPrecisionQuantizeLpTest,
                         ::testing::ValuesIn(kQParamArrayAVX2));
#endif

#if HAVE_SSE2
const QuantizeParam kQParamArraySSE2[] = {
  make_tuple(&av1_quantize_lp_c, &av1_quantize_lp_sse2,
             static_cast<TX_SIZE>(TX_16X16), AOM_BITS_8),
  make_tuple(&av1_quantize_lp_c, &av1_quantize_lp_sse2,
             static_cast<TX_SIZE>(TX_8X8), AOM_BITS_8),
  make_tuple(&av1_quantize_lp_c, &av1_quantize_lp_sse2,
             static_cast<TX_SIZE>(TX_4X4), AOM_BITS_8)
};

INSTANTIATE_TEST_SUITE_P(SSE2, FullPrecisionQuantizeLpTest,
                         ::testing::ValuesIn(kQParamArraySSE2));
#endif

}  // namespace
