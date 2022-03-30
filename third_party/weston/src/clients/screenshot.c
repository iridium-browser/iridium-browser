/*
 * Copyright © 2008 Kristian Høgsberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <cairo.h>

#include <wayland-client.h>
#include "weston-screenshooter-client-protocol.h"
#include "shared/os-compatibility.h"
#include "shared/xalloc.h"
#include "shared/file-util.h"

/* The screenshooter is a good example of a custom object exposed by
 * the compositor and serves as a test bed for implementing client
 * side marshalling outside libwayland.so */


struct screenshooter_output {
	struct wl_output *output;
	struct wl_buffer *buffer;
	int width, height, offset_x, offset_y;
	void *data;
	struct wl_list link;
};

struct buffer_size {
	int width, height;

	int min_x, min_y;
	int max_x, max_y;
};

struct screenshooter_data {
	struct wl_shm *shm;
	struct wl_list output_list;

	struct weston_screenshooter *screenshooter;
	int buffer_copy_done;
};


static void
display_handle_geometry(void *data,
			struct wl_output *wl_output,
			int x,
			int y,
			int physical_width,
			int physical_height,
			int subpixel,
			const char *make,
			const char *model,
			int transform)
{
	struct screenshooter_output *output;

	output = wl_output_get_user_data(wl_output);

	if (wl_output == output->output) {
		output->offset_x = x;
		output->offset_y = y;
	}
}

static void
display_handle_mode(void *data,
		    struct wl_output *wl_output,
		    uint32_t flags,
		    int width,
		    int height,
		    int refresh)
{
	struct screenshooter_output *output;

	output = wl_output_get_user_data(wl_output);

	if (wl_output == output->output && (flags & WL_OUTPUT_MODE_CURRENT)) {
		output->width = width;
		output->height = height;
	}
}

static const struct wl_output_listener output_listener = {
	display_handle_geometry,
	display_handle_mode
};

static void
screenshot_done(void *data, struct weston_screenshooter *screenshooter)
{
	struct screenshooter_data *sh_data = data;
	sh_data->buffer_copy_done = 1;
}

static const struct weston_screenshooter_listener screenshooter_listener = {
	screenshot_done
};

static void
handle_global(void *data, struct wl_registry *registry,
	      uint32_t name, const char *interface, uint32_t version)
{
	static struct screenshooter_output *output;
	struct screenshooter_data *sh_data = data;

	if (strcmp(interface, "wl_output") == 0) {
		output = xmalloc(sizeof *output);
		output->output = wl_registry_bind(registry, name,
						  &wl_output_interface, 1);
		wl_list_insert(&sh_data->output_list, &output->link);
		wl_output_add_listener(output->output, &output_listener, output);
	} else if (strcmp(interface, "wl_shm") == 0) {
		sh_data->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, "weston_screenshooter") == 0) {
		sh_data->screenshooter = wl_registry_bind(registry, name,
							  &weston_screenshooter_interface,
							  1);
	}
}

static void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	/* XXX: unimplemented */
}

static const struct wl_registry_listener registry_listener = {
	handle_global,
	handle_global_remove
};

static struct wl_buffer *
screenshot_create_shm_buffer(int width, int height, void **data_out,
			     struct wl_shm *shm)
{
	struct wl_shm_pool *pool;
	struct wl_buffer *buffer;
	int fd, size, stride;
	void *data;

	stride = width * 4;
	size = stride * height;

	fd = os_create_anonymous_file(size);
	if (fd < 0) {
		fprintf(stderr, "creating a buffer file for %d B failed: %s\n",
			size, strerror(errno));
		return NULL;
	}

	data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		fprintf(stderr, "mmap failed: %s\n", strerror(errno));
		close(fd);
		return NULL;
	}

	pool = wl_shm_create_pool(shm, fd, size);
	close(fd);
	buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
					   WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(pool);

	*data_out = data;

	return buffer;
}

static void
screenshot_write_png(const struct buffer_size *buff_size,
		     struct wl_list *output_list)
{
	int output_stride, buffer_stride, i;
	cairo_surface_t *surface;
	void *data, *d, *s;
	struct screenshooter_output *output, *next;
	FILE *fp;
	char filepath[PATH_MAX];

	buffer_stride = buff_size->width * 4;

	data = xmalloc(buffer_stride * buff_size->height);
	if (!data)
		return;

	wl_list_for_each_safe(output, next, output_list, link) {
		output_stride = output->width * 4;
		s = output->data;
		d = data + (output->offset_y - buff_size->min_y) * buffer_stride +
			   (output->offset_x - buff_size->min_x) * 4;

		for (i = 0; i < output->height; i++) {
			memcpy(d, s, output_stride);
			d += buffer_stride;
			s += output_stride;
		}

		free(output);
	}

	surface = cairo_image_surface_create_for_data(data,
						      CAIRO_FORMAT_ARGB32,
						      buff_size->width,
						      buff_size->height,
						      buffer_stride);

	fp = file_create_dated(getenv("XDG_PICTURES_DIR"), "wayland-screenshot-",
			       ".png", filepath, sizeof(filepath));
	if (fp) {
		fclose (fp);
		cairo_surface_write_to_png(surface, filepath);
	}
	cairo_surface_destroy(surface);
	free(data);
}

static int
screenshot_set_buffer_size(struct buffer_size *buff_size, struct wl_list *output_list)
{
	struct screenshooter_output *output;
	buff_size->min_x = buff_size->min_y = INT_MAX;
	buff_size->max_x = buff_size->max_y = INT_MIN;
	int position = 0;

	wl_list_for_each_reverse(output, output_list, link) {
		output->offset_x = position;
		position += output->width;
	}

	wl_list_for_each(output, output_list, link) {
		buff_size->min_x = MIN(buff_size->min_x, output->offset_x);
		buff_size->min_y = MIN(buff_size->min_y, output->offset_y);
		buff_size->max_x =
			MAX(buff_size->max_x, output->offset_x + output->width);
		buff_size->max_y =
			MAX(buff_size->max_y, output->offset_y + output->height);
	}

	if (buff_size->max_x <= buff_size->min_x ||
	    buff_size->max_y <= buff_size->min_y)
		return -1;

	buff_size->width = buff_size->max_x - buff_size->min_x;
	buff_size->height = buff_size->max_y - buff_size->min_y;

	return 0;
}

int main(int argc, char *argv[])
{
	struct wl_display *display;
	struct wl_registry *registry;
	struct screenshooter_output *output;
	struct buffer_size buff_size = {};
	struct screenshooter_data sh_data = {};

	display = wl_display_connect(NULL);
	if (display == NULL) {
		fprintf(stderr, "failed to create display: %s\n",
			strerror(errno));
		return -1;
	}

	wl_list_init(&sh_data.output_list);
	registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, &sh_data);
	wl_display_dispatch(display);
	wl_display_roundtrip(display);
	if (sh_data.screenshooter == NULL) {
		fprintf(stderr, "display doesn't support screenshooter\n");
		return -1;
	}

	weston_screenshooter_add_listener(sh_data.screenshooter,
					  &screenshooter_listener,
					  &sh_data);

	if (screenshot_set_buffer_size(&buff_size, &sh_data.output_list))
		return -1;


	wl_list_for_each(output, &sh_data.output_list, link) {
		output->buffer =
			screenshot_create_shm_buffer(output->width,
						     output->height,
						     &output->data,
						     sh_data.shm);
		weston_screenshooter_shoot(sh_data.screenshooter,
					   output->output,
					   output->buffer);
		sh_data.buffer_copy_done = 0;
		while (!sh_data.buffer_copy_done)
			wl_display_roundtrip(display);
	}

	screenshot_write_png(&buff_size, &sh_data.output_list);

	return 0;
}
