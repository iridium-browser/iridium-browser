/*
 * Copyright 2017-2018 Collabora, Ltd.
 * Copyright 2017-2018 General Electric Company
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

#include <string.h>
#include <wayland-server.h>

#include "shared/helpers.h"
#include "shared/string-helpers.h"
#include <libweston/zalloc.h>
#include "shared/timespec-util.h"
#include <libweston/libweston.h>
#include "libweston-internal.h"
#include "backend.h"

#include "weston-touch-calibration-server-protocol.h"

struct weston_touch_calibrator {
	struct wl_resource *resource;

	struct weston_compositor *compositor;

	struct weston_surface *surface;
	struct wl_listener surface_destroy_listener;
	struct wl_listener surface_commit_listener;

	struct weston_touch_device *device;
	struct wl_listener device_destroy_listener;

	struct weston_output *output;
	struct wl_listener output_destroy_listener;

	struct weston_view *view;

	/** The calibration procedure has been cancelled. */
	bool calibration_cancelled;

	/** The current touch sequence has been cancelled. */
	bool touch_cancelled;
};

static struct weston_touch_calibrator *
calibrator_from_device(struct weston_touch_device *device)
{
	return device->aggregate->seat->compositor->touch_calibrator;
}

static uint32_t
wire_uint_from_double(double c)
{
	assert(c >= 0.0);
	assert(c <= 1.0);

	return round(c * 0xffffffff);
}

static bool
normalized_is_valid(const struct weston_point2d_device_normalized *p)
{
	return p->x >= 0.0 && p->x <= 1.0 &&
	       p->y >= 0.0 && p->y <= 1.0;
}

WL_EXPORT void
notify_touch_calibrator(struct weston_touch_device *device,
			const struct timespec *time, int32_t slot,
			const struct weston_point2d_device_normalized *norm,
			int touch_type)
{
	struct weston_touch_calibrator *calibrator;
	struct wl_resource *res;
	uint32_t msecs;
	uint32_t x = 0;
	uint32_t y = 0;

	calibrator = calibrator_from_device(device);
	if (!calibrator)
		return;

	res = calibrator->resource;

	/* Ignore any touch events coming from another device */
	if (device != calibrator->device) {
		if (touch_type == WL_TOUCH_DOWN)
			weston_touch_calibrator_send_invalid_touch(res);
		return;
	}

	/* Ignore all events if we have sent 'cancel' event until all
	 * touches (on the seat) are up.
	 */
	if (calibrator->touch_cancelled) {
		if (calibrator->device->aggregate->num_tp == 0) {
			assert(touch_type == WL_TOUCH_UP);
			calibrator->touch_cancelled = false;
		}
		return;
	}

	msecs = timespec_to_msec(time);
	if (touch_type != WL_TOUCH_UP) {
		if (normalized_is_valid(norm)) {
			x = wire_uint_from_double(norm->x);
			y = wire_uint_from_double(norm->y);
		} else {
			/* Coordinates are out of bounds */
			if (touch_type == WL_TOUCH_MOTION) {
				weston_touch_calibrator_send_cancel(res);
				calibrator->touch_cancelled = true;
			}
			weston_touch_calibrator_send_invalid_touch(res);
			return;
		}
	}

	switch (touch_type) {
	case WL_TOUCH_UP:
		weston_touch_calibrator_send_up(res, msecs, slot);
		break;
	case WL_TOUCH_DOWN:
		weston_touch_calibrator_send_down(res, msecs, slot, x, y);
		break;
	case WL_TOUCH_MOTION:
		weston_touch_calibrator_send_motion(res, msecs, slot, x, y);
		break;
	default:
		return;
	}
}

WL_EXPORT void
notify_touch_calibrator_frame(struct weston_touch_device *device)
{
	struct weston_touch_calibrator *calibrator;

	calibrator = calibrator_from_device(device);
	if (!calibrator)
		return;

	weston_touch_calibrator_send_frame(calibrator->resource);
}

WL_EXPORT void
notify_touch_calibrator_cancel(struct weston_touch_device *device)
{
	struct weston_touch_calibrator *calibrator;

	calibrator = calibrator_from_device(device);
	if (!calibrator)
		return;

	weston_touch_calibrator_send_cancel(calibrator->resource);
}

static void
map_calibrator(struct weston_touch_calibrator *calibrator)
{
	struct weston_compositor *c = calibrator->compositor;
	struct weston_touch_device *device = calibrator->device;
	static const struct weston_touch_device_matrix identity = {
		.m = { 1, 0, 0, 0, 1, 0}
	};

	assert(!calibrator->view);
	assert(calibrator->output);
	assert(calibrator->surface);
	assert(calibrator->surface->resource);

	calibrator->view = weston_view_create(calibrator->surface);
	if (!calibrator->view) {
		wl_resource_post_no_memory(calibrator->surface->resource);
		return;
	}

	weston_layer_entry_insert(&c->calibrator_layer.view_list,
				  &calibrator->view->layer_link);

	weston_view_set_position(calibrator->view,
				 calibrator->output->x,
				 calibrator->output->y);
	calibrator->view->output = calibrator->surface->output;
	calibrator->view->is_mapped = true;

	calibrator->surface->output = calibrator->output;
	weston_surface_map(calibrator->surface);

	weston_output_schedule_repaint(calibrator->output);

	device->ops->get_calibration(device, &device->saved_calibration);
	device->ops->set_calibration(device, &identity);
}

static void
unmap_calibrator(struct weston_touch_calibrator *calibrator)
{
	struct weston_touch_device *device = calibrator->device;

	wl_list_remove(&calibrator->surface_commit_listener.link);
	wl_list_init(&calibrator->surface_commit_listener.link);

	if (!calibrator->view)
		return;

	weston_view_destroy(calibrator->view);
	calibrator->view = NULL;
	weston_surface_unmap(calibrator->surface);

	/* Reload saved calibration */
	if (device)
		device->ops->set_calibration(device,
					     &device->saved_calibration);
}

void
touch_calibrator_mode_changed(struct weston_compositor *compositor)
{
	struct weston_touch_calibrator *calibrator;

	calibrator = compositor->touch_calibrator;
	if (!calibrator)
		return;

	if (calibrator->calibration_cancelled)
		return;

	if (compositor->touch_mode == WESTON_TOUCH_MODE_CALIB)
		map_calibrator(calibrator);
}

static void
touch_calibrator_surface_committed(struct wl_listener *listener, void *data)
{
	struct weston_touch_calibrator *calibrator =
		container_of(listener, struct weston_touch_calibrator,
			     surface_commit_listener);
	struct weston_surface *surface = calibrator->surface;

	wl_list_remove(&calibrator->surface_commit_listener.link);
	wl_list_init(&calibrator->surface_commit_listener.link);

	if (surface->width != calibrator->output->width ||
	    surface->height != calibrator->output->height) {
		wl_resource_post_error(calibrator->resource,
				       WESTON_TOUCH_CALIBRATOR_ERROR_BAD_SIZE,
				       "calibrator surface size does not match");
		return;
	}

	weston_compositor_set_touch_mode_calib(calibrator->compositor);
	/* results in call to touch_calibrator_mode_changed() */
}

static void
touch_calibrator_convert(struct wl_client *client,
			 struct wl_resource *resource,
			 int32_t x,
			 int32_t y,
			 uint32_t coordinate_id)
{
	struct weston_touch_calibrator *calibrator;
	struct wl_resource *coordinate_resource;
	struct weston_output *output;
	struct weston_surface *surface;
	uint32_t version;
	struct weston_coord_surface ps;
	struct weston_coord_global pg;
	struct weston_point2d_device_normalized norm;

	version = wl_resource_get_version(resource);
	calibrator = wl_resource_get_user_data(resource);
	surface = calibrator->surface;
	output = calibrator->output;

	coordinate_resource =
		wl_resource_create(client, &weston_touch_coordinate_interface,
				   version, coordinate_id);
	if (!coordinate_resource) {
		wl_client_post_no_memory(client);
		return;
	}

	if (calibrator->calibration_cancelled) {
		weston_touch_coordinate_send_result(coordinate_resource, 0, 0);
		wl_resource_destroy(coordinate_resource);
		return;
	}

	if (!surface || !surface->is_mapped) {
		wl_resource_post_error(resource,
				       WESTON_TOUCH_CALIBRATOR_ERROR_NOT_MAPPED,
				       "calibrator surface is not mapped");
		return;
	}
	assert(calibrator->view);
	assert(output);

	if (x < 0 || y < 0 || x >= surface->width || y >= surface->height) {
		wl_resource_post_error(resource,
				       WESTON_TOUCH_CALIBRATOR_ERROR_BAD_COORDINATES,
				       "convert(%d, %d) input is out of bounds",
				       x, y);
		return;
	}

	/* Convert from surface-local coordinates into global, from global
	 * into output-raw, do perspective division and normalize.
	 */
	ps = weston_coord_surface(x, y, calibrator->view->surface);
	pg = weston_coord_surface_to_global(calibrator->view, ps);
	pg.c = weston_matrix_transform_coord(&output->matrix, pg.c);
	norm.x = pg.c.x / output->current_mode->width;
	norm.y = pg.c.y / output->current_mode->height;

	if (!normalized_is_valid(&norm)) {
		wl_resource_post_error(resource,
				       WESTON_TOUCH_CALIBRATOR_ERROR_BAD_COORDINATES,
				       "convert(%d, %d) output is out of bounds",
				       x, y);
		return;
	}

	weston_touch_coordinate_send_result(coordinate_resource,
					    wire_uint_from_double(norm.x),
					    wire_uint_from_double(norm.y));
	wl_resource_destroy(coordinate_resource);
}

static void
destroy_touch_calibrator(struct wl_resource *resource)
{
	struct weston_touch_calibrator *calibrator;

	calibrator = wl_resource_get_user_data(resource);

	calibrator->compositor->touch_calibrator = NULL;

	weston_compositor_set_touch_mode_normal(calibrator->compositor);

	if (calibrator->surface) {
		unmap_calibrator(calibrator);
		wl_list_remove(&calibrator->surface_destroy_listener.link);
		wl_list_remove(&calibrator->surface_commit_listener.link);
	}

	if (calibrator->device)
		wl_list_remove(&calibrator->device_destroy_listener.link);

	if (calibrator->output)
		wl_list_remove(&calibrator->output_destroy_listener.link);

	free(calibrator);
}

static void
touch_calibrator_destroy(struct wl_client *client,
			 struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct weston_touch_calibrator_interface
touch_calibrator_implementation = {
	touch_calibrator_destroy,
	touch_calibrator_convert
};

static void
touch_calibrator_cancel_calibration(struct weston_touch_calibrator *calibrator)
{
	weston_touch_calibrator_send_cancel_calibration(calibrator->resource);
	calibrator->calibration_cancelled = true;

	if (calibrator->surface)
		unmap_calibrator(calibrator);
}

static void
touch_calibrator_output_destroyed(struct wl_listener *listener, void *data)
{
	struct weston_touch_calibrator *calibrator =
		container_of(listener, struct weston_touch_calibrator,
			     output_destroy_listener);

	assert(calibrator->output == data);
	calibrator->output = NULL;

	touch_calibrator_cancel_calibration(calibrator);
}

static void
touch_calibrator_device_destroyed(struct wl_listener *listener, void *data)
{
	struct weston_touch_calibrator *calibrator =
		container_of(listener, struct weston_touch_calibrator,
			     device_destroy_listener);

	assert(calibrator->device == data);
	calibrator->device = NULL;

	touch_calibrator_cancel_calibration(calibrator);
}

static void
touch_calibrator_surface_destroyed(struct wl_listener *listener, void *data)
{
	struct weston_touch_calibrator *calibrator =
		container_of(listener, struct weston_touch_calibrator,
			     surface_destroy_listener);

	assert(calibrator->surface->resource == data);

	unmap_calibrator(calibrator);
	calibrator->surface = NULL;
}

static void
touch_calibration_destroy(struct wl_client *client,
			  struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static struct weston_touch_device *
weston_compositor_find_touch_device_by_syspath(
	struct weston_compositor *compositor,
	const char *syspath)
{
	struct weston_seat *seat;
	struct weston_touch *touch;
	struct weston_touch_device *device;

	if (!syspath)
		return NULL;

	wl_list_for_each(seat, &compositor->seat_list, link) {
		touch = weston_seat_get_touch(seat);
		if (!touch)
			continue;

		wl_list_for_each(device, &touch->device_list, link) {
			if (strcmp(device->syspath, syspath) == 0)
				return device;
		}
	}

	return NULL;
}

static void
touch_calibration_create_calibrator(
	struct wl_client *client,
	struct wl_resource *touch_calibration_resource,
	struct wl_resource *surface_resource,
	const char *syspath,
	uint32_t calibrator_id)
{
	struct weston_compositor *compositor;
	struct weston_touch_calibrator *calibrator;
	struct weston_touch_device *device;
	struct weston_output *output = NULL;
	struct weston_surface *surface;
	uint32_t version;
	int ret;

	version = wl_resource_get_version(touch_calibration_resource);
	compositor = wl_resource_get_user_data(touch_calibration_resource);

	if (compositor->touch_calibrator != NULL) {
		wl_resource_post_error(touch_calibration_resource,
				WESTON_TOUCH_CALIBRATION_ERROR_ALREADY_EXISTS,
				"a calibrator has already been created");
		return;
	}

	calibrator = zalloc(sizeof *calibrator);
	if (!calibrator) {
		wl_client_post_no_memory(client);
		return;
	}

	calibrator->compositor = compositor;
	calibrator->resource = wl_resource_create(client,
					&weston_touch_calibrator_interface,
					version, calibrator_id);
	if (!calibrator->resource) {
		wl_client_post_no_memory(client);
		goto err_dealloc;
	}

	surface = wl_resource_get_user_data(surface_resource);
	assert(surface);
	ret = weston_surface_set_role(surface, "weston_touch_calibrator",
		touch_calibration_resource,
		WESTON_TOUCH_CALIBRATION_ERROR_INVALID_SURFACE);
	if (ret < 0)
		goto err_destroy_resource;

	calibrator->surface_destroy_listener.notify =
		touch_calibrator_surface_destroyed;
	wl_resource_add_destroy_listener(surface->resource,
					 &calibrator->surface_destroy_listener);
	calibrator->surface = surface;

	calibrator->surface_commit_listener.notify =
		touch_calibrator_surface_committed;
	wl_signal_add(&surface->commit_signal,
		      &calibrator->surface_commit_listener);

	device = weston_compositor_find_touch_device_by_syspath(compositor,
								syspath);
	if (device) {
		output = device->ops->get_output(device);
		if (weston_touch_device_can_calibrate(device) && output)
			calibrator->device = device;
	}

	if (!calibrator->device) {
		wl_resource_post_error(touch_calibration_resource,
				WESTON_TOUCH_CALIBRATION_ERROR_INVALID_DEVICE,
				"the given touch device '%s' is not valid",
				syspath ?: "");
		goto err_unlink_surface;
	}

	calibrator->device_destroy_listener.notify =
		touch_calibrator_device_destroyed;
	wl_signal_add(&calibrator->device->destroy_signal,
		      &calibrator->device_destroy_listener);

	wl_resource_set_implementation(calibrator->resource,
				       &touch_calibrator_implementation,
				       calibrator, destroy_touch_calibrator);

	assert(output);
	calibrator->output_destroy_listener.notify =
		touch_calibrator_output_destroyed;
	wl_signal_add(&output->destroy_signal,
		      &calibrator->output_destroy_listener);
	calibrator->output = output;

	weston_touch_calibrator_send_configure(calibrator->resource,
					       output->width,
					       output->height);

	compositor->touch_calibrator = calibrator;

	return;

err_unlink_surface:
	wl_list_remove(&calibrator->surface_commit_listener.link);
	wl_list_remove(&calibrator->surface_destroy_listener.link);

err_destroy_resource:
	wl_resource_destroy(calibrator->resource);

err_dealloc:
	free(calibrator);
}

static void
touch_calibration_save(struct wl_client *client,
		       struct wl_resource *touch_calibration_resource,
		       const char *device_name,
		       struct wl_array *matrix_data)
{
	struct weston_touch_device *device;
	struct weston_compositor *compositor;
	struct weston_touch_device_matrix calibration;
	struct weston_touch_calibrator *calibrator;
	int i = 0;
	float *c;

	compositor = wl_resource_get_user_data(touch_calibration_resource);

	device = weston_compositor_find_touch_device_by_syspath(compositor,
								device_name);
	if (!device || !weston_touch_device_can_calibrate(device)) {
		wl_resource_post_error(touch_calibration_resource,
				WESTON_TOUCH_CALIBRATION_ERROR_INVALID_DEVICE,
				"the given device is not valid");
		return;
	}

	wl_array_for_each(c, matrix_data) {
		calibration.m[i++] = *c;
	}

	/* If calibration can't be saved, don't set it as current */
	if (compositor->touch_calibration_save &&
	    compositor->touch_calibration_save(compositor, device,
					       &calibration) < 0)
		return;

	/* If calibrator is still mapped, the compositor will use
	 * saved_calibration when going back to normal touch handling.
	 * Continuing calibrating after save request is undefined. */
	calibrator = compositor->touch_calibrator;
	if (calibrator &&
	    calibrator->surface &&
	    weston_surface_is_mapped(calibrator->surface))
		device->saved_calibration = calibration;
	else
		device->ops->set_calibration(device, &calibration);
}

static const struct weston_touch_calibration_interface
touch_calibration_implementation = {
	touch_calibration_destroy,
	touch_calibration_create_calibrator,
	touch_calibration_save
};

static void
bind_touch_calibration(struct wl_client *client,
		       void *data, uint32_t version, uint32_t id)
{
	struct weston_compositor *compositor = data;
	struct wl_resource *resource;
	struct weston_touch_device *device;
	struct weston_seat *seat;
	struct weston_touch *touch;
	const char *name;

	resource = wl_resource_create(client,
				      &weston_touch_calibration_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource,
				       &touch_calibration_implementation,
				       compositor, NULL);

	wl_list_for_each(seat, &compositor->seat_list, link) {
		touch = weston_seat_get_touch(seat);
		if (!touch)
			continue;

		wl_list_for_each(device, &touch->device_list, link) {
			if (!weston_touch_device_can_calibrate(device))
				continue;

			name = device->ops->get_calibration_head_name(device);
			if (!name)
				continue;

			weston_touch_calibration_send_touch_device(resource,
							device->syspath, name);
		}
	}
}

void
weston_compositor_destroy_touch_calibrator(struct weston_compositor *ec)
{
	/* TODO: handle weston_compositor::touch_calibrator destruction
	 * see
	 * https://gitlab.freedesktop.org/wayland/weston/-/merge_requests/819#note_1345191
	 */
	weston_layer_fini(&ec->calibrator_layer);
}

/** Advertise touch_calibration support
 *
 * \param compositor The compositor to init for.
 * \param save The callback function for saving a new calibration, or NULL.
 * \return Zero on success, -1 on failure or if already enabled.
 *
 * Calling this initializes the weston_touch_calibration protocol support,
 * so that the interface will be advertised to clients. It is recommended
 * to use some mechanism, e.g. wl_display_set_global_filter(), to restrict
 * access to the interface.
 *
 * There is no way to disable this once enabled.
 *
 * If the save callback is NULL, a new calibration provided by a client will
 * always be accepted. If the save callback is not NULL, it must return
 * success for the new calibration to be accepted.
 */
WL_EXPORT int
weston_compositor_enable_touch_calibrator(struct weston_compositor *compositor,
				weston_touch_calibration_save_func save)
{
	if (compositor->touch_calibration)
		return -1;

	compositor->touch_calibration = wl_global_create(compositor->wl_display,
					&weston_touch_calibration_interface, 1,
					compositor, bind_touch_calibration);
	if (!compositor->touch_calibration)
		return -1;

	compositor->touch_calibration_save = save;
	weston_layer_init(&compositor->calibrator_layer, compositor);

	/* needs to be stacked above everything except lock screen and cursor,
	 * otherwise the position value is arbitrary */
	weston_layer_set_position(&compositor->calibrator_layer,
				  WESTON_LAYER_POSITION_TOP_UI + 120);

	return 0;
}
