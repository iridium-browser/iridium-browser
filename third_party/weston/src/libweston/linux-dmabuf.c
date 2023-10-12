/*
 * Copyright © 2014, 2015 Collabora, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <libweston/libweston.h>
#include "linux-dmabuf.h"
#include "linux-dmabuf-unstable-v1-server-protocol.h"
#include "shared/os-compatibility.h"
#include "shared/helpers.h"
#include "libweston-internal.h"
#include "shared/weston-drm-fourcc.h"

static void
linux_dmabuf_buffer_destroy(struct linux_dmabuf_buffer *buffer)
{
	int i;

	for (i = 0; i < buffer->attributes.n_planes; i++) {
		close(buffer->attributes.fd[i]);
		buffer->attributes.fd[i] = -1;
	}

	buffer->attributes.n_planes = 0;
	free(buffer);
}

static void
destroy_params(struct wl_resource *params_resource)
{
	struct linux_dmabuf_buffer *buffer;

	buffer = wl_resource_get_user_data(params_resource);

	if (!buffer)
		return;

	linux_dmabuf_buffer_destroy(buffer);
}

static void
params_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
params_add(struct wl_client *client,
	   struct wl_resource *params_resource,
	   int32_t name_fd,
	   uint32_t plane_idx,
	   uint32_t offset,
	   uint32_t stride,
	   uint32_t modifier_hi,
	   uint32_t modifier_lo)
{
	struct linux_dmabuf_buffer *buffer;

	buffer = wl_resource_get_user_data(params_resource);
	if (!buffer) {
		wl_resource_post_error(params_resource,
			ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_ALREADY_USED,
			"params was already used to create a wl_buffer");
		close(name_fd);
		return;
	}

	assert(buffer->params_resource == params_resource);
	assert(!buffer->buffer_resource);

	if (plane_idx >= MAX_DMABUF_PLANES) {
		wl_resource_post_error(params_resource,
			ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_PLANE_IDX,
			"plane index %u is too high", plane_idx);
		close(name_fd);
		return;
	}

	if (buffer->attributes.fd[plane_idx] != -1) {
		wl_resource_post_error(params_resource,
			ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_PLANE_SET,
			"a dmabuf has already been added for plane %u",
			plane_idx);
		close(name_fd);
		return;
	}

	buffer->attributes.fd[plane_idx] = name_fd;
	buffer->attributes.offset[plane_idx] = offset;
	buffer->attributes.stride[plane_idx] = stride;

	if (wl_resource_get_version(params_resource) < ZWP_LINUX_DMABUF_V1_MODIFIER_SINCE_VERSION)
		buffer->attributes.modifier[plane_idx] = DRM_FORMAT_MOD_INVALID;
	else
		buffer->attributes.modifier[plane_idx] = u64_from_u32s(modifier_hi, modifier_lo);

	buffer->attributes.n_planes++;
}

static void
linux_dmabuf_wl_buffer_destroy(struct wl_client *client,
			       struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_buffer_interface linux_dmabuf_buffer_implementation = {
	linux_dmabuf_wl_buffer_destroy
};

static void
destroy_linux_dmabuf_wl_buffer(struct wl_resource *resource)
{
	struct linux_dmabuf_buffer *buffer;

	buffer = wl_resource_get_user_data(resource);
	assert(buffer->buffer_resource == resource);
	assert(!buffer->params_resource);

	if (buffer->user_data_destroy_func)
		buffer->user_data_destroy_func(buffer);

	linux_dmabuf_buffer_destroy(buffer);
}

static void
params_create_common(struct wl_client *client,
		     struct wl_resource *params_resource,
		     uint32_t buffer_id,
		     int32_t width,
		     int32_t height,
		     uint32_t format,
		     uint32_t flags)
{
	struct linux_dmabuf_buffer *buffer;
	int i;

	buffer = wl_resource_get_user_data(params_resource);

	if (!buffer) {
		wl_resource_post_error(params_resource,
			ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_ALREADY_USED,
			"params was already used to create a wl_buffer");
		return;
	}

	assert(buffer->params_resource == params_resource);
	assert(!buffer->buffer_resource);

	/* Switch the linux_dmabuf_buffer object from params resource to
	 * eventually wl_buffer resource.
	 */
	wl_resource_set_user_data(buffer->params_resource, NULL);
	buffer->params_resource = NULL;

	if (!buffer->attributes.n_planes) {
		wl_resource_post_error(params_resource,
			ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INCOMPLETE,
			"no dmabuf has been added to the params");
		goto err_out;
	}

	/* Check for holes in the dmabufs set (e.g. [0, 1, 3]) */
	for (i = 0; i < buffer->attributes.n_planes; i++) {
		if (buffer->attributes.fd[i] == -1) {
			wl_resource_post_error(params_resource,
				ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INCOMPLETE,
				"no dmabuf has been added for plane %i", i);
			goto err_out;
		}
	}

	buffer->attributes.width = width;
	buffer->attributes.height = height;
	buffer->attributes.format = format;
	buffer->attributes.flags = flags;

	if (width < 1 || height < 1) {
		wl_resource_post_error(params_resource,
			ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INVALID_DIMENSIONS,
			"invalid width %d or height %d", width, height);
		goto err_out;
	}

	for (i = 0; i < buffer->attributes.n_planes; i++) {
		off_t size;

		if ((uint64_t) buffer->attributes.offset[i] + buffer->attributes.stride[i] > UINT32_MAX) {
			wl_resource_post_error(params_resource,
				ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_OUT_OF_BOUNDS,
				"size overflow for plane %i", i);
			goto err_out;
		}

		if (i == 0 &&
		   (uint64_t) buffer->attributes.offset[i] +
		   (uint64_t) buffer->attributes.stride[i] * height > UINT32_MAX) {
			wl_resource_post_error(params_resource,
				ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_OUT_OF_BOUNDS,
				"size overflow for plane %i", i);
			goto err_out;
		}

		/* Don't report an error as it might be caused
		 * by the kernel not supporting seeking on dmabuf */
		size = lseek(buffer->attributes.fd[i], 0, SEEK_END);
		if (size == -1)
			continue;

		if (buffer->attributes.offset[i] >= size) {
			wl_resource_post_error(params_resource,
				ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_OUT_OF_BOUNDS,
				"invalid offset %i for plane %i",
				buffer->attributes.offset[i], i);
			goto err_out;
		}

		if (buffer->attributes.offset[i] + buffer->attributes.stride[i] > size) {
			wl_resource_post_error(params_resource,
				ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_OUT_OF_BOUNDS,
				"invalid stride %i for plane %i",
				buffer->attributes.stride[i], i);
			goto err_out;
		}

		/* Only valid for first plane as other planes might be
		 * sub-sampled according to fourcc format */
		if (i == 0 &&
		    buffer->attributes.offset[i] + buffer->attributes.stride[i] * height > size) {
			wl_resource_post_error(params_resource,
				ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_OUT_OF_BOUNDS,
				"invalid buffer stride or height for plane %i", i);
			goto err_out;
		}
	}

	if (buffer->direct_display) {
		if (!weston_compositor_dmabuf_can_scanout(buffer->compositor,
							  buffer))
			goto err_failed;

		goto avoid_gpu_import;
	}

	if (!weston_compositor_import_dmabuf(buffer->compositor, buffer))
		goto err_failed;

avoid_gpu_import:
	buffer->buffer_resource = wl_resource_create(client,
						     &wl_buffer_interface,
						     1, buffer_id);
	if (!buffer->buffer_resource) {
		wl_resource_post_no_memory(params_resource);
		goto err_buffer;
	}

	wl_resource_set_implementation(buffer->buffer_resource,
				       &linux_dmabuf_buffer_implementation,
				       buffer, destroy_linux_dmabuf_wl_buffer);

	/* send 'created' event when the request is not for an immediate
	 * import, ie buffer_id is zero */
	if (buffer_id == 0)
		zwp_linux_buffer_params_v1_send_created(params_resource,
						buffer->buffer_resource);

	return;

err_buffer:
	if (buffer->user_data_destroy_func)
		buffer->user_data_destroy_func(buffer);

err_failed:
	if (buffer_id == 0)
		zwp_linux_buffer_params_v1_send_failed(params_resource);
	else
		/* since the behavior is left implementation defined by the
		 * protocol in case of create_immed failure due to an unknown cause,
		 * we choose to treat it as a fatal error and immediately kill the
		 * client instead of creating an invalid handle and waiting for it
		 * to be used.
		 */
		wl_resource_post_error(params_resource,
			ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INVALID_WL_BUFFER,
			"importing the supplied dmabufs failed");

err_out:
	linux_dmabuf_buffer_destroy(buffer);
}

static void
params_create(struct wl_client *client,
	      struct wl_resource *params_resource,
	      int32_t width,
	      int32_t height,
	      uint32_t format,
	      uint32_t flags)
{
	params_create_common(client, params_resource, 0, width, height, format,
			     flags);
}

static void
params_create_immed(struct wl_client *client,
		    struct wl_resource *params_resource,
		    uint32_t buffer_id,
		    int32_t width,
		    int32_t height,
		    uint32_t format,
		    uint32_t flags)
{
	params_create_common(client, params_resource, buffer_id, width, height,
			     format, flags);
}

static const struct zwp_linux_buffer_params_v1_interface
zwp_linux_buffer_params_implementation = {
	params_destroy,
	params_add,
	params_create,
	params_create_immed
};

static void
linux_dmabuf_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
linux_dmabuf_create_params(struct wl_client *client,
			   struct wl_resource *linux_dmabuf_resource,
			   uint32_t params_id)
{
	struct weston_compositor *compositor;
	struct linux_dmabuf_buffer *buffer;
	uint32_t version;
	int i;

	version = wl_resource_get_version(linux_dmabuf_resource);
	compositor = wl_resource_get_user_data(linux_dmabuf_resource);

	buffer = zalloc(sizeof *buffer);
	if (!buffer)
		goto err_out;

	for (i = 0; i < MAX_DMABUF_PLANES; i++)
		buffer->attributes.fd[i] = -1;

	buffer->compositor = compositor;
	buffer->params_resource =
		wl_resource_create(client,
				   &zwp_linux_buffer_params_v1_interface,
				   version, params_id);
	buffer->direct_display = false;
	if (!buffer->params_resource)
		goto err_dealloc;

	wl_resource_set_implementation(buffer->params_resource,
				       &zwp_linux_buffer_params_implementation,
				       buffer, destroy_params);

	return;

err_dealloc:
	free(buffer);

err_out:
	wl_resource_post_no_memory(linux_dmabuf_resource);
}

/** Creates dma-buf feedback tranche
 *
 * The tranche is added to dma-buf feedback's tranche list
 *
 * @param dmabuf_feedback The dma-buf feedback object to which the tranche is added
 * @param format_table The dma-buf feedback formats table
 * @param target_device The target device of the new tranche
 * @param flags The flags of the new tranche
 * @param preference The preference of the new tranche
 * @return The tranche created, or NULL on failure
 */
WL_EXPORT struct weston_dmabuf_feedback_tranche *
weston_dmabuf_feedback_tranche_create(struct weston_dmabuf_feedback *dmabuf_feedback,
				      struct weston_dmabuf_feedback_format_table *format_table,
				      dev_t target_device, uint32_t flags,
				      enum weston_dmabuf_feedback_tranche_preference preference)
{
	struct weston_dmabuf_feedback_tranche *tranche, *ptr;
	struct wl_list *pos;

	tranche = zalloc(sizeof(*tranche));
	if (!tranche) {
		weston_log("%s: out of memory\n", __func__);
		return NULL;
	}

	tranche->active = true;
	tranche->target_device = target_device;
	tranche->flags = flags;
	tranche->preference = preference;

	/* Get the formats indices array */
	if (flags == 0) {
		if (wl_array_copy(&tranche->formats_indices,
				  &format_table->renderer_formats_indices) < 0) {
			weston_log("%s: out of memory\n", __func__);
			goto err;
		}
	} else if (flags & ZWP_LINUX_DMABUF_FEEDBACK_V1_TRANCHE_FLAGS_SCANOUT) {
		if (wl_array_copy(&tranche->formats_indices,
				  &format_table->scanout_formats_indices) < 0) {
			weston_log("%s: out of memory\n", __func__);
			goto err;
		}
	} else {
		weston_log("error: for now we just have renderer and scanout "
			   "tranches, can't create other type of tranche\n");
		goto err;
	}

	/* The list of tranches is ordered by preference.
	 * Highest preference comes first. */
	pos = &dmabuf_feedback->tranche_list;
	wl_list_for_each(ptr, &dmabuf_feedback->tranche_list, link) {
		pos = &ptr->link;
		if (ptr->preference <= tranche->preference)
			break;
	}
	wl_list_insert(pos->prev, &tranche->link);

	return tranche;

err:
	free(tranche);
	return NULL;
}

static void
weston_dmabuf_feedback_tranche_destroy(struct weston_dmabuf_feedback_tranche *tranche)
{
	wl_array_release(&tranche->formats_indices);
	wl_list_remove(&tranche->link);
	free(tranche);
}

static int
format_table_add_renderer_formats(struct weston_dmabuf_feedback_format_table *format_table,
				  const struct weston_drm_format_array *renderer_formats)
{
	struct weston_drm_format *fmt;
	unsigned int num_modifiers;
	const uint64_t *modifiers;
	uint16_t index, *index_ptr;
	unsigned int size;
	unsigned int i;

	size = sizeof(index) *
	       weston_drm_format_array_count_pairs(renderer_formats);

	if (!wl_array_add(&format_table->renderer_formats_indices, size)) {
		weston_log("%s: out of memory\n", __func__);
		return -1;
	}

	index = 0;
	wl_array_for_each(fmt, &renderer_formats->arr) {
		modifiers = weston_drm_format_get_modifiers(fmt, &num_modifiers);
		for (i = 0; i < num_modifiers; i++) {
			format_table->data[index].format = fmt->format;
			format_table->data[index].modifier = modifiers[i];
			index++;
		}
	}

	index = 0;
	wl_array_for_each(index_ptr, &format_table->renderer_formats_indices)
		*index_ptr = index++;

	return 0;
}

/** Creates dma-buf feedback format table
 *
 * @param renderer_formats The formats that compose the table
 * @return The dma-buf feedback format table, or NULL on failure
 */
WL_EXPORT struct weston_dmabuf_feedback_format_table *
weston_dmabuf_feedback_format_table_create(const struct weston_drm_format_array *renderer_formats)
{
	struct weston_dmabuf_feedback_format_table *format_table;
	int ret;

	format_table = zalloc(sizeof(*format_table));
	if (!format_table) {
		weston_log("%s: out of memory\n", __func__);
		return NULL;
	}
	wl_array_init(&format_table->renderer_formats_indices);
	wl_array_init(&format_table->scanout_formats_indices);

	/* Creates formats file table and mmap it */
	format_table->size = weston_drm_format_array_count_pairs(renderer_formats) *
			     sizeof(*format_table->data);
	format_table->fd = os_create_anonymous_file(format_table->size);
	if (format_table->fd < 0) {
		weston_log("error: failed to create format table file: %s\n",
			   strerror(errno));
		goto err_fd;
	}
	format_table->data = mmap(NULL, format_table->size, PROT_READ | PROT_WRITE,
				  MAP_SHARED, format_table->fd, 0);
	if (format_table->data == MAP_FAILED) {
		weston_log("error: mmap for format table failed: %s\n",
			   strerror(errno));
		goto err_mmap;
	}

	/* Add renderer formats to file table */
	ret = format_table_add_renderer_formats(format_table, renderer_formats);
	if (ret < 0)
		goto err_formats;

	return format_table;

err_formats:
	munmap(format_table->data, format_table->size);
err_mmap:
	close(format_table->fd);
err_fd:
	wl_array_release(&format_table->renderer_formats_indices);
	free(format_table);
	return NULL;
}

/** Destroys dma-buf feedback formats table
 *
 * @param format_table The dma-buf feedback format table to destroy
 */
WL_EXPORT void
weston_dmabuf_feedback_format_table_destroy(struct weston_dmabuf_feedback_format_table *format_table)
{
	wl_array_release(&format_table->renderer_formats_indices);
	wl_array_release(&format_table->scanout_formats_indices);

	munmap(format_table->data, format_table->size);
	close(format_table->fd);

	free(format_table);
}

static int
format_table_get_format_index(struct weston_dmabuf_feedback_format_table *format_table,
			      uint32_t format, uint64_t modifier, uint16_t *index_out)
{
	uint16_t index;
	unsigned int num_elements = format_table->size / sizeof(index);

	for (index = 0; index < num_elements; index++) {
		if (format_table->data[index].format == format &&
		    format_table->data[index].modifier == modifier) {
			*index_out = index;
			return 0;
		}
	}

	return -1;
}

/** Set scanout formats indices in the dma-buf feedback format table
 *
 * The table consists of the formats supported by the renderer. A dma-buf
 * feedback scanout tranche consists of the union of the KMS plane's formats
 * intersected with the renderer formats. With this function we compute the
 * indices of these plane's formats in the table and save them in the
 * table->scanout_formats_indices, allowing us to create scanout tranches.
 *
 * @param format_table The dma-buf feedback format table
 * @param scanout_formats The scanout formats
 * @return 0 on success, -1 on failure
 */
WL_EXPORT int
weston_dmabuf_feedback_format_table_set_scanout_indices(struct weston_dmabuf_feedback_format_table *format_table,
							const struct weston_drm_format_array *scanout_formats)
{
	struct weston_drm_format *fmt;
	unsigned int num_modifiers;
	const uint64_t *modifiers;
	uint16_t index, *index_ptr;
	unsigned int i;
	int ret;

	wl_array_for_each(fmt, &scanout_formats->arr) {
		modifiers = weston_drm_format_get_modifiers(fmt, &num_modifiers);
		for (i = 0; i < num_modifiers; i++) {
			index_ptr =
				wl_array_add(&format_table->scanout_formats_indices,
					     sizeof(index));
			if (!index_ptr)
				goto err;

			ret = format_table_get_format_index(format_table, fmt->format,
							    modifiers[i], &index);
			if (ret < 0)
				goto err;

			*index_ptr = index;
		}
	}

	return 0;

err:
	wl_array_release(&format_table->scanout_formats_indices);
	wl_array_init(&format_table->scanout_formats_indices);
	return -1;
}

/** Creates dma-buf feedback object
 *
 * @param main_device The main device of the dma-buf feedback
 * @return The dma-buf feedback object created, or NULL on failure
 */
WL_EXPORT struct weston_dmabuf_feedback *
weston_dmabuf_feedback_create(dev_t main_device)
{
	struct weston_dmabuf_feedback *dmabuf_feedback;

	dmabuf_feedback = zalloc(sizeof(*dmabuf_feedback));
	if (!dmabuf_feedback) {
		weston_log("%s: out of memory\n", __func__);
		return NULL;
	}

	dmabuf_feedback->main_device = main_device;
	wl_list_init(&dmabuf_feedback->tranche_list);
	wl_list_init(&dmabuf_feedback->resource_list);

	return dmabuf_feedback;
}

/** Destroy dma-buf feedback object
 *
 * @param dmabuf_feedback The dma-buf feedback object to destroy
 */
WL_EXPORT void
weston_dmabuf_feedback_destroy(struct weston_dmabuf_feedback *dmabuf_feedback)
{
	struct weston_dmabuf_feedback_tranche *tranche, *tranche_tmp;
	struct wl_resource *res, *res_tmp;

	wl_list_for_each_safe(tranche, tranche_tmp, &dmabuf_feedback->tranche_list, link)
		weston_dmabuf_feedback_tranche_destroy(tranche);

	wl_resource_for_each_safe(res, res_tmp, &dmabuf_feedback->resource_list) {
		wl_list_remove(wl_resource_get_link(res));
		wl_list_init(wl_resource_get_link(res));
		wl_resource_set_user_data(res, NULL);
	}

	free(dmabuf_feedback);
}

/** Find tranche in a dma-buf feedback object
 *
 * @param dmabuf_feedback The dma-buf feedback object where to look for
 * @param target_device The target device of the tranche
 * @param flags The flags of the tranche
 * @param preference The preference of the tranche
 * @return The tranche, or NULL if it was not found
 */
WL_EXPORT struct weston_dmabuf_feedback_tranche *
weston_dmabuf_feedback_find_tranche(struct weston_dmabuf_feedback *dmabuf_feedback,
				    dev_t target_device, uint32_t flags,
				    enum weston_dmabuf_feedback_tranche_preference preference)
{
	struct weston_dmabuf_feedback_tranche *tranche;

	wl_list_for_each(tranche, &dmabuf_feedback->tranche_list, link)
		if (tranche->target_device == target_device &&
		    tranche->flags == flags && tranche->preference == preference)
			return tranche;

	return NULL;
}

static void
weston_dmabuf_feedback_send(struct weston_dmabuf_feedback *dmabuf_feedback,
			    struct weston_dmabuf_feedback_format_table *format_table,
			    struct wl_resource *res, bool advertise_format_table)
{
	struct weston_dmabuf_feedback_tranche *tranche;
	struct wl_array device;
	dev_t *dev;

	/* main_device and target_device events need a dev_t as parameter,
	 * but we can't use this directly to communicate with the Wayland
	 * client. The solution is to use a wl_array, which is supported by
	 * Wayland, and add the dev_t as an element of the array. */
	wl_array_init(&device);
	dev = wl_array_add(&device, sizeof(*dev));
	if (!dev) {
		wl_resource_post_no_memory(res);
		return;
	}

	/* format_table event - In Weston, we never modify the dma-buf feedback
	 * format table. So we have this flag in order to advertise the format
	 * table only if the client has just subscribed to receive the events
	 * for this feedback object. When we need to re-send the feedback events
	 * for this client, the table event won't be sent. */
	if (advertise_format_table)
		zwp_linux_dmabuf_feedback_v1_send_format_table(res, format_table->fd,
							       format_table->size);

	/* main_device event */
	*dev = dmabuf_feedback->main_device;
	zwp_linux_dmabuf_feedback_v1_send_main_device(res, &device);

	/* send events for each tranche */
	wl_list_for_each(tranche, &dmabuf_feedback->tranche_list, link) {
		if (!tranche->active)
			continue;

		/* tranche_target_device event */
		*dev = tranche->target_device;
		zwp_linux_dmabuf_feedback_v1_send_tranche_target_device(res, &device);

		/* tranche_flags event */
		zwp_linux_dmabuf_feedback_v1_send_tranche_flags(res, tranche->flags);

		/* tranche_formats event */
		zwp_linux_dmabuf_feedback_v1_send_tranche_formats(res, &tranche->formats_indices);

		/* tranche_done_event */
		zwp_linux_dmabuf_feedback_v1_send_tranche_done(res);
	}

	/* compositor_done_event */
	zwp_linux_dmabuf_feedback_v1_send_done(res);

	wl_array_release(&device);
}

/** Sends the feedback events for a dma-buf feedback object
 *
 * Given a dma-buf feedback object, this will send events to clients that are
 * subscribed to it. This is useful for the per-surface dma-buf feedback, which
 * is dynamic and can change throughout compositor's life. These changes results
 * in the need to resend the feedback events to clients.
 *
 * @param dmabuf_feedback The weston_dmabuf_feedback object
 * @param format_table The dma-buf feedback formats table
 */
WL_EXPORT void
weston_dmabuf_feedback_send_all(struct weston_dmabuf_feedback *dmabuf_feedback,
				struct weston_dmabuf_feedback_format_table *format_table)
{
	struct wl_resource *res;

	assert(!wl_list_empty(&dmabuf_feedback->resource_list));
	wl_resource_for_each(res, &dmabuf_feedback->resource_list)
		weston_dmabuf_feedback_send(dmabuf_feedback,
					    format_table, res, false);
}

static void
dmabuf_feedback_resource_destroy(struct wl_resource *resource)
{
	struct weston_surface *surface =
		wl_resource_get_user_data(resource);

	wl_list_remove(wl_resource_get_link(resource));

	if (surface &&
	    wl_list_empty(&surface->dmabuf_feedback->resource_list)) {
		weston_dmabuf_feedback_destroy(surface->dmabuf_feedback);
		surface->dmabuf_feedback = NULL;
	}
}

static void
dmabuf_feedback_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct zwp_linux_dmabuf_feedback_v1_interface
zwp_linux_dmabuf_feedback_implementation = {
	dmabuf_feedback_destroy
};

static struct wl_resource *
dmabuf_feedback_resource_create(struct wl_resource *dmabuf_resource,
				struct wl_client *client, uint32_t dmabuf_feedback_id,
				struct weston_surface *surface)
{
	struct wl_resource *dmabuf_feedback_res;
	uint32_t version;

	version = wl_resource_get_version(dmabuf_resource);

	dmabuf_feedback_res =
		wl_resource_create(client, &zwp_linux_dmabuf_feedback_v1_interface,
				   version, dmabuf_feedback_id);
	if (!dmabuf_feedback_res)
		return NULL;

	wl_list_init(wl_resource_get_link(dmabuf_feedback_res));
	wl_resource_set_implementation(dmabuf_feedback_res,
				       &zwp_linux_dmabuf_feedback_implementation,
				       surface, dmabuf_feedback_resource_destroy);

	return dmabuf_feedback_res;
}

static void
linux_dmabuf_get_default_feedback(struct wl_client *client,
				  struct wl_resource *dmabuf_resource,
				  uint32_t dmabuf_feedback_id)
{
	struct weston_compositor *compositor =
		wl_resource_get_user_data(dmabuf_resource);
	struct wl_resource *dmabuf_feedback_resource;

	dmabuf_feedback_resource =
		dmabuf_feedback_resource_create(dmabuf_resource,
						client, dmabuf_feedback_id,
						NULL);
	if (!dmabuf_feedback_resource) {
		wl_resource_post_no_memory(dmabuf_resource);
		return;
	}

	weston_dmabuf_feedback_send(compositor->default_dmabuf_feedback,
				    compositor->dmabuf_feedback_format_table,
				    dmabuf_feedback_resource, true);
}

static int
create_surface_dmabuf_feedback(struct weston_compositor *ec,
			       struct weston_surface *surface)
{
	struct weston_dmabuf_feedback_tranche *tranche;
	dev_t main_device = ec->default_dmabuf_feedback->main_device;
	uint32_t flags = 0;

	surface->dmabuf_feedback = weston_dmabuf_feedback_create(main_device);
	if (!surface->dmabuf_feedback)
		return -1;

	tranche = weston_dmabuf_feedback_tranche_create(surface->dmabuf_feedback,
							ec->dmabuf_feedback_format_table,
							main_device, flags,
							RENDERER_PREF);
	if (!tranche) {
		weston_dmabuf_feedback_destroy(surface->dmabuf_feedback);
		surface->dmabuf_feedback = NULL;
		return -1;
	}

	return 0;
}

static void
linux_dmabuf_get_per_surface_feedback(struct wl_client *client,
				      struct wl_resource *dmabuf_resource,
				      uint32_t dmabuf_feedback_id,
				      struct wl_resource *surface_resource)
{
	struct weston_compositor *compositor =
		wl_resource_get_user_data(dmabuf_resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct wl_resource *dmabuf_feedback_resource;
	int ret;

	dmabuf_feedback_resource =
		dmabuf_feedback_resource_create(dmabuf_resource,
						client, dmabuf_feedback_id,
						surface);
	if (!dmabuf_feedback_resource)
		goto err;

	if (!surface->dmabuf_feedback) {
		ret = create_surface_dmabuf_feedback(compositor, surface);
		if (ret < 0)
			goto err_feedback;
	}

	/* Surface dma-buf feedback is dynamic and may need to be resent to
	 * clients when they change. So we need to keep the resources list */
	wl_list_insert(&surface->dmabuf_feedback->resource_list,
		       wl_resource_get_link(dmabuf_feedback_resource));

	weston_dmabuf_feedback_send(surface->dmabuf_feedback,
				    surface->compositor->dmabuf_feedback_format_table,
				    dmabuf_feedback_resource, true);
	return;

err_feedback:
	wl_resource_set_user_data(dmabuf_feedback_resource, NULL);
	wl_resource_destroy(dmabuf_feedback_resource);
err:
	wl_resource_post_no_memory(dmabuf_resource);
}

/** Get the linux_dmabuf_buffer from a wl_buffer resource
 *
 * If the given wl_buffer resource was created through the linux_dmabuf
 * protocol interface, returns the linux_dmabuf_buffer object. This can
 * be used as a type check for a wl_buffer.
 *
 * \param resource A wl_buffer resource.
 * \return The linux_dmabuf_buffer if it exists, or NULL otherwise.
 */
WL_EXPORT struct linux_dmabuf_buffer *
linux_dmabuf_buffer_get(struct wl_resource *resource)
{
	struct linux_dmabuf_buffer *buffer;

	if (!resource)
		return NULL;

	if (!wl_resource_instance_of(resource, &wl_buffer_interface,
				     &linux_dmabuf_buffer_implementation))
		return NULL;

	buffer = wl_resource_get_user_data(resource);
	assert(buffer);
	assert(!buffer->params_resource);
	assert(buffer->buffer_resource == resource);

	return buffer;
}

/** Set renderer-private data
 *
 * Set the user data for the linux_dmabuf_buffer. It is invalid to overwrite
 * a non-NULL user data with a new non-NULL pointer. This is meant to
 * protect against renderers fighting over linux_dmabuf_buffer user data
 * ownership.
 *
 * The renderer-private data is usually set from the
 * weston_renderer::import_dmabuf hook.
 *
 * \param buffer The linux_dmabuf_buffer object to set for.
 * \param data The new renderer-private data pointer.
 * \param func Destructor function to be called for the renderer-private
 *             data when the linux_dmabuf_buffer gets destroyed.
 *
 * \sa weston_compositor_import_dmabuf
 */
WL_EXPORT void
linux_dmabuf_buffer_set_user_data(struct linux_dmabuf_buffer *buffer,
				  void *data,
				  dmabuf_user_data_destroy_func func)
{
	assert(data == NULL || buffer->user_data == NULL);

	buffer->user_data = data;
	buffer->user_data_destroy_func = func;
}

/** Get renderer-private data
 *
 * Get the user data from the linux_dmabuf_buffer.
 *
 * \param buffer The linux_dmabuf_buffer to query.
 * \return Renderer-private data pointer.
 *
 * \sa linux_dmabuf_buffer_set_user_data
 */
WL_EXPORT void *
linux_dmabuf_buffer_get_user_data(struct linux_dmabuf_buffer *buffer)
{
	return buffer->user_data;
}

static const struct zwp_linux_dmabuf_v1_interface linux_dmabuf_implementation = {
	linux_dmabuf_destroy,
	linux_dmabuf_create_params,
	linux_dmabuf_get_default_feedback,
	linux_dmabuf_get_per_surface_feedback
};

static void
bind_linux_dmabuf(struct wl_client *client,
		  void *data, uint32_t version, uint32_t id)
{
	struct weston_compositor *compositor = data;
	const struct weston_drm_format_array *supported_formats;
	struct wl_resource *resource;
	struct weston_drm_format *fmt;
	const uint64_t *modifiers;
	unsigned int num_modifiers;
	unsigned int i;

	resource = wl_resource_create(client, &zwp_linux_dmabuf_v1_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &linux_dmabuf_implementation,
				       compositor, NULL);

	/* Advertise formats/modifiers. From version 4 onwards, we should not send
	 * zwp_linux_dmabuf_v1_send_modifier and zwp_linux_dmabuf_v1_send_format
	 * events, instead we must send the dma-buf feedback events. */
	if (version >= 4)
		return;

	/* If we got here, it means that the renderer is able to import dma-buf
	 * buffers, and so it must have get_supported_formats() set. */
	assert(compositor->renderer->get_supported_formats != NULL);
	supported_formats = compositor->renderer->get_supported_formats(compositor);

	wl_array_for_each(fmt, &supported_formats->arr) {
		modifiers = weston_drm_format_get_modifiers(fmt, &num_modifiers);
		for (i = 0; i < num_modifiers; i++) {
			if (version >= ZWP_LINUX_DMABUF_V1_MODIFIER_SINCE_VERSION) {
				uint32_t modifier_lo = modifiers[i] & 0xFFFFFFFF;
				uint32_t modifier_hi = modifiers[i] >> 32;
				zwp_linux_dmabuf_v1_send_modifier(resource,
								  fmt->format,
								  modifier_hi,
								  modifier_lo);
			} else if (modifiers[i] == DRM_FORMAT_MOD_LINEAR ||
				   modifiers[i] == DRM_FORMAT_MOD_INVALID) {
				zwp_linux_dmabuf_v1_send_format(resource,
								fmt->format);
			}
		}
	}
}

/** Advertise linux_dmabuf support
 *
 * Calling this initializes the zwp_linux_dmabuf protocol support, so that
 * the interface will be advertised to clients. Essentially it creates a
 * global. Do not call this function multiple times in the compositor's
 * lifetime. There is no way to deinit explicitly, globals will be reaped
 * when the wl_display gets destroyed.
 *
 * \param compositor The compositor to init for.
 * \return Zero on success, -1 on failure.
 */
WL_EXPORT int
linux_dmabuf_setup(struct weston_compositor *compositor)
{
	int max_version;

	/* If we were able to create the default dma-buf feedback for the
	 * compositor, that means that we are able to advertise dma-buf feedback
	 * events. In such case we support the version 4 of the protocol. */
	max_version = compositor->default_dmabuf_feedback ? 4 : 3;

	if (!wl_global_create(compositor->wl_display,
			      &zwp_linux_dmabuf_v1_interface,
			      max_version, compositor, bind_linux_dmabuf))
		return -1;

	return 0;
}

/** Resolve an internal compositor error by disconnecting the client.
 *
 * This function is used in cases when the dmabuf-based wl_buffer
 * turns out unusable and there is no fallback path. This is used by
 * renderers which are the fallback path in the first place.
 *
 * It is possible the fault is caused by a compositor bug, the underlying
 * graphics stack bug or normal behaviour, or perhaps a client mistake.
 * In any case, the options are to either composite garbage or nothing,
 * or disconnect the client. This is a helper function for the latter.
 *
 * The error is sent as an INVALID_OBJECT error on the client's wl_display.
 *
 * \param buffer The linux_dmabuf_buffer that is unusable.
 * \param msg A custom error message attached to the protocol error.
 */
WL_EXPORT void
linux_dmabuf_buffer_send_server_error(struct linux_dmabuf_buffer *buffer,
				      const char *msg)
{
	struct wl_client *client;
	struct wl_resource *display_resource;
	uint32_t id;

	assert(buffer->buffer_resource);
	id = wl_resource_get_id(buffer->buffer_resource);
	client = wl_resource_get_client(buffer->buffer_resource);
	display_resource = wl_client_get_object(client, 1);

	assert(display_resource);
	wl_resource_post_error(display_resource,
			       WL_DISPLAY_ERROR_INVALID_OBJECT,
			       "linux_dmabuf server error with "
			       "wl_buffer@%u: %s", id, msg);
}
