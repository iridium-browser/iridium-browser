/*
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2017, 2018 General Electric Company
 * Copyright © 2012, 2017-2019 Collabora, Ltd.
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

#ifndef LIBWESTON_INTERNAL_H
#define LIBWESTON_INTERNAL_H

/*
 * This is the internal (private) part of libweston. All symbols found here
 * are, and should be only (with a few exceptions) used within the internal
 * parts of libweston.  Notable exception(s) include a few files in tests/ that
 * need access to these functions, screen-share file from compositor/ and those
 * remoting/. Those will require some further fixing as to avoid including this
 * private header.
 *
 * Eventually, these symbols should reside naturally into their own scope. New
 * features should either provide their own (internal) header or use this one.
 */


/* weston_buffer */

void
weston_buffer_send_server_error(struct weston_buffer *buffer,
				const char *msg);
void
weston_buffer_reference(struct weston_buffer_reference *ref,
			struct weston_buffer *buffer);

void
weston_buffer_release_move(struct weston_buffer_release_reference *dest,
			   struct weston_buffer_release_reference *src);

void
weston_buffer_release_reference(struct weston_buffer_release_reference *ref,
				struct weston_buffer_release *buf_release);

/* weston_bindings */
void
weston_binding_list_destroy_all(struct wl_list *list);

/* weston_compositor */

void
touch_calibrator_mode_changed(struct weston_compositor *compositor);

int
noop_renderer_init(struct weston_compositor *ec);

void
weston_compositor_add_head(struct weston_compositor *compositor,
			   struct weston_head *head);
void
weston_compositor_add_pending_output(struct weston_output *output,
				     struct weston_compositor *compositor);
bool
weston_compositor_import_dmabuf(struct weston_compositor *compositor,
				struct linux_dmabuf_buffer *buffer);
bool
weston_compositor_dmabuf_can_scanout(struct weston_compositor *compositor,
					struct linux_dmabuf_buffer *buffer);
void
weston_compositor_offscreen(struct weston_compositor *compositor);

char *
weston_compositor_print_scene_graph(struct weston_compositor *ec);

void
weston_compositor_read_presentation_clock(
			const struct weston_compositor *compositor,
			struct timespec *ts);

int
weston_compositor_run_axis_binding(struct weston_compositor *compositor,
				   struct weston_pointer *pointer,
				   const struct timespec *time,
				   struct weston_pointer_axis_event *event);
void
weston_compositor_run_button_binding(struct weston_compositor *compositor,
				     struct weston_pointer *pointer,
				     const struct timespec *time,
				     uint32_t button,
				     enum wl_pointer_button_state value);
int
weston_compositor_run_debug_binding(struct weston_compositor *compositor,
				    struct weston_keyboard *keyboard,
				    const struct timespec *time,
				    uint32_t key,
				    enum wl_keyboard_key_state state);
void
weston_compositor_run_key_binding(struct weston_compositor *compositor,
				  struct weston_keyboard *keyboard,
				  const struct timespec *time,
				  uint32_t key,
				  enum wl_keyboard_key_state state);
void
weston_compositor_run_modifier_binding(struct weston_compositor *compositor,
				       struct weston_keyboard *keyboard,
				       enum weston_keyboard_modifier modifier,
				       enum wl_keyboard_key_state state);
void
weston_compositor_run_touch_binding(struct weston_compositor *compositor,
				    struct weston_touch *touch,
				    const struct timespec *time,
				    int touch_type);
void
weston_compositor_stack_plane(struct weston_compositor *ec,
			      struct weston_plane *plane,
			      struct weston_plane *above);
void
weston_compositor_set_touch_mode_normal(struct weston_compositor *compositor);

void
weston_compositor_set_touch_mode_calib(struct weston_compositor *compositor);

int
weston_compositor_set_presentation_clock(struct weston_compositor *compositor,
					 clockid_t clk_id);
int
weston_compositor_set_presentation_clock_software(
					struct weston_compositor *compositor);
void
weston_compositor_shutdown(struct weston_compositor *ec);

void
weston_compositor_xkb_destroy(struct weston_compositor *ec);

int
weston_input_init(struct weston_compositor *compositor);

/* weston_output */

void
weston_output_disable_planes_incr(struct weston_output *output);

void
weston_output_disable_planes_decr(struct weston_output *output);

/* weston_plane */

void
weston_plane_init(struct weston_plane *plane,
			struct weston_compositor *ec,
			int32_t x, int32_t y);
void
weston_plane_release(struct weston_plane *plane);

/* weston_seat */

struct clipboard *
clipboard_create(struct weston_seat *seat);

void
weston_seat_init(struct weston_seat *seat, struct weston_compositor *ec,
		 const char *seat_name);

void
weston_seat_repick(struct weston_seat *seat);

void
weston_seat_release(struct weston_seat *seat);

void
weston_seat_send_selection(struct weston_seat *seat, struct wl_client *client);

void
weston_seat_init_pointer(struct weston_seat *seat);

int
weston_seat_init_keyboard(struct weston_seat *seat, struct xkb_keymap *keymap);

void
weston_seat_init_touch(struct weston_seat *seat);

void
weston_seat_release_keyboard(struct weston_seat *seat);

void
weston_seat_release_pointer(struct weston_seat *seat);

void
weston_seat_release_touch(struct weston_seat *seat);

void
weston_seat_update_keymap(struct weston_seat *seat, struct xkb_keymap *keymap);

void
wl_data_device_set_keyboard_focus(struct weston_seat *seat);

/* weston_pointer */

void
weston_pointer_clamp(struct weston_pointer *pointer,
		     wl_fixed_t *fx, wl_fixed_t *fy);
void
weston_pointer_set_default_grab(struct weston_pointer *pointer,
			        const struct weston_pointer_grab_interface *interface);

void
weston_pointer_constraint_destroy(struct weston_pointer_constraint *constraint);

/* weston_keyboard */
bool
weston_keyboard_has_focus_resource(struct weston_keyboard *keyboard);

/* weston_touch */

struct weston_touch_device *
weston_touch_create_touch_device(struct weston_touch *touch,
				 const char *syspath,
				 void *backend_data,
				 const struct weston_touch_device_ops *ops);

void
weston_touch_device_destroy(struct weston_touch_device *device);

bool
weston_touch_has_focus_resource(struct weston_touch *touch);

int
weston_touch_start_drag(struct weston_touch *touch,
			struct weston_data_source *source,
			struct weston_surface *icon,
			struct wl_client *client);


/* weston_touch_device */

bool
weston_touch_device_can_calibrate(struct weston_touch_device *device);

/* weston_surface */
void
weston_surface_to_buffer_float(struct weston_surface *surface,
			       float x, float y, float *bx, float *by);
pixman_box32_t
weston_surface_to_buffer_rect(struct weston_surface *surface,
			      pixman_box32_t rect);

void
weston_surface_to_buffer_region(struct weston_surface *surface,
				pixman_region32_t *surface_region,
				pixman_region32_t *buffer_region);
void
weston_surface_schedule_repaint(struct weston_surface *surface);

/* weston_spring */

void
weston_spring_init(struct weston_spring *spring,
		   double k, double current, double target);
int
weston_spring_done(struct weston_spring *spring);

void
weston_spring_update(struct weston_spring *spring, const struct timespec *time);

/* weston_view */

void
weston_view_to_global_fixed(struct weston_view *view,
			    wl_fixed_t sx, wl_fixed_t sy,
			    wl_fixed_t *x, wl_fixed_t *y);
void
weston_view_from_global_float(struct weston_view *view,
			      float x, float y, float *vx, float *vy);
bool
weston_view_is_opaque(struct weston_view *ev, pixman_region32_t *region);

bool
weston_view_has_valid_buffer(struct weston_view *ev);

bool
weston_view_matches_output_entirely(struct weston_view *ev,
				    struct weston_output *output);
void
weston_view_move_to_plane(struct weston_view *view,
			  struct weston_plane *plane);

void
weston_transformed_coord(int width, int height,
			 enum wl_output_transform transform,
			 int32_t scale,
			 float sx, float sy, float *bx, float *by);
pixman_box32_t
weston_transformed_rect(int width, int height,
			enum wl_output_transform transform,
			int32_t scale,
			pixman_box32_t rect);
void
weston_transformed_region(int width, int height,
			  enum wl_output_transform transform,
			  int32_t scale,
			  pixman_region32_t *src, pixman_region32_t *dest);
void
weston_matrix_transform_region(pixman_region32_t *dest,
			       struct weston_matrix *matrix,
			       pixman_region32_t *src);

/* protected_surface */
void
weston_protected_surface_send_event(struct protected_surface *psurface,
				    enum weston_hdcp_protection protection);

/* others */
int
wl_data_device_manager_init(struct wl_display *display);

#endif
