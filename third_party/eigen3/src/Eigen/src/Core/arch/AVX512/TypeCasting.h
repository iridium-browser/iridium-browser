// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2019 Rasmus Munk Larsen <rmlarsen@google.com>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef EIGEN_TYPE_CASTING_AVX512_H
#define EIGEN_TYPE_CASTING_AVX512_H

#include "../../InternalHeaderCheck.h"

namespace Eigen {

namespace internal {

template<> EIGEN_STRONG_INLINE Packet16i pcast<Packet16f, Packet16i>(const Packet16f& a) {
  return _mm512_cvttps_epi32(a);
}

template<> EIGEN_STRONG_INLINE Packet16f pcast<Packet16i, Packet16f>(const Packet16i& a) {
  return _mm512_cvtepi32_ps(a);
}

template<> EIGEN_STRONG_INLINE Packet16i preinterpret<Packet16i, Packet16f>(const Packet16f& a) {
  return _mm512_castps_si512(a);
}

template<> EIGEN_STRONG_INLINE Packet16f preinterpret<Packet16f, Packet16i>(const Packet16i& a) {
  return _mm512_castsi512_ps(a);
}

template<> EIGEN_STRONG_INLINE Packet8d preinterpret<Packet8d, Packet16f>(const Packet16f& a) {
  return _mm512_castps_pd(a);
}

template<> EIGEN_STRONG_INLINE Packet16f preinterpret<Packet16f, Packet8d>(const Packet8d& a) {
  return _mm512_castpd_ps(a);
}

template<> EIGEN_STRONG_INLINE Packet8f preinterpret<Packet8f, Packet16f>(const Packet16f& a) {
  return _mm512_castps512_ps256(a);
}

template<> EIGEN_STRONG_INLINE Packet4f preinterpret<Packet4f, Packet16f>(const Packet16f& a) {
  return _mm512_castps512_ps128(a);
}

template<> EIGEN_STRONG_INLINE Packet4d preinterpret<Packet4d, Packet8d>(const Packet8d& a) {
  return _mm512_castpd512_pd256(a);
}

template<> EIGEN_STRONG_INLINE Packet2d preinterpret<Packet2d, Packet8d>(const Packet8d& a) {
  return _mm512_castpd512_pd128(a);
}

template<> EIGEN_STRONG_INLINE Packet16f preinterpret<Packet16f, Packet8f>(const Packet8f& a) {
  return _mm512_castps256_ps512(a);
}

template<> EIGEN_STRONG_INLINE Packet16f preinterpret<Packet16f, Packet4f>(const Packet4f& a) {
  return _mm512_castps128_ps512(a);
}

template<> EIGEN_STRONG_INLINE Packet8d preinterpret<Packet8d, Packet4d>(const Packet4d& a) {
  return _mm512_castpd256_pd512(a);
}

template<> EIGEN_STRONG_INLINE Packet8d preinterpret<Packet8d, Packet2d>(const Packet2d& a) {
  return _mm512_castpd128_pd512(a);
}

template<> EIGEN_STRONG_INLINE Packet16f preinterpret<Packet16f, Packet16f>(const Packet16f& a) {
  return a;
}

template<> EIGEN_STRONG_INLINE Packet8d preinterpret<Packet8d, Packet8d>(const Packet8d& a) {
  return a;
}

#ifndef EIGEN_VECTORIZE_AVX512FP16

template <>
struct type_casting_traits<half, float> {
  enum {
    VectorizedCast = 1,
    SrcCoeffRatio = 1,
    TgtCoeffRatio = 1
  };
};

template<> EIGEN_STRONG_INLINE Packet16f pcast<Packet16h, Packet16f>(const Packet16h& a) {
  return half2float(a);
}

template <>
struct type_casting_traits<float, half> {
  enum {
    VectorizedCast = 1,
    SrcCoeffRatio = 1,
    TgtCoeffRatio = 1
  };
};

template<> EIGEN_STRONG_INLINE Packet16h pcast<Packet16f, Packet16h>(const Packet16f& a) {
  return float2half(a);
}

#endif

template <>
struct type_casting_traits<bfloat16, float> {
  enum {
    VectorizedCast = 1,
    SrcCoeffRatio = 1,
    TgtCoeffRatio = 1
  };
};

template<> EIGEN_STRONG_INLINE Packet16f pcast<Packet16bf, Packet16f>(const Packet16bf& a) {
  return Bf16ToF32(a);
}

template <>
struct type_casting_traits<float, bfloat16> {
  enum {
    VectorizedCast = 1,
    SrcCoeffRatio = 1,
    TgtCoeffRatio = 1
  };
};

template<> EIGEN_STRONG_INLINE Packet16bf pcast<Packet16f, Packet16bf>(const Packet16f& a) {
  return F32ToBf16(a);
}

#ifdef EIGEN_VECTORIZE_AVX512FP16

template <>
struct type_casting_traits<half, float> {
  enum {
    VectorizedCast = 1,
    SrcCoeffRatio = 1,
    TgtCoeffRatio = 2
  };
};

template <>
struct type_casting_traits<float, half> {
  enum {
    VectorizedCast = 1,
    SrcCoeffRatio = 2,
    TgtCoeffRatio = 1
  };
};

template <>
EIGEN_STRONG_INLINE Packet16f pcast<Packet32h, Packet16f>(const Packet32h& a) {
  // Discard second-half of input.
  Packet16h low = _mm256_castpd_si256(_mm512_extractf64x4_pd(_mm512_castph_pd(a), 0));
  return _mm512_cvtxph_ps(_mm256_castsi256_ph(low));
}


template <>
EIGEN_STRONG_INLINE Packet32h pcast<Packet16f, Packet32h>(const Packet16f& a, const Packet16f& b) {
  __m512d result = _mm512_undefined_pd();
  result = _mm512_insertf64x4(result, _mm256_castsi256_pd(_mm512_cvtps_ph(a, _MM_FROUND_TO_NEAREST_INT|_MM_FROUND_NO_EXC)), 0);
  result = _mm512_insertf64x4(result, _mm256_castsi256_pd(_mm512_cvtps_ph(b, _MM_FROUND_TO_NEAREST_INT|_MM_FROUND_NO_EXC)), 1);
  return _mm512_castpd_ph(result);
}

template <>
EIGEN_STRONG_INLINE Packet8f pcast<Packet16h, Packet8f>(const Packet16h& a) {
  // Discard second-half of input.
  Packet8h low = _mm_castps_si128(_mm256_extractf32x4_ps(_mm256_castsi256_ps(a), 0));
  return _mm256_cvtxph_ps(_mm_castsi128_ph(low));
}


template <>
EIGEN_STRONG_INLINE Packet16h pcast<Packet8f, Packet16h>(const Packet8f& a, const Packet8f& b) {
  __m256d result = _mm256_undefined_pd();
  result = _mm256_insertf64x2(result, _mm_castsi128_pd(_mm256_cvtps_ph(a, _MM_FROUND_TO_NEAREST_INT|_MM_FROUND_NO_EXC)), 0);
  result = _mm256_insertf64x2(result, _mm_castsi128_pd(_mm256_cvtps_ph(b, _MM_FROUND_TO_NEAREST_INT|_MM_FROUND_NO_EXC)), 1);
  return _mm256_castpd_si256(result);
}

template <>
EIGEN_STRONG_INLINE Packet4f pcast<Packet8h, Packet4f>(const Packet8h& a) {
  Packet8f full = _mm256_cvtxph_ps(_mm_castsi128_ph(a));
  // Discard second-half of input.
  return _mm256_extractf32x4_ps(full, 0);
}


template <>
EIGEN_STRONG_INLINE Packet8h pcast<Packet4f, Packet8h>(const Packet4f& a, const Packet4f& b) {
  __m256 result = _mm256_undefined_ps();
  result = _mm256_insertf128_ps(result, a, 0);
  result = _mm256_insertf128_ps(result, b, 1);
  return _mm256_cvtps_ph(result, _MM_FROUND_TO_NEAREST_INT|_MM_FROUND_NO_EXC);
}


#endif

} // end namespace internal

} // end namespace Eigen

#endif // EIGEN_TYPE_CASTING_AVX512_H
