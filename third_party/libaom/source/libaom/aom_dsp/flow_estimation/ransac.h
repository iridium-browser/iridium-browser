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

#ifndef AOM_AOM_DSP_FLOW_ESTIMATION_RANSAC_H_
#define AOM_AOM_DSP_FLOW_ESTIMATION_RANSAC_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#include "aom_dsp/flow_estimation/flow_estimation.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*RansacFunc)(int *matched_points, int npoints,
                          int *num_inliers_by_motion,
                          MotionModel *params_by_motion, int num_motions);
typedef int (*RansacFuncDouble)(double *matched_points, int npoints,
                                int *num_inliers_by_motion,
                                MotionModel *params_by_motion, int num_motions);
RansacFunc av1_get_ransac_type(TransformationType type);
RansacFuncDouble av1_get_ransac_double_prec_type(TransformationType type);

#ifdef __cplusplus
}
#endif

#endif  // AOM_AOM_DSP_FLOW_ESTIMATION_RANSAC_H_
