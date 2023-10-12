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

/*
 * This header contains the libweston ABI exported only for internal backends.
 */

#ifndef LIBWESTON_BACKEND_INTERNAL_H
#define LIBWESTON_BACKEND_INTERNAL_H

struct weston_hdr_metadata_type1;

struct weston_backend {
	void (*destroy)(struct weston_backend *backend);

	/** Begin a repaint sequence
	 *
	 * Provides the backend with explicit markers around repaint
	 * sequences, which may allow the backend to aggregate state
	 * application. This call will be bracketed by the repaint_flush (on
	 * success), or repaint_cancel (when any output in the grouping fails
	 * repaint).
	 *
	 * Returns an opaque pointer, which the backend may use as private
	 * data referring to the repaint cycle.
	 */
	void (*repaint_begin)(struct weston_backend *backend);

	/** Cancel a repaint sequence
	 *
	 * Cancels a repaint sequence, when an error has occurred during
	 * one output's repaint; see repaint_begin.
	 */
	void (*repaint_cancel)(struct weston_backend *backend);

	/** Conclude a repaint sequence
	 *
	 * Called on successful completion of a repaint sequence; see
	 * repaint_begin.
	 */
	int (*repaint_flush)(struct weston_backend *backend);

	/** Allocate a new output
	 *
	 * @param backend The backend.
	 * @param name Name for the new output.
	 *
	 * Allocates a new output structure that embeds a weston_output,
	 * initializes it, and returns the pointer to the weston_output
	 * member.
	 *
	 * Must set weston_output members @c destroy, @c enable and @c disable.
	 */
	struct weston_output *
	(*create_output)(struct weston_backend *backend, const char *name);

	/** Notify of device addition/removal
	 *
	 * @param backend The backend.
	 * @param device The device that has changed.
	 * @param added Where it was added (or removed)
	 *
	 * Called when a device has been added/removed from the session.
	 * The backend can decide what to do based on whether it is a
	 * device that it is controlling or not.
	 */
	void (*device_changed)(struct weston_backend *backend,
			       dev_t device, bool added);

	/** Verifies if the dmabuf can be used directly/scanned-out by the HW.
	 *
	 * @param backend The backend.
	 * @param buffer The dmabuf to verify.
	 *
	 * Determines if the buffer can be imported directly by the display
	 * controller/HW. Back-ends can use this to check if the supplied
	 * buffer can be scanned-out, as to void importing it into the GPU.
	 */
	bool (*can_scanout_dmabuf)(struct weston_backend *backend,
				   struct linux_dmabuf_buffer *buffer);
};

/* weston_head */

void
weston_head_init(struct weston_head *head, const char *name);

void
weston_head_release(struct weston_head *head);

void
weston_head_set_connection_status(struct weston_head *head, bool connected);

void
weston_head_set_internal(struct weston_head *head);

void
weston_head_set_monitor_strings(struct weston_head *head,
				const char *make,
				const char *model,
				const char *serialno);
void
weston_head_set_non_desktop(struct weston_head *head, bool non_desktop);

void
weston_head_set_physical_size(struct weston_head *head,
			      int32_t mm_width, int32_t mm_height);

void
weston_head_set_subpixel(struct weston_head *head,
			 enum wl_output_subpixel sp);

void
weston_head_set_transform(struct weston_head *head, uint32_t transform);

void
weston_head_set_supported_eotf_mask(struct weston_head *head,
				    uint32_t eotf_mask);

/* weston_output */

void
weston_output_init(struct weston_output *output,
		   struct weston_compositor *compositor,
		   const char *name);
void
weston_output_damage(struct weston_output *output);

void
weston_output_release(struct weston_output *output);

void
weston_output_finish_frame(struct weston_output *output,
			   const struct timespec *stamp,
			   uint32_t presented_flags);

void
weston_output_repaint_failed(struct weston_output *output);

int
weston_output_mode_set_native(struct weston_output *output,
			      struct weston_mode *mode,
			      int32_t scale);

struct weston_coord_global
weston_coord_global_from_output_point(double x, double y,
				      const struct weston_output *output);

void
weston_region_global_to_output(pixman_region32_t *dst,
			       struct weston_output *output,
			       pixman_region32_t *src);

const struct weston_hdr_metadata_type1 *
weston_output_get_hdr_metadata_type1(const struct weston_output *output);

/* weston_seat */

void
notify_axis(struct weston_seat *seat, const struct timespec *time,
	    struct weston_pointer_axis_event *event);
void
notify_axis_source(struct weston_seat *seat, uint32_t source);

void
notify_button(struct weston_seat *seat, const struct timespec *time,
	      int32_t button, enum wl_pointer_button_state state);

void
notify_key(struct weston_seat *seat, const struct timespec *time, uint32_t key,
	   enum wl_keyboard_key_state state,
	   enum weston_key_state_update update_state);
void
notify_keyboard_focus_in(struct weston_seat *seat, struct wl_array *keys,
			 enum weston_key_state_update update_state);
void
notify_keyboard_focus_out(struct weston_seat *seat);

void
notify_motion(struct weston_seat *seat, const struct timespec *time,
	      struct weston_pointer_motion_event *event);
void
notify_motion_absolute(struct weston_seat *seat, const struct timespec *time,
		       struct weston_coord_global pos);
void
notify_modifiers(struct weston_seat *seat, uint32_t serial);

void
notify_pointer_frame(struct weston_seat *seat);

void
notify_pointer_focus(struct weston_seat *seat, struct weston_output *output,
		     struct weston_coord_global pos);

void
clear_pointer_focus(struct weston_seat *seat);

/* weston_touch_device */

void
notify_touch_normalized(struct weston_touch_device *device,
			const struct timespec *time,
			int touch_id,
			const struct weston_coord_global *pos,
			const struct weston_point2d_device_normalized *norm,
			int touch_type);

/** Feed in touch down, motion, and up events, non-calibratable device.
 *
 * @sa notify_touch_cal
 */
static inline void
notify_touch(struct weston_touch_device *device, const struct timespec *time,
	     int touch_id, const struct weston_coord_global *pos, int touch_type)
{
	notify_touch_normalized(device, time, touch_id, pos, NULL, touch_type);
}

void
notify_touch_frame(struct weston_touch_device *device);

void
notify_touch_cancel(struct weston_touch_device *device);

void
notify_touch_calibrator(struct weston_touch_device *device,
			const struct timespec *time, int32_t slot,
			const struct weston_point2d_device_normalized *norm,
			int touch_type);
void
notify_touch_calibrator_cancel(struct weston_touch_device *device);
void
notify_touch_calibrator_frame(struct weston_touch_device *device);

void
notify_tablet_added(struct weston_tablet *tablet);

void
notify_tablet_tool_added(struct weston_tablet_tool *tool);

void
notify_tablet_tool_proximity_in(struct weston_tablet_tool *tool,
				const struct timespec *time,
				struct weston_tablet *tablet);
void
notify_tablet_tool_proximity_out(struct weston_tablet_tool *tool,
				 const struct timespec *time);
void
notify_tablet_tool_motion(struct weston_tablet_tool *tool,
			  const struct timespec *time,
			  struct weston_coord_global pos);
void
notify_tablet_tool_pressure(struct weston_tablet_tool *tool,
			    const struct timespec *time, uint32_t pressure);
void
notify_tablet_tool_distance(struct weston_tablet_tool *tool,
			    const struct timespec *time, uint32_t distance);
void
notify_tablet_tool_tilt(struct weston_tablet_tool *tool,
			const struct timespec *time,
			int32_t tilt_x, int32_t tilt_y);
void
notify_tablet_tool_button(struct weston_tablet_tool *tool,
			  const struct timespec *time,
			  uint32_t button,
			  uint32_t state);
void
notify_tablet_tool_up(struct weston_tablet_tool *tool,
		      const struct timespec *time);
void
notify_tablet_tool_down(struct weston_tablet_tool *tool,
			const struct timespec *time);
void
notify_tablet_tool_frame(struct weston_tablet_tool *tool,
			 const struct timespec *time);

#endif
