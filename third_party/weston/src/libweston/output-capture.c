/*
 * Copyright 2022 Collabora, Ltd.
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

#include <libweston/libweston.h>
#include <libweston/weston-log.h>
#include "libweston-internal.h"
#include "output-capture.h"
#include "pixel-formats.h"
#include "shared/helpers.h"
#include "shared/weston-drm-fourcc.h"
#include "shared/xalloc.h"
#include "weston-output-capture-server-protocol.h"

/* Lifetimes
 *
 * weston_output_capture_info is created at weston_output enable, and
 * destroyed at weston_output disable. It maintains lists of
 * weston_capture_source and weston_capture_task.
 *
 * Protocol request weston_capture_v1.create creates weston_capture_source
 * whose lifetime is equal to the weston_capture_source_v1 protocol object
 * (wl_resource) lifetime.
 *
 * weston_capture_source is associated with a weston_output. When the
 * weston_output is disabled, weston_capture_source is removed from the list
 * in weston_output_capture_info and any pending task is retired as failed.
 * Furhter capture attempts on the source will be immediately failed.
 *
 * Protocol request weston_capture_source_v1.capture creates
 * weston_capture_task, if the weston_capture_source still has its output and
 * no pending task. weston_capture_task becomes the pending task for the
 * weston_capture_source, and is added to the list in
 * weston_output_capture_info. Retiring weston_capture_task destroys it.
 *
 * Each weston_capture_task is associated with a wl_buffer (weston_buffer).
 * If the buffer is destroyed, the task is retired as failed.
 *
 * Operation
 *
 * Each weston_capture_source has a "pixel source" property. Pixel source
 * describes what the capture shall actually contain. See
 * weston_capture_v1.create request in the protocol specification.
 * One pixel source can be provided by at most one component at a time.
 *
 * Whenever a renderer or DRM-backend is repainting an output, they will
 * use weston_output_pull_capture_task() at the appropriate stages to see
 * if there are any capture tasks to be serviced for a specific pixel source.
 * The renderer or DRM-backend must then retire the returned tasks by either
 * failing or completing them.
 *
 * When an output repaint completes, no weston_capture_task shall remain in
 * the list. Renderers or backends could stash them in their own lists though.
 *
 * In order to allow clients to allocate correctly sized and formatted buffers
 * to hold captured images, weston_output_capture_info maintains the current
 * size and format for each type of pixel source. Renderers and DRM-backend
 * who provide pixel sources are also responsible for keeping the buffer
 * requirements information up-to-date with
 * weston_output_update_capture_info().
 */

/** Implementation of weston_capture_source_v1 protocol object */
struct weston_capture_source {
	struct wl_resource *resource;

	/* struct weston_output_capture_info::capture_source_list */
	struct wl_list link;

	enum weston_output_capture_source pixel_source;

	/* weston_output_capture_info_destroy() will reset this. */
	struct weston_output *output;

	struct weston_capture_task *pending;
};

/** A pending task to capture an output */
struct weston_capture_task {
	/* We get cleaned up through owner->pending pointing to us. */
	struct weston_capture_source *owner;

	/* struct weston_output_capture_info::pending_capture_list */
	struct wl_list link;

	struct weston_buffer *buffer;
	struct wl_listener buffer_resource_destroy_listener;
};

/** Buffer requirements broadcasting for a pixel source */
struct weston_output_capture_source_info {
	enum weston_output_capture_source pixel_source;

	int width;
	int height;
	uint32_t drm_format;
};

/** Capture records for an output */
struct weston_output_capture_info {
	/* struct weston_capture_task::link */
	struct wl_list pending_capture_list;

	/* struct weston_capture_source::link */
	struct wl_list capture_source_list;

	struct weston_output_capture_source_info source_info[WESTON_OUTPUT_CAPTURE_SOURCE__COUNT];
};

/** Create capture tracking information on weston_output enable */
struct weston_output_capture_info *
weston_output_capture_info_create(void)
{
	struct weston_output_capture_info *ci;
	unsigned i;

	ci = xzalloc(sizeof *ci);

	wl_list_init(&ci->pending_capture_list);
	wl_list_init(&ci->capture_source_list);

	/*
	 * Initialize to no sources available by leaving
	 * width, height and drm_format as zero.
	 */
	for (i = 0; i < ARRAY_LENGTH(ci->source_info); i++)
		ci->source_info[i].pixel_source = i;

	return ci;
}

/** Clean up capture tracking information on weston_output disable */
void
weston_output_capture_info_destroy(struct weston_output_capture_info **cip)
{
	struct weston_output_capture_info *ci = *cip;
	struct weston_capture_source *csrc, *tmp;

	assert(ci);

	/* Unlink sources. They get destroyed by their wl_resource later. */
	wl_list_for_each_safe(csrc, tmp, &ci->capture_source_list, link) {
		csrc->output = NULL;

		wl_list_remove(&csrc->link);
		wl_list_init(&csrc->link);

		if (csrc->pending)
			weston_capture_task_retire_failed(csrc->pending, "output removed");
	}

	assert(wl_list_empty(&ci->pending_capture_list));

	free(ci);
	*cip = NULL;
}

/** Assert that all capture tasks were taken
 *
 * This is called at the end of a weston_output repaint cycle when the renderer
 * and the backend have had their chance to service all pending capture tasks.
 * The remaining tasks would not be serviced by anything, so make sure none
 * linger.
 */
void
weston_output_capture_info_repaint_done(struct weston_output_capture_info *ci)
{
	assert(wl_list_empty(&ci->pending_capture_list));
}

static bool
source_info_is_available(const struct weston_output_capture_source_info *csi)
{
	return csi->width > 0 && csi->height > 0 &&
	       csi->drm_format != DRM_FORMAT_INVALID;
}

static void
capture_info_send_source_info(struct weston_output_capture_info *ci,
			      struct weston_output_capture_source_info *csi)
{
	struct weston_capture_source *csrc;

	wl_list_for_each(csrc, &ci->capture_source_list, link) {
		if (csrc->pixel_source != csi->pixel_source)
			continue;

		weston_capture_source_v1_send_format(csrc->resource,
						     csi->drm_format);
		weston_capture_source_v1_send_size(csrc->resource,
						   csi->width, csi->height);
	}
}

static struct weston_output_capture_source_info *
capture_info_get_csi(struct weston_output_capture_info *ci,
		     enum weston_output_capture_source src)
{
	int srcidx = src;

	assert(ci);
	assert(srcidx >= 0 && srcidx < (int)ARRAY_LENGTH(ci->source_info));

	return &ci->source_info[srcidx];
}

/** Update capture requirements broadcast to clients
 *
 * This is called by renderers and DRM-backend to update the buffer
 * requirements information that is delivered to clients wanting to capture
 * the output. This is how clients know what size and format buffer they
 * need to allocate for the given output and pixel source.
 *
 * \param output The output whose capture info to update.
 * \param src The source type on the output.
 * \param width The new buffer width.
 * \param height The new buffer height.
 * \param format The new pixel format.
 *
 * If any one of width, height or format is zero/NULL, the source becomes
 * unavailable to clients. Otherwise the source becomes available.
 *
 * Initially all sources are unavailable.
 */
WL_EXPORT void
weston_output_update_capture_info(struct weston_output *output,
				  enum weston_output_capture_source src,
				  int width, int height,
				  const struct pixel_format_info *format)
{
	struct weston_output_capture_info *ci = output->capture_info;
	struct weston_output_capture_source_info *csi;

	csi = capture_info_get_csi(ci, src);

	if (csi->width == width &&
	    csi->height == height &&
	    csi->drm_format == format->format)
		return;

	csi->width = width;
	csi->height = height;
	csi->drm_format = format->format;

	if (source_info_is_available(csi)) {
		capture_info_send_source_info(ci, csi);
	} else {
		struct weston_capture_task *ct, *tmp;

		/*
		 * This source just became unavailable, so fail all pending
		 * tasks using it.
		 */
		wl_list_for_each_safe(ct, tmp,
				      &ci->pending_capture_list, link) {
			if (ct->owner->pixel_source != csi->pixel_source)
				continue;

			weston_capture_task_retire_failed(ct, "source removed");
		}
	}
}

static bool
buffer_is_compatible(struct weston_buffer *buffer,
		     struct weston_output_capture_source_info *csi)
{
	return buffer->width == csi->width &&
	       buffer->height == csi->height &&
	       buffer->pixel_format->format == csi->drm_format &&
	       buffer->format_modifier == DRM_FORMAT_MOD_LINEAR;
}

static void
weston_capture_task_destroy(struct weston_capture_task *ct)
{
	if (ct->owner->pixel_source != WESTON_OUTPUT_CAPTURE_SOURCE_WRITEBACK &&
	    ct->owner->output)
		weston_output_disable_planes_decr(ct->owner->output);

	assert(ct->owner->pending == ct);
	ct->owner->pending = NULL;
	wl_list_remove(&ct->link);
	wl_list_remove(&ct->buffer_resource_destroy_listener.link);
	free(ct);
}

static void
weston_capture_task_buffer_destroy_handler(struct wl_listener *l, void *data)
{
	struct weston_capture_task *ct =
		wl_container_of(l, ct, buffer_resource_destroy_listener);

	/*
	 * Client destroyed the wl_buffer object. By protocol spec, this is
	 * undefined behaviour. Do the most sensible thing.
	 */
	weston_capture_task_retire_failed(ct, "wl_buffer destroyed");
}

static struct weston_capture_task *
weston_capture_task_create(struct weston_capture_source *csrc,
			   struct weston_buffer *buffer)
{
	struct weston_capture_task *ct;

	ct = xzalloc(sizeof *ct);

	ct->owner = csrc;
	/* Owner will explicitly destroy us if the owner gets destroyed. */

	ct->buffer = buffer;
	ct->buffer_resource_destroy_listener.notify = weston_capture_task_buffer_destroy_handler;
	wl_resource_add_destroy_listener(buffer->resource,
					 &ct->buffer_resource_destroy_listener);

	wl_list_insert(&csrc->output->capture_info->pending_capture_list,
		       &ct->link);

	if (ct->owner->pixel_source != WESTON_OUTPUT_CAPTURE_SOURCE_WRITEBACK)
		weston_output_disable_planes_incr(ct->owner->output);

	return ct;
}

static bool
capture_is_authorized(struct weston_capture_source *csrc)
{
	struct weston_compositor *compositor = csrc->output->compositor;
	const struct weston_output_capture_client who = {
		.client = wl_resource_get_client(csrc->resource),
		.output = csrc->output,
	};
	struct weston_output_capture_attempt att = {
		.who = &who,
		.authorized = false,
		.denied = false,
	};

	wl_signal_emit(&compositor->output_capture.ask_auth, &att);

	return att.authorized && !att.denied;
}

/** Fetch the next capture task
 *
 * This is used by renderers and DRM-backend to get the next capture task
 * they want to service. Only tasks for the given pixel source will be
 * returned.
 *
 * Width, height and drm_format are for ensuring that
 * weston_output_update_capture_info() was up-to-date before this.
 *
 * \return A capture task, or NULL if no more tasks.
 */
WL_EXPORT struct weston_capture_task *
weston_output_pull_capture_task(struct weston_output *output,
				enum weston_output_capture_source src,
				int width, int height,
				const struct pixel_format_info *format)
{
	struct weston_output_capture_info *ci = output->capture_info;
	struct weston_output_capture_source_info *csi;
	struct weston_capture_task *ct, *tmp;

	/*
	 * Make sure the capture provider (renderers, DRM-backend) called
	 * weston_output_update_capture_info() if something changed, so that
	 * the 'retry' event keeps its promise of size/format events been
	 * already sent.
	 */
	csi = capture_info_get_csi(ci, src);
	assert(csi->width == width);
	assert(csi->height == height);
	assert(csi->drm_format == format->format);

	wl_list_for_each_safe(ct, tmp, &ci->pending_capture_list, link) {
		assert(ct->owner->output == output);

		if (ct->owner->pixel_source != src)
			continue;

		if (!capture_is_authorized(ct->owner)) {
			weston_capture_task_retire_failed(ct, "unauthorized");
			continue;
		}

		/*
		 * Tell the client to retry, if requirements changed after
		 * the task was filed.
		 */
		if (!buffer_is_compatible(ct->buffer, csi)) {
			weston_capture_source_v1_send_retry(ct->owner->resource);
			weston_capture_task_destroy(ct);
			continue;
		}

		/* pass ct ownership to the caller */
		wl_list_remove(&ct->link);
		wl_list_init(&ct->link);

		return ct;
	}

	return NULL;
}

/** Check if any renderer-based capture tasks are waiting on the output */
WL_EXPORT bool
weston_output_has_renderer_capture_tasks(struct weston_output *output)
{
	struct weston_output_capture_info *ci = output->capture_info;
	struct weston_capture_task *ct;

	wl_list_for_each(ct, &ci->pending_capture_list, link)
		if (ct->owner->pixel_source != WESTON_OUTPUT_CAPTURE_SOURCE_WRITEBACK)
			return true;
	return false;
}

/** Get the destination buffer */
WL_EXPORT struct weston_buffer *
weston_capture_task_get_buffer(struct weston_capture_task *ct)
{
	return ct->buffer;
}

/** Signal completion of the capture task
 *
 * Sends 'complete' protocol event to the client, and destroys the task.
 */
WL_EXPORT void
weston_capture_task_retire_complete(struct weston_capture_task *ct)
{
	weston_capture_source_v1_send_complete(ct->owner->resource);
	weston_capture_task_destroy(ct);
}

/** Signal failure of the capture task
 *
 * Sends 'failed' protocol event to the client, and destroys the task.
 */
WL_EXPORT void
weston_capture_task_retire_failed(struct weston_capture_task *ct,
				  const char *err_msg)
{
	weston_capture_source_v1_send_failed(ct->owner->resource, err_msg);
	weston_capture_task_destroy(ct);
}

static void
destroy_capture_source(struct wl_resource *csrc_resource)
{
	struct weston_capture_source *csrc;

	csrc = wl_resource_get_user_data(csrc_resource);
	assert(csrc_resource == csrc->resource);

	if (csrc->pending)
		weston_capture_task_destroy(csrc->pending);

	wl_list_remove(&csrc->link);
	free(csrc);
}

static void
weston_capture_source_v1_destroy(struct wl_client *client,
				 struct wl_resource *csrc_resource)
{
	wl_resource_destroy(csrc_resource);
}

static void
weston_capture_source_v1_capture(struct wl_client *client,
				 struct wl_resource *csrc_resource,
				 struct wl_resource *buffer_resource)
{
	struct weston_output_capture_source_info *csi;
	struct weston_capture_source *csrc;
	struct weston_buffer *buffer;

	csrc = wl_resource_get_user_data(csrc_resource);
	assert(csrc_resource == csrc->resource);

	/* A capture task already exists? */
	if (csrc->pending) {
		wl_resource_post_error(csrc->resource,
				       WESTON_CAPTURE_SOURCE_V1_ERROR_SEQUENCE,
				       "capture attempted before previous capture retired");
		return;
	}

	/* weston_output disabled after creating the source? */
	if (!csrc->output) {
		weston_capture_source_v1_send_failed(csrc->resource, "output removed");
		return;
	}

	/* Is the pixel source not available? */
	csi = capture_info_get_csi(csrc->output->capture_info,
				   csrc->pixel_source);
	if (!source_info_is_available(csi)) {
		weston_capture_source_v1_send_failed(csrc->resource, "source unavailable");
		return;
	}

	buffer = weston_buffer_from_resource(csrc->output->compositor,
					     buffer_resource);
	if (!buffer) {
		wl_client_post_no_memory(client);
		return;
	}

	/* If the buffer not up-to-date with the size and format? */
	if (!buffer_is_compatible(buffer, csi)) {
		weston_capture_source_v1_send_retry(csrc->resource);
		return;
	}

	csrc->pending = weston_capture_task_create(csrc, buffer);
	weston_output_schedule_repaint(csrc->output);
}

static const struct weston_capture_source_v1_interface weston_capture_source_v1_impl = {
	.destroy = weston_capture_source_v1_destroy,
	.capture = weston_capture_source_v1_capture,
};

static int32_t
pixel_source_from_proto(uint32_t v)
{
	switch (v) {
	case WESTON_CAPTURE_V1_SOURCE_WRITEBACK:
		return WESTON_OUTPUT_CAPTURE_SOURCE_WRITEBACK;
	case WESTON_CAPTURE_V1_SOURCE_FRAMEBUFFER:
		return WESTON_OUTPUT_CAPTURE_SOURCE_FRAMEBUFFER;
	case WESTON_CAPTURE_V1_SOURCE_FULL_FRAMEBUFFER:
		return WESTON_OUTPUT_CAPTURE_SOURCE_FULL_FRAMEBUFFER;
	case WESTON_CAPTURE_V1_SOURCE_BLENDING:
		return WESTON_OUTPUT_CAPTURE_SOURCE_BLENDING;
	default:
		return -1;
	}
}

static void
weston_capture_v1_create(struct wl_client *client,
			 struct wl_resource *capture_resource,
			 struct wl_resource *output_resource,
			 uint32_t source,
			 uint32_t capture_source_new_id)
{
	int32_t isrc = pixel_source_from_proto(source);
	struct weston_capture_source *csrc;
	struct weston_head *head;

	if (isrc < 0) {
		wl_resource_post_error(capture_resource,
				       WESTON_CAPTURE_V1_ERROR_INVALID_SOURCE,
				       "%u is not a valid source", source);
		return;
	}

	csrc = zalloc(sizeof *csrc);
	if (!csrc) {
		wl_client_post_no_memory(client);
		return;
	}

	csrc->pixel_source = isrc;
	wl_list_init(&csrc->link);

	csrc->resource = wl_resource_create(client,
					    &weston_capture_source_v1_interface,
					    wl_resource_get_version(capture_resource),
					    capture_source_new_id);
	if (!csrc->resource) {
		free(csrc);
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(csrc->resource,
				       &weston_capture_source_v1_impl,
				       csrc, destroy_capture_source);

	head = weston_head_from_resource(output_resource);
	if (head) {
		struct weston_output *output = head->output;
		struct weston_output_capture_info *ci = output->capture_info;
		struct weston_output_capture_source_info *csi;

		csi = capture_info_get_csi(ci, csrc->pixel_source);
		wl_list_insert(&ci->capture_source_list, &csrc->link);

		csrc->output = output;

		if (source_info_is_available(csi))
			capture_info_send_source_info(ci, csi);
	}
	/*
	 * if (!head) then weston_capture_source_v1_capture() will respond with
	 * the failed event.
	 */
}

static void
weston_capture_v1_destroy(struct wl_client *client,
			  struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct weston_capture_v1_interface weston_capture_v1_impl = {
	.destroy = weston_capture_v1_destroy,
	.create = weston_capture_v1_create,
};

static void
bind_weston_capture(struct wl_client *client,
		    void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;

	/* Access control is done at capture request. */
	resource = wl_resource_create(client, &weston_capture_v1_interface,
				      version, id);
	if (!resource) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &weston_capture_v1_impl,
				       NULL, NULL);
}

void
weston_compositor_install_capture_protocol(struct weston_compositor *compositor)
{
	compositor->output_capture.weston_capture_v1 =
		wl_global_create(compositor->wl_display,
				 &weston_capture_v1_interface,
				 1, NULL, bind_weston_capture);
	abort_oom_if_null(compositor->output_capture.weston_capture_v1);
}

/** Add a new authority that may authorize or deny screenshots
 *
 * \param compositor The compositor instance.
 * \param listener The listener to populate, and which will be passed as the
 * listener to the auth callback.
 * \param auth The callback function which shall be called every time any
 * client sends a request to capture an output.
 *
 * The callback function \c auth is called with argument \c att. If you
 * want to authorize the screenshot after inspecting the fields in
 * \c att->who , you must set \c att->authorized to true. If you want to
 * deny the screenshot instead, set \c att->denied to true. Otherwise,
 * do not change anything.
 *
 * Any screenshot is carried out only if after iterating through all
 * authorities \c att->authorized is true and \c att->denied is false.
 * Both default to false, which forbids screenshots without any authorities.
 *
 * You can remove an added authority by \c wl_list_remove(&listener->link) .
 */
WL_EXPORT void
weston_compositor_add_screenshot_authority(struct weston_compositor *compositor,
					   struct wl_listener *listener,
					   void (*auth)(struct wl_listener *l,
					   		struct weston_output_capture_attempt *att))
{
	listener->notify = (wl_notify_func_t)auth;
	wl_signal_add(&compositor->output_capture.ask_auth, listener);
}
