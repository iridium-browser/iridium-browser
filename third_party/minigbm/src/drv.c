/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <xf86drm.h>

#ifdef __ANDROID__
#include <cutils/log.h>
#include <libgen.h>
#endif

#include "drv_helpers.h"
#include "drv_priv.h"
#include "util.h"

#ifdef DRV_AMDGPU
extern const struct backend backend_amdgpu;
#endif
#ifdef DRV_I915
extern const struct backend backend_i915;
#endif
#ifdef DRV_MEDIATEK
extern const struct backend backend_mediatek;
#endif
#ifdef DRV_MSM
extern const struct backend backend_msm;
#endif
#ifdef DRV_ROCKCHIP
extern const struct backend backend_rockchip;
#endif
#ifdef DRV_VC4
extern const struct backend backend_vc4;
#endif

// Dumb / generic drivers
extern const struct backend backend_evdi;
extern const struct backend backend_marvell;
extern const struct backend backend_meson;
extern const struct backend backend_nouveau;
extern const struct backend backend_komeda;
extern const struct backend backend_radeon;
extern const struct backend backend_synaptics;
extern const struct backend backend_virtgpu;
extern const struct backend backend_udl;
extern const struct backend backend_vkms;

static const struct backend *drv_get_backend(int fd)
{
	drmVersionPtr drm_version;
	unsigned int i;

	drm_version = drmGetVersion(fd);

	if (!drm_version)
		return NULL;

	const struct backend *backend_list[] = {
#ifdef DRV_AMDGPU
		&backend_amdgpu,
#endif
#ifdef DRV_I915
		&backend_i915,
#endif
#ifdef DRV_MEDIATEK
		&backend_mediatek,
#endif
#ifdef DRV_MSM
		&backend_msm,
#endif
#ifdef DRV_ROCKCHIP
		&backend_rockchip,
#endif
#ifdef DRV_VC4
		&backend_vc4,
#endif
		&backend_evdi,	   &backend_marvell, &backend_meson,	 &backend_nouveau,
		&backend_komeda,   &backend_radeon,  &backend_synaptics, &backend_virtgpu,
		&backend_udl,	   &backend_virtgpu, &backend_vkms
	};

	for (i = 0; i < ARRAY_SIZE(backend_list); i++) {
		const struct backend *b = backend_list[i];
		if (!strcmp(drm_version->name, b->name)) {
			drmFreeVersion(drm_version);
			return b;
		}
	}

	drmFreeVersion(drm_version);
	return NULL;
}

struct driver *drv_create(int fd)
{
	struct driver *drv;
	int ret;

	drv = (struct driver *)calloc(1, sizeof(*drv));

	if (!drv)
		return NULL;

	char *minigbm_debug;
	minigbm_debug = getenv("MINIGBM_DEBUG");
	drv->compression = (minigbm_debug == NULL) || (strcmp(minigbm_debug, "nocompression") != 0);

	drv->fd = fd;
	drv->backend = drv_get_backend(fd);

	if (!drv->backend)
		goto free_driver;

	if (pthread_mutex_init(&drv->buffer_table_lock, NULL))
		goto free_driver;

	drv->buffer_table = drmHashCreate();
	if (!drv->buffer_table)
		goto free_buffer_table_lock;

	if (pthread_mutex_init(&drv->mappings_lock, NULL))
		goto free_buffer_table;

	drv->mappings = drv_array_init(sizeof(struct mapping));
	if (!drv->mappings)
		goto free_mappings_lock;

	drv->combos = drv_array_init(sizeof(struct combination));
	if (!drv->combos)
		goto free_mappings;

	if (drv->backend->init) {
		ret = drv->backend->init(drv);
		if (ret) {
			drv_array_destroy(drv->combos);
			goto free_mappings;
		}
	}

	return drv;

free_mappings:
	drv_array_destroy(drv->mappings);
free_mappings_lock:
	pthread_mutex_destroy(&drv->mappings_lock);
free_buffer_table:
	drmHashDestroy(drv->buffer_table);
free_buffer_table_lock:
	pthread_mutex_destroy(&drv->buffer_table_lock);
free_driver:
	free(drv);
	return NULL;
}

void drv_destroy(struct driver *drv)
{
	if (drv->backend->close)
		drv->backend->close(drv);

	drv_array_destroy(drv->combos);

	drv_array_destroy(drv->mappings);
	pthread_mutex_destroy(&drv->mappings_lock);

	drmHashDestroy(drv->buffer_table);
	pthread_mutex_destroy(&drv->buffer_table_lock);

	free(drv);
}

int drv_get_fd(struct driver *drv)
{
	return drv->fd;
}

const char *drv_get_name(struct driver *drv)
{
	return drv->backend->name;
}

struct combination *drv_get_combination(struct driver *drv, uint32_t format, uint64_t use_flags)
{
	struct combination *curr, *best;

	if (format == DRM_FORMAT_NONE || use_flags == BO_USE_NONE)
		return 0;

	best = NULL;
	uint32_t i;
	for (i = 0; i < drv_array_size(drv->combos); i++) {
		curr = drv_array_at_idx(drv->combos, i);
		if ((format == curr->format) && use_flags == (curr->use_flags & use_flags))
			if (!best || best->metadata.priority < curr->metadata.priority)
				best = curr;
	}

	return best;
}

struct bo *drv_bo_new(struct driver *drv, uint32_t width, uint32_t height, uint32_t format,
		      uint64_t use_flags, bool is_test_buffer)
{

	struct bo *bo;
	bo = (struct bo *)calloc(1, sizeof(*bo));

	if (!bo)
		return NULL;

	bo->drv = drv;
	bo->meta.width = width;
	bo->meta.height = height;
	bo->meta.format = format;
	bo->meta.use_flags = use_flags;
	bo->meta.num_planes = drv_num_planes_from_format(format);
	bo->is_test_buffer = is_test_buffer;

	if (!bo->meta.num_planes) {
		free(bo);
		errno = EINVAL;
		return NULL;
	}

	return bo;
}

static void drv_bo_mapping_destroy(struct bo *bo)
{
	struct driver *drv = bo->drv;
	uint32_t idx = 0;

	/*
	 * This function is called right before the buffer is destroyed. It will free any mappings
	 * associated with the buffer.
	 */
	pthread_mutex_lock(&drv->mappings_lock);
	for (size_t plane = 0; plane < bo->meta.num_planes; plane++) {
		while (idx < drv_array_size(drv->mappings)) {
			struct mapping *mapping =
			    (struct mapping *)drv_array_at_idx(drv->mappings, idx);
			if (mapping->vma->handle != bo->handles[plane].u32) {
				idx++;
				continue;
			}

			if (!--mapping->vma->refcount) {
				int ret = drv->backend->bo_unmap(bo, mapping->vma);
				if (ret) {
					pthread_mutex_unlock(&drv->mappings_lock);
					assert(ret);
					drv_loge("munmap failed\n");
					return;
				}

				free(mapping->vma);
			}

			/* This shrinks and shifts the array, so don't increment idx. */
			drv_array_remove(drv->mappings, idx);
		}
	}
	pthread_mutex_unlock(&drv->mappings_lock);
}

/*
 * Acquire a reference on plane buffers of the bo.
 */
static void drv_bo_acquire(struct bo *bo)
{
	struct driver *drv = bo->drv;

	pthread_mutex_lock(&drv->buffer_table_lock);
	for (size_t plane = 0; plane < bo->meta.num_planes; plane++) {
		uintptr_t num = 0;

		if (!drmHashLookup(drv->buffer_table, bo->handles[plane].u32, (void **)&num))
			drmHashDelete(drv->buffer_table, bo->handles[plane].u32);

		drmHashInsert(drv->buffer_table, bo->handles[plane].u32, (void *)(num + 1));
	}
	pthread_mutex_unlock(&drv->buffer_table_lock);
}

/*
 * Release a reference on plane buffers of the bo. Return true when the bo has lost all its
 * references. Otherwise, return false.
 */
static bool drv_bo_release(struct bo *bo)
{
	struct driver *drv = bo->drv;
	uintptr_t num;

	if (drv->backend->bo_release)
		drv->backend->bo_release(bo);

	pthread_mutex_lock(&drv->buffer_table_lock);
	for (size_t plane = 0; plane < bo->meta.num_planes; plane++) {
		if (!drmHashLookup(drv->buffer_table, bo->handles[plane].u32, (void **)&num)) {
			drmHashDelete(drv->buffer_table, bo->handles[plane].u32);

			if (num > 1) {
				drmHashInsert(drv->buffer_table, bo->handles[plane].u32,
					      (void *)(num - 1));
			}
		}
	}

	/* The same buffer can back multiple planes with different offsets. */
	for (size_t plane = 0; plane < bo->meta.num_planes; plane++) {
		if (!drmHashLookup(drv->buffer_table, bo->handles[plane].u32, (void **)&num)) {
			/* num is positive if found in the hashmap. */
			pthread_mutex_unlock(&drv->buffer_table_lock);
			return false;
		}
	}
	pthread_mutex_unlock(&drv->buffer_table_lock);

	return true;
}

struct bo *drv_bo_create(struct driver *drv, uint32_t width, uint32_t height, uint32_t format,
			 uint64_t use_flags)
{
	int ret;
	struct bo *bo;
	bool is_test_alloc;

	is_test_alloc = use_flags & BO_USE_TEST_ALLOC;
	use_flags &= ~BO_USE_TEST_ALLOC;

	bo = drv_bo_new(drv, width, height, format, use_flags, is_test_alloc);

	if (!bo)
		return NULL;

	ret = -EINVAL;
	if (drv->backend->bo_compute_metadata) {
		ret = drv->backend->bo_compute_metadata(bo, width, height, format, use_flags, NULL,
							0);
		if (!is_test_alloc && ret == 0)
			ret = drv->backend->bo_create_from_metadata(bo);
	} else if (!is_test_alloc) {
		ret = drv->backend->bo_create(bo, width, height, format, use_flags);
	}

	if (ret) {
		errno = -ret;
		free(bo);
		return NULL;
	}

	drv_bo_acquire(bo);

	return bo;
}

struct bo *drv_bo_create_with_modifiers(struct driver *drv, uint32_t width, uint32_t height,
					uint32_t format, const uint64_t *modifiers, uint32_t count)
{
	int ret;
	struct bo *bo;

	if (!drv->backend->bo_create_with_modifiers && !drv->backend->bo_compute_metadata) {
		errno = ENOENT;
		return NULL;
	}

	bo = drv_bo_new(drv, width, height, format, BO_USE_NONE, false);

	if (!bo)
		return NULL;

	ret = -EINVAL;
	if (drv->backend->bo_compute_metadata) {
		ret = drv->backend->bo_compute_metadata(bo, width, height, format, BO_USE_NONE,
							modifiers, count);
		if (ret == 0)
			ret = drv->backend->bo_create_from_metadata(bo);
	} else {
		ret = drv->backend->bo_create_with_modifiers(bo, width, height, format, modifiers,
							     count);
	}

	if (ret) {
		free(bo);
		return NULL;
	}

	drv_bo_acquire(bo);

	return bo;
}

void drv_bo_destroy(struct bo *bo)
{
	if (!bo->is_test_buffer && drv_bo_release(bo)) {
		drv_bo_mapping_destroy(bo);
		bo->drv->backend->bo_destroy(bo);
	}

	free(bo);
}

struct bo *drv_bo_import(struct driver *drv, struct drv_import_fd_data *data)
{
	int ret;
	size_t plane;
	struct bo *bo;
	off_t seek_end;

	bo = drv_bo_new(drv, data->width, data->height, data->format, data->use_flags, false);

	if (!bo)
		return NULL;

	ret = drv->backend->bo_import(bo, data);
	if (ret) {
		free(bo);
		return NULL;
	}

	drv_bo_acquire(bo);

	bo->meta.format_modifier = data->format_modifier;
	for (plane = 0; plane < bo->meta.num_planes; plane++) {
		bo->meta.strides[plane] = data->strides[plane];
		bo->meta.offsets[plane] = data->offsets[plane];

		seek_end = lseek(data->fds[plane], 0, SEEK_END);
		if (seek_end == (off_t)(-1)) {
			drv_loge("lseek() failed with %s\n", strerror(errno));
			goto destroy_bo;
		}

		lseek(data->fds[plane], 0, SEEK_SET);
		if (plane == bo->meta.num_planes - 1 || data->offsets[plane + 1] == 0)
			bo->meta.sizes[plane] = seek_end - data->offsets[plane];
		else
			bo->meta.sizes[plane] = data->offsets[plane + 1] - data->offsets[plane];

		if ((int64_t)bo->meta.offsets[plane] + bo->meta.sizes[plane] > seek_end) {
			drv_loge("buffer size is too large.\n");
			goto destroy_bo;
		}

		bo->meta.total_size += bo->meta.sizes[plane];
	}

	return bo;

destroy_bo:
	drv_bo_destroy(bo);
	return NULL;
}

void *drv_bo_map(struct bo *bo, const struct rectangle *rect, uint32_t map_flags,
		 struct mapping **map_data, size_t plane)
{
	struct driver *drv = bo->drv;
	uint32_t i;
	uint8_t *addr;
	struct mapping mapping = { 0 };

	assert(rect->width >= 0);
	assert(rect->height >= 0);
	assert(rect->x + rect->width <= drv_bo_get_width(bo));
	assert(rect->y + rect->height <= drv_bo_get_height(bo));
	assert(BO_MAP_READ_WRITE & map_flags);
	/* No CPU access for protected buffers. */
	assert(!(bo->meta.use_flags & BO_USE_PROTECTED));

	if (bo->is_test_buffer)
		return MAP_FAILED;

	mapping.rect = *rect;
	mapping.refcount = 1;

	pthread_mutex_lock(&drv->mappings_lock);

	for (i = 0; i < drv_array_size(drv->mappings); i++) {
		struct mapping *prior = (struct mapping *)drv_array_at_idx(drv->mappings, i);
		if (prior->vma->handle != bo->handles[plane].u32 ||
		    prior->vma->map_flags != map_flags)
			continue;

		if (rect->x != prior->rect.x || rect->y != prior->rect.y ||
		    rect->width != prior->rect.width || rect->height != prior->rect.height)
			continue;

		prior->refcount++;
		*map_data = prior;
		goto exact_match;
	}

	for (i = 0; i < drv_array_size(drv->mappings); i++) {
		struct mapping *prior = (struct mapping *)drv_array_at_idx(drv->mappings, i);
		if (prior->vma->handle != bo->handles[plane].u32 ||
		    prior->vma->map_flags != map_flags)
			continue;

		prior->vma->refcount++;
		mapping.vma = prior->vma;
		goto success;
	}

	mapping.vma = calloc(1, sizeof(*mapping.vma));
	if (!mapping.vma) {
		*map_data = NULL;
		pthread_mutex_unlock(&drv->mappings_lock);
		return MAP_FAILED;
	}

	memcpy(mapping.vma->map_strides, bo->meta.strides, sizeof(mapping.vma->map_strides));
	addr = drv->backend->bo_map(bo, mapping.vma, plane, map_flags);
	if (addr == MAP_FAILED) {
		*map_data = NULL;
		free(mapping.vma);
		pthread_mutex_unlock(&drv->mappings_lock);
		return MAP_FAILED;
	}

	mapping.vma->refcount = 1;
	mapping.vma->addr = addr;
	mapping.vma->handle = bo->handles[plane].u32;
	mapping.vma->map_flags = map_flags;

success:
	*map_data = drv_array_append(drv->mappings, &mapping);
exact_match:
	drv_bo_invalidate(bo, *map_data);
	addr = (uint8_t *)((*map_data)->vma->addr);
	addr += drv_bo_get_plane_offset(bo, plane);
	pthread_mutex_unlock(&drv->mappings_lock);
	return (void *)addr;
}

int drv_bo_unmap(struct bo *bo, struct mapping *mapping)
{
	struct driver *drv = bo->drv;
	uint32_t i;
	int ret = 0;

	pthread_mutex_lock(&drv->mappings_lock);

	if (--mapping->refcount)
		goto out;

	if (!--mapping->vma->refcount) {
		ret = drv->backend->bo_unmap(bo, mapping->vma);
		free(mapping->vma);
	}

	for (i = 0; i < drv_array_size(drv->mappings); i++) {
		if (mapping == (struct mapping *)drv_array_at_idx(drv->mappings, i)) {
			drv_array_remove(drv->mappings, i);
			break;
		}
	}

out:
	pthread_mutex_unlock(&drv->mappings_lock);
	return ret;
}

int drv_bo_invalidate(struct bo *bo, struct mapping *mapping)
{
	int ret = 0;

	assert(mapping);
	assert(mapping->vma);
	assert(mapping->refcount > 0);
	assert(mapping->vma->refcount > 0);

	if (bo->drv->backend->bo_invalidate)
		ret = bo->drv->backend->bo_invalidate(bo, mapping);

	return ret;
}

int drv_bo_flush(struct bo *bo, struct mapping *mapping)
{
	int ret = 0;

	assert(mapping);
	assert(mapping->vma);
	assert(mapping->refcount > 0);
	assert(mapping->vma->refcount > 0);

	if (bo->drv->backend->bo_flush)
		ret = bo->drv->backend->bo_flush(bo, mapping);

	return ret;
}

int drv_bo_flush_or_unmap(struct bo *bo, struct mapping *mapping)
{
	int ret = 0;

	assert(mapping);
	assert(mapping->vma);
	assert(mapping->refcount > 0);
	assert(mapping->vma->refcount > 0);
	assert(!(bo->meta.use_flags & BO_USE_PROTECTED));

	if (bo->drv->backend->bo_flush)
		ret = bo->drv->backend->bo_flush(bo, mapping);
	else
		ret = drv_bo_unmap(bo, mapping);

	return ret;
}

uint32_t drv_bo_get_width(struct bo *bo)
{
	return bo->meta.width;
}

uint32_t drv_bo_get_height(struct bo *bo)
{
	return bo->meta.height;
}

size_t drv_bo_get_num_planes(struct bo *bo)
{
	return bo->meta.num_planes;
}

union bo_handle drv_bo_get_plane_handle(struct bo *bo, size_t plane)
{
	return bo->handles[plane];
}

#ifndef DRM_RDWR
#define DRM_RDWR O_RDWR
#endif

int drv_bo_get_plane_fd(struct bo *bo, size_t plane)
{

	int ret, fd;
	assert(plane < bo->meta.num_planes);

	if (bo->is_test_buffer)
		return -EINVAL;

	ret = drmPrimeHandleToFD(bo->drv->fd, bo->handles[plane].u32, DRM_CLOEXEC | DRM_RDWR, &fd);

	// Older DRM implementations blocked DRM_RDWR, but gave a read/write mapping anyways
	if (ret)
		ret = drmPrimeHandleToFD(bo->drv->fd, bo->handles[plane].u32, DRM_CLOEXEC, &fd);

	if (ret)
		drv_loge("Failed to get plane fd: %s\n", strerror(errno));

	return (ret) ? ret : fd;
}

uint32_t drv_bo_get_plane_offset(struct bo *bo, size_t plane)
{
	assert(plane < bo->meta.num_planes);
	return bo->meta.offsets[plane];
}

uint32_t drv_bo_get_plane_size(struct bo *bo, size_t plane)
{
	assert(plane < bo->meta.num_planes);
	return bo->meta.sizes[plane];
}

uint32_t drv_bo_get_plane_stride(struct bo *bo, size_t plane)
{
	assert(plane < bo->meta.num_planes);
	return bo->meta.strides[plane];
}

uint64_t drv_bo_get_format_modifier(struct bo *bo)
{
	return bo->meta.format_modifier;
}

uint32_t drv_bo_get_format(struct bo *bo)
{
	return bo->meta.format;
}

uint32_t drv_bo_get_tiling(struct bo *bo)
{
	return bo->meta.tiling;
}

uint64_t drv_bo_get_use_flags(struct bo *bo)
{
	return bo->meta.use_flags;
}

size_t drv_bo_get_total_size(struct bo *bo)
{
	return bo->meta.total_size;
}

/*
 * Map internal fourcc codes back to standard fourcc codes.
 */
uint32_t drv_get_standard_fourcc(uint32_t fourcc_internal)
{
	return (fourcc_internal == DRM_FORMAT_YVU420_ANDROID) ? DRM_FORMAT_YVU420 : fourcc_internal;
}

void drv_resolve_format_and_use_flags(struct driver *drv, uint32_t format, uint64_t use_flags,
				      uint32_t *out_format, uint64_t *out_use_flags)
{
	assert(drv->backend->resolve_format_and_use_flags);

	drv->backend->resolve_format_and_use_flags(drv, format, use_flags, out_format,
						   out_use_flags);
}

uint32_t drv_num_buffers_per_bo(struct bo *bo)
{
	uint32_t count = 0;
	size_t plane, p;

	if (bo->is_test_buffer)
		return 0;

	for (plane = 0; plane < bo->meta.num_planes; plane++) {
		for (p = 0; p < plane; p++)
			if (bo->handles[p].u32 == bo->handles[plane].u32)
				break;
		if (p == plane)
			count++;
	}

	return count;
}

void drv_log_prefix(enum drv_log_level level, const char *prefix, const char *file, int line,
		    const char *format, ...)
{
	char buf[50];
	snprintf(buf, sizeof(buf), "[%s:%s(%d)]", prefix, basename(file), line);

	va_list args;
	va_start(args, format);
#ifdef __ANDROID__
	int prio = ANDROID_LOG_ERROR;
	switch (level) {
	case DRV_LOGV:
		prio = ANDROID_LOG_VERBOSE;
		break;
	case DRV_LOGD:
		prio = ANDROID_LOG_DEBUG;
		break;
	case DRV_LOGI:
		prio = ANDROID_LOG_INFO;
		break;
	case DRV_LOGE:
	default:
		break;
	};
	__android_log_vprint(prio, buf, format, args);
#else
	if (level == DRV_LOGE) {
		fprintf(stderr, "%s ", buf);
		vfprintf(stderr, format, args);
	} else {
		fprintf(stdout, "%s ", buf);
		vfprintf(stdout, format, args);
	}
#endif
	va_end(args);
}

int drv_resource_info(struct bo *bo, uint32_t strides[DRV_MAX_PLANES],
		      uint32_t offsets[DRV_MAX_PLANES], uint64_t *format_modifier)
{
	for (uint32_t plane = 0; plane < bo->meta.num_planes; plane++) {
		strides[plane] = bo->meta.strides[plane];
		offsets[plane] = bo->meta.offsets[plane];
	}
	*format_modifier = bo->meta.format_modifier;

	if (bo->drv->backend->resource_info)
		return bo->drv->backend->resource_info(bo, strides, offsets, format_modifier);

	return 0;
}

uint32_t drv_get_max_texture_2d_size(struct driver *drv)
{
	if (drv->backend->get_max_texture_2d_size)
		return drv->backend->get_max_texture_2d_size(drv);

	return UINT32_MAX;
}
