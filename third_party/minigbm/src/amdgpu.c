/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifdef DRV_AMDGPU
#include <amdgpu.h>
#include <amdgpu_drm.h>
#include <assert.h>
#include <drm_fourcc.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "dri.h"
#include "drv_helpers.h"
#include "drv_priv.h"
#include "util.h"

// clang-format off
#define DRI_PATH STRINGIZE(DRI_DRIVER_DIR/radeonsi_dri.so)
// clang-format on

#define TILE_TYPE_LINEAR 0
/* We decide a modifier and then use DRI to manage allocation */
#define TILE_TYPE_DRI_MODIFIER 1
/* DRI backend decides tiling in this case. */
#define TILE_TYPE_DRI 2

/* Height alignement for Encoder/Decoder buffers */
#define CHROME_HEIGHT_ALIGN 16

struct amdgpu_priv {
	struct dri_driver dri;
	int drm_version;

	/* sdma */
	struct drm_amdgpu_info_device dev_info;
	uint32_t sdma_ctx;
	uint32_t sdma_cmdbuf_bo;
	uint64_t sdma_cmdbuf_addr;
	uint64_t sdma_cmdbuf_size;
	uint32_t *sdma_cmdbuf_map;
};

struct amdgpu_linear_vma_priv {
	uint32_t handle;
	uint32_t map_flags;
};

const static uint32_t render_target_formats[] = {
	DRM_FORMAT_ABGR8888,	  DRM_FORMAT_ARGB8888,	  DRM_FORMAT_RGB565,
	DRM_FORMAT_XBGR8888,	  DRM_FORMAT_XRGB8888,	  DRM_FORMAT_ABGR2101010,
	DRM_FORMAT_ARGB2101010,	  DRM_FORMAT_XBGR2101010, DRM_FORMAT_XRGB2101010,
	DRM_FORMAT_ABGR16161616F,
};

const static uint32_t texture_source_formats[] = {
	DRM_FORMAT_GR88,	   DRM_FORMAT_R8,     DRM_FORMAT_NV21, DRM_FORMAT_NV12,
	DRM_FORMAT_YVU420_ANDROID, DRM_FORMAT_YVU420, DRM_FORMAT_P010
};

static int query_dev_info(int fd, struct drm_amdgpu_info_device *dev_info)
{
	struct drm_amdgpu_info info_args = { 0 };

	info_args.return_pointer = (uintptr_t)dev_info;
	info_args.return_size = sizeof(*dev_info);
	info_args.query = AMDGPU_INFO_DEV_INFO;

	return drmCommandWrite(fd, DRM_AMDGPU_INFO, &info_args, sizeof(info_args));
}

static int sdma_init(struct amdgpu_priv *priv, int fd)
{
	union drm_amdgpu_ctx ctx_args = { { 0 } };
	union drm_amdgpu_gem_create gem_create = { { 0 } };
	struct drm_amdgpu_gem_va va_args = { 0 };
	union drm_amdgpu_gem_mmap gem_map = { { 0 } };
	struct drm_gem_close gem_close = { 0 };
	int ret;

	/* Ensure we can make a submission without BO lists. */
	if (priv->drm_version < 27)
		return 0;

	/* Anything outside this range needs adjustments to the SDMA copy commands */
	if (priv->dev_info.family < AMDGPU_FAMILY_CI || priv->dev_info.family > AMDGPU_FAMILY_NV)
		return 0;

	ctx_args.in.op = AMDGPU_CTX_OP_ALLOC_CTX;

	ret = drmCommandWriteRead(fd, DRM_AMDGPU_CTX, &ctx_args, sizeof(ctx_args));
	if (ret < 0)
		return ret;

	priv->sdma_ctx = ctx_args.out.alloc.ctx_id;

	priv->sdma_cmdbuf_size = ALIGN(4096, priv->dev_info.virtual_address_alignment);
	gem_create.in.bo_size = priv->sdma_cmdbuf_size;
	gem_create.in.alignment = 4096;
	gem_create.in.domains = AMDGPU_GEM_DOMAIN_GTT;

	ret = drmCommandWriteRead(fd, DRM_AMDGPU_GEM_CREATE, &gem_create, sizeof(gem_create));
	if (ret < 0)
		goto fail_ctx;

	priv->sdma_cmdbuf_bo = gem_create.out.handle;

	priv->sdma_cmdbuf_addr =
	    ALIGN(priv->dev_info.virtual_address_offset, priv->dev_info.virtual_address_alignment);

	/* Map the buffer into the GPU address space so we can use it from the GPU */
	va_args.handle = priv->sdma_cmdbuf_bo;
	va_args.operation = AMDGPU_VA_OP_MAP;
	va_args.flags = AMDGPU_VM_PAGE_READABLE | AMDGPU_VM_PAGE_EXECUTABLE;
	va_args.va_address = priv->sdma_cmdbuf_addr;
	va_args.offset_in_bo = 0;
	va_args.map_size = priv->sdma_cmdbuf_size;

	ret = drmCommandWrite(fd, DRM_AMDGPU_GEM_VA, &va_args, sizeof(va_args));
	if (ret)
		goto fail_bo;

	gem_map.in.handle = priv->sdma_cmdbuf_bo;
	ret = drmIoctl(fd, DRM_IOCTL_AMDGPU_GEM_MMAP, &gem_map);
	if (ret)
		goto fail_va;

	priv->sdma_cmdbuf_map = mmap(0, priv->sdma_cmdbuf_size, PROT_READ | PROT_WRITE, MAP_SHARED,
				     fd, gem_map.out.addr_ptr);
	if (priv->sdma_cmdbuf_map == MAP_FAILED) {
		priv->sdma_cmdbuf_map = NULL;
		ret = -ENOMEM;
		goto fail_va;
	}

	return 0;
fail_va:
	va_args.operation = AMDGPU_VA_OP_UNMAP;
	va_args.flags = 0;
	drmCommandWrite(fd, DRM_AMDGPU_GEM_VA, &va_args, sizeof(va_args));
fail_bo:
	gem_close.handle = priv->sdma_cmdbuf_bo;
	drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
fail_ctx:
	memset(&ctx_args, 0, sizeof(ctx_args));
	ctx_args.in.op = AMDGPU_CTX_OP_FREE_CTX;
	ctx_args.in.ctx_id = priv->sdma_ctx;
	drmCommandWriteRead(fd, DRM_AMDGPU_CTX, &ctx_args, sizeof(ctx_args));
	return ret;
}

static void sdma_finish(struct amdgpu_priv *priv, int fd)
{
	union drm_amdgpu_ctx ctx_args = { { 0 } };
	struct drm_amdgpu_gem_va va_args = { 0 };
	struct drm_gem_close gem_close = { 0 };

	if (!priv->sdma_cmdbuf_map)
		return;

	va_args.handle = priv->sdma_cmdbuf_bo;
	va_args.operation = AMDGPU_VA_OP_UNMAP;
	va_args.flags = 0;
	va_args.va_address = priv->sdma_cmdbuf_addr;
	va_args.offset_in_bo = 0;
	va_args.map_size = priv->sdma_cmdbuf_size;
	drmCommandWrite(fd, DRM_AMDGPU_GEM_VA, &va_args, sizeof(va_args));

	gem_close.handle = priv->sdma_cmdbuf_bo;
	drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &gem_close);

	ctx_args.in.op = AMDGPU_CTX_OP_FREE_CTX;
	ctx_args.in.ctx_id = priv->sdma_ctx;
	drmCommandWriteRead(fd, DRM_AMDGPU_CTX, &ctx_args, sizeof(ctx_args));
}

static int sdma_copy(struct amdgpu_priv *priv, int fd, uint32_t src_handle, uint32_t dst_handle,
		     uint64_t size)
{
	const uint64_t max_size_per_cmd = 0x3fff00;
	const uint32_t cmd_size = 7 * sizeof(uint32_t); /* 7 dwords, see loop below. */
	const uint64_t max_commands = priv->sdma_cmdbuf_size / cmd_size;
	uint64_t src_addr = priv->sdma_cmdbuf_addr + priv->sdma_cmdbuf_size;
	uint64_t dst_addr = src_addr + size;
	struct drm_amdgpu_gem_va va_args = { 0 };
	unsigned cmd = 0;
	uint64_t remaining_size = size;
	uint64_t cur_src_addr = src_addr;
	uint64_t cur_dst_addr = dst_addr;
	struct drm_amdgpu_cs_chunk_ib ib = { 0 };
	struct drm_amdgpu_cs_chunk chunks[2] = { { 0 } };
	uint64_t chunk_ptrs[2];
	union drm_amdgpu_cs cs = { { 0 } };
	struct drm_amdgpu_bo_list_in bo_list = { 0 };
	struct drm_amdgpu_bo_list_entry bo_list_entries[3] = { { 0 } };
	union drm_amdgpu_wait_cs wait_cs = { { 0 } };
	int ret = 0;

	if (size > UINT64_MAX - max_size_per_cmd ||
	    DIV_ROUND_UP(size, max_size_per_cmd) > max_commands)
		return -ENOMEM;

	/* Map both buffers into the GPU address space so we can access them from the GPU. */
	va_args.handle = src_handle;
	va_args.operation = AMDGPU_VA_OP_MAP;
	va_args.flags = AMDGPU_VM_PAGE_READABLE | AMDGPU_VM_DELAY_UPDATE;
	va_args.va_address = src_addr;
	va_args.map_size = size;

	ret = drmCommandWrite(fd, DRM_AMDGPU_GEM_VA, &va_args, sizeof(va_args));
	if (ret)
		return ret;

	va_args.handle = dst_handle;
	va_args.flags = AMDGPU_VM_PAGE_READABLE | AMDGPU_VM_PAGE_WRITEABLE | AMDGPU_VM_DELAY_UPDATE;
	va_args.va_address = dst_addr;

	ret = drmCommandWrite(fd, DRM_AMDGPU_GEM_VA, &va_args, sizeof(va_args));
	if (ret)
		goto unmap_src;

	while (remaining_size) {
		uint64_t cur_size = remaining_size;
		if (cur_size > max_size_per_cmd)
			cur_size = max_size_per_cmd;

		priv->sdma_cmdbuf_map[cmd++] = 0x01; /* linear copy */
		priv->sdma_cmdbuf_map[cmd++] =
		    priv->dev_info.family >= AMDGPU_FAMILY_AI ? (cur_size - 1) : cur_size;
		priv->sdma_cmdbuf_map[cmd++] = 0;
		priv->sdma_cmdbuf_map[cmd++] = cur_src_addr;
		priv->sdma_cmdbuf_map[cmd++] = cur_src_addr >> 32;
		priv->sdma_cmdbuf_map[cmd++] = cur_dst_addr;
		priv->sdma_cmdbuf_map[cmd++] = cur_dst_addr >> 32;

		remaining_size -= cur_size;
		cur_src_addr += cur_size;
		cur_dst_addr += cur_size;
	}

	ib.va_start = priv->sdma_cmdbuf_addr;
	ib.ib_bytes = cmd * 4;
	ib.ip_type = AMDGPU_HW_IP_DMA;

	chunks[1].chunk_id = AMDGPU_CHUNK_ID_IB;
	chunks[1].length_dw = sizeof(ib) / 4;
	chunks[1].chunk_data = (uintptr_t)&ib;

	bo_list_entries[0].bo_handle = priv->sdma_cmdbuf_bo;
	bo_list_entries[0].bo_priority = 8; /* Middle of range, like RADV. */
	bo_list_entries[1].bo_handle = src_handle;
	bo_list_entries[1].bo_priority = 8;
	bo_list_entries[2].bo_handle = dst_handle;
	bo_list_entries[2].bo_priority = 8;

	bo_list.bo_number = 3;
	bo_list.bo_info_size = sizeof(bo_list_entries[0]);
	bo_list.bo_info_ptr = (uintptr_t)bo_list_entries;

	chunks[0].chunk_id = AMDGPU_CHUNK_ID_BO_HANDLES;
	chunks[0].length_dw = sizeof(bo_list) / 4;
	chunks[0].chunk_data = (uintptr_t)&bo_list;

	chunk_ptrs[0] = (uintptr_t)&chunks[0];
	chunk_ptrs[1] = (uintptr_t)&chunks[1];

	cs.in.ctx_id = priv->sdma_ctx;
	cs.in.num_chunks = 2;
	cs.in.chunks = (uintptr_t)chunk_ptrs;

	ret = drmCommandWriteRead(fd, DRM_AMDGPU_CS, &cs, sizeof(cs));
	if (ret) {
		drv_loge("SDMA copy command buffer submission failed %d\n", ret);
		goto unmap_dst;
	}

	wait_cs.in.handle = cs.out.handle;
	wait_cs.in.ip_type = AMDGPU_HW_IP_DMA;
	wait_cs.in.ctx_id = priv->sdma_ctx;
	wait_cs.in.timeout = INT64_MAX;

	ret = drmCommandWriteRead(fd, DRM_AMDGPU_WAIT_CS, &wait_cs, sizeof(wait_cs));
	if (ret) {
		drv_loge("Could not wait for CS to finish\n");
	} else if (wait_cs.out.status) {
		drv_loge("Infinite wait timed out, likely GPU hang.\n");
		ret = -ENODEV;
	}

unmap_dst:
	va_args.handle = dst_handle;
	va_args.operation = AMDGPU_VA_OP_UNMAP;
	va_args.flags = AMDGPU_VM_DELAY_UPDATE;
	va_args.va_address = dst_addr;
	drmCommandWrite(fd, DRM_AMDGPU_GEM_VA, &va_args, sizeof(va_args));

unmap_src:
	va_args.handle = src_handle;
	va_args.operation = AMDGPU_VA_OP_UNMAP;
	va_args.flags = AMDGPU_VM_DELAY_UPDATE;
	va_args.va_address = src_addr;
	drmCommandWrite(fd, DRM_AMDGPU_GEM_VA, &va_args, sizeof(va_args));

	return ret;
}

static bool is_modifier_scanout_capable(struct amdgpu_priv *priv, uint32_t format,
					uint64_t modifier)
{
	unsigned bytes_per_pixel = drv_stride_from_format(format, 1, 0);

	if (modifier == DRM_FORMAT_MOD_LINEAR)
		return true;

	if ((modifier >> 56) != DRM_FORMAT_MOD_VENDOR_AMD)
		return false;

	unsigned swizzle = AMD_FMT_MOD_GET(TILE, modifier);
	if (priv->dev_info.family >= AMDGPU_FAMILY_RV) { /* DCN based GPUs */
		/* D swizzle only supported for 64 bpp */
		if ((swizzle & 3) == 2 && bytes_per_pixel != 8)
			return false;

		/* S swizzle not supported for 64 bpp */
		if ((swizzle & 3) == 1 && bytes_per_pixel == 8)
			return false;
	} else { /* DCE based GPUs with GFX9 based modifier swizzling. */
		assert(priv->dev_info.family == AMDGPU_FAMILY_AI);
		/* Only D swizzles are allowed for display */
		if ((swizzle & 3) != 2)
			return false;
	}

	if (AMD_FMT_MOD_GET(DCC, modifier) &&
	    (AMD_FMT_MOD_GET(DCC_PIPE_ALIGN, modifier) || !AMD_FMT_MOD_GET(DCC_RETILE, modifier)))
		return false;
	return true;
}

static int amdgpu_init(struct driver *drv)
{
	struct amdgpu_priv *priv;
	drmVersionPtr drm_version;
	struct format_metadata metadata;
	uint64_t use_flags = BO_USE_RENDER_MASK;

	priv = calloc(1, sizeof(struct amdgpu_priv));
	if (!priv)
		return -ENOMEM;

	drm_version = drmGetVersion(drv_get_fd(drv));
	if (!drm_version) {
		free(priv);
		return -ENODEV;
	}

	priv->drm_version = drm_version->version_minor;
	drmFreeVersion(drm_version);

	drv->priv = priv;

	if (query_dev_info(drv_get_fd(drv), &priv->dev_info)) {
		free(priv);
		drv->priv = NULL;
		return -ENODEV;
	}
	if (dri_init(drv, DRI_PATH, "radeonsi")) {
		free(priv);
		drv->priv = NULL;
		return -ENODEV;
	}

	/* Continue on failure, as we can still succesfully map things without SDMA. */
	if (sdma_init(priv, drv_get_fd(drv)))
		drv_loge("SDMA init failed\n");

	metadata.tiling = TILE_TYPE_LINEAR;
	metadata.priority = 1;
	metadata.modifier = DRM_FORMAT_MOD_LINEAR;

	drv_add_combinations(drv, render_target_formats, ARRAY_SIZE(render_target_formats),
			     &metadata, use_flags);

	drv_add_combinations(drv, texture_source_formats, ARRAY_SIZE(texture_source_formats),
			     &metadata, BO_USE_TEXTURE_MASK);

	/* NV12 format for camera, display, decoding and encoding. */
	drv_modify_combination(drv, DRM_FORMAT_NV12, &metadata,
			       BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE | BO_USE_SCANOUT |
				   BO_USE_HW_VIDEO_DECODER | BO_USE_HW_VIDEO_ENCODER |
				   BO_USE_PROTECTED);

	drv_modify_combination(drv, DRM_FORMAT_P010, &metadata,
			       BO_USE_SCANOUT | BO_USE_HW_VIDEO_DECODER | BO_USE_HW_VIDEO_ENCODER |
				   BO_USE_PROTECTED);

	/* Android CTS tests require this. */
	drv_add_combination(drv, DRM_FORMAT_BGR888, &metadata, BO_USE_SW_MASK);

	/* Linear formats supported by display. */
	drv_modify_combination(drv, DRM_FORMAT_ARGB8888, &metadata, BO_USE_CURSOR | BO_USE_SCANOUT);
	drv_modify_combination(drv, DRM_FORMAT_XRGB8888, &metadata, BO_USE_CURSOR | BO_USE_SCANOUT);
	drv_modify_combination(drv, DRM_FORMAT_ABGR8888, &metadata, BO_USE_SCANOUT);
	drv_modify_combination(drv, DRM_FORMAT_XBGR8888, &metadata, BO_USE_SCANOUT);
	drv_modify_combination(drv, DRM_FORMAT_RGB565, &metadata, BO_USE_SCANOUT);

	drv_modify_combination(drv, DRM_FORMAT_ABGR2101010, &metadata, BO_USE_SCANOUT);
	drv_modify_combination(drv, DRM_FORMAT_ARGB2101010, &metadata, BO_USE_SCANOUT);
	drv_modify_combination(drv, DRM_FORMAT_XBGR2101010, &metadata, BO_USE_SCANOUT);
	drv_modify_combination(drv, DRM_FORMAT_XRGB2101010, &metadata, BO_USE_SCANOUT);

	drv_modify_combination(drv, DRM_FORMAT_NV21, &metadata, BO_USE_SCANOUT);

	/*
	 * R8 format is used for Android's HAL_PIXEL_FORMAT_BLOB and is used for JPEG snapshots
	 * from camera and input/output from hardware decoder/encoder.
	 */
	drv_modify_combination(drv, DRM_FORMAT_R8, &metadata,
			       BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE | BO_USE_HW_VIDEO_DECODER |
				   BO_USE_HW_VIDEO_ENCODER | BO_USE_GPU_DATA_BUFFER |
				   BO_USE_SENSOR_DIRECT_DATA);

	/*
	 * The following formats will be allocated by the DRI backend and may be potentially tiled.
	 * Since format modifier support hasn't been implemented fully yet, it's not
	 * possible to enumerate the different types of buffers (like i915 can).
	 */
	use_flags &= ~BO_USE_RENDERSCRIPT;
	use_flags &= ~BO_USE_SW_WRITE_OFTEN;
	use_flags &= ~BO_USE_SW_READ_OFTEN;
#if __ANDROID__
	use_flags &= ~BO_USE_SW_WRITE_RARELY;
	use_flags &= ~BO_USE_SW_READ_RARELY;
#endif
	use_flags &= ~BO_USE_LINEAR;

	metadata.priority = 2;

	for (unsigned f = 0; f < ARRAY_SIZE(render_target_formats); ++f) {
		uint32_t format = render_target_formats[f];
		int mod_cnt;
		if (dri_query_modifiers(drv, format, 0, NULL, &mod_cnt) && mod_cnt) {
			uint64_t *modifiers = calloc(mod_cnt, sizeof(uint64_t));
			dri_query_modifiers(drv, format, mod_cnt, modifiers, &mod_cnt);
			metadata.tiling = TILE_TYPE_DRI_MODIFIER;
			for (int i = 0; i < mod_cnt; ++i) {
				bool scanout =
				    is_modifier_scanout_capable(drv->priv, format, modifiers[i]);

				/* LINEAR will be handled using the LINEAR metadata. */
				if (modifiers[i] == DRM_FORMAT_MOD_LINEAR)
					continue;

				/* The virtgpu minigbm can't handle auxiliary planes in the host. */
				if (dri_num_planes_from_modifier(drv, format, modifiers[i]) !=
				    drv_num_planes_from_format(format))
					continue;

				metadata.modifier = modifiers[i];
				drv_add_combination(drv, format, &metadata,
						    use_flags | (scanout ? BO_USE_SCANOUT : 0));
			}
			free(modifiers);
		} else {
			bool scanout = false;
			switch (format) {
			case DRM_FORMAT_ARGB8888:
			case DRM_FORMAT_XRGB8888:
			case DRM_FORMAT_ABGR8888:
			case DRM_FORMAT_XBGR8888:
			case DRM_FORMAT_ABGR2101010:
			case DRM_FORMAT_ARGB2101010:
			case DRM_FORMAT_XBGR2101010:
			case DRM_FORMAT_XRGB2101010:
				scanout = true;
				break;
			default:
				break;
			}
			metadata.tiling = TILE_TYPE_DRI;
			drv_add_combination(drv, format, &metadata,
					    use_flags | (scanout ? BO_USE_SCANOUT : 0));
		}
	}
	return 0;
}

static void amdgpu_close(struct driver *drv)
{
	sdma_finish(drv->priv, drv_get_fd(drv));
	dri_close(drv);
	free(drv->priv);
	drv->priv = NULL;
}

static int amdgpu_create_bo_linear(struct bo *bo, uint32_t width, uint32_t height, uint32_t format,
				   uint64_t use_flags)
{
	int ret;
	size_t num_planes;
	uint32_t plane, stride;
	union drm_amdgpu_gem_create gem_create = { { 0 } };
	struct amdgpu_priv *priv = bo->drv->priv;

	stride = drv_stride_from_format(format, width, 0);
	num_planes = drv_num_planes_from_format(format);

	/*
	 * For multiplane formats, align the stride to 512 to ensure that subsample strides are 256
	 * aligned. This uses more memory than necessary since the first plane only needs to be
	 * 256 aligned, but it's acceptable for a short-term fix. It's probably safe for other gpu
	 * families, but let's restrict it to Raven and Stoney for now (b/171013552, b/190484589).
	 * This only applies to the Android YUV (multiplane) format.
	 * */
	if (format == DRM_FORMAT_YVU420_ANDROID && (priv->dev_info.family == AMDGPU_FAMILY_RV ||
						    priv->dev_info.family == AMDGPU_FAMILY_CZ))
		stride = ALIGN(stride, 512);
	else
		stride = ALIGN(stride, 256);

	/*
	 * Currently, allocator used by chrome aligns the height for Encoder/
	 * Decoder buffers while allocator used by android(gralloc/minigbm)
	 * doesn't provide any aligment.
	 *
	 * See b/153130069
	 */
	if (use_flags & (BO_USE_HW_VIDEO_DECODER | BO_USE_HW_VIDEO_ENCODER))
		height = ALIGN(height, CHROME_HEIGHT_ALIGN);

	drv_bo_from_format(bo, stride, height, format);

	gem_create.in.bo_size =
	    ALIGN(bo->meta.total_size, priv->dev_info.virtual_address_alignment);
	gem_create.in.alignment = 256;
	gem_create.in.domain_flags = 0;

	if (use_flags & (BO_USE_LINEAR | BO_USE_SW_MASK))
		gem_create.in.domain_flags |= AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED;

	gem_create.in.domains = AMDGPU_GEM_DOMAIN_GTT;

	/* Scanout in GTT requires USWC, otherwise try to use cachable memory
	 * for buffers that are read often, because uncacheable reads can be
	 * very slow. USWC should be faster on the GPU though. */
	if ((use_flags & BO_USE_SCANOUT) || !(use_flags & BO_USE_SW_READ_OFTEN))
		gem_create.in.domain_flags |= AMDGPU_GEM_CREATE_CPU_GTT_USWC;

	/* For protected data Buffer needs to be allocated from TMZ */
	if (use_flags & BO_USE_PROTECTED)
		gem_create.in.domain_flags |= AMDGPU_GEM_CREATE_ENCRYPTED;

	/* Allocate the buffer with the preferred heap. */
	ret = drmCommandWriteRead(drv_get_fd(bo->drv), DRM_AMDGPU_GEM_CREATE, &gem_create,
				  sizeof(gem_create));
	if (ret < 0)
		return ret;

	for (plane = 0; plane < bo->meta.num_planes; plane++)
		bo->handles[plane].u32 = gem_create.out.handle;

	bo->meta.format_modifier = DRM_FORMAT_MOD_LINEAR;

	return 0;
}

static int amdgpu_create_bo(struct bo *bo, uint32_t width, uint32_t height, uint32_t format,
			    uint64_t use_flags)
{
	struct combination *combo;
	struct amdgpu_priv *priv = bo->drv->priv;

	combo = drv_get_combination(bo->drv, format, use_flags);
	if (!combo)
		return -EINVAL;

	if (combo->metadata.tiling == TILE_TYPE_DRI) {
		// See b/122049612
		if (use_flags & (BO_USE_SCANOUT) && priv->dev_info.family == AMDGPU_FAMILY_CZ) {
			uint32_t bytes_per_pixel = drv_bytes_per_pixel_from_format(format, 0);
			width = ALIGN(width, 256 / bytes_per_pixel);
		}

		return dri_bo_create(bo, width, height, format, use_flags);
	} else if (combo->metadata.tiling == TILE_TYPE_DRI_MODIFIER) {
		return dri_bo_create_with_modifiers(bo, width, height, format,
						    &combo->metadata.modifier, 1);
	}

	return amdgpu_create_bo_linear(bo, width, height, format, use_flags);
}

static int amdgpu_create_bo_with_modifiers(struct bo *bo, uint32_t width, uint32_t height,
					   uint32_t format, const uint64_t *modifiers,
					   uint32_t count)
{
	bool only_use_linear = true;

	for (uint32_t i = 0; i < count; ++i)
		if (modifiers[i] != DRM_FORMAT_MOD_LINEAR)
			only_use_linear = false;

	if (only_use_linear)
		return amdgpu_create_bo_linear(bo, width, height, format, BO_USE_SCANOUT);

	return dri_bo_create_with_modifiers(bo, width, height, format, modifiers, count);
}

static int amdgpu_import_bo(struct bo *bo, struct drv_import_fd_data *data)
{
	bool dri_tiling = data->format_modifier != DRM_FORMAT_MOD_LINEAR;
	if (data->format_modifier == DRM_FORMAT_MOD_INVALID) {
		struct combination *combo;
		combo = drv_get_combination(bo->drv, data->format, data->use_flags);
		if (!combo)
			return -EINVAL;

		dri_tiling = combo->metadata.tiling != TILE_TYPE_LINEAR;
	}

	bo->meta.num_planes =
	    dri_num_planes_from_modifier(bo->drv, data->format, data->format_modifier);

	if (dri_tiling)
		return dri_bo_import(bo, data);
	else
		return drv_prime_bo_import(bo, data);
}

static int amdgpu_release_bo(struct bo *bo)
{
	if (bo->priv)
		return dri_bo_release(bo);

	return 0;
}

static int amdgpu_destroy_bo(struct bo *bo)
{
	if (bo->priv)
		return dri_bo_destroy(bo);
	else
		return drv_gem_bo_destroy(bo);
}

static void *amdgpu_map_bo(struct bo *bo, struct vma *vma, size_t plane, uint32_t map_flags)
{
	void *addr = MAP_FAILED;
	int ret;
	union drm_amdgpu_gem_mmap gem_map = { { 0 } };
	struct drm_amdgpu_gem_create_in bo_info = { 0 };
	struct drm_amdgpu_gem_op gem_op = { 0 };
	uint32_t handle = bo->handles[plane].u32;
	struct amdgpu_linear_vma_priv *priv = NULL;
	struct amdgpu_priv *drv_priv;

	if (bo->priv)
		return dri_bo_map(bo, vma, plane, map_flags);

	drv_priv = bo->drv->priv;
	gem_op.handle = handle;
	gem_op.op = AMDGPU_GEM_OP_GET_GEM_CREATE_INFO;
	gem_op.value = (uintptr_t)&bo_info;

	ret = drmCommandWriteRead(bo->drv->fd, DRM_AMDGPU_GEM_OP, &gem_op, sizeof(gem_op));
	if (ret)
		return MAP_FAILED;

	vma->length = bo_info.bo_size;

	if (((bo_info.domains & AMDGPU_GEM_DOMAIN_VRAM) ||
	     (bo_info.domain_flags & AMDGPU_GEM_CREATE_CPU_GTT_USWC)) &&
	    drv_priv->sdma_cmdbuf_map) {
		union drm_amdgpu_gem_create gem_create = { { 0 } };

		priv = calloc(1, sizeof(struct amdgpu_linear_vma_priv));
		if (!priv)
			return MAP_FAILED;

		gem_create.in.bo_size = bo_info.bo_size;
		gem_create.in.alignment = 4096;
		gem_create.in.domains = AMDGPU_GEM_DOMAIN_GTT;

		ret = drmCommandWriteRead(bo->drv->fd, DRM_AMDGPU_GEM_CREATE, &gem_create,
					  sizeof(gem_create));
		if (ret < 0) {
			drv_loge("GEM create failed\n");
			free(priv);
			return MAP_FAILED;
		}

		priv->map_flags = map_flags;
		handle = priv->handle = gem_create.out.handle;

		ret = sdma_copy(bo->drv->priv, bo->drv->fd, bo->handles[0].u32, priv->handle,
				bo_info.bo_size);
		if (ret) {
			drv_loge("SDMA copy for read failed\n");
			goto fail;
		}
	}

	gem_map.in.handle = handle;
	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_AMDGPU_GEM_MMAP, &gem_map);
	if (ret) {
		drv_loge("DRM_IOCTL_AMDGPU_GEM_MMAP failed\n");
		goto fail;
	}

	addr = mmap(0, bo->meta.total_size, drv_get_prot(map_flags), MAP_SHARED, bo->drv->fd,
		    gem_map.out.addr_ptr);
	if (addr == MAP_FAILED)
		goto fail;

	vma->priv = priv;
	return addr;

fail:
	if (priv) {
		struct drm_gem_close gem_close = { 0 };
		gem_close.handle = priv->handle;
		drmIoctl(bo->drv->fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
		free(priv);
	}
	return MAP_FAILED;
}

static int amdgpu_unmap_bo(struct bo *bo, struct vma *vma)
{
	if (bo->priv) {
		return dri_bo_unmap(bo, vma);
	} else {
		int r = munmap(vma->addr, vma->length);
		if (r)
			return r;

		if (vma->priv) {
			struct amdgpu_linear_vma_priv *priv = vma->priv;
			struct drm_gem_close gem_close = { 0 };

			if (BO_MAP_WRITE & priv->map_flags) {
				r = sdma_copy(bo->drv->priv, bo->drv->fd, priv->handle,
					      bo->handles[0].u32, vma->length);
				if (r)
					return r;
			}

			gem_close.handle = priv->handle;
			r = drmIoctl(bo->drv->fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
		}

		return 0;
	}
}

static int amdgpu_bo_invalidate(struct bo *bo, struct mapping *mapping)
{
	int ret;
	union drm_amdgpu_gem_wait_idle wait_idle = { { 0 } };

	if (bo->priv)
		return 0;

	wait_idle.in.handle = bo->handles[0].u32;
	wait_idle.in.timeout = AMDGPU_TIMEOUT_INFINITE;

	ret = drmCommandWriteRead(bo->drv->fd, DRM_AMDGPU_GEM_WAIT_IDLE, &wait_idle,
				  sizeof(wait_idle));

	if (ret < 0) {
		drv_loge("DRM_AMDGPU_GEM_WAIT_IDLE failed with %d\n", ret);
		return ret;
	}

	if (ret == 0 && wait_idle.out.status)
		drv_loge("DRM_AMDGPU_GEM_WAIT_IDLE BO is busy\n");

	return 0;
}

const struct backend backend_amdgpu = {
	.name = "amdgpu",
	.init = amdgpu_init,
	.close = amdgpu_close,
	.bo_create = amdgpu_create_bo,
	.bo_create_with_modifiers = amdgpu_create_bo_with_modifiers,
	.bo_release = amdgpu_release_bo,
	.bo_destroy = amdgpu_destroy_bo,
	.bo_import = amdgpu_import_bo,
	.bo_map = amdgpu_map_bo,
	.bo_unmap = amdgpu_unmap_bo,
	.bo_invalidate = amdgpu_bo_invalidate,
	.resolve_format_and_use_flags = drv_resolve_format_and_use_flags_helper,
	.num_planes_from_modifier = dri_num_planes_from_modifier,
};

#endif
