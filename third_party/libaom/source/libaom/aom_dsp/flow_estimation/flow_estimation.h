/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_AOM_DSP_FLOW_ESTIMATION_H_
#define AOM_AOM_DSP_FLOW_ESTIMATION_H_

#include "aom_ports/mem.h"
#include "aom_scale/yv12config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PARAMDIM 9
#define MAX_CORNERS 4096
#define MIN_INLIER_PROB 0.1

/* clang-format off */
enum {
  IDENTITY = 0,      // identity transformation, 0-parameter
  TRANSLATION = 1,   // translational motion 2-parameter
  ROTZOOM = 2,       // simplified affine with rotation + zoom only, 4-parameter
  AFFINE = 3,        // affine, 6-parameter
  TRANS_TYPES,
} UENUM1BYTE(TransformationType);
/* clang-format on */

// number of parameters used by each transformation in TransformationTypes
static const int trans_model_params[TRANS_TYPES] = { 0, 2, 4, 6 };

typedef enum {
  GLOBAL_MOTION_FEATURE_BASED,
  GLOBAL_MOTION_DISFLOW_BASED,
} GlobalMotionEstimationType;

typedef struct {
  double params[MAX_PARAMDIM - 1];
  int *inliers;
  int num_inliers;
} MotionModel;

int aom_compute_global_motion(TransformationType type,
                              unsigned char *src_buffer, int src_width,
                              int src_height, int src_stride, int *src_corners,
                              int num_src_corners, YV12_BUFFER_CONFIG *ref,
                              int bit_depth,
                              GlobalMotionEstimationType gm_estimation_type,
                              int *num_inliers_by_motion,
                              MotionModel *params_by_motion, int num_motions);

unsigned char *av1_downconvert_frame(YV12_BUFFER_CONFIG *frm, int bit_depth);

#ifdef __cplusplus
}
#endif

#endif  // AOM_AOM_DSP_FLOW_ESTIMATION_H_
