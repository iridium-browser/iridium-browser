/*
 * Copyright © 2014, 2015 Collabora, Ltd.
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

#ifndef WESTON_LINUX_DMABUF_H
#define WESTON_LINUX_DMABUF_H

#include <libweston/libweston.h>

#define MAX_DMABUF_PLANES 4

struct linux_dmabuf_buffer;
typedef void (*dmabuf_user_data_destroy_func)(
			struct linux_dmabuf_buffer *buffer);

struct dmabuf_attributes {
	int32_t width;
	int32_t height;
	uint32_t format;
	uint32_t flags; /* enum zlinux_buffer_params_flags */
	int n_planes;
	int fd[MAX_DMABUF_PLANES];
	uint32_t offset[MAX_DMABUF_PLANES];
	uint32_t stride[MAX_DMABUF_PLANES];
	uint64_t modifier[MAX_DMABUF_PLANES];
};

struct linux_dmabuf_buffer {
	struct wl_resource *buffer_resource;
	struct wl_resource *params_resource;
	struct weston_compositor *compositor;
	struct dmabuf_attributes attributes;

	void *user_data;
	dmabuf_user_data_destroy_func user_data_destroy_func;

	/* XXX:
	 *
	 * Add backend private data. This would be for the backend
	 * to do all additional imports it might ever use in advance.
	 * The basic principle, even if not implemented in drivers today,
	 * is that dmabufs are first attached, but the actual allocation
	 * is deferred to first use. This would allow the exporter and all
	 * attachers to agree on how to allocate.
	 *
	 * The DRM backend would use this to create drmFBs for each
	 * dmabuf_buffer, just in case at some point it would become
	 * feasible to scan it out directly. This would improve the
	 * possibilities to successfully scan out, avoiding compositing.
	 */

	/**< marked as scan-out capable, avoids any composition */
	bool direct_display;
};

enum weston_dmabuf_feedback_tranche_preference {
	RENDERER_PREF = 0,
	SCANOUT_PREF = 1
};

struct weston_dmabuf_feedback_format_table {
	int fd;
	unsigned int size;

	/* This is a pointer to the region of memory where we mapped the file
	 * that clients receive. We fill it with the format/modifier pairs
	 * supported by the renderer. We don't formats not supported by the
	 * renderer in the table, as we must always be able to fallback to the
	 * renderer if direct scanout fails. */
	struct {
		uint32_t format;
		uint32_t pad; /* unused */
		uint64_t modifier;
	} *data;

	/* Indices of the renderer formats in the table. As the table consists
	 * of formats supported by the renderer, this goes from 0 to the number
	 * of pairs in the table. */
	struct wl_array renderer_formats_indices;
	/* Indices of the scanout formats (union of KMS plane's supported
         * formats intersected with the renderer formats). */
	struct wl_array scanout_formats_indices;
};

struct weston_dmabuf_feedback {
	/* We can have multiple clients subscribing to the same surface dma-buf
	 * feedback. As they are dynamic and we need to re-send them multiple
	 * times during Weston's lifetime, we need to keep track of the
	 * resources of each client. In the case of the default feedback this is
	 * not necessary, as we only advertise them when clients subscribe. IOW,
	 * default feedback events are never re-sent. */
	struct wl_list resource_list;

	dev_t main_device;

	/* weston_dmabuf_feedback_tranche::link */
	struct wl_list tranche_list;

	/* We use this timer to know if the scene has stabilized and that would
	 * be useful to resend dma-buf feedback events to clients. Consider the
	 * timer off when action_needed == ACTION_NEEDED_NONE. See enum
	 * actions_needed_dmabuf_feedback. */
	struct timespec timer;
	uint32_t action_needed;
};

struct weston_dmabuf_feedback_tranche {
	/* weston_dmabuf_feedback::tranche_list */
	struct wl_list link;

	/* Instead of destroying tranches and reconstructing them when necessary
	 * (it can be expensive), we have this flag to know if the tranche
	 * should be advertised or not. This is particularly useful for the
	 * scanout tranche, as based on the DRM-backend feedback and the current
	 * scene (which changes a lot during compositor lifetime) we can decide
	 * to send it or not. */
	bool active;

	dev_t target_device;
	uint32_t flags;
	enum weston_dmabuf_feedback_tranche_preference preference;

	struct wl_array formats_indices;
};

int
linux_dmabuf_setup(struct weston_compositor *compositor);

int
weston_direct_display_setup(struct weston_compositor *compositor);

struct linux_dmabuf_buffer *
linux_dmabuf_buffer_get(struct wl_resource *resource);

void
linux_dmabuf_buffer_set_user_data(struct linux_dmabuf_buffer *buffer,
				  void *data,
				  dmabuf_user_data_destroy_func func);
void *
linux_dmabuf_buffer_get_user_data(struct linux_dmabuf_buffer *buffer);

void
linux_dmabuf_buffer_send_server_error(struct linux_dmabuf_buffer *buffer,
				      const char *msg);

struct weston_dmabuf_feedback *
weston_dmabuf_feedback_create(dev_t main_device);

void
weston_dmabuf_feedback_destroy(struct weston_dmabuf_feedback *dmabuf_feedback);

void
weston_dmabuf_feedback_send_all(struct weston_dmabuf_feedback *dmabuf_feedback,
				struct weston_dmabuf_feedback_format_table *format_table);

struct weston_dmabuf_feedback_tranche *
weston_dmabuf_feedback_find_tranche(struct weston_dmabuf_feedback *dmabuf_feedback,
				    dev_t target_device, uint32_t flags,
				    enum weston_dmabuf_feedback_tranche_preference preference);

struct weston_dmabuf_feedback_format_table *
weston_dmabuf_feedback_format_table_create(const struct weston_drm_format_array *renderer_formats);

void
weston_dmabuf_feedback_format_table_destroy(struct weston_dmabuf_feedback_format_table *format_table);

int
weston_dmabuf_feedback_format_table_set_scanout_indices(struct weston_dmabuf_feedback_format_table *format_table,
							const struct weston_drm_format_array *scanout_formats);

struct weston_dmabuf_feedback_tranche *
weston_dmabuf_feedback_tranche_create(struct weston_dmabuf_feedback *dmabuf_feedback,
				      struct weston_dmabuf_feedback_format_table *format_table,
				      dev_t target_device, uint32_t flags,
				      enum weston_dmabuf_feedback_tranche_preference preference);

#endif /* WESTON_LINUX_DMABUF_H */
