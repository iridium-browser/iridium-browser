/*
 * Copyright (c) 2020, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_AV1_ENCODER_ENCODER_ALLOC_H_
#define AOM_AV1_ENCODER_ENCODER_ALLOC_H_

#include "av1/encoder/block.h"
#include "av1/encoder/encoder.h"
#include "av1/encoder/encodetxb.h"
#include "av1/encoder/ethread.h"

#ifdef __cplusplus
extern "C" {
#endif

static AOM_INLINE void dealloc_context_buffers_ext(
    MBMIExtFrameBufferInfo *mbmi_ext_info) {
  if (mbmi_ext_info->frame_base) {
    aom_free(mbmi_ext_info->frame_base);
    mbmi_ext_info->frame_base = NULL;
    mbmi_ext_info->alloc_size = 0;
  }
}

static AOM_INLINE void alloc_context_buffers_ext(
    AV1_COMMON *cm, MBMIExtFrameBufferInfo *mbmi_ext_info) {
  const CommonModeInfoParams *const mi_params = &cm->mi_params;

  const int mi_alloc_size_1d = mi_size_wide[mi_params->mi_alloc_bsize];
  const int mi_alloc_rows =
      (mi_params->mi_rows + mi_alloc_size_1d - 1) / mi_alloc_size_1d;
  const int mi_alloc_cols =
      (mi_params->mi_cols + mi_alloc_size_1d - 1) / mi_alloc_size_1d;
  const int new_ext_mi_size = mi_alloc_rows * mi_alloc_cols;

  if (new_ext_mi_size > mbmi_ext_info->alloc_size) {
    dealloc_context_buffers_ext(mbmi_ext_info);
    CHECK_MEM_ERROR(
        cm, mbmi_ext_info->frame_base,
        aom_calloc(new_ext_mi_size, sizeof(*mbmi_ext_info->frame_base)));
    mbmi_ext_info->alloc_size = new_ext_mi_size;
  }
  // The stride needs to be updated regardless of whether new allocation
  // happened or not.
  mbmi_ext_info->stride = mi_alloc_cols;
}

static AOM_INLINE void alloc_compressor_data(AV1_COMP *cpi) {
  AV1_COMMON *cm = &cpi->common;
  TokenInfo *token_info = &cpi->token_info;

  if (av1_alloc_context_buffers(cm, cm->width, cm->height)) {
    aom_internal_error(cm->error, AOM_CODEC_MEM_ERROR,
                       "Failed to allocate context buffers");
  }

  if (!is_stat_generation_stage(cpi)) {
    av1_alloc_txb_buf(cpi);

    alloc_context_buffers_ext(cm, &cpi->mbmi_ext_info);
  }

  free_token_info(token_info);

  if (!is_stat_generation_stage(cpi)) {
    alloc_token_info(cm, token_info);
  }
  if (cpi->td.mb.mv_costs) {
    aom_free(cpi->td.mb.mv_costs);
    cpi->td.mb.mv_costs = NULL;
  }
  CHECK_MEM_ERROR(cm, cpi->td.mb.mv_costs,
                  (MvCosts *)aom_calloc(1, sizeof(MvCosts)));

  if (cpi->td.mb.dv_costs) {
    aom_free(cpi->td.mb.dv_costs);
    cpi->td.mb.dv_costs = NULL;
  }
  CHECK_MEM_ERROR(cm, cpi->td.mb.dv_costs,
                  (IntraBCMVCosts *)aom_malloc(sizeof(*cpi->td.mb.dv_costs)));

  av1_setup_shared_coeff_buffer(cm->seq_params, &cpi->td.shared_coeff_buf,
                                cm->error);
  av1_setup_sms_tree(cpi, &cpi->td);
  cpi->td.firstpass_ctx =
      av1_alloc_pmc(cpi, BLOCK_16X16, &cpi->td.shared_coeff_buf);
}

static AOM_INLINE void realloc_segmentation_maps(AV1_COMP *cpi) {
  AV1_COMMON *const cm = &cpi->common;
  CommonModeInfoParams *const mi_params = &cm->mi_params;

  // Create the encoder segmentation map and set all entries to 0
  aom_free(cpi->enc_seg.map);
  CHECK_MEM_ERROR(cm, cpi->enc_seg.map,
                  aom_calloc(mi_params->mi_rows * mi_params->mi_cols, 1));

  // Create a map used for cyclic background refresh.
  if (cpi->cyclic_refresh) av1_cyclic_refresh_free(cpi->cyclic_refresh);
  CHECK_MEM_ERROR(
      cm, cpi->cyclic_refresh,
      av1_cyclic_refresh_alloc(mi_params->mi_rows, mi_params->mi_cols));

  // Create a map used to mark inactive areas.
  aom_free(cpi->active_map.map);
  CHECK_MEM_ERROR(cm, cpi->active_map.map,
                  aom_calloc(mi_params->mi_rows * mi_params->mi_cols, 1));
}

static AOM_INLINE void alloc_obmc_buffers(
    OBMCBuffer *obmc_buffer, struct aom_internal_error_info *error) {
  AOM_CHECK_MEM_ERROR(
      error, obmc_buffer->wsrc,
      (int32_t *)aom_memalign(16, MAX_SB_SQUARE * sizeof(*obmc_buffer->wsrc)));
  AOM_CHECK_MEM_ERROR(
      error, obmc_buffer->mask,
      (int32_t *)aom_memalign(16, MAX_SB_SQUARE * sizeof(*obmc_buffer->mask)));
  AOM_CHECK_MEM_ERROR(
      error, obmc_buffer->above_pred,
      (uint8_t *)aom_memalign(
          16, MAX_MB_PLANE * MAX_SB_SQUARE * sizeof(*obmc_buffer->above_pred)));
  AOM_CHECK_MEM_ERROR(
      error, obmc_buffer->left_pred,
      (uint8_t *)aom_memalign(
          16, MAX_MB_PLANE * MAX_SB_SQUARE * sizeof(*obmc_buffer->left_pred)));
}

static AOM_INLINE void release_obmc_buffers(OBMCBuffer *obmc_buffer) {
  aom_free(obmc_buffer->mask);
  aom_free(obmc_buffer->above_pred);
  aom_free(obmc_buffer->left_pred);
  aom_free(obmc_buffer->wsrc);

  obmc_buffer->mask = NULL;
  obmc_buffer->above_pred = NULL;
  obmc_buffer->left_pred = NULL;
  obmc_buffer->wsrc = NULL;
}

static AOM_INLINE void alloc_compound_type_rd_buffers(
    struct aom_internal_error_info *error, CompoundTypeRdBuffers *const bufs) {
  AOM_CHECK_MEM_ERROR(
      error, bufs->pred0,
      (uint8_t *)aom_memalign(16, 2 * MAX_SB_SQUARE * sizeof(*bufs->pred0)));
  AOM_CHECK_MEM_ERROR(
      error, bufs->pred1,
      (uint8_t *)aom_memalign(16, 2 * MAX_SB_SQUARE * sizeof(*bufs->pred1)));
  AOM_CHECK_MEM_ERROR(
      error, bufs->residual1,
      (int16_t *)aom_memalign(32, MAX_SB_SQUARE * sizeof(*bufs->residual1)));
  AOM_CHECK_MEM_ERROR(
      error, bufs->diff10,
      (int16_t *)aom_memalign(32, MAX_SB_SQUARE * sizeof(*bufs->diff10)));
  AOM_CHECK_MEM_ERROR(error, bufs->tmp_best_mask_buf,
                      (uint8_t *)aom_malloc(2 * MAX_SB_SQUARE *
                                            sizeof(*bufs->tmp_best_mask_buf)));
}

static AOM_INLINE void release_compound_type_rd_buffers(
    CompoundTypeRdBuffers *const bufs) {
  aom_free(bufs->pred0);
  aom_free(bufs->pred1);
  aom_free(bufs->residual1);
  aom_free(bufs->diff10);
  aom_free(bufs->tmp_best_mask_buf);
  av1_zero(*bufs);  // Set all pointers to NULL for safety.
}

static AOM_INLINE void dealloc_compressor_data(AV1_COMP *cpi) {
  AV1_COMMON *const cm = &cpi->common;
  TokenInfo *token_info = &cpi->token_info;

  dealloc_context_buffers_ext(&cpi->mbmi_ext_info);

  aom_free(cpi->tile_data);
  cpi->tile_data = NULL;

  // Delete sementation map
  aom_free(cpi->enc_seg.map);
  cpi->enc_seg.map = NULL;

  av1_cyclic_refresh_free(cpi->cyclic_refresh);
  cpi->cyclic_refresh = NULL;

  aom_free(cpi->active_map.map);
  cpi->active_map.map = NULL;

  aom_free(cpi->ssim_rdmult_scaling_factors);
  cpi->ssim_rdmult_scaling_factors = NULL;

#if CONFIG_TUNE_VMAF
  aom_free(cpi->vmaf_info.rdmult_scaling_factors);
  cpi->vmaf_info.rdmult_scaling_factors = NULL;
  aom_close_vmaf_model(cpi->vmaf_info.vmaf_model);
#endif

#if CONFIG_TUNE_BUTTERAUGLI
  aom_free(cpi->butteraugli_info.rdmult_scaling_factors);
  cpi->butteraugli_info.rdmult_scaling_factors = NULL;
  aom_free_frame_buffer(&cpi->butteraugli_info.source);
  aom_free_frame_buffer(&cpi->butteraugli_info.resized_source);
#endif

  release_obmc_buffers(&cpi->td.mb.obmc_buffer);

  if (cpi->td.mb.mv_costs) {
    aom_free(cpi->td.mb.mv_costs);
    cpi->td.mb.mv_costs = NULL;
  }

  if (cpi->td.mb.dv_costs) {
    aom_free(cpi->td.mb.dv_costs);
    cpi->td.mb.dv_costs = NULL;
  }

  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 2; j++) {
      aom_free(cpi->td.mb.intrabc_hash_info.hash_value_buffer[i][j]);
      cpi->td.mb.intrabc_hash_info.hash_value_buffer[i][j] = NULL;
    }

  aom_free(cm->tpl_mvs);
  cm->tpl_mvs = NULL;

  if (cpi->td.pixel_gradient_info) {
    aom_free(cpi->td.pixel_gradient_info);
    cpi->td.pixel_gradient_info = NULL;
  }

  if (cpi->td.vt64x64) {
    aom_free(cpi->td.vt64x64);
    cpi->td.vt64x64 = NULL;
  }

  av1_free_pmc(cpi->td.firstpass_ctx, av1_num_planes(cm));
  cpi->td.firstpass_ctx = NULL;

  av1_free_txb_buf(cpi);
  av1_free_context_buffers(cm);

  aom_free_frame_buffer(&cpi->last_frame_uf);
#if !CONFIG_REALTIME_ONLY
  av1_free_restoration_buffers(cm);
#endif

  if (!is_stat_generation_stage(cpi)) {
    int num_cdef_workers =
        av1_get_num_mod_workers_for_alloc(&cpi->ppi->p_mt_info, MOD_CDEF);
    av1_free_cdef_buffers(cm, &cpi->ppi->p_mt_info.cdef_worker,
                          &cpi->mt_info.cdef_sync, num_cdef_workers);
  }

  aom_free_frame_buffer(&cpi->trial_frame_rst);
  aom_free_frame_buffer(&cpi->scaled_source);
  aom_free_frame_buffer(&cpi->scaled_last_source);

  free_token_info(token_info);

  av1_free_shared_coeff_buffer(&cpi->td.shared_coeff_buf);
  av1_free_sms_tree(&cpi->td);

  aom_free(cpi->td.mb.palette_buffer);
  release_compound_type_rd_buffers(&cpi->td.mb.comp_rd_buffer);
  aom_free(cpi->td.mb.tmp_conv_dst);
  for (int j = 0; j < 2; ++j) {
    aom_free(cpi->td.mb.tmp_pred_bufs[j]);
  }

#if CONFIG_DENOISE
  if (cpi->denoise_and_model) {
    aom_denoise_and_model_free(cpi->denoise_and_model);
    cpi->denoise_and_model = NULL;
  }
#endif
  if (cpi->film_grain_table) {
    aom_film_grain_table_free(cpi->film_grain_table);
    cpi->film_grain_table = NULL;
  }

  if (cpi->ppi->use_svc) av1_free_svc_cyclic_refresh(cpi);

  if (cpi->consec_zero_mv) {
    aom_free(cpi->consec_zero_mv);
    cpi->consec_zero_mv = NULL;
  }

  aom_free(cpi->mb_weber_stats);
  cpi->mb_weber_stats = NULL;

  aom_free(cpi->mb_delta_q);
  cpi->mb_delta_q = NULL;
}

static AOM_INLINE void allocate_gradient_info_for_hog(
    PixelLevelGradientInfo **pixel_gradient_info, AV1_COMP *cpi) {
  const AV1_COMMON *const cm = &cpi->common;

  if (!*pixel_gradient_info) {
    const int plane_types = PLANE_TYPES >> cm->seq_params->monochrome;
    CHECK_MEM_ERROR(cm, *pixel_gradient_info,
                    aom_malloc(sizeof(**pixel_gradient_info) * plane_types *
                               MAX_SB_SQUARE));
  }

  cpi->td.mb.pixel_gradient_info = *pixel_gradient_info;
}

static AOM_INLINE void variance_partition_alloc(AV1_COMP *cpi) {
  AV1_COMMON *const cm = &cpi->common;
  const int num_64x64_blocks = (cm->seq_params->sb_size == BLOCK_64X64) ? 1 : 4;
  if (cpi->td.vt64x64) {
    if (num_64x64_blocks != cpi->td.num_64x64_blocks) {
      aom_free(cpi->td.vt64x64);
      cpi->td.vt64x64 = NULL;
    }
  }
  if (!cpi->td.vt64x64) {
    CHECK_MEM_ERROR(cm, cpi->td.vt64x64,
                    aom_malloc(sizeof(*cpi->td.vt64x64) * num_64x64_blocks));
    cpi->td.num_64x64_blocks = num_64x64_blocks;
  }
}

static AOM_INLINE void alloc_altref_frame_buffer(AV1_COMP *cpi) {
  AV1_COMMON *cm = &cpi->common;
  const SequenceHeader *const seq_params = cm->seq_params;
  const AV1EncoderConfig *oxcf = &cpi->oxcf;

  // When lag_in_frames <= 1, alt-ref frames are not enabled. In this case,
  // temporal filtering of key frames is disabled as well. Hence alt_ref_buffer
  // allocation is avoided.
  if (oxcf->gf_cfg.lag_in_frames <= 1) return;

  // TODO(agrange) Check if ARF is enabled and skip allocation if not.
  if (aom_realloc_frame_buffer(
          &cpi->ppi->alt_ref_buffer, oxcf->frm_dim_cfg.width,
          oxcf->frm_dim_cfg.height, seq_params->subsampling_x,
          seq_params->subsampling_y, seq_params->use_highbitdepth,
          cpi->oxcf.border_in_pixels, cm->features.byte_alignment, NULL, NULL,
          NULL, cpi->oxcf.tool_cfg.enable_global_motion))
    aom_internal_error(cm->error, AOM_CODEC_MEM_ERROR,
                       "Failed to allocate altref buffer");
}

static AOM_INLINE void alloc_util_frame_buffers(AV1_COMP *cpi) {
  AV1_COMMON *const cm = &cpi->common;
  const SequenceHeader *const seq_params = cm->seq_params;
  const int byte_alignment = cm->features.byte_alignment;
  if (aom_realloc_frame_buffer(
          &cpi->last_frame_uf, cm->width, cm->height, seq_params->subsampling_x,
          seq_params->subsampling_y, seq_params->use_highbitdepth,
          cpi->oxcf.border_in_pixels, byte_alignment, NULL, NULL, NULL, 0))
    aom_internal_error(cm->error, AOM_CODEC_MEM_ERROR,
                       "Failed to allocate last frame buffer");

  // The frame buffer trial_frame_rst is used during loop restoration filter
  // search. Hence it is allocated only when loop restoration is used.
  const int use_restoration = cm->seq_params->enable_restoration &&
                              !cm->features.all_lossless &&
                              !cm->tiles.large_scale;
  if (use_restoration) {
    if (aom_realloc_frame_buffer(
            &cpi->trial_frame_rst, cm->superres_upscaled_width,
            cm->superres_upscaled_height, seq_params->subsampling_x,
            seq_params->subsampling_y, seq_params->use_highbitdepth,
            AOM_RESTORATION_FRAME_BORDER, byte_alignment, NULL, NULL, NULL, 0))
      aom_internal_error(cm->error, AOM_CODEC_MEM_ERROR,
                         "Failed to allocate trial restored frame buffer");
  }

  if (aom_realloc_frame_buffer(
          &cpi->scaled_source, cm->width, cm->height, seq_params->subsampling_x,
          seq_params->subsampling_y, seq_params->use_highbitdepth,
          cpi->oxcf.border_in_pixels, byte_alignment, NULL, NULL, NULL,
          cpi->oxcf.tool_cfg.enable_global_motion))
    aom_internal_error(cm->error, AOM_CODEC_MEM_ERROR,
                       "Failed to allocate scaled source buffer");

  // The frame buffer cpi->scaled_last_source is used to hold the previous
  // source frame information. As the previous source frame buffer allocation in
  // the lookahead queue is avoided for all-intra frame encoding,
  // cpi->unscaled_last_source will be NULL in such cases. As
  // cpi->unscaled_last_source is NULL, cpi->scaled_last_source will not be used
  // for all-intra frame encoding. Hence, the buffer is allocated conditionally.
  if (cpi->oxcf.kf_cfg.key_freq_max > 0) {
    if (aom_realloc_frame_buffer(
            &cpi->scaled_last_source, cm->width, cm->height,
            seq_params->subsampling_x, seq_params->subsampling_y,
            seq_params->use_highbitdepth, cpi->oxcf.border_in_pixels,
            byte_alignment, NULL, NULL, NULL,
            cpi->oxcf.tool_cfg.enable_global_motion))
      aom_internal_error(cm->error, AOM_CODEC_MEM_ERROR,
                         "Failed to allocate scaled last source buffer");
  }
}

static AOM_INLINE YV12_BUFFER_CONFIG *realloc_and_scale_source(
    AV1_COMP *cpi, int scaled_width, int scaled_height) {
  AV1_COMMON *cm = &cpi->common;
  const int num_planes = av1_num_planes(cm);

  if (scaled_width == cpi->unscaled_source->y_crop_width &&
      scaled_height == cpi->unscaled_source->y_crop_height) {
    return cpi->unscaled_source;
  }

  if (aom_realloc_frame_buffer(
          &cpi->scaled_source, scaled_width, scaled_height,
          cm->seq_params->subsampling_x, cm->seq_params->subsampling_y,
          cm->seq_params->use_highbitdepth, AOM_BORDER_IN_PIXELS,
          cm->features.byte_alignment, NULL, NULL, NULL,
          cpi->oxcf.tool_cfg.enable_global_motion))
    aom_internal_error(cm->error, AOM_CODEC_MEM_ERROR,
                       "Failed to reallocate scaled source buffer");
  assert(cpi->scaled_source.y_crop_width == scaled_width);
  assert(cpi->scaled_source.y_crop_height == scaled_height);
  av1_resize_and_extend_frame_nonnormative(
      cpi->unscaled_source, &cpi->scaled_source, (int)cm->seq_params->bit_depth,
      num_planes);
  return &cpi->scaled_source;
}

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_AV1_ENCODER_ENCODER_ALLOC_H_
