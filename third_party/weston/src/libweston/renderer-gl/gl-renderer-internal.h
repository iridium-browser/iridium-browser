/*
 * Copyright © 2019 Collabora, Ltd.
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

#ifndef GL_RENDERER_INTERNAL_H
#define GL_RENDERER_INTERNAL_H

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "shared/weston-egl-ext.h"  /* for PFN* stuff */

struct gl_shader {
	GLuint program;
	GLuint vertex_shader, fragment_shader;
	GLint proj_uniform;
	GLint tex_uniforms[3];
	GLint alpha_uniform;
	GLint color_uniform;
	const char *vertex_source, *fragment_source;
};

struct gl_renderer {
	struct weston_renderer base;
	bool fragment_shader_debug;
	bool fan_debug;
	struct weston_binding *fragment_binding;
	struct weston_binding *fan_binding;

	EGLenum platform;
	EGLDisplay egl_display;
	EGLContext egl_context;
	EGLConfig egl_config;

	EGLSurface dummy_surface;

	uint32_t gl_version;

	struct wl_array vertices;
	struct wl_array vtxcnt;

	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target_texture_2d;
	PFNEGLCREATEIMAGEKHRPROC create_image;
	PFNEGLDESTROYIMAGEKHRPROC destroy_image;
	PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC swap_buffers_with_damage;

	PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display;
	PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC create_platform_window;
	bool has_platform_base;

	bool has_unpack_subimage;

	PFNEGLBINDWAYLANDDISPLAYWL bind_display;
	PFNEGLUNBINDWAYLANDDISPLAYWL unbind_display;
	PFNEGLQUERYWAYLANDBUFFERWL query_buffer;
	bool has_bind_display;

	bool has_context_priority;

	bool has_egl_image_external;

	bool has_egl_buffer_age;
	bool has_egl_partial_update;
	PFNEGLSETDAMAGEREGIONKHRPROC set_damage_region;

	bool has_configless_context;

	bool has_surfaceless_context;

	bool has_dmabuf_import;
	struct wl_list dmabuf_images;
	struct wl_list dmabuf_formats;

	bool has_gl_texture_rg;

	struct gl_shader texture_shader_rgba;
	struct gl_shader texture_shader_rgbx;
	struct gl_shader texture_shader_egl_external;
	struct gl_shader texture_shader_y_uv;
	struct gl_shader texture_shader_y_u_v;
	struct gl_shader texture_shader_y_xuxv;
	struct gl_shader texture_shader_xyuv;
	struct gl_shader invert_color_shader;
	struct gl_shader solid_shader;
	struct gl_shader *current_shader;

	struct wl_signal destroy_signal;

	struct wl_listener output_destroy_listener;

	bool has_dmabuf_import_modifiers;
	PFNEGLQUERYDMABUFFORMATSEXTPROC query_dmabuf_formats;
	PFNEGLQUERYDMABUFMODIFIERSEXTPROC query_dmabuf_modifiers;

	bool has_native_fence_sync;
	PFNEGLCREATESYNCKHRPROC create_sync;
	PFNEGLDESTROYSYNCKHRPROC destroy_sync;
	PFNEGLDUPNATIVEFENCEFDANDROIDPROC dup_native_fence_fd;

	bool has_wait_sync;
	PFNEGLWAITSYNCKHRPROC wait_sync;
};

static inline struct gl_renderer *
get_renderer(struct weston_compositor *ec)
{
	return (struct gl_renderer *)ec->renderer;
}

void
gl_renderer_print_egl_error_state(void);

void
gl_renderer_log_extensions(const char *name, const char *extensions);

void
log_egl_config_info(EGLDisplay egldpy, EGLConfig eglconfig);

EGLConfig
gl_renderer_get_egl_config(struct gl_renderer *gr,
			   EGLint egl_surface_type,
			   const uint32_t *drm_formats,
			   unsigned drm_formats_count);

int
gl_renderer_setup_egl_display(struct gl_renderer *gr, void *native_display);

int
gl_renderer_setup_egl_client_extensions(struct gl_renderer *gr);

int
gl_renderer_setup_egl_extensions(struct weston_compositor *ec);

#endif /* GL_RENDERER_INTERNAL_H */
