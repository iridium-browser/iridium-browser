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
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#include <libweston/libweston.h>
#include "linux-dmabuf.h"
#include "linux-dmabuf-unstable-v1-server-protocol.h"
#include "libweston-internal.h"

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
		buffer->attributes.modifier[plane_idx] = ((uint64_t)modifier_hi << 32) |
							 modifier_lo;

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
	linux_dmabuf_create_params
};

static void
bind_linux_dmabuf(struct wl_client *client,
		  void *data, uint32_t version, uint32_t id)
{
	struct weston_compositor *compositor = data;
	struct wl_resource *resource;
	int *formats = NULL;
	uint64_t *modifiers = NULL;
	int num_formats, num_modifiers;
	uint64_t modifier_invalid = DRM_FORMAT_MOD_INVALID;
	int i, j;

	resource = wl_resource_create(client, &zwp_linux_dmabuf_v1_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &linux_dmabuf_implementation,
				       compositor, NULL);

	/*
	 * Use EGL_EXT_image_dma_buf_import_modifiers to query and advertise
	 * format/modifier codes.
	 */
	compositor->renderer->query_dmabuf_formats(compositor, &formats,
						   &num_formats);

	for (i = 0; i < num_formats; i++) {
		compositor->renderer->query_dmabuf_modifiers(compositor,
							     formats[i],
							     &modifiers,
							     &num_modifiers);

		/* send DRM_FORMAT_MOD_INVALID token when no modifiers are supported
		 * for this format */
		if (num_modifiers == 0) {
			num_modifiers = 1;
			modifiers = &modifier_invalid;
		}
		for (j = 0; j < num_modifiers; j++) {
			if (version >= ZWP_LINUX_DMABUF_V1_MODIFIER_SINCE_VERSION) {
				uint32_t modifier_lo = modifiers[j] & 0xFFFFFFFF;
				uint32_t modifier_hi = modifiers[j] >> 32;
				zwp_linux_dmabuf_v1_send_modifier(resource,
								  formats[i],
								  modifier_hi,
								  modifier_lo);
			} else if (modifiers[j] == DRM_FORMAT_MOD_LINEAR ||
				   modifiers == &modifier_invalid) {
				zwp_linux_dmabuf_v1_send_format(resource,
								formats[i]);
			}
		}
		if (modifiers != &modifier_invalid)
			free(modifiers);
	}
	free(formats);
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
	if (!wl_global_create(compositor->wl_display,
			      &zwp_linux_dmabuf_v1_interface, 3,
			      compositor, bind_linux_dmabuf))
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
