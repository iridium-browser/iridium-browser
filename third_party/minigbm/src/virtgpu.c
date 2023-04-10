/*
 * Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <xf86drm.h>

#include "drv_priv.h"
#include "external/virtgpu_drm.h"
#include "util.h"
#include "virtgpu.h"

#define PARAM(x)                                                                                   \
	(struct virtgpu_param)                                                                     \
	{                                                                                          \
		x, #x, 0                                                                           \
	}

struct virtgpu_param params[] = {
	PARAM(VIRTGPU_PARAM_3D_FEATURES),	   PARAM(VIRTGPU_PARAM_CAPSET_QUERY_FIX),
	PARAM(VIRTGPU_PARAM_RESOURCE_BLOB),	   PARAM(VIRTGPU_PARAM_HOST_VISIBLE),
	PARAM(VIRTGPU_PARAM_CROSS_DEVICE),	   PARAM(VIRTGPU_PARAM_CONTEXT_INIT),
	PARAM(VIRTGPU_PARAM_SUPPORTED_CAPSET_IDs), PARAM(VIRTGPU_PARAM_CREATE_GUEST_HANDLE),
	PARAM(VIRTGPU_PARAM_RESOURCE_SYNC),	   PARAM(VIRTGPU_PARAM_GUEST_VRAM),
};

extern const struct backend virtgpu_virgl;
extern const struct backend virtgpu_cross_domain;

static int virtgpu_init(struct driver *drv)
{
	int ret = 0;
	const struct backend *virtgpu_backends[2] = {
		&virtgpu_cross_domain,
		&virtgpu_virgl,
	};

	for (uint32_t i = 0; i < ARRAY_SIZE(params); i++) {
		struct drm_virtgpu_getparam get_param = { 0 };

		get_param.param = params[i].param;
		get_param.value = (uint64_t)(uintptr_t)&params[i].value;
		int ret = drmIoctl(drv->fd, DRM_IOCTL_VIRTGPU_GETPARAM, &get_param);
		if (ret)
			drv_logi("virtgpu backend not enabling %s\n", params[i].name);
	}

	for (uint32_t i = 0; i < ARRAY_SIZE(virtgpu_backends); i++) {
		const struct backend *backend = virtgpu_backends[i];
		ret = backend->init(drv);
		if (ret)
			continue;

		drv->backend = backend;
		return 0;
	}

	return ret;
}

const struct backend backend_virtgpu = {
	.name = "virtio_gpu",
	.init = virtgpu_init,
};
