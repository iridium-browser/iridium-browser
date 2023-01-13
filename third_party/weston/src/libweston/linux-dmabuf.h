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

#include <stdint.h>

#define MAX_DMABUF_PLANES 4
#ifndef DRM_FORMAT_MOD_INVALID
#define DRM_FORMAT_MOD_INVALID ((1ULL<<56) - 1)
#endif
#ifndef DRM_FORMAT_MOD_LINEAR
#define DRM_FORMAT_MOD_LINEAR 0
#endif

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

#endif /* WESTON_LINUX_DMABUF_H */
