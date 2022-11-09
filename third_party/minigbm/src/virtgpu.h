/*
 * Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

struct virtgpu_param {
	uint64_t param;
	const char *name;
	uint32_t value;
};

enum virtgpu_param_id {
	param_3d,
	param_capset_fix,
	param_resource_blob,
	param_host_visible,
	param_cross_device,
	param_context_init,
	param_supported_capset_ids,
	param_create_guest_handle,
	param_resource_sync,
	param_guest_vram,
	param_max,
};
