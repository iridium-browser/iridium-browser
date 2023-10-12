/*
 * Copyright © 2021-2023 Pengutronix, Philipp Zabel <p.zabel@pengutronix.de>
 * based on backend-rdp:
 * Copyright © 2013 Hardening <rdp.effort@gmail.com>
 * and pipewire-plugin:
 * Copyright © 2019 Pengutronix, Michael Olbrich <m.olbrich@pengutronix.de>
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
#include <linux/input.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <drm_fourcc.h>
#include <fcntl.h>

#include <pipewire/pipewire.h>
#include <spa/buffer/meta.h>
#include <spa/debug/types.h>
#include <spa/param/format-utils.h>
#include <spa/param/video/format-utils.h>
#include <spa/utils/defs.h>
#include <spa/utils/result.h>

#include "shared/helpers.h"
#include "shared/timespec-util.h"
#include "shared/xalloc.h"
#include <libweston/libweston.h>
#include <libweston/backend-pipewire.h>
#include <libweston/weston-log.h>
#include "pixel-formats.h"
#include "pixman-renderer.h"

struct pipewire_backend {
	struct weston_backend base;
	struct weston_compositor *compositor;

	const struct pixel_format_info *pixel_format;

	struct weston_log_scope *debug;

	struct pw_loop *loop;
	struct wl_event_source *loop_source;

	struct pw_context *context;
	struct pw_core *core;
	struct spa_hook core_listener;
};

struct pipewire_output {
	struct weston_output base;
	struct pipewire_backend *backend;

	uint32_t seq;
	struct pw_stream *stream;
	struct spa_hook stream_listener;

	const struct pixel_format_info *pixel_format;

	struct wl_event_source *finish_frame_timer;
	struct wl_list link;
};

struct pipewire_head {
	struct weston_head base;
	struct pipewire_config config;
};

struct pipewire_frame_data {
	struct pipewire_output *output;
	struct pw_buffer *buffer;
	struct weston_renderbuffer *renderbuffer;
};

/* Pipewire default configuration for heads */
static const struct pipewire_config default_config = {
	.width = 640,
	.height = 480,
	.framerate = 30,
};

static void
pipewire_debug_impl(struct pipewire_backend *pipewire,
		    struct pipewire_output *output,
		    const char *fmt, va_list ap)
{
	FILE *fp;
	char *logstr;
	size_t logsize;
	char timestr[128];

	if (!weston_log_scope_is_enabled(pipewire->debug))
		return;

	fp = open_memstream(&logstr, &logsize);
	if (!fp)
		return;

	weston_log_scope_timestamp(pipewire->debug, timestr, sizeof timestr);
	fprintf(fp, "%s", timestr);

	if (output)
		fprintf(fp, "[%s]", output->base.name);

	fprintf(fp, " ");
	vfprintf(fp, fmt, ap);
	fprintf(fp, "\n");

	if (fclose(fp) == 0)
		weston_log_scope_write(pipewire->debug, logstr, logsize);

	free(logstr);
}

static void
pipewire_output_debug(struct pipewire_output *output, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	pipewire_debug_impl(output->backend, output, fmt, ap);
	va_end(ap);
}

static inline struct pipewire_backend *
to_pipewire_backend(struct weston_backend *base)
{
	return container_of(base, struct pipewire_backend, base);
}

static void
pipewire_output_destroy(struct weston_output *base);

static inline struct pipewire_output *
to_pipewire_output(struct weston_output *base)
{
	if (base->destroy != pipewire_output_destroy)
		return NULL;
	return container_of(base, struct pipewire_output, base);
}

static void
pipewire_head_destroy(struct weston_head *base);

static void
pipewire_destroy(struct weston_backend *backend);

static inline struct pipewire_head *
to_pipewire_head(struct weston_head *base)
{
	if (base->backend->destroy != pipewire_destroy)
		return NULL;
	return container_of(base, struct pipewire_head, base);
}

static enum spa_video_format
spa_video_format_from_drm_fourcc(uint32_t fourcc)
{
	switch (fourcc) {
	case DRM_FORMAT_XRGB8888:
		return SPA_VIDEO_FORMAT_BGRx;
	case DRM_FORMAT_RGB565:
		return SPA_VIDEO_FORMAT_RGB16;
	default:
		return SPA_VIDEO_FORMAT_UNKNOWN;
	}
}

static int
pipewire_output_connect(struct pipewire_output *output)
{
	uint8_t buffer[1024];
	struct spa_pod_builder builder =
		SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
	const struct spa_pod *params[1];
	int framerate;
	int width;
	int height;
	enum spa_video_format format;
	int ret;

	framerate = output->base.current_mode->refresh / 1000;
	width = output->base.width;
	height = output->base.height;

	format = spa_video_format_from_drm_fourcc(output->pixel_format->format);

	params[0] = spa_pod_builder_add_object(&builder,
		SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
		SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
		SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
		SPA_FORMAT_VIDEO_format, SPA_POD_Id(format),
		SPA_FORMAT_VIDEO_size, SPA_POD_Rectangle(&SPA_RECTANGLE(width, height)),
		SPA_FORMAT_VIDEO_framerate, SPA_POD_Fraction(&SPA_FRACTION (0, 1)),
		SPA_FORMAT_VIDEO_maxFramerate,
		SPA_POD_CHOICE_RANGE_Fraction(&SPA_FRACTION(framerate, 1),
			&SPA_FRACTION(1, 1),
			&SPA_FRACTION(framerate, 1)));

	ret = pw_stream_connect(output->stream, PW_DIRECTION_OUTPUT, PW_ID_ANY,
				PW_STREAM_FLAG_DRIVER |
				PW_STREAM_FLAG_MAP_BUFFERS,
				params, 1);
	if (ret != 0) {
		weston_log("Failed to connect PipeWire stream: %s",
			   spa_strerror(ret));
		return -1;
	}

	return 0;
}

static int
finish_frame_handler(void *data)
{
	struct pipewire_output *output = data;
	int refresh_nsec = millihz_to_nsec(output->base.current_mode->refresh);
	struct timespec ts;
	struct timespec now;
	int64_t delta;

	/*
	 * Skip weston_output_finish_frame() if the repaint state machine was
	 * reset, e.g. by calling weston_compositor_sleep().
	 */
	if (output->base.repaint_status != REPAINT_AWAITING_COMPLETION)
		return 1;

	/*
	 * The timer only has msec precision, but if the approximately hit our
	 * target, report an exact time stamp by adding to the previous frame
	 * time.
	 */
	timespec_add_nsec(&ts,&output->base.frame_time, refresh_nsec);

	/* If we are more than 1.5 ms late, report the current time instead. */
	weston_compositor_read_presentation_clock(output->base.compositor, &now);
	delta = timespec_sub_to_nsec(&now, &ts);
	if (delta > 1500000)
		ts = now;

	weston_output_finish_frame(&output->base, &ts, 0);

	return 1;
}

static int
pipewire_output_enable(struct weston_output *base)
{
	struct weston_renderer *renderer = base->compositor->renderer;
	struct pipewire_output *output = to_pipewire_output(base);
	struct pipewire_backend *backend;
	struct wl_event_loop *loop;
	int ret;
	const struct pixman_renderer_output_options options = {
		.use_shadow = true,
		.fb_size = {
			.width = output->base.width,
			.height = output->base.height,
		},
		.format = output->pixel_format,
	};

	backend = output->backend;

	ret = renderer->pixman->output_create(&output->base, &options);
	if (ret < 0)
		return ret;

	loop = wl_display_get_event_loop(backend->compositor->wl_display);
	output->finish_frame_timer = wl_event_loop_add_timer(loop,
							     finish_frame_handler,
							     output);

	ret = pipewire_output_connect(output);
	if (ret < 0)
		goto err;

	return 0;
err:
	renderer->pixman->output_destroy(&output->base);

	wl_event_source_remove(output->finish_frame_timer);

	return ret;
}

static int
pipewire_output_disable(struct weston_output *base)
{
	struct weston_renderer *renderer = base->compositor->renderer;
	struct pipewire_output *output = to_pipewire_output(base);

	if (!output->base.enabled)
		return 0;

	pw_stream_disconnect(output->stream);

	renderer->pixman->output_destroy(&output->base);

	wl_event_source_remove(output->finish_frame_timer);

	return 0;
}

static void
pipewire_output_destroy(struct weston_output *base)
{
	struct pipewire_output *output = to_pipewire_output(base);

	assert(output);

	pipewire_output_disable(&output->base);
	weston_output_release(&output->base);

	pw_stream_destroy(output->stream);

	free(output);
}

static void
pipewire_output_stream_state_changed(void *data, enum pw_stream_state old,
				     enum pw_stream_state state,
				     const char *error_message)
{
	struct pipewire_output *output = data;

	pipewire_output_debug(output, "state changed: %s -> %s",
			      pw_stream_state_as_string(old),
			      pw_stream_state_as_string(state));

	switch (state) {
	case PW_STREAM_STATE_STREAMING:
		/* Repaint required to push the frame to the new consumer. */
		weston_output_damage(&output->base);
		weston_output_schedule_repaint(&output->base);
		break;
	default:
		break;
	}
}

static void
pipewire_output_stream_param_changed(void *data, uint32_t id,
				     const struct spa_pod *format)
{
	struct pipewire_output *output = data;
	uint8_t buffer[1024];
	struct spa_pod_builder builder =
		SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
	const struct spa_pod *params[2];
	struct spa_video_info video_info;
	int32_t width;
	int32_t height;
	int32_t stride;
	int32_t size;

	if (!format || id != SPA_PARAM_Format)
		return;

	if (spa_format_parse(format, &video_info.media_type,
			     &video_info.media_subtype) < 0)
		return;
	if (video_info.media_type != SPA_MEDIA_TYPE_video ||
	    video_info.media_subtype != SPA_MEDIA_SUBTYPE_raw)
		return;

	spa_format_video_raw_parse(format, &video_info.info.raw);

	pipewire_output_debug(output, "param changed: %dx%d@(%d/%d) (%s)",
			      video_info.info.raw.size.width,
			      video_info.info.raw.size.height,
			      video_info.info.raw.max_framerate.num,
			      video_info.info.raw.max_framerate.denom,
			      spa_debug_type_find_short_name(spa_type_video_format,
				      video_info.info.raw.format));

	width = video_info.info.raw.size.width;
	height = video_info.info.raw.size.height;
	stride = width * output->pixel_format->bpp / 8;
	size = height * stride;


	params[0] = spa_pod_builder_add_object(&builder,
		SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
		SPA_PARAM_BUFFERS_size, SPA_POD_Int(size),
		SPA_PARAM_BUFFERS_stride, SPA_POD_Int(stride),
		SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(4, 2, 8));

	params[1] = spa_pod_builder_add_object(&builder,
		SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
		SPA_PARAM_META_type, SPA_POD_Id(SPA_META_Header),
		SPA_PARAM_META_size, SPA_POD_Int(sizeof(struct spa_meta_header)));

	pw_stream_update_params(output->stream, params, 2);
}

static void
pipewire_output_stream_add_buffer(void *data, struct pw_buffer *buffer)
{
	struct pipewire_output *output = data;
	struct weston_compositor *ec = output->base.compositor;
	const struct pixman_renderer_interface *pixman = ec->renderer->pixman;
	struct pipewire_frame_data *frame_data;
	const struct pixel_format_info *format;
	unsigned int width;
	unsigned int height;
	unsigned int stride;
	void *ptr;

	pipewire_output_debug(output, "add buffer: %p", buffer);

	frame_data = xzalloc(sizeof *frame_data);
	buffer->user_data = frame_data;

	format = output->pixel_format;
	width = output->base.width;
	height = output->base.height;
	stride = width * format->bpp / 8;
	ptr = buffer->buffer->datas[0].data;

	frame_data->renderbuffer = pixman->create_image_from_ptr(&output->base,
								 format, width,
								 height, ptr,
								 stride);
}

static void
pipewire_output_stream_remove_buffer(void *data, struct pw_buffer *buffer)
{
	struct pipewire_output *output = data;
	struct pipewire_frame_data *frame_data = buffer->user_data;

	pipewire_output_debug(output, "remove buffer: %p", buffer);

	weston_renderbuffer_unref(frame_data->renderbuffer);
	free(frame_data);
}

static const struct pw_stream_events stream_events = {
	PW_VERSION_STREAM_EVENTS,
	.state_changed = pipewire_output_stream_state_changed,
	.param_changed = pipewire_output_stream_param_changed,
	.add_buffer = pipewire_output_stream_add_buffer,
	.remove_buffer = pipewire_output_stream_remove_buffer,
};

static struct weston_output *
pipewire_create_output(struct weston_backend *backend, const char *name)
{
	struct pipewire_output *output;
	struct pipewire_backend *b = container_of(backend, struct pipewire_backend, base);
	struct pw_properties *props;

	output = zalloc(sizeof *output);
	if (output == NULL)
		return NULL;

	weston_output_init(&output->base, b->compositor, name);

	output->base.destroy = pipewire_output_destroy;
	output->base.disable = pipewire_output_disable;
	output->base.enable = pipewire_output_enable;
	output->base.attach_head = NULL;

	weston_compositor_add_pending_output(&output->base, b->compositor);

	output->backend = b;
	output->pixel_format = b->pixel_format;

	props = pw_properties_new(NULL, NULL);
	pw_properties_setf(props, PW_KEY_NODE_NAME, "weston.%s", name);

	output->stream = pw_stream_new(b->core, name, props);
	if (!output->stream) {
		weston_log("Cannot initialize PipeWire stream\n");
		free(output);
		return NULL;
	}

	pw_stream_add_listener(output->stream, &output->stream_listener,
			       &stream_events, output);

	return &output->base;
}

static void
pipewire_destroy(struct weston_backend *base)
{
	struct pipewire_backend *b = container_of(base, struct pipewire_backend, base);
	struct weston_compositor *ec = b->compositor;
	struct weston_head *head, *next;

	weston_log_scope_destroy(b->debug);
	b->debug = NULL;

	weston_compositor_shutdown(ec);

	pw_loop_leave(b->loop);
	pw_loop_destroy(b->loop);
	wl_event_source_remove(b->loop_source);

	wl_list_for_each_safe(head, next, &ec->head_list, compositor_link)
		pipewire_head_destroy(head);

	free(b);
}

static void
pipewire_head_create(struct weston_backend *backend, const char *name,
		     const struct pipewire_config *config)
{
	struct pipewire_backend *b = to_pipewire_backend(backend);
	struct pipewire_head *head;
	struct weston_head *base;

	head = xzalloc(sizeof *head);

	head->config = *config;

	base = &head->base;
	weston_head_init(base, name);
	weston_head_set_monitor_strings(base, "PipeWire", name, NULL);
	weston_head_set_physical_size(base, config->width, config->height);

	base->backend = &b->base;

	weston_head_set_connection_status(base, true);
	weston_compositor_add_head(b->compositor, base);
}

static void
pipewire_head_destroy(struct weston_head *base)
{
	struct pipewire_head *head = to_pipewire_head(base);

	if (!head)
		return;

	weston_head_release(&head->base);
	free(head);
}

static int
pipewire_output_start_repaint_loop(struct weston_output *output)
{
	struct timespec ts;

	weston_compositor_read_presentation_clock(output->compositor, &ts);
	weston_output_finish_frame(output, &ts, WP_PRESENTATION_FEEDBACK_INVALID);

	return 0;
}

static void
pipewire_submit_buffer(struct pipewire_output *output,
		       struct pw_buffer *buffer)
{
	struct spa_buffer *spa_buffer;
	struct spa_meta_header *h;
	const struct pixel_format_info *pixel_format;
	unsigned int stride;
	size_t size;

	pixel_format = output->pixel_format;
	stride = output->base.width * pixel_format->bpp / 8;
	size = output->base.height * stride;

	spa_buffer = buffer->buffer;

	if ((h = spa_buffer_find_meta_data(spa_buffer, SPA_META_Header,
				     sizeof(struct spa_meta_header)))) {
		struct timespec ts;

		clock_gettime(CLOCK_MONOTONIC, &ts);
		h->pts = SPA_TIMESPEC_TO_NSEC(&ts);
		h->flags = 0;
		h->seq = output->seq;
		h->dts_offset = 0;
	}

	spa_buffer->datas[0].chunk->offset = 0;
	spa_buffer->datas[0].chunk->stride = stride;
	spa_buffer->datas[0].chunk->size = size;

	pipewire_output_debug(output, "queue buffer: %p (seq %d)",
			      buffer, output->seq);
	pw_stream_queue_buffer(output->stream, buffer);

	output->seq++;
}

static void
pipewire_output_arm_timer(struct pipewire_output *output)
{
	struct weston_compositor *ec = output->base.compositor;
	struct timespec now;
	struct timespec target;
	int refresh_nsec = millihz_to_nsec(output->base.current_mode->refresh);
	int refresh_msec = refresh_nsec / 1000000;
	int next_frame_delta;

	weston_compositor_read_presentation_clock(ec, &now);
	timespec_add_nsec(&target, &output->base.frame_time, refresh_nsec);

	next_frame_delta = (int)timespec_sub_to_msec(&target, &now);
	if (next_frame_delta < 1)
		next_frame_delta = 1;
	if (next_frame_delta > refresh_msec)
		next_frame_delta = refresh_msec;

	wl_event_source_timer_update(output->finish_frame_timer, next_frame_delta);
}

static int
pipewire_output_repaint(struct weston_output *base, pixman_region32_t *damage)
{
	struct pipewire_output *output = to_pipewire_output(base);
	struct weston_compositor *ec = output->base.compositor;
	struct pw_buffer *buffer;
	struct pipewire_frame_data *frame_data;

	assert(output);

	if (pw_stream_get_state(output->stream, NULL) != PW_STREAM_STATE_STREAMING)
		goto out;

	if (!pixman_region32_not_empty(damage))
		goto out;

	buffer = pw_stream_dequeue_buffer(output->stream);
	if (!buffer) {
		weston_log("Failed to dequeue PipeWire buffer\n");
		goto out;
	}
	pipewire_output_debug(output, "dequeued buffer: %p", buffer);

	frame_data = buffer->user_data;
	ec->renderer->repaint_output(&output->base, damage, frame_data->renderbuffer);

	pipewire_submit_buffer(output, buffer);

	pixman_region32_subtract(&ec->primary_plane.damage,
				 &ec->primary_plane.damage, damage);
out:

	pipewire_output_arm_timer(output);

	return 0;
}

static struct weston_mode *
pipewire_insert_new_mode(struct weston_output *output,
			 int width, int height, int rate)
{
	struct weston_mode *mode;

	mode = zalloc(sizeof *mode);
	if (!mode)
		return NULL;
	mode->width = width;
	mode->height = height;
	mode->refresh = rate;
	wl_list_insert(&output->mode_list, &mode->link);

	return mode;
}

static struct weston_mode *
pipewire_ensure_matching_mode(struct weston_output *output, struct weston_mode *target)
{
	struct weston_mode *local;

	wl_list_for_each(local, &output->mode_list, link) {
		if ((local->width == target->width) && (local->height == target->height))
			return local;
	}

	return pipewire_insert_new_mode(output,
					target->width, target->height,
					target->refresh);
}

static int
pipewire_switch_mode(struct weston_output *base, struct weston_mode *target_mode)
{
	struct pipewire_output *output = to_pipewire_output(base);
	struct weston_mode *local_mode;
	struct weston_size fb_size;

	assert(output);

	local_mode = pipewire_ensure_matching_mode(base, target_mode);

	base->current_mode->flags &= ~WL_OUTPUT_MODE_CURRENT;

	base->current_mode = base->native_mode = local_mode;
	base->current_mode->flags |= WL_OUTPUT_MODE_CURRENT;

	fb_size.width = target_mode->width;
	fb_size.height = target_mode->height;

	weston_renderer_resize_output(base, &fb_size, NULL);

	return 0;
}

static int
pipewire_output_set_size(struct weston_output *base, int width, int height)
{
	struct pipewire_output *output = to_pipewire_output(base);
	struct weston_head *head;
	struct pipewire_head *pw_head;
	struct weston_mode *current_mode;
	struct weston_mode init_mode;
	int framerate = -1;

	/* We can only be called once. */
	assert(!output->base.current_mode);

	wl_list_for_each(head, &output->base.head_list, output_link) {
		pw_head = to_pipewire_head(head);

		if (width == -1)
			width = pw_head->config.width;
		if (height == -1)
			height = pw_head->config.height;
		framerate = pw_head->config.framerate;
	}
	if (framerate == -1 || width == -1 || height == -1)
		return -1;

	init_mode.width = width;
	init_mode.height = height;
	init_mode.refresh = framerate * 1000;

	current_mode = pipewire_ensure_matching_mode(&output->base, &init_mode);
	current_mode->flags = WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;

	output->base.current_mode = output->base.native_mode = current_mode;

	output->base.start_repaint_loop = pipewire_output_start_repaint_loop;
	output->base.repaint = pipewire_output_repaint;
	output->base.assign_planes = NULL;
	output->base.set_backlight = NULL;
	output->base.set_dpms = NULL;
	output->base.switch_mode = pipewire_switch_mode;

	return 0;
}

static int
parse_gbm_format(const char *gbm_format,
		 const struct pixel_format_info *default_format,
		 const struct pixel_format_info **format)
{
	if (gbm_format == NULL) {
		*format = default_format;
		return 0;
	}

	*format = pixel_format_get_info_by_drm_name(gbm_format);
	if (!*format) {
		weston_log("Invalid output format %s: using default format (%s)\n",
			   gbm_format, default_format->drm_format_name);
		*format = default_format;
	}

	return 0;
}

static void
pipewire_output_set_gbm_format(struct weston_output *base, const char *gbm_format)
{
	struct pipewire_output *output = to_pipewire_output(base);
	struct pipewire_backend *backend = output->backend;

	parse_gbm_format(gbm_format, backend->pixel_format,
			 &output->pixel_format);
}

static const struct weston_pipewire_output_api api = {
	pipewire_head_create,
	pipewire_output_set_size,
	pipewire_output_set_gbm_format,
};

static int
weston_pipewire_loop_handler(int fd, uint32_t mask, void *data)
{
	struct pipewire_backend *pipewire = data;
	int ret;

	ret = pw_loop_iterate(pipewire->loop, 0);
	if (ret < 0)
		weston_log("pipewire_loop_iterate failed: %s\n",
			   spa_strerror(ret));

	return 0;
}

static void
weston_pipewire_error(void *data, uint32_t id, int seq, int res,
		      const char *error)
{
	weston_log("PipeWire remote error: %s\n", error);
}

static const struct pw_core_events core_events = {
	PW_VERSION_CORE_EVENTS,
	.error = weston_pipewire_error,
};

static int
weston_pipewire_init(struct pipewire_backend *backend)
{
	struct wl_event_loop *loop;

	pw_init(NULL, NULL);

	backend->loop = pw_loop_new(NULL);
	if (!backend->loop)
		return -1;

	pw_loop_enter(backend->loop);

	backend->context = pw_context_new(backend->loop, NULL, 0);
	if (!backend->context) {
		weston_log("Failed to create PipeWire context\n");
		goto err_loop;
	}

	backend->core = pw_context_connect(backend->context, NULL, 0);
	if (!backend->core) {
		weston_log("Failed to connect to PipeWire context\n");
		goto err_context;
	}

	pw_core_add_listener(backend->core,
			     &backend->core_listener, &core_events, backend);

	loop = wl_display_get_event_loop(backend->compositor->wl_display);
	backend->loop_source =
		wl_event_loop_add_fd(loop, pw_loop_get_fd(backend->loop),
				     WL_EVENT_READABLE,
				     weston_pipewire_loop_handler,
				     backend);

	return 0;

err_context:
	pw_context_destroy(backend->context);
	backend->context = NULL;
err_loop:
	pw_loop_leave(backend->loop);
	pw_loop_destroy(backend->loop);
	backend->loop = NULL;

	return -1;
}

static void
pipewire_backend_create_outputs(struct pipewire_backend *backend,
				int num_outputs)
{
	char name[32] = "pipewire";
	int i;

	for (i = 0; i < num_outputs; i++) {
		if (num_outputs > 1)
			snprintf(name, sizeof name, "pipewire-%u", i);
		pipewire_head_create(&backend->base, name, &default_config);
	}
}

static struct pipewire_backend *
pipewire_backend_create(struct weston_compositor *compositor,
			struct weston_pipewire_backend_config *config)
{
	struct pipewire_backend *backend;
	int ret;

	backend = zalloc(sizeof *backend);
	if (backend == NULL)
		return NULL;

	backend->compositor = compositor;
	backend->base.destroy = pipewire_destroy;
	backend->base.create_output = pipewire_create_output;

	compositor->backend = &backend->base;

	if (weston_compositor_set_presentation_clock_software(compositor) < 0)
		goto err_compositor;

	switch (config->renderer) {
	case WESTON_RENDERER_AUTO:
	case WESTON_RENDERER_PIXMAN:
		break;
	default:
		weston_log("Unsupported renderer requested\n");
		goto err_compositor;
	}

	if (weston_compositor_init_renderer(compositor, WESTON_RENDERER_PIXMAN,
					    NULL) < 0)
		goto err_compositor;

	compositor->capabilities |= WESTON_CAP_ARBITRARY_MODES;

	ret = weston_pipewire_init(backend);
	if (ret < 0) {
		weston_log("Failed to initialize PipeWire\n");
		goto err_compositor;
	}

	ret = weston_plugin_api_register(compositor, WESTON_PIPEWIRE_OUTPUT_API_NAME,
					 &api, sizeof(api));
	if (ret < 0) {
		weston_log("Failed to register PipeWire output API\n");
		goto err_compositor;
	}

	parse_gbm_format(config->gbm_format,
			 pixel_format_get_info(DRM_FORMAT_XRGB8888),
			 &backend->pixel_format);

	pipewire_backend_create_outputs(backend, config->num_outputs);

	return backend;

err_compositor:
	weston_compositor_shutdown(compositor);

	free(backend);
	return NULL;
}

static void
config_init_to_defaults(struct weston_pipewire_backend_config *config)
{
	config->gbm_format = "xrgb8888";
	config->num_outputs = 1;
}

WL_EXPORT int
weston_backend_init(struct weston_compositor *compositor,
		    struct weston_backend_config *config_base)
{
	struct pipewire_backend *backend;
	struct weston_pipewire_backend_config config = {{ 0, }};

	weston_log("Initializing PipeWire backend\n");

	if (config_base == NULL ||
	    config_base->struct_version != WESTON_PIPEWIRE_BACKEND_CONFIG_VERSION ||
	    config_base->struct_size > sizeof(struct weston_pipewire_backend_config)) {
		weston_log("PipeWire backend config structure is invalid\n");
		return -1;
	}

	config_init_to_defaults(&config);
	memcpy(&config, config_base, config_base->struct_size);

	backend = pipewire_backend_create(compositor, &config);
	if (backend == NULL)
		return -1;

	backend->debug =
		weston_compositor_add_log_scope(compositor, "pipewire",
						"Debug messages from pipewire backend\n",
						NULL, NULL, NULL);

	return 0;
}
