/*
 * Copyright © 2013 Hardening <rdp.effort@gmail.com>
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <linux/input.h>
#include <unistd.h>

#include "rdp.h"

#include <winpr/input.h>
#include <winpr/ssl.h>

#include "shared/timespec-util.h"
#include "shared/xalloc.h"
#include <libweston/libweston.h>
#include <libweston/backend-rdp.h>
#include <libweston/pixel-formats.h>
#include "pixman-renderer.h"

/* These can be removed when we bump FreeRDP dependency past 3.0.0 in the future */
#ifndef KBD_PERSIAN
#define KBD_PERSIAN 0x50429
#endif
#ifndef KBD_HEBREW_STANDARD
#define KBD_HEBREW_STANDARD 0x2040D
#endif

extern PWtsApiFunctionTable FreeRDP_InitWtsApi(void);

static BOOL
xf_peer_adjust_monitor_layout(freerdp_peer *client);

static struct rdp_output *
rdp_get_first_output(struct rdp_backend *b)
{
	struct weston_output *output;
	struct rdp_output *rdp_output;

	wl_list_for_each(output, &b->compositor->output_list, link) {
		rdp_output = to_rdp_output(output);
		if (rdp_output)
			return rdp_output;
	}

	return NULL;
}

static void
rdp_peer_refresh_rfx(pixman_region32_t *damage, pixman_image_t *image, freerdp_peer *peer)
{
	int width, height, nrects, i;
	pixman_box32_t *region, *rects;
	uint32_t *ptr;
	RFX_RECT *rfxRect;
	rdpUpdate *update = peer->context->update;
	SURFACE_BITS_COMMAND cmd = { 0 };
	RdpPeerContext *context = (RdpPeerContext *)peer->context;

	Stream_Clear(context->encode_stream);
	Stream_SetPosition(context->encode_stream, 0);

	width = (damage->extents.x2 - damage->extents.x1);
	height = (damage->extents.y2 - damage->extents.y1);

	cmd.skipCompression = TRUE;
	cmd.cmdType = CMDTYPE_STREAM_SURFACE_BITS;
	cmd.destLeft = damage->extents.x1;
	cmd.destTop = damage->extents.y1;
	cmd.destRight = damage->extents.x2;
	cmd.destBottom = damage->extents.y2;
	cmd.bmp.bpp = 32;
	cmd.bmp.codecID = peer->context->settings->RemoteFxCodecId;
	cmd.bmp.width = width;
	cmd.bmp.height = height;

	ptr = pixman_image_get_data(image) + damage->extents.x1 +
				damage->extents.y1 * (pixman_image_get_stride(image) / sizeof(uint32_t));

	rects = pixman_region32_rectangles(damage, &nrects);
	context->rfx_rects = realloc(context->rfx_rects, nrects * sizeof *rfxRect);

	for (i = 0; i < nrects; i++) {
		region = &rects[i];
		rfxRect = &context->rfx_rects[i];

		rfxRect->x = (region->x1 - damage->extents.x1);
		rfxRect->y = (region->y1 - damage->extents.y1);
		rfxRect->width = (region->x2 - region->x1);
		rfxRect->height = (region->y2 - region->y1);
	}

	rfx_compose_message(context->rfx_context, context->encode_stream, context->rfx_rects, nrects,
			(BYTE *)ptr, width, height,
			pixman_image_get_stride(image)
	);

	cmd.bmp.bitmapDataLength = Stream_GetPosition(context->encode_stream);
	cmd.bmp.bitmapData = Stream_Buffer(context->encode_stream);

	update->SurfaceBits(update->context, &cmd);
}


static void
rdp_peer_refresh_nsc(pixman_region32_t *damage, pixman_image_t *image, freerdp_peer *peer)
{
	int width, height;
	uint32_t *ptr;
	rdpUpdate *update = peer->context->update;
	SURFACE_BITS_COMMAND cmd = { 0 };
	RdpPeerContext *context = (RdpPeerContext *)peer->context;

	Stream_Clear(context->encode_stream);
	Stream_SetPosition(context->encode_stream, 0);

	width = (damage->extents.x2 - damage->extents.x1);
	height = (damage->extents.y2 - damage->extents.y1);

	cmd.cmdType = CMDTYPE_SET_SURFACE_BITS;
	cmd.skipCompression = TRUE;
	cmd.destLeft = damage->extents.x1;
	cmd.destTop = damage->extents.y1;
	cmd.destRight = damage->extents.x2;
	cmd.destBottom = damage->extents.y2;
	cmd.bmp.bpp = 32;
	cmd.bmp.codecID = peer->context->settings->NSCodecId;
	cmd.bmp.width = width;
	cmd.bmp.height = height;

	ptr = pixman_image_get_data(image) + damage->extents.x1 +
				damage->extents.y1 * (pixman_image_get_stride(image) / sizeof(uint32_t));

	nsc_compose_message(context->nsc_context, context->encode_stream, (BYTE *)ptr,
			width, height,
			pixman_image_get_stride(image));

	cmd.bmp.bitmapDataLength = Stream_GetPosition(context->encode_stream);
	cmd.bmp.bitmapData = Stream_Buffer(context->encode_stream);

	update->SurfaceBits(update->context, &cmd);
}

static void
pixman_image_flipped_subrect(const pixman_box32_t *rect, pixman_image_t *img, BYTE *dest)
{
	int stride = pixman_image_get_stride(img);
	int h;
	int toCopy = (rect->x2 - rect->x1) * 4;
	int height = (rect->y2 - rect->y1);
	const BYTE *src = (const BYTE *)pixman_image_get_data(img);
	src += ((rect->y2-1) * stride) + (rect->x1 * 4);

	for (h = 0; h < height; h++, src -= stride, dest += toCopy)
		   memcpy(dest, src, toCopy);
}

static void
rdp_peer_refresh_raw(pixman_region32_t *region, pixman_image_t *image, freerdp_peer *peer)
{
	rdpUpdate *update = peer->context->update;
	SURFACE_BITS_COMMAND cmd = { 0 };
	SURFACE_FRAME_MARKER marker;
	pixman_box32_t *rect, subrect;
	int nrects, i;
	int heightIncrement, remainingHeight, top;

	rect = pixman_region32_rectangles(region, &nrects);
	if (!nrects)
		return;

	marker.frameId++;
	marker.frameAction = SURFACECMD_FRAMEACTION_BEGIN;
	update->SurfaceFrameMarker(peer->context, &marker);

	cmd.cmdType = CMDTYPE_SET_SURFACE_BITS;
	cmd.bmp.bpp = 32;
	cmd.bmp.codecID = 0;

	for (i = 0; i < nrects; i++, rect++) {
		/*weston_log("rect(%d,%d, %d,%d)\n", rect->x1, rect->y1, rect->x2, rect->y2);*/
		cmd.destLeft = rect->x1;
		cmd.destRight = rect->x2;
		cmd.bmp.width = (rect->x2 - rect->x1);

		heightIncrement = peer->context->settings->MultifragMaxRequestSize / (16 + cmd.bmp.width * 4);
		remainingHeight = rect->y2 - rect->y1;
		top = rect->y1;

		subrect.x1 = rect->x1;
		subrect.x2 = rect->x2;

		while (remainingHeight) {
			   cmd.bmp.height = (remainingHeight > heightIncrement) ? heightIncrement : remainingHeight;
			   cmd.destTop = top;
			   cmd.destBottom = top + cmd.bmp.height;
			   cmd.bmp.bitmapDataLength = cmd.bmp.width * cmd.bmp.height * 4;
			   cmd.bmp.bitmapData = (BYTE *)realloc(cmd.bmp.bitmapData, cmd.bmp.bitmapDataLength);

			   subrect.y1 = top;
			   subrect.y2 = top + cmd.bmp.height;
			   pixman_image_flipped_subrect(&subrect, image, cmd.bmp.bitmapData);

			   /*weston_log("*  sending (%d,%d, %d,%d)\n", subrect.x1, subrect.y1, subrect.x2, subrect.y2); */
			   update->SurfaceBits(peer->context, &cmd);

			   remainingHeight -= cmd.bmp.height;
			   top += cmd.bmp.height;
		}
	}

	free(cmd.bmp.bitmapData);

	marker.frameAction = SURFACECMD_FRAMEACTION_END;
	update->SurfaceFrameMarker(peer->context, &marker);
}

static void
rdp_peer_refresh_region(pixman_region32_t *region, freerdp_peer *peer)
{
	RdpPeerContext *context = (RdpPeerContext *)peer->context;
	struct rdp_output *output = rdp_get_first_output(context->rdpBackend);
	rdpSettings *settings = peer->context->settings;
	const struct weston_renderer *renderer;
	pixman_image_t *image;

	renderer = output->base.compositor->renderer;
	image = renderer->pixman->renderbuffer_get_image(output->renderbuffer);

	if (settings->RemoteFxCodec)
		rdp_peer_refresh_rfx(region, image, peer);
	else if (settings->NSCodec)
		rdp_peer_refresh_nsc(region, image, peer);
	else
		rdp_peer_refresh_raw(region, image, peer);
}

static int
rdp_output_start_repaint_loop(struct weston_output *output)
{
	struct timespec ts;

	weston_compositor_read_presentation_clock(output->compositor, &ts);
	weston_output_finish_frame(output, &ts, WP_PRESENTATION_FEEDBACK_INVALID);

	return 0;
}

static int
rdp_output_repaint(struct weston_output *output_base, pixman_region32_t *damage)
{
	struct rdp_output *output = container_of(output_base, struct rdp_output, base);
	struct weston_compositor *ec = output->base.compositor;
	struct rdp_backend *b = output->backend;
	struct rdp_peers_item *peer;
	struct timespec now, target;
	int refresh_nsec = millihz_to_nsec(output_base->current_mode->refresh);
	int refresh_msec = refresh_nsec / 1000000;
	int next_frame_delta;

	/* Calculate the time we should complete this frame such that frames
	   are spaced out by the specified monitor refresh. Note that our timer
	   mechanism only has msec precision, so we won't exactly hit our
	   target refresh rate.
	 */
	weston_compositor_read_presentation_clock(ec, &now);

	timespec_add_nsec(&target, &output_base->frame_time, refresh_nsec);

	next_frame_delta = (int)timespec_sub_to_msec(&target, &now);
	if (next_frame_delta < 1 || next_frame_delta > refresh_msec) {
		next_frame_delta = refresh_msec;
	}

	assert(output);

	ec->renderer->repaint_output(&output->base, damage,
				     output->renderbuffer);

	if (pixman_region32_not_empty(damage)) {
		pixman_region32_t transformed_damage;
		pixman_region32_init(&transformed_damage);
		weston_region_global_to_output(&transformed_damage,
					       output_base,
					       damage);
		wl_list_for_each(peer, &b->peers, link) {
			if ((peer->flags & RDP_PEER_ACTIVATED) &&
			    (peer->flags & RDP_PEER_OUTPUT_ENABLED)) {
				rdp_peer_refresh_region(&transformed_damage, peer->peer);
			}
		}
		pixman_region32_fini(&transformed_damage);
	}

	pixman_region32_subtract(&ec->primary_plane.damage,
				 &ec->primary_plane.damage, damage);

	wl_event_source_timer_update(output->finish_frame_timer, next_frame_delta);
	return 0;
}

static int
finish_frame_handler(void *data)
{
	struct rdp_output *output = data;
	struct timespec ts;

	weston_compositor_read_presentation_clock(output->base.compositor, &ts);
	weston_output_finish_frame(&output->base, &ts, 0);

	return 1;
}

static struct weston_mode *
rdp_insert_new_mode(struct weston_output *output, int width, int height, int rate)
{
	struct weston_mode *ret;
	ret = xzalloc(sizeof *ret);
	ret->width = width;
	ret->height = height;
	ret->refresh = rate;
	ret->flags = WL_OUTPUT_MODE_PREFERRED;
	wl_list_insert(&output->mode_list, &ret->link);
	return ret;
}

/* It doesn't make sense for RDP to have more than one mode, so
 * we make sure that we have only one.
 */
static struct weston_mode *
ensure_single_mode(struct weston_output *output, struct weston_mode *target)
{
	struct rdp_output *rdpOutput = to_rdp_output(output);
	struct rdp_backend *b = rdpOutput->backend;
	struct weston_mode *iter, *local = NULL, *new_mode;

	wl_list_for_each(iter, &output->mode_list, link) {
		assert(!local);

		if ((iter->width == target->width) &&
		    (iter->height == target->height) &&
		    (iter->refresh == target->refresh)) {
			return iter;
		} else {
			local = iter;
		}
	}
	/* Make sure we create the new one before freeing the old one
	 * because some mode switch code uses pointer comparisons! If
	 * we freed the old mode first, malloc could theoretically give
	 * us back the same pointer.
	 */
	new_mode = rdp_insert_new_mode(output,
				       target->width, target->height,
				       b->rdp_monitor_refresh_rate);
	if (local) {
		wl_list_remove(&local->link);
		free(local);
	}

	return new_mode;
}

static void
rdp_output_set_mode(struct weston_output *base, struct weston_mode *mode)
{
	struct rdp_output *rdpOutput = container_of(base, struct rdp_output, base);
	struct rdp_backend *b = rdpOutput->backend;
	struct weston_mode *cur;
	struct weston_output *output = base;
	struct rdp_peers_item *rdpPeer;
	rdpSettings *settings;
	struct weston_renderbuffer *new_renderbuffer;

	mode->refresh = b->rdp_monitor_refresh_rate;
	cur = ensure_single_mode(base, mode);

	base->current_mode = cur;
	base->native_mode = cur;
	if (base->enabled) {
		const struct pixman_renderer_interface *pixman;
		const struct pixel_format_info *pfmt;
		pixman_image_t *old_image, *new_image;

		weston_renderer_resize_output(output, &(struct weston_size){
			.width = output->current_mode->width,
			.height = output->current_mode->height }, NULL);

		pixman = b->compositor->renderer->pixman;

		old_image =
			pixman->renderbuffer_get_image(rdpOutput->renderbuffer);
		pfmt = pixel_format_get_info_by_pixman(PIXMAN_x8r8g8b8);
		new_renderbuffer =
			pixman->create_image_from_ptr(output, pfmt,
						      mode->width, mode->height,
						      0, mode->width * 4);
		new_image = pixman->renderbuffer_get_image(new_renderbuffer);
		pixman_image_composite32(PIXMAN_OP_SRC, old_image, 0, new_image,
					 0, 0, 0, 0, 0, 0, mode->width,
					 mode->height);
		weston_renderbuffer_unref(rdpOutput->renderbuffer);
		rdpOutput->renderbuffer = new_renderbuffer;
	}

	/* Apparently settings->DesktopWidth is supposed to be primary only.
	 * For now we only work with a single monitor, so we don't need to
	 * check that we're primary here.
	 */
	wl_list_for_each(rdpPeer, &b->peers, link) {
		settings = rdpPeer->peer->context->settings;
		if (settings->DesktopWidth == (uint32_t)mode->width &&
		    settings->DesktopHeight == (uint32_t)mode->height)
			continue;

		if (!settings->DesktopResize) {
			/* too bad this peer does not support desktop resize */
			weston_log("desktop resize is not allowed\n");
			rdpPeer->peer->Close(rdpPeer->peer);
		} else {
			settings->DesktopWidth = mode->width;
			settings->DesktopHeight = mode->height;
			rdpPeer->peer->context->update->DesktopResize(rdpPeer->peer->context);
		}
	}
}

static int
rdp_output_switch_mode(struct weston_output *base, struct weston_mode *mode)
{
	rdp_output_set_mode(base, mode);

	return 0;
}

static void
rdp_head_get_monitor(struct weston_head *base,
		     struct weston_rdp_monitor *monitor)
{
	struct rdp_head *h = to_rdp_head(base);

	monitor->x = h->config.x;
	monitor->y = h->config.y;
	monitor->width = h->config.width;
	monitor->height = h->config.height;
	monitor->desktop_scale = h->config.attributes.desktopScaleFactor;
}

static int
rdp_output_enable(struct weston_output *base)
{
	const struct weston_renderer *renderer = base->compositor->renderer;
	const struct pixman_renderer_interface *pixman = renderer->pixman;
	struct rdp_output *output = to_rdp_output(base);
	struct rdp_backend *b;
	struct wl_event_loop *loop;
	const struct pixman_renderer_output_options options = {
		.fb_size = {
			.width = output->base.current_mode->width,
			.height = output->base.current_mode->height
		},
		.format = pixel_format_get_info_by_pixman(PIXMAN_x8r8g8b8)
	};

	assert(output);

	b = output->backend;

	if (renderer->pixman->output_create(&output->base, &options) < 0) {
		return -1;
	}

	output->renderbuffer =
		pixman->create_image_from_ptr(&output->base, options.format,
					      output->base.current_mode->width,
					      output->base.current_mode->height,
					      NULL,
					      output->base.current_mode->width * 4);
	if (output->renderbuffer == NULL) {
		weston_log("Failed to create surface for frame buffer.\n");
		renderer->pixman->output_destroy(&output->base);
		return -1;
	}

	loop = wl_display_get_event_loop(b->compositor->wl_display);
	output->finish_frame_timer = wl_event_loop_add_timer(loop, finish_frame_handler, output);

	return 0;
}

static int
rdp_output_disable(struct weston_output *base)
{
	struct weston_renderer *renderer = base->compositor->renderer;
	struct rdp_output *output = to_rdp_output(base);

	assert(output);

	if (!output->base.enabled)
		return 0;

	weston_renderbuffer_unref(output->renderbuffer);
	output->renderbuffer = NULL;
	renderer->pixman->output_destroy(&output->base);

	wl_event_source_remove(output->finish_frame_timer);

	return 0;
}

void
rdp_output_destroy(struct weston_output *base)
{
	struct rdp_output *output = to_rdp_output(base);

	assert(output);

	rdp_output_disable(&output->base);
	weston_output_release(&output->base);

	free(output);
}

static struct weston_output *
rdp_output_create(struct weston_backend *backend, const char *name)
{
	struct rdp_backend *b = container_of(backend, struct rdp_backend, base);
	struct weston_compositor *compositor = b->compositor;
	struct rdp_output *output;

	output = xzalloc(sizeof *output);

	weston_output_init(&output->base, compositor, name);

	output->base.destroy = rdp_output_destroy;
	output->base.disable = rdp_output_disable;
	output->base.enable = rdp_output_enable;

	output->base.start_repaint_loop = rdp_output_start_repaint_loop;
	output->base.repaint = rdp_output_repaint;
	output->base.switch_mode = rdp_output_switch_mode;

	output->backend = b;

	weston_compositor_add_pending_output(&output->base, compositor);

	return &output->base;
}

void
rdp_head_create(struct rdp_backend *backend, rdpMonitor *config)
{
	struct rdp_head *head;
	char name[13] = {}; /* "rdp-" + 8 chars for hex uint32_t + NULL. */

	head = xzalloc(sizeof *head);
	head->index = backend->head_index++;
	if (config)
		head->config = *config;
	else {
		/* Before any client conenctions we create a default head
		 * with no configuration. Make it the primary, and make
		 * it avoid the high dpi scaling paths.
		 */
		head->config.is_primary = true;
		head->config.attributes.desktopScaleFactor = 0;
	}

	sprintf(name, "rdp-%x", head->index);

	weston_head_init(&head->base, name);
	weston_head_set_monitor_strings(&head->base, "weston", "rdp", NULL);

	if (config)
		weston_head_set_physical_size(&head->base,
					      config->attributes.physicalWidth,
					      config->attributes.physicalHeight);
	else
		weston_head_set_physical_size(&head->base, 0, 0);

	head->base.backend = &backend->base;

	weston_head_set_connection_status(&head->base, true);
	weston_compositor_add_head(backend->compositor, &head->base);
}

void
rdp_head_destroy(struct weston_head *base)
{
	struct rdp_head *head = to_rdp_head(base);

	assert(head);

	weston_head_release(&head->base);
	free(head);
}

void
rdp_destroy(struct weston_backend *backend)
{
	struct rdp_backend *b = container_of(backend, struct rdp_backend, base);
	struct weston_compositor *ec = b->compositor;
	struct weston_head *base, *next;
	struct rdp_peers_item *rdp_peer, *tmp;
	int i;

	wl_list_for_each_safe(rdp_peer, tmp, &b->peers, link) {
		freerdp_peer* client = rdp_peer->peer;

		client->Disconnect(client);
		freerdp_peer_context_free(client);
		freerdp_peer_free(client);
	}

	for (i = 0; i < MAX_FREERDP_FDS; i++)
		if (b->listener_events[i])
			wl_event_source_remove(b->listener_events[i]);

	if (b->clipboard_debug) {
		weston_log_scope_destroy(b->clipboard_debug);
		b->clipboard_debug = NULL;
	}

	if (b->clipboard_verbose) {
		weston_log_scope_destroy(b->clipboard_verbose);
		b->clipboard_verbose = NULL;
	}

	if (b->debug) {
		weston_log_scope_destroy(b->debug);
		b->debug = NULL;
	}

	if (b->verbose) {
		weston_log_scope_destroy(b->verbose);
		b->verbose = NULL;
	}

	weston_compositor_shutdown(ec);

	wl_list_for_each_safe(base, next, &ec->head_list, compositor_link) {
		if (to_rdp_head(base))
			rdp_head_destroy(base);
	}

	freerdp_listener_free(b->listener);

	free(b->server_cert);
	free(b->server_key);
	free(b->rdp_key);
	free(b);
}

static
int rdp_listener_activity(int fd, uint32_t mask, void *data)
{
	freerdp_listener* instance = (freerdp_listener *)data;

	if (!(mask & WL_EVENT_READABLE))
		return 0;
	if (!instance->CheckFileDescriptor(instance)) {
		weston_log("failed to check FreeRDP file descriptor\n");
		return -1;
	}
	return 0;
}

static
int rdp_implant_listener(struct rdp_backend *b, freerdp_listener* instance)
{
	int i, fd;
	int handle_count = 0;
	HANDLE handles[MAX_FREERDP_FDS];
	struct wl_event_loop *loop;

	handle_count = instance->GetEventHandles(instance, handles, MAX_FREERDP_FDS);
	if (!handle_count) {
		weston_log("Failed to get FreeRDP handles\n");
		return -1;
	}

	loop = wl_display_get_event_loop(b->compositor->wl_display);
	for (i = 0; i < handle_count; i++) {
		fd = GetEventFileDescriptor(handles[i]);
		b->listener_events[i] = wl_event_loop_add_fd(loop, fd, WL_EVENT_READABLE,
				rdp_listener_activity, instance);
	}

	for ( ; i < MAX_FREERDP_FDS; i++)
		b->listener_events[i] = 0;
	return 0;
}


static BOOL
rdp_peer_context_new(freerdp_peer* client, RdpPeerContext* context)
{
	context->item.peer = client;
	context->item.flags = RDP_PEER_OUTPUT_ENABLED;

	context->loop_task_event_source_fd = -1;
	context->loop_task_event_source = NULL;
	wl_list_init(&context->loop_task_list);

	context->rfx_context = rfx_context_new(TRUE);
	if (!context->rfx_context)
		return FALSE;

	context->rfx_context->mode = RLGR3;
	context->rfx_context->width = client->context->settings->DesktopWidth;
	context->rfx_context->height = client->context->settings->DesktopHeight;
	rfx_context_set_pixel_format(context->rfx_context, DEFAULT_PIXEL_FORMAT);

	context->nsc_context = nsc_context_new();
	if (!context->nsc_context)
		goto out_error_nsc;

	nsc_context_set_parameters(context->nsc_context, NSC_COLOR_FORMAT, DEFAULT_PIXEL_FORMAT);
	context->encode_stream = Stream_New(NULL, 65536);
	if (!context->encode_stream)
		goto out_error_stream;

	return TRUE;

out_error_nsc:
	rfx_context_free(context->rfx_context);
out_error_stream:
	nsc_context_free(context->nsc_context);
	return FALSE;
}

static void
rdp_peer_context_free(freerdp_peer* client, RdpPeerContext* context)
{
	struct rdp_backend *b;
	unsigned i;

	if (!context)
		return;

	b = context->rdpBackend;

	wl_list_remove(&context->item.link);

	for (i = 0; i < ARRAY_LENGTH(context->events); i++) {
		if (context->events[i])
			wl_event_source_remove(context->events[i]);
	}

	if (context->audio_in_private)
		b->audio_in_teardown(context->audio_in_private);

	if (context->audio_out_private)
		b->audio_out_teardown(context->audio_out_private);

	rdp_clipboard_destroy(context);

	if (context->vcm)
		WTSCloseServer(context->vcm);

	rdp_destroy_dispatch_task_event_source(context);

	if (context->item.flags & RDP_PEER_ACTIVATED) {
		weston_seat_release_keyboard(context->item.seat);
		weston_seat_release_pointer(context->item.seat);
		weston_seat_release(context->item.seat);
		free(context->item.seat);
	}

	Stream_Free(context->encode_stream, TRUE);
	nsc_context_free(context->nsc_context);
	rfx_context_free(context->rfx_context);
	free(context->rfx_rects);
}


static int
rdp_client_activity(int fd, uint32_t mask, void *data)
{
	freerdp_peer* client = (freerdp_peer *)data;
	RdpPeerContext *peerCtx = (RdpPeerContext *)client->context;

	if (!client->CheckFileDescriptor(client)) {
		weston_log("unable to checkDescriptor for %p\n", client);
		goto out_clean;
	}

	if (peerCtx && peerCtx->vcm)
	{
		if (!WTSVirtualChannelManagerCheckFileDescriptor(peerCtx->vcm)) {
			weston_log("failed to check FreeRDP WTS VC file descriptor for %p\n", client);
			goto out_clean;
		}
	}

	return 0;

out_clean:
	freerdp_peer_context_free(client);
	freerdp_peer_free(client);
	return 0;
}

static BOOL
xf_peer_capabilities(freerdp_peer* client)
{
	return TRUE;
}

struct rdp_to_xkb_keyboard_layout {
	UINT32 rdpLayoutCode;
	const char *xkbLayout;
	const char *xkbVariant;
};

/* table reversed from
	https://github.com/awakecoding/FreeRDP/blob/master/libfreerdp/locale/xkb_layout_ids.c#L811 */
static const
struct rdp_to_xkb_keyboard_layout rdp_keyboards[] = {
	{KBD_ARABIC_101, "ara", 0},
	{KBD_BULGARIAN, 0, 0},
	{KBD_CHINESE_TRADITIONAL_US, 0},
	{KBD_CZECH, "cz", 0},
	{KBD_CZECH_PROGRAMMERS, "cz", "bksl"},
	{KBD_CZECH_QWERTY, "cz", "qwerty"},
	{KBD_DANISH, "dk", 0},
	{KBD_GERMAN, "de", 0},
	{KBD_GERMAN_NEO, "de", "neo"},
	{KBD_GERMAN_IBM, "de", "qwerty"},
	{KBD_GREEK, "gr", 0},
	{KBD_GREEK_220, "gr", "simple"},
	{KBD_GREEK_319, "gr", "extended"},
	{KBD_GREEK_POLYTONIC, "gr", "polytonic"},
	{KBD_US, "us", 0},
	{KBD_UNITED_STATES_INTERNATIONAL, "us", "intl"},
	{KBD_US_ENGLISH_TABLE_FOR_IBM_ARABIC_238_L, "ara", "buckwalter"},
	{KBD_SPANISH, "es", 0},
	{KBD_SPANISH_VARIATION, "es", "nodeadkeys"},
	{KBD_FINNISH, "fi", 0},
	{KBD_FRENCH, "fr", 0},
	{KBD_HEBREW, "il", 0},
	{KBD_HEBREW_STANDARD, "il", "basic"},
	{KBD_HUNGARIAN, "hu", 0},
	{KBD_HUNGARIAN_101_KEY, "hu", "standard"},
	{KBD_ICELANDIC, "is", 0},
	{KBD_ITALIAN, "it", 0},
	{KBD_ITALIAN_142, "it", "nodeadkeys"},
	{KBD_JAPANESE, "jp", 0},
	{KBD_JAPANESE_INPUT_SYSTEM_MS_IME2002, "jp", 0},
	{KBD_KOREAN, "kr", 0},
	{KBD_KOREAN_INPUT_SYSTEM_IME_2000, "kr", "kr104"},
	{KBD_DUTCH, "nl", 0},
	{KBD_NORWEGIAN, "no", 0},
	{KBD_POLISH_PROGRAMMERS, "pl", 0},
	{KBD_POLISH_214, "pl", "qwertz"},
	{KBD_ROMANIAN, "ro", 0},
	{KBD_RUSSIAN, "ru", 0},
	{KBD_RUSSIAN_TYPEWRITER, "ru", "typewriter"},
	{KBD_CROATIAN, "hr", 0},
	{KBD_SLOVAK, "sk", 0},
	{KBD_SLOVAK_QWERTY, "sk", "qwerty"},
	{KBD_ALBANIAN, 0, 0},
	{KBD_SWEDISH, "se", 0},
	{KBD_THAI_KEDMANEE, "th", 0},
	{KBD_THAI_KEDMANEE_NON_SHIFTLOCK, "th", "tis"},
	{KBD_TURKISH_Q, "tr", 0},
	{KBD_TURKISH_F, "tr", "f"},
	{KBD_URDU, "in", "urd-phonetic3"},
	{KBD_UKRAINIAN, "ua", 0},
	{KBD_BELARUSIAN, "by", 0},
	{KBD_SLOVENIAN, "si", 0},
	{KBD_ESTONIAN, "ee", 0},
	{KBD_LATVIAN, "lv", 0},
	{KBD_LITHUANIAN_IBM, "lt", "ibm"},
	{KBD_FARSI, "ir", "pes"},
	{KBD_PERSIAN, "af", "basic"},
	{KBD_VIETNAMESE, "vn", 0},
	{KBD_ARMENIAN_EASTERN, "am", 0},
	{KBD_AZERI_LATIN, 0, 0},
	{KBD_FYRO_MACEDONIAN, "mk", 0},
	{KBD_GEORGIAN, "ge", 0},
	{KBD_FAEROESE, 0, 0},
	{KBD_DEVANAGARI_INSCRIPT, 0, 0},
	{KBD_MALTESE_47_KEY, 0, 0},
	{KBD_NORWEGIAN_WITH_SAMI, "no", "smi"},
	{KBD_KAZAKH, "kz", 0},
	{KBD_KYRGYZ_CYRILLIC, "kg", "phonetic"},
	{KBD_TATAR, "ru", "tt"},
	{KBD_BENGALI, "bd", 0},
	{KBD_BENGALI_INSCRIPT, "bd", "probhat"},
	{KBD_PUNJABI, 0, 0},
	{KBD_GUJARATI, "in", "guj"},
	{KBD_TAMIL, "in", "tam"},
	{KBD_TELUGU, "in", "tel"},
	{KBD_KANNADA, "in", "kan"},
	{KBD_MALAYALAM, "in", "mal"},
	{KBD_HINDI_TRADITIONAL, "in", 0},
	{KBD_MARATHI, 0, 0},
	{KBD_MONGOLIAN_CYRILLIC, "mn", 0},
	{KBD_UNITED_KINGDOM_EXTENDED, "gb", "intl"},
	{KBD_SYRIAC, "syc", 0},
	{KBD_SYRIAC_PHONETIC, "syc", "syc_phonetic"},
	{KBD_NEPALI, "np", 0},
	{KBD_PASHTO, "af", "ps"},
	{KBD_DIVEHI_PHONETIC, 0, 0},
	{KBD_LUXEMBOURGISH, 0, 0},
	{KBD_MAORI, "mao", 0},
	{KBD_CHINESE_SIMPLIFIED_US, 0, 0},
	{KBD_SWISS_GERMAN, "ch", "de_nodeadkeys"},
	{KBD_UNITED_KINGDOM, "gb", 0},
	{KBD_LATIN_AMERICAN, "latam", 0},
	{KBD_BELGIAN_FRENCH, "be", 0},
	{KBD_BELGIAN_PERIOD, "be", "oss_sundeadkeys"},
	{KBD_PORTUGUESE, "pt", 0},
	{KBD_SERBIAN_LATIN, "rs", 0},
	{KBD_AZERI_CYRILLIC, "az", "cyrillic"},
	{KBD_SWEDISH_WITH_SAMI, "se", "smi"},
	{KBD_UZBEK_CYRILLIC, "af", "uz"},
	{KBD_INUKTITUT_LATIN, "ca", "ike"},
	{KBD_CANADIAN_FRENCH_LEGACY, "ca", "fr-legacy"},
	{KBD_SERBIAN_CYRILLIC, "rs", 0},
	{KBD_CANADIAN_FRENCH, "ca", "fr-legacy"},
	{KBD_SWISS_FRENCH, "ch", "fr"},
	{KBD_BOSNIAN, "ba", 0},
	{KBD_IRISH, 0, 0},
	{KBD_BOSNIAN_CYRILLIC, "ba", "us"},
	{KBD_UNITED_STATES_DVORAK, "us", "dvorak"},
	{KBD_PORTUGUESE_BRAZILIAN_ABNT2, "br", "abnt2"},
	{KBD_CANADIAN_MULTILINGUAL_STANDARD, "ca", "multix"},
	{KBD_GAELIC, "ie", "CloGaelach"},

	{0x00000000, 0, 0},
};

void
convert_rdp_keyboard_to_xkb_rule_names(UINT32 KeyboardType,
				       UINT32 KeyboardSubType,
				       UINT32 KeyboardLayout,
				       struct xkb_rule_names *xkbRuleNames)
{
	int i;

	memset(xkbRuleNames, 0, sizeof(*xkbRuleNames));
	xkbRuleNames->model = "pc105";
	for (i = 0; rdp_keyboards[i].rdpLayoutCode; i++) {
		if (rdp_keyboards[i].rdpLayoutCode == KeyboardLayout) {
			xkbRuleNames->layout = rdp_keyboards[i].xkbLayout;
			xkbRuleNames->variant = rdp_keyboards[i].xkbVariant;
			break;
		}
	}

	/* Korean keyboard support (KeyboardType 8, LangID 0x412) */
	if (KeyboardType == KBD_TYPE_KOREAN && ((KeyboardLayout & 0xFFFF) == 0x412)) {
		/* TODO: PC/AT 101 Enhanced Korean Keyboard (Type B) and (Type C) are not supported yet
			 because default Xkb settings for Korean layout don't have corresponding
			 configuration.
			 (Type B): KeyboardSubType:4: rctrl_hangul/ratl_hanja
			 (Type C): KeyboardSubType:5: shift_space_hangul/crtl_space_hanja
		 */
		if (KeyboardSubType == 0 ||
		    KeyboardSubType == 3) /* PC/AT 101 Enhanced Korean Keyboard (Type A) */
			xkbRuleNames->variant = "kr104"; /* kr(ralt_hangul)/kr(rctrl_hanja) */
		else if (KeyboardSubType == 6) /* PC/AT 103 Enhanced Korean Keyboard */
			xkbRuleNames->variant = "kr106"; /* kr(hw_keys) */
	} else if (KeyboardType != KBD_TYPE_JAPANESE && ((KeyboardLayout & 0xFFFF) == 0x411)) {
		/* when Japanese keyboard layout is used without a Japanese 106/109
		 * keyboard (keyboard type 7), use the "us" layout, since the "jp"
		 * layout in xkb expects the Japanese 106/109 keyboard layout.
		 */
		xkbRuleNames->layout = "us";
		xkbRuleNames->variant = 0;
	}

	weston_log("%s: matching model=%s layout=%s variant=%s\n",
		   __func__, xkbRuleNames->model, xkbRuleNames->layout,
		   xkbRuleNames->variant);
}

static void
rdp_full_refresh(freerdp_peer *peer, struct rdp_output *output)
{
	pixman_box32_t box;
	pixman_region32_t damage;

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = output->base.current_mode->width;
	box.y2 = output->base.current_mode->height;
	pixman_region32_init_with_extents(&damage, &box);

	rdp_peer_refresh_region(&damage, peer);

	pixman_region32_fini(&damage);
}

static BOOL
xf_peer_activate(freerdp_peer* client)
{
	RdpPeerContext *peerCtx;
	struct rdp_backend *b;
	struct rdp_output *output;
	rdpSettings *settings;
	rdpPointerUpdate *pointer;
	struct rdp_peers_item *peersItem;
	struct xkb_rule_names xkbRuleNames;
	struct xkb_keymap *keymap;
	struct weston_output *weston_output;
	char seat_name[50];
	POINTER_SYSTEM_UPDATE pointer_system;
	int width, height;

	peerCtx = (RdpPeerContext *)client->context;
	b = peerCtx->rdpBackend;
	peersItem = &peerCtx->item;
	output = rdp_get_first_output(b);
	settings = client->context->settings;

	if (!settings->SurfaceCommandsEnabled) {
		weston_log("client doesn't support required SurfaceCommands\n");
		return FALSE;
	}

	if (b->force_no_compression && settings->CompressionEnabled) {
		rdp_debug(b, "Forcing compression off\n");
		settings->CompressionEnabled = FALSE;
	}

	settings->AudioPlayback = b->audio_out_setup && b->audio_out_teardown;
	settings->AudioCapture = b->audio_in_setup && b->audio_in_teardown;

	if (settings->RedirectClipboard ||
	    settings->AudioPlayback ||
	    settings->AudioCapture) {
		if (!peerCtx->vcm) {
			weston_log("Virtual channel is required for clipboard, audio playback/capture\n");
			goto error_exit;
		}
		/* Audio setup will return NULL on failure, and we'll proceed without audio */
		if (settings->AudioPlayback)
			peerCtx->audio_out_private = b->audio_out_setup(b->compositor, peerCtx->vcm);

		if (settings->AudioCapture)
			peerCtx->audio_in_private = b->audio_in_setup(b->compositor, peerCtx->vcm);
	}

	/* If we don't allow resize, we need to tell the client to resize itself.
	 * We still need the xf_peer_adjust_monitor_layout() call to make sure
	 * we've set up scaling appropriately.
	 */
	if (b->no_clients_resize) {
		struct weston_mode *mode = output->base.current_mode;

		if (mode->width != (int)settings->DesktopWidth ||
		    mode->height != (int)settings->DesktopHeight) {
			if (!settings->DesktopResize) {
				/* peer does not support desktop resize */
				weston_log("client doesn't support resizing, closing connection\n");
				return FALSE;
			} else {
				settings->DesktopWidth = mode->width;
				settings->DesktopHeight = mode->height;
				client->context->update->DesktopResize(client->context);
			}
		}
	} else {
		xf_peer_adjust_monitor_layout(client);
	}

	weston_output = &output->base;
	width = weston_output->width * weston_output->scale;
	height = weston_output->height * weston_output->scale;
	rfx_context_reset(peerCtx->rfx_context, width, height);
	nsc_context_reset(peerCtx->nsc_context, width, height);

	if (peersItem->flags & RDP_PEER_ACTIVATED)
		return TRUE;

	/* when here it's the first reactivation, we need to setup a little more */
	rdp_debug(b, "kbd_layout:0x%x kbd_type:0x%x kbd_subType:0x%x kbd_functionKeys:0x%x\n",
			settings->KeyboardLayout, settings->KeyboardType, settings->KeyboardSubType,
			settings->KeyboardFunctionKey);

	convert_rdp_keyboard_to_xkb_rule_names(settings->KeyboardType,
					       settings->KeyboardSubType,
					       settings->KeyboardLayout,
					       &xkbRuleNames);

	keymap = NULL;
	if (xkbRuleNames.layout) {
		keymap = xkb_keymap_new_from_names(b->compositor->xkb_context,
						   &xkbRuleNames, 0);
	}

	if (settings->ClientHostname)
		snprintf(seat_name, sizeof(seat_name), "RDP %s", settings->ClientHostname);
	else
		snprintf(seat_name, sizeof(seat_name), "RDP peer @%s", settings->ClientAddress);

	peersItem->seat = zalloc(sizeof(*peersItem->seat));
	if (!peersItem->seat) {
		xkb_keymap_unref(keymap);
		weston_log("unable to create a weston_seat\n");
		return FALSE;
	}

	weston_seat_init(peersItem->seat, b->compositor, seat_name);
	weston_seat_init_keyboard(peersItem->seat, keymap);
	xkb_keymap_unref(keymap);
	weston_seat_init_pointer(peersItem->seat);

	/* Initialize RDP clipboard after seat is initialized */
	if (settings->RedirectClipboard)
		if (rdp_clipboard_init(client) != 0)
			goto error_exit;

	peersItem->flags |= RDP_PEER_ACTIVATED;

	/* disable pointer on the client side */
	pointer = client->context->update->pointer;
	pointer_system.type = SYSPTR_NULL;
	pointer->PointerSystem(client->context, &pointer_system);

	rdp_full_refresh(client, output);

	return TRUE;

error_exit:

	rdp_clipboard_destroy(peerCtx);

	if (settings->AudioPlayback && peerCtx->audio_out_private)
		b->audio_out_teardown(peerCtx->audio_out_private);

	if (settings->AudioCapture && peerCtx->audio_in_private)
		b->audio_in_teardown(peerCtx->audio_in_private);

	return FALSE;
}

static BOOL
xf_peer_post_connect(freerdp_peer *client)
{
	return TRUE;
}

static bool
rdp_translate_and_notify_mouse_position(RdpPeerContext *peerContext, UINT16 x, UINT16 y)
{
	struct weston_coord_global pos;
	struct timespec time;
	int sx, sy;

	if (!peerContext->item.seat)
		return FALSE;

	/* (TS_POINTERX_EVENT):The xy-coordinate of the pointer relative to the top-left
	 *                     corner of the server's desktop combined all monitors */

	/* first, convert the coordinate based on primary monitor's upper-left as (0,0) */
	sx = x + peerContext->desktop_left;
	sy = y + peerContext->desktop_top;

	/* translate client's x/y to the coordinate in weston space. */
	/* TODO: to_weston_coordinate() is translate based on where pointer is,
	         not based-on where/which window underneath. Thus, this doesn't
	         work when window lays across more than 2 monitors and each monitor has
	         different scaling. In such case, hit test to that window area on
	         non primary-resident monitor (surface->output) dosn't work. */
	if (to_weston_coordinate(peerContext, &sx, &sy)) {
		pos.c = weston_coord(sx, sy);
		weston_compositor_get_time(&time);
		notify_motion_absolute(peerContext->item.seat, &time, pos);
		return TRUE;
	}
	return FALSE;
}

static void
dump_mouseinput(RdpPeerContext *peerContext, UINT16 flags, UINT16 x, UINT16 y, bool is_ex)
{
	struct rdp_backend *b = peerContext->rdpBackend;

	rdp_debug_verbose(b, "RDP mouse input%s: (%d, %d): flags:%x: ", is_ex ? "_ex" : "", x, y, flags);
	if (is_ex) {
		if (flags & PTR_XFLAGS_DOWN)
			rdp_debug_verbose_continue(b, "DOWN ");
		if (flags & PTR_XFLAGS_BUTTON1)
			rdp_debug_verbose_continue(b, "XBUTTON1 ");
		if (flags & PTR_XFLAGS_BUTTON2)
			rdp_debug_verbose_continue(b, "XBUTTON2 ");
	} else {
		if (flags & PTR_FLAGS_WHEEL)
			rdp_debug_verbose_continue(b, "WHEEL ");
		if (flags & PTR_FLAGS_WHEEL_NEGATIVE)
			rdp_debug_verbose_continue(b, "WHEEL_NEGATIVE ");
		if (flags & PTR_FLAGS_HWHEEL)
			rdp_debug_verbose_continue(b, "HWHEEL ");
		if (flags & PTR_FLAGS_MOVE)
			rdp_debug_verbose_continue(b, "MOVE ");
		if (flags & PTR_FLAGS_DOWN)
			rdp_debug_verbose_continue(b, "DOWN ");
		if (flags & PTR_FLAGS_BUTTON1)
			rdp_debug_verbose_continue(b, "BUTTON1 ");
		if (flags & PTR_FLAGS_BUTTON2)
			rdp_debug_verbose_continue(b, "BUTTON2 ");
		if (flags & PTR_FLAGS_BUTTON3)
			rdp_debug_verbose_continue(b, "BUTTON3 ");
	}
	rdp_debug_verbose_continue(b, "\n");
}

static void
rdp_validate_button_state(RdpPeerContext *peerContext, bool pressed, uint32_t *button)
{
	struct rdp_backend *b = peerContext->rdpBackend;
	uint32_t index;

	if (*button < BTN_LEFT || *button > BTN_EXTRA) {
		weston_log("RDP client posted invalid button event\n");
		goto ignore;
	}

	index = *button - BTN_LEFT;
	assert(index < ARRAY_LENGTH(peerContext->button_state));

	if (pressed == peerContext->button_state[index]) {
		rdp_debug_verbose(b, "%s: inconsistent button state button:%d (index:%d) pressed:%d\n",
				  __func__, *button, index, pressed);
		goto ignore;
	} else {
		peerContext->button_state[index] = pressed;
	}
	return;
ignore:
	/* ignore button input */
	*button = 0;
}

static bool
rdp_notify_wheel_scroll(RdpPeerContext *peerContext, UINT16 flags, uint32_t axis)
{
	struct weston_pointer_axis_event weston_event;
	struct rdp_backend *b = peerContext->rdpBackend;
	int ivalue;
	double value;
	struct timespec time;
	int *accumWheelRotationPrecise;
	int *accumWheelRotationDiscrete;

	/*
	* The RDP specs says the lower bits of flags contains the "the number of rotation
	* units the mouse wheel was rotated".
	*/
	ivalue = ((int)flags & 0x000000ff);
	if (flags & PTR_FLAGS_WHEEL_NEGATIVE)
		ivalue = (0xff - ivalue) * -1;

	/*
	* Flip the scroll direction as the RDP direction is inverse of X/Wayland
	* for vertical scroll
	*/
	if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
		ivalue *= -1;

		accumWheelRotationPrecise = &peerContext->verticalAccumWheelRotationPrecise;
		accumWheelRotationDiscrete = &peerContext->verticalAccumWheelRotationDiscrete;
	}
	else {
		accumWheelRotationPrecise = &peerContext->horizontalAccumWheelRotationPrecise;
		accumWheelRotationDiscrete = &peerContext->horizontalAccumWheelRotationDiscrete;
	}

	/*
	* Accumulate the wheel increments.
	*
	* Every 12 wheel increments, we will send an update to our Wayland
	* clients with an updated value for the wheel for smooth scrolling.
	*
	* Every 120 wheel increments, we tick one discrete wheel click.
	*
	* https://devblogs.microsoft.com/oldnewthing/20130123-00/?p=5473 explains the 120 value
	*/
	*accumWheelRotationPrecise += ivalue;
	*accumWheelRotationDiscrete += ivalue;
	rdp_debug_verbose(b, "wheel: rawValue:%d accumPrecise:%d accumDiscrete %d\n",
			  ivalue, *accumWheelRotationPrecise, *accumWheelRotationDiscrete);

	if (abs(*accumWheelRotationPrecise) >= 12) {
		value = (double)(*accumWheelRotationPrecise / 12);

		weston_event.axis = axis;
		weston_event.value = value;
		weston_event.discrete = *accumWheelRotationDiscrete / 120;
		weston_event.has_discrete = true;

		rdp_debug_verbose(b, "wheel: value:%f discrete:%d\n",
				  weston_event.value, weston_event.discrete);

		weston_compositor_get_time(&time);

		notify_axis(peerContext->item.seat, &time, &weston_event);

		*accumWheelRotationPrecise %= 12;
		*accumWheelRotationDiscrete %= 120;

		return true;
	}

	return false;
}

static BOOL
xf_mouseEvent(rdpInput *input, UINT16 flags, UINT16 x, UINT16 y)
{
	RdpPeerContext *peerContext = (RdpPeerContext *)input->context;
	uint32_t button = 0;
	bool need_frame = false;
	struct timespec time;

	dump_mouseinput(peerContext, flags, x, y, false);

	/* Per RDP spec, the x,y position is valid on all input mouse messages,
	 * except for PTR_FLAGS_WHEEL and PTR_FLAGS_HWHEEL event. Take the opportunity
	 * to resample our x,y position even when PTR_FLAGS_MOVE isn't explicitly set,
	 * for example a button down/up only notification, to ensure proper sync with
	 * the RDP client.
	 */
	if (!(flags & (PTR_FLAGS_WHEEL | PTR_FLAGS_HWHEEL))) {
		if (rdp_translate_and_notify_mouse_position(peerContext, x, y))
			need_frame = true;
	}

	if (flags & PTR_FLAGS_BUTTON1)
		button = BTN_LEFT;
	else if (flags & PTR_FLAGS_BUTTON2)
		button = BTN_RIGHT;
	else if (flags & PTR_FLAGS_BUTTON3)
		button = BTN_MIDDLE;

	if (button) {
		rdp_validate_button_state(peerContext,
					  flags & PTR_FLAGS_DOWN ? true : false,
					  &button);
	}

	if (button) {
		weston_compositor_get_time(&time);
		notify_button(peerContext->item.seat, &time, button,
			(flags & PTR_FLAGS_DOWN) ? WL_POINTER_BUTTON_STATE_PRESSED : WL_POINTER_BUTTON_STATE_RELEASED
		);
		need_frame = true;
	}

	/* Per RDP spec, if both PTRFLAGS_WHEEL and PTRFLAGS_HWHEEL are specified
	 * then PTRFLAGS_WHEEL takes precedent
	 */
	if (flags & PTR_FLAGS_WHEEL) {
		if (rdp_notify_wheel_scroll(peerContext, flags, WL_POINTER_AXIS_VERTICAL_SCROLL))
			need_frame = true;
	} else if (flags & PTR_FLAGS_HWHEEL) {
		if (rdp_notify_wheel_scroll(peerContext, flags, WL_POINTER_AXIS_HORIZONTAL_SCROLL))
			need_frame = true;
	}

	if (need_frame)
		notify_pointer_frame(peerContext->item.seat);

	return TRUE;
}

static BOOL
xf_extendedMouseEvent(rdpInput *input, UINT16 flags, UINT16 x, UINT16 y)
{
	RdpPeerContext *peerContext = (RdpPeerContext *)input->context;
	uint32_t button = 0;
	bool need_frame = false;
	struct rdp_output *output;
	struct timespec time;
	struct weston_coord_global pos;

	dump_mouseinput(peerContext, flags, x, y, true);

	if (flags & PTR_XFLAGS_BUTTON1)
		button = BTN_SIDE;
	else if (flags & PTR_XFLAGS_BUTTON2)
		button = BTN_EXTRA;

	if (button) {
		rdp_validate_button_state(peerContext,
					  flags & PTR_XFLAGS_DOWN ? true : false,
					  &button);
	}

	if (button) {
		weston_compositor_get_time(&time);
		notify_button(peerContext->item.seat, &time, button,
			      (flags & PTR_XFLAGS_DOWN) ? WL_POINTER_BUTTON_STATE_PRESSED : WL_POINTER_BUTTON_STATE_RELEASED);
		need_frame = true;
	}

	output = rdp_get_first_output(peerContext->rdpBackend);
	if (x < output->base.width && y < output->base.height) {
		weston_compositor_get_time(&time);
		pos.c = weston_coord(x, y);
		notify_motion_absolute(peerContext->item.seat, &time, pos);
		need_frame = true;
	}

	if (need_frame)
		notify_pointer_frame(peerContext->item.seat);

	return TRUE;
}


static BOOL
xf_input_synchronize_event(rdpInput *input, UINT32 flags)
{
	freerdp_peer *client = input->context->peer;
	RdpPeerContext *peerCtx = (RdpPeerContext *)input->context;
	struct rdp_backend *b = peerCtx->rdpBackend;
	struct rdp_output *output = rdp_get_first_output(b);
	struct weston_keyboard *keyboard;

        rdp_debug_verbose(b, "RDP backend: %s ScrLk:%d, NumLk:%d, CapsLk:%d, KanaLk:%d\n",
			  __func__,
			  flags & KBD_SYNC_SCROLL_LOCK ? 1 : 0,
			  flags & KBD_SYNC_NUM_LOCK ? 1 : 0,
			  flags & KBD_SYNC_CAPS_LOCK ? 1 : 0,
			  flags & KBD_SYNC_KANA_LOCK ? 1 : 0);

	keyboard = weston_seat_get_keyboard(peerCtx->item.seat);
	if (keyboard) {
		uint32_t value = 0;

		if (flags & KBD_SYNC_NUM_LOCK)
			value |= WESTON_NUM_LOCK;
		if (flags & KBD_SYNC_CAPS_LOCK)
			value |= WESTON_CAPS_LOCK;
		weston_keyboard_set_locks(keyboard,
					  WESTON_NUM_LOCK | WESTON_CAPS_LOCK,
					  value);
	}

	rdp_full_refresh(client, output);

	return TRUE;
}


static BOOL
xf_input_keyboard_event(rdpInput *input, UINT16 flags, UINT16 code)
{
	uint32_t scan_code, vk_code, full_code;
	enum wl_keyboard_key_state keyState;
	freerdp_peer *client = input->context->peer;
	RdpPeerContext *peerContext = (RdpPeerContext *)input->context;
	bool send_release_key = false;
	int notify = 0;
	struct timespec time;

	if (!(peerContext->item.flags & RDP_PEER_ACTIVATED))
		return TRUE;

	if (flags & KBD_FLAGS_DOWN) {
		keyState = WL_KEYBOARD_KEY_STATE_PRESSED;
		notify = 1;
	} else if (flags & KBD_FLAGS_RELEASE) {
		keyState = WL_KEYBOARD_KEY_STATE_RELEASED;
		notify = 1;
	}

	if (notify) {
		full_code = code;
		if (flags & KBD_FLAGS_EXTENDED)
			full_code |= KBD_FLAGS_EXTENDED;

		/* Korean keyboard support:
		 * WinPR's GetVirtualKeyCodeFromVirtualScanCode() can't handle hangul/hanja keys
		 * hanja and hangeul keys are only present on Korean 103 keyboard (Type 8:SubType 6) */
		if (client->context->settings->KeyboardType == 8 &&
		    client->context->settings->KeyboardSubType == 6 &&
		    ((full_code == (KBD_FLAGS_EXTENDED | ATKBD_RET_HANJA)) ||
		     (full_code == (KBD_FLAGS_EXTENDED | ATKBD_RET_HANGEUL)))) {
			if (full_code == (KBD_FLAGS_EXTENDED | ATKBD_RET_HANJA))
				vk_code = VK_HANJA;
			else if (full_code == (KBD_FLAGS_EXTENDED | ATKBD_RET_HANGEUL))
				vk_code = VK_HANGUL;
			/* From Linux's keyboard driver at drivers/input/keyboard/atkbd.c */
			/*
			 * HANGEUL and HANJA keys do not send release events so we need to
			 * generate such events ourselves
			 */
			/* Similarly, for RDP there is no release for those 2 Korean keys,
			 * thus generate release right after press. */
			if (keyState != WL_KEYBOARD_KEY_STATE_PRESSED) {
				weston_log("RDP: Received invalid key release\n");
				return TRUE;
			}
			send_release_key = true;
		} else {
			vk_code = GetVirtualKeyCodeFromVirtualScanCode(full_code, client->context->settings->KeyboardType);
		}
		/* Korean keyboard support */
		/* WinPR's GetKeycodeFromVirtualKeyCode() expects no extended bit for VK_HANGUL and VK_HANJA */
		if (vk_code != VK_HANGUL && vk_code != VK_HANJA)
			if (flags & KBD_FLAGS_EXTENDED)
				vk_code |= KBDEXT;

		scan_code = GetKeycodeFromVirtualKeyCode(vk_code, KEYCODE_TYPE_EVDEV);

		/*weston_log("code=%x ext=%d vk_code=%x scan_code=%x\n", code, (flags & KBD_FLAGS_EXTENDED) ? 1 : 0,
				vk_code, scan_code);*/
		weston_compositor_get_time(&time);
		notify_key(peerContext->item.seat, &time,
					scan_code - 8, keyState, STATE_UPDATE_AUTOMATIC);

		if (send_release_key) {
			notify_key(peerContext->item.seat, &time,
				   scan_code - 8,
				   WL_KEYBOARD_KEY_STATE_RELEASED,
				   STATE_UPDATE_AUTOMATIC);
		}
	}

	return TRUE;
}

static BOOL
xf_input_unicode_keyboard_event(rdpInput *input, UINT16 flags, UINT16 code)
{
	RdpPeerContext *peerContext = (RdpPeerContext *)input->context;
	struct rdp_backend *b = peerContext->rdpBackend;

	rdp_debug(b, "Client sent a unicode keyboard event (flags:0x%X code:0x%X)\n", flags, code);

	return TRUE;
}


static BOOL
xf_suppress_output(rdpContext *context, BYTE allow, const RECTANGLE_16 *area)
{
	RdpPeerContext *peerContext = (RdpPeerContext *)context;

	if (allow)
		peerContext->item.flags |= RDP_PEER_OUTPUT_ENABLED;
	else
		peerContext->item.flags &= (~RDP_PEER_OUTPUT_ENABLED);

	return TRUE;
}

static BOOL
xf_peer_adjust_monitor_layout(freerdp_peer *client)
{
	RdpPeerContext *peerCtx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = peerCtx->rdpBackend;
	rdpSettings *settings = client->context->settings;
	rdpMonitor *monitors;
	unsigned int monitor_count;
	BOOL success;
	bool fallback = false;
	unsigned int i;

	rdp_debug(b, "%s:\n", __func__);
	rdp_debug(b, "  DesktopWidth:%d, DesktopHeight:%d\n", settings->DesktopWidth, settings->DesktopHeight);
	rdp_debug(b, "  UseMultimon:%d\n", settings->UseMultimon);
	rdp_debug(b, "  ForceMultimon:%d\n", settings->ForceMultimon);
	rdp_debug(b, "  MonitorCount:%d\n", settings->MonitorCount);
	rdp_debug(b, "  HasMonitorAttributes:%d\n", settings->HasMonitorAttributes);
	rdp_debug(b, "  HiDefRemoteApp:%d\n", settings->HiDefRemoteApp);

	if (settings->MonitorCount > 1) {
		weston_log("multiple monitor is not supported");
		fallback = true;
	}

	if (b->no_clients_resize)
		fallback = true;

	if (settings->MonitorCount > RDP_MAX_MONITOR) {
		weston_log("Client reports more monitors then expected:(%d)\n",
			   settings->MonitorCount);
		return FALSE;
	}
	if ((settings->MonitorCount > 0 && settings->MonitorDefArray) && !fallback) {
		rdpMonitor *rdp_monitor = settings->MonitorDefArray;
		monitor_count = settings->MonitorCount;
		monitors = xmalloc(sizeof(*monitors) * monitor_count);
		for (i = 0; i < monitor_count; i++) {
			monitors[i] = rdp_monitor[i];
			if (!settings->HasMonitorAttributes) {
				monitors[i].attributes.physicalWidth = 0;
				monitors[i].attributes.physicalHeight = 0;
				monitors[i].attributes.orientation = ORIENTATION_LANDSCAPE;
				monitors[i].attributes.desktopScaleFactor = 100;
				monitors[i].attributes.deviceScaleFactor = 100;
			}
		}
	} else {
		monitor_count = 1;
		monitors = xmalloc(sizeof(*monitors) * monitor_count);
		/* when no monitor array provided, generate from desktop settings */
		monitors[0].x = 0;
		monitors[0].y = 0;
		monitors[0].width = settings->DesktopWidth;
		monitors[0].height = settings->DesktopHeight;
		monitors[0].is_primary = 1;
		monitors[0].attributes.physicalWidth = settings->DesktopPhysicalWidth;
		monitors[0].attributes.physicalHeight = settings->DesktopPhysicalHeight;
		monitors[0].attributes.orientation = settings->DesktopOrientation;
		monitors[0].attributes.desktopScaleFactor = settings->DesktopScaleFactor;
		monitors[0].attributes.deviceScaleFactor = settings->DeviceScaleFactor;
		monitors[0].orig_screen = 0;

		if (b->no_clients_resize) {
			/* If we're not allowing clients to resize us, set these
			 * to 0 so the front end knows it needs to make something
			 * up.
			 */
			monitors[0].width = 0;
			monitors[0].height = 0;
			monitors[0].attributes.desktopScaleFactor = 0;
		}
	}
	success = handle_adjust_monitor_layout(client, monitor_count, monitors);

	free(monitors);
	return success;
}

static int
rdp_peer_init(freerdp_peer *client, struct rdp_backend *b)
{
	int handle_count = 0;
	HANDLE handles[MAX_FREERDP_FDS + 1]; /* +1 for virtual channel */
	int i, fd;
	struct wl_event_loop *loop;
	rdpSettings	*settings;
	rdpInput *input;
	RdpPeerContext *peerCtx;

	client->ContextSize = sizeof(RdpPeerContext);
	client->ContextNew = (psPeerContextNew)rdp_peer_context_new;
	client->ContextFree = (psPeerContextFree)rdp_peer_context_free;
	freerdp_peer_context_new(client);

	peerCtx = (RdpPeerContext *) client->context;
	peerCtx->rdpBackend = b;

	settings = client->context->settings;
	/* configure security settings */
	if (b->rdp_key)
		settings->RdpKeyFile = strdup(b->rdp_key);
	if (b->tls_enabled) {
		settings->CertificateFile = strdup(b->server_cert);
		settings->PrivateKeyFile = strdup(b->server_key);
	} else {
		settings->TlsSecurity = FALSE;
	}
	settings->NlaSecurity = FALSE;

	if (!client->Initialize(client)) {
		weston_log("peer initialization failed\n");
		goto error_initialize;
	}

	settings->OsMajorType = OSMAJORTYPE_UNIX;
	settings->OsMinorType = OSMINORTYPE_PSEUDO_XSERVER;
	settings->ColorDepth = 32;
	settings->RefreshRect = TRUE;
	settings->RemoteFxCodec = b->remotefx_codec;
	settings->NSCodec = TRUE;
	settings->FrameMarkerCommandEnabled = TRUE;
	settings->SurfaceFrameMarkerEnabled = TRUE;
	settings->RedirectClipboard = TRUE;
	settings->HasExtendedMouseEvent = TRUE;
	settings->HasHorizontalWheel = TRUE;

	client->Capabilities = xf_peer_capabilities;
	client->PostConnect = xf_peer_post_connect;
	client->Activate = xf_peer_activate;

	if (!b->no_clients_resize) {
		settings->SupportMonitorLayoutPdu = TRUE;
		client->AdjustMonitorsLayout = xf_peer_adjust_monitor_layout;
	}

	client->context->update->SuppressOutput = (pSuppressOutput)xf_suppress_output;

	input = client->context->input;
	input->SynchronizeEvent = xf_input_synchronize_event;
	input->MouseEvent = xf_mouseEvent;
	input->ExtendedMouseEvent = xf_extendedMouseEvent;
	input->KeyboardEvent = xf_input_keyboard_event;
	input->UnicodeKeyboardEvent = xf_input_unicode_keyboard_event;

	handle_count = client->GetEventHandles(client, handles, MAX_FREERDP_FDS);
	if (!handle_count) {
		weston_log("unable to retrieve client handles\n");
		goto error_initialize;
	}

	PWtsApiFunctionTable fn = FreeRDP_InitWtsApi();
	WTSRegisterWtsApiFunctionTable(fn);
	peerCtx->vcm = WTSOpenServerA((LPSTR)peerCtx);
	if (peerCtx->vcm) {
		handles[handle_count++] = WTSVirtualChannelManagerGetEventHandle(peerCtx->vcm);
	} else {
		weston_log("WTSOpenServer is failed! continue without virtual channel.\n");
	}

	loop = wl_display_get_event_loop(b->compositor->wl_display);
	for (i = 0; i < handle_count; i++) {
		fd = GetEventFileDescriptor(handles[i]);

		peerCtx->events[i] = wl_event_loop_add_fd(loop, fd, WL_EVENT_READABLE,
				rdp_client_activity, client);
	}
	for ( ; i < (int)ARRAY_LENGTH(peerCtx->events); i++)
		peerCtx->events[i] = 0;

	wl_list_insert(&b->peers, &peerCtx->item.link);

	if (!rdp_initialize_dispatch_task_event_source(peerCtx))
		goto error_dispatch_initialize;

	return 0;

error_dispatch_initialize:
	for (i = 0; i < (int)ARRAY_LENGTH(peerCtx->events); i++) {
		if (peerCtx->events[i]) {
			wl_event_source_remove(peerCtx->events[i]);
			peerCtx->events[i] = NULL;
		}
	}
	if (peerCtx->vcm) {
		WTSCloseServer(peerCtx->vcm);
		peerCtx->vcm = NULL;
	}

error_initialize:
	client->Close(client);
	return -1;
}


static BOOL
rdp_incoming_peer(freerdp_listener *instance, freerdp_peer *client)
{
	struct rdp_backend *b = (struct rdp_backend *)instance->param4;
	if (rdp_peer_init(client, b) < 0) {
		weston_log("error when treating incoming peer\n");
		return FALSE;
	}

	return TRUE;
}

static const struct weston_rdp_output_api api = {
	rdp_head_get_monitor,
	rdp_output_set_mode,
};

static struct rdp_backend *
rdp_backend_create(struct weston_compositor *compositor,
		   struct weston_rdp_backend_config *config)
{
	struct rdp_backend *b;
	char *fd_str;
	char *fd_tail;
	int fd, ret;

	struct weston_head *base, *next;

	b = xzalloc(sizeof *b);
	b->compositor_tid = gettid();
	b->compositor = compositor;
	b->base.destroy = rdp_destroy;
	b->base.create_output = rdp_output_create;
	b->rdp_key = config->rdp_key ? strdup(config->rdp_key) : NULL;
	b->no_clients_resize = config->no_clients_resize;
	b->force_no_compression = config->force_no_compression;
	b->remotefx_codec = config->remotefx_codec;
	b->audio_in_setup = config->audio_in_setup;
	b->audio_in_teardown = config->audio_in_teardown;
	b->audio_out_setup = config->audio_out_setup;
	b->audio_out_teardown = config->audio_out_teardown;

	b->debug = weston_compositor_add_log_scope(compositor,
						   "rdp-backend",
						   "Debug messages from RDP backend\n",
						   NULL, NULL, NULL);
	b->verbose = weston_compositor_add_log_scope(compositor,
						     "rdp-backend-verbose",
						     "Verbose debug messages from RDP backend\n",
						     NULL, NULL, NULL);

	/* After here, rdp_debug() is ready to be used */

	b->rdp_monitor_refresh_rate = config->refresh_rate * 1000;
	rdp_debug(b, "RDP backend: WESTON_RDP_MONITOR_REFRESH_RATE: %d\n", b->rdp_monitor_refresh_rate);

	b->clipboard_debug = weston_log_ctx_add_log_scope(b->compositor->weston_log_ctx,
							  "rdp-backend-clipboard",
							  "Debug messages from RDP backend clipboard\n",
							  NULL, NULL, NULL);
	b->clipboard_verbose = weston_log_ctx_add_log_scope(b->compositor->weston_log_ctx,
							    "rdp-backend-clipboard-verbose",
							    "Debug messages from RDP backend clipboard\n",
							    NULL, NULL, NULL);

	compositor->backend = &b->base;

	if (config->server_cert && config->server_key) {
		b->server_cert = strdup(config->server_cert);
		b->server_key = strdup(config->server_key);
		if (!b->server_cert || !b->server_key)
			goto err_free_strings;
	}

	switch (config->renderer) {
	case WESTON_RENDERER_PIXMAN:
	case WESTON_RENDERER_AUTO:
		break;
	default:
		weston_log("Unsupported renderer requested\n");
		goto err_free_strings;
	}

	/* if we are listening for client connections on an external listener
	 * fd, we don't need to enforce TLS or RDP security, since FreeRDP
	 * will consider it to be a local connection */
	fd = config->external_listener_fd;
	if (fd < 0) {
		if (!b->rdp_key && (!b->server_cert || !b->server_key)) {
			weston_log("the RDP compositor requires keys and an optional certificate for RDP or TLS security ("
				   "--rdp4-key or --rdp-tls-cert/--rdp-tls-key)\n");
			goto err_free_strings;
		}
		if (b->server_cert && b->server_key) {
			b->tls_enabled = 1;
			rdp_debug(b, "TLS support activated\n");
		}
	}

	wl_list_init(&b->peers);

	if (weston_compositor_set_presentation_clock_software(compositor) < 0)
		goto err_compositor;

	if (weston_compositor_init_renderer(compositor, WESTON_RENDERER_PIXMAN,
					    NULL) < 0)
		goto err_compositor;

	rdp_head_create(b, NULL);

	compositor->capabilities |= WESTON_CAP_ARBITRARY_MODES;

	if (!config->env_socket) {
		b->listener = freerdp_listener_new();
		b->listener->PeerAccepted = rdp_incoming_peer;
		b->listener->param4 = b;
		if (fd >= 0) {
			rdp_debug(b, "Using external fd for incoming connections: %d\n", fd);

			if (!b->listener->OpenFromSocket(b->listener, fd)) {
				weston_log("RDP unable to use external listener fd: %d\n", fd);
				goto err_listener;
			}
		} else {
			if (!b->listener->Open(b->listener, config->bind_address, config->port)) {
				weston_log("RDP unable to bind socket\n");
				goto err_listener;
			}
		}

		if (rdp_implant_listener(b, b->listener) < 0)
			goto err_listener;
	} else {
		/* get the socket from RDP_FD var */
		fd_str = getenv("RDP_FD");
		if (!fd_str) {
			weston_log("RDP_FD env variable not set\n");
			goto err_compositor;
		}

		fd = strtoul(fd_str, &fd_tail, 10);
		if (errno != 0 || fd_tail == fd_str || *fd_tail != '\0'
		    || rdp_peer_init(freerdp_peer_new(fd), b))
			goto err_compositor;
	}

	ret = weston_plugin_api_register(compositor, WESTON_RDP_OUTPUT_API_NAME,
					 &api, sizeof(api));

	if (ret < 0) {
		weston_log("Failed to register output API.\n");
		goto err_listener;
	}

	return b;

err_listener:
	freerdp_listener_free(b->listener);
err_compositor:
	wl_list_for_each_safe(base, next, &compositor->head_list, compositor_link) {
		if (to_rdp_head(base))
			rdp_head_destroy(base);
	}

	weston_compositor_shutdown(compositor);
err_free_strings:
	if (b->clipboard_debug)
		weston_log_scope_destroy(b->clipboard_debug);
	if (b->clipboard_verbose)
		weston_log_scope_destroy(b->clipboard_verbose);
	if (b->debug)
		weston_log_scope_destroy(b->debug);
	if (b->verbose)
		weston_log_scope_destroy(b->verbose);
	free(b->rdp_key);
	free(b->server_cert);
	free(b->server_key);
	free(b);
	return NULL;
}

static void
config_init_to_defaults(struct weston_rdp_backend_config *config)
{
	config->renderer = WESTON_RENDERER_AUTO;
	config->bind_address = NULL;
	config->port = 3389;
	config->rdp_key = NULL;
	config->server_cert = NULL;
	config->server_key = NULL;
	config->env_socket = 0;
	config->no_clients_resize = 0;
	config->force_no_compression = 0;
	config->remotefx_codec = true;
	config->external_listener_fd = -1;
	config->refresh_rate = RDP_DEFAULT_FREQ;
	config->audio_in_setup = NULL;
	config->audio_in_teardown = NULL;
	config->audio_out_setup = NULL;
	config->audio_out_teardown = NULL;
}

WL_EXPORT int
weston_backend_init(struct weston_compositor *compositor,
		    struct weston_backend_config *config_base)
{
	struct rdp_backend *b;
	struct weston_rdp_backend_config config = {{ 0, }};
	int major, minor, revision;

#if FREERDP_VERSION_MAJOR >= 2
	winpr_InitializeSSL(0);
#endif
	freerdp_get_version(&major, &minor, &revision);
	weston_log("using FreeRDP version %d.%d.%d\n", major, minor, revision);

	if (config_base == NULL ||
	    config_base->struct_version != WESTON_RDP_BACKEND_CONFIG_VERSION ||
	    config_base->struct_size > sizeof(struct weston_rdp_backend_config)) {
		weston_log("RDP backend config structure is invalid\n");
		return -1;
	}

	config_init_to_defaults(&config);
	memcpy(&config, config_base, config_base->struct_size);

	b = rdp_backend_create(compositor, &config);
	if (b == NULL)
		return -1;
	return 0;
}
