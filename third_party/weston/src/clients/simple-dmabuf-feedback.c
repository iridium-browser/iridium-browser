/*
 * Copyright © 2020 Collabora, Ltd.
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

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <libudev.h>
#include <sys/mman.h>
#include <time.h>

#include "shared/helpers.h"
#include "shared/platform.h"
#include "shared/weston-drm-fourcc.h"
#include <libweston/zalloc.h>
#include <libweston/pixel-formats.h>
#include "xdg-shell-client-protocol.h"
#include "linux-dmabuf-unstable-v1-client-protocol.h"
#include "presentation-time-client-protocol.h"

#include <xf86drm.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#define L_LINE "│   "
#define L_VAL  "├───"
#define L_LAST "└───"
#define L_GAP  "    "

#define NUM_BUFFERS 4

/* We have to hack the DRM-backend to pretend that planes of the underlying
 * hardware don't support this format. If you change the value of this constant,
 * do not forget to change in the DRM-backend as well. See main() description
 * for more details. */
#define INITIAL_BUFFER_FORMAT DRM_FORMAT_XRGB8888

static const char *vert_shader_text =
	"attribute vec4 pos;\n"
	"attribute vec4 color;\n"
	"varying vec4 v_color;\n"
	"void main() {\n"
	"	// We need to render upside-down, because rendering through an\n"
	"	// FBO causes the bottom of the image to be written to the top\n"
	"	// pixel row of the buffer, y-flipping the image.\n"
	"	gl_Position = vec4(1.0, -1.0, 1.0, 1.0) * pos;\n"
	"	v_color = color;\n"
	"}\n";

static const char *frag_shader_text =
	"precision mediump float;\n"
	"varying vec4 v_color;\n"
	"void main() {\n"
	"	gl_FragColor = v_color;\n"
	"}\n";

struct drm_format {
	uint32_t format;
	struct wl_array modifiers;
};

struct drm_format_array {
	struct wl_array arr;
};

struct dmabuf_feedback_format_table {
   unsigned int size;
   struct {
      uint32_t format;
      uint32_t padding; /* unused */
      uint64_t modifier;
   } *data;
};

struct dmabuf_feedback_tranche {
	dev_t target_device;
	bool is_scanout_tranche;
	struct drm_format_array formats;
};

struct dmabuf_feedback {
	dev_t main_device;
	struct dmabuf_feedback_format_table format_table;
	struct wl_array tranches;
	struct dmabuf_feedback_tranche pending_tranche;
};

struct output {
	struct wl_output *wl_output;
	int x, y;
	int width, height;
	int scale;
	bool initialized;
	struct {
		int width, height;
	} configure;
};

struct egl {
	EGLDisplay display;
	EGLContext context;
	EGLConfig conf;
	PFNEGLQUERYDMABUFMODIFIERSEXTPROC query_dmabuf_modifiers;
	PFNEGLCREATEIMAGEKHRPROC create_image;
	PFNEGLDESTROYIMAGEKHRPROC destroy_image;
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target_texture_2d;
};

struct gl {
	GLuint program;
	GLuint pos;
	GLuint color;
};

struct display {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct output output;
	struct xdg_wm_base *wm_base;
	struct zwp_linux_dmabuf_v1 *dmabuf;
	struct wp_presentation *presentation;
	struct gbm_device *gbm_device;
	struct egl egl;
};

struct buffer {
	bool created;
	bool valid;
	struct window *window;
	struct wl_buffer *buffer;
	enum {
		NOT_CREATED,
		IN_USE,
		AVAILABLE
	} status;
	int dmabuf_fds[4];
	struct gbm_bo *bo;
	EGLImageKHR egl_image;
	GLuint gl_texture;
	GLuint gl_fbo;
	int num_planes;
	uint32_t width, height, strides[4], offsets[4];
	uint32_t format;
	uint64_t modifier;
};

struct window {
	struct display *display;
	struct gl gl;
	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	struct wl_callback *callback;
	struct wp_presentation_feedback *presentation_feedback;
	bool wait_for_configure;
	bool presented_zero_copy;
	struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback_obj;
	struct dmabuf_feedback dmabuf_feedback, pending_dmabuf_feedback;
	int card_fd;
	struct drm_format format;
	uint32_t bo_flags;
	struct buffer buffers[NUM_BUFFERS];
};

static void
drm_format_array_init(struct drm_format_array *formats)
{
	wl_array_init(&formats->arr);
}

static void
drm_format_array_fini(struct drm_format_array *formats)
{
	struct drm_format *fmt;

	wl_array_for_each(fmt, &formats->arr)
		wl_array_release(&fmt->modifiers);

	wl_array_release(&formats->arr);
}

static struct drm_format *
drm_format_array_add_format(struct drm_format_array *formats, uint32_t format)
{
	struct drm_format *fmt;

	wl_array_for_each(fmt, &formats->arr)
		if (fmt->format == format)
			return fmt;

	fmt = wl_array_add(&formats->arr, sizeof(*fmt));
	assert(fmt && "error: could not allocate memory for format");

	fmt->format = format;
	wl_array_init(&fmt->modifiers);

	return fmt;
}

static void
drm_format_add_modifier(struct drm_format *format, uint64_t modifier)
{
	uint64_t *mod;

	wl_array_for_each(mod, &format->modifiers)
		if (*mod == modifier)
			return;

	mod = wl_array_add(&format->modifiers, sizeof(uint64_t));
	assert(mod && "error: could not allocate memory for modifier");

	*mod = modifier;
}

static void
dmabuf_feedback_format_table_fini(struct dmabuf_feedback_format_table *format_table)
{
	if (format_table->data && format_table->data != MAP_FAILED)
		munmap(format_table->data, format_table->size);
}

static void
dmabuf_feedback_format_table_init(struct dmabuf_feedback_format_table *format_table)
{
	memset(format_table, 0, sizeof(*format_table));
}

static void
dmabuf_feedback_tranche_fini(struct dmabuf_feedback_tranche *tranche)
{
	drm_format_array_fini(&tranche->formats);
}

static void
dmabuf_feedback_tranche_init(struct dmabuf_feedback_tranche *tranche)
{
	memset(tranche, 0, sizeof(*tranche));

	drm_format_array_init(&tranche->formats);
}

static void
dmabuf_feedback_fini(struct dmabuf_feedback *feedback)
{
	struct dmabuf_feedback_tranche *tranche;

	dmabuf_feedback_tranche_fini(&feedback->pending_tranche);

	wl_array_for_each(tranche, &feedback->tranches)
		dmabuf_feedback_tranche_fini(tranche);

	dmabuf_feedback_format_table_fini(&feedback->format_table);
}

static void
dmabuf_feedback_init(struct dmabuf_feedback *feedback)
{
	memset(feedback, 0, sizeof(*feedback));

	dmabuf_feedback_tranche_init(&feedback->pending_tranche);

	wl_array_init(&feedback->tranches);

	dmabuf_feedback_format_table_init(&feedback->format_table);
}

static GLuint
create_shader(const char *source, GLenum shader_type)
{
	GLuint shader;
	GLint status;

	shader = glCreateShader(shader_type);
	assert(shader != 0);

	glShaderSource(shader, 1, (const char **) &source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetShaderInfoLog(shader, 1000, &len, log);
		fprintf(stderr, "error: compiling %s: %.*s\n",
			shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment",
			len, log);
		return 0;
	}

	return shader;
}

static GLuint
create_and_link_program(GLuint vert, GLuint frag)
{
	GLint status;
	GLuint program = glCreateProgram();

	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetProgramInfoLog(program, 1000, &len, log);
		fprintf(stderr, "error: linking:\n%.*s\n", len, log);
		return 0;
	}

	return program;
}

static void
create_fbo_for_buffer(struct buffer *buffer)
{
	struct display *display = buffer->window->display;
	static const int general_attribs = 3;
	static const int plane_attribs = 5;
	static const int entries_per_attrib = 2;
	EGLint attribs[(general_attribs + (plane_attribs * 4)) * entries_per_attrib + 1];
	unsigned int atti = 0;

	attribs[atti++] = EGL_WIDTH;
	attribs[atti++] = buffer->width;
	attribs[atti++] = EGL_HEIGHT;
	attribs[atti++] = buffer->height;
	attribs[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
	attribs[atti++] = buffer->format;

	attribs[atti++] = EGL_DMA_BUF_PLANE0_FD_EXT;
	attribs[atti++] = buffer->dmabuf_fds[0];
	attribs[atti++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
	attribs[atti++] = (int) buffer->offsets[0];
	attribs[atti++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
	attribs[atti++] = (int) buffer->strides[0];
	attribs[atti++] = EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT;
	attribs[atti++] = buffer->modifier & 0xFFFFFFFF;
	attribs[atti++] = EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT;
	attribs[atti++] = buffer->modifier >> 32;

	if (buffer->num_planes > 1) {
		attribs[atti++] = EGL_DMA_BUF_PLANE1_FD_EXT;
		attribs[atti++] = buffer->dmabuf_fds[1];
		attribs[atti++] = EGL_DMA_BUF_PLANE1_OFFSET_EXT;
		attribs[atti++] = (int) buffer->offsets[1];
		attribs[atti++] = EGL_DMA_BUF_PLANE1_PITCH_EXT;
		attribs[atti++] = (int) buffer->strides[1];
		attribs[atti++] = EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT;
		attribs[atti++] = buffer->modifier & 0xFFFFFFFF;
		attribs[atti++] = EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT;
		attribs[atti++] = buffer->modifier >> 32;
	}

	if (buffer->num_planes > 2) {
		attribs[atti++] = EGL_DMA_BUF_PLANE2_FD_EXT;
		attribs[atti++] = buffer->dmabuf_fds[2];
		attribs[atti++] = EGL_DMA_BUF_PLANE2_OFFSET_EXT;
		attribs[atti++] = (int) buffer->offsets[2];
		attribs[atti++] = EGL_DMA_BUF_PLANE2_PITCH_EXT;
		attribs[atti++] = (int) buffer->strides[2];
		attribs[atti++] = EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT;
		attribs[atti++] = buffer->modifier & 0xFFFFFFFF;
		attribs[atti++] = EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT;
		attribs[atti++] = buffer->modifier >> 32;
	}

	if (buffer->num_planes > 3) {
		attribs[atti++] = EGL_DMA_BUF_PLANE3_FD_EXT;
		attribs[atti++] = buffer->dmabuf_fds[3];
		attribs[atti++] = EGL_DMA_BUF_PLANE3_OFFSET_EXT;
		attribs[atti++] = (int) buffer->offsets[3];
		attribs[atti++] = EGL_DMA_BUF_PLANE3_PITCH_EXT;
		attribs[atti++] = (int) buffer->strides[3];
		attribs[atti++] = EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT;
		attribs[atti++] = buffer->modifier & 0xFFFFFFFF;
		attribs[atti++] = EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT;
		attribs[atti++] = buffer->modifier >> 32;
	}

	attribs[atti] = EGL_NONE;

	assert(atti < ARRAY_LENGTH(attribs));

	buffer->egl_image = display->egl.create_image(display->egl.display,
						      EGL_NO_CONTEXT,
						      EGL_LINUX_DMA_BUF_EXT,
						      NULL, attribs);
	assert(buffer->egl_image != EGL_NO_IMAGE_KHR &&
	       "error: EGLImageKHR creation failed");

	if (eglMakeCurrent(display->egl.display, EGL_NO_SURFACE,
			   EGL_NO_SURFACE, display->egl.context) != EGL_TRUE)
		assert(0 && "error: failed to make context current");

	glGenTextures(1, &buffer->gl_texture);
	glBindTexture(GL_TEXTURE_2D, buffer->gl_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	display->egl.image_target_texture_2d(GL_TEXTURE_2D, buffer->egl_image);

	glGenFramebuffers(1, &buffer->gl_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, buffer->gl_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			       GL_TEXTURE_2D, buffer->gl_texture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		assert(0 && "error: FBO creation failed");
}

static void
buffer_free(struct buffer *buf)
{
	struct egl *egl = &buf->window->display->egl;
	int i;

	if (buf->buffer)
		wl_buffer_destroy(buf->buffer);

	if (buf->gl_fbo)
		glDeleteFramebuffers(1, &buf->gl_fbo);

	if (buf->gl_texture)
		glDeleteTextures(1, &buf->gl_texture);

	if (buf->egl_image)
		egl->destroy_image(egl->display, buf->egl_image);

	if (buf->bo)
		gbm_bo_destroy(buf->bo);

	for (i = 0; i < buf->num_planes; i++)
		close(buf->dmabuf_fds[i]);

	buf->created = false;
}

static void
create_dmabuf_buffer(struct window *window, struct buffer *buf, uint32_t width,
		     uint32_t height, uint32_t format, unsigned int count_modifiers,
		     uint64_t *modifiers, uint32_t bo_flags);

static void
buffer_recreate(struct buffer *buf, struct window *window)
{
	uint32_t width = window->display->output.width;
	uint32_t height = window->display->output.height;

	if (buf->created)
		buffer_free(buf);

	create_dmabuf_buffer(window, buf, width, height,
			     window->format.format,
			     window->format.modifiers.size / sizeof(uint64_t),
			     window->format.modifiers.data, window->bo_flags);
	buf->created = true;
	buf->valid = true;
}

static void
buffer_release(void *data, struct wl_buffer *buffer)
{
	struct buffer *buf = data;

	buf->status = AVAILABLE;
}

static const struct wl_buffer_listener buffer_listener = {
	buffer_release
};

static void
create_succeeded(void *data, struct zwp_linux_buffer_params_v1 *params,
		 struct wl_buffer *new_buffer)
{
	struct buffer *buf = data;

	buf->status = AVAILABLE;
	buf->buffer = new_buffer;
	wl_buffer_add_listener(buf->buffer, &buffer_listener, buf);
	zwp_linux_buffer_params_v1_destroy(params);
}

static void
create_failed(void *data, struct zwp_linux_buffer_params_v1 *params)
{
	struct buffer *buf = data;

	buf->buffer = NULL;
	zwp_linux_buffer_params_v1_destroy(params);

	assert(0 && "error: zwp_linux_buffer_params.create failed");
}

static const struct zwp_linux_buffer_params_v1_listener params_listener = {
	create_succeeded,
	create_failed
};

static void
create_dmabuf_buffer(struct window *window, struct buffer *buf, uint32_t width,
		     uint32_t height, uint32_t format, unsigned int count_modifiers,
		     uint64_t *modifiers, uint32_t bo_flags)
{
	struct display *display = window->display;
	static uint32_t flags = 0;
	struct zwp_linux_buffer_params_v1 *params;
	int i;

	buf->status = NOT_CREATED;
	buf->window = window;
	buf->width = width;
	buf->height = height;
	buf->format = format;

#ifdef HAVE_GBM_MODIFIERS
	if (count_modifiers > 0) {
#ifdef HAVE_GBM_BO_CREATE_WITH_MODIFIERS2
		buf->bo = gbm_bo_create_with_modifiers2(display->gbm_device,
							buf->width, buf->height,
							format, modifiers,
							count_modifiers,
							bo_flags);
#else
		buf->bo = gbm_bo_create_with_modifiers(display->gbm_device,
						       buf->width, buf->height,
						       format, modifiers,
						       count_modifiers);
#endif
		if (buf->bo)
			buf->modifier = gbm_bo_get_modifier(buf->bo);
	}
#endif

	if (!buf->bo) {
		buf->bo = gbm_bo_create(display->gbm_device, buf->width,
					buf->height, buf->format,
					bo_flags);
		buf->modifier = DRM_FORMAT_MOD_INVALID;
	}

	assert(buf->bo && "error: could not create GBM bo for buffer");

	buf->num_planes = gbm_bo_get_plane_count(buf->bo);

	params = zwp_linux_dmabuf_v1_create_params(window->display->dmabuf);
	zwp_linux_buffer_params_v1_add_listener(params, &params_listener, buf);

	for (i = 0; i < buf->num_planes; i++) {
		buf->dmabuf_fds[i] = gbm_bo_get_fd_for_plane(buf->bo, i);
		buf->strides[i] = gbm_bo_get_stride_for_plane(buf->bo, i);
		buf->offsets[i] = gbm_bo_get_offset(buf->bo, i);
		assert(buf->dmabuf_fds[i] >= 0 &&
		       "error: could not get fd for GBM bo");
		assert(buf->strides[i] > 0 &&
		       "error: could not get stride for GBM bo");

		zwp_linux_buffer_params_v1_add(params, buf->dmabuf_fds[i], i,
					       buf->offsets[i], buf->strides[i],
					       buf->modifier >> 32,
					       buf->modifier & 0xffffffff);
	}

	zwp_linux_buffer_params_v1_create(params, buf->width, buf->height,
					  buf->format, flags);

	create_fbo_for_buffer(buf);
}

static struct buffer *
window_next_buffer(struct window *window)
{
	unsigned int i;

	for (i = 0; i < NUM_BUFFERS; i++) {
		if (!window->buffers[i].created ||
		    (!window->buffers[i].valid &&
		     window->buffers[i].status == AVAILABLE))
			buffer_recreate(&window->buffers[i], window);
	}

	for (i = 0; i < NUM_BUFFERS; i++)
		if (window->buffers[i].status == AVAILABLE)
			return &window->buffers[i];

	while (true) {
		/* In this client, we create buffers lazily and also sometimes
		* have to recreate the buffers. As we are not using the
		* create_immed request from zwp_linux_dmabuf_v1, we need to wait
		* for an event from the server (what leads to create_succeeded()
		* being called in this client). */
		wl_display_roundtrip(window->display->display);

		for (i = 0; i < NUM_BUFFERS; i++)
			if (window->buffers[i].status == AVAILABLE)
				return &window->buffers[i];
	}

	return NULL;
}

static void
render(struct buffer *buffer)
{
	struct window *window = buffer->window;

	static const GLfloat verts[4][2] = {
		{ -0.5, -0.5 },
		{ -0.5,  0.5 },
		{  0.5, -0.5 },
		{  0.5,  0.5 }
	};
	static const GLfloat colors[4][3] = {
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 },
		{ 1, 1, 0 }
	};

	glBindFramebuffer(GL_FRAMEBUFFER, buffer->gl_fbo);

	glViewport(0, 0, buffer->width, buffer->height);

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glVertexAttribPointer(window->gl.pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
	glVertexAttribPointer(window->gl.color, 3, GL_FLOAT, GL_FALSE, 0, colors);
	glEnableVertexAttribArray(window->gl.pos);
	glEnableVertexAttribArray(window->gl.color);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(window->gl.pos);
	glDisableVertexAttribArray(window->gl.color);

	glFinish();
}

static const struct wl_callback_listener frame_listener;
static const struct wp_presentation_feedback_listener presentation_feedback_listener;

static void
redraw(void *data, struct wl_callback *callback, uint32_t time)
{
	struct window *window = data;
	struct buffer *buf;
	struct wl_region *region;

	buf = window_next_buffer(window);
	assert(buf && "error: all buffers are busy");

	render(buf);

	wl_surface_attach(window->surface, buf->buffer, 0, 0);
	wl_surface_damage(window->surface, 0, 0, buf->width, buf->height);

	if (callback)
		wl_callback_destroy(callback);

	window->callback = wl_surface_frame(window->surface);
	wl_callback_add_listener(window->callback, &frame_listener, window);

	if (window->presentation_feedback) {
		wp_presentation_feedback_destroy(window->presentation_feedback);
		window->presentation_feedback = NULL;
	}
	if (window->display->presentation) {
		window->presentation_feedback =
			wp_presentation_feedback(window->display->presentation,
						 window->surface);
		wp_presentation_feedback_add_listener(window->presentation_feedback,
						      &presentation_feedback_listener,
						      window);
	}

	wl_surface_commit(window->surface);
	buf->status = IN_USE;

	region = wl_compositor_create_region(window->display->compositor);
	wl_region_add(region, 0, 0, window->display->output.width,
		      window->display->output.height);
	wl_surface_set_opaque_region(window->surface, region);
	wl_region_destroy(region);
}

static const struct wl_callback_listener frame_listener = {
	redraw
};

static void presentation_feedback_handle_sync_output(void *data,
						     struct wp_presentation_feedback *feedback,
						     struct wl_output *output)
{
}

static void presentation_feedback_handle_presented(void *data,
						   struct wp_presentation_feedback *feedback,
						   uint32_t tv_sec_hi,
						   uint32_t tv_sec_lo,
						   uint32_t tv_nsec,
						   uint32_t refresh,
						   uint32_t seq_hi,
						   uint32_t seq_lo,
						   uint32_t flags)
{
	struct window *window = data;
	bool zero_copy = flags & WP_PRESENTATION_FEEDBACK_KIND_ZERO_COPY;

	if (zero_copy && !window->presented_zero_copy) {
		fprintf(stderr, "Presenting in zero-copy mode\n");
	}
	if (!zero_copy && window->presented_zero_copy) {
		fprintf(stderr, "Stopped presenting in zero-copy mode\n");
	}

	window->presented_zero_copy = zero_copy;
	wp_presentation_feedback_destroy(feedback);
	window->presentation_feedback = NULL;
}

static void presentation_feedback_handle_discarded(void *data,
						   struct wp_presentation_feedback *feedback)
{
	struct window *window = data;
	wp_presentation_feedback_destroy(feedback);
	window->presentation_feedback = NULL;
}

static const struct wp_presentation_feedback_listener presentation_feedback_listener = {
	.sync_output = presentation_feedback_handle_sync_output,
	.presented = presentation_feedback_handle_presented,
	.discarded = presentation_feedback_handle_discarded,
};


static void
window_buffers_invalidate(struct window *window)
{
	unsigned int i;

	for (i = 0; i < NUM_BUFFERS; i++)
		window->buffers[i].valid = false;
}

static void
xdg_surface_handle_configure(void *data, struct xdg_surface *surface,
			     uint32_t serial)
{
	struct window *window = data;
	struct output *output = &window->display->output;

	if (output->configure.width != output->width ||
	    output->configure.height != output->height) {
		output->width = output->configure.width;
		output->height = output->configure.height;

		window_buffers_invalidate (window);
	}

	xdg_surface_ack_configure(surface, serial);
	window->wait_for_configure = false;
}

static const struct xdg_surface_listener xdg_surface_listener = {
	xdg_surface_handle_configure,
};

static void
xdg_toplevel_handle_configure(void *data, struct xdg_toplevel *toplevel,
			      int32_t width, int32_t height,
			      struct wl_array *states)
{
	struct window *window = data;
	struct output *output = &window->display->output;

	output->configure.width = width;
	output->configure.height = height;
}

static void
xdg_toplevel_handle_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	assert(0 && "error: window closed, this should not happen");
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	xdg_toplevel_handle_configure,
	xdg_toplevel_handle_close,
};

static void
gbm_setup(struct window *window)
{
	struct display *display = window->display;

	display->gbm_device = gbm_create_device(window->card_fd);
	assert(display->gbm_device && "error: could not create GBM device");
}

static void
egl_setup(struct window *window)
{
	struct display *display = window->display;
	struct egl *egl = &display->egl;
	const char *egl_extensions = NULL;
	const char *gl_extensions = NULL;
	EGLint major, minor;
	EGLint ret;

	static const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
	};

	egl->display = weston_platform_get_egl_display(EGL_PLATFORM_GBM_KHR,
						       display->gbm_device, NULL);
	assert(egl->display && "error: could not create EGL display");

	ret = eglInitialize(egl->display, &major, &minor);
	assert(ret != EGL_FALSE && "error: failed to intialized EGL display");

	ret = eglBindAPI(EGL_OPENGL_ES_API);
	assert(ret != EGL_FALSE && "error: failed to set EGL API");

	egl_extensions = eglQueryString(egl->display, EGL_EXTENSIONS);
	assert(egl_extensions &&
	       "error: could not retrieve supported EGL extensions");

	assert(weston_check_egl_extension(egl_extensions,
					  "EGL_EXT_image_dma_buf_import"));
	assert(weston_check_egl_extension(egl_extensions,
					  "EGL_KHR_surfaceless_context"));
	assert(weston_check_egl_extension(egl_extensions,
					  "EGL_EXT_image_dma_buf_import_modifiers"));
	assert(weston_check_egl_extension(egl_extensions,
					  "EGL_KHR_no_config_context"));

	egl->context = eglCreateContext(egl->display, EGL_NO_CONFIG_KHR,
					EGL_NO_CONTEXT, context_attribs);
	assert(egl->context != EGL_NO_CONTEXT &&
	       "error: failed to create EGLContext");

	ret = eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
			     egl->context);
	assert(ret == EGL_TRUE && "error: failed to make context current");

	gl_extensions = (const char *) glGetString(GL_EXTENSIONS);
	assert(gl_extensions &&
	       "error: could not retrieve supported GL extensions");

	assert(weston_check_egl_extension(gl_extensions,
					  "GL_OES_EGL_image"));

	egl->query_dmabuf_modifiers =
		(void *) eglGetProcAddress("eglQueryDmaBufModifiersEXT");
	egl->create_image =
		(void *) eglGetProcAddress("eglCreateImageKHR");
	egl->destroy_image =
		(void *) eglGetProcAddress("eglDestroyImageKHR");
	egl->image_target_texture_2d =
		(void *) eglGetProcAddress("glEGLImageTargetTexture2DOES");
}

static void
gl_setup(struct window *window)
{
	struct gl *gl = &window->gl;
	GLuint vert;
	GLuint frag;

	vert = create_shader(vert_shader_text, GL_VERTEX_SHADER);
	assert(vert != 0 && "error: failed to compile vertex shader");
	frag = create_shader(frag_shader_text, GL_FRAGMENT_SHADER);
	assert(frag != 0 && "error: failed to compile fragment shader");

	gl->program = create_and_link_program(vert ,frag);
	assert(gl->program != 0 &&
	       "error: failed to attach shaders and create a program");

	glDeleteShader(vert);
	glDeleteShader(frag);

	gl->pos = glGetAttribLocation(window->gl.program, "pos");
	gl->color = glGetAttribLocation(window->gl.program, "color");

	glUseProgram(gl->program);
}

static void
destroy_window(struct window *window)
{
	unsigned int i;

	if (window->callback)
		wl_callback_destroy(window->callback);
	if (window->presentation_feedback)
		wp_presentation_feedback_destroy(window->presentation_feedback);

	for (i = 0; i < NUM_BUFFERS; i++)
		if (window->buffers[i].buffer)
			buffer_free(&window->buffers[i]);

	if (window->xdg_toplevel)
		xdg_toplevel_destroy(window->xdg_toplevel);
	if (window->xdg_surface)
		xdg_surface_destroy(window->xdg_surface);

	wl_surface_destroy(window->surface);

	close(window->card_fd);

	wl_array_release(&window->format.modifiers);

	dmabuf_feedback_fini(&window->dmabuf_feedback);
	dmabuf_feedback_fini(&window->pending_dmabuf_feedback);

	free(window);
}

static const struct zwp_linux_dmabuf_feedback_v1_listener dmabuf_feedback_listener;

static struct window *
create_window(struct display *display)
{
	struct window *window;

	window = zalloc(sizeof *window);
	assert(window && "error: failed to allocate memory for window");

	window->display = display;
	window->surface = wl_compositor_create_surface(display->compositor);

	dmabuf_feedback_init(&window->dmabuf_feedback);
	dmabuf_feedback_init(&window->pending_dmabuf_feedback);

	wl_array_init(&window->format.modifiers);

	window->dmabuf_feedback_obj =
		zwp_linux_dmabuf_v1_get_surface_feedback(display->dmabuf,
							 window->surface);

	zwp_linux_dmabuf_feedback_v1_add_listener(window->dmabuf_feedback_obj,
						  &dmabuf_feedback_listener,
						  window);
	wl_display_roundtrip(display->display);

	assert(window->format.format == INITIAL_BUFFER_FORMAT &&
	       "error: could not setup window->format based on dma-buf feedback");

	gbm_setup(window);
	egl_setup(window);
	gl_setup(window);


	window->xdg_surface = xdg_wm_base_get_xdg_surface(display->wm_base,
							  window->surface);
	assert(window->xdg_surface && "error: could not get XDG surface");
	xdg_surface_add_listener(window->xdg_surface, &xdg_surface_listener,
				 window);

	window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
	assert(window->xdg_toplevel && "error: could not get XDG toplevel");
	xdg_toplevel_add_listener(window->xdg_toplevel, &xdg_toplevel_listener,
				  window);

	xdg_toplevel_set_title(window->xdg_toplevel, "simple-dmabuf-feedback");
	xdg_toplevel_set_app_id(window->xdg_toplevel,
			"org.freedesktop.weston.simple-dmabuf-feedback");
	xdg_toplevel_set_fullscreen(window->xdg_toplevel, NULL);

	window->wait_for_configure = true;
	wl_surface_commit(window->surface);

	wl_display_roundtrip(display->display);

	assert(!window->wait_for_configure &&
	       "error: could not configure XDG surface");

	return window;
}

static char *
get_most_appropriate_node(const char *drm_node, bool is_scanout_device)
{
	drmDevice **devices;
	drmDevice *match = NULL;
	char *appropriate_node = NULL;
	int num_devices;
	int i, j;

	num_devices = drmGetDevices2(0, NULL, 0);
	assert(num_devices > 0 && "error: no drm devices available");

	devices = zalloc(num_devices * sizeof(*devices));
	assert(devices && "error: failed to allocate memory for drm devices");

	num_devices = drmGetDevices2(0, devices, num_devices);
	assert(num_devices > 0 && "error: no drm devices available");

	for (i = 0; i < num_devices && match == NULL; i++) {
		for (j = 0; j < DRM_NODE_MAX && match == NULL; j++) {
			if (!(devices[i]->available_nodes & (1 << j)))
				continue;
			if (strcmp(devices[i]->nodes[j], drm_node) == 0)
				match = devices[i];
		}
	}
	assert(match != NULL && "error: could not find device on the list");
	assert(match->available_nodes & (1 << DRM_NODE_PRIMARY));

	if (is_scanout_device) {
		appropriate_node = strdup(match->nodes[DRM_NODE_PRIMARY]);
	} else {
		if (match->available_nodes & (1 << DRM_NODE_RENDER))
			appropriate_node = strdup(match->nodes[DRM_NODE_RENDER]);
		else
			appropriate_node = strdup(match->nodes[DRM_NODE_PRIMARY]);
	}
	assert(appropriate_node && "error: could not get drm node");

	for (i = 0; i < num_devices; i++)
		drmFreeDevice(&devices[i]);
	free(devices);

	return appropriate_node;
}

static char *
get_drm_node(dev_t device, bool is_scanout_device)
{
	struct udev *udev;
	struct udev_device *udev_dev;
	const char *drm_node;

	udev = udev_new();
	assert(udev && "error: failed to create udev context object");

	udev_dev = udev_device_new_from_devnum(udev, 'c', device);
	assert(udev_dev && "error: failed to create udev device");

	drm_node = udev_device_get_devnode(udev_dev);
	assert(drm_node && "error: failed to retrieve drm node");

	udev_unref(udev);

	return get_most_appropriate_node(drm_node, is_scanout_device);
}

static void
dmabuf_feedback_format_table(void *data,
			       struct zwp_linux_dmabuf_feedback_v1 *zwp_linux_dmabuf_feedback_v1,
			       int32_t fd, uint32_t size)
{
	struct window *window = data;
	struct dmabuf_feedback *feedback = &window->pending_dmabuf_feedback;

	feedback->format_table.size = size;
	feedback->format_table.data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
}

static void
dmabuf_feedback_main_device(void *data,
			    struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
			    struct wl_array *dev)
{
	struct window *window = data;
	struct dmabuf_feedback *feedback = &window->pending_dmabuf_feedback;
	char *drm_node;

	assert(dev->size == sizeof(feedback->main_device) &&
	       "error: compositor didn't send a dev_t, size is wrong");
	memcpy(&feedback->main_device, dev->data, sizeof(dev));

	drm_node = get_drm_node(feedback->main_device, false);
	assert(drm_node && "error: failed to retrieve drm node");

	fprintf(stderr, "feedback: main device %s\n", drm_node);

	if (!window->card_fd) {
		window->card_fd = open(drm_node, O_RDWR | O_CLOEXEC);
		assert(window->card_fd > 0 && "error: could not open card node");
	}

	free(drm_node);
}

static void
dmabuf_feedback_tranche_target_device(void *data,
				   struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
				   struct wl_array *dev)
{
	struct window *window = data;
	struct dmabuf_feedback *feedback = &window->pending_dmabuf_feedback;

	assert(dev->size == sizeof(feedback->pending_tranche.target_device) &&
	       "error: compositor didn't send a dev_t, size is wrong");

	memcpy(&feedback->pending_tranche.target_device, dev->data, sizeof(dev));
}

static void
dmabuf_feedback_tranche_flags(void *data,
			   struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
			   uint32_t flags)
{
	struct window *window = data;
	struct dmabuf_feedback *feedback = &window->pending_dmabuf_feedback;

	if (flags & ZWP_LINUX_DMABUF_FEEDBACK_V1_TRANCHE_FLAGS_SCANOUT)
		feedback->pending_tranche.is_scanout_tranche = true;
}

static void
dmabuf_feedback_tranche_formats(void *data,
			     struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback,
			     struct wl_array *indices)
{
	struct window *window = data;
	struct dmabuf_feedback *feedback = &window->pending_dmabuf_feedback;
	struct drm_format *fmt;
	uint64_t modifier;
	uint32_t format;
	uint16_t *index;

	/* Compositor may advertise or not a format table. If it does, we use
	 * it. Otherwise, we steal the most recent advertised format table */
	if (feedback->format_table.data == NULL) {
		feedback->format_table = window->dmabuf_feedback.format_table;
		dmabuf_feedback_format_table_init(&window->dmabuf_feedback.format_table);
	}
	assert(feedback->format_table.data != NULL &&
	       "error: compositor should advertise format table");
	assert(feedback->format_table.data != MAP_FAILED &&
	       "error: we could not map format table advertised by compositor");

	wl_array_for_each(index, indices) {
		format = feedback->format_table.data[*index].format;
		modifier = feedback->format_table.data[*index].modifier;

		fmt = drm_format_array_add_format(&feedback->pending_tranche.formats,
						  format);
		drm_format_add_modifier(fmt, modifier);
	}
}

static char
bits2graph(uint32_t value, unsigned bitoffset)
{
	int c = (value >> bitoffset) & 0xff;

	if (isgraph(c) || isspace(c))
		return c;

	return '?';
}

static void
fourcc2str(uint32_t format, char *str, int len)
{
	int i;

	assert(len >= 5);

	for (i = 0; i < 4; i++)
		str[i] = bits2graph(format, i * 8);
	str[i] = '\0';
}

static void
print_tranche_format_modifier(uint32_t format, uint64_t modifier)
{
	const struct pixel_format_info *fmt_info;
	char *format_str;
	char *mod_name;
	int len;

	mod_name = pixel_format_get_modifier(modifier);
	fmt_info = pixel_format_get_info(format);

	if (fmt_info) {
		len = asprintf(&format_str, "%s", fmt_info->drm_format_name);
	} else {
		char fourcc_str[5];

		fourcc2str(format, fourcc_str, sizeof(fourcc_str));
		len = asprintf(&format_str, "%s (0x%08x)", fourcc_str, format);
	}
	assert(len > 0);

	fprintf(stderr, L_LINE L_VAL " format %s, modifier %s\n",
		format_str, mod_name);

	free(format_str);
	free(mod_name);
}

static void
print_dmabuf_feedback_tranche(struct dmabuf_feedback_tranche *tranche)
{
	char *drm_node;
	struct drm_format *fmt;
	uint64_t *mod;

	drm_node = get_drm_node(tranche->target_device, tranche->is_scanout_tranche);
	assert(drm_node && "error: could not retrieve drm node");

	fprintf(stderr, L_VAL " tranche: target device %s, %s\n",
		drm_node, tranche->is_scanout_tranche ? "scanout" : "no flags");

	wl_array_for_each(fmt, &tranche->formats.arr)
		wl_array_for_each(mod, &fmt->modifiers)
			print_tranche_format_modifier(fmt->format, *mod);

	fprintf(stderr, L_LINE L_LAST " end of tranche\n");
}

static void
dmabuf_feedback_tranche_done(void *data,
			  struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback)
{
	struct window *window = data;
	struct dmabuf_feedback *feedback = &window->pending_dmabuf_feedback;
	struct dmabuf_feedback_tranche *tranche;

	print_dmabuf_feedback_tranche(&feedback->pending_tranche);

	tranche = wl_array_add(&feedback->tranches, sizeof(*tranche));
	assert(tranche && "error: could not allocate memory for tranche");

	memcpy(tranche, &feedback->pending_tranche, sizeof(*tranche));

	dmabuf_feedback_tranche_init(&feedback->pending_tranche);
}

static bool
pick_initial_format_from_renderer_tranche(struct window *window,
					  struct dmabuf_feedback_tranche *tranche)
{
	struct drm_format *fmt;

	wl_array_for_each(fmt, &tranche->formats.arr) {
		/* Skip formats that are not the one we want to start with. */
		if (fmt->format != INITIAL_BUFFER_FORMAT)
			continue;

		window->format.format = fmt->format;
		wl_array_copy(&window->format.modifiers, &fmt->modifiers);

		window->bo_flags = GBM_BO_USE_RENDERING;

		return true;
	}
	return false;
}

static bool
pick_format_from_scanout_tranche(struct window *window,
				 struct dmabuf_feedback_tranche *tranche)
{
	struct drm_format *fmt;
	const struct pixel_format_info *format_info;

	wl_array_for_each(fmt, &tranche->formats.arr) {

		/* Ignore the format that we want to pick from the render
		 * tranche. */
		if (fmt->format == INITIAL_BUFFER_FORMAT)
			continue;

		/* Format should be supported by the compositor. */
		format_info = pixel_format_get_info(fmt->format);
		if (!format_info)
			continue;

		wl_array_release(&window->format.modifiers);
		wl_array_init(&window->format.modifiers);

		window->format.format = fmt->format;
		wl_array_copy(&window->format.modifiers, &fmt->modifiers);

		window->bo_flags = GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT;

		return true;
	}
	return false;
}

static void
dmabuf_feedback_done(void *data, struct zwp_linux_dmabuf_feedback_v1 *dmabuf_feedback)
{
	struct window *window = data;
	struct dmabuf_feedback_tranche *tranche;
	bool got_scanout_tranche = false;

	fprintf(stderr, L_LAST " end of dma-buf feedback\n\n");

	/* The first time that we receive dma-buf feedback for a surface it
	 * contains only the renderer tranches. We pick the INITIAL_BUFFER_FORMAT
	 * from there. Then the compositor should detect that the format is
	 * unsupported by the underlying hardware (not actually, but you should
	 * have faked this in the DRM-backend) and send the scanout tranches. We
	 * use the formats/modifiers of the scanout tranches to reallocate our
	 * buffers. */
	wl_array_for_each(tranche, &window->pending_dmabuf_feedback.tranches) {
		if (tranche->is_scanout_tranche) {
			got_scanout_tranche = true;
			if (pick_format_from_scanout_tranche(window, tranche)) {
				window_buffers_invalidate(window);
				break;
			}
		}
		if (pick_initial_format_from_renderer_tranche(window, tranche))
			break;
	}

	if (got_scanout_tranche) {
		assert(window->format.format != INITIAL_BUFFER_FORMAT &&
		       "error: no valid pair of format/modifier in the scanout tranches");
	} else {
		assert(window->format.format == INITIAL_BUFFER_FORMAT &&
		       "error: INITIAL_BUFFER_FORMAT not supported by the hardware");
	}

	dmabuf_feedback_fini(&window->dmabuf_feedback);
	window->dmabuf_feedback = window->pending_dmabuf_feedback;
	dmabuf_feedback_init(&window->pending_dmabuf_feedback);
}

static const struct zwp_linux_dmabuf_feedback_v1_listener dmabuf_feedback_listener = {
	.format_table = dmabuf_feedback_format_table,
	.main_device = dmabuf_feedback_main_device,
	.tranche_target_device = dmabuf_feedback_tranche_target_device,
	.tranche_formats = dmabuf_feedback_tranche_formats,
	.tranche_flags = dmabuf_feedback_tranche_flags,
	.tranche_done = dmabuf_feedback_tranche_done,
	.done = dmabuf_feedback_done,
};

static void
output_handle_geometry(void *data, struct wl_output *wl_output, int x, int y,
		       int physical_width, int physical_height, int subpixel,
		       const char *make, const char *model, int32_t transform)
{
	struct output *output = data;

	output->x = x;
	output->y = y;
}

static void
output_handle_mode(void *data, struct wl_output *wl_output, uint32_t flags,
		   int width, int height, int refresh)
{
}

static void
output_handle_scale(void *data, struct wl_output *wl_output, int scale)
{
	struct output *output = data;

	output->scale = scale;
}

static void
output_handle_done(void *data, struct wl_output *wl_output)
{
	struct output *output = data;

	output->initialized = true;
}

static const struct wl_output_listener output_listener = {
	output_handle_geometry,
	output_handle_mode,
	output_handle_done,
	output_handle_scale,
};

static void
xdg_wm_base_ping(void *data, struct xdg_wm_base *wm_base, uint32_t serial)
{
	xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
	xdg_wm_base_ping,
};

static void
registry_handle_global(void *data, struct wl_registry *registry,
		       uint32_t id, const char *interface, uint32_t version)
{
	struct display *d = data;

	if (strcmp(interface, "wl_compositor") == 0) {
		d->compositor = wl_registry_bind(registry, id,
						 &wl_compositor_interface,
						 1);
 	} else if (strcmp(interface, "xdg_wm_base") == 0) {
		d->wm_base = wl_registry_bind(registry, id,
					      &xdg_wm_base_interface,
					      1);
		xdg_wm_base_add_listener(d->wm_base, &xdg_wm_base_listener, d);
	} else if (strcmp(interface, "wl_output") == 0) {
		d->output.wl_output = wl_registry_bind(registry, id,
						       &wl_output_interface,
						       MIN(version, 3));
		wl_output_add_listener(d->output.wl_output,
				       &output_listener, &d->output);
	} else if (strcmp(interface, "zwp_linux_dmabuf_v1") == 0) {
		if (version < ZWP_LINUX_DMABUF_V1_GET_DEFAULT_FEEDBACK_SINCE_VERSION)
			return;
		d->dmabuf = wl_registry_bind(registry, id,
					     &zwp_linux_dmabuf_v1_interface,
					     MIN(version, 4));
	} else if (strcmp(interface, "wp_presentation") == 0) {
		d->presentation = wl_registry_bind(registry, id,
					           &wp_presentation_interface,
						   1);
	}
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
			      uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};

static void
destroy_display(struct display *display)
{
	gbm_device_destroy(display->gbm_device);

	if (display->egl.context != EGL_NO_CONTEXT)
		eglDestroyContext(display->egl.display, display->egl.context);
	if (display->egl.display != EGL_NO_DISPLAY)
		eglTerminate(display->egl.display);

	if (display->presentation)
		wp_presentation_destroy(display->presentation);

	zwp_linux_dmabuf_v1_destroy(display->dmabuf);
	xdg_wm_base_destroy(display->wm_base);

	wl_compositor_destroy(display->compositor);
	wl_registry_destroy(display->registry);

	wl_display_flush(display->display);
	wl_display_disconnect(display->display);

	free(display);
}

static struct display *
create_display()
{
	struct display *display = NULL;

	display = zalloc(sizeof *display);
	assert(display && "error: failed to allocate memory for display");

	display->display = wl_display_connect(NULL);
	assert(display->display && "error: could not connect to compositor");

	display->registry = wl_display_get_registry(display->display);
	assert(display->registry && "error: could not get registry");
	wl_registry_add_listener(display->registry, &registry_listener, display);

	wl_display_roundtrip(display->display);
	assert(display->compositor && "error: could not create compositor interface");
	assert(display->dmabuf && "error: dma-buf feedback is not supported by compositor");

	wl_display_roundtrip(display->display);
	assert(display->wm_base && "error: xdg shell is not supported by compositor");
	assert(display->output.initialized && "error: output not initialized");

	return display;
}

/* Simple client to test the dma-buf feedback implementation. This does not
 * replace the need to implement a dma-buf feedback test that can be run in the
 * CI. But as we still don't know exactly how to do this, this client can be
 * helpful to run tests manually. It can also be helpful to test other
 * compositors.
 *
 * In order to use this client, we have to hack the DRM-backend of the
 * compositor to pretend that INITIAL_BUFFER_FORMAT is not supported by the
 * planes of the underlying hardware.
 *
 * How this client works:
 *
 * This client creates a surface and buffers for it with the same resolution of
 * the output mode in use. Also, it sets the surface to fullscreen. So we have
 * everything set to allow the surface to be placed in an overlay plane. But as
 * these buffers are created with INITIAL_BUFFER_FORMAT, they are not eligible
 * for direct scanout.
 *
 * When Weston creates a client's surface, it adds only the renderer tranche to
 * its dma-buf feedback object and send the feedback to the client. But as the
 * repaint cycles start and Weston detects that the view of the client is not
 * eligible for direct scanout because of the incompatibility of the
 * framebuffer's format/modifier pair and the KMS device, it adds a scanout
 * tranche to the feedback and resend it. In this scanout tranche the client can
 * find parameters to re-allocate its buffers and increase its chances of
 * hitting direct scanout. */
int
main(int argc, char **argv)
{
	struct display *display;
	struct window *window;
	int ret = 0;
	struct timespec start_time, current_time;
	const time_t MAX_TIME_SECONDS = 3;
	time_t delta_time = 0;

	fprintf(stderr, "This client was written with the purpose of manually test " \
			"Weston's dma-buf feedback implementation. See main() " \
			"description for more details on how to test this.\n\n");

	display = create_display();
	window = create_window(display);

	clock_gettime(CLOCK_MONOTONIC, &start_time);

	redraw(window, NULL, 0);
	while (ret != -1 && delta_time < MAX_TIME_SECONDS) {
		ret = wl_display_dispatch(display->display);
		clock_gettime(CLOCK_MONOTONIC, &current_time);
		delta_time = current_time.tv_sec - start_time.tv_sec;
	}

	destroy_window(window);
	destroy_display(display);

	return 0;
}
