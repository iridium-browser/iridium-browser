/*
 * Copyright (c) 2017, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_AOM_DSP_MATHUTILS_H_
#define AOM_AOM_DSP_MATHUTILS_H_

#include <assert.h>
#include <math.h>
#include <string.h>

#include "aom_dsp/aom_dsp_common.h"
#include "aom_mem/aom_mem.h"

static const double TINY_NEAR_ZERO = 1.0E-16;

// Solves Ax = b, where x and b are column vectors of size nx1 and A is nxn
static INLINE int linsolve(int n, double *A, int stride, double *b, double *x) {
  int i, j, k;
  double c;
  // Forward elimination
  for (k = 0; k < n - 1; k++) {
    // Bring the largest magnitude to the diagonal position
    for (i = n - 1; i > k; i--) {
      if (fabs(A[(i - 1) * stride + k]) < fabs(A[i * stride + k])) {
        for (j = 0; j < n; j++) {
          c = A[i * stride + j];
          A[i * stride + j] = A[(i - 1) * stride + j];
          A[(i - 1) * stride + j] = c;
        }
        c = b[i];
        b[i] = b[i - 1];
        b[i - 1] = c;
      }
    }
    for (i = k; i < n - 1; i++) {
      if (fabs(A[k * stride + k]) < TINY_NEAR_ZERO) return 0;
      c = A[(i + 1) * stride + k] / A[k * stride + k];
      for (j = 0; j < n; j++) A[(i + 1) * stride + j] -= c * A[k * stride + j];
      b[i + 1] -= c * b[k];
    }
  }
  // Backward substitution
  for (i = n - 1; i >= 0; i--) {
    if (fabs(A[i * stride + i]) < TINY_NEAR_ZERO) return 0;
    c = 0;
    for (j = i + 1; j <= n - 1; j++) c += A[i * stride + j] * x[j];
    x[i] = (b[i] - c) / A[i * stride + i];
  }

  return 1;
}

////////////////////////////////////////////////////////////////////////////////
// Least-squares
// Solves for n-dim x in a least squares sense to minimize |Ax - b|^2
// The solution is simply x = (A'A)^-1 A'b or simply the solution for
// the system: A'A x = A'b
static INLINE int least_squares(int n, double *A, int rows, int stride,
                                double *b, double *scratch, double *x) {
  int i, j, k;
  double *scratch_ = NULL;
  double *AtA, *Atb;
  if (!scratch) {
    scratch_ = (double *)aom_malloc(sizeof(*scratch) * n * (n + 1));
    if (!scratch_) return 0;
    scratch = scratch_;
  }
  AtA = scratch;
  Atb = scratch + n * n;

  for (i = 0; i < n; ++i) {
    for (j = i; j < n; ++j) {
      AtA[i * n + j] = 0.0;
      for (k = 0; k < rows; ++k)
        AtA[i * n + j] += A[k * stride + i] * A[k * stride + j];
      AtA[j * n + i] = AtA[i * n + j];
    }
    Atb[i] = 0;
    for (k = 0; k < rows; ++k) Atb[i] += A[k * stride + i] * b[k];
  }
  int ret = linsolve(n, AtA, n, Atb, x);
  aom_free(scratch_);
  return ret;
}

// Matrix multiply
static INLINE void multiply_mat(const double *m1, const double *m2, double *res,
                                const int m1_rows, const int inner_dim,
                                const int m2_cols) {
  double sum;

  int row, col, inner;
  for (row = 0; row < m1_rows; ++row) {
    for (col = 0; col < m2_cols; ++col) {
      sum = 0;
      for (inner = 0; inner < inner_dim; ++inner)
        sum += m1[row * inner_dim + inner] * m2[inner * m2_cols + col];
      *(res++) = sum;
    }
  }
}

#endif  // AOM_AOM_DSP_MATHUTILS_H_
