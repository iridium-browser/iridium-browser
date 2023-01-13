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

#ifndef AOM_AOM_DSP_FLOW_ESTIMATION_CORNER_DETECT_H_
#define AOM_AOM_DSP_FLOW_ESTIMATION_CORNER_DETECT_H_

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#ifdef __cplusplus
extern "C" {
#endif

int av1_fast_corner_detect(unsigned char *buf, int width, int height,
                           int stride, int *points, int max_points);

#ifdef __cplusplus
}
#endif

#endif  // AOM_AOM_DSP_FLOW_ESTIMATION_CORNER_DETECT_H_
