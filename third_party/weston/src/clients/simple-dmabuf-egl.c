/*
 * Copyright © 2011 Benjamin Franzke
 * Copyright © 2010 Intel Corporation
 * Copyright © 2014,2018 Collabora Ltd.
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
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <xf86drm.h>
#include <gbm.h>

#include <wayland-client.h>
#include "shared/helpers.h"
#include "shared/platform.h"
#include "shared/weston-drm-fourcc.h"
#include <libweston/zalloc.h>
#include "xdg-shell-client-protocol.h"
#include "fullscreen-shell-unstable-v1-client-protocol.h"
#include "linux-dmabuf-unstable-v1-client-protocol.h"
#include "weston-direct-display-client-protocol.h"
#include "linux-explicit-synchronization-unstable-v1-client-protocol.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <libweston/matrix.h>
#include "shared/weston-egl-ext.h"

/* Possible options that affect the displayed image */
#define OPT_IMMEDIATE     (1 << 0)  /* create wl_buffer immediately */
#define OPT_IMPLICIT_SYNC (1 << 1)  /* force implicit sync */
#define OPT_MANDELBROT    (1 << 2)  /* render mandelbrot */
#define OPT_DIRECT_DISPLAY     (1 << 3)  /* direct-display */

#define MAX_BUFFER_PLANES 4

struct display {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct xdg_wm_base *wm_base;
	struct zwp_fullscreen_shell_v1 *fshell;
	struct zwp_linux_dmabuf_v1 *dmabuf;
	struct weston_direct_display_v1 *direct_display;
	struct zwp_linux_explicit_synchronization_v1 *explicit_sync;
	uint32_t format;
	bool format_supported;
	uint64_t *modifiers;
	int modifiers_count;
	int req_dmabuf_immediate;
	bool use_explicit_sync;
	struct {
		EGLDisplay display;
		EGLContext context;
		EGLConfig conf;
		bool has_dma_buf_import_modifiers;
		bool has_no_config_context;
		PFNEGLQUERYDMABUFMODIFIERSEXTPROC query_dma_buf_modifiers;
		PFNEGLCREATEIMAGEKHRPROC create_image;
		PFNEGLDESTROYIMAGEKHRPROC destroy_image;
		PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target_texture_2d;
		PFNEGLCREATESYNCKHRPROC create_sync;
		PFNEGLDESTROYSYNCKHRPROC destroy_sync;
		PFNEGLCLIENTWAITSYNCKHRPROC client_wait_sync;
		PFNEGLDUPNATIVEFENCEFDANDROIDPROC dup_native_fence_fd;
		PFNEGLWAITSYNCKHRPROC wait_sync;
	} egl;
	struct {
		int drm_fd;
		struct gbm_device *device;
	} gbm;
};

struct buffer {
	struct display *display;
	struct wl_buffer *buffer;
	int busy;

	struct gbm_bo *bo;

	int width;
	int height;
	int format;
	uint64_t modifier;
	int plane_count;
	int dmabuf_fds[MAX_BUFFER_PLANES];
	uint32_t strides[MAX_BUFFER_PLANES];
	uint32_t offsets[MAX_BUFFER_PLANES];

	EGLImageKHR egl_image;
	GLuint gl_texture;
	GLuint gl_fbo;

	struct zwp_linux_buffer_release_v1 *buffer_release;
	/* The buffer owns the release_fence_fd, until it passes ownership
	 * to it to EGL (see wait_for_buffer_release_fence). */
	int release_fence_fd;
};

#define NUM_BUFFERS 4

struct window {
	struct display *display;
	int width, height;
	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	struct zwp_linux_surface_synchronization_v1 *surface_sync;
	struct buffer buffers[NUM_BUFFERS];
	struct wl_callback *callback;
	bool initialized;
	bool wait_for_configure;
	struct {
		GLuint program;
		GLuint pos;
		GLuint color;
		GLuint offset_uniform;
		GLuint reflection_uniform;
	} gl;
	bool render_mandelbrot;
};

static sig_atomic_t running = 1;

static void
redraw(void *data, struct wl_callback *callback, uint32_t time);

static void
buffer_release(void *data, struct wl_buffer *buffer)
{
	struct buffer *mybuf = data;

	mybuf->busy = 0;
}

static const struct wl_buffer_listener buffer_listener = {
	buffer_release
};

static void
buffer_free(struct buffer *buf)
{
	int i;

	if (buf->release_fence_fd >= 0)
		close(buf->release_fence_fd);

	if (buf->buffer_release)
		zwp_linux_buffer_release_v1_destroy(buf->buffer_release);

	if (buf->gl_fbo)
		glDeleteFramebuffers(1, &buf->gl_fbo);

	if (buf->gl_texture)
		glDeleteTextures(1, &buf->gl_texture);

	if (buf->egl_image) {
		buf->display->egl.destroy_image(buf->display->egl.display,
						buf->egl_image);
	}

	if (buf->buffer)
		wl_buffer_destroy(buf->buffer);

	if (buf->bo)
		gbm_bo_destroy(buf->bo);

	for (i = 0; i < buf->plane_count; ++i) {
		if (buf->dmabuf_fds[i] >= 0)
			close(buf->dmabuf_fds[i]);
	}
}

static void
create_succeeded(void *data,
		 struct zwp_linux_buffer_params_v1 *params,
		 struct wl_buffer *new_buffer)
{
	struct buffer *buffer = data;

	buffer->buffer = new_buffer;
	/* When not using explicit synchronization listen to wl_buffer.release
	 * for release notifications, otherwise we are going to use
	 * zwp_linux_buffer_release_v1. */
	if (!buffer->display->use_explicit_sync) {
		wl_buffer_add_listener(buffer->buffer, &buffer_listener,
				       buffer);
	}

	zwp_linux_buffer_params_v1_destroy(params);
}

static void
create_failed(void *data, struct zwp_linux_buffer_params_v1 *params)
{
	struct buffer *buffer = data;

	buffer->buffer = NULL;
	running = 0;

	zwp_linux_buffer_params_v1_destroy(params);

	fprintf(stderr, "Error: zwp_linux_buffer_params.create failed.\n");
}

static const struct zwp_linux_buffer_params_v1_listener params_listener = {
	create_succeeded,
	create_failed
};

static bool
create_fbo_for_buffer(struct display *display, struct buffer *buffer)
{
	static const int general_attribs = 3;
	static const int plane_attribs = 5;
	static const int entries_per_attrib = 2;
	EGLint attribs[(general_attribs + plane_attribs * MAX_BUFFER_PLANES) *
			entries_per_attrib + 1];
	unsigned int atti = 0;

	attribs[atti++] = EGL_WIDTH;
	attribs[atti++] = buffer->width;
	attribs[atti++] = EGL_HEIGHT;
	attribs[atti++] = buffer->height;
	attribs[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
	attribs[atti++] = buffer->format;

#define ADD_PLANE_ATTRIBS(plane_idx) { \
	attribs[atti++] = EGL_DMA_BUF_PLANE ## plane_idx ## _FD_EXT; \
	attribs[atti++] = buffer->dmabuf_fds[plane_idx]; \
	attribs[atti++] = EGL_DMA_BUF_PLANE ## plane_idx ## _OFFSET_EXT; \
	attribs[atti++] = (int) buffer->offsets[plane_idx]; \
	attribs[atti++] = EGL_DMA_BUF_PLANE ## plane_idx ## _PITCH_EXT; \
	attribs[atti++] = (int) buffer->strides[plane_idx]; \
	if (display->egl.has_dma_buf_import_modifiers) { \
		attribs[atti++] = EGL_DMA_BUF_PLANE ## plane_idx ## _MODIFIER_LO_EXT; \
		attribs[atti++] = buffer->modifier & 0xFFFFFFFF; \
		attribs[atti++] = EGL_DMA_BUF_PLANE ## plane_idx ## _MODIFIER_HI_EXT; \
		attribs[atti++] = buffer->modifier >> 32; \
	} \
	}

	if (buffer->plane_count > 0)
		ADD_PLANE_ATTRIBS(0);

	if (buffer->plane_count > 1)
		ADD_PLANE_ATTRIBS(1);

	if (buffer->plane_count > 2)
		ADD_PLANE_ATTRIBS(2);

	if (buffer->plane_count > 3)
		ADD_PLANE_ATTRIBS(3);

#undef ADD_PLANE_ATTRIBS

	attribs[atti] = EGL_NONE;

	assert(atti < ARRAY_LENGTH(attribs));

	buffer->egl_image = display->egl.create_image(display->egl.display,
						      EGL_NO_CONTEXT,
						      EGL_LINUX_DMA_BUF_EXT,
						      NULL, attribs);
	if (buffer->egl_image == EGL_NO_IMAGE_KHR) {
		fprintf(stderr, "EGLImageKHR creation failed\n");
		return false;
	}

	eglMakeCurrent(display->egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE,
			display->egl.context);

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
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fprintf(stderr, "FBO creation failed\n");
		return false;
	}

	return true;
}


static int
create_dmabuf_buffer(struct display *display, struct buffer *buffer,
		     int width, int height, uint32_t opts)
{
	static uint32_t flags = 0;
	struct zwp_linux_buffer_params_v1 *params;
	int i;

	buffer->display = display;
	buffer->width = width;
	buffer->height = height;
	buffer->format = display->format;
	buffer->release_fence_fd = -1;

#ifdef HAVE_GBM_MODIFIERS
	if (display->modifiers_count > 0) {
#ifdef HAVE_GBM_BO_CREATE_WITH_MODIFIERS2
		buffer->bo = gbm_bo_create_with_modifiers2(display->gbm.device,
							   buffer->width,
							   buffer->height,
							   buffer->format,
							   display->modifiers,
							   display->modifiers_count,
							   GBM_BO_USE_RENDERING);
#else
		buffer->bo = gbm_bo_create_with_modifiers(display->gbm.device,
							  buffer->width,
							  buffer->height,
							  buffer->format,
							  display->modifiers,
							  display->modifiers_count);
#endif
		if (buffer->bo)
			buffer->modifier = gbm_bo_get_modifier(buffer->bo);
	}
#endif

	if (!buffer->bo) {
		buffer->bo = gbm_bo_create(display->gbm.device,
					   buffer->width,
					   buffer->height,
					   buffer->format,
					   GBM_BO_USE_RENDERING);
		buffer->modifier = DRM_FORMAT_MOD_INVALID;
	}

	if (!buffer->bo) {
		fprintf(stderr, "create_bo failed\n");
		goto error;
	}

#ifdef HAVE_GBM_MODIFIERS
	buffer->plane_count = gbm_bo_get_plane_count(buffer->bo);
	for (i = 0; i < buffer->plane_count; ++i) {
		int ret;
		union gbm_bo_handle handle;

		handle = gbm_bo_get_handle_for_plane(buffer->bo, i);
		if (handle.s32 == -1) {
			fprintf(stderr, "error: failed to get gbm_bo_handle\n");
			goto error;
		}

		ret = drmPrimeHandleToFD(display->gbm.drm_fd, handle.u32, 0,
					 &buffer->dmabuf_fds[i]);
		if (ret < 0 || buffer->dmabuf_fds[i] < 0) {
			fprintf(stderr, "error: failed to get dmabuf_fd\n");
			goto error;
		}
		buffer->strides[i] = gbm_bo_get_stride_for_plane(buffer->bo, i);
		buffer->offsets[i] = gbm_bo_get_offset(buffer->bo, i);
	}
#else
	buffer->plane_count = 1;
	buffer->strides[0] = gbm_bo_get_stride(buffer->bo);
	buffer->dmabuf_fds[0] = gbm_bo_get_fd(buffer->bo);
	if (buffer->dmabuf_fds[0] < 0) {
		fprintf(stderr, "error: failed to get dmabuf_fd\n");
		goto error;
	}
#endif

	params = zwp_linux_dmabuf_v1_create_params(display->dmabuf);

	if ((opts & OPT_DIRECT_DISPLAY) && display->direct_display)
		weston_direct_display_v1_enable(display->direct_display, params);

	for (i = 0; i < buffer->plane_count; ++i) {
		zwp_linux_buffer_params_v1_add(params,
					       buffer->dmabuf_fds[i],
					       i,
					       buffer->offsets[i],
					       buffer->strides[i],
					       buffer->modifier >> 32,
					       buffer->modifier & 0xffffffff);
	}

	zwp_linux_buffer_params_v1_add_listener(params, &params_listener, buffer);
	if (display->req_dmabuf_immediate) {
		buffer->buffer =
			zwp_linux_buffer_params_v1_create_immed(params,
								buffer->width,
								buffer->height,
								buffer->format,
								flags);
		/* When not using explicit synchronization listen to
		 * wl_buffer.release for release notifications, otherwise we
		 * are going to use zwp_linux_buffer_release_v1. */
		if (!buffer->display->use_explicit_sync) {
			wl_buffer_add_listener(buffer->buffer,
					       &buffer_listener,
					       buffer);
		}
	}
	else {
		zwp_linux_buffer_params_v1_create(params,
						  buffer->width,
						  buffer->height,
						  buffer->format,
						  flags);
	}

	if (!create_fbo_for_buffer(display, buffer))
		goto error;

	return 0;

error:
	buffer_free(buffer);
	return -1;
}

static void
xdg_surface_handle_configure(void *data, struct xdg_surface *surface,
			     uint32_t serial)
{
	struct window *window = data;

	xdg_surface_ack_configure(surface, serial);

	if (window->initialized && window->wait_for_configure)
		redraw(window, NULL, 0);
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
}

static void
xdg_toplevel_handle_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	running = 0;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	xdg_toplevel_handle_configure,
	xdg_toplevel_handle_close,
};

static const char *vert_shader_text =
	"uniform float offset;\n"
	"uniform mat4 reflection;\n"
	"attribute vec4 pos;\n"
	"attribute vec4 color;\n"
	"varying vec4 v_color;\n"
	"void main() {\n"
	"  gl_Position = reflection * (pos + vec4(offset, offset, 0.0, 0.0));\n"
	"  v_color = color;\n"
	"}\n";

static const char *frag_shader_text =
	"precision mediump float;\n"
	"varying vec4 v_color;\n"
	"void main() {\n"
	"  gl_FragColor = v_color;\n"
	"}\n";

static const char *vert_shader_mandelbrot_text =
	"uniform float offset;\n"
	"uniform mat4 reflection;\n"
	"attribute vec4 pos;\n"
	"varying vec2 v_pos;\n"
	"void main() {\n"
	"  v_pos = pos.xy;\n"
	"  gl_Position = reflection * (pos + vec4(offset, offset, 0.0, 0.0));\n"
	"}\n";


/* Mandelbrot set shader using the escape time algorithm. */
static const char *frag_shader_mandelbrot_text =
	"precision mediump float;\n"
	"varying vec2 v_pos;\n"
	"void main() {\n"
	"  const int max_iteration = 500;\n"
	"  // Scale and translate position to get a nice mandelbrot drawing for\n"
	"  // the used v_pos x and y range (-0.5 to 0.5).\n"
	"  float x0 = 3.0 * v_pos.x - 0.5;\n"
	"  float y0 = 3.0 * v_pos.y;\n"
	"  float x = 0.0;\n"
	"  float y = 0.0;\n"
	"  int iteration = 0;\n"
	"  while (x * x + y * y <= 4.0 && iteration < max_iteration) {\n"
	"    float xtemp = x * x - y * y + x0;\n"
	"    y = 2.0 * x * y + y0;\n"
	"    x = xtemp;\n"
	"    ++iteration;\n"
	"  }\n"
	"  float red = iteration == max_iteration ?\n"
	"              0.0 : 1.0 - fract(float(iteration) / 20.0);\n"
	"  gl_FragColor = vec4(red, 0.0, 0.0, 1.0);\n"
	"}\n";

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
		fprintf(stderr, "Error: compiling %s: %.*s\n",
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
		fprintf(stderr, "Error: linking:\n%.*s\n", len, log);
		return 0;
	}

	return program;
}

static bool
window_set_up_gl(struct window *window)
{
	GLuint vert = create_shader(
		window->render_mandelbrot ? vert_shader_mandelbrot_text :
					    vert_shader_text,
		GL_VERTEX_SHADER);
	GLuint frag = create_shader(
		window->render_mandelbrot ? frag_shader_mandelbrot_text :
					    frag_shader_text,
		GL_FRAGMENT_SHADER);

	window->gl.program = create_and_link_program(vert, frag);

	glDeleteShader(vert);
	glDeleteShader(frag);

	window->gl.pos = glGetAttribLocation(window->gl.program, "pos");
	window->gl.color = glGetAttribLocation(window->gl.program, "color");

	glUseProgram(window->gl.program);

	window->gl.offset_uniform =
		glGetUniformLocation(window->gl.program, "offset");
	window->gl.reflection_uniform =
		glGetUniformLocation(window->gl.program, "reflection");

	return window->gl.program != 0;
}

static void
destroy_window(struct window *window)
{
	int i;

	if (window->gl.program)
		glDeleteProgram(window->gl.program);

	if (window->callback)
		wl_callback_destroy(window->callback);

	for (i = 0; i < NUM_BUFFERS; i++) {
		if (window->buffers[i].buffer)
			buffer_free(&window->buffers[i]);
	}

	if (window->xdg_toplevel)
		xdg_toplevel_destroy(window->xdg_toplevel);
	if (window->xdg_surface)
		xdg_surface_destroy(window->xdg_surface);
	if (window->surface_sync)
		zwp_linux_surface_synchronization_v1_destroy(window->surface_sync);
	wl_surface_destroy(window->surface);
	free(window);
}

static struct window *
create_window(struct display *display, int width, int height, int opts)
{
	struct window *window;
	int i;
	int ret;

	window = zalloc(sizeof *window);
	if (!window)
		return NULL;

	window->callback = NULL;
	window->display = display;
	window->width = width;
	window->height = height;
	window->surface = wl_compositor_create_surface(display->compositor);

	if (display->wm_base) {
		window->xdg_surface =
			xdg_wm_base_get_xdg_surface(display->wm_base,
						    window->surface);

		assert(window->xdg_surface);

		xdg_surface_add_listener(window->xdg_surface,
					 &xdg_surface_listener, window);

		window->xdg_toplevel =
			xdg_surface_get_toplevel(window->xdg_surface);

		assert(window->xdg_toplevel);

		xdg_toplevel_add_listener(window->xdg_toplevel,
					  &xdg_toplevel_listener, window);

		xdg_toplevel_set_title(window->xdg_toplevel, "simple-dmabuf-egl");
		xdg_toplevel_set_app_id(window->xdg_toplevel,
				"org.freedesktop.weston.simple-dmabuf-egl");

		window->wait_for_configure = true;
		wl_surface_commit(window->surface);
	} else if (display->fshell) {
		zwp_fullscreen_shell_v1_present_surface(display->fshell,
							window->surface,
							ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_DEFAULT,
							NULL);
	} else {
		assert(0);
	}

	if (display->explicit_sync) {
		window->surface_sync =
			zwp_linux_explicit_synchronization_v1_get_synchronization(
					display->explicit_sync, window->surface);
		assert(window->surface_sync);
	}

	for (i = 0; i < NUM_BUFFERS; ++i) {
		int j;
		for (j = 0; j < MAX_BUFFER_PLANES; ++j)
			window->buffers[i].dmabuf_fds[j] = -1;

	}

	for (i = 0; i < NUM_BUFFERS; ++i) {
		ret = create_dmabuf_buffer(display, &window->buffers[i],
		                           width, height, opts);

		if (ret < 0)
			goto error;
	}

	window->render_mandelbrot = opts & OPT_MANDELBROT;

	if (!window_set_up_gl(window))
		goto error;

	return window;

error:
	if (window)
		destroy_window(window);

	return NULL;
}

static int
create_egl_fence_fd(struct window *window)
{
	struct display *d = window->display;
	EGLSyncKHR sync = d->egl.create_sync(d->egl.display,
					     EGL_SYNC_NATIVE_FENCE_ANDROID,
					     NULL);
	int fd;

	assert(sync != EGL_NO_SYNC_KHR);
	/* We need to flush before we can get the fence fd. */
	glFlush();
	fd = d->egl.dup_native_fence_fd(d->egl.display, sync);
	assert(fd >= 0);

	d->egl.destroy_sync(d->egl.display, sync);

	return fd;
}

static struct buffer *
window_next_buffer(struct window *window)
{
	int i;

	for (i = 0; i < NUM_BUFFERS; i++)
		if (!window->buffers[i].busy)
			return &window->buffers[i];

	return NULL;
}

static const struct wl_callback_listener frame_listener;

/* Renders a square moving from the lower left corner to the
 * upper right corner of the window. The square's vertices have
 * the following colors:
 *
 *  green +-----+ yellow
 *        |     |
 *        |     |
 *    red +-----+ blue
 */
static void
render(struct window *window, struct buffer *buffer)
{
	/* Complete a movement iteration in 5000 ms. */
	static const uint64_t iteration_ms = 5000;
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
	GLfloat offset;
	struct timeval tv;
	uint64_t time_ms;
	struct weston_matrix reflection;

	gettimeofday(&tv, NULL);
	time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	/* Split time_ms in repeating windows of [0, iteration_ms) and map them
	 * to offsets in the [-0.5, 0.5) range. */
	offset = (time_ms % iteration_ms) / (float) iteration_ms - 0.5;

	weston_matrix_init(&reflection);
	/* perform a reflection about x-axis to keep the same orientation of
	 * the vertices colors,  as outlined in the comment at the beginning
	 * of this function.
	 *
	 * We need to render upside-down, because rendering through an FBO
	 * causes the bottom of the image to be written to the top pixel row of
	 * the buffer, y-flipping the image.
	 *
	 * Reflection is a specialized version of scaling with the
	 * following matrix:
	 *
	 * [1,  0,  0]
	 * [0, -1,  0]
	 * [0,  0,  1]
	 */
	weston_matrix_scale(&reflection, 1, -1, 1);

	/* Direct all GL draws to the buffer through the FBO */
	glBindFramebuffer(GL_FRAMEBUFFER, buffer->gl_fbo);

	glViewport(0, 0, window->width, window->height);

	glUniform1f(window->gl.offset_uniform, offset);
	glUniformMatrix4fv(window->gl.reflection_uniform, 1, GL_FALSE,
			   (GLfloat *) reflection.d);

	glClearColor(0.0,0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glVertexAttribPointer(window->gl.pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
	glVertexAttribPointer(window->gl.color, 3, GL_FLOAT, GL_FALSE, 0, colors);
	glEnableVertexAttribArray(window->gl.pos);
	glEnableVertexAttribArray(window->gl.color);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(window->gl.pos);
	glDisableVertexAttribArray(window->gl.color);
}

static void
render_mandelbrot(struct window *window, struct buffer *buffer)
{
	/* Complete a movement iteration in 5000 ms. */
	static const uint64_t iteration_ms = 5000;
	/* Split drawing in a square grid consisting of grid_side * grid_side
	 * cells. */
	static const int grid_side = 4;
	GLfloat norm_cell_side = 1.0 / grid_side;
	int num_cells = grid_side * grid_side;
	GLfloat offset;
	struct timeval tv;
	uint64_t time_ms;
	int i;
	struct weston_matrix reflection;

	gettimeofday(&tv, NULL);
	time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	/* Split time_ms in repeating windows of [0, iteration_ms) and map them
	 * to offsets in the [-0.5, 0.5) range. */
	offset = (time_ms % iteration_ms) / (float) iteration_ms - 0.5;

	weston_matrix_init(&reflection);
	weston_matrix_scale(&reflection, 1, -1, 1);

	/* Direct all GL draws to the buffer through the FBO */
	glBindFramebuffer(GL_FRAMEBUFFER, buffer->gl_fbo);

	glViewport(0, 0, window->width, window->height);

	glUniform1f(window->gl.offset_uniform, offset);
	glUniformMatrix4fv(window->gl.reflection_uniform, 1, GL_FALSE,
			   (GLfloat *) reflection.d);

	glClearColor(0.6, 0.6, 0.6, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	for (i = 0; i < num_cells; ++i) {
		/* Calculate the vertex coordinates of the current grid cell. */
		int row = i / grid_side;
		int col = i % grid_side;
		GLfloat left = -0.5 + norm_cell_side * col;
		GLfloat right = left + norm_cell_side;
		GLfloat top = 0.5 - norm_cell_side * row;
		GLfloat bottom = top - norm_cell_side;
		GLfloat verts[4][2] = {
			{ left,  bottom },
			{ left,  top },
			{ right, bottom },
			{ right, top }
		};

		/* ... and draw it. */
		glVertexAttribPointer(window->gl.pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
		glEnableVertexAttribArray(window->gl.pos);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDisableVertexAttribArray(window->gl.pos);
	}
}

static void
buffer_fenced_release(void *data,
		      struct zwp_linux_buffer_release_v1 *release,
                      int32_t fence)
{
	struct buffer *buffer = data;

	assert(release == buffer->buffer_release);
	assert(buffer->release_fence_fd == -1);

	buffer->busy = 0;
	buffer->release_fence_fd = fence;
	zwp_linux_buffer_release_v1_destroy(buffer->buffer_release);
	buffer->buffer_release = NULL;
}

static void
buffer_immediate_release(void *data,
			 struct zwp_linux_buffer_release_v1 *release)
{
	struct buffer *buffer = data;

	assert(release == buffer->buffer_release);
	assert(buffer->release_fence_fd == -1);

	buffer->busy = 0;
	zwp_linux_buffer_release_v1_destroy(buffer->buffer_release);
	buffer->buffer_release = NULL;
}

static const struct zwp_linux_buffer_release_v1_listener buffer_release_listener = {
       buffer_fenced_release,
       buffer_immediate_release,
};

static void
wait_for_buffer_release_fence(struct buffer *buffer)
{
	struct display *d = buffer->display;
	EGLint attrib_list[] = {
		EGL_SYNC_NATIVE_FENCE_FD_ANDROID, buffer->release_fence_fd,
		EGL_NONE,
	};
	EGLSyncKHR sync = d->egl.create_sync(d->egl.display,
					     EGL_SYNC_NATIVE_FENCE_ANDROID,
					     attrib_list);
	int ret;

	assert(sync);

	/* EGLSyncKHR takes ownership of the fence fd. */
	buffer->release_fence_fd = -1;

	if (d->egl.wait_sync)
		ret = d->egl.wait_sync(d->egl.display, sync, 0);
	else
		ret = d->egl.client_wait_sync(d->egl.display, sync, 0,
					      EGL_FOREVER_KHR);
	assert(ret == EGL_TRUE);

	ret = d->egl.destroy_sync(d->egl.display, sync);
	assert(ret == EGL_TRUE);
}

static void
redraw(void *data, struct wl_callback *callback, uint32_t time)
{
	struct window *window = data;
	struct buffer *buffer;

	buffer = window_next_buffer(window);
	if (!buffer) {
		fprintf(stderr,
			!callback ? "Failed to create the first buffer.\n" :
			"All buffers busy at redraw(). Server bug?\n");
		abort();
	}

	if (buffer->release_fence_fd >= 0)
		wait_for_buffer_release_fence(buffer);

	if (window->render_mandelbrot)
		render_mandelbrot(window, buffer);
	else
		render(window, buffer);

	if (window->display->use_explicit_sync) {
		int fence_fd = create_egl_fence_fd(window);
		zwp_linux_surface_synchronization_v1_set_acquire_fence(
			window->surface_sync, fence_fd);
		close(fence_fd);

		buffer->buffer_release =
			zwp_linux_surface_synchronization_v1_get_release(window->surface_sync);
		zwp_linux_buffer_release_v1_add_listener(
			buffer->buffer_release, &buffer_release_listener, buffer);
	} else {
		glFinish();
	}

	wl_surface_attach(window->surface, buffer->buffer, 0, 0);
	wl_surface_damage(window->surface, 0, 0, window->width, window->height);

	if (callback)
		wl_callback_destroy(callback);

	window->callback = wl_surface_frame(window->surface);
	wl_callback_add_listener(window->callback, &frame_listener, window);
	wl_surface_commit(window->surface);
	buffer->busy = 1;
}

static const struct wl_callback_listener frame_listener = {
	redraw
};

static void
dmabuf_modifiers(void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf,
		 uint32_t format, uint32_t modifier_hi, uint32_t modifier_lo)
{
	struct display *d = data;
	uint64_t modifier = u64_from_u32s(modifier_hi, modifier_lo);

	if (format != d->format) {
		return;
	}

	d->format_supported = true;

	if (modifier != DRM_FORMAT_MOD_INVALID) {
		++d->modifiers_count;
		d->modifiers = realloc(d->modifiers,
				       d->modifiers_count * sizeof(*d->modifiers));
		d->modifiers[d->modifiers_count - 1] = modifier;
	}
}

static void
dmabuf_format(void *data, struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf, uint32_t format)
{
	/* XXX: deprecated */
}

static const struct zwp_linux_dmabuf_v1_listener dmabuf_listener = {
	dmabuf_format,
	dmabuf_modifiers
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
		d->compositor =
			wl_registry_bind(registry,
					 id, &wl_compositor_interface, 1);
	} else if (strcmp(interface, "xdg_wm_base") == 0) {
		d->wm_base = wl_registry_bind(registry,
					      id, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(d->wm_base, &xdg_wm_base_listener, d);
	} else if (strcmp(interface, "zwp_fullscreen_shell_v1") == 0) {
		d->fshell = wl_registry_bind(registry,
					     id, &zwp_fullscreen_shell_v1_interface, 1);
	} else if (strcmp(interface, "zwp_linux_dmabuf_v1") == 0) {
		if (version < 3)
			return;
		d->dmabuf = wl_registry_bind(registry,
					     id, &zwp_linux_dmabuf_v1_interface, 3);
		zwp_linux_dmabuf_v1_add_listener(d->dmabuf, &dmabuf_listener, d);
	} else if (strcmp(interface, "zwp_linux_explicit_synchronization_v1") == 0) {
		d->explicit_sync = wl_registry_bind(
			registry, id,
			&zwp_linux_explicit_synchronization_v1_interface, 1);
	} else if (strcmp(interface, "weston_direct_display_v1") == 0) {
		d->direct_display = wl_registry_bind(registry,
						     id, &weston_direct_display_v1_interface, 1);
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
	if (display->gbm.device)
		gbm_device_destroy(display->gbm.device);

	if (display->gbm.drm_fd >= 0)
		close(display->gbm.drm_fd);

	if (display->egl.context != EGL_NO_CONTEXT)
		eglDestroyContext(display->egl.display, display->egl.context);

	if (display->egl.display != EGL_NO_DISPLAY)
		eglTerminate(display->egl.display);

	free(display->modifiers);

	if (display->dmabuf)
		zwp_linux_dmabuf_v1_destroy(display->dmabuf);

	if (display->wm_base)
		xdg_wm_base_destroy(display->wm_base);

	if (display->fshell)
		zwp_fullscreen_shell_v1_release(display->fshell);

	if (display->compositor)
		wl_compositor_destroy(display->compositor);

	if (display->registry)
		wl_registry_destroy(display->registry);

	if (display->display) {
		wl_display_flush(display->display);
		wl_display_disconnect(display->display);
	}

	free(display);
}

static bool
display_set_up_egl(struct display *display)
{
	static const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	EGLint major, minor, ret, count;
	const char *egl_extensions = NULL;
	const char *gl_extensions = NULL;

	EGLint config_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 1,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	display->egl.display =
		weston_platform_get_egl_display(EGL_PLATFORM_GBM_KHR,
						display->gbm.device, NULL);
	if (display->egl.display == EGL_NO_DISPLAY) {
		fprintf(stderr, "Failed to create EGLDisplay\n");
		goto error;
	}

	if (eglInitialize(display->egl.display, &major, &minor) == EGL_FALSE) {
		fprintf(stderr, "Failed to initialize EGLDisplay\n");
		goto error;
	}

	if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
		fprintf(stderr, "Failed to bind OpenGL ES API\n");
		goto error;
	}

	egl_extensions = eglQueryString(display->egl.display, EGL_EXTENSIONS);
	assert(egl_extensions != NULL);

	if (!weston_check_egl_extension(egl_extensions,
					"EGL_EXT_image_dma_buf_import")) {
		fprintf(stderr, "EGL_EXT_image_dma_buf_import not supported\n");
		goto error;
	}

	if (!weston_check_egl_extension(egl_extensions,
					"EGL_KHR_surfaceless_context")) {
		fprintf(stderr, "EGL_KHR_surfaceless_context not supported\n");
		goto error;
	}

	if (weston_check_egl_extension(egl_extensions,
					"EGL_KHR_no_config_context")) {
		display->egl.has_no_config_context = true;
	}

	if (display->egl.has_no_config_context) {
		display->egl.conf = EGL_NO_CONFIG_KHR;
	} else {
		fprintf(stderr,
			"Warning: EGL_KHR_no_config_context not supported\n");
		ret = eglChooseConfig(display->egl.display, config_attribs,
			      &display->egl.conf, 1, &count);
		assert(ret && count >= 1);
	}

	display->egl.context = eglCreateContext(display->egl.display,
						display->egl.conf,
						EGL_NO_CONTEXT,
						context_attribs);
	if (display->egl.context == EGL_NO_CONTEXT) {
		fprintf(stderr, "Failed to create EGLContext\n");
		goto error;
	}

	eglMakeCurrent(display->egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE,
		       display->egl.context);

	gl_extensions = (const char *) glGetString(GL_EXTENSIONS);
	assert(gl_extensions != NULL);

	if (!weston_check_egl_extension(gl_extensions,
					"GL_OES_EGL_image")) {
		fprintf(stderr, "GL_OES_EGL_image not supported\n");
		goto error;
	}

	if (weston_check_egl_extension(egl_extensions,
				       "EGL_EXT_image_dma_buf_import_modifiers")) {
		display->egl.has_dma_buf_import_modifiers = true;
		display->egl.query_dma_buf_modifiers =
			(void *) eglGetProcAddress("eglQueryDmaBufModifiersEXT");
		assert(display->egl.query_dma_buf_modifiers);
	}

	display->egl.create_image =
		(void *) eglGetProcAddress("eglCreateImageKHR");
	assert(display->egl.create_image);

	display->egl.destroy_image =
		(void *) eglGetProcAddress("eglDestroyImageKHR");
	assert(display->egl.destroy_image);

	display->egl.image_target_texture_2d =
		(void *) eglGetProcAddress("glEGLImageTargetTexture2DOES");
	assert(display->egl.image_target_texture_2d);

	if (weston_check_egl_extension(egl_extensions, "EGL_KHR_fence_sync") &&
	    weston_check_egl_extension(egl_extensions,
				       "EGL_ANDROID_native_fence_sync")) {
		display->egl.create_sync =
			(void *) eglGetProcAddress("eglCreateSyncKHR");
		assert(display->egl.create_sync);

		display->egl.destroy_sync =
			(void *) eglGetProcAddress("eglDestroySyncKHR");
		assert(display->egl.destroy_sync);

		display->egl.client_wait_sync =
			(void *) eglGetProcAddress("eglClientWaitSyncKHR");
		assert(display->egl.client_wait_sync);

		display->egl.dup_native_fence_fd =
			(void *) eglGetProcAddress("eglDupNativeFenceFDANDROID");
		assert(display->egl.dup_native_fence_fd);
	}

	if (weston_check_egl_extension(egl_extensions,
				       "EGL_KHR_wait_sync")) {
		display->egl.wait_sync =
			(void *) eglGetProcAddress("eglWaitSyncKHR");
		assert(display->egl.wait_sync);
	}

	return true;

error:
	return false;
}

static bool
display_update_supported_modifiers_for_egl(struct display *d)
{
	uint64_t *egl_modifiers = NULL;
	int num_egl_modifiers = 0;
	EGLBoolean ret;
	int i;
	bool try_modifiers = d->egl.has_dma_buf_import_modifiers;

	if (try_modifiers) {
		ret = d->egl.query_dma_buf_modifiers(d->egl.display,
						     d->format,
						     0,    /* max_modifiers */
						     NULL, /* modifiers */
						     NULL, /* external_only */
						     &num_egl_modifiers);
		if (ret == EGL_FALSE) {
			fprintf(stderr, "Failed to query num EGL modifiers for format\n");
			goto error;
		}
	}

	if (!num_egl_modifiers)
		try_modifiers = false;

	/* If EGL doesn't support modifiers, don't use them at all. */
	if (!try_modifiers) {
		d->modifiers_count = 0;
		free(d->modifiers);
		d->modifiers = NULL;
		return true;
	}

	egl_modifiers = zalloc(num_egl_modifiers * sizeof(*egl_modifiers));

	ret = d->egl.query_dma_buf_modifiers(d->egl.display,
					     d->format,
					     num_egl_modifiers,
					     egl_modifiers,
					     NULL, /* external_only */
					     &num_egl_modifiers);
	if (ret == EGL_FALSE) {
		fprintf(stderr, "Failed to query EGL modifiers for format\n");
		goto error;
	}

	/* Poor person's set intersection: d->modifiers INTERSECT
	 * egl_modifiers.  If a modifier is not supported, replace it with
	 * DRM_FORMAT_MOD_INVALID in the d->modifiers array.
	 */
	for (i = 0; i < d->modifiers_count; ++i) {
		uint64_t mod = d->modifiers[i];
		bool egl_supported = false;
		int j;

		for (j = 0; j < num_egl_modifiers; ++j) {
			if (egl_modifiers[j] == mod) {
				egl_supported = true;
				break;
			}
		}

		if (!egl_supported)
			d->modifiers[i] = DRM_FORMAT_MOD_INVALID;
	}

	free(egl_modifiers);

	return true;

error:
	free(egl_modifiers);

	return false;
}

static bool
display_set_up_gbm(struct display *display, char const* drm_render_node)
{
	display->gbm.drm_fd = open(drm_render_node, O_RDWR);
	if (display->gbm.drm_fd < 0) {
		fprintf(stderr, "Failed to open drm render node %s\n",
			drm_render_node);
		return false;
	}

	display->gbm.device = gbm_create_device(display->gbm.drm_fd);
	if (display->gbm.device == NULL) {
		fprintf(stderr, "Failed to create gbm device\n");
		return false;
	}

	return true;
}

static struct display *
create_display(char const *drm_render_node, uint32_t format, int opts)
{
	struct display *display = NULL;

	display = zalloc(sizeof *display);
	if (display == NULL) {
		fprintf(stderr, "out of memory\n");
		goto error;
	}

	display->gbm.drm_fd = -1;

	display->display = wl_display_connect(NULL);
	assert(display->display);

	display->format = format;
	display->req_dmabuf_immediate = opts & OPT_IMMEDIATE;

	display->registry = wl_display_get_registry(display->display);
	wl_registry_add_listener(display->registry,
				 &registry_listener, display);
	wl_display_roundtrip(display->display);
	if (display->dmabuf == NULL) {
		fprintf(stderr, "No zwp_linux_dmabuf global\n");
		goto error;
	}

	wl_display_roundtrip(display->display);

	if (!display->format_supported) {
		fprintf(stderr, "format 0x%"PRIX32" is not available\n",
			display->format);
		goto error;
	}

	/* GBM needs to be initialized before EGL, so that we have a valid
	 * render node gbm_device to create the EGL display from. */
	if (!display_set_up_gbm(display, drm_render_node))
		goto error;

	if (!display_set_up_egl(display))
		goto error;

	if (!display_update_supported_modifiers_for_egl(display))
		goto error;

	/* We use explicit synchronization only if the user hasn't disabled it,
	 * the compositor supports it, we can handle fence fds. */
	display->use_explicit_sync =
		!(opts & OPT_IMPLICIT_SYNC) &&
		display->explicit_sync &&
		display->egl.dup_native_fence_fd;

	if (opts & OPT_IMPLICIT_SYNC) {
		fprintf(stderr, "Warning: Not using explicit sync, disabled by user\n");
	} else if (!display->explicit_sync) {
		fprintf(stderr,
			"Warning: zwp_linux_explicit_synchronization_v1 not supported,\n"
			"         will not use explicit synchronization\n");
	} else if (!display->egl.dup_native_fence_fd) {
		fprintf(stderr,
			"Warning: EGL_ANDROID_native_fence_sync not supported,\n"
			"         will not use explicit synchronization\n");
	} else if (!display->egl.wait_sync) {
		fprintf(stderr,
			"Warning: EGL_KHR_wait_sync not supported,\n"
			"         will not use server-side wait\n");
	}

	return display;

error:
	if (display != NULL)
		destroy_display(display);
	return NULL;
}

static void
signal_int(int signum)
{
	running = 0;
}

static void
print_usage_and_exit(void)
{
	printf("usage flags:\n"
		"\t'-i,--import-immediate=<>'"
		"\n\t\t0 to import dmabuf via roundtrip, "
		"\n\t\t1 to enable import without roundtrip\n"
		"\t'-d,--drm-render-node=<>'"
		"\n\t\tthe full path to the drm render node to use\n"
		"\t'-s,--size=<>'"
		"\n\t\tthe window size in pixels (default: 256)\n"
		"\t'-e,--explicit-sync=<>'"
		"\n\t\t0 to disable explicit sync, "
		"\n\t\t1 to enable explicit sync (default: 1)\n"
		"\t'-f,--format=0x<>'"
		"\n\t\tthe DRM format code to use\n"
		"\t'-m,--mandelbrot'"
		"\n\t\trender a mandelbrot set with multiple draw calls\n"
		"\t'-g,--direct-display'"
		"\n\t\tenables weston-direct-display extension to attempt "
		"direct scan-out;\n\t\tnote this will cause the image to be "
		"displayed inverted as GL uses a\n\t\tdifferent texture "
		"coordinate system\n");
	exit(0);
}

static int
is_true(const char* c)
{
	if (!strcmp(c, "1"))
		return 1;
	else if (!strcmp(c, "0"))
		return 0;
	else
		print_usage_and_exit();

	return 0;
}

int
main(int argc, char **argv)
{
	struct sigaction sigint;
	struct display *display;
	struct window *window;
	uint32_t format = DRM_FORMAT_XRGB8888;
	int opts = 0;
	char const *drm_render_node = "/dev/dri/renderD128";
	int c, option_index, ret = 0;
	int window_size = 256;

	static struct option long_options[] = {
		{"import-immediate", required_argument, 0,  'i' },
		{"drm-render-node",  required_argument, 0,  'd' },
		{"size",	     required_argument, 0,  's' },
		{"explicit-sync",    required_argument, 0,  'e' },
		{"format",           required_argument, 0,  'f' },
		{"mandelbrot",       no_argument,	0,  'm' },
		{"direct-display",   no_argument,	0,  'g' },
		{"help",             no_argument      , 0,  'h' },
		{0, 0, 0, 0}
	};

	while ((c = getopt_long(argc, argv, "hi:d:s:e:f:mg",
				long_options, &option_index)) != -1) {
		switch (c) {
		case 'i':
			if (is_true(optarg))
				opts |= OPT_IMMEDIATE;
			break;
		case 'd':
			drm_render_node = optarg;
			break;
		case 's':
			window_size = strtol(optarg, NULL, 10);
			break;
		case 'e':
			if (!is_true(optarg))
				opts |= OPT_IMPLICIT_SYNC;
			break;
		case 'm':
			opts |= OPT_MANDELBROT;
			break;
		case 'g':
			opts |= OPT_DIRECT_DISPLAY;
			break;
		case 'f':
			format = strtoul(optarg, NULL, 0);
			break;
		default:
			print_usage_and_exit();
		}
	}

	display = create_display(drm_render_node, format, opts);
	if (!display)
		return 1;
	window = create_window(display, window_size, window_size, opts);
	if (!window)
		return 1;

	sigint.sa_handler = signal_int;
	sigemptyset(&sigint.sa_mask);
	sigint.sa_flags = SA_RESETHAND;
	sigaction(SIGINT, &sigint, NULL);

	/* Here we retrieve the linux-dmabuf objects if executed without immed,
	 * or error */
	wl_display_roundtrip(display->display);

	if (!running)
		return 1;

	window->initialized = true;

	if (!window->wait_for_configure)
		redraw(window, NULL, 0);

	while (running && ret != -1)
		ret = wl_display_dispatch(display->display);

	fprintf(stderr, "simple-dmabuf-egl exiting\n");
	destroy_window(window);
	destroy_display(display);

	return 0;
}
