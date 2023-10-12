/*
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

#include <libweston/pipewire-plugin.h>
#include "backend.h"
#include "libweston-internal.h"
#include "shared/timespec-util.h"
#include "shared/string-helpers.h"
#include <libweston/backend-drm.h>
#include <libweston/weston-log.h>

#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

#include <pipewire/pipewire.h>

#include <spa/param/format-utils.h>
#include <spa/param/video/format-utils.h>
#include <spa/utils/defs.h>

#include <spa/buffer/meta.h>
#include <spa/utils/result.h>

#define PROP_RANGE(min, max) 2, (min), (max)

struct weston_pipewire {
	struct weston_compositor *compositor;
	struct wl_list output_list;
	struct wl_listener destroy_listener;
	const struct weston_drm_virtual_output_api *virtual_output_api;

	struct weston_log_scope *debug;

	struct pw_loop *loop;
	struct wl_event_source *loop_source;

	struct pw_context *context;
	struct pw_core *core;
	struct pw_type *t;
	struct spa_hook core_listener;
};

struct pipewire_output {
	struct weston_output *output;
	int (*saved_enable)(struct weston_output *output);
	int (*saved_disable)(struct weston_output *output);
	int (*saved_start_repaint_loop)(struct weston_output *output);

	struct weston_head *head;

	struct weston_pipewire *pipewire;

	uint32_t seq;
	struct pw_stream *stream;
	struct spa_hook stream_listener;

	struct spa_video_info_raw video_format;

	struct wl_event_source *finish_frame_timer;
	struct wl_list link;
	bool submitted_frame;
	enum dpms_enum dpms;
};

struct pipewire_frame_data {
	struct pipewire_output *output;
	int fd;
	int stride;
	struct drm_fb *drm_buffer;
	int fence_sync_fd;
	struct wl_event_source *fence_sync_event_source;
};

static void
pipewire_debug_impl(struct weston_pipewire *pipewire,
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
		fprintf(fp, "[%s]", output->output->name);

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
	pipewire_debug_impl(output->pipewire, output, fmt, ap);
	va_end(ap);
}

static struct weston_pipewire *
weston_pipewire_get(struct weston_compositor *compositor);

static struct pipewire_output *
lookup_pipewire_output(struct weston_output *base_output)
{
	struct weston_compositor *c = base_output->compositor;
	struct weston_pipewire *pipewire = weston_pipewire_get(c);
	struct pipewire_output *output;

	/* XXX: This could happen on the compositor shutdown path with our
	 * destroy listener being removed, and pipewire_output_destroy() being
	 * called as a virtual destructor.
	 *
	 * See https://gitlab.freedesktop.org/wayland/weston/-/issues/591 for
	 * an alternative to the shutdown sequence.
	 */
	if (!pipewire)
		return NULL;

	wl_list_for_each(output, &pipewire->output_list, link) {
		if (output->output == base_output)
			return output;
	}
	return NULL;
}

static void
pipewire_output_handle_frame(struct pipewire_output *output, int fd,
			     int stride, struct drm_fb *drm_buffer)
{
	const struct weston_drm_virtual_output_api *api =
		output->pipewire->virtual_output_api;
	size_t size = output->output->height * stride;
	struct pw_buffer *buffer;
	struct spa_buffer *spa_buffer;
	struct spa_meta_header *h;
	void *ptr;

	if (pw_stream_get_state(output->stream, NULL) !=
	    PW_STREAM_STATE_STREAMING)
		goto out;

	buffer = pw_stream_dequeue_buffer(output->stream);
	if (!buffer) {
		weston_log("Failed to dequeue a pipewire buffer\n");
		goto out;
	}

	spa_buffer = buffer->buffer;

	if ((h = spa_buffer_find_meta_data(spa_buffer, SPA_META_Header,
				     sizeof(struct spa_meta_header)))) {
		struct timespec ts;

		clock_gettime(CLOCK_MONOTONIC, &ts);
		h->pts = SPA_TIMESPEC_TO_NSEC(&ts);
		h->flags = 0;
		h->seq = output->seq++;
		h->dts_offset = 0;
	}

	ptr = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	memcpy(spa_buffer->datas[0].data, ptr, size);
	munmap(ptr, size);

	spa_buffer->datas[0].chunk->offset = 0;
	spa_buffer->datas[0].chunk->stride = stride;
	spa_buffer->datas[0].chunk->size = spa_buffer->datas[0].maxsize;

	pipewire_output_debug(output, "push frame");
	pw_stream_queue_buffer(output->stream, buffer);

out:
	close(fd);
	output->submitted_frame = true;
	api->buffer_released(drm_buffer);
}

static int
pipewire_output_fence_sync_handler(int fd, uint32_t mask, void *data)
{
	struct pipewire_frame_data *frame_data = data;
	struct pipewire_output *output = frame_data->output;

	pipewire_output_handle_frame(output, frame_data->fd, frame_data->stride,
				     frame_data->drm_buffer);

	wl_event_source_remove(frame_data->fence_sync_event_source);
	close(frame_data->fence_sync_fd);
	free(frame_data);

	return 0;
}

static int
pipewire_output_submit_frame(struct weston_output *base_output, int fd,
			     int stride, struct drm_fb *drm_buffer)
{
	struct pipewire_output *output = lookup_pipewire_output(base_output);
	struct weston_pipewire *pipewire = output->pipewire;
	const struct weston_drm_virtual_output_api *api =
		pipewire->virtual_output_api;
	struct wl_event_loop *loop;
	struct pipewire_frame_data *frame_data;
	int fence_sync_fd;

	pipewire_output_debug(output, "submit frame: fd = %d drm_fb = %p",
			      fd, drm_buffer);

	fence_sync_fd = api->get_fence_sync_fd(output->output);
	if (fence_sync_fd == -1) {
		pipewire_output_handle_frame(output, fd, stride, drm_buffer);
		return 0;
	}

	frame_data = zalloc(sizeof *frame_data);
	if (!frame_data) {
		close(fence_sync_fd);
		pipewire_output_handle_frame(output, fd, stride, drm_buffer);
		return 0;
	}

	loop = wl_display_get_event_loop(pipewire->compositor->wl_display);

	frame_data->output = output;
	frame_data->fd = fd;
	frame_data->stride = stride;
	frame_data->drm_buffer = drm_buffer;
	frame_data->fence_sync_fd = fence_sync_fd;
	frame_data->fence_sync_event_source =
		wl_event_loop_add_fd(loop, frame_data->fence_sync_fd,
				     WL_EVENT_READABLE,
				     pipewire_output_fence_sync_handler,
				     frame_data);

	return 0;
}

static void
pipewire_output_timer_update(struct pipewire_output *output)
{
	int64_t msec;
	int32_t refresh;

	if (pw_stream_get_state(output->stream, NULL) ==
	    PW_STREAM_STATE_STREAMING)
		refresh = output->output->current_mode->refresh;
	else
		refresh = 1000;

	msec = millihz_to_nsec(refresh) / 1000000;
	wl_event_source_timer_update(output->finish_frame_timer, msec);
}

static int
pipewire_output_finish_frame_handler(void *data)
{
	struct pipewire_output *output = data;
	const struct weston_drm_virtual_output_api *api
		= output->pipewire->virtual_output_api;
	struct timespec now;

	if (output->submitted_frame) {
		struct weston_compositor *c = output->pipewire->compositor;
		output->submitted_frame = false;
		weston_compositor_read_presentation_clock(c, &now);
		api->finish_frame(output->output, &now, 0);
	}

	if (output->dpms == WESTON_DPMS_ON)
		pipewire_output_timer_update(output);
	else
		wl_event_source_timer_update(output->finish_frame_timer, 0);

	return 0;
}

static void
pipewire_output_destroy(struct weston_output *base_output)
{
	struct pipewire_output *output = lookup_pipewire_output(base_output);
	struct weston_mode *mode, *next;

	if (!output)
		return;

	weston_head_release(output->head);

	wl_list_for_each_safe(mode, next, &base_output->mode_list, link) {
		wl_list_remove(&mode->link);
		free(mode);
	}

	pw_stream_destroy(output->stream);

	wl_list_remove(&output->link);
	free(output->head);
	free(output);
}

static int
pipewire_output_start_repaint_loop(struct weston_output *base_output)
{
	struct pipewire_output *output = lookup_pipewire_output(base_output);

	pipewire_output_debug(output, "start repaint loop");
	output->saved_start_repaint_loop(base_output);

	pipewire_output_timer_update(output);

	return 0;
}

static void
pipewire_set_dpms(struct weston_output *base_output, enum dpms_enum level)
{
	struct pipewire_output *output = lookup_pipewire_output(base_output);

	if (output->dpms == level)
		return;

	output->dpms = level;
	pipewire_output_finish_frame_handler(output);
}

static int
pipewire_output_connect(struct pipewire_output *output)
{
	uint8_t buffer[1024];
	struct spa_pod_builder builder =
		SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
	const struct spa_pod *params[1];
	int frame_rate = output->output->current_mode->refresh / 1000;
	int width = output->output->width;
	int height = output->output->height;
	int ret;

	params[0] = spa_pod_builder_add_object(&builder,
		SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
		SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
		SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
		SPA_FORMAT_VIDEO_format, SPA_POD_Id(SPA_VIDEO_FORMAT_BGRx),
		SPA_FORMAT_VIDEO_size, SPA_POD_Rectangle(&SPA_RECTANGLE(width, height)),
		SPA_FORMAT_VIDEO_framerate, SPA_POD_Fraction(&SPA_FRACTION (0, 1)),
		SPA_FORMAT_VIDEO_maxFramerate,
		SPA_POD_CHOICE_RANGE_Fraction(&SPA_FRACTION(frame_rate, 1),
			&SPA_FRACTION(1, 1),
			&SPA_FRACTION(frame_rate, 1)));

	ret = pw_stream_connect(output->stream, PW_DIRECTION_OUTPUT, SPA_ID_INVALID,
				(PW_STREAM_FLAG_DRIVER |
				 PW_STREAM_FLAG_MAP_BUFFERS),
				params, 1);
	if (ret != 0) {
		weston_log("Failed to connect pipewire stream: %s",
			   spa_strerror(ret));
		return -1;
	}

	return 0;
}

static int
pipewire_output_enable(struct weston_output *base_output)
{
	struct pipewire_output *output = lookup_pipewire_output(base_output);
	struct weston_compositor *c = base_output->compositor;
	const struct weston_drm_virtual_output_api *api
		= output->pipewire->virtual_output_api;
	struct wl_event_loop *loop;
	int ret;

	api->set_submit_frame_cb(base_output, pipewire_output_submit_frame);

	ret = pipewire_output_connect(output);
	if (ret < 0)
		return ret;

	ret = output->saved_enable(base_output);
	if (ret < 0)
		return ret;

	output->saved_start_repaint_loop = base_output->start_repaint_loop;
	base_output->start_repaint_loop = pipewire_output_start_repaint_loop;
	base_output->set_dpms = pipewire_set_dpms;

	loop = wl_display_get_event_loop(c->wl_display);
	output->finish_frame_timer =
		wl_event_loop_add_timer(loop,
					pipewire_output_finish_frame_handler,
					output);
	output->dpms = WESTON_DPMS_ON;

	return 0;
}

static int
pipewire_output_disable(struct weston_output *base_output)
{
	struct pipewire_output *output = lookup_pipewire_output(base_output);

	wl_event_source_remove(output->finish_frame_timer);

	pw_stream_disconnect(output->stream);

	return output->saved_disable(base_output);
}

static void
pipewire_output_stream_state_changed(void *data, enum pw_stream_state old,
				     enum pw_stream_state state,
				     const char *error_message)
{
	struct pipewire_output *output = data;

	pipewire_output_debug(output, "state changed %s -> %s",
			      pw_stream_state_as_string(old),
			      pw_stream_state_as_string(state));

	switch (state) {
	case PW_STREAM_STATE_STREAMING:
		weston_output_schedule_repaint(output->output);
		break;
	default:
		break;
	}
}

static void
pipewire_output_stream_param_changed(void *data, uint32_t id, const struct spa_pod *format)
{
	struct pipewire_output *output = data;
	uint8_t buffer[1024];
	struct spa_pod_builder builder =
		SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
	const struct spa_pod *params[2];
	int32_t width, height, stride, size;
	const int bpp = 4;

	if (!format) {
		pipewire_output_debug(output, "format = None");
		pw_stream_update_params(output->stream, NULL, 0);
		return;
	}

	spa_format_video_raw_parse(format, &output->video_format);

	width = output->video_format.size.width;
	height = output->video_format.size.height;
	stride = SPA_ROUND_UP_N(width * bpp, 4);
	size = height * stride;

	pipewire_output_debug(output, "format = %dx%d", width, height);

	params[0] = spa_pod_builder_add_object(&builder,
		SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
		SPA_PARAM_BUFFERS_size, SPA_POD_Int(size),
		SPA_PARAM_BUFFERS_stride, SPA_POD_Int(stride),
		SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(4, 2, 8),
		SPA_PARAM_BUFFERS_align, SPA_POD_Int(16));

	params[1] = spa_pod_builder_add_object(&builder,
		SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
		SPA_PARAM_META_type, SPA_POD_Id(SPA_META_Header),
		SPA_PARAM_META_size, SPA_POD_Int(sizeof(struct spa_meta_header)));

	pw_stream_update_params(output->stream, params, 2);
}

static const struct pw_stream_events stream_events = {
	PW_VERSION_STREAM_EVENTS,
	.state_changed = pipewire_output_stream_state_changed,
	.param_changed = pipewire_output_stream_param_changed,
};

static struct weston_output *
pipewire_output_create(struct weston_compositor *c, char *name)
{
	struct weston_pipewire *pipewire = weston_pipewire_get(c);
	struct pipewire_output *output;
	struct weston_head *head;
	const struct weston_drm_virtual_output_api *api;
	const char *make = "Weston";
	const char *model = "Virtual Display";
	const char *serial_number = "unknown";
	const char *connector_name = "pipewire";
	char *remoting_name;

	if (!name || !strlen(name))
		return NULL;

	api = pipewire->virtual_output_api;

	output = zalloc(sizeof *output);
	if (!output)
		return NULL;

	head = zalloc(sizeof *head);
	if (!head)
		goto err;

	output->stream = pw_stream_new(pipewire->core, name, NULL);
	if (!output->stream) {
		weston_log("Cannot initialize pipewire stream\n");
		goto err;
	}

	pw_stream_add_listener(output->stream, &output->stream_listener,
			       &stream_events, output);

	output->output = api->create_output(c, name, pipewire_output_destroy);
	if (!output->output) {
		weston_log("Cannot create virtual output\n");
		goto err;
	}

	output->saved_enable = output->output->enable;
	output->output->enable = pipewire_output_enable;
	output->saved_disable = output->output->disable;
	output->output->disable = pipewire_output_disable;
	output->pipewire = pipewire;
	wl_list_insert(pipewire->output_list.prev, &output->link);

	str_printf(&remoting_name, "%s-%s", connector_name, name);
	weston_head_init(head, remoting_name);
	weston_head_set_subpixel(head, WL_OUTPUT_SUBPIXEL_NONE);
	weston_head_set_monitor_strings(head, make, model, serial_number);
	head->compositor = c;
	output->head = head;

	weston_output_attach_head(output->output, head);

	free(remoting_name);
	pipewire_output_debug(output, "created");

	return output->output;
err:
	if (output->stream)
		pw_stream_destroy(output->stream);
	if (head)
		free(head);
	free(output);
	return NULL;
}

static bool
pipewire_output_is_pipewire(struct weston_output *output)
{
	return lookup_pipewire_output(output) != NULL;
}

static int
pipewire_output_set_mode(struct weston_output *base_output, const char *modeline)
{
	struct pipewire_output *output = lookup_pipewire_output(base_output);
	const struct weston_drm_virtual_output_api *api =
		output->pipewire->virtual_output_api;
	struct weston_mode *mode;
	int n, width, height, refresh = 0;

	if (output == NULL) {
		weston_log("Output is not pipewire.\n");
		return -1;
	}

	if (!modeline)
		return -1;

	n = sscanf(modeline, "%dx%d@%d", &width, &height, &refresh);
	if (n != 2 && n != 3)
		return -1;

	if (pw_stream_get_state(output->stream, NULL) !=
	    PW_STREAM_STATE_UNCONNECTED) {
		return -1;
	}

	mode = zalloc(sizeof *mode);
	if (!mode)
		return -1;

	pipewire_output_debug(output, "mode = %dx%d@%d", width, height, refresh);

	mode->flags = WL_OUTPUT_MODE_CURRENT;
	mode->width = width;
	mode->height = height;
	mode->refresh = (refresh ? refresh : 60) * 1000LL;

	wl_list_insert(base_output->mode_list.prev, &mode->link);

	base_output->current_mode = mode;

	api->set_gbm_format(base_output, "XRGB8888");

	return 0;
}

static void
pipewire_output_set_seat(struct weston_output *output, const char *seat)
{
}

static void
weston_pipewire_destroy(struct wl_listener *l, void *data)
{
	struct weston_pipewire *pipewire =
		wl_container_of(l, pipewire, destroy_listener);
	struct pipewire_output *p_output, *p_output_next;

	weston_log_scope_destroy(pipewire->debug);
	pipewire->debug = NULL;

	wl_list_for_each_safe(p_output, p_output_next, &pipewire->output_list, link)
		pipewire_output_destroy(p_output->output);

	wl_event_source_remove(pipewire->loop_source);
	pw_loop_leave(pipewire->loop);
	pw_loop_destroy(pipewire->loop);

	free(pipewire);
}

static struct weston_pipewire *
weston_pipewire_get(struct weston_compositor *compositor)
{
	struct wl_listener *listener;
	struct weston_pipewire *pipewire;

	listener = wl_signal_get(&compositor->destroy_signal,
				 weston_pipewire_destroy);
	if (!listener)
		return NULL;

	pipewire = wl_container_of(listener, pipewire, destroy_listener);
	return pipewire;
}

static int
weston_pipewire_loop_handler(int fd, uint32_t mask, void *data)
{
	struct weston_pipewire *pipewire = data;
	int ret;

	ret = pw_loop_iterate(pipewire->loop, 0);
	if (ret < 0)
		weston_log("pipewire_loop_iterate failed: %s",
			   spa_strerror(ret));

	return 0;
}

static void
weston_pipewire_error(void *data, uint32_t id, int seq, int res,
			      const char *error)
{
	weston_log("pipewire remote error: %s\n", error);
}

static const struct pw_core_events core_events = {
	PW_VERSION_CORE_EVENTS,
	.error = weston_pipewire_error,
};

static int
weston_pipewire_init(struct weston_pipewire *pipewire)
{
	struct wl_event_loop *loop;

	pw_init(NULL, NULL);

	pipewire->loop = pw_loop_new(NULL);
	if (!pipewire->loop)
		return -1;

	pw_loop_enter(pipewire->loop);

	pipewire->context = pw_context_new(pipewire->loop, NULL, 0);
	pipewire->core = pw_context_connect(pipewire->context, NULL, 0);

	pw_core_add_listener(pipewire->core,
			       &pipewire->core_listener,
			       &core_events, pipewire);

	loop = wl_display_get_event_loop(pipewire->compositor->wl_display);
	pipewire->loop_source =
		wl_event_loop_add_fd(loop, pw_loop_get_fd(pipewire->loop),
				     WL_EVENT_READABLE,
				     weston_pipewire_loop_handler,
				     pipewire);

	return 0;
}

static const struct weston_pipewire_api pipewire_api = {
	pipewire_output_create,
	pipewire_output_is_pipewire,
	pipewire_output_set_mode,
	pipewire_output_set_seat,
};

WL_EXPORT int
weston_module_init(struct weston_compositor *compositor)
{
	int ret;
	struct weston_pipewire *pipewire;
	const struct weston_drm_virtual_output_api *api =
		weston_drm_virtual_output_get_api(compositor);

	if (!api)
		return -1;

	pipewire = zalloc(sizeof *pipewire);
	if (!pipewire)
		return -1;

	if (!weston_compositor_add_destroy_listener_once(compositor,
							 &pipewire->destroy_listener,
							 weston_pipewire_destroy)) {
		free(pipewire);
		return 0;
	}

	pipewire->virtual_output_api = api;
	pipewire->compositor = compositor;
	wl_list_init(&pipewire->output_list);

	ret = weston_plugin_api_register(compositor, WESTON_PIPEWIRE_API_NAME,
					 &pipewire_api, sizeof(pipewire_api));

	if (ret < 0) {
		weston_log("Failed to register pipewire API.\n");
		goto failed;
	}

	ret = weston_pipewire_init(pipewire);
	if (ret < 0) {
		weston_log("Failed to initialize pipewire.\n");
		goto failed;
	}

	pipewire->debug =
		weston_compositor_add_log_scope(compositor, "pipewire",
						"Debug messages from pipewire plugin\n",
						NULL, NULL, NULL);

	return 0;

failed:
	wl_list_remove(&pipewire->destroy_listener.link);
	free(pipewire);
	return -1;
}
