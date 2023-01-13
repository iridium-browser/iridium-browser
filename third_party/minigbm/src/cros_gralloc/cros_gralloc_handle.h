/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CROS_GRALLOC_HANDLE_H
#define CROS_GRALLOC_HANDLE_H

#include <cstdint>
#include <cutils/native_handle.h>

#define DRV_MAX_PLANES 4
#define DRV_MAX_FDS (DRV_MAX_PLANES + 1)

struct cros_gralloc_handle : public native_handle_t {
	/*
	 * File descriptors must immediately follow the native_handle_t base and used file
	 * descriptors must be packed at the beginning of this array to work with
	 * native_handle_clone().
	 *
	 * This field contains 'num_planes' plane file descriptors followed by an optional metadata
	 * reserved region file descriptor if 'reserved_region_size' is greater than zero.
	 */
	int32_t fds[DRV_MAX_FDS];
	uint32_t strides[DRV_MAX_PLANES];
	uint32_t offsets[DRV_MAX_PLANES];
	uint32_t sizes[DRV_MAX_PLANES];
	uint32_t id;
	uint32_t width;
	uint32_t height;
	uint32_t format; /* DRM format */
	uint32_t tiling;
	uint64_t format_modifier;
	uint64_t use_flags; /* Buffer creation flags */
	uint32_t magic;
	uint32_t pixel_stride;
	int32_t droid_format;
	int32_t usage; /* Android usage. */
	uint32_t num_planes;
	uint64_t reserved_region_size;
	uint64_t total_size; /* Total allocation size */
} __attribute__((packed));

typedef const struct cros_gralloc_handle *cros_gralloc_handle_t;

#endif
