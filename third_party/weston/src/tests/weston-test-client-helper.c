/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2015 Samsung Electronics Co., Ltd
 * Copyright 2016, 2017 Collabora, Ltd.
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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <cairo.h>

#include "test-config.h"
#include "shared/os-compatibility.h"
#include "shared/xalloc.h"
#include <libweston/zalloc.h>
#include "weston-test-client-helper.h"

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) > (b)) ? (b) : (a))
#define clip(x, a, b)  min(max(x, a), b)

int
surface_contains(struct surface *surface, int x, int y)
{
	/* test whether a global x,y point is contained in the surface */
	int sx = surface->x;
	int sy = surface->y;
	int sw = surface->width;
	int sh = surface->height;
	return x >= sx && y >= sy && x < sx + sw && y < sy + sh;
}

static void
frame_callback_handler(void *data, struct wl_callback *callback, uint32_t time)
{
	int *done = data;

	*done = 1;

	wl_callback_destroy(callback);
}

static const struct wl_callback_listener frame_listener = {
	frame_callback_handler
};

struct wl_callback *
frame_callback_set(struct wl_surface *surface, int *done)
{
	struct wl_callback *callback;

	*done = 0;
	callback = wl_surface_frame(surface);
	wl_callback_add_listener(callback, &frame_listener, done);

	return callback;
}

int
frame_callback_wait_nofail(struct client *client, int *done)
{
	while (!*done) {
		if (wl_display_dispatch(client->wl_display) < 0)
			return 0;
	}

	return 1;
}

void
move_client(struct client *client, int x, int y)
{
	struct surface *surface = client->surface;
	int done;

	client->surface->x = x;
	client->surface->y = y;
	weston_test_move_surface(client->test->weston_test, surface->wl_surface,
			     surface->x, surface->y);
	/* The attach here is necessary because commit() will call configure
	 * only on surfaces newly attached, and the one that sets the surface
	 * position is the configure. */
	wl_surface_attach(surface->wl_surface, surface->buffer->proxy, 0, 0);
	wl_surface_damage(surface->wl_surface, 0, 0, surface->width,
			  surface->height);

	frame_callback_set(surface->wl_surface, &done);

	wl_surface_commit(surface->wl_surface);

	frame_callback_wait(client, &done);
}

static void
pointer_handle_enter(void *data, struct wl_pointer *wl_pointer,
		     uint32_t serial, struct wl_surface *wl_surface,
		     wl_fixed_t x, wl_fixed_t y)
{
	struct pointer *pointer = data;

	if (wl_surface)
		pointer->focus = wl_surface_get_user_data(wl_surface);
	else
		pointer->focus = NULL;

	pointer->x = wl_fixed_to_int(x);
	pointer->y = wl_fixed_to_int(y);

	testlog("test-client: got pointer enter %d %d, surface %p\n",
		pointer->x, pointer->y, pointer->focus);
}

static void
pointer_handle_leave(void *data, struct wl_pointer *wl_pointer,
		     uint32_t serial, struct wl_surface *wl_surface)
{
	struct pointer *pointer = data;

	pointer->focus = NULL;

	testlog("test-client: got pointer leave, surface %p\n",
		wl_surface ? wl_surface_get_user_data(wl_surface) : NULL);
}

static void
pointer_handle_motion(void *data, struct wl_pointer *wl_pointer,
		      uint32_t time_msec, wl_fixed_t x, wl_fixed_t y)
{
	struct pointer *pointer = data;

	pointer->x = wl_fixed_to_int(x);
	pointer->y = wl_fixed_to_int(y);
	pointer->motion_time_msec = time_msec;
	pointer->motion_time_timespec = pointer->input_timestamp;
	pointer->input_timestamp = (struct timespec) { 0 };

	testlog("test-client: got pointer motion %d %d\n",
		pointer->x, pointer->y);
}

static void
pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
		      uint32_t serial, uint32_t time_msec, uint32_t button,
		      uint32_t state)
{
	struct pointer *pointer = data;

	pointer->button = button;
	pointer->state = state;
	pointer->button_time_msec = time_msec;
	pointer->button_time_timespec = pointer->input_timestamp;
	pointer->input_timestamp = (struct timespec) { 0 };

	testlog("test-client: got pointer button %u %u\n", button, state);
}

static void
pointer_handle_axis(void *data, struct wl_pointer *wl_pointer,
		    uint32_t time_msec, uint32_t axis, wl_fixed_t value)
{
	struct pointer *pointer = data;

	pointer->axis = axis;
	pointer->axis_value = wl_fixed_to_double(value);
	pointer->axis_time_msec = time_msec;
	pointer->axis_time_timespec = pointer->input_timestamp;
	pointer->input_timestamp = (struct timespec) { 0 };

	testlog("test-client: got pointer axis %u %f\n",
		axis, wl_fixed_to_double(value));
}

static void
pointer_handle_frame(void *data, struct wl_pointer *wl_pointer)
{
	testlog("test-client: got pointer frame\n");
}

static void
pointer_handle_axis_source(void *data, struct wl_pointer *wl_pointer,
			     uint32_t source)
{
	testlog("test-client: got pointer axis source %u\n", source);
}

static void
pointer_handle_axis_stop(void *data, struct wl_pointer *wl_pointer,
			 uint32_t time_msec, uint32_t axis)
{
	struct pointer *pointer = data;

	pointer->axis = axis;
	pointer->axis_stop_time_msec = time_msec;
	pointer->axis_stop_time_timespec = pointer->input_timestamp;
	pointer->input_timestamp = (struct timespec) { 0 };

	testlog("test-client: got pointer axis stop %u\n", axis);
}

static void
pointer_handle_axis_discrete(void *data, struct wl_pointer *wl_pointer,
			     uint32_t axis, int32_t value)
{
	testlog("test-client: got pointer axis discrete %u %d\n", axis, value);
}

static const struct wl_pointer_listener pointer_listener = {
	pointer_handle_enter,
	pointer_handle_leave,
	pointer_handle_motion,
	pointer_handle_button,
	pointer_handle_axis,
	pointer_handle_frame,
	pointer_handle_axis_source,
	pointer_handle_axis_stop,
	pointer_handle_axis_discrete,
};

static void
keyboard_handle_keymap(void *data, struct wl_keyboard *wl_keyboard,
		       uint32_t format, int fd, uint32_t size)
{
	close(fd);

	testlog("test-client: got keyboard keymap\n");
}

static void
keyboard_handle_enter(void *data, struct wl_keyboard *wl_keyboard,
		      uint32_t serial, struct wl_surface *wl_surface,
		      struct wl_array *keys)
{
	struct keyboard *keyboard = data;

	if (wl_surface)
		keyboard->focus = wl_surface_get_user_data(wl_surface);
	else
		keyboard->focus = NULL;

	testlog("test-client: got keyboard enter, surface %p\n",
		keyboard->focus);
}

static void
keyboard_handle_leave(void *data, struct wl_keyboard *wl_keyboard,
		      uint32_t serial, struct wl_surface *wl_surface)
{
	struct keyboard *keyboard = data;

	keyboard->focus = NULL;

	testlog("test-client: got keyboard leave, surface %p\n",
		wl_surface ? wl_surface_get_user_data(wl_surface) : NULL);
}

static void
keyboard_handle_key(void *data, struct wl_keyboard *wl_keyboard,
		    uint32_t serial, uint32_t time_msec, uint32_t key,
		    uint32_t state)
{
	struct keyboard *keyboard = data;

	keyboard->key = key;
	keyboard->state = state;
	keyboard->key_time_msec = time_msec;
	keyboard->key_time_timespec = keyboard->input_timestamp;
	keyboard->input_timestamp = (struct timespec) { 0 };

	testlog("test-client: got keyboard key %u %u\n", key, state);
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *wl_keyboard,
			  uint32_t serial, uint32_t mods_depressed,
			  uint32_t mods_latched, uint32_t mods_locked,
			  uint32_t group)
{
	struct keyboard *keyboard = data;

	keyboard->mods_depressed = mods_depressed;
	keyboard->mods_latched = mods_latched;
	keyboard->mods_locked = mods_locked;
	keyboard->group = group;

	testlog("test-client: got keyboard modifiers %u %u %u %u\n",
		mods_depressed, mods_latched, mods_locked, group);
}

static void
keyboard_handle_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
			    int32_t rate, int32_t delay)
{
	struct keyboard *keyboard = data;

	keyboard->repeat_info.rate = rate;
	keyboard->repeat_info.delay = delay;

	testlog("test-client: got keyboard repeat_info %d %d\n", rate, delay);
}

static const struct wl_keyboard_listener keyboard_listener = {
	keyboard_handle_keymap,
	keyboard_handle_enter,
	keyboard_handle_leave,
	keyboard_handle_key,
	keyboard_handle_modifiers,
	keyboard_handle_repeat_info,
};

static void
touch_handle_down(void *data, struct wl_touch *wl_touch,
		  uint32_t serial, uint32_t time_msec,
		  struct wl_surface *surface, int32_t id,
		  wl_fixed_t x_w, wl_fixed_t y_w)
{
	struct touch *touch = data;

	touch->down_x = wl_fixed_to_int(x_w);
	touch->down_y = wl_fixed_to_int(y_w);
	touch->id = id;
	touch->down_time_msec = time_msec;
	touch->down_time_timespec = touch->input_timestamp;
	touch->input_timestamp = (struct timespec) { 0 };

	testlog("test-client: got touch down %d %d, surf: %p, id: %d\n",
		touch->down_x, touch->down_y, surface, id);
}

static void
touch_handle_up(void *data, struct wl_touch *wl_touch,
		uint32_t serial, uint32_t time_msec, int32_t id)
{
	struct touch *touch = data;
	touch->up_id = id;
	touch->up_time_msec = time_msec;
	touch->up_time_timespec = touch->input_timestamp;
	touch->input_timestamp = (struct timespec) { 0 };

	testlog("test-client: got touch up, id: %d\n", id);
}

static void
touch_handle_motion(void *data, struct wl_touch *wl_touch,
		    uint32_t time_msec, int32_t id,
		    wl_fixed_t x_w, wl_fixed_t y_w)
{
	struct touch *touch = data;
	touch->x = wl_fixed_to_int(x_w);
	touch->y = wl_fixed_to_int(y_w);
	touch->motion_time_msec = time_msec;
	touch->motion_time_timespec = touch->input_timestamp;
	touch->input_timestamp = (struct timespec) { 0 };

	testlog("test-client: got touch motion, %d %d, id: %d\n",
		touch->x, touch->y, id);
}

static void
touch_handle_frame(void *data, struct wl_touch *wl_touch)
{
	struct touch *touch = data;

	++touch->frame_no;

	testlog("test-client: got touch frame (%d)\n", touch->frame_no);
}

static void
touch_handle_cancel(void *data, struct wl_touch *wl_touch)
{
	struct touch *touch = data;

	++touch->cancel_no;

	testlog("test-client: got touch cancel (%d)\n", touch->cancel_no);
}

static const struct wl_touch_listener touch_listener = {
	touch_handle_down,
	touch_handle_up,
	touch_handle_motion,
	touch_handle_frame,
	touch_handle_cancel,
};

static void
surface_enter(void *data,
	      struct wl_surface *wl_surface, struct wl_output *output)
{
	struct surface *surface = data;

	surface->output = wl_output_get_user_data(output);

	testlog("test-client: got surface enter output %p\n", surface->output);
}

static void
surface_leave(void *data,
	      struct wl_surface *wl_surface, struct wl_output *output)
{
	struct surface *surface = data;

	surface->output = NULL;

	testlog("test-client: got surface leave output %p\n",
		wl_output_get_user_data(output));
}

static const struct wl_surface_listener surface_listener = {
	surface_enter,
	surface_leave
};

static struct buffer *
create_shm_buffer(struct client *client, int width, int height,
		  pixman_format_code_t format, uint32_t wlfmt)
{
	struct wl_shm *shm = client->wl_shm;
	struct buffer *buf;
	size_t stride_bytes;
	struct wl_shm_pool *pool;
	int fd;
	void *data;
	size_t bytes_pp;

	assert(width > 0);
	assert(height > 0);

	buf = xzalloc(sizeof *buf);

	bytes_pp = PIXMAN_FORMAT_BPP(format) / 8;
	stride_bytes = width * bytes_pp;
	/* round up to multiple of 4 bytes for Pixman */
	stride_bytes = (stride_bytes + 3) & ~3u;
	assert(stride_bytes / bytes_pp >= (unsigned)width);

	buf->len = stride_bytes * height;
	assert(buf->len / stride_bytes == (unsigned)height);

	fd = os_create_anonymous_file(buf->len);
	assert(fd >= 0);

	data = mmap(NULL, buf->len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		close(fd);
		assert(data != MAP_FAILED);
	}

	pool = wl_shm_create_pool(shm, fd, buf->len);
	buf->proxy = wl_shm_pool_create_buffer(pool, 0, width, height,
					       stride_bytes, wlfmt);
	wl_shm_pool_destroy(pool);
	close(fd);

	buf->image = pixman_image_create_bits(format, width, height,
					      data, stride_bytes);

	assert(buf->proxy);
	assert(buf->image);

	return buf;
}

struct buffer *
create_shm_buffer_a8r8g8b8(struct client *client, int width, int height)
{
	assert(client->has_argb);

	return create_shm_buffer(client, width, height,
				 PIXMAN_a8r8g8b8, WL_SHM_FORMAT_ARGB8888);
}

void
buffer_destroy(struct buffer *buf)
{
	void *pixels;

	pixels = pixman_image_get_data(buf->image);

	if (buf->proxy) {
		wl_buffer_destroy(buf->proxy);
		assert(munmap(pixels, buf->len) == 0);
	}

	assert(pixman_image_unref(buf->image));

	free(buf);
}

static void
shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
	struct client *client = data;

	if (format == WL_SHM_FORMAT_ARGB8888)
		client->has_argb = 1;
}

struct wl_shm_listener shm_listener = {
	shm_format
};

static void
test_handle_pointer_position(void *data, struct weston_test *weston_test,
			     wl_fixed_t x, wl_fixed_t y)
{
	struct test *test = data;
	test->pointer_x = wl_fixed_to_int(x);
	test->pointer_y = wl_fixed_to_int(y);

	testlog("test-client: got global pointer %d %d\n",
		test->pointer_x, test->pointer_y);
}

static void
test_handle_capture_screenshot_done(void *data, struct weston_test *weston_test)
{
	struct test *test = data;

	testlog("Screenshot has been captured\n");
	test->buffer_copy_done = 1;
}

static const struct weston_test_listener test_listener = {
	test_handle_pointer_position,
	test_handle_capture_screenshot_done,
};

static void
input_destroy(struct input *inp)
{
	if (inp->pointer) {
		wl_pointer_release(inp->pointer->wl_pointer);
		free(inp->pointer);
	}
	if (inp->keyboard) {
		wl_keyboard_release(inp->keyboard->wl_keyboard);
		free(inp->keyboard);
	}
	if (inp->touch) {
		wl_touch_release(inp->touch->wl_touch);
		free(inp->touch);
	}
	wl_list_remove(&inp->link);
	wl_seat_release(inp->wl_seat);
	free(inp->seat_name);
	free(inp);
}

static void
input_update_devices(struct input *input)
{
	struct pointer *pointer;
	struct keyboard *keyboard;
	struct touch *touch;

	struct wl_seat *seat = input->wl_seat;
	enum wl_seat_capability caps = input->caps;

	if ((caps & WL_SEAT_CAPABILITY_POINTER) && !input->pointer) {
		pointer = xzalloc(sizeof *pointer);
		pointer->wl_pointer = wl_seat_get_pointer(seat);
		wl_pointer_set_user_data(pointer->wl_pointer, pointer);
		wl_pointer_add_listener(pointer->wl_pointer, &pointer_listener,
					pointer);
		input->pointer = pointer;
	} else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && input->pointer) {
		wl_pointer_destroy(input->pointer->wl_pointer);
		free(input->pointer);
		input->pointer = NULL;
	}

	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !input->keyboard) {
		keyboard = xzalloc(sizeof *keyboard);
		keyboard->wl_keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_set_user_data(keyboard->wl_keyboard, keyboard);
		wl_keyboard_add_listener(keyboard->wl_keyboard, &keyboard_listener,
					 keyboard);
		input->keyboard = keyboard;
	} else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && input->keyboard) {
		wl_keyboard_destroy(input->keyboard->wl_keyboard);
		free(input->keyboard);
		input->keyboard = NULL;
	}

	if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !input->touch) {
		touch = xzalloc(sizeof *touch);
		touch->wl_touch = wl_seat_get_touch(seat);
		wl_touch_set_user_data(touch->wl_touch, touch);
		wl_touch_add_listener(touch->wl_touch, &touch_listener,
					 touch);
		input->touch = touch;
	} else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && input->touch) {
		wl_touch_destroy(input->touch->wl_touch);
		free(input->touch);
		input->touch = NULL;
	}
}

static void
seat_handle_capabilities(void *data, struct wl_seat *seat,
			 enum wl_seat_capability caps)
{
	struct input *input = data;

	input->caps = caps;

	/* we will create/update the devices only with the right (test) seat.
	 * If we haven't discovered which seat is the test seat, just
	 * store capabilities and bail out */
	if (input->seat_name && strcmp(input->seat_name, "test-seat") == 0)
		input_update_devices(input);

	testlog("test-client: got seat %p capabilities: %x\n", input, caps);
}

static void
seat_handle_name(void *data, struct wl_seat *seat, const char *name)
{
	struct input *input = data;

	input->seat_name = strdup(name);
	assert(input->seat_name && "No memory");

	/* We only update the devices and set client input for the test seat */
	if (strcmp(name, "test-seat") == 0) {
		assert(!input->client->input &&
		       "Multiple test seats detected!");

		input_update_devices(input);
		input->client->input = input;
	}

	testlog("test-client: got seat %p name: \'%s\'\n", input, name);
}

static const struct wl_seat_listener seat_listener = {
	seat_handle_capabilities,
	seat_handle_name,
};

static void
output_handle_geometry(void *data,
		       struct wl_output *wl_output,
		       int x, int y,
		       int physical_width,
		       int physical_height,
		       int subpixel,
		       const char *make,
		       const char *model,
		       int32_t transform)
{
	struct output *output = data;

	output->x = x;
	output->y = y;
}

static void
output_handle_mode(void *data,
		   struct wl_output *wl_output,
		   uint32_t flags,
		   int width,
		   int height,
		   int refresh)
{
	struct output *output = data;

	if (flags & WL_OUTPUT_MODE_CURRENT) {
		output->width = width;
		output->height = height;
	}
}

static void
output_handle_scale(void *data,
		    struct wl_output *wl_output,
		    int scale)
{
	struct output *output = data;

	output->scale = scale;
}

static void
output_handle_done(void *data,
		   struct wl_output *wl_output)
{
	struct output *output = data;

	output->initialized = 1;
}

static const struct wl_output_listener output_listener = {
	output_handle_geometry,
	output_handle_mode,
	output_handle_done,
	output_handle_scale,
};

static void
output_destroy(struct output *output)
{
	assert(wl_proxy_get_version((struct wl_proxy *)output->wl_output) >= 3);
	wl_output_release(output->wl_output);
	wl_list_remove(&output->link);
	free(output);
}

static void
handle_global(void *data, struct wl_registry *registry,
	      uint32_t id, const char *interface, uint32_t version)
{
	struct client *client = data;
	struct output *output;
	struct test *test;
	struct global *global;
	struct input *input;

	global = xzalloc(sizeof *global);
	global->name = id;
	global->interface = strdup(interface);
	assert(interface);
	global->version = version;
	wl_list_insert(client->global_list.prev, &global->link);

	/* We deliberately bind all globals with the maximum (advertised)
	 * version, because this test suite must be kept up-to-date with
	 * Weston. We must always implement at least the version advertised
	 * by Weston. This is not ok for normal clients, but it is ok in
	 * this test suite.
	 */

	if (strcmp(interface, "wl_compositor") == 0) {
		client->wl_compositor =
			wl_registry_bind(registry, id,
					 &wl_compositor_interface, version);
	} else if (strcmp(interface, "wl_seat") == 0) {
		input = xzalloc(sizeof *input);
		input->client = client;
		input->global_name = global->name;
		input->wl_seat =
			wl_registry_bind(registry, id,
					 &wl_seat_interface, version);
		wl_seat_add_listener(input->wl_seat, &seat_listener, input);
		wl_list_insert(&client->inputs, &input->link);
	} else if (strcmp(interface, "wl_shm") == 0) {
		client->wl_shm =
			wl_registry_bind(registry, id,
					 &wl_shm_interface, version);
		wl_shm_add_listener(client->wl_shm, &shm_listener, client);
	} else if (strcmp(interface, "wl_output") == 0) {
		output = xzalloc(sizeof *output);
		output->wl_output =
			wl_registry_bind(registry, id,
					 &wl_output_interface, version);
		wl_output_add_listener(output->wl_output,
				       &output_listener, output);
		wl_list_insert(&client->output_list, &output->link);
		client->output = output;
	} else if (strcmp(interface, "weston_test") == 0) {
		test = xzalloc(sizeof *test);
		test->weston_test =
			wl_registry_bind(registry, id,
					 &weston_test_interface, version);
		weston_test_add_listener(test->weston_test, &test_listener, test);
		client->test = test;
	} else if (strcmp(interface, "wl_drm") == 0) {
		client->has_wl_drm = true;
	}
}

static struct global *
client_find_global_with_name(struct client *client, uint32_t name)
{
	struct global *global;

	wl_list_for_each(global, &client->global_list, link) {
		if (global->name == name)
			return global;
	}

	return NULL;
}

static struct input *
client_find_input_with_name(struct client *client, uint32_t name)
{
	struct input *input;

	wl_list_for_each(input, &client->inputs, link) {
		if (input->global_name == name)
			return input;
	}

	return NULL;
}

static void
global_destroy(struct global *global)
{
	wl_list_remove(&global->link);
	free(global->interface);
	free(global);
}

static void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	struct client *client = data;
	struct global *global;
	struct input *input;

	global = client_find_global_with_name(client, name);
	assert(global && "Request to remove unknown global");

	if (strcmp(global->interface, "wl_seat") == 0) {
		input = client_find_input_with_name(client, name);
		if (input) {
			if (client->input == input)
				client->input = NULL;
			input_destroy(input);
		}
	}

	/* XXX: handle wl_output */

	global_destroy(global);
}

static const struct wl_registry_listener registry_listener = {
	handle_global,
	handle_global_remove,
};

void
skip(const char *fmt, ...)
{
	va_list argp;

	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);

	/* automake tests uses exit code 77. weston-test-runner will see
	 * this and use it, and then weston-test's sigchld handler (in the
	 * weston process) will use that as an exit status, which is what
	 * ninja will see in the end. */
	exit(77);
}

void
expect_protocol_error(struct client *client,
		      const struct wl_interface *intf,
		      uint32_t code)
{
	int err;
	uint32_t errcode, failed = 0;
	const struct wl_interface *interface;
	unsigned int id;

	/* if the error has not come yet, make it happen */
	wl_display_roundtrip(client->wl_display);

	err = wl_display_get_error(client->wl_display);

	assert(err && "Expected protocol error but nothing came");
	assert(err == EPROTO && "Expected protocol error but got local error");

	errcode = wl_display_get_protocol_error(client->wl_display,
						&interface, &id);

	/* check error */
	if (errcode != code) {
		testlog("Should get error code %d but got %d\n", code, errcode);
		failed = 1;
	}

	/* this should be definitely set */
	assert(interface);

	if (strcmp(intf->name, interface->name) != 0) {
		testlog("Should get interface '%s' but got '%s'\n",
			intf->name, interface->name);
		failed = 1;
	}

	if (failed) {
		testlog("Expected other protocol error\n");
		abort();
	}

	/* all OK */
	testlog("Got expected protocol error on '%s' (object id: %d) "
		"with code %d\n", interface->name, id, errcode);
}

static void
log_handler(const char *fmt, va_list args)
{
	fprintf(stderr, "libwayland: ");
	vfprintf(stderr, fmt, args);
}

struct client *
create_client(void)
{
	struct client *client;

	wl_log_set_handler_client(log_handler);

	/* connect to display */
	client = xzalloc(sizeof *client);
	client->wl_display = wl_display_connect(NULL);
	assert(client->wl_display);
	wl_list_init(&client->global_list);
	wl_list_init(&client->inputs);
	wl_list_init(&client->output_list);

	/* setup registry so we can bind to interfaces */
	client->wl_registry = wl_display_get_registry(client->wl_display);
	wl_registry_add_listener(client->wl_registry, &registry_listener,
				 client);

	/* this roundtrip makes sure we have all globals and we bound to them */
	client_roundtrip(client);
	/* this roundtrip makes sure we got all wl_shm.format and wl_seat.*
	 * events */
	client_roundtrip(client);

	/* must have WL_SHM_FORMAT_ARGB32 */
	assert(client->has_argb);

	/* must have weston_test interface */
	assert(client->test);

	/* must have an output */
	assert(client->output);

	/* the output must be initialized */
	assert(client->output->initialized == 1);

	/* must have seat set */
	assert(client->input);

	return client;
}

struct surface *
create_test_surface(struct client *client)
{
	struct surface *surface;

	surface = xzalloc(sizeof *surface);

	surface->wl_surface =
		wl_compositor_create_surface(client->wl_compositor);
	assert(surface->wl_surface);

	wl_surface_add_listener(surface->wl_surface, &surface_listener,
				surface);

	wl_surface_set_user_data(surface->wl_surface, surface);

	return surface;
}

void
surface_destroy(struct surface *surface)
{
	if (surface->wl_surface)
		wl_surface_destroy(surface->wl_surface);
	if (surface->buffer)
		buffer_destroy(surface->buffer);
	free(surface);
}

struct client *
create_client_and_test_surface(int x, int y, int width, int height)
{
	struct client *client;
	struct surface *surface;
	pixman_color_t color = { 16384, 16384, 16384, 16384 }; /* uint16_t */
	pixman_image_t *solid;

	client = create_client();

	/* initialize the client surface */
	surface = create_test_surface(client);
	client->surface = surface;

	surface->width = width;
	surface->height = height;
	surface->buffer = create_shm_buffer_a8r8g8b8(client, width, height);

	solid = pixman_image_create_solid_fill(&color);
	pixman_image_composite32(PIXMAN_OP_SRC,
				 solid, /* src */
				 NULL, /* mask */
				 surface->buffer->image, /* dst */
				 0, 0, /* src x,y */
				 0, 0, /* mask x,y */
				 0, 0, /* dst x,y */
				 width, height);
	pixman_image_unref(solid);

	move_client(client, x, y);

	return client;
}

void
client_destroy(struct client *client)
{
	if (client->surface)
		surface_destroy(client->surface);

	while (!wl_list_empty(&client->inputs)) {
		input_destroy(container_of(client->inputs.next,
					   struct input, link));
	}

	while (!wl_list_empty(&client->output_list)) {
		output_destroy(container_of(client->output_list.next,
					    struct output, link));
	}

	while (!wl_list_empty(&client->global_list)) {
		global_destroy(container_of(client->global_list.next,
					    struct global, link));
	}

	if (client->test) {
		weston_test_destroy(client->test->weston_test);
		free(client->test);
	}

	if (client->wl_shm)
		wl_shm_destroy(client->wl_shm);
	if (client->wl_compositor)
		wl_compositor_destroy(client->wl_compositor);
	if (client->wl_registry)
		wl_registry_destroy(client->wl_registry);

	client_roundtrip(client);

	if (client->wl_display)
		wl_display_disconnect(client->wl_display);
	free(client);
}

static const char*
output_path(void)
{
	char *path = getenv("WESTON_TEST_OUTPUT_PATH");

	if (!path)
		return ".";

	return path;
}

char*
screenshot_output_filename(const char *basename, uint32_t seq)
{
	char *filename;

	if (asprintf(&filename, "%s/%s-%02d.png",
				 output_path(), basename, seq) < 0)
		return NULL;
	return filename;
}

static const char*
reference_path(void)
{
	char *path = getenv("WESTON_TEST_REFERENCE_PATH");

	if (!path)
		return WESTON_TEST_REFERENCE_PATH;
	return path;
}

char*
screenshot_reference_filename(const char *basename, uint32_t seq)
{
	char *filename;

	if (asprintf(&filename, "%s/%s-%02d.png",
				 reference_path(), basename, seq) < 0)
		return NULL;
	return filename;
}

char *
image_filename(const char *basename)
{
	char *filename;

	if (asprintf(&filename, "%s/%s.png", reference_path(), basename) < 0)
		assert(0);
	return filename;
}

struct format_map_entry {
	cairo_format_t cairo;
	pixman_format_code_t pixman;
};

static const struct format_map_entry format_map[] = {
	{ CAIRO_FORMAT_ARGB32,    PIXMAN_a8r8g8b8 },
	{ CAIRO_FORMAT_RGB24,     PIXMAN_x8r8g8b8 },
	{ CAIRO_FORMAT_A8,        PIXMAN_a8 },
	{ CAIRO_FORMAT_RGB16_565, PIXMAN_r5g6b5 },
};

static pixman_format_code_t
format_cairo2pixman(cairo_format_t fmt)
{
	unsigned i;

	for (i = 0; i < ARRAY_LENGTH(format_map); i++)
		if (format_map[i].cairo == fmt)
			return format_map[i].pixman;

	assert(0 && "unknown Cairo pixel format");
}

static cairo_format_t
format_pixman2cairo(pixman_format_code_t fmt)
{
	unsigned i;

	for (i = 0; i < ARRAY_LENGTH(format_map); i++)
		if (format_map[i].pixman == fmt)
			return format_map[i].cairo;

	assert(0 && "unknown Pixman pixel format");
}

/**
 * Validate range
 *
 * \param r Range to validate or NULL.
 * \return The given range, or {0, 0} for NULL.
 *
 * Will abort if range is invalid, that is a > b.
 */
static struct range
range_get(const struct range *r)
{
	if (!r)
		return (struct range){ 0, 0 };

	assert(r->a <= r->b);
	return *r;
}

/**
 * Compute the ROI for image comparisons
 *
 * \param img_a An image.
 * \param img_b Another image.
 * \param clip_rect Explicit ROI, or NULL for using the whole
 * image area.
 *
 * \return The region of interest (ROI) that is guaranteed to be inside both
 * images.
 *
 * If clip_rect is given, it must fall inside of both images.
 * If clip_rect is NULL, the images must be of the same size.
 * If any precondition is violated, this function aborts with an error.
 *
 * The ROI is given as pixman_box32_t, where x2,y2 are non-inclusive.
 */
static pixman_box32_t
image_check_get_roi(pixman_image_t *img_a, pixman_image_t *img_b,
		   const struct rectangle *clip_rect)
{
	int width_a;
	int width_b;
	int height_a;
	int height_b;
	pixman_box32_t box;

	width_a = pixman_image_get_width(img_a);
	height_a = pixman_image_get_height(img_a);

	width_b = pixman_image_get_width(img_b);
	height_b = pixman_image_get_height(img_b);

	if (clip_rect) {
		box.x1 = clip_rect->x;
		box.y1 = clip_rect->y;
		box.x2 = clip_rect->x + clip_rect->width;
		box.y2 = clip_rect->y + clip_rect->height;
	} else {
		box.x1 = 0;
		box.y1 = 0;
		box.x2 = max(width_a, width_b);
		box.y2 = max(height_a, height_b);
	}

	assert(box.x1 >= 0);
	assert(box.y1 >= 0);
	assert(box.x2 > box.x1);
	assert(box.y2 > box.y1);
	assert(box.x2 <= width_a);
	assert(box.x2 <= width_b);
	assert(box.y2 <= height_a);
	assert(box.y2 <= height_b);

	return box;
}

struct image_iterator {
	char *data;
	int stride; /* bytes */
};

static void
image_iter_init(struct image_iterator *it, pixman_image_t *image)
{
	pixman_format_code_t fmt;

	it->stride = pixman_image_get_stride(image);
	it->data = (void *)pixman_image_get_data(image);

	fmt = pixman_image_get_format(image);
	assert(PIXMAN_FORMAT_BPP(fmt) == 32);
}

static uint32_t *
image_iter_get_row(struct image_iterator *it, int y)
{
	return (uint32_t *)(it->data + y * it->stride);
}

struct pixel_diff_stat {
	struct pixel_diff_stat_channel {
		int min_diff;
		int max_diff;
	} ch[4];
};

static void
testlog_pixel_diff_stat(const struct pixel_diff_stat *stat)
{
	int i;

	testlog("Image difference statistics:\n");
	for (i = 0; i < 4; i++) {
		testlog("\tch %d: [%d, %d]\n",
			i, stat->ch[i].min_diff, stat->ch[i].max_diff);
	}
}

static bool
fuzzy_match_pixels(uint32_t pix_a, uint32_t pix_b,
		   const struct range *fuzz,
		   struct pixel_diff_stat *stat)
{
	bool ret = true;
	int shift;
	int i;

	for (shift = 0, i = 0; i < 4; shift += 8, i++) {
		int val_a = (pix_a >> shift) & 0xffu;
		int val_b = (pix_b >> shift) & 0xffu;
		int d = val_b - val_a;

		stat->ch[i].min_diff = min(stat->ch[i].min_diff, d);
		stat->ch[i].max_diff = max(stat->ch[i].max_diff, d);

		if (d < fuzz->a || d > fuzz->b)
			ret = false;
	}

	return ret;
}

/**
 * Test if a given region within two images are pixel-identical
 *
 * Returns true if the two images pixel-wise identical, and false otherwise.
 *
 * \param img_a First image.
 * \param img_b Second image.
 * \param clip_rect The region of interest, or NULL for comparing the whole
 * images.
 * \param prec Per-channel allowed difference, or NULL for identical match
 * required.
 *
 * This function hard-fails if clip_rect is not inside both images. If clip_rect
 * is given, the images do not have to match in size, otherwise size mismatch
 * will be a hard failure.
 *
 * The per-pixel, per-channel difference is computed as img_b - img_a which is
 * required to be in the range [prec->a, prec->b] inclusive. The difference is
 * signed. All four channels are compared the same way, without any special
 * meaning on alpha channel.
 */
bool
check_images_match(pixman_image_t *img_a, pixman_image_t *img_b,
		   const struct rectangle *clip_rect, const struct range *prec)
{
	struct range fuzz = range_get(prec);
	struct pixel_diff_stat diffstat = {};
	struct image_iterator it_a;
	struct image_iterator it_b;
	pixman_box32_t box;
	int x, y;
	uint32_t *pix_a;
	uint32_t *pix_b;

	box = image_check_get_roi(img_a, img_b, clip_rect);

	image_iter_init(&it_a, img_a);
	image_iter_init(&it_b, img_b);

	for (y = box.y1; y < box.y2; y++) {
		pix_a = image_iter_get_row(&it_a, y) + box.x1;
		pix_b = image_iter_get_row(&it_b, y) + box.x1;

		for (x = box.x1; x < box.x2; x++) {
			if (!fuzzy_match_pixels(*pix_a, *pix_b,
						&fuzz, &diffstat))
				return false;

			pix_a++;
			pix_b++;
		}
	}

	return true;
}

/**
 * Tint a color
 *
 * \param src Source pixel as x8r8g8b8.
 * \param add The tint as x8r8g8b8, x8 must be zero; r8, g8 and b8 must be
 * no greater than 0xc0 to avoid overflow to another channel.
 * \return The tinted pixel color as x8r8g8b8, x8 guaranteed to be 0xff.
 *
 * The source pixel RGB values are divided by 4, and then the tint is added.
 * To achieve colors outside of the range of src, a tint color channel must be
 * at least 0x40. (0xff / 4 = 0x3f, 0xff - 0x3f = 0xc0)
 */
static uint32_t
tint(uint32_t src, uint32_t add)
{
	uint32_t v;

	v = ((src & 0xfcfcfcfc) >> 2) | 0xff000000;

	return v + add;
}

/**
 * Create a visualization of image differences.
 *
 * \param img_a First image, which is used as the basis for the output.
 * \param img_b Second image.
 * \param clip_rect The region of interest, or NULL for comparing the whole
 * images.
 * \param prec Per-channel allowed difference, or NULL for identical match
 * required.
 * \return A new image with the differences highlighted.
 *
 * Regions outside of the region of interest are shaded with black, matching
 * pixels are shaded with green, and differing pixels are shaded with
 * bright red.
 *
 * This function hard-fails if clip_rect is not inside both images. If clip_rect
 * is given, the images do not have to match in size, otherwise size mismatch
 * will be a hard failure.
 *
 * The per-pixel, per-channel difference is computed as img_b - img_a which is
 * required to be in the range [prec->a, prec->b] inclusive. The difference is
 * signed. All four channels are compared the same way, without any special
 * meaning on alpha channel.
 */
pixman_image_t *
visualize_image_difference(pixman_image_t *img_a, pixman_image_t *img_b,
			   const struct rectangle *clip_rect,
			   const struct range *prec)
{
	struct range fuzz = range_get(prec);
	struct pixel_diff_stat diffstat = {};
	pixman_image_t *diffimg;
	pixman_image_t *shade;
	struct image_iterator it_a;
	struct image_iterator it_b;
	struct image_iterator it_d;
	int width;
	int height;
	pixman_box32_t box;
	int x, y;
	uint32_t *pix_a;
	uint32_t *pix_b;
	uint32_t *pix_d;
	pixman_color_t shade_color = { 0, 0, 0, 32768 };

	width = pixman_image_get_width(img_a);
	height = pixman_image_get_height(img_a);
	box = image_check_get_roi(img_a, img_b, clip_rect);

	diffimg = pixman_image_create_bits_no_clear(PIXMAN_x8r8g8b8,
						    width, height, NULL, 0);

	/* Fill diffimg with a black-shaded copy of img_a, and then fill
	 * the clip_rect area with original img_a.
	 */
	shade = pixman_image_create_solid_fill(&shade_color);
	pixman_image_composite32(PIXMAN_OP_SRC, img_a, shade, diffimg,
				 0, 0, 0, 0, 0, 0, width, height);
	pixman_image_unref(shade);
	pixman_image_composite32(PIXMAN_OP_SRC, img_a, NULL, diffimg,
				 box.x1, box.y1, 0, 0, box.x1, box.y1,
				 box.x2 - box.x1, box.y2 - box.y1);

	image_iter_init(&it_a, img_a);
	image_iter_init(&it_b, img_b);
	image_iter_init(&it_d, diffimg);

	for (y = box.y1; y < box.y2; y++) {
		pix_a = image_iter_get_row(&it_a, y) + box.x1;
		pix_b = image_iter_get_row(&it_b, y) + box.x1;
		pix_d = image_iter_get_row(&it_d, y) + box.x1;

		for (x = box.x1; x < box.x2; x++) {
			if (fuzzy_match_pixels(*pix_a, *pix_b,
					       &fuzz, &diffstat))
				*pix_d = tint(*pix_d, 0x00008000); /* green */
			else
				*pix_d = tint(*pix_d, 0x00c00000); /* red */

			pix_a++;
			pix_b++;
			pix_d++;
		}
	}

	testlog_pixel_diff_stat(&diffstat);

	return diffimg;
}

/**
 * Write an image into a PNG file.
 *
 * \param image The image.
 * \param fname The name and path for the file.
 *
 * \returns true if successfully saved file; false otherwise.
 *
 * \note Only image formats directly supported by Cairo are accepted, not all
 * Pixman formats.
 */
bool
write_image_as_png(pixman_image_t *image, const char *fname)
{
	cairo_surface_t *cairo_surface;
	cairo_status_t status;
	cairo_format_t fmt;

	fmt = format_pixman2cairo(pixman_image_get_format(image));

	cairo_surface = cairo_image_surface_create_for_data(
			(void *)pixman_image_get_data(image),
			fmt,
			pixman_image_get_width(image),
			pixman_image_get_height(image),
			pixman_image_get_stride(image));

	status = cairo_surface_write_to_png(cairo_surface, fname);
	if (status != CAIRO_STATUS_SUCCESS) {
		testlog("Failed to save image '%s': %s\n", fname,
			cairo_status_to_string(status));

		return false;
	}

	cairo_surface_destroy(cairo_surface);

	return true;
}

static pixman_image_t *
image_convert_to_a8r8g8b8(pixman_image_t *image)
{
	pixman_image_t *ret;
	int width;
	int height;

	if (pixman_image_get_format(image) == PIXMAN_a8r8g8b8)
		return pixman_image_ref(image);

	width = pixman_image_get_width(image);
	height = pixman_image_get_height(image);

	ret = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, width, height,
						NULL, 0);
	assert(ret);

	pixman_image_composite32(PIXMAN_OP_SRC, image, NULL, ret,
				 0, 0, 0, 0, 0, 0, width, height);

	return ret;
}

static void
destroy_cairo_surface(pixman_image_t *image, void *data)
{
	cairo_surface_t *surface = data;

	cairo_surface_destroy(surface);
}

/**
 * Load an image from a PNG file
 *
 * Reads a PNG image from disk using the given filename (and path)
 * and returns as a Pixman image. Use pixman_image_unref() to free it.
 *
 * The returned image is always in PIXMAN_a8r8g8b8 format.
 *
 * @returns Pixman image, or NULL in case of error.
 */
pixman_image_t *
load_image_from_png(const char *fname)
{
	pixman_image_t *image;
	pixman_image_t *converted;
	cairo_format_t cairo_fmt;
	pixman_format_code_t pixman_fmt;
	cairo_surface_t *reference_cairo_surface;
	cairo_status_t status;
	int width;
	int height;
	int stride;
	void *data;

	reference_cairo_surface = cairo_image_surface_create_from_png(fname);
	cairo_surface_flush(reference_cairo_surface);
	status = cairo_surface_status(reference_cairo_surface);
	if (status != CAIRO_STATUS_SUCCESS) {
		testlog("Could not open %s: %s\n", fname,
			cairo_status_to_string(status));
		cairo_surface_destroy(reference_cairo_surface);
		return NULL;
	}

	cairo_fmt = cairo_image_surface_get_format(reference_cairo_surface);
	pixman_fmt = format_cairo2pixman(cairo_fmt);

	width = cairo_image_surface_get_width(reference_cairo_surface);
	height = cairo_image_surface_get_height(reference_cairo_surface);
	stride = cairo_image_surface_get_stride(reference_cairo_surface);
	data = cairo_image_surface_get_data(reference_cairo_surface);

	/* The Cairo surface will own the data, so we keep it around. */
	image = pixman_image_create_bits_no_clear(pixman_fmt,
						  width, height, data, stride);
	assert(image);

	pixman_image_set_destroy_function(image, destroy_cairo_surface,
					  reference_cairo_surface);

	converted = image_convert_to_a8r8g8b8(image);
	pixman_image_unref(image);

	return converted;
}

/**
 * Take screenshot of a single output
 *
 * Requests a screenshot from the server of the output that the
 * client appears on. This implies that the compositor goes through an output
 * repaint to provide the screenshot before this function returns. This
 * function is therefore both a server roundtrip and a wait for a repaint.
 *
 * @returns A new buffer object, that should be freed with buffer_destroy().
 */
struct buffer *
capture_screenshot_of_output(struct client *client)
{
	struct buffer *buffer;

	buffer = create_shm_buffer_a8r8g8b8(client,
					    client->output->width,
					    client->output->height);

	client->test->buffer_copy_done = 0;
	weston_test_capture_screenshot(client->test->weston_test,
				       client->output->wl_output,
				       buffer->proxy);
	while (client->test->buffer_copy_done == 0)
		if (wl_display_dispatch(client->wl_display) < 0)
			break;

	/* FIXME: Document somewhere the orientation the screenshot is taken
	 * and how the clip coords are interpreted, in case of scaling/transform.
	 * If we're using read_pixels() just make sure it is documented somewhere.
	 * Protocol docs in the XML, comparison function docs in Doxygen style.
	 */

	return buffer;
}

static void
write_visual_diff(pixman_image_t *ref_image,
		  struct buffer *shot,
		  const struct rectangle *clip,
		  const char *test_name,
		  int seq_no,
		  const struct range *fuzz)
{
	char *fname;
	char *ext_test_name;
	pixman_image_t *diff;
	int ret;

	ret = asprintf(&ext_test_name, "%s-diff", test_name);
	assert(ret >= 0);

	fname = screenshot_output_filename(ext_test_name, seq_no);
	diff = visualize_image_difference(ref_image, shot->image, clip, fuzz);
	write_image_as_png(diff, fname);

	pixman_image_unref(diff);
	free(fname);
	free(ext_test_name);
}

/**
 * Take a screenshot and verify its contents
 *
 * Takes a screenshot and writes the image into a PNG file named with
 * get_test_name() and seq_no. Compares the contents to the given reference
 * image over the given clip rectangle, reports whether they match to the
 * test log, and if they do not match writes a visual diff into a PNG file.
 *
 * The compositor output size and the reference image size must both contain
 * the clip rectangle.
 *
 * This function uses the pixel value allowed fuzz approriate for GL-renderer
 * with 8 bits per channel data.
 *
 * \param client The client, for connecting to the compositor.
 * \param ref_image The reference image file basename, without sequence number
 * and .png suffix.
 * \param ref_seq_no The reference image sequence number.
 * \param clip The region of interest, or NULL for comparing the whole
 * images.
 * \param seq_no Test sequence number, for writing output files.
 * \return True if the screen contents matches the reference image,
 * false otherwise.
 *
 * For bootstrapping, ref_image can be NULL or the file can be missing.
 * In that case the screenshot file is written but no comparison is performed,
 * and false is returned.
 */
bool
verify_screen_content(struct client *client,
		      const char *ref_image,
		      int ref_seq_no,
		      const struct rectangle *clip,
		      int seq_no)
{
	const char *test_name = get_test_name();
	const struct range gl_fuzz = { -3, 4 };
	struct buffer *shot;
	pixman_image_t *ref = NULL;
	char *ref_fname = NULL;
	char *shot_fname;
	bool match;

	shot = capture_screenshot_of_output(client);
	assert(shot);
	shot_fname = screenshot_output_filename(test_name, seq_no);
	write_image_as_png(shot->image, shot_fname);

	if (ref_image) {
		ref_fname = screenshot_reference_filename(ref_image, ref_seq_no);
		ref = load_image_from_png(ref_fname);
	}

	if (ref) {
		match = check_images_match(ref, shot->image, clip, &gl_fuzz);
		testlog("Verify reference image %s vs. shot %s: %s\n",
			ref_fname, shot_fname, match ? "PASS" : "FAIL");

		if (!match) {
			write_visual_diff(ref, shot, clip,
					  test_name, seq_no, &gl_fuzz);
		}

		pixman_image_unref(ref);
	} else {
		testlog("No reference image, shot %s: FAIL\n", shot_fname);
		match = false;
	}

	free(ref_fname);
	buffer_destroy(shot);
	free(shot_fname);

	return match;
}

/**
 * Create a wl_buffer from a PNG file
 *
 * Loads the named PNG file from the directory of reference images,
 * creates a wl_buffer with scale times the image dimensions in pixels,
 * and copies the image content into the buffer using nearest-neighbor filter.
 *
 * \param client The client, for the Wayland connection.
 * \param basename The PNG file name without .png suffix.
 * \param scale Upscaling factor >= 1.
 */
struct buffer *
client_buffer_from_image_file(struct client *client,
			      const char *basename,
			      int scale)
{
	struct buffer *buf;
	char *fname;
	pixman_image_t *img;
	int buf_w, buf_h;
	pixman_transform_t scaling;

	assert(scale >= 1);

	fname = image_filename(basename);
	img = load_image_from_png(fname);
	free(fname);
	assert(img);

	buf_w = scale * pixman_image_get_width(img);
	buf_h = scale * pixman_image_get_height(img);
	buf = create_shm_buffer_a8r8g8b8(client, buf_w, buf_h);

	pixman_transform_init_scale(&scaling,
				    pixman_fixed_1 / scale,
				    pixman_fixed_1 / scale);
	pixman_image_set_transform(img, &scaling);
	pixman_image_set_filter(img, PIXMAN_FILTER_NEAREST, NULL, 0);

	pixman_image_composite32(PIXMAN_OP_SRC,
				 img, /* src */
				 NULL, /* mask */
				 buf->image, /* dst */
				 0, 0, /* src x,y */
				 0, 0, /* mask x,y */
				 0, 0, /* dst x,y */
				 buf_w, buf_h);
	pixman_image_unref(img);

	return buf;
}

/**
 * Bind to a singleton global in wl_registry
 *
 * \param client Client whose registry and globals to use.
 * \param iface The Wayland interface to look for.
 * \param version The version to bind the interface with.
 * \return A struct wl_proxy, which you need to cast to the proper type.
 *
 * Asserts that the global being searched for is a singleton and is found.
 *
 * Binds with the exact version given, does not take compositor interface
 * version into account.
 */
void *
bind_to_singleton_global(struct client *client,
			 const struct wl_interface *iface,
			 int version)
{
	struct global *tmp;
	struct global *g = NULL;
	struct wl_proxy *proxy;

	wl_list_for_each(tmp, &client->global_list, link) {
		if (strcmp(tmp->interface, iface->name))
			continue;

		assert(!g && "multiple singleton objects");
		g = tmp;
	}

	assert(g && "singleton not found");

	proxy = wl_registry_bind(client->wl_registry, g->name, iface, version);
	assert(proxy);

	return proxy;
}

/**
 * Create a wp_viewport for the client surface
 *
 * \param client The client->surface to use.
 * \return A fresh viewport object.
 */
struct wp_viewport *
client_create_viewport(struct client *client)
{
	struct wp_viewporter *viewporter;
	struct wp_viewport *viewport;

	viewporter = bind_to_singleton_global(client,
					      &wp_viewporter_interface, 1);
	viewport = wp_viewporter_get_viewport(viewporter,
					      client->surface->wl_surface);
	assert(viewport);
	wp_viewporter_destroy(viewporter);

	return viewport;
}

/**
 * Fill the image with the given color
 *
 * \param image The image to write to.
 * \param color The color to use.
 */
void
fill_image_with_color(pixman_image_t *image, pixman_color_t *color)
{
	pixman_image_t *solid;
	int width;
	int height;

	width = pixman_image_get_width(image);
	height = pixman_image_get_height(image);

	solid = pixman_image_create_solid_fill(color);
	pixman_image_composite32(PIXMAN_OP_SRC,
				 solid, /* src */
				 NULL, /* mask */
				 image, /* dst */
				 0, 0, /* src x,y */
				 0, 0, /* mask x,y */
				 0, 0, /* dst x,y */
				 width, height);
	pixman_image_unref(solid);
}

/**
 * Convert 8-bit RGB to opaque Pixman color
 *
 * \param tmp Pixman color struct to fill in.
 * \param r Red value, 0 - 255.
 * \param g Green value, 0 - 255.
 * \param b Blue value, 0 - 255.
 * \return tmp
 */
pixman_color_t *
color_rgb888(pixman_color_t *tmp, uint8_t r, uint8_t g, uint8_t b)
{
	tmp->alpha = 65535;
	tmp->red = (r << 8) + r;
	tmp->green = (g << 8) + g;
	tmp->blue = (b << 8) + b;

	return tmp;
}
