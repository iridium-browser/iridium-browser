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

#ifndef AOM_AV1_ENCODER_THIRDPASS_H_
#define AOM_AV1_ENCODER_THIRDPASS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "av1/encoder/firstpass.h"
#include "av1/encoder/ratectrl.h"

// TODO(bohanli): optimize this number
#define MAX_THIRD_PASS_BUF (2 * MAX_GF_INTERVAL + 1)

// Struct to store useful information about a frame for the third pass.
// The members are extracted from the decoder by function get_frame_info.
typedef struct {
  int base_q_idx;
  int is_show_existing_frame;
  int is_show_frame;
  FRAME_TYPE frame_type;
  unsigned int order_hint;
} THIRD_PASS_FRAME_INFO;

typedef struct {
  /* --- Input and decoding related members --- */
  // the input file
  const char *input_file_name;
#if CONFIG_THREE_PASS
  // input context
  struct AvxInputContext *input_ctx;
#endif
  // decoder codec context
  aom_codec_ctx_t decoder;
  // start of the frame in buf
  const unsigned char *frame;
  // end of the frame(s) in buf
  const unsigned char *end_frame;
  // whether we still have following frames in buf
  int have_frame;
  // pointer to buffer for the read frames
  uint8_t *buf;
  // size of data in buffer
  size_t bytes_in_buffer;
  // current buffer size
  size_t buffer_size;
  // error info pointer
  struct aom_internal_error_info *err_info;

  /* --- Members for third pass encoding --- */
  // Array to store info about each frame.
  // frame_info[0] should point to the current frame.
  THIRD_PASS_FRAME_INFO frame_info[MAX_THIRD_PASS_BUF];
  // number of frames available in frame_info
  int frame_info_count;
  // the end of the previous GOP (order hint)
  int prev_gop_end;
} THIRD_PASS_DEC_CTX;

void av1_init_thirdpass_ctx(AV1_COMMON *cm, THIRD_PASS_DEC_CTX **ctx,
                            const char *file);
void av1_free_thirdpass_ctx(THIRD_PASS_DEC_CTX *ctx);

// Set the GOP structure from the twopass bitstream.
// TODO(bohanli): this is currently a skeleton and we only return the gop
// length. This function also saves all frame information in the array
// ctx->frame_info for this GOP.
void av1_set_gop_third_pass(THIRD_PASS_DEC_CTX *ctx, GF_GROUP *gf_group,
                            int order_hint_bits, int *gf_len);

// Pop one frame out of the array ctx->frame_info. This function is used to make
// sure that frame_info[0] always corresponds to the current frame.
void av1_pop_third_pass_info(THIRD_PASS_DEC_CTX *ctx);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_AV1_ENCODER_THIRDPASS_H_
