/*
 * Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CROS_GRALLOC_TYPES_H
#define CROS_GRALLOC_TYPES_H

#include <string>

// Reserve the GRALLOC_USAGE_PRIVATE_0 bit from hardware/gralloc.h for buffers
// used for front rendering. minigbm backend later decides to use
// BO_USE_FRONT_RENDERING or BO_USE_LINEAR upon buffer allocaton.
#define BUFFER_USAGE_FRONT_RENDERING (1U << 28)

struct cros_gralloc_buffer_descriptor {
	uint32_t width;
	uint32_t height;
	int32_t droid_format;
	int32_t droid_usage;
	uint32_t drm_format;
	uint64_t use_flags;
	uint64_t reserved_region_size;
	std::string name;
};

#endif
