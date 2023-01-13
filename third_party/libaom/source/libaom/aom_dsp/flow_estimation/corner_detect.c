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

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <assert.h>

#include "third_party/fastfeat/fast.h"

#include "aom_dsp/flow_estimation/corner_detect.h"

// Fast_9 wrapper
#define FAST_BARRIER 18
int av1_fast_corner_detect(unsigned char *buf, int width, int height,
                           int stride, int *points, int max_points) {
  int num_points;
  xy *const frm_corners_xy = aom_fast9_detect_nonmax(buf, width, height, stride,
                                                     FAST_BARRIER, &num_points);
  num_points = (num_points <= max_points ? num_points : max_points);
  if (num_points > 0 && frm_corners_xy) {
    memcpy(points, frm_corners_xy, sizeof(*frm_corners_xy) * num_points);
    free(frm_corners_xy);
    return num_points;
  }
  free(frm_corners_xy);
  return 0;
}
