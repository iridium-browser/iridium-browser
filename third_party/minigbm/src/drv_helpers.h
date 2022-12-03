/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef DRV_HELPERS_H
#define DRV_HELPERS_H

#include <stdbool.h>

#include "drv.h"
#include "drv_array_helpers.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

struct format_metadata;

uint32_t drv_height_from_format(uint32_t format, uint32_t height, size_t plane);
uint32_t drv_vertical_subsampling_from_format(uint32_t format, size_t plane);
uint32_t drv_size_from_format(uint32_t format, uint32_t stride, uint32_t height, size_t plane);
int drv_bo_from_format(struct bo *bo, uint32_t stride, uint32_t aligned_height, uint32_t format);
int drv_bo_from_format_and_padding(struct bo *bo, uint32_t stride, uint32_t aligned_height,
				   uint32_t format, uint32_t padding[DRV_MAX_PLANES]);
int drv_dumb_bo_create(struct bo *bo, uint32_t width, uint32_t height, uint32_t format,
		       uint64_t use_flags);
int drv_dumb_bo_create_ex(struct bo *bo, uint32_t width, uint32_t height, uint32_t format,
			  uint64_t use_flags, uint64_t quirks);
int drv_dumb_bo_destroy(struct bo *bo);
int drv_gem_bo_destroy(struct bo *bo);
int drv_prime_bo_import(struct bo *bo, struct drv_import_fd_data *data);
void *drv_dumb_bo_map(struct bo *bo, struct vma *vma, size_t plane, uint32_t map_flags);
int drv_bo_munmap(struct bo *bo, struct vma *vma);
int drv_get_prot(uint32_t map_flags);
void drv_add_combination(struct driver *drv, uint32_t format, struct format_metadata *metadata,
			 uint64_t usage);
void drv_add_combinations(struct driver *drv, const uint32_t *formats, uint32_t num_formats,
			  struct format_metadata *metadata, uint64_t usage);
void drv_modify_combination(struct driver *drv, uint32_t format, struct format_metadata *metadata,
			    uint64_t usage);
int drv_modify_linear_combinations(struct driver *drv);
uint64_t drv_pick_modifier(const uint64_t *modifiers, uint32_t count,
			   const uint64_t *modifier_order, uint32_t order_count);
bool drv_has_modifier(const uint64_t *list, uint32_t count, uint64_t modifier);
void drv_resolve_format_and_use_flags_helper(struct driver *drv, uint32_t format,
					     uint64_t use_flags, uint32_t *out_format,
					     uint64_t *out_use_flags);

#endif
