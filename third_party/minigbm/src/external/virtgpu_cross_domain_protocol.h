// Copyright 2021 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIRTGPU_CROSS_DOMAIN_PROTOCOL_H
#define VIRTGPU_CROSS_DOMAIN_PROTOCOL_H

#include <stdint.h>

// Cross-domain commands (only a maximum of 255 supported)
#define CROSS_DOMAIN_CMD_INIT 1
#define CROSS_DOMAIN_CMD_GET_IMAGE_REQUIREMENTS 2

// Channel types (must match rutabaga channel types)
#define CROSS_DOMAIN_CHANNEL_TYPE_WAYLAND 0x0001
#define CROSS_DOMAIN_CHANNEL_TYPE_CAMERA 0x0002

struct CrossDomainCapabilities {
	uint32_t version;
	uint32_t supported_channels;
	uint32_t supports_dmabuf;
	uint32_t supports_external_gpu_memory;
};

struct CrossDomainImageRequirements {
	uint32_t strides[4];
	uint32_t offsets[4];
	uint64_t modifier;
	uint64_t size;
	uint64_t blob_id;
	uint32_t map_info;
	uint32_t pad;
	int32_t memory_idx;
	int32_t physical_device_idx;
};

struct CrossDomainHeader {
	uint8_t cmd;
	uint8_t fence_ctx_idx;
	uint16_t cmd_size;
	uint32_t pad;
};

struct CrossDomainInit {
	struct CrossDomainHeader hdr;
	uint32_t ring_id;
	uint32_t channel_type;
};

struct CrossDomainGetImageRequirements {
	struct CrossDomainHeader hdr;
	uint32_t width;
	uint32_t height;
	uint32_t drm_format;
	uint32_t flags;
};

#endif
