/*
 * Copyright 2017 Advanced Micro Devices. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifdef DRV_AMDGPU

// Avoid transitively including a bunch of unnecessary headers.
#define GL_GLEXT_LEGACY
#include "GL/internal/dri_interface.h"
#undef GL_GLEXT_LEGACY

#include "drv.h"

struct dri_driver {
	int fd;
	void *driver_handle;
	__DRIscreen *device;
	__DRIcontext *context; /* Needed for map/unmap operations. */
	const __DRIextension **extensions;
	const __DRIcoreExtension *core_extension;
	const __DRIdri2Extension *dri2_extension;
	const __DRIimageExtension *image_extension;
	const __DRI2flushExtension *flush_extension;
	const __DRIconfig **configs;
};

int dri_init(struct driver *drv, const char *dri_so_path, const char *driver_suffix);
void dri_close(struct driver *drv);
int dri_bo_create(struct bo *bo, uint32_t width, uint32_t height, uint32_t format,
		  uint64_t use_flags);
int dri_bo_create_with_modifiers(struct bo *bo, uint32_t width, uint32_t height, uint32_t format,
				 const uint64_t *modifiers, uint32_t modifier_count);
int dri_bo_import(struct bo *bo, struct drv_import_fd_data *data);
int dri_bo_destroy(struct bo *bo);
void *dri_bo_map(struct bo *bo, struct vma *vma, size_t plane, uint32_t map_flags);
int dri_bo_unmap(struct bo *bo, struct vma *vma);
size_t dri_num_planes_from_modifier(struct driver *drv, uint32_t format, uint64_t modifier);

#endif
