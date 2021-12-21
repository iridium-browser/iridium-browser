/*
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2011 Intel Corporation
 * Copyright © 2012 Raspberry Pi Foundation
 * Copyright © 2013 Philip Withnall
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

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <assert.h>

#include <libudev.h>

#include "shared/helpers.h"
#include <libweston/libweston.h>
#include <libweston/backend-fbdev.h>
#include "launcher-util.h"
#include "pixman-renderer.h"
#include "libinput-seat.h"
#include "presentation-time-server-protocol.h"

struct fbdev_backend {
	struct weston_backend base;
	struct weston_compositor *compositor;
	uint32_t prev_state;

	struct udev *udev;
	struct udev_input input;
	uint32_t output_transform;
	struct wl_listener session_listener;
};

struct fbdev_screeninfo {
	unsigned int x_resolution; /* pixels, visible area */
	unsigned int y_resolution; /* pixels, visible area */
	unsigned int width_mm; /* visible screen width in mm */
	unsigned int height_mm; /* visible screen height in mm */
	unsigned int bits_per_pixel;

	size_t buffer_length; /* length of frame buffer memory in bytes */
	size_t line_length; /* length of a line in bytes */
	char id[16]; /* screen identifier */

	pixman_format_code_t pixel_format; /* frame buffer pixel format */
	unsigned int refresh_rate; /* Hertz */
};

struct fbdev_head {
	struct weston_head base;

	/* Frame buffer details. */
	char *device;
	struct fbdev_screeninfo fb_info;
};

struct fbdev_output {
	struct fbdev_backend *backend;
	struct weston_output base;

	struct weston_mode mode;
	struct wl_event_source *finish_frame_timer;

	/* framebuffer mmap details */
	size_t buffer_length;
	void *fb;

	/* pixman details. */
	pixman_image_t *hw_surface;
};

static const char default_seat[] = "seat0";

static inline struct fbdev_head *
to_fbdev_head(struct weston_head *base)
{
	return container_of(base, struct fbdev_head, base);
}

static inline struct fbdev_output *
to_fbdev_output(struct weston_output *base)
{
	return container_of(base, struct fbdev_output, base);
}

static inline struct fbdev_backend *
to_fbdev_backend(struct weston_compositor *base)
{
	return container_of(base->backend, struct fbdev_backend, base);
}

static struct fbdev_head *
fbdev_output_get_head(struct fbdev_output *output)
{
	if (wl_list_length(&output->base.head_list) != 1)
		return NULL;

	return container_of(output->base.head_list.next,
			    struct fbdev_head, base.output_link);
}

static int
fbdev_output_start_repaint_loop(struct weston_output *output)
{
	struct timespec ts;

	weston_compositor_read_presentation_clock(output->compositor, &ts);
	weston_output_finish_frame(output, &ts, WP_PRESENTATION_FEEDBACK_INVALID);

	return 0;
}

static int
fbdev_output_repaint(struct weston_output *base, pixman_region32_t *damage,
		     void *repaint_data)
{
	struct fbdev_output *output = to_fbdev_output(base);
	struct weston_compositor *ec = output->base.compositor;

	/* Repaint the damaged region onto the back buffer. */
	pixman_renderer_output_set_buffer(base, output->hw_surface);
	ec->renderer->repaint_output(base, damage);

	/* Update the damage region. */
	pixman_region32_subtract(&ec->primary_plane.damage,
	                         &ec->primary_plane.damage, damage);

	/* Schedule the end of the frame. We do not sync this to the frame
	 * buffer clock because users who want that should be using the DRM
	 * compositor. FBIO_WAITFORVSYNC blocks and FB_ACTIVATE_VBL requires
	 * panning, which is broken in most kernel drivers.
	 *
	 * Finish the frame synchronised to the specified refresh rate. The
	 * refresh rate is given in mHz and the interval in ms. */
	wl_event_source_timer_update(output->finish_frame_timer,
	                             1000000 / output->mode.refresh);

	return 0;
}

static int
finish_frame_handler(void *data)
{
	struct fbdev_output *output = data;
	struct timespec ts;

	weston_compositor_read_presentation_clock(output->base.compositor, &ts);
	weston_output_finish_frame(&output->base, &ts, 0);

	return 1;
}

static pixman_format_code_t
calculate_pixman_format(struct fb_var_screeninfo *vinfo,
                        struct fb_fix_screeninfo *finfo)
{
	/* Calculate the pixman format supported by the frame buffer from the
	 * buffer's metadata. Return 0 if no known pixman format is supported
	 * (since this has depth 0 it's guaranteed to not conflict with any
	 * actual pixman format).
	 *
	 * Documentation on the vinfo and finfo structures:
	 *    http://www.mjmwired.net/kernel/Documentation/fb/api.txt
	 *
	 * TODO: Try a bit harder to support other formats, including setting
	 * the preferred format in the hardware. */
	int type;

	weston_log("Calculating pixman format from:\n"
	           STAMP_SPACE " - type: %i (aux: %i)\n"
	           STAMP_SPACE " - visual: %i\n"
	           STAMP_SPACE " - bpp: %i (grayscale: %i)\n"
	           STAMP_SPACE " - red: offset: %i, length: %i, MSB: %i\n"
	           STAMP_SPACE " - green: offset: %i, length: %i, MSB: %i\n"
	           STAMP_SPACE " - blue: offset: %i, length: %i, MSB: %i\n"
	           STAMP_SPACE " - transp: offset: %i, length: %i, MSB: %i\n",
	           finfo->type, finfo->type_aux, finfo->visual,
	           vinfo->bits_per_pixel, vinfo->grayscale,
	           vinfo->red.offset, vinfo->red.length, vinfo->red.msb_right,
	           vinfo->green.offset, vinfo->green.length,
	           vinfo->green.msb_right,
	           vinfo->blue.offset, vinfo->blue.length,
	           vinfo->blue.msb_right,
	           vinfo->transp.offset, vinfo->transp.length,
	           vinfo->transp.msb_right);

	/* We only handle packed formats at the moment. */
	if (finfo->type != FB_TYPE_PACKED_PIXELS)
		return 0;

	/* We only handle true-colour frame buffers at the moment. */
	switch(finfo->visual) {
		case FB_VISUAL_TRUECOLOR:
		case FB_VISUAL_DIRECTCOLOR:
			if (vinfo->grayscale != 0)
				return 0;
		break;
		default:
			return 0;
	}

	/* We only support formats with MSBs on the left. */
	if (vinfo->red.msb_right != 0 || vinfo->green.msb_right != 0 ||
	    vinfo->blue.msb_right != 0)
		return 0;

	/* Work out the format type from the offsets. We only support RGBA, ARGB
	 * and ABGR at the moment. */
	type = PIXMAN_TYPE_OTHER;

	if ((vinfo->transp.offset >= vinfo->red.offset ||
	     vinfo->transp.length == 0) &&
	    vinfo->red.offset >= vinfo->green.offset &&
	    vinfo->green.offset >= vinfo->blue.offset)
		type = PIXMAN_TYPE_ARGB;
	else if (vinfo->red.offset >= vinfo->green.offset &&
	         vinfo->green.offset >= vinfo->blue.offset &&
	         vinfo->blue.offset >= vinfo->transp.offset)
		type = PIXMAN_TYPE_RGBA;
	else if (vinfo->transp.offset >= vinfo->blue.offset &&
	         vinfo->blue.offset >= vinfo->green.offset &&
	         vinfo->green.offset >= vinfo->red.offset)
		type = PIXMAN_TYPE_ABGR;

	if (type == PIXMAN_TYPE_OTHER)
		return 0;

	/* Build the format. */
	return PIXMAN_FORMAT(vinfo->bits_per_pixel, type,
	                     vinfo->transp.length,
	                     vinfo->red.length,
	                     vinfo->green.length,
	                     vinfo->blue.length);
}

static int
calculate_refresh_rate(struct fb_var_screeninfo *vinfo)
{
	uint64_t quot;

	/* Calculate monitor refresh rate. Default is 60 Hz. Units are mHz. */
	quot = (vinfo->upper_margin + vinfo->lower_margin + vinfo->yres);
	quot *= (vinfo->left_margin + vinfo->right_margin + vinfo->xres);
	quot *= vinfo->pixclock;

	if (quot > 0) {
		uint64_t refresh_rate;

		refresh_rate = 1000000000000000LLU / quot;
		if (refresh_rate > 200000)
			refresh_rate = 200000; /* cap at 200 Hz */

		if (refresh_rate >= 1000) /* at least 1 Hz */
			return refresh_rate;
	}

	return 60 * 1000; /* default to 60 Hz */
}

static int
fbdev_query_screen_info(int fd, struct fbdev_screeninfo *info)
{
	struct fb_var_screeninfo varinfo;
	struct fb_fix_screeninfo fixinfo;

	/* Probe the device for screen information. */
	if (ioctl(fd, FBIOGET_FSCREENINFO, &fixinfo) < 0 ||
	    ioctl(fd, FBIOGET_VSCREENINFO, &varinfo) < 0) {
		return -1;
	}

	/* Store the pertinent data. */
	info->x_resolution = varinfo.xres;
	info->y_resolution = varinfo.yres;
	info->width_mm = varinfo.width;
	info->height_mm = varinfo.height;
	info->bits_per_pixel = varinfo.bits_per_pixel;

	info->buffer_length = fixinfo.smem_len;
	info->line_length = fixinfo.line_length;
	strncpy(info->id, fixinfo.id, sizeof(info->id));
	info->id[sizeof(info->id)-1] = '\0';

	info->pixel_format = calculate_pixman_format(&varinfo, &fixinfo);
	info->refresh_rate = calculate_refresh_rate(&varinfo);

	if (info->pixel_format == 0) {
		weston_log("Frame buffer uses an unsupported format.\n");
		return -1;
	}

	return 1;
}

static int
fbdev_set_screen_info(int fd, struct fbdev_screeninfo *info)
{
	struct fb_var_screeninfo varinfo;

	/* Grab the current screen information. */
	if (ioctl(fd, FBIOGET_VSCREENINFO, &varinfo) < 0) {
		return -1;
	}

	/* Update the information. */
	varinfo.xres = info->x_resolution;
	varinfo.yres = info->y_resolution;
	varinfo.width = info->width_mm;
	varinfo.height = info->height_mm;
	varinfo.bits_per_pixel = info->bits_per_pixel;

	/* Try to set up an ARGB (x8r8g8b8) pixel format. */
	varinfo.grayscale = 0;
	varinfo.transp.offset = 24;
	varinfo.transp.length = 0;
	varinfo.transp.msb_right = 0;
	varinfo.red.offset = 16;
	varinfo.red.length = 8;
	varinfo.red.msb_right = 0;
	varinfo.green.offset = 8;
	varinfo.green.length = 8;
	varinfo.green.msb_right = 0;
	varinfo.blue.offset = 0;
	varinfo.blue.length = 8;
	varinfo.blue.msb_right = 0;

	/* Set the device's screen information. */
	if (ioctl(fd, FBIOPUT_VSCREENINFO, &varinfo) < 0) {
		return -1;
	}

	return 1;
}

static int
fbdev_wakeup_screen(int fd, struct fbdev_screeninfo *info)
{
	struct fb_var_screeninfo varinfo;

	/* Grab the current screen information. */
	if (ioctl(fd, FBIOGET_VSCREENINFO, &varinfo) < 0) {
		return -1;
	}

	/* force the framebuffer to wake up */
	varinfo.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;

	/* Set the device's screen information. */
	if (ioctl(fd, FBIOPUT_VSCREENINFO, &varinfo) < 0) {
		return -1;
	}

	return 1;
}

/* Returns an FD for the frame buffer device. */
static int
fbdev_frame_buffer_open(const char *fb_dev,
			struct fbdev_screeninfo *screen_info)
{
	int fd = -1;

	weston_log("Opening fbdev frame buffer.\n");

	/* Open the frame buffer device. */
	fd = open(fb_dev, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		weston_log("Failed to open frame buffer device ‘%s’: %s\n",
		           fb_dev, strerror(errno));
		return -1;
	}

	/* Grab the screen info. */
	if (fbdev_query_screen_info(fd, screen_info) < 0) {
		weston_log("Failed to get frame buffer info: %s\n",
		           strerror(errno));

		close(fd);
		return -1;
	}

	/* Attempt to wake up the framebuffer device, needed for secondary
	 * framebuffer devices */
	if (fbdev_wakeup_screen(fd, screen_info) < 0) {
		weston_log("Failed to activate framebuffer display. "
		           "Attempting to open output anyway.\n");
	}


	return fd;
}

/* Closes the FD on success or failure. */
static int
fbdev_frame_buffer_map(struct fbdev_output *output, int fd)
{
	struct fbdev_head *head;
	int retval = -1;

	head = fbdev_output_get_head(output);

	weston_log("Mapping fbdev frame buffer.\n");

	/* Map the frame buffer. Write-only mode, since we don't want to read
	 * anything back (because it's slow). */
	output->buffer_length = head->fb_info.buffer_length;
	output->fb = mmap(NULL, output->buffer_length,
	                  PROT_WRITE, MAP_SHARED, fd, 0);
	if (output->fb == MAP_FAILED) {
		weston_log("Failed to mmap frame buffer: %s\n",
		           strerror(errno));
		output->fb = NULL;
		goto out_close;
	}

	/* Create a pixman image to wrap the memory mapped frame buffer. */
	output->hw_surface =
		pixman_image_create_bits(head->fb_info.pixel_format,
		                         head->fb_info.x_resolution,
		                         head->fb_info.y_resolution,
		                         output->fb,
		                         head->fb_info.line_length);
	if (output->hw_surface == NULL) {
		weston_log("Failed to create surface for frame buffer.\n");
		goto out_unmap;
	}

	/* Success! */
	retval = 0;

out_unmap:
	if (retval != 0 && output->fb != NULL) {
		munmap(output->fb, output->buffer_length);
		output->fb = NULL;
	}

out_close:
	if (fd >= 0)
		close(fd);

	return retval;
}

static void
fbdev_frame_buffer_unmap(struct fbdev_output *output)
{
	if (!output->fb) {
		assert(!output->hw_surface);
		return;
	}

	weston_log("Unmapping fbdev frame buffer.\n");

	if (output->hw_surface)
		pixman_image_unref(output->hw_surface);
	output->hw_surface = NULL;

	if (munmap(output->fb, output->buffer_length) < 0)
		weston_log("Failed to munmap frame buffer: %s\n",
		           strerror(errno));

	output->fb = NULL;
}


static int
fbdev_output_attach_head(struct weston_output *output_base,
			 struct weston_head *head_base)
{
	struct fbdev_output *output = to_fbdev_output(output_base);
	struct fbdev_head *head = to_fbdev_head(head_base);

	/* Clones not supported. */
	if (!wl_list_empty(&output->base.head_list))
		return -1;

	/* only one static mode in list */
	output->mode.flags = WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;
	output->mode.width = head->fb_info.x_resolution;
	output->mode.height = head->fb_info.y_resolution;
	output->mode.refresh = head->fb_info.refresh_rate;
	wl_list_init(&output->base.mode_list);
	wl_list_insert(&output->base.mode_list, &output->mode.link);
	output->base.current_mode = &output->mode;

	return 0;
}

static void fbdev_output_destroy(struct weston_output *base);

static int
fbdev_output_enable(struct weston_output *base)
{
	struct fbdev_output *output = to_fbdev_output(base);
	struct fbdev_backend *backend = to_fbdev_backend(base->compositor);
	struct fbdev_head *head;
	int fb_fd;
	struct wl_event_loop *loop;
	const struct pixman_renderer_output_options options = {
		.use_shadow = true,
	};

	head = fbdev_output_get_head(output);

	/* Create the frame buffer. */
	fb_fd = fbdev_frame_buffer_open(head->device, &head->fb_info);
	if (fb_fd < 0) {
		weston_log("Creating frame buffer failed.\n");
		return -1;
	}

	if (fbdev_frame_buffer_map(output, fb_fd) < 0) {
		weston_log("Mapping frame buffer failed.\n");
		return -1;
	}

	output->base.start_repaint_loop = fbdev_output_start_repaint_loop;
	output->base.repaint = fbdev_output_repaint;

	if (pixman_renderer_output_create(&output->base, &options) < 0)
		goto out_hw_surface;

	loop = wl_display_get_event_loop(backend->compositor->wl_display);
	output->finish_frame_timer =
		wl_event_loop_add_timer(loop, finish_frame_handler, output);

	weston_log("fbdev output %d×%d px\n",
	           output->mode.width, output->mode.height);
	weston_log_continue(STAMP_SPACE "guessing %d Hz and 96 dpi\n",
	                    output->mode.refresh / 1000);

	return 0;

out_hw_surface:
	fbdev_frame_buffer_unmap(output);

	return -1;
}

static int
fbdev_output_disable(struct weston_output *base)
{
	struct fbdev_output *output = to_fbdev_output(base);

	if (!base->enabled)
		return 0;

	wl_event_source_remove(output->finish_frame_timer);
	output->finish_frame_timer = NULL;

	pixman_renderer_output_destroy(&output->base);
	fbdev_frame_buffer_unmap(output);

	return 0;
}

static struct fbdev_head *
fbdev_head_create(struct fbdev_backend *backend, const char *device)
{
	struct fbdev_head *head;
	int fb_fd;

	head = zalloc(sizeof *head);
	if (!head)
		return NULL;

	head->device = strdup(device);

	/* Create the frame buffer. */
	fb_fd = fbdev_frame_buffer_open(head->device, &head->fb_info);
	if (fb_fd < 0) {
		weston_log("Creating frame buffer head failed.\n");
		goto out_free;
	}
	close(fb_fd);

	weston_head_init(&head->base, "fbdev");
	weston_head_set_connection_status(&head->base, true);
	weston_head_set_monitor_strings(&head->base, "unknown",
					head->fb_info.id, NULL);
	weston_head_set_subpixel(&head->base, WL_OUTPUT_SUBPIXEL_UNKNOWN);
	weston_head_set_physical_size(&head->base, head->fb_info.width_mm,
				      head->fb_info.height_mm);

	weston_compositor_add_head(backend->compositor, &head->base);

	weston_log("Created head '%s' for device %s (%s)\n",
		   head->base.name, head->device, head->base.model);

	return head;

out_free:
	free(head->device);
	free(head);

	return NULL;
}

static void
fbdev_head_destroy(struct fbdev_head *head)
{
	weston_head_release(&head->base);
	free(head->device);
	free(head);
}

static struct weston_output *
fbdev_output_create(struct weston_compositor *compositor,
                    const char *name)
{
	struct fbdev_output *output;

	weston_log("Creating fbdev output.\n");

	output = zalloc(sizeof *output);
	if (output == NULL)
		return NULL;

	output->backend = to_fbdev_backend(compositor);

	weston_output_init(&output->base, compositor, name);

	output->base.destroy = fbdev_output_destroy;
	output->base.disable = fbdev_output_disable;
	output->base.enable = fbdev_output_enable;
	output->base.attach_head = fbdev_output_attach_head;

	weston_compositor_add_pending_output(&output->base, compositor);

	return &output->base;
}

static void
fbdev_output_destroy(struct weston_output *base)
{
	struct fbdev_output *output = to_fbdev_output(base);

	weston_log("Destroying fbdev output.\n");

	fbdev_output_disable(base);

	/* Remove the output. */
	weston_output_release(&output->base);

	free(output);
}

/* strcmp()-style return values. */
static int
compare_screen_info (const struct fbdev_screeninfo *a,
                     const struct fbdev_screeninfo *b)
{
	if (a->x_resolution == b->x_resolution &&
	    a->y_resolution == b->y_resolution &&
	    a->width_mm == b->width_mm &&
	    a->height_mm == b->height_mm &&
	    a->bits_per_pixel == b->bits_per_pixel &&
	    a->pixel_format == b->pixel_format &&
	    a->refresh_rate == b->refresh_rate)
		return 0;

	return 1;
}

static int
fbdev_output_reenable(struct fbdev_backend *backend,
                      struct weston_output *base)
{
	struct fbdev_output *output = to_fbdev_output(base);
	struct fbdev_head *head;
	struct fbdev_screeninfo new_screen_info;
	int fb_fd;

	head = fbdev_output_get_head(output);

	weston_log("Re-enabling fbdev output.\n");
	assert(output->base.enabled);

	/* Create the frame buffer. */
	fb_fd = fbdev_frame_buffer_open(head->device, &new_screen_info);
	if (fb_fd < 0) {
		weston_log("Creating frame buffer failed.\n");
		return -1;
	}

	/* Check whether the frame buffer details have changed since we were
	 * disabled. */
	if (compare_screen_info(&head->fb_info, &new_screen_info) != 0) {
		/* Perform a mode-set to restore the old mode. */
		if (fbdev_set_screen_info(fb_fd, &head->fb_info) < 0) {
			weston_log("Failed to restore mode settings. "
			           "Attempting to re-open output anyway.\n");
		}

		close(fb_fd);

		/* Disable and enable the output so that resources depending on
		 * the frame buffer X/Y resolution (such as the shadow buffer)
		 * are re-initialised. */
		fbdev_output_disable(&output->base);
		return fbdev_output_enable(&output->base);
	}

	/* Map the device if it has the same details as before. */
	if (fbdev_frame_buffer_map(output, fb_fd) < 0) {
		weston_log("Mapping frame buffer failed.\n");
		return -1;
	}

	return 0;
}

static void
fbdev_backend_destroy(struct weston_compositor *base)
{
	struct fbdev_backend *backend = to_fbdev_backend(base);
	struct weston_head *head, *next;

	udev_input_destroy(&backend->input);

	/* Destroy the output. */
	weston_compositor_shutdown(base);

	wl_list_for_each_safe(head, next, &base->head_list, compositor_link)
		fbdev_head_destroy(to_fbdev_head(head));

	/* Chain up. */
	weston_launcher_destroy(base->launcher);

	udev_unref(backend->udev);

	free(backend);
}

static void
session_notify(struct wl_listener *listener, void *data)
{
	struct weston_compositor *compositor = data;
	struct fbdev_backend *backend = to_fbdev_backend(compositor);
	struct weston_output *output;

	if (compositor->session_active) {
		weston_log("entering VT\n");
		compositor->state = backend->prev_state;

		wl_list_for_each(output, &compositor->output_list, link) {
			fbdev_output_reenable(backend, output);
		}

		weston_compositor_damage_all(compositor);

		udev_input_enable(&backend->input);
	} else {
		weston_log("leaving VT\n");
		udev_input_disable(&backend->input);

		wl_list_for_each(output, &compositor->output_list, link) {
			fbdev_frame_buffer_unmap(to_fbdev_output(output));
		}

		backend->prev_state = compositor->state;
		weston_compositor_offscreen(compositor);

		/* If we have a repaint scheduled (from the idle handler), make
		 * sure we cancel that so we don't try to pageflip when we're
		 * vt switched away.  The OFFSCREEN state will prevent
		 * further attempts at repainting.  When we switch
		 * back, we schedule a repaint, which will process
		 * pending frame callbacks. */

		wl_list_for_each(output,
				 &compositor->output_list, link) {
			output->repaint_needed = false;
		}
	}
}

static char *
find_framebuffer_device(struct fbdev_backend *b, const char *seat)
{
	struct udev_enumerate *e;
	struct udev_list_entry *entry;
	const char *path, *device_seat, *id;
	char *fb_device_path = NULL;
	struct udev_device *device, *fb_device, *pci;

	e = udev_enumerate_new(b->udev);
	udev_enumerate_add_match_subsystem(e, "graphics");
	udev_enumerate_add_match_sysname(e, "fb[0-9]*");

	udev_enumerate_scan_devices(e);
	fb_device = NULL;
	udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
		bool is_boot_vga = false;

		path = udev_list_entry_get_name(entry);
		device = udev_device_new_from_syspath(b->udev, path);
		if (!device)
			continue;
		device_seat = udev_device_get_property_value(device, "ID_SEAT");
		if (!device_seat)
			device_seat = default_seat;
		if (strcmp(device_seat, seat)) {
			udev_device_unref(device);
			continue;
		}

		pci = udev_device_get_parent_with_subsystem_devtype(device,
								"pci", NULL);
		if (pci) {
			id = udev_device_get_sysattr_value(pci, "boot_vga");
			if (id && !strcmp(id, "1"))
				is_boot_vga = true;
		}

		/* If a framebuffer device was found, and this device isn't
		 * the boot-VGA device, don't use it. */
		if (!is_boot_vga && fb_device) {
			udev_device_unref(device);
			continue;
		}

		/* There can only be one boot_vga device. Try to use it
		 * at all costs. */
		if (is_boot_vga) {
			if (fb_device)
				udev_device_unref(fb_device);
			fb_device = device;
			break;
		}

		/* Per the (!is_boot_vga && fb_device) test above, only
		 * trump existing saved devices with boot-VGA devices, so if
		 * the test ends up here, this must be the first device seen. */
		assert(!fb_device);
		fb_device = device;
	}

	udev_enumerate_unref(e);

	if (fb_device) {
		fb_device_path = strdup(udev_device_get_devnode(fb_device));
		udev_device_unref(fb_device);
	}

	return fb_device_path;
}

static struct fbdev_backend *
fbdev_backend_create(struct weston_compositor *compositor,
                     struct weston_fbdev_backend_config *param)
{
	struct fbdev_backend *backend;
	const char *seat_id = default_seat;
	const char *session_seat;

	session_seat = getenv("XDG_SEAT");
	if (session_seat)
		seat_id = session_seat;
	if (param->seat_id)
		seat_id = param->seat_id;

	weston_log("initializing fbdev backend\n");

	backend = zalloc(sizeof *backend);
	if (backend == NULL)
		return NULL;

	backend->compositor = compositor;
	compositor->backend = &backend->base;
	if (weston_compositor_set_presentation_clock_software(
							compositor) < 0)
		goto out_compositor;

	backend->udev = udev_new();
	if (backend->udev == NULL) {
		weston_log("Failed to initialize udev context.\n");
		goto out_compositor;
	}

	if (!param->device)
		param->device = find_framebuffer_device(backend, seat_id);
	if (!param->device) {
		weston_log("fatal: no framebuffer devices detected.\n");
		goto out_udev;
	}

	/* Set up the TTY. */
	backend->session_listener.notify = session_notify;
	wl_signal_add(&compositor->session_signal,
		      &backend->session_listener);
	compositor->launcher =
		weston_launcher_connect(compositor, param->tty, seat_id, false);
	if (!compositor->launcher) {
		weston_log("fatal: fbdev backend should be run using "
			   "weston-launch binary, or your system should "
			   "provide the logind D-Bus API.\n");
		goto out_udev;
	}

	backend->base.destroy = fbdev_backend_destroy;
	backend->base.create_output = fbdev_output_create;

	backend->prev_state = WESTON_COMPOSITOR_ACTIVE;

	weston_setup_vt_switch_bindings(compositor);

	if (pixman_renderer_init(compositor) < 0)
		goto out_launcher;

	if (!fbdev_head_create(backend, param->device))
		goto out_launcher;

	free(param->device);

	udev_input_init(&backend->input, compositor, backend->udev,
			seat_id, param->configure_device);

	return backend;

out_launcher:
	free(param->device);
	weston_launcher_destroy(compositor->launcher);

out_udev:
	udev_unref(backend->udev);

out_compositor:
	weston_compositor_shutdown(compositor);
	free(backend);

	return NULL;
}

static void
config_init_to_defaults(struct weston_fbdev_backend_config *config)
{
	config->tty = 0; /* default to current tty */
	config->device = NULL;
	config->seat_id = NULL;
}

WL_EXPORT int
weston_backend_init(struct weston_compositor *compositor,
		    struct weston_backend_config *config_base)
{
	struct fbdev_backend *b;
	struct weston_fbdev_backend_config config = {{ 0, }};

	if (config_base == NULL ||
	    config_base->struct_version != WESTON_FBDEV_BACKEND_CONFIG_VERSION ||
	    config_base->struct_size > sizeof(struct weston_fbdev_backend_config)) {
		weston_log("fbdev backend config structure is invalid\n");
		return -1;
	}

	config_init_to_defaults(&config);
	memcpy(&config, config_base, config_base->struct_size);

	b = fbdev_backend_create(compositor, &config);
	if (b == NULL)
		return -1;
	return 0;
}
