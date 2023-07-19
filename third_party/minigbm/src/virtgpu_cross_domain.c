/*
 * Copyright 2021 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <xf86drm.h>

#include "drv_helpers.h"
#include "drv_priv.h"
#include "external/virtgpu_cross_domain_protocol.h"
#include "external/virtgpu_drm.h"
#include "util.h"
#include "virtgpu.h"

#define CAPSET_CROSS_DOMAIN 5
#define CAPSET_CROSS_FAKE 30

static const uint32_t scanout_render_formats[] = { DRM_FORMAT_ABGR2101010, DRM_FORMAT_ABGR8888,
						   DRM_FORMAT_ARGB2101010, DRM_FORMAT_ARGB8888,
						   DRM_FORMAT_RGB565,	   DRM_FORMAT_XBGR2101010,
						   DRM_FORMAT_XBGR8888,	   DRM_FORMAT_XRGB2101010,
						   DRM_FORMAT_XRGB8888 };

static const uint32_t render_formats[] = { DRM_FORMAT_ABGR16161616F };

static const uint32_t texture_only_formats[] = { DRM_FORMAT_R8, DRM_FORMAT_NV12, DRM_FORMAT_P010,
						 DRM_FORMAT_YVU420, DRM_FORMAT_YVU420_ANDROID };

extern struct virtgpu_param params[];

struct cross_domain_private {
	uint32_t ring_handle;
	void *ring_addr;
	struct drv_array *metadata_cache;
	pthread_mutex_t metadata_cache_lock;
};

static void cross_domain_release_private(struct driver *drv)
{
	int ret;
	struct cross_domain_private *priv = drv->priv;
	struct drm_gem_close gem_close = { 0 };

	if (priv->ring_addr != MAP_FAILED)
		munmap(priv->ring_addr, PAGE_SIZE);

	if (priv->ring_handle) {
		gem_close.handle = priv->ring_handle;

		ret = drmIoctl(drv->fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
		if (ret) {
			drv_loge("DRM_IOCTL_GEM_CLOSE failed (handle=%x) error %d\n",
				 priv->ring_handle, ret);
		}
	}

	if (priv->metadata_cache)
		drv_array_destroy(priv->metadata_cache);

	pthread_mutex_destroy(&priv->metadata_cache_lock);

	free(priv);
}

static void add_combinations(struct driver *drv)
{
	struct format_metadata metadata;

	// Linear metadata always supported.
	metadata.tiling = 0;
	metadata.priority = 1;
	metadata.modifier = DRM_FORMAT_MOD_LINEAR;

	drv_add_combinations(drv, scanout_render_formats, ARRAY_SIZE(scanout_render_formats),
			     &metadata, BO_USE_RENDER_MASK | BO_USE_SCANOUT);

	drv_add_combinations(drv, render_formats, ARRAY_SIZE(render_formats), &metadata,
			     BO_USE_RENDER_MASK);

	drv_add_combinations(drv, texture_only_formats, ARRAY_SIZE(texture_only_formats), &metadata,
			     BO_USE_TEXTURE_MASK);

	/* Android CTS tests require this. */
	drv_add_combination(drv, DRM_FORMAT_BGR888, &metadata, BO_USE_SW_MASK);

	drv_modify_combination(drv, DRM_FORMAT_YVU420, &metadata, BO_USE_HW_VIDEO_ENCODER);
	drv_modify_combination(drv, DRM_FORMAT_NV12, &metadata,
			       BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE | BO_USE_HW_VIDEO_DECODER |
				   BO_USE_SCANOUT | BO_USE_HW_VIDEO_ENCODER);

	/*
	 * R8 format is used for Android's HAL_PIXEL_FORMAT_BLOB and is used for JPEG snapshots
	 * from camera, input/output from hardware decoder/encoder and sensors, and
	 * AHBs used as SSBOs/UBOs.
	 */
	drv_modify_combination(drv, DRM_FORMAT_R8, &metadata,
			       BO_USE_CAMERA_READ | BO_USE_CAMERA_WRITE | BO_USE_HW_VIDEO_DECODER |
				   BO_USE_HW_VIDEO_ENCODER | BO_USE_SENSOR_DIRECT_DATA |
				   BO_USE_GPU_DATA_BUFFER);

	drv_modify_linear_combinations(drv);
}

static int cross_domain_submit_cmd(struct driver *drv, uint32_t *cmd, uint32_t cmd_size, bool wait)
{
	int ret;
	struct drm_virtgpu_3d_wait wait_3d = { 0 };
	struct drm_virtgpu_execbuffer exec = { 0 };
	struct cross_domain_private *priv = drv->priv;

	exec.command = (uint64_t)&cmd[0];
	exec.size = cmd_size;
	if (wait) {
		exec.flags = VIRTGPU_EXECBUF_RING_IDX;
		exec.bo_handles = (uint64_t)&priv->ring_handle;
		exec.num_bo_handles = 1;
	}

	ret = drmIoctl(drv->fd, DRM_IOCTL_VIRTGPU_EXECBUFFER, &exec);
	if (ret < 0) {
		drv_loge("DRM_IOCTL_VIRTGPU_EXECBUFFER failed with %s\n", strerror(errno));
		return -EINVAL;
	}

	ret = -EAGAIN;
	while (ret == -EAGAIN) {
		wait_3d.handle = priv->ring_handle;
		ret = drmIoctl(drv->fd, DRM_IOCTL_VIRTGPU_WAIT, &wait_3d);
	}

	if (ret < 0) {
		drv_loge("DRM_IOCTL_VIRTGPU_WAIT failed with %s\n", strerror(errno));
		return ret;
	}

	return 0;
}

static bool metadata_equal(struct bo_metadata *current, struct bo_metadata *cached)
{
	if ((current->width == cached->width) && (current->height == cached->height) &&
	    (current->format == cached->format) && (current->use_flags == cached->use_flags))
		return true;
	return false;
}

static int cross_domain_metadata_query(struct driver *drv, struct bo_metadata *metadata)
{
	int ret = 0;
	struct bo_metadata *cached_data = NULL;
	struct cross_domain_private *priv = drv->priv;
	struct CrossDomainGetImageRequirements cmd_get_reqs;
	uint32_t *addr = (uint32_t *)priv->ring_addr;
	uint32_t plane, remaining_size;

	memset(&cmd_get_reqs, 0, sizeof(cmd_get_reqs));
	pthread_mutex_lock(&priv->metadata_cache_lock);
	for (uint32_t i = 0; i < drv_array_size(priv->metadata_cache); i++) {
		cached_data = (struct bo_metadata *)drv_array_at_idx(priv->metadata_cache, i);
		if (!metadata_equal(metadata, cached_data))
			continue;

		memcpy(metadata, cached_data, sizeof(*cached_data));
		goto out_unlock;
	}

	cmd_get_reqs.hdr.cmd = CROSS_DOMAIN_CMD_GET_IMAGE_REQUIREMENTS;
	cmd_get_reqs.hdr.cmd_size = sizeof(struct CrossDomainGetImageRequirements);

	cmd_get_reqs.width = metadata->width;
	cmd_get_reqs.height = metadata->height;
	cmd_get_reqs.drm_format =
	    (metadata->format == DRM_FORMAT_YVU420_ANDROID) ? DRM_FORMAT_YVU420 : metadata->format;
	cmd_get_reqs.flags = metadata->use_flags;

	/*
	 * It is possible to avoid blocking other bo_create() calls by unlocking before
	 * cross_domain_submit_cmd() and re-locking afterwards.  However, that would require
	 * another scan of the metadata cache before drv_array_append in case two bo_create() calls
	 * do the same metadata query.  Until cross_domain functionality is more widely tested,
	 * leave this optimization out for now.
	 */
	ret = cross_domain_submit_cmd(drv, (uint32_t *)&cmd_get_reqs, cmd_get_reqs.hdr.cmd_size,
				      true);
	if (ret < 0)
		goto out_unlock;

	memcpy(&metadata->strides, &addr[0], 4 * sizeof(uint32_t));
	memcpy(&metadata->offsets, &addr[4], 4 * sizeof(uint32_t));
	memcpy(&metadata->format_modifier, &addr[8], sizeof(uint64_t));
	memcpy(&metadata->total_size, &addr[10], sizeof(uint64_t));
	memcpy(&metadata->blob_id, &addr[12], sizeof(uint32_t));

	metadata->map_info = addr[13];
	metadata->memory_idx = addr[14];
	metadata->physical_device_idx = addr[15];

	/* Detect buffers, which have no particular stride alignment requirement: */
	if ((metadata->height == 1) && (metadata->format == DRM_FORMAT_R8)) {
		metadata->strides[0] = metadata->width;
	}

	remaining_size = metadata->total_size;
	for (plane = 0; plane < metadata->num_planes; plane++) {
		if (plane != 0) {
			metadata->sizes[plane - 1] = metadata->offsets[plane];
			remaining_size -= metadata->offsets[plane];
		}
	}

	metadata->sizes[plane - 1] = remaining_size;
	drv_array_append(priv->metadata_cache, metadata);

out_unlock:
	pthread_mutex_unlock(&priv->metadata_cache_lock);
	return ret;
}

/* Fill out metadata for guest buffers, used only for CPU access: */
void cross_domain_get_emulated_metadata(struct bo_metadata *metadata)
{
	uint32_t offset = 0;

	for (size_t i = 0; i < metadata->num_planes; i++) {
		metadata->strides[i] = drv_stride_from_format(metadata->format, metadata->width, i);
		metadata->sizes[i] = drv_size_from_format(metadata->format, metadata->strides[i],
							  metadata->height, i);
		metadata->offsets[i] = offset;
		offset += metadata->sizes[i];
	}

	metadata->total_size = offset;
}

static int cross_domain_init(struct driver *drv)
{
	int ret;
	struct cross_domain_private *priv;
	struct drm_virtgpu_map map = { 0 };
	struct drm_virtgpu_get_caps args = { 0 };
	struct drm_virtgpu_context_init init = { 0 };
	struct drm_virtgpu_resource_create_blob drm_rc_blob = { 0 };
	struct drm_virtgpu_context_set_param ctx_set_params[2] = { { 0 } };

	struct CrossDomainInit cmd_init;
	struct CrossDomainCapabilities cross_domain_caps;

	memset(&cmd_init, 0, sizeof(cmd_init));
	if (!params[param_context_init].value)
		return -ENOTSUP;

	if ((params[param_supported_capset_ids].value & (1 << CAPSET_CROSS_DOMAIN)) == 0)
		return -ENOTSUP;

	if (!params[param_resource_blob].value)
		return -ENOTSUP;

	/// Need zero copy memory
	if (!params[param_host_visible].value && !params[param_create_guest_handle].value)
		return -ENOTSUP;

	priv = calloc(1, sizeof(*priv));
	if (!priv)
		return -ENOMEM;

	ret = pthread_mutex_init(&priv->metadata_cache_lock, NULL);
	if (ret) {
		free(priv);
		return ret;
	}

	priv->metadata_cache = drv_array_init(sizeof(struct bo_metadata));
	if (!priv->metadata_cache) {
		ret = -ENOMEM;
		goto free_private;
	}

	priv->ring_addr = MAP_FAILED;
	drv->priv = priv;

	args.cap_set_id = CAPSET_CROSS_DOMAIN;
	args.size = sizeof(struct CrossDomainCapabilities);
	args.addr = (unsigned long long)&cross_domain_caps;

	ret = drmIoctl(drv->fd, DRM_IOCTL_VIRTGPU_GET_CAPS, &args);
	if (ret) {
		drv_loge("DRM_IOCTL_VIRTGPU_GET_CAPS failed with %s\n", strerror(errno));
		goto free_private;
	}

	// When 3D features are avilable, but the host does not support external memory, fall back
	// to the virgl minigbm backend.  This typically means the guest side minigbm resource will
	// be backed by a host OpenGL texture.
	if (!cross_domain_caps.supports_external_gpu_memory && params[param_3d].value) {
		ret = -ENOTSUP;
		goto free_private;
	}

	// Intialize the cross domain context.  Create one fence context to wait for metadata
	// queries.
	ctx_set_params[0].param = VIRTGPU_CONTEXT_PARAM_CAPSET_ID;
	ctx_set_params[0].value = CAPSET_CROSS_DOMAIN;
	ctx_set_params[1].param = VIRTGPU_CONTEXT_PARAM_NUM_RINGS;
	ctx_set_params[1].value = 1;

	init.ctx_set_params = (unsigned long long)&ctx_set_params[0];
	init.num_params = 2;
	ret = drmIoctl(drv->fd, DRM_IOCTL_VIRTGPU_CONTEXT_INIT, &init);
	if (ret) {
		drv_loge("DRM_IOCTL_VIRTGPU_CONTEXT_INIT failed with %s\n", strerror(errno));
		goto free_private;
	}

	// Create a shared ring buffer to read metadata queries.
	drm_rc_blob.size = PAGE_SIZE;
	drm_rc_blob.blob_mem = VIRTGPU_BLOB_MEM_GUEST;
	drm_rc_blob.blob_flags = VIRTGPU_BLOB_FLAG_USE_MAPPABLE;

	ret = drmIoctl(drv->fd, DRM_IOCTL_VIRTGPU_RESOURCE_CREATE_BLOB, &drm_rc_blob);
	if (ret < 0) {
		drv_loge("DRM_VIRTGPU_RESOURCE_CREATE_BLOB failed with %s\n", strerror(errno));
		goto free_private;
	}

	priv->ring_handle = drm_rc_blob.bo_handle;

	// Map shared ring buffer.
	map.handle = priv->ring_handle;
	ret = drmIoctl(drv->fd, DRM_IOCTL_VIRTGPU_MAP, &map);
	if (ret < 0) {
		drv_loge("DRM_IOCTL_VIRTGPU_MAP failed with %s\n", strerror(errno));
		goto free_private;
	}

	priv->ring_addr =
	    mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, drv->fd, map.offset);

	if (priv->ring_addr == MAP_FAILED) {
		drv_loge("mmap failed with %s\n", strerror(errno));
		goto free_private;
	}

	// Notify host about ring buffer
	cmd_init.hdr.cmd = CROSS_DOMAIN_CMD_INIT;
	cmd_init.hdr.cmd_size = sizeof(struct CrossDomainInit);
	cmd_init.ring_id = drm_rc_blob.res_handle;
	ret = cross_domain_submit_cmd(drv, (uint32_t *)&cmd_init, cmd_init.hdr.cmd_size, false);
	if (ret < 0)
		goto free_private;

	// minigbm bookkeeping
	add_combinations(drv);
	return 0;

free_private:
	cross_domain_release_private(drv);
	return ret;
}

static void cross_domain_close(struct driver *drv)
{
	cross_domain_release_private(drv);
}

static int cross_domain_bo_create(struct bo *bo, uint32_t width, uint32_t height, uint32_t format,
				  uint64_t use_flags)
{
	int ret;
	uint32_t blob_flags = VIRTGPU_BLOB_FLAG_USE_SHAREABLE;
	struct drm_virtgpu_resource_create_blob drm_rc_blob = { 0 };

	if (use_flags & (BO_USE_SW_MASK | BO_USE_GPU_DATA_BUFFER))
		blob_flags |= VIRTGPU_BLOB_FLAG_USE_MAPPABLE;

	if (!(use_flags & BO_USE_HW_MASK)) {
		cross_domain_get_emulated_metadata(&bo->meta);
		drm_rc_blob.blob_mem = VIRTGPU_BLOB_MEM_GUEST;
	} else {
		ret = cross_domain_metadata_query(bo->drv, &bo->meta);
		if (ret < 0) {
			drv_loge("Metadata query failed");
			return ret;
		}

		if (params[param_cross_device].value)
			blob_flags |= VIRTGPU_BLOB_FLAG_USE_CROSS_DEVICE;

		/// It may be possible to have host3d blobs and handles from guest memory at the
		/// same time. But for the immediate use cases, we will either have one or the
		/// other.  For now, just prefer guest memory since adding that feature is more
		/// involved (requires --udmabuf flag to crosvm), so developers would likely test
		/// that.
		if (params[param_create_guest_handle].value) {
			drm_rc_blob.blob_mem = VIRTGPU_BLOB_MEM_GUEST;
			blob_flags |= VIRTGPU_BLOB_FLAG_CREATE_GUEST_HANDLE;
		} else if (params[param_host_visible].value) {
			drm_rc_blob.blob_mem = VIRTGPU_BLOB_MEM_HOST3D;
		}
		drm_rc_blob.blob_id = (uint64_t)bo->meta.blob_id;
	}

	drm_rc_blob.size = bo->meta.total_size;
	drm_rc_blob.blob_flags = blob_flags;

	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_VIRTGPU_RESOURCE_CREATE_BLOB, &drm_rc_blob);
	if (ret < 0) {
		drv_loge("DRM_VIRTGPU_RESOURCE_CREATE_BLOB failed with %s\n", strerror(errno));
		return -errno;
	}

	for (uint32_t plane = 0; plane < bo->meta.num_planes; plane++)
		bo->handles[plane].u32 = drm_rc_blob.bo_handle;

	return 0;
}

static void *cross_domain_bo_map(struct bo *bo, struct vma *vma, size_t plane, uint32_t map_flags)
{
	int ret;
	struct drm_virtgpu_map gem_map = { 0 };

	gem_map.handle = bo->handles[0].u32;
	ret = drmIoctl(bo->drv->fd, DRM_IOCTL_VIRTGPU_MAP, &gem_map);
	if (ret) {
		drv_loge("DRM_IOCTL_VIRTGPU_MAP failed with %s\n", strerror(errno));
		return MAP_FAILED;
	}

	vma->length = bo->meta.total_size;
	return mmap(0, bo->meta.total_size, drv_get_prot(map_flags), MAP_SHARED, bo->drv->fd,
		    gem_map.offset);
}

const struct backend virtgpu_cross_domain = {
	.name = "virtgpu_cross_domain",
	.init = cross_domain_init,
	.close = cross_domain_close,
	.bo_create = cross_domain_bo_create,
	.bo_import = drv_prime_bo_import,
	.bo_destroy = drv_gem_bo_destroy,
	.bo_map = cross_domain_bo_map,
	.bo_unmap = drv_bo_munmap,
	.resolve_format_and_use_flags = drv_resolve_format_and_use_flags_helper,
};
