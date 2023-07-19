/*
 * Copyright © 2012 Intel Corporation
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

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <libweston/libweston.h>
#include <libweston-desktop/libweston-desktop.h>
#include <libweston/weston-log.h>
#include <wayland-server-core.h>
#include "backend.h"
#include "libweston-internal.h"
#include "compositor/weston.h"
#include "weston-test-server-protocol.h"
#include "weston.h"
#include "weston-testsuite-data.h"

#include "shared/helpers.h"
#include "shared/timespec-util.h"

#define MAX_TOUCH_DEVICES 32

struct weston_test {
	struct weston_compositor *compositor;
	struct wl_listener destroy_listener;

	struct weston_log_scope *log;

	struct weston_layer layer;
	struct weston_seat seat;
	struct weston_touch_device *touch_device[MAX_TOUCH_DEVICES];
	int nr_touch_devices;
	bool is_seat_initialized;

	pthread_t client_thread;
	struct wl_event_source *client_source;
};

struct weston_test_surface {
	struct weston_surface *surface;
	struct weston_view *view;
	int32_t x, y;
	struct weston_test *test;
};

static void
touch_device_add(struct weston_test *test)
{
	char buf[128];
	int i = test->nr_touch_devices;

	assert(i < MAX_TOUCH_DEVICES);
	assert(!test->touch_device[i]);

	snprintf(buf, sizeof buf, "test-touch-device-%d", i);

	test->touch_device[i] = weston_touch_create_touch_device(
				test->seat.touch_state, buf, NULL, NULL);
	test->nr_touch_devices++;
}

static void
touch_device_remove(struct weston_test *test)
{
	int i = test->nr_touch_devices - 1;

	assert(i >= 0);
	assert(test->touch_device[i]);
	weston_touch_device_destroy(test->touch_device[i]);
	test->touch_device[i] = NULL;
	--test->nr_touch_devices;
}

static int
test_seat_init(struct weston_test *test)
{
	assert(!test->is_seat_initialized &&
	       "Trying to add already added test seat");

	/* create our own seat */
	weston_seat_init(&test->seat, test->compositor, "test-seat");
	test->is_seat_initialized = true;

	/* add devices */
	weston_seat_init_pointer(&test->seat);
	if (weston_seat_init_keyboard(&test->seat, NULL) < 0)
		return -1;
	weston_seat_init_touch(&test->seat);
	touch_device_add(test);

	return 0;
}

static void
test_seat_release(struct weston_test *test)
{
	while (test->nr_touch_devices > 0)
		touch_device_remove(test);

	assert(test->is_seat_initialized &&
	       "Trying to release already released test seat");
	test->is_seat_initialized = false;
	weston_seat_release(&test->seat);
	memset(&test->seat, 0, sizeof test->seat);
}

static struct weston_seat *
get_seat(struct weston_test *test)
{
	return &test->seat;
}

static void
notify_pointer_position(struct weston_test *test, struct wl_resource *resource)
{
	struct weston_seat *seat = get_seat(test);
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);

	weston_test_send_pointer_position(resource, pointer->x, pointer->y);
}

static void
test_surface_committed(struct weston_surface *surface, int32_t sx, int32_t sy)
{
	struct weston_test_surface *test_surface = surface->committed_private;
	struct weston_test *test = test_surface->test;

	if (wl_list_empty(&test_surface->view->layer_link.link))
		weston_layer_entry_insert(&test->layer.view_list,
					  &test_surface->view->layer_link);

	weston_view_set_position(test_surface->view,
				 test_surface->x, test_surface->y);

	weston_view_update_transform(test_surface->view);

	test_surface->surface->is_mapped = true;
	test_surface->view->is_mapped = true;
}

static void
move_surface(struct wl_client *client, struct wl_resource *resource,
	     struct wl_resource *surface_resource,
	     int32_t x, int32_t y)
{
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct weston_test_surface *test_surface;

	test_surface = surface->committed_private;
	if (!test_surface) {
		test_surface = malloc(sizeof *test_surface);
		if (!test_surface) {
			wl_resource_post_no_memory(resource);
			return;
		}

		test_surface->view = weston_view_create(surface);
		if (!test_surface->view) {
			wl_resource_post_no_memory(resource);
			free(test_surface);
			return;
		}

		surface->committed_private = test_surface;
		surface->committed = test_surface_committed;
	}

	test_surface->surface = surface;
	test_surface->test = wl_resource_get_user_data(resource);
	test_surface->x = x;
	test_surface->y = y;
}

static void
move_pointer(struct wl_client *client, struct wl_resource *resource,
	     struct wl_resource *surface_resource, uint32_t tv_sec_hi,
	     uint32_t tv_sec_lo, uint32_t tv_nsec,
	     int32_t x, int32_t y)
{
	struct weston_test *test = wl_resource_get_user_data(resource);
	struct weston_seat *seat = get_seat(test);
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);
	struct weston_pointer_motion_event event = { 0 };
	struct timespec time;

	if (surface_resource) {
		struct weston_surface *surface =
			wl_resource_get_user_data(surface_resource);
		struct weston_desktop_surface *desktop_surface = surface->committed_private;
		if (!desktop_surface) {
			desktop_surface = weston_surface_get_desktop_surface(surface);
			if (!desktop_surface) {
				wl_resource_post_no_memory(resource);
				return;
			}
		}

		struct weston_geometry geometry;
		weston_desktop_surface_get_root_geometry(desktop_surface, &geometry);

		// Translate the request from surface local to global coordinates.
		x = x + geometry.x;
		y = y + geometry.y;
	}

	event = (struct weston_pointer_motion_event) {
		.mask = WESTON_POINTER_MOTION_REL,
		.dx = wl_fixed_to_double(wl_fixed_from_int(x) - pointer->x),
		.dy = wl_fixed_to_double(wl_fixed_from_int(y) - pointer->y),
	};

	timespec_from_proto(&time, tv_sec_hi, tv_sec_lo, tv_nsec);

	notify_motion(seat, &time, &event);

	notify_pointer_position(test, resource);
}

static void
send_button(struct wl_client *client, struct wl_resource *resource,
	    uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec,
	    int32_t button, uint32_t state)
{
	struct timespec time;

	struct weston_test *test = wl_resource_get_user_data(resource);
	struct weston_seat *seat = get_seat(test);

	timespec_from_proto(&time, tv_sec_hi, tv_sec_lo, tv_nsec);

	notify_button(seat, &time, button, state);

	weston_test_send_pointer_button(resource, button, state);
}

static void
reset_pointer(struct wl_client *client, struct wl_resource *resource) {
	struct timespec time;

	struct weston_test *test = wl_resource_get_user_data(resource);
	struct weston_seat *seat = get_seat(test);
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);

	if (!pointer->grab_button)
		return;

	while (pointer->button_count > 0) {
		clock_gettime(CLOCK_MONOTONIC, &time);
		notify_button(seat, &time, pointer->grab_button, WL_POINTER_BUTTON_STATE_RELEASED);
	}
}

static void
send_axis(struct wl_client *client, struct wl_resource *resource,
	  uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec,
	  uint32_t axis, wl_fixed_t value)
{
	struct weston_test *test = wl_resource_get_user_data(resource);
	struct weston_seat *seat = get_seat(test);
	struct timespec time;
	struct weston_pointer_axis_event axis_event;

	timespec_from_proto(&time, tv_sec_hi, tv_sec_lo, tv_nsec);
	axis_event.axis = axis;
	axis_event.value = wl_fixed_to_double(value);
	axis_event.has_discrete = false;
	axis_event.discrete = 0;

	notify_axis(seat, &time, &axis_event);
}

static void
activate_surface(struct wl_client *client, struct wl_resource *resource,
		 struct wl_resource *surface_resource)
{
	struct weston_surface *surface = surface_resource ?
		wl_resource_get_user_data(surface_resource) : NULL;
	struct weston_test *test = wl_resource_get_user_data(resource);
	struct weston_seat *seat;
	struct weston_keyboard *keyboard;

	seat = get_seat(test);
	keyboard = weston_seat_get_keyboard(seat);
	if (surface) {
		weston_seat_set_keyboard_focus(seat, surface);
		notify_keyboard_focus_in(seat, &keyboard->keys,
					 STATE_UPDATE_AUTOMATIC);
	}
	else {
		notify_keyboard_focus_out(seat);
		weston_seat_set_keyboard_focus(seat, surface);
	}
}

static void
send_key(struct wl_client *client, struct wl_resource *resource,
	 uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec,
	 uint32_t key, enum wl_keyboard_key_state state)
{
	struct weston_test *test = wl_resource_get_user_data(resource);
	struct weston_seat *seat = get_seat(test);
	struct timespec time;

	timespec_from_proto(&time, tv_sec_hi, tv_sec_lo, tv_nsec);

	notify_key(seat, &time, key, state, STATE_UPDATE_AUTOMATIC);

	weston_test_send_keyboard_key(resource, key, state);
}

static void
device_release(struct wl_client *client,
	       struct wl_resource *resource, const char *device)
{
	struct weston_test *test = wl_resource_get_user_data(resource);
	struct weston_seat *seat = get_seat(test);

	if (strcmp(device, "pointer") == 0) {
		weston_seat_release_pointer(seat);
	} else if (strcmp(device, "keyboard") == 0) {
		weston_seat_release_keyboard(seat);
	} else if (strcmp(device, "touch") == 0) {
		touch_device_remove(test);
		weston_seat_release_touch(seat);
	} else if (strcmp(device, "seat") == 0) {
		test_seat_release(test);
	} else {
		assert(0 && "Unsupported device");
	}
}

static void
device_add(struct wl_client *client,
	   struct wl_resource *resource, const char *device)
{
	struct weston_test *test = wl_resource_get_user_data(resource);
	struct weston_seat *seat = get_seat(test);

	if (strcmp(device, "pointer") == 0) {
		weston_seat_init_pointer(seat);
	} else if (strcmp(device, "keyboard") == 0) {
		weston_seat_init_keyboard(seat, NULL);
	} else if (strcmp(device, "touch") == 0) {
		weston_seat_init_touch(seat);
		touch_device_add(test);
	} else if (strcmp(device, "seat") == 0) {
		test_seat_init(test);
	} else {
		assert(0 && "Unsupported device");
	}
}

enum weston_test_screenshot_outcome {
	WESTON_TEST_SCREENSHOT_SUCCESS,
	WESTON_TEST_SCREENSHOT_NO_MEMORY,
	WESTON_TEST_SCREENSHOT_BAD_BUFFER
	};

typedef void (*weston_test_screenshot_done_func_t)(void *data,
						   enum weston_test_screenshot_outcome outcome);

struct test_screenshot {
	struct weston_compositor *compositor;
	struct wl_global *global;
	struct wl_client *client;
	struct weston_process process;
	struct wl_listener destroy_listener;
};

struct test_screenshot_frame_listener {
	struct wl_listener listener;
	struct weston_buffer *buffer;
	struct weston_output *output;
	weston_test_screenshot_done_func_t done;
	void *data;
};

static void
copy_bgra_yflip(uint8_t *dst, uint8_t *src, int height, int stride)
{
	uint8_t *end;

	end = dst + height * stride;
	while (dst < end) {
		memcpy(dst, src, stride);
		dst += stride;
		src -= stride;
	}
}


static void
copy_bgra(uint8_t *dst, uint8_t *src, int height, int stride)
{
	/* TODO: optimize this out */
	memcpy(dst, src, height * stride);
}

static void
copy_row_swap_RB(void *vdst, void *vsrc, int bytes)
{
	uint32_t *dst = vdst;
	uint32_t *src = vsrc;
	uint32_t *end = dst + bytes / 4;

	while (dst < end) {
		uint32_t v = *src++;
		/*                    A R G B */
		uint32_t tmp = v & 0xff00ff00;
		tmp |= (v >> 16) & 0x000000ff;
		tmp |= (v << 16) & 0x00ff0000;
		*dst++ = tmp;
	}
}

static void
copy_rgba_yflip(uint8_t *dst, uint8_t *src, int height, int stride)
{
	uint8_t *end;

	end = dst + height * stride;
	while (dst < end) {
		copy_row_swap_RB(dst, src, stride);
		dst += stride;
		src -= stride;
	}
}

static void
copy_rgba(uint8_t *dst, uint8_t *src, int height, int stride)
{
	uint8_t *end;

	end = dst + height * stride;
	while (dst < end) {
		copy_row_swap_RB(dst, src, stride);
		dst += stride;
		src += stride;
	}
}

static void
test_screenshot_frame_notify(struct wl_listener *listener, void *data)
{
	struct test_screenshot_frame_listener *l =
		container_of(listener,
			     struct test_screenshot_frame_listener, listener);
	struct weston_output *output = l->output;
	struct weston_compositor *compositor = output->compositor;
	int32_t stride;
	uint8_t *pixels, *d, *s;

	weston_output_disable_planes_decr(output);
	wl_list_remove(&listener->link);
	stride = l->buffer->width * (PIXMAN_FORMAT_BPP(compositor->read_format) / 8);
	pixels = malloc(stride * l->buffer->height);

	if (pixels == NULL) {
		l->done(l->data, WESTON_TEST_SCREENSHOT_NO_MEMORY);
		free(l);
		return;
	}

	/* FIXME: Needs to handle output transformations */

	compositor->renderer->read_pixels(output,
					  compositor->read_format,
					  pixels,
					  0, 0,
					  output->current_mode->width,
					  output->current_mode->height);

	stride = wl_shm_buffer_get_stride(l->buffer->shm_buffer);

	d = wl_shm_buffer_get_data(l->buffer->shm_buffer);
	s = pixels + stride * (l->buffer->height - 1);

	wl_shm_buffer_begin_access(l->buffer->shm_buffer);

	/* XXX: It would be nice if we used Pixman to do all this rather
	 *  than our own implementation
	 */
	switch (compositor->read_format) {
	case PIXMAN_a8r8g8b8:
	case PIXMAN_x8r8g8b8:
		if (compositor->capabilities & WESTON_CAP_CAPTURE_YFLIP)
			copy_bgra_yflip(d, s, output->current_mode->height, stride);
		else
			copy_bgra(d, pixels, output->current_mode->height, stride);
		break;
	case PIXMAN_x8b8g8r8:
	case PIXMAN_a8b8g8r8:
		if (compositor->capabilities & WESTON_CAP_CAPTURE_YFLIP)
			copy_rgba_yflip(d, s, output->current_mode->height, stride);
		else
			copy_rgba(d, pixels, output->current_mode->height, stride);
		break;
	default:
		break;
	}

	wl_shm_buffer_end_access(l->buffer->shm_buffer);

	l->done(l->data, WESTON_TEST_SCREENSHOT_SUCCESS);
	free(pixels);
	free(l);
}

static bool
weston_test_screenshot_shoot(struct weston_output *output,
			     struct weston_buffer *buffer,
			     weston_test_screenshot_done_func_t done,
			     void *data)
{
	struct test_screenshot_frame_listener *l;

	/* Get the shm buffer resource the client created */
	if (!wl_shm_buffer_get(buffer->resource)) {
		done(data, WESTON_TEST_SCREENSHOT_BAD_BUFFER);
		return false;
	}

	buffer->shm_buffer = wl_shm_buffer_get(buffer->resource);
	buffer->width = wl_shm_buffer_get_width(buffer->shm_buffer);
	buffer->height = wl_shm_buffer_get_height(buffer->shm_buffer);

	/* Verify buffer is big enough */
	if (buffer->width < output->current_mode->width ||
		buffer->height < output->current_mode->height) {
		done(data, WESTON_TEST_SCREENSHOT_BAD_BUFFER);
		return false;
	}

	/* allocate the frame listener */
	l = malloc(sizeof *l);
	if (l == NULL) {
		done(data, WESTON_TEST_SCREENSHOT_NO_MEMORY);
		return false;
	}

	/* Set up the listener */
	l->buffer = buffer;
	l->output = output;
	l->done = done;
	l->data = data;
	l->listener.notify = test_screenshot_frame_notify;
	wl_signal_add(&output->frame_signal, &l->listener);

	/* Fire off a repaint */
	weston_output_disable_planes_incr(output);
	weston_output_schedule_repaint(output);

	return true;
}

static void
capture_screenshot_done(void *data, enum weston_test_screenshot_outcome outcome)
{
	struct wl_resource *resource = data;

	switch (outcome) {
	case WESTON_TEST_SCREENSHOT_SUCCESS:
		weston_test_send_capture_screenshot_done(resource);
		break;
	case WESTON_TEST_SCREENSHOT_NO_MEMORY:
		wl_resource_post_no_memory(resource);
		break;
	default:
		break;
	}
}


/**
 * Grabs a snapshot of the screen.
 */
static void
capture_screenshot(struct wl_client *client,
		   struct wl_resource *resource,
		   struct wl_resource *output_resource,
		   struct wl_resource *buffer_resource)
{
	struct weston_output *output =
		weston_head_from_resource(output_resource)->output;
	struct weston_buffer *buffer =
		weston_buffer_from_resource(buffer_resource);

	if (buffer == NULL) {
		wl_resource_post_no_memory(resource);
		return;
	}

	weston_test_screenshot_shoot(output, buffer,
				     capture_screenshot_done, resource);
}

static void
send_touch(struct wl_client *client, struct wl_resource *resource,
	   uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec,
	   int32_t touch_id, wl_fixed_t x, wl_fixed_t y, uint32_t touch_type)
{
	struct weston_test *test = wl_resource_get_user_data(resource);
	struct weston_touch_device *device = test->touch_device[0];
	struct timespec time;

	assert(device);

	timespec_from_proto(&time, tv_sec_hi, tv_sec_lo, tv_nsec);

	notify_touch(device, &time, touch_id, wl_fixed_to_double(x),
		     wl_fixed_to_double(y), touch_type);
}

static const struct weston_test_interface test_implementation = {
	move_surface,
	move_pointer,
	send_button,
	reset_pointer,
	send_axis,
	activate_surface,
	send_key,
	device_release,
	device_add,
	capture_screenshot,
	send_touch,
};

static void
bind_test(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct weston_test *test = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &weston_test_interface, 1, id);
	if (!resource) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource,
				       &test_implementation, test, NULL);

	notify_pointer_position(test, resource);
}

static void
client_thread_cleanup(void *data_)
{
	struct wet_testsuite_data *data = data_;

	close(data->thread_event_pipe);
	data->thread_event_pipe = -1;
}

static void *
client_thread_routine(void *data_)
{
	struct wet_testsuite_data *data = data_;

	pthread_setname_np(pthread_self(), "client");
	pthread_cleanup_push(client_thread_cleanup, data);
	data->run(data);
	pthread_cleanup_pop(true);

	return NULL;
}

static void
client_thread_join(struct weston_test *test)
{
	assert(test->client_source);

	pthread_join(test->client_thread, NULL);
	wl_event_source_remove(test->client_source);
	test->client_source = NULL;

	weston_log_scope_printf(test->log, "Test thread reaped.\n");
}

static int
handle_client_thread_event(int fd, uint32_t mask, void *data_)
{
	struct weston_test *test = data_;

	weston_log_scope_printf(test->log,
				"Received thread event mask 0x%x\n", mask);

	if (mask != WL_EVENT_HANGUP)
		weston_log("%s: unexpected event %u\n", __func__, mask);

	client_thread_join(test);
	weston_compositor_exit(test->compositor);

	return 0;
}

static int
create_client_thread(struct weston_test *test, struct wet_testsuite_data *data)
{
	struct wl_event_loop *loop;
	int pipefd[2] = { -1, -1 };
	sigset_t saved;
	sigset_t blocked;
	int ret;

	weston_log_scope_printf(test->log, "Creating a thread for running tests...\n");

	if (pipe2(pipefd, O_CLOEXEC | O_NONBLOCK) < 0) {
		weston_log("Creating pipe for a client thread failed: %s\n",
			   strerror(errno));
		return -1;
	}

	loop = wl_display_get_event_loop(test->compositor->wl_display);
	test->client_source = wl_event_loop_add_fd(loop, pipefd[0],
						   WL_EVENT_READABLE,
						   handle_client_thread_event,
						   test);
	close(pipefd[0]);

	if (!test->client_source) {
		weston_log("Adding client thread fd to event loop failed.\n");
		goto out_pipe;
	}

	data->thread_event_pipe = pipefd[1];

	/* Ensure we don't accidentally get signals to the thread. */
	sigfillset(&blocked);
	sigdelset(&blocked, SIGSEGV);
	sigdelset(&blocked, SIGFPE);
	sigdelset(&blocked, SIGILL);
	sigdelset(&blocked, SIGCONT);
	sigdelset(&blocked, SIGSYS);
	if (pthread_sigmask(SIG_BLOCK, &blocked, &saved) != 0)
		goto out_source;

	ret = pthread_create(&test->client_thread, NULL,
			     client_thread_routine, data);

	pthread_sigmask(SIG_SETMASK, &saved, NULL);

	if (ret != 0) {
		weston_log("Creating client thread failed: %s (%d)\n",
			   strerror(ret), ret);
		goto out_source;
	}

	return 0;

out_source:
	data->thread_event_pipe = -1;
	wl_event_source_remove(test->client_source);
	test->client_source = NULL;

out_pipe:
	close(pipefd[1]);

	return -1;
}

static void
idle_launch_testsuite(void *test_)
{
	struct weston_test *test = test_;
	struct wet_testsuite_data *data = wet_testsuite_data_get();

	if (!data)
		return;

	switch (data->type) {
	case TEST_TYPE_CLIENT:
		if (create_client_thread(test, data) < 0) {
			weston_log("Error: creating client thread for test suite failed.\n");
			weston_compositor_exit_with_code(test->compositor,
							 RESULT_HARD_ERROR);
		}
		break;

	case TEST_TYPE_PLUGIN:
		data->compositor = test->compositor;
		weston_log_scope_printf(test->log,
					"Running tests from idle handler...\n");
		data->run(data);
		weston_compositor_exit(test->compositor);
		break;

	case TEST_TYPE_STANDALONE:
		weston_log("Error: unknown test internal type %d.\n",
			   data->type);
		weston_compositor_exit_with_code(test->compositor,
						 RESULT_HARD_ERROR);
	}
}

static void
handle_compositor_destroy(struct wl_listener *listener,
			  void *weston_compositor)
{
	struct weston_test *test;

	test = wl_container_of(listener, test, destroy_listener);

	if (test->client_source) {
		weston_log_scope_printf(test->log, "Cancelling client thread...\n");
		pthread_cancel(test->client_thread);
		client_thread_join(test);
	}

	if (test->is_seat_initialized)
		test_seat_release(test);

	wl_list_remove(&test->layer.view_list.link);
	wl_list_remove(&test->layer.link);

	weston_log_scope_destroy(test->log);
	free(test);
}

WL_EXPORT int
wet_module_init(struct weston_compositor *ec,
		int *argc, char *argv[])
{
	struct weston_test *test;
	struct wl_event_loop *loop;

	test = zalloc(sizeof *test);
	if (test == NULL)
		return -1;

	if (!weston_compositor_add_destroy_listener_once(ec,
							 &test->destroy_listener,
							 handle_compositor_destroy)) {
		free(test);
		return 0;
	}

	test->compositor = ec;
	weston_layer_init(&test->layer, ec);
	weston_layer_set_position(&test->layer, WESTON_LAYER_POSITION_CURSOR - 1);

	test->log = weston_compositor_add_log_scope(ec, "test-harness-plugin",
					"weston-test plugin's own actions",
					NULL, NULL, NULL);

	if (wl_global_create(ec->wl_display, &weston_test_interface, 1,
			     test, bind_test) == NULL)
		goto out_free;

	if (test_seat_init(test) == -1)
		goto out_free;

	loop = wl_display_get_event_loop(ec->wl_display);
	wl_event_loop_add_idle(loop, idle_launch_testsuite, test);

	return 0;

out_free:
	wl_list_remove(&test->destroy_listener.link);
	free(test);
	return -1;
}
