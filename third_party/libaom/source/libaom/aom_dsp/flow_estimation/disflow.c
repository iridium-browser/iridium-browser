/*
 * Copyright (c) 2022, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 3-Clause Clear License
 * and the Alliance for Open Media Patent License 1.0. If the BSD 3-Clause Clear
 * License was not distributed with this source code in the LICENSE file, you
 * can obtain it at aomedia.org/license/software-license/bsd-3-c-c/.  If the
 * Alliance for Open Media Patent License 1.0 was not distributed with this
 * source code in the PATENTS file, you can obtain it at
 * aomedia.org/license/patent-license/.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "aom_dsp/flow_estimation/disflow.h"
#include "aom_dsp/flow_estimation/flow_estimation.h"
#include "aom_dsp/flow_estimation/ransac.h"

#include "aom_scale/yv12config.h"

#include "av1/common/resize.h"

// Number of pyramid levels in disflow computation
#define N_LEVELS 2
// Size of square patches in the disflow dense grid
#define PATCH_SIZE 8
// Center point of square patch
#define PATCH_CENTER ((PATCH_SIZE + 1) >> 1)
// Step size between patches, lower value means greater patch overlap
#define PATCH_STEP 1
// Minimum size of border padding for disflow
#define MIN_PAD 7
// Warp error convergence threshold for disflow
#define DISFLOW_ERROR_TR 0.01
// Max number of iterations if warp convergence is not found
#define DISFLOW_MAX_ITR 10

// Struct for an image pyramid
typedef struct {
  int n_levels;
  int pad_size;
  int has_gradient;
  int widths[N_LEVELS];
  int heights[N_LEVELS];
  int strides[N_LEVELS];
  int level_loc[N_LEVELS];
  unsigned char *level_buffer;
  double *level_dx_buffer;
  double *level_dy_buffer;
} ImagePyramid;

// Don't use points around the frame border since they are less reliable
static INLINE int valid_point(int x, int y, int width, int height) {
  return (x > (PATCH_SIZE + PATCH_CENTER)) &&
         (x < (width - PATCH_SIZE - PATCH_CENTER)) &&
         (y > (PATCH_SIZE + PATCH_CENTER)) &&
         (y < (height - PATCH_SIZE - PATCH_CENTER));
}

static int determine_disflow_correspondence(int *frm_corners,
                                            int num_frm_corners, double *flow_u,
                                            double *flow_v, int width,
                                            int height, int stride,
                                            double *correspondences) {
  int num_correspondences = 0;
  int x, y;
  for (int i = 0; i < num_frm_corners; ++i) {
    x = frm_corners[2 * i];
    y = frm_corners[2 * i + 1];
    if (valid_point(x, y, width, height)) {
      correspondences[4 * num_correspondences] = x;
      correspondences[4 * num_correspondences + 1] = y;
      correspondences[4 * num_correspondences + 2] = x + flow_u[y * stride + x];
      correspondences[4 * num_correspondences + 3] = y + flow_v[y * stride + x];
      num_correspondences++;
    }
  }
  return num_correspondences;
}

static double getCubicValue(double p[4], double x) {
  return p[1] + 0.5 * x *
                    (p[2] - p[0] +
                     x * (2.0 * p[0] - 5.0 * p[1] + 4.0 * p[2] - p[3] +
                          x * (3.0 * (p[1] - p[2]) + p[3] - p[0])));
}

static void get_subcolumn(unsigned char *ref, double col[4], int stride, int x,
                          int y_start) {
  int i;
  for (i = 0; i < 4; ++i) {
    col[i] = ref[(i + y_start) * stride + x];
  }
}

static double bicubic(unsigned char *ref, double x, double y, int stride) {
  double arr[4];
  int k;
  int i = (int)x;
  int j = (int)y;
  for (k = 0; k < 4; ++k) {
    double arr_temp[4];
    get_subcolumn(ref, arr_temp, stride, i + k - 1, j - 1);
    arr[k] = getCubicValue(arr_temp, y - j);
  }
  return getCubicValue(arr, x - i);
}

// Interpolate a warped block using bicubic interpolation when possible
static unsigned char interpolate(unsigned char *ref, double x, double y,
                                 int width, int height, int stride) {
  if (x < 0 && y < 0)
    return ref[0];
  else if (x < 0 && y > height - 1)
    return ref[(height - 1) * stride];
  else if (x > width - 1 && y < 0)
    return ref[width - 1];
  else if (x > width - 1 && y > height - 1)
    return ref[(height - 1) * stride + (width - 1)];
  else if (x < 0) {
    int v;
    int i = (int)y;
    double a = y - i;
    if (y > 1 && y < height - 2) {
      double arr[4];
      get_subcolumn(ref, arr, stride, 0, i - 1);
      return clamp((int)(getCubicValue(arr, a) + 0.5), 0, 255);
    }
    v = (int)(ref[i * stride] * (1 - a) + ref[(i + 1) * stride] * a + 0.5);
    return clamp(v, 0, 255);
  } else if (y < 0) {
    int v;
    int j = (int)x;
    double b = x - j;
    if (x > 1 && x < width - 2) {
      double arr[4] = { ref[j - 1], ref[j], ref[j + 1], ref[j + 2] };
      return clamp((int)(getCubicValue(arr, b) + 0.5), 0, 255);
    }
    v = (int)(ref[j] * (1 - b) + ref[j + 1] * b + 0.5);
    return clamp(v, 0, 255);
  } else if (x > width - 1) {
    int v;
    int i = (int)y;
    double a = y - i;
    if (y > 1 && y < height - 2) {
      double arr[4];
      get_subcolumn(ref, arr, stride, width - 1, i - 1);
      return clamp((int)(getCubicValue(arr, a) + 0.5), 0, 255);
    }
    v = (int)(ref[i * stride + width - 1] * (1 - a) +
              ref[(i + 1) * stride + width - 1] * a + 0.5);
    return clamp(v, 0, 255);
  } else if (y > height - 1) {
    int v;
    int j = (int)x;
    double b = x - j;
    if (x > 1 && x < width - 2) {
      int row = (height - 1) * stride;
      double arr[4] = { ref[row + j - 1], ref[row + j], ref[row + j + 1],
                        ref[row + j + 2] };
      return clamp((int)(getCubicValue(arr, b) + 0.5), 0, 255);
    }
    v = (int)(ref[(height - 1) * stride + j] * (1 - b) +
              ref[(height - 1) * stride + j + 1] * b + 0.5);
    return clamp(v, 0, 255);
  } else if (x > 1 && y > 1 && x < width - 2 && y < height - 2) {
    return clamp((int)(bicubic(ref, x, y, stride) + 0.5), 0, 255);
  } else {
    int i = (int)y;
    int j = (int)x;
    double a = y - i;
    double b = x - j;
    int v = (int)(ref[i * stride + j] * (1 - a) * (1 - b) +
                  ref[i * stride + j + 1] * (1 - a) * b +
                  ref[(i + 1) * stride + j] * a * (1 - b) +
                  ref[(i + 1) * stride + j + 1] * a * b);
    return clamp(v, 0, 255);
  }
}

// Warps a block using flow vector [u, v] and computes the mse
static double compute_warp_and_error(unsigned char *ref, unsigned char *frm,
                                     int width, int height, int stride, int x,
                                     int y, double u, double v, int16_t *dt) {
  int i, j;
  unsigned char warped;
  double x_w, y_w;
  double mse = 0;
  int16_t err = 0;
  for (i = y; i < y + PATCH_SIZE; ++i)
    for (j = x; j < x + PATCH_SIZE; ++j) {
      x_w = (double)j + u;
      y_w = (double)i + v;
      warped = interpolate(ref, x_w, y_w, width, height, stride);
      err = warped - frm[j + i * stride];
      mse += err * err;
      dt[(i - y) * PATCH_SIZE + (j - x)] = err;
    }

  mse /= (PATCH_SIZE * PATCH_SIZE);
  return mse;
}

// Computes the components of the system of equations used to solve for
// a flow vector. This includes:
// 1.) The hessian matrix for optical flow. This matrix is in the
// form of:
//
//       M = |sum(dx * dx)  sum(dx * dy)|
//           |sum(dx * dy)  sum(dy * dy)|
//
// 2.)   b = |sum(dx * dt)|
//           |sum(dy * dt)|
// Where the sums are computed over a square window of PATCH_SIZE.
static INLINE void compute_flow_system(const double *dx, int dx_stride,
                                       const double *dy, int dy_stride,
                                       const int16_t *dt, int dt_stride,
                                       double *M, double *b) {
  for (int i = 0; i < PATCH_SIZE; i++) {
    for (int j = 0; j < PATCH_SIZE; j++) {
      M[0] += dx[i * dx_stride + j] * dx[i * dx_stride + j];
      M[1] += dx[i * dx_stride + j] * dy[i * dy_stride + j];
      M[3] += dy[i * dy_stride + j] * dy[i * dy_stride + j];

      b[0] += dx[i * dx_stride + j] * dt[i * dt_stride + j];
      b[1] += dy[i * dy_stride + j] * dt[i * dt_stride + j];
    }
  }

  M[2] = M[1];
}

// Solves a general Mx = b where M is a 2x2 matrix and b is a 2x1 matrix
static INLINE void solve_2x2_system(const double *M, const double *b,
                                    double *output_vec) {
  double M_0 = M[0];
  double M_3 = M[3];
  double det = (M_0 * M_3) - (M[1] * M[2]);
  if (det < 1e-5) {
    // Handle singular matrix
    // TODO(sarahparker) compare results using pseudo inverse instead
    M_0 += 1e-10;
    M_3 += 1e-10;
    det = (M_0 * M_3) - (M[1] * M[2]);
  }
  const double det_inv = 1 / det;
  const double mult_b0 = det_inv * b[0];
  const double mult_b1 = det_inv * b[1];
  output_vec[0] = M_3 * mult_b0 - M[1] * mult_b1;
  output_vec[1] = -M[2] * mult_b0 + M_0 * mult_b1;
}

/*
static INLINE void image_difference(const uint8_t *src, int src_stride,
                                    const uint8_t *ref, int ref_stride,
                                    int16_t *dst, int dst_stride, int height,
                                    int width) {
  const int block_unit = 8;
  // Take difference in 8x8 blocks to make use of optimized diff function
  for (int i = 0; i < height; i += block_unit) {
    for (int j = 0; j < width; j += block_unit) {
      aom_subtract_block(block_unit, block_unit, dst + i * dst_stride + j,
                         dst_stride, src + i * src_stride + j, src_stride,
                         ref + i * ref_stride + j, ref_stride);
    }
  }
}
*/

static INLINE void convolve_2d_sobel_y(const uint8_t *src, int src_stride,
                                       double *dst, int dst_stride, int w,
                                       int h, int dir, double norm) {
  int16_t im_block[(MAX_SB_SIZE + MAX_FILTER_TAP - 1) * MAX_SB_SIZE];
  DECLARE_ALIGNED(256, static const int16_t, sobel_a[3]) = { 1, 0, -1 };
  DECLARE_ALIGNED(256, static const int16_t, sobel_b[3]) = { 1, 2, 1 };
  const int taps = 3;
  int im_h = h + taps - 1;
  int im_stride = w;
  const int fo_vert = 1;
  const int fo_horiz = 1;

  // horizontal filter
  const uint8_t *src_horiz = src - fo_vert * src_stride;
  const int16_t *x_filter = dir ? sobel_a : sobel_b;
  for (int y = 0; y < im_h; ++y) {
    for (int x = 0; x < w; ++x) {
      int16_t sum = 0;
      for (int k = 0; k < taps; ++k) {
        sum += x_filter[k] * src_horiz[y * src_stride + x - fo_horiz + k];
      }
      im_block[y * im_stride + x] = sum;
    }
  }

  // vertical filter
  int16_t *src_vert = im_block + fo_vert * im_stride;
  const int16_t *y_filter = dir ? sobel_b : sobel_a;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      int16_t sum = 0;
      for (int k = 0; k < taps; ++k) {
        sum += y_filter[k] * src_vert[(y - fo_vert + k) * im_stride + x];
      }
      dst[y * dst_stride + x] = sum * norm;
    }
  }
}

// Compute an image gradient using a sobel filter.
// If dir == 1, compute the x gradient. If dir == 0, compute y. This function
// assumes the images have been padded so that they can be processed in units
// of 8.
static INLINE void sobel_xy_image_gradient(const uint8_t *src, int src_stride,
                                           double *dst, int dst_stride,
                                           int height, int width, int dir) {
  double norm = 1.0;
  // TODO(sarahparker) experiment with doing this over larger block sizes
  const int block_unit = 8;
  // Filter in 8x8 blocks to eventually make use of optimized convolve function
  for (int i = 0; i < height; i += block_unit) {
    for (int j = 0; j < width; j += block_unit) {
      convolve_2d_sobel_y(src + i * src_stride + j, src_stride,
                          dst + i * dst_stride + j, dst_stride, block_unit,
                          block_unit, dir, norm);
    }
  }
}

static void free_pyramid(ImagePyramid *pyr) {
  aom_free(pyr->level_buffer);
  if (pyr->has_gradient) {
    aom_free(pyr->level_dx_buffer);
    aom_free(pyr->level_dy_buffer);
  }
  aom_free(pyr);
}

static ImagePyramid *alloc_pyramid(int width, int height, int pad_size,
                                   int compute_gradient) {
  ImagePyramid *pyr = aom_calloc(1, sizeof(*pyr));
  if (!pyr) return NULL;
  pyr->has_gradient = compute_gradient;
  // 2 * width * height is the upper bound for a buffer that fits
  // all pyramid levels + padding for each level
  const int buffer_size = sizeof(*pyr->level_buffer) * 2 * width * height +
                          (width + 2 * pad_size) * 2 * pad_size * N_LEVELS;
  pyr->level_buffer = aom_malloc(buffer_size);
  if (!pyr->level_buffer) {
    free_pyramid(pyr);
    return NULL;
  }
  memset(pyr->level_buffer, 0, buffer_size);

  if (compute_gradient) {
    const int gradient_size =
        sizeof(*pyr->level_dx_buffer) * 2 * width * height +
        (width + 2 * pad_size) * 2 * pad_size * N_LEVELS;
    pyr->level_dx_buffer = aom_calloc(1, gradient_size);
    pyr->level_dy_buffer = aom_calloc(1, gradient_size);
    if (!(pyr->level_dx_buffer && pyr->level_dy_buffer)) {
      free_pyramid(pyr);
      return NULL;
    }
  }
  return pyr;
}

static INLINE void update_level_dims(ImagePyramid *frm_pyr, int level) {
  frm_pyr->widths[level] = frm_pyr->widths[level - 1] >> 1;
  frm_pyr->heights[level] = frm_pyr->heights[level - 1] >> 1;
  frm_pyr->strides[level] = frm_pyr->widths[level] + 2 * frm_pyr->pad_size;
  // Point the beginning of the next level buffer to the correct location inside
  // the padded border
  frm_pyr->level_loc[level] =
      frm_pyr->level_loc[level - 1] +
      frm_pyr->strides[level - 1] *
          (2 * frm_pyr->pad_size + frm_pyr->heights[level - 1]);
}

// Compute coarse to fine pyramids for a frame
static void compute_flow_pyramids(unsigned char *frm, const int frm_width,
                                  const int frm_height, const int frm_stride,
                                  int n_levels, int pad_size, int compute_grad,
                                  ImagePyramid *frm_pyr) {
  int cur_width, cur_height, cur_stride, cur_loc;
  assert((frm_width >> n_levels) > 0);
  assert((frm_height >> n_levels) > 0);

  // Initialize first level
  frm_pyr->n_levels = n_levels;
  frm_pyr->pad_size = pad_size;
  frm_pyr->widths[0] = frm_width;
  frm_pyr->heights[0] = frm_height;
  frm_pyr->strides[0] = frm_width + 2 * frm_pyr->pad_size;
  // Point the beginning of the level buffer to the location inside
  // the padded border
  frm_pyr->level_loc[0] =
      frm_pyr->strides[0] * frm_pyr->pad_size + frm_pyr->pad_size;
  // This essentially copies the original buffer into the pyramid buffer
  // without the original padding
  av1_resize_plane(frm, frm_height, frm_width, frm_stride,
                   frm_pyr->level_buffer + frm_pyr->level_loc[0],
                   frm_pyr->heights[0], frm_pyr->widths[0],
                   frm_pyr->strides[0]);

  if (compute_grad) {
    cur_width = frm_pyr->widths[0];
    cur_height = frm_pyr->heights[0];
    cur_stride = frm_pyr->strides[0];
    cur_loc = frm_pyr->level_loc[0];
    assert(frm_pyr->has_gradient && frm_pyr->level_dx_buffer != NULL &&
           frm_pyr->level_dy_buffer != NULL);
    // Computation x gradient
    sobel_xy_image_gradient(frm_pyr->level_buffer + cur_loc, cur_stride,
                            frm_pyr->level_dx_buffer + cur_loc, cur_stride,
                            cur_height, cur_width, 1);

    // Computation y gradient
    sobel_xy_image_gradient(frm_pyr->level_buffer + cur_loc, cur_stride,
                            frm_pyr->level_dy_buffer + cur_loc, cur_stride,
                            cur_height, cur_width, 0);
  }

  // Start at the finest level and resize down to the coarsest level
  for (int level = 1; level < n_levels; ++level) {
    update_level_dims(frm_pyr, level);
    cur_width = frm_pyr->widths[level];
    cur_height = frm_pyr->heights[level];
    cur_stride = frm_pyr->strides[level];
    cur_loc = frm_pyr->level_loc[level];

    av1_resize_plane(frm_pyr->level_buffer + frm_pyr->level_loc[level - 1],
                     frm_pyr->heights[level - 1], frm_pyr->widths[level - 1],
                     frm_pyr->strides[level - 1],
                     frm_pyr->level_buffer + cur_loc, cur_height, cur_width,
                     cur_stride);

    if (compute_grad) {
      assert(frm_pyr->has_gradient && frm_pyr->level_dx_buffer != NULL &&
             frm_pyr->level_dy_buffer != NULL);
      // Computation x gradient
      sobel_xy_image_gradient(frm_pyr->level_buffer + cur_loc, cur_stride,
                              frm_pyr->level_dx_buffer + cur_loc, cur_stride,
                              cur_height, cur_width, 1);

      // Computation y gradient
      sobel_xy_image_gradient(frm_pyr->level_buffer + cur_loc, cur_stride,
                              frm_pyr->level_dy_buffer + cur_loc, cur_stride,
                              cur_height, cur_width, 0);
    }
  }
}

static INLINE void compute_flow_at_point(unsigned char *frm, unsigned char *ref,
                                         double *dx, double *dy, int x, int y,
                                         int width, int height, int stride,
                                         double *u, double *v) {
  double M[4] = { 0 };
  double b[2] = { 0 };
  double tmp_output_vec[2] = { 0 };
  double error = 0;
  int16_t dt[PATCH_SIZE * PATCH_SIZE];
  double o_u = *u;
  double o_v = *v;

  for (int itr = 0; itr < DISFLOW_MAX_ITR; itr++) {
    error = compute_warp_and_error(ref, frm, width, height, stride, x, y, *u,
                                   *v, dt);
    if (error <= DISFLOW_ERROR_TR) break;
    compute_flow_system(dx, stride, dy, stride, dt, PATCH_SIZE, M, b);
    solve_2x2_system(M, b, tmp_output_vec);
    *u += tmp_output_vec[0];
    *v += tmp_output_vec[1];
  }
  if (fabs(*u - o_u) > PATCH_SIZE || fabs(*v - o_u) > PATCH_SIZE) {
    *u = o_u;
    *v = o_v;
  }
}

// make sure flow_u and flow_v start at 0
static bool compute_flow_field(ImagePyramid *frm_pyr, ImagePyramid *ref_pyr,
                               double *flow_u, double *flow_v) {
  int cur_width, cur_height, cur_stride, cur_loc, patch_loc, patch_center;
  double *u_upscale =
      aom_malloc(frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_u));
  double *v_upscale =
      aom_malloc(frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_v));
  if (!(u_upscale && v_upscale)) {
    aom_free(u_upscale);
    aom_free(v_upscale);
    return false;
  }

  assert(frm_pyr->n_levels == ref_pyr->n_levels);

  // Compute flow field from coarsest to finest level of the pyramid
  for (int level = frm_pyr->n_levels - 1; level >= 0; --level) {
    cur_width = frm_pyr->widths[level];
    cur_height = frm_pyr->heights[level];
    cur_stride = frm_pyr->strides[level];
    cur_loc = frm_pyr->level_loc[level];

    for (int i = PATCH_SIZE; i < cur_height - PATCH_SIZE; i += PATCH_STEP) {
      for (int j = PATCH_SIZE; j < cur_width - PATCH_SIZE; j += PATCH_STEP) {
        patch_loc = i * cur_stride + j;
        patch_center = patch_loc + PATCH_CENTER * cur_stride + PATCH_CENTER;
        compute_flow_at_point(frm_pyr->level_buffer + cur_loc,
                              ref_pyr->level_buffer + cur_loc,
                              frm_pyr->level_dx_buffer + cur_loc + patch_loc,
                              frm_pyr->level_dy_buffer + cur_loc + patch_loc, j,
                              i, cur_width, cur_height, cur_stride,
                              flow_u + patch_center, flow_v + patch_center);
      }
    }
    // TODO(sarahparker) Replace this with upscale function in resize.c
    if (level > 0) {
      int h_upscale = frm_pyr->heights[level - 1];
      int w_upscale = frm_pyr->widths[level - 1];
      int s_upscale = frm_pyr->strides[level - 1];
      for (int i = 0; i < h_upscale; ++i) {
        for (int j = 0; j < w_upscale; ++j) {
          u_upscale[j + i * s_upscale] =
              flow_u[(int)(j >> 1) + (int)(i >> 1) * cur_stride];
          v_upscale[j + i * s_upscale] =
              flow_v[(int)(j >> 1) + (int)(i >> 1) * cur_stride];
        }
      }
      memcpy(flow_u, u_upscale,
             frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_u));
      memcpy(flow_v, v_upscale,
             frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_v));
    }
  }
  aom_free(u_upscale);
  aom_free(v_upscale);
  return true;
}

int av1_compute_global_motion_disflow_based(
    TransformationType type, unsigned char *frm_buffer, int frm_width,
    int frm_height, int frm_stride, int *frm_corners, int num_frm_corners,
    YV12_BUFFER_CONFIG *ref, int bit_depth, int *num_inliers_by_motion,
    MotionModel *params_by_motion, int num_motions) {
  unsigned char *ref_buffer = ref->y_buffer;
  const int ref_width = ref->y_width;
  const int ref_height = ref->y_height;
  const int pad_size = AOMMAX(PATCH_SIZE, MIN_PAD);
  int num_correspondences;
  double *correspondences;
  RansacFuncDouble ransac = av1_get_ransac_double_prec_type(type);
  assert(frm_width == ref_width);
  assert(frm_height == ref_height);

  // Ensure the number of pyramid levels will work with the frame resolution
  const int msb =
      frm_width < frm_height ? get_msb(frm_width) : get_msb(frm_height);
  const int n_levels = AOMMIN(msb, N_LEVELS);

  if (ref->flags & YV12_FLAG_HIGHBITDEPTH) {
    ref_buffer = av1_downconvert_frame(ref, bit_depth);
  }

  // TODO(sarahparker) We will want to do the source pyramid computation
  // outside of this function so it doesn't get recomputed for every
  // reference. We also don't need to compute every pyramid level for the
  // reference in advance, since lower levels can be overwritten once their
  // flow field is computed and upscaled. I'll add these optimizations
  // once the full implementation is working.
  // Allocate frm image pyramids
  int compute_gradient = 1;
  ImagePyramid *frm_pyr =
      alloc_pyramid(frm_width, frm_height, pad_size, compute_gradient);
  if (!frm_pyr) return 0;
  compute_flow_pyramids(frm_buffer, frm_width, frm_height, frm_stride, n_levels,
                        pad_size, compute_gradient, frm_pyr);
  // Allocate ref image pyramids
  compute_gradient = 0;
  ImagePyramid *ref_pyr =
      alloc_pyramid(ref_width, ref_height, pad_size, compute_gradient);
  if (!ref_pyr) {
    free_pyramid(frm_pyr);
    return 0;
  }
  compute_flow_pyramids(ref_buffer, ref_width, ref_height, ref->y_stride,
                        n_levels, pad_size, compute_gradient, ref_pyr);

  int ret = 0;
  double *flow_u =
      aom_malloc(frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_u));
  double *flow_v =
      aom_malloc(frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_v));
  if (!(flow_u && flow_v)) goto Error;

  memset(flow_u, 0,
         frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_u));
  memset(flow_v, 0,
         frm_pyr->strides[0] * frm_pyr->heights[0] * sizeof(*flow_v));

  if (!compute_flow_field(frm_pyr, ref_pyr, flow_u, flow_v)) goto Error;

  // find correspondences between the two images using the flow field
  correspondences = aom_malloc(num_frm_corners * 4 * sizeof(*correspondences));
  if (!correspondences) goto Error;
  num_correspondences = determine_disflow_correspondence(
      frm_corners, num_frm_corners, flow_u, flow_v, frm_width, frm_height,
      frm_pyr->strides[0], correspondences);
  ransac(correspondences, num_correspondences, num_inliers_by_motion,
         params_by_motion, num_motions);

  // Set num_inliers = 0 for motions with too few inliers so they are ignored.
  for (int i = 0; i < num_motions; ++i) {
    if (num_inliers_by_motion[i] < MIN_INLIER_PROB * num_correspondences) {
      num_inliers_by_motion[i] = 0;
    }
  }

  // Return true if any one of the motions has inliers.
  for (int i = 0; i < num_motions; ++i) {
    if (num_inliers_by_motion[i] > 0) {
      ret = 1;
      break;
    }
  }

  aom_free(correspondences);
Error:
  free_pyramid(frm_pyr);
  free_pyramid(ref_pyr);
  aom_free(flow_u);
  aom_free(flow_v);
  return ret;
}
