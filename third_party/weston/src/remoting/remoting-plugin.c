/*
 * Copyright © 2018 Renesas Electronics Corp.
 *
 * Based on vaapi-recorder by:
 *   Copyright (c) 2012 Intel Corporation. All Rights Reserved.
 *   Copyright © 2013 Intel Corporation
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
 *
 * Authors: IGEL Co., Ltd.
 */

#include "config.h"

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <gst/gst.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/gstvideometa.h>

#include <libweston/remoting-plugin.h>
#include <libweston/backend-drm.h>
#include "shared/helpers.h"
#include "shared/timespec-util.h"
#include "shared/weston-drm-fourcc.h"
#include "shared/string-helpers.h"
#include "backend.h"
#include "libweston-internal.h"

#define MAX_RETRY_COUNT	3

struct weston_remoting {
	struct weston_compositor *compositor;
	struct wl_list output_list;
	struct wl_listener destroy_listener;
	const struct weston_drm_virtual_output_api *virtual_output_api;

	GstAllocator *allocator;
};

struct remoted_gstpipe {
	int readfd;
	int writefd;
	struct wl_event_source *source;
};

/* supported gbm format list */
struct remoted_output_support_gbm_format {
	/* GBM_FORMAT_* tokens are strictly aliased with DRM_FORMAT_*, so we
	 * use the latter to avoid a dependency on GBM */
	uint32_t gbm_format;
	const char *gst_format_string;
	GstVideoFormat gst_video_format;
};

static const struct remoted_output_support_gbm_format supported_formats[] = {
	{
		.gbm_format = DRM_FORMAT_XRGB8888,
		.gst_format_string = "BGRx",
		.gst_video_format = GST_VIDEO_FORMAT_BGRx,
	}, {
		.gbm_format = DRM_FORMAT_RGB565,
		.gst_format_string = "RGB16",
		.gst_video_format = GST_VIDEO_FORMAT_RGB16,
	}, {
		.gbm_format = DRM_FORMAT_XRGB2101010,
		.gst_format_string = "r210",
		.gst_video_format = GST_VIDEO_FORMAT_r210,
	}
};

struct remoted_output {
	struct weston_output *output;
	int (*saved_enable)(struct weston_output *output);
	int (*saved_disable)(struct weston_output *output);
	int (*saved_start_repaint_loop)(struct weston_output *output);

	char *host;
	int port;
	char *gst_pipeline;
	const struct remoted_output_support_gbm_format *format;

	struct weston_head *head;

	struct weston_remoting *remoting;
	struct wl_event_source *finish_frame_timer;
	struct wl_list link;
	bool submitted_frame;
	int fence_sync_fd;
	struct wl_event_source *fence_sync_event_source;

	GstElement *pipeline;
	GstAppSrc *appsrc;
	GstBus *bus;
	struct remoted_gstpipe gstpipe;
	GstClockTime start_time;
	int retry_count;
	enum dpms_enum dpms;
};

struct mem_free_cb_data {
	struct remoted_output *output;
	struct drm_fb *output_buffer;
};

struct gst_frame_buffer_data {
	struct remoted_output *output;
	GstBuffer *buffer;
};

/* message type for pipe */
#define GSTPIPE_MSG_BUS_SYNC		1
#define GSTPIPE_MSG_BUFFER_RELEASE	2

struct gstpipe_msg_data {
	int type;
	void *data;
};

static int
remoting_gst_init(struct weston_remoting *remoting)
{
	GError *err = NULL;

	if (!gst_init_check(NULL, NULL, &err)) {
		weston_log("GStreamer initialization error: %s\n",
			   err->message);
		g_error_free(err);
		return -1;
	}

	remoting->allocator = gst_dmabuf_allocator_new();

	return 0;
}

static void
remoting_gst_deinit(struct weston_remoting *remoting)
{
	gst_object_unref(remoting->allocator);
}

static GstBusSyncReply
remoting_gst_bus_sync_handler(GstBus *bus, GstMessage *message,
			      gpointer user_data)
{
	struct remoted_gstpipe *pipe = user_data;
	struct gstpipe_msg_data msg = {
		.type = GSTPIPE_MSG_BUS_SYNC,
		.data = NULL
	};
	ssize_t ret;

	ret = write(pipe->writefd, &msg, sizeof(msg));
	if (ret != sizeof(msg))
		weston_log("ERROR: failed to write, ret=%zd, errno=%d\n",
			   ret, errno);

	return GST_BUS_PASS;
}

static int
remoting_gst_pipeline_init(struct remoted_output *output)
{
	GstCaps *caps;
	GError *err = NULL;
	GstStateChangeReturn ret;
	struct weston_mode *mode = output->output->current_mode;

	if (!output->gst_pipeline) {
		char pipeline_str[1024];
		/* TODO: use encodebin instead of jpegenc */
		snprintf(pipeline_str, sizeof(pipeline_str),
			 "rtpbin name=rtpbin "
			 "appsrc name=src ! videoconvert ! "
			 "video/x-raw,format=I420 ! jpegenc ! rtpjpegpay ! "
			 "rtpbin.send_rtp_sink_0 "
			 "rtpbin.send_rtp_src_0 ! "
			 "udpsink name=sink host=%s port=%d "
			 "rtpbin.send_rtcp_src_0 ! "
			 "udpsink host=%s port=%d sync=false async=false "
			 "udpsrc port=%d ! rtpbin.recv_rtcp_sink_0",
			 output->host, output->port, output->host,
			 output->port + 1, output->port + 2);
		output->gst_pipeline = strdup(pipeline_str);
	}
	weston_log("GST pipeline: %s\n", output->gst_pipeline);

	output->pipeline = gst_parse_launch(output->gst_pipeline, &err);
	if (!output->pipeline) {
		weston_log("Could not create gstreamer pipeline. Error: %s\n",
			   err->message);
		g_error_free(err);
		return -1;
	}

	output->appsrc = (GstAppSrc*)
		gst_bin_get_by_name(GST_BIN(output->pipeline), "src");
	if (!output->appsrc) {
		weston_log("Could not get appsrc from gstreamer pipeline\n");
		goto err;
	}

	/* check sink */
	if (!gst_bin_get_by_name(GST_BIN(output->pipeline), "sink")) {
		weston_log("Could not get sink from gstreamer pipeline\n");
		goto err;
	}

	caps = gst_caps_new_simple("video/x-raw",
				   "format", G_TYPE_STRING,
					     output->format->gst_format_string,
				   "width", G_TYPE_INT, mode->width,
				   "height", G_TYPE_INT, mode->height,
				   "framerate", GST_TYPE_FRACTION,
						mode->refresh, 1000,
				   NULL);
	if (!caps) {
		weston_log("Could not create gstreamer caps.\n");
		goto err;
	}
	g_object_set(G_OBJECT(output->appsrc),
		     "caps", caps,
		     "stream-type", 0,
		     "format", GST_FORMAT_TIME,
		     "is-live", TRUE,
		     NULL);
	gst_caps_unref(caps);

	output->bus = gst_pipeline_get_bus(GST_PIPELINE(output->pipeline));
	if (!output->bus) {
		weston_log("Could not get bus from gstreamer pipeline\n");
		goto err;
	}
	gst_bus_set_sync_handler(output->bus, remoting_gst_bus_sync_handler,
				 &output->gstpipe, NULL);

	output->start_time = 0;
	ret = gst_element_set_state(output->pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		weston_log("Couldn't set GST_STATE_PLAYING to pipeline\n");
		goto err;
	}

	return 0;

err:
	gst_object_unref(GST_OBJECT(output->pipeline));
	output->pipeline = NULL;
	return -1;
}

static void
remoting_gst_pipeline_deinit(struct remoted_output *output)
{
	if (!output->pipeline)
		return;

	gst_element_set_state(output->pipeline, GST_STATE_NULL);
	if (output->bus)
		gst_object_unref(GST_OBJECT(output->bus));
	gst_object_unref(GST_OBJECT(output->pipeline));
	output->pipeline = NULL;
}

static int
remoting_output_disable(struct weston_output *output);

static void
remoting_gst_restart(void *data)
{
	struct remoted_output *output = data;

	if (remoting_gst_pipeline_init(output) < 0) {
		weston_log("gst: Could not restart pipeline!!\n");
		remoting_output_disable(output->output);
	}
}

static void
remoting_gst_schedule_restart(struct remoted_output *output)
{
	struct wl_event_loop *loop;
	struct weston_compositor *c = output->remoting->compositor;

	loop = wl_display_get_event_loop(c->wl_display);
	wl_event_loop_add_idle(loop, remoting_gst_restart, output);
}

static void
remoting_gst_bus_message_handler(struct remoted_output *output)
{
	GstMessage *message;
	GError *error;
	gchar *debug;

	/* get message from bus queue */
	message = gst_bus_pop(output->bus);
	if (!message)
		return;

	switch (GST_MESSAGE_TYPE(message)) {
	case GST_MESSAGE_STATE_CHANGED: {
		GstState new_state;
		gst_message_parse_state_changed(message, NULL, &new_state,
						NULL);
		if (!strcmp(GST_OBJECT_NAME(message->src), "sink") &&
		    new_state == GST_STATE_PLAYING)
			output->retry_count = 0;
		break;
	}
	case GST_MESSAGE_WARNING:
		gst_message_parse_warning(message, &error, &debug);
		weston_log("gst: Warning: %s: %s\n",
			   GST_OBJECT_NAME(message->src), error->message);
		break;
	case GST_MESSAGE_ERROR:
		gst_message_parse_error(message, &error, &debug);
		weston_log("gst: Error: %s: %s\n",
			   GST_OBJECT_NAME(message->src), error->message);
		if (output->retry_count < MAX_RETRY_COUNT) {
			output->retry_count++;
			remoting_gst_pipeline_deinit(output);
			remoting_gst_schedule_restart(output);
		} else {
			remoting_output_disable(output->output);
		}
		break;
	default:
		break;
	}
}

static void
remoting_output_buffer_release(struct remoted_output *output, void *buffer)
{
	const struct weston_drm_virtual_output_api *api
		= output->remoting->virtual_output_api;

	api->buffer_released(buffer);
}

static int
remoting_gstpipe_handler(int fd, uint32_t mask, void *data)
{
	ssize_t ret;
	struct gstpipe_msg_data msg;
	struct remoted_output *output = data;

	/* receive message */
	ret = read(fd, &msg, sizeof(msg));
	if (ret != sizeof(msg)) {
		weston_log("ERROR: failed to read, ret=%zd, errno=%d\n",
			   ret, errno);
		remoting_output_disable(output->output);
		return 0;
	}

	switch (msg.type) {
	case GSTPIPE_MSG_BUS_SYNC:
		remoting_gst_bus_message_handler(output);
		break;
	case GSTPIPE_MSG_BUFFER_RELEASE:
		remoting_output_buffer_release(output, msg.data);
		break;
	default:
		weston_log("Received unknown message! msg=%d\n", msg.type);
	}
	return 1;
}

static int
remoting_gstpipe_init(struct weston_compositor *c,
		      struct remoted_output *output)
{
	struct wl_event_loop *loop;
	int fd[2];

	if (pipe2(fd, O_CLOEXEC) == -1)
		return -1;

	output->gstpipe.readfd = fd[0];
	output->gstpipe.writefd = fd[1];
	loop = wl_display_get_event_loop(c->wl_display);
	output->gstpipe.source =
		wl_event_loop_add_fd(loop, output->gstpipe.readfd,
				     WL_EVENT_READABLE,
				     remoting_gstpipe_handler, output);
	if (!output->gstpipe.source) {
		close(fd[0]);
		close(fd[1]);
		return -1;
	}

	return 0;
}

static void
remoting_gstpipe_release(struct remoted_gstpipe *pipe)
{
	wl_event_source_remove(pipe->source);
	close(pipe->readfd);
	close(pipe->writefd);
}

static void
remoting_output_destroy(struct weston_output *output);

static void
weston_remoting_destroy(struct wl_listener *l, void *data)
{
	struct weston_remoting *remoting =
		container_of(l, struct weston_remoting, destroy_listener);
	struct remoted_output *output, *next;

	wl_list_for_each_safe(output, next, &remoting->output_list, link)
		remoting_output_destroy(output->output);

	/* Finalize gstreamer */
	remoting_gst_deinit(remoting);

	wl_list_remove(&remoting->destroy_listener.link);
	free(remoting);
}

static struct weston_remoting *
weston_remoting_get(struct weston_compositor *compositor)
{
	struct wl_listener *listener;
	struct weston_remoting *remoting;

	listener = wl_signal_get(&compositor->destroy_signal,
				 weston_remoting_destroy);
	if (!listener)
		return NULL;

	remoting = wl_container_of(listener, remoting, destroy_listener);
	return remoting;
}

static int
remoting_output_finish_frame_handler(void *data)
{
	struct remoted_output *output = data;
	const struct weston_drm_virtual_output_api *api
		= output->remoting->virtual_output_api;
	struct timespec now;
	int64_t msec;

	if (output->submitted_frame) {
		struct weston_compositor *c = output->remoting->compositor;
		output->submitted_frame = false;
		weston_compositor_read_presentation_clock(c, &now);
		api->finish_frame(output->output, &now, 0);
	}

	if (output->dpms == WESTON_DPMS_ON) {
		msec = millihz_to_nsec(output->output->current_mode->refresh) / 1000000;
		wl_event_source_timer_update(output->finish_frame_timer, msec);
	} else {
		wl_event_source_timer_update(output->finish_frame_timer, 0);
	}
	return 0;
}

static void
remoting_gst_mem_free_cb(struct mem_free_cb_data *cb_data, GstMiniObject *obj)
{
	struct remoted_output *output = cb_data->output;
	struct remoted_gstpipe *pipe = &output->gstpipe;
	struct gstpipe_msg_data msg = {
		.type = GSTPIPE_MSG_BUFFER_RELEASE,
		.data = cb_data->output_buffer
	};
	ssize_t ret;

	ret = write(pipe->writefd, &msg, sizeof(msg));
	if (ret != sizeof(msg))
		weston_log("ERROR: failed to write, ret=%zd, errno=%d\n", ret,
			   errno);
	free(cb_data);
}

static struct remoted_output *
lookup_remoted_output(struct weston_output *output)
{
	struct weston_compositor *c = output->compositor;
	struct weston_remoting *remoting = weston_remoting_get(c);
	struct remoted_output *remoted_output;

	/* XXX: This could happen on the compositor shutdown path with our
	 * destroy listener being removed, and remoting_output_destroy() being
	 * called as a virtual destructor.
	 *
	 * See https://gitlab.freedesktop.org/wayland/weston/-/issues/591 for
	 * an alternative to the shutdown sequence.
	 */
	if (!remoting)
		return NULL;

	wl_list_for_each(remoted_output, &remoting->output_list, link) {
		if (remoted_output->output == output)
			return remoted_output;
	}

	weston_log("%s: %s: could not find output\n", __FILE__, __func__);
	return NULL;
}

static void
remoting_output_gst_push_buffer(struct remoted_output *output,
				GstBuffer *buffer)
{
	struct timespec current_frame_ts;
	GstClockTime ts, current_frame_time;

	weston_compositor_read_presentation_clock(output->remoting->compositor,
						  &current_frame_ts);
	current_frame_time = GST_TIMESPEC_TO_TIME(current_frame_ts);
	if (output->start_time == 0)
		output->start_time = current_frame_time;
	ts = current_frame_time - output->start_time;

	if (GST_CLOCK_TIME_IS_VALID(ts))
		GST_BUFFER_PTS(buffer) = ts;
	else
		GST_BUFFER_PTS(buffer) = GST_CLOCK_TIME_NONE;
	GST_BUFFER_DURATION(buffer) = GST_CLOCK_TIME_NONE;

	gst_app_src_push_buffer(output->appsrc, buffer);
	output->submitted_frame = true;
}

static int
remoting_output_fence_sync_handler(int fd, uint32_t mask, void *data)
{
	struct gst_frame_buffer_data *frame_data = data;
	struct remoted_output *output = frame_data->output;

	remoting_output_gst_push_buffer(output, frame_data->buffer);

	wl_event_source_remove(output->fence_sync_event_source);
	close(output->fence_sync_fd);
	free(frame_data);

	return 0;
}

static int
remoting_output_frame(struct weston_output *output_base, int fd, int stride,
		      struct drm_fb *output_buffer)
{
	struct remoted_output *output = lookup_remoted_output(output_base);
	struct weston_remoting *remoting = output->remoting;
	struct weston_mode *mode;
	const struct weston_drm_virtual_output_api *api
		= output->remoting->virtual_output_api;
	struct wl_event_loop *loop;
	GstBuffer *buf;
	GstMemory *mem;
	gsize offsets[4] = { 0, };
	gint strides[4] = { stride, };
	struct mem_free_cb_data *cb_data;
	struct gst_frame_buffer_data *frame_data;

	if (!output)
		return -1;

	cb_data = zalloc(sizeof *cb_data);
	if (!cb_data)
		return -1;

	mode = output->output->current_mode;
	buf = gst_buffer_new();
	mem = gst_dmabuf_allocator_alloc(remoting->allocator, fd,
					 stride * mode->height);
	gst_buffer_append_memory(buf, mem);
	gst_buffer_add_video_meta_full(buf,
				       GST_VIDEO_FRAME_FLAG_NONE,
				       output->format->gst_video_format,
				       mode->width,
				       mode->height,
				       1,
				       offsets,
				       strides);

	cb_data->output = output;
	cb_data->output_buffer = output_buffer;
	gst_mini_object_weak_ref(GST_MINI_OBJECT(mem),
				 (GstMiniObjectNotify)remoting_gst_mem_free_cb,
				 cb_data);

	output->fence_sync_fd = api->get_fence_sync_fd(output->output);
	/* Push buffer to gstreamer immediately on get_fence_sync_fd failure */
	if (output->fence_sync_fd == -1) {
		remoting_output_gst_push_buffer(output, buf);
		return 0;
	}

	frame_data = zalloc(sizeof *frame_data);
	if (!frame_data) {
		close(output->fence_sync_fd);
		remoting_output_gst_push_buffer(output, buf);
		return 0;
	}

	frame_data->output = output;
	frame_data->buffer = buf;
	loop = wl_display_get_event_loop(remoting->compositor->wl_display);
	output->fence_sync_event_source =
		wl_event_loop_add_fd(loop, output->fence_sync_fd,
				     WL_EVENT_READABLE,
				     remoting_output_fence_sync_handler,
				     frame_data);

	return 0;
}

static void
remoting_output_destroy(struct weston_output *output)
{
	struct remoted_output *remoted_output = lookup_remoted_output(output);
	struct weston_mode *mode, *next;

	if (!remoted_output)
		return;

	weston_head_release(remoted_output->head);

	wl_list_for_each_safe(mode, next, &output->mode_list, link) {
		wl_list_remove(&mode->link);
		free(mode);
	}

	remoting_gst_pipeline_deinit(remoted_output);
	remoting_gstpipe_release(&remoted_output->gstpipe);

	if (remoted_output->host)
		free(remoted_output->host);
	if (remoted_output->gst_pipeline)
		free(remoted_output->gst_pipeline);

	wl_list_remove(&remoted_output->link);
	free(remoted_output->head);
	free(remoted_output);
}

static int
remoting_output_start_repaint_loop(struct weston_output *output)
{
	struct remoted_output *remoted_output = lookup_remoted_output(output);
	int64_t msec;

	remoted_output->saved_start_repaint_loop(output);

	msec = millihz_to_nsec(remoted_output->output->current_mode->refresh)
			/ 1000000;
	wl_event_source_timer_update(remoted_output->finish_frame_timer, msec);

	return 0;
}

static void
remoting_output_set_dpms(struct weston_output *base_output, enum dpms_enum level)
{
	struct remoted_output *output = lookup_remoted_output(base_output);

	if (output->dpms == level)
		return;

	output->dpms = level;
	remoting_output_finish_frame_handler(output);
}

static int
remoting_output_enable(struct weston_output *output)
{
	struct remoted_output *remoted_output = lookup_remoted_output(output);
	struct weston_compositor *c = output->compositor;
	const struct weston_drm_virtual_output_api *api
		= remoted_output->remoting->virtual_output_api;
	struct wl_event_loop *loop;
	int ret;

	api->set_submit_frame_cb(output, remoting_output_frame);

	ret = remoted_output->saved_enable(output);
	if (ret < 0)
		return ret;

	remoted_output->saved_start_repaint_loop = output->start_repaint_loop;
	output->start_repaint_loop = remoting_output_start_repaint_loop;
	output->set_dpms = remoting_output_set_dpms;

	ret = remoting_gst_pipeline_init(remoted_output);
	if (ret < 0) {
		remoted_output->saved_disable(output);
		return ret;
	}

	loop = wl_display_get_event_loop(c->wl_display);
	remoted_output->finish_frame_timer =
		wl_event_loop_add_timer(loop,
					remoting_output_finish_frame_handler,
					remoted_output);

	remoted_output->dpms = WESTON_DPMS_ON;
	return 0;
}

static int
remoting_output_disable(struct weston_output *output)
{
	struct remoted_output *remoted_output = lookup_remoted_output(output);

	wl_event_source_remove(remoted_output->finish_frame_timer);
	remoting_gst_pipeline_deinit(remoted_output);

	return remoted_output->saved_disable(output);
}

static struct weston_output *
remoting_output_create(struct weston_compositor *c, char *name)
{
	struct weston_remoting *remoting = weston_remoting_get(c);
	struct remoted_output *output;
	struct weston_head *head;
	const struct weston_drm_virtual_output_api *api;
	const char *make = "Renesas";
	const char *model = "Virtual Display";
	const char *serial_number = "unknown";
	const char *connector_name = "remoting";
	char *remoting_name;

	if (!name || !strlen(name))
		return NULL;

	api = remoting->virtual_output_api;

	output = zalloc(sizeof *output);
	if (!output)
		return NULL;

	head = zalloc(sizeof *head);
	if (!head)
		goto err;

	if (remoting_gstpipe_init(c, output) < 0) {
		weston_log("Can not create pipe for gstreamer\n");
		goto err;
	}

	output->output = api->create_output(c, name, remoting_output_destroy);
	if (!output->output) {
		weston_log("Can not create virtual output\n");
		goto err;
	}

	output->saved_enable = output->output->enable;
	output->output->enable = remoting_output_enable;
	output->saved_disable = output->output->disable;
	output->output->disable = remoting_output_disable;
	output->remoting = remoting;
	wl_list_insert(remoting->output_list.prev, &output->link);

	str_printf(&remoting_name, "%s-%s", connector_name, name);
	weston_head_init(head, remoting_name);
	weston_head_set_subpixel(head, WL_OUTPUT_SUBPIXEL_NONE);
	weston_head_set_monitor_strings(head, make, model, serial_number);
	head->compositor = c;

	weston_output_attach_head(output->output, head);
	output->head = head;

	/* set XRGB8888 format */
	output->format = &supported_formats[0];
	free(remoting_name);

	return output->output;

err:
	if (output->gstpipe.source)
		remoting_gstpipe_release(&output->gstpipe);
	if (head)
		free(head);
	free(output);
	return NULL;
}

static bool
remoting_output_is_remoted(struct weston_output *output)
{
	struct remoted_output *remoted_output = lookup_remoted_output(output);

	if (remoted_output)
		return true;

	return false;
}

static int
remoting_output_set_mode(struct weston_output *output, const char *modeline)
{
	struct weston_mode *mode;
	int n, width, height, refresh = 0;

	if (!remoting_output_is_remoted(output)) {
		weston_log("Output is not remoted.\n");
		return -1;
	}

	if (!modeline)
		return -1;

	n = sscanf(modeline, "%dx%d@%d", &width, &height, &refresh);
	if (n != 2 && n != 3)
		return -1;

	mode = zalloc(sizeof *mode);
	if (!mode)
		return -1;

	mode->flags = WL_OUTPUT_MODE_CURRENT;
	mode->width = width;
	mode->height = height;
	mode->refresh = (refresh ? refresh : 60) * 1000LL;

	wl_list_insert(output->mode_list.prev, &mode->link);

	output->current_mode = mode;

	return 0;
}

static void
remoting_output_set_gbm_format(struct weston_output *output,
			       const char *gbm_format)
{
	struct remoted_output *remoted_output = lookup_remoted_output(output);
	const struct weston_drm_virtual_output_api *api;
	uint32_t format, i;

	if (!remoted_output)
		return;

	api = remoted_output->remoting->virtual_output_api;
	format = api->set_gbm_format(output, gbm_format);

	for (i = 0; i < ARRAY_LENGTH(supported_formats); i++) {
		if (format == supported_formats[i].gbm_format) {
			remoted_output->format = &supported_formats[i];
			return;
		}
	}
}

static void
remoting_output_set_seat(struct weston_output *output, const char *seat)
{
	/* for now, nothing todo */
}

static void
remoting_output_set_host(struct weston_output *output, char *host)
{
	struct remoted_output *remoted_output = lookup_remoted_output(output);

	if (!remoted_output)
		return;

	if (remoted_output->host)
		free(remoted_output->host);
	remoted_output->host = strdup(host);
}

static void
remoting_output_set_port(struct weston_output *output, int port)
{
	struct remoted_output *remoted_output = lookup_remoted_output(output);

	if (remoted_output)
		remoted_output->port = port;
}

static void
remoting_output_set_gst_pipeline(struct weston_output *output,
				 char *gst_pipeline)
{
	struct remoted_output *remoted_output = lookup_remoted_output(output);

	if (!remoted_output)
		return;

	if (remoted_output->gst_pipeline)
		free(remoted_output->gst_pipeline);
	remoted_output->gst_pipeline = strdup(gst_pipeline);
}

static const struct weston_remoting_api remoting_api = {
	remoting_output_create,
	remoting_output_is_remoted,
	remoting_output_set_mode,
	remoting_output_set_gbm_format,
	remoting_output_set_seat,
	remoting_output_set_host,
	remoting_output_set_port,
	remoting_output_set_gst_pipeline,
};

WL_EXPORT int
weston_module_init(struct weston_compositor *compositor)
{
	int ret;
	struct weston_remoting *remoting;
	const struct weston_drm_virtual_output_api *api =
		weston_drm_virtual_output_get_api(compositor);

	if (!api)
		return -1;

	remoting = zalloc(sizeof *remoting);
	if (!remoting)
		return -1;

	if (!weston_compositor_add_destroy_listener_once(compositor,
							 &remoting->destroy_listener,
							 weston_remoting_destroy)) {
		free(remoting);
		return 0;
	}

	remoting->virtual_output_api = api;
	remoting->compositor = compositor;
	wl_list_init(&remoting->output_list);

	ret = weston_plugin_api_register(compositor, WESTON_REMOTING_API_NAME,
					 &remoting_api, sizeof(remoting_api));

	if (ret < 0) {
		weston_log("Failed to register remoting API.\n");
		goto failed;
	}

	/* Initialize gstreamer */
	ret = remoting_gst_init(remoting);
	if (ret < 0) {
		weston_log("Failed to initialize gstreamer.\n");
		goto failed;
	}

	return 0;

failed:
	wl_list_remove(&remoting->destroy_listener.link);
	free(remoting);
	return -1;
}
