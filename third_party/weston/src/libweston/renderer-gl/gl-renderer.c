/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2015,2019,2021 Collabora, Ltd.
 * Copyright © 2016 NVIDIA Corporation
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

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <float.h>
#include <assert.h>
#include <linux/input.h>
#include <unistd.h>

#include "linux-sync-file.h"
#include "timeline.h"

#include "color.h"
#include "gl-renderer.h"
#include "gl-renderer-internal.h"
#include "vertex-clipping.h"
#include "linux-dmabuf.h"
#include "linux-dmabuf-unstable-v1-server-protocol.h"
#include "linux-explicit-synchronization.h"
#include "output-capture.h"
#include "pixel-formats.h"

#include "shared/fd-util.h"
#include "shared/helpers.h"
#include "shared/platform.h"
#include "shared/string-helpers.h"
#include "shared/timespec-util.h"
#include "shared/weston-drm-fourcc.h"
#include "shared/weston-egl-ext.h"
#include "shared/xalloc.h"

#define BUFFER_DAMAGE_COUNT 2

enum gl_border_status {
	BORDER_STATUS_CLEAN = 0,
	BORDER_TOP_DIRTY = 1 << GL_RENDERER_BORDER_TOP,
	BORDER_LEFT_DIRTY = 1 << GL_RENDERER_BORDER_LEFT,
	BORDER_RIGHT_DIRTY = 1 << GL_RENDERER_BORDER_RIGHT,
	BORDER_BOTTOM_DIRTY = 1 << GL_RENDERER_BORDER_BOTTOM,
	BORDER_ALL_DIRTY = 0xf,
	BORDER_SIZE_CHANGED = 0x10
};

struct gl_border_image {
	GLuint tex;
	int32_t width, height;
	int32_t tex_width;
	void *data;
};

struct gl_fbo_texture {
	GLuint fbo;
	GLuint tex;
	int32_t width;
	int32_t height;
};

struct gl_output_state {
	struct weston_size fb_size; /**< in pixels, including borders */
	struct weston_geometry area; /**< composited area in pixels inside fb */

	EGLSurface egl_surface;
	pixman_region32_t buffer_damage[BUFFER_DAMAGE_COUNT];
	int buffer_damage_index;
	enum gl_border_status border_damage[BUFFER_DAMAGE_COUNT];
	struct gl_border_image borders[4];
	enum gl_border_status border_status;
	bool swap_behavior_is_preserved;

	struct weston_matrix output_matrix;

	EGLSyncKHR render_sync;
	GLuint render_query;

	/* struct timeline_render_point::link */
	struct wl_list timeline_render_point_list;

	const struct pixel_format_info *shadow_format;
	struct gl_fbo_texture shadow;
};

struct gl_renderer;

struct dmabuf_format {
	uint32_t format;
	struct wl_list link;

	uint64_t *modifiers;
	unsigned *external_only;
	int num_modifiers;
};

/*
 * yuv_format_descriptor and yuv_plane_descriptor describe the translation
 * between YUV and RGB formats. When native YUV sampling is not available, we
 * bind each YUV plane as one or more RGB plane and convert in the shader.
 * This structure describes the mapping: output_planes is the number of
 * RGB images we need to bind, each of which has a yuv_plane_descriptor
 * describing the GL format and the input (YUV) plane index to bind.
 *
 * The specified shader_variant is then used to sample.
 */
struct yuv_plane_descriptor {
	uint32_t format;
	int plane_index;
};

struct yuv_format_descriptor {
	uint32_t format;
	int output_planes;
	enum gl_shader_texture_variant shader_variant;
	struct yuv_plane_descriptor plane[3];
};

struct gl_buffer_state {
	struct gl_renderer *gr;

	GLfloat color[4];

	bool needs_full_upload;
	pixman_region32_t texture_damage;

	/* Only needed between attach() and flush_damage() */
	int pitch; /* plane 0 pitch in pixels */
	GLenum gl_pixel_type;
	GLenum gl_format[3];
	int offset[3]; /* per-plane pitch in bytes */

	EGLImageKHR images[3];
	int num_images;
	enum gl_shader_texture_variant shader_variant;

	GLuint textures[3];
	int num_textures;

	struct wl_listener destroy_listener;
};

struct gl_surface_state {
	struct weston_surface *surface;

	struct gl_buffer_state *buffer;

	/* These buffer references should really be attached to paint nodes
	 * rather than either buffer or surface state */
	struct weston_buffer_reference buffer_ref;
	struct weston_buffer_release_reference buffer_release_ref;

	/* Whether this surface was used in the current output repaint.
	   Used only in the context of a gl_renderer_repaint_output call. */
	bool used_in_output_repaint;

	struct wl_listener surface_destroy_listener;
	struct wl_listener renderer_destroy_listener;
};

struct timeline_render_point {
	struct wl_list link; /* gl_output_state::timeline_render_point_list */

	int fd;
	GLuint query;
	struct weston_output *output;
	struct wl_event_source *event_source;
};

static uint32_t
gr_gl_version(uint16_t major, uint16_t minor)
{
	return ((uint32_t)major << 16) | minor;
}

static int
gr_gl_version_major(uint32_t ver)
{
	return ver >> 16;
}

static int
gr_gl_version_minor(uint32_t ver)
{
	return ver & 0xffff;
}

static inline const char *
dump_format(uint32_t format, char out[4])
{
#if BYTE_ORDER == BIG_ENDIAN
	format = __builtin_bswap32(format);
#endif
	memcpy(out, &format, 4);
	return out;
}

static inline struct gl_output_state *
get_output_state(struct weston_output *output)
{
	return (struct gl_output_state *)output->renderer_state;
}

static int
gl_renderer_create_surface(struct weston_surface *surface);

static inline struct gl_surface_state *
get_surface_state(struct weston_surface *surface)
{
	if (!surface->renderer_state)
		gl_renderer_create_surface(surface);

	return (struct gl_surface_state *)surface->renderer_state;
}

static bool
shadow_exists(const struct gl_output_state *go)
{
	return go->shadow.fbo != 0;
}

struct yuv_format_descriptor yuv_formats[] = {
	{
		.format = DRM_FORMAT_YUYV,
		.output_planes = 2,
		.shader_variant = SHADER_VARIANT_Y_XUXV,
		{{
			.format = DRM_FORMAT_GR88,
			.plane_index = 0
		}, {
			.format = DRM_FORMAT_ARGB8888,
			.plane_index = 0
		}}
	}, {
		.format = DRM_FORMAT_NV12,
		.output_planes = 2,
		.shader_variant = SHADER_VARIANT_Y_UV,
		{{
			.format = DRM_FORMAT_R8,
			.plane_index = 0
		}, {
			.format = DRM_FORMAT_GR88,
			.plane_index = 1
		}}
	}, {
		.format = DRM_FORMAT_YUV420,
		.output_planes = 3,
		.shader_variant = SHADER_VARIANT_Y_U_V,
		{{
			.format = DRM_FORMAT_R8,
			.plane_index = 0
		}, {
			.format = DRM_FORMAT_R8,
			.plane_index = 1
		}, {
			.format = DRM_FORMAT_R8,
			.plane_index = 2
		}}
	}, {
		.format = DRM_FORMAT_YUV444,
		.output_planes = 3,
		.shader_variant = SHADER_VARIANT_Y_U_V,
		{{
			.format = DRM_FORMAT_R8,
			.plane_index = 0
		}, {
			.format = DRM_FORMAT_R8,
			.plane_index = 1
		}, {
			.format = DRM_FORMAT_R8,
			.plane_index = 2
		}}
	}, {
		.format = DRM_FORMAT_XYUV8888,
		.output_planes = 1,
		.shader_variant = SHADER_VARIANT_XYUV,
		{{
			.format = DRM_FORMAT_XBGR8888,
			.plane_index = 0
		}}
	}
};

static void
timeline_begin_render_query(struct gl_renderer *gr, GLuint query)
{
	if (weston_log_scope_is_enabled(gr->compositor->timeline) &&
	    gr->has_native_fence_sync &&
	    gr->has_disjoint_timer_query)
		gr->begin_query(GL_TIME_ELAPSED_EXT, query);
}

static void
timeline_end_render_query(struct gl_renderer *gr)
{
	if (weston_log_scope_is_enabled(gr->compositor->timeline) &&
	    gr->has_native_fence_sync &&
	    gr->has_disjoint_timer_query)
		gr->end_query(GL_TIME_ELAPSED_EXT);
}

static void
timeline_render_point_destroy(struct timeline_render_point *trp)
{
	wl_list_remove(&trp->link);
	wl_event_source_remove(trp->event_source);
	close(trp->fd);
	free(trp);
}

static int
timeline_render_point_handler(int fd, uint32_t mask, void *data)
{
	struct timeline_render_point *trp = data;
	struct timespec end;

	if ((mask & WL_EVENT_READABLE) &&
	    (weston_linux_sync_file_read_timestamp(trp->fd, &end) == 0)) {
		struct gl_renderer *gr = get_renderer(trp->output->compositor);
		struct timespec begin;
		GLuint64 elapsed;
#if !defined(NDEBUG)
		GLint result_available;

		/* The elapsed time result must now be available since the
		 * begin/end queries are meant to be queued prior to fence sync
		 * creation. */
		gr->get_query_object_iv(trp->query,
					GL_QUERY_RESULT_AVAILABLE_EXT,
					&result_available);
		assert(result_available == GL_TRUE);
#endif

		gr->get_query_object_ui64v(trp->query, GL_QUERY_RESULT_EXT,
					   &elapsed);
		timespec_add_nsec(&begin, &end, -elapsed);

		TL_POINT(trp->output->compositor, "renderer_gpu_begin",
			 TLP_GPU(&begin), TLP_OUTPUT(trp->output), TLP_END);
		TL_POINT(trp->output->compositor, "renderer_gpu_end",
			 TLP_GPU(&end), TLP_OUTPUT(trp->output), TLP_END);
	}

	timeline_render_point_destroy(trp);

	return 0;
}

static EGLSyncKHR
create_render_sync(struct gl_renderer *gr)
{
	static const EGLint attribs[] = { EGL_NONE };

	if (!gr->has_native_fence_sync)
		return EGL_NO_SYNC_KHR;

	return gr->create_sync(gr->egl_display, EGL_SYNC_NATIVE_FENCE_ANDROID,
			       attribs);
}

static void
timeline_submit_render_sync(struct gl_renderer *gr,
			    struct weston_output *output,
			    EGLSyncKHR sync,
			    GLuint query)
{
	struct gl_output_state *go;
	struct wl_event_loop *loop;
	int fd;
	struct timeline_render_point *trp;

	if (!weston_log_scope_is_enabled(gr->compositor->timeline) ||
	    !gr->has_native_fence_sync ||
	    !gr->has_disjoint_timer_query ||
	    sync == EGL_NO_SYNC_KHR)
		return;

	go = get_output_state(output);
	loop = wl_display_get_event_loop(gr->compositor->wl_display);

	fd = gr->dup_native_fence_fd(gr->egl_display, sync);
	if (fd == EGL_NO_NATIVE_FENCE_FD_ANDROID)
		return;

	trp = zalloc(sizeof *trp);
	if (trp == NULL) {
		close(fd);
		return;
	}

	trp->fd = fd;
	trp->query = query;
	trp->output = output;
	trp->event_source = wl_event_loop_add_fd(loop, fd,
						 WL_EVENT_READABLE,
						 timeline_render_point_handler,
						 trp);

	wl_list_insert(&go->timeline_render_point_list, &trp->link);
}

/*
 * Compute the boundary vertices of the intersection of the global coordinate
 * aligned rectangle 'rect', and an arbitrary quadrilateral produced from
 * 'surf_rect' when transformed from surface coordinates into global coordinates.
 * The vertices are written to 'ex' and 'ey', and the return value is the
 * number of vertices. Vertices are produced in clockwise winding order.
 * Guarantees to produce either zero vertices, or 3-8 vertices with non-zero
 * polygon area.
 */
static int
calculate_edges(struct weston_view *ev, pixman_box32_t *rect,
		pixman_box32_t *surf_rect, struct weston_coord *e)
{

	struct clip_context ctx;
	int i, n;
	GLfloat min_x, max_x, min_y, max_y;
	struct weston_surface *es = ev->surface;
	struct weston_coord_surface tmp[4] = {
		weston_coord_surface(surf_rect->x1, surf_rect->y1, es),
		weston_coord_surface(surf_rect->x2, surf_rect->y1, es),
		weston_coord_surface(surf_rect->x2, surf_rect->y2, es),
		weston_coord_surface(surf_rect->x1, surf_rect->y2, es),
	};
	struct polygon8 surf;

	surf.n = 4;

	ctx.clip.x1 = rect->x1;
	ctx.clip.y1 = rect->y1;
	ctx.clip.x2 = rect->x2;
	ctx.clip.y2 = rect->y2;

	/* transform surface to screen space: */
	for (i = 0; i < surf.n; i++)
		surf.pos[i] = weston_coord_surface_to_global(ev, tmp[i]).c;

	/* find bounding box: */
	min_x = max_x = surf.pos[0].x;
	min_y = max_y = surf.pos[0].y;

	for (i = 1; i < surf.n; i++) {
		min_x = MIN(min_x, surf.pos[i].x);
		max_x = MAX(max_x, surf.pos[i].x);
		min_y = MIN(min_y, surf.pos[i].y);
		max_y = MAX(max_y, surf.pos[i].y);
	}

	/* First, simple bounding box check to discard early transformed
	 * surface rects that do not intersect with the clip region:
	 */
	if ((min_x >= ctx.clip.x2) || (max_x <= ctx.clip.x1) ||
	    (min_y >= ctx.clip.y2) || (max_y <= ctx.clip.y1))
		return 0;

	/* Simple case, bounding box edges are parallel to surface edges,
	 * there will be only four edges.  We just need to clip the surface
	 * vertices to the clip rect bounds:
	 */
	if (!ev->transform.enabled)
		return clip_simple(&ctx, &surf, e);

	/* Transformed case: use a general polygon clipping algorithm to
	 * clip the surface rectangle with each side of 'rect'.
	 * The algorithm is Sutherland-Hodgman, as explained in
	 * http://www.codeguru.com/cpp/misc/misc/graphics/article.php/c8965/Polygon-Clipping.htm
	 * but without looking at any of that code.
	 */
	n = clip_transformed(&ctx, &surf, e);

	if (n < 3)
		return 0;

	return n;
}

static bool
merge_down(pixman_box32_t *a, pixman_box32_t *b, pixman_box32_t *merge)
{
	if (a->x1 == b->x1 && a->x2 == b->x2 && a->y1 == b->y2) {
		merge->x1 = a->x1;
		merge->x2 = a->x2;
		merge->y1 = b->y1;
		merge->y2 = a->y2;
		return true;
	}
	return false;
}

static int
compress_bands(pixman_box32_t *inrects, int nrects, pixman_box32_t **outrects)
{
	bool merged = false;
	pixman_box32_t *out, merge_rect;
	int i, j, nout;

	if (!nrects) {
		*outrects = NULL;
		return 0;
	}

	/* nrects is an upper bound - we're not too worried about
	 * allocating a little extra
	 */
	out = malloc(sizeof(pixman_box32_t) * nrects);
	out[0] = inrects[0];
	nout = 1;
	for (i = 1; i < nrects; i++) {
		for (j = 0; j < nout; j++) {
			merged = merge_down(&inrects[i], &out[j], &merge_rect);
			if (merged) {
				out[j] = merge_rect;
				break;
			}
		}
		if (!merged) {
			out[nout] = inrects[i];
			nout++;
		}
	}
	*outrects = out;
	return nout;
}

static int
texture_region(struct weston_paint_node *pnode,
	       pixman_region32_t *region,
	       pixman_region32_t *surf_region)
{
	struct gl_surface_state *gs = get_surface_state(pnode->surface);
	struct weston_buffer *buffer = gs->buffer_ref.buffer;
	struct weston_compositor *ec = pnode->surface->compositor;
	struct weston_view *ev = pnode->view;
	struct gl_renderer *gr = get_renderer(ec);
	GLfloat *v, inv_width, inv_height;
	unsigned int *vtxcnt, nvtx = 0;
	pixman_box32_t *rects, *surf_rects;
	pixman_box32_t *raw_rects;
	int i, j, k, nrects, nsurf, raw_nrects;
	bool used_band_compression;
	raw_rects = pixman_region32_rectangles(region, &raw_nrects);
	surf_rects = pixman_region32_rectangles(surf_region, &nsurf);

	if (raw_nrects < 4) {
		used_band_compression = false;
		nrects = raw_nrects;
		rects = raw_rects;
	} else {
		nrects = compress_bands(raw_rects, raw_nrects, &rects);
		used_band_compression = true;
	}
	/* worst case we can have 8 vertices per rect (ie. clipped into
	 * an octagon):
	 */
	v = wl_array_add(&gr->vertices, nrects * nsurf * 8 * 4 * sizeof *v);
	vtxcnt = wl_array_add(&gr->vtxcnt, nrects * nsurf * sizeof *vtxcnt);

	inv_width = 1.0 / buffer->width;
	inv_height = 1.0 / buffer->height;

	for (i = 0; i < nrects; i++) {
		pixman_box32_t *rect = &rects[i];
		for (j = 0; j < nsurf; j++) {
			pixman_box32_t *surf_rect = &surf_rects[j];
			struct weston_coord e[8];      /* edge points in screen space */
			int n;

			/* The transformed surface, after clipping to the clip region,
			 * can have as many as eight sides, emitted as a triangle-fan.
			 * The first vertex in the triangle fan can be chosen arbitrarily,
			 * since the area is guaranteed to be convex.
			 *
			 * If a corner of the transformed surface falls outside of the
			 * clip region, instead of emitting one vertex for the corner
			 * of the surface, up to two are emitted for two corresponding
			 * intersection point(s) between the surface and the clip region.
			 *
			 * To do this, we first calculate the (up to eight) points that
			 * form the intersection of the clip rect and the transformed
			 * surface.
			 */
			n = calculate_edges(ev, rect, surf_rect, e);
			if (n < 3)
				continue;

			/* emit edge points: */
			for (k = 0; k < n; k++) {
				struct weston_coord_global pos_g;
				struct weston_coord_surface pos_s;
				struct weston_coord_buffer pos_b;

				pos_g.c = e[k];

				/* position: */
				*(v++) = pos_g.c.x;
				*(v++) = pos_g.c.y;

				/* texcoord: */
				pos_s = weston_coord_global_to_surface(ev, pos_g);
				pos_b = weston_coord_surface_to_buffer(ev->surface, pos_s);

				*(v++) = pos_b.c.x * inv_width;
				if (buffer->buffer_origin == ORIGIN_TOP_LEFT) {
					*(v++) = pos_b.c.y * inv_height;
				} else {
					*(v++) = (buffer->height - pos_b.c.y) * inv_height;
				}
			}

			vtxcnt[nvtx++] = n;
		}
	}

	if (used_band_compression)
		free(rects);
	return nvtx;
}

/** Create a texture and a framebuffer object
 *
 * \param fbotex To be initialized.
 * \param width Texture width in pixels.
 * \param height Texture heigh in pixels.
 * \param internal_format See glTexImage2D.
 * \param format See glTexImage2D.
 * \param type See glTexImage2D.
 * \return True on success, false otherwise.
 */
static bool
gl_fbo_texture_init(struct gl_fbo_texture *fbotex,
			 int32_t width,
			 int32_t height,
			 GLint internal_format,
			 GLenum format,
			 GLenum type)
{
	int fb_status;
	GLuint shadow_fbo;
	GLuint shadow_tex;

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &shadow_tex);
	glBindTexture(GL_TEXTURE_2D, shadow_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0,
		     format, type, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadow_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			       GL_TEXTURE_2D, shadow_tex, 0);

	fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
		glDeleteFramebuffers(1, &shadow_fbo);
		glDeleteTextures(1, &shadow_tex);
		return false;
	}

	fbotex->fbo = shadow_fbo;
	fbotex->tex = shadow_tex;
	fbotex->width = width;
	fbotex->height = height;

	return true;
}

static void
gl_fbo_texture_fini(struct gl_fbo_texture *fbotex)
{
	glDeleteFramebuffers(1, &fbotex->fbo);
	fbotex->fbo = 0;
	glDeleteTextures(1, &fbotex->tex);
	fbotex->tex = 0;
}

static bool
gl_renderer_do_capture(struct gl_renderer *gr, struct weston_buffer *into,
		       const struct weston_geometry *rect)
{
	struct wl_shm_buffer *shm = into->shm_buffer;
	const struct pixel_format_info *fmt = into->pixel_format;
	void *shm_pixels;
	void *read_target;
	int32_t stride;
	pixman_image_t *tmp = NULL;

	assert(fmt->gl_type != 0);
	assert(fmt->gl_format != 0);
	assert(into->type == WESTON_BUFFER_SHM);
	assert(shm);

	stride = wl_shm_buffer_get_stride(shm);
	if (stride % 4 != 0)
		return false;

	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	shm_pixels = wl_shm_buffer_get_data(shm);

	if (gr->has_pack_reverse) {
		/* Make glReadPixels() return top row first. */
		glPixelStorei(GL_PACK_REVERSE_ROW_ORDER_ANGLE, GL_TRUE);
		read_target = shm_pixels;
	} else {
		/*
		 * glReadPixels() returns bottom row first. We need to
		 * read into a temporary buffer and y-flip it.
		 */
		tmp = pixman_image_create_bits(fmt->pixman_format,
					       rect->width, rect->height,
					       NULL, 0);
		if (!tmp)
			return false;

		read_target = pixman_image_get_data(tmp);
	}

	wl_shm_buffer_begin_access(shm);

	glReadPixels(rect->x, rect->y, rect->width, rect->height,
		     fmt->gl_format, fmt->gl_type, read_target);

	if (tmp) {
		pixman_image_t *shm_image;
		pixman_transform_t flip;

		shm_image = pixman_image_create_bits_no_clear(fmt->pixman_format,
							      rect->width,
							      rect->height,
							      shm_pixels,
							      stride);
		abort_oom_if_null(shm_image);

		pixman_transform_init_scale(&flip, pixman_fixed_1,
					    pixman_fixed_minus_1);
		pixman_transform_translate(&flip, NULL,	0,
					   pixman_int_to_fixed(rect->height));
		pixman_image_set_transform(tmp, &flip);

		pixman_image_composite32(PIXMAN_OP_SRC,
					 tmp,       /* src */
					 NULL,      /* mask */
					 shm_image, /* dest */
					 0, 0,      /* src x,y */
					 0, 0,      /* mask x,y */
					 0, 0,      /* dest x,y */
					 rect->width, rect->height);

		pixman_image_unref(shm_image);
		pixman_image_unref(tmp);
	}

	wl_shm_buffer_end_access(shm);

	return true;
}

static void
gl_renderer_do_capture_tasks(struct gl_renderer *gr,
			     struct weston_output *output,
			     enum weston_output_capture_source source)
{
	struct gl_output_state *go = get_output_state(output);
	const struct pixel_format_info *format;
	struct weston_capture_task *ct;
	struct weston_geometry rect;

	switch (source) {
	case WESTON_OUTPUT_CAPTURE_SOURCE_FRAMEBUFFER:
		format = output->compositor->read_format;
		rect = go->area;
		/* Because glReadPixels has bottom-left origin */
		rect.y = go->fb_size.height - go->area.y - go->area.height;
		break;
	case WESTON_OUTPUT_CAPTURE_SOURCE_FULL_FRAMEBUFFER:
		format = output->compositor->read_format;
		rect.x = 0;
		rect.y = 0;
		rect.width = go->fb_size.width;
		rect.height = go->fb_size.height;
		break;
	default:
		assert(0);
		return;
	}

	while ((ct = weston_output_pull_capture_task(output, source, rect.width,
						     rect.height, format))) {
		struct weston_buffer *buffer = weston_capture_task_get_buffer(ct);

		assert(buffer->width == rect.width);
		assert(buffer->height == rect.height);
		assert(buffer->pixel_format->format == format->format);

		if (buffer->type != WESTON_BUFFER_SHM ||
		    buffer->buffer_origin != ORIGIN_TOP_LEFT) {
			weston_capture_task_retire_failed(ct, "GL: unsupported buffer");
			continue;
		}

		if (gl_renderer_do_capture(gr, buffer, &rect))
			weston_capture_task_retire_complete(ct);
		else
			weston_capture_task_retire_failed(ct, "GL: capture failed");
	}
}

static void
gl_renderer_send_shader_error(struct weston_paint_node *pnode)
{
	struct wl_resource *resource = pnode->surface->resource;

	if (!resource)
		return;

	wl_client_post_implementation_error(wl_resource_get_client(resource),
		"Weston GL-renderer shader failed for wl_surface@%u",
		wl_resource_get_id(resource));
}

static void
triangle_fan_debug(struct gl_renderer *gr,
		   const struct gl_shader_config *sconf,
		   struct weston_output *output,
		   int first, int count)
{
	int i;
	/* There can be at most eight vertices for a given view. */
	GLushort buffer[(8 - 1 + 8 - 2) * 2];
	GLushort *index;
	GLsizei nelems;
	static int color_idx = 0;
	struct gl_shader_config alt;
	const GLfloat *col;
	struct weston_color_transform *ctransf;
	static const GLfloat color[][4] = {
			{ 1.0, 0.0, 0.0, 1.0 },
			{ 0.0, 1.0, 0.0, 1.0 },
			{ 0.0, 0.0, 1.0, 1.0 },
			{ 1.0, 1.0, 1.0, 1.0 },
	};

	col = color[color_idx++ % ARRAY_LENGTH(color)];
	alt = (struct gl_shader_config) {
		.req = {
			.variant = SHADER_VARIANT_SOLID,
			.input_is_premult = true,
		},
		.projection = sconf->projection,
		.view_alpha = 1.0f,
		.unicolor = { col[0], col[1], col[2], col[3] },
	};

	ctransf = output->color_outcome->from_sRGB_to_blend;
	if (!gl_shader_config_set_color_transform(&alt, ctransf)) {
		weston_log("GL-renderer: %s failed to generate a color transformation.\n",
			   __func__);
		return;
	}

	gl_renderer_use_program(gr, &alt);

	nelems = (count - 1 + count - 2) * 2;
	assert((unsigned long)nelems <= ARRAY_LENGTH(buffer));

	index = buffer;

	for (i = 1; i < count; i++) {
		*index++ = first;
		*index++ = first + i;
	}

	for (i = 2; i < count; i++) {
		*index++ = first + i - 1;
		*index++ = first + i;
	}

	glDrawElements(GL_LINES, nelems, GL_UNSIGNED_SHORT, buffer);

	gl_renderer_use_program(gr, sconf);
}

static void
repaint_region(struct gl_renderer *gr,
	       struct weston_paint_node *pnode,
	       pixman_region32_t *region,
	       pixman_region32_t *surf_region,
	       const struct gl_shader_config *sconf)
{
	struct weston_output *output = pnode->output;
	GLfloat *v;
	unsigned int *vtxcnt;
	int i, first, nfans;

	/* The final region to be painted is the intersection of
	 * 'region' and 'surf_region'. However, 'region' is in the global
	 * coordinates, and 'surf_region' is in the surface-local
	 * coordinates. texture_region() will iterate over all pairs of
	 * rectangles from both regions, compute the intersection
	 * polygon for each pair, and store it as a triangle fan if
	 * it has a non-zero area (at least 3 vertices, actually).
	 */
	nfans = texture_region(pnode, region, surf_region);

	v = gr->vertices.data;
	vtxcnt = gr->vtxcnt.data;

	/* position: */
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof *v, &v[0]);
	/* texcoord: */
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof *v, &v[2]);

	if (!gl_renderer_use_program(gr, sconf)) {
		gl_renderer_send_shader_error(pnode);
		/* continue drawing with the fallback shader */
	}

	for (i = 0, first = 0; i < nfans; i++) {
		glDrawArrays(GL_TRIANGLE_FAN, first, vtxcnt[i]);
		if (gr->fan_debug)
			triangle_fan_debug(gr, sconf, output, first, vtxcnt[i]);
		first += vtxcnt[i];
	}

	gr->vertices.size = 0;
	gr->vtxcnt.size = 0;
}

static int
use_output(struct weston_output *output)
{
	static int errored;
	struct gl_output_state *go = get_output_state(output);
	struct gl_renderer *gr = get_renderer(output->compositor);
	EGLBoolean ret;

	ret = eglMakeCurrent(gr->egl_display, go->egl_surface,
			     go->egl_surface, gr->egl_context);

	if (ret == EGL_FALSE) {
		if (errored)
			return -1;
		errored = 1;
		weston_log("Failed to make EGL context current.\n");
		gl_renderer_print_egl_error_state();
		return -1;
	}

	return 0;
}

static int
ensure_surface_buffer_is_ready(struct gl_renderer *gr,
			       struct gl_surface_state *gs)
{
	EGLint attribs[] = {
		EGL_SYNC_NATIVE_FENCE_FD_ANDROID,
		-1,
		EGL_NONE
	};
	struct weston_surface *surface = gs->surface;
	struct weston_buffer *buffer = gs->buffer_ref.buffer;
	EGLSyncKHR sync;
	EGLint wait_ret;
	EGLint destroy_ret;

	if (!buffer)
		return 0;

	if (surface->acquire_fence_fd < 0)
		return 0;

	/* We should only get a fence if we support EGLSyncKHR, since
	 * we don't advertise the explicit sync protocol otherwise. */
	assert(gr->has_native_fence_sync);
	/* We should only get a fence for non-SHM buffers, since surface
	 * commit would have failed otherwise. */
	assert(buffer->type != WESTON_BUFFER_SHM);

	attribs[1] = dup(surface->acquire_fence_fd);
	if (attribs[1] == -1) {
		linux_explicit_synchronization_send_server_error(
			gs->surface->synchronization_resource,
			"Failed to dup acquire fence");
		return -1;
	}

	sync = gr->create_sync(gr->egl_display,
			       EGL_SYNC_NATIVE_FENCE_ANDROID,
			       attribs);
	if (sync == EGL_NO_SYNC_KHR) {
		linux_explicit_synchronization_send_server_error(
			gs->surface->synchronization_resource,
			"Failed to create EGLSyncKHR object");
		close(attribs[1]);
		return -1;
	}

	wait_ret = gr->wait_sync(gr->egl_display, sync, 0);
	if (wait_ret == EGL_FALSE) {
		linux_explicit_synchronization_send_server_error(
			gs->surface->synchronization_resource,
			"Failed to wait on EGLSyncKHR object");
		/* Continue to try to destroy the sync object. */
	}


	destroy_ret = gr->destroy_sync(gr->egl_display, sync);
	if (destroy_ret == EGL_FALSE) {
		linux_explicit_synchronization_send_server_error(
			gs->surface->synchronization_resource,
			"Failed to destroy on EGLSyncKHR object");
	}

	return (wait_ret == EGL_TRUE && destroy_ret == EGL_TRUE) ? 0 : -1;
}

static void
censor_override(struct gl_shader_config *sconf,
		struct weston_output *output)
{
	struct weston_color_transform *ctransf;
	struct gl_shader_config alt = {
		.req = {
			.variant = SHADER_VARIANT_SOLID,
			.input_is_premult = true,
		},
		.projection = sconf->projection,
		.view_alpha = sconf->view_alpha,
		.unicolor = { 0.40, 0.0, 0.0, 1.0 },
	};

	ctransf = output->color_outcome->from_sRGB_to_blend;
	if (!gl_shader_config_set_color_transform(&alt, ctransf)) {
		weston_log("GL-renderer: %s failed to generate a color transformation.\n",
			   __func__);
	}

	*sconf = alt;
}

 /* Checks if a view needs to be censored on an output
  * Checks for 2 types of censor requirements
  * - recording_censor: Censor protected view when a
  *   protected view is captured.
  * - unprotected_censor: Censor regions of protected views
  *   when displayed on an output which has lower protection capability.
  * If censoring is needed, smashes the GL shader config.
  */
static void
maybe_censor_override(struct gl_shader_config *sconf,
		      struct weston_paint_node *pnode)
{
	struct weston_output *output = pnode->output;
	struct weston_surface *surface = pnode->surface;
	struct gl_surface_state *gs = get_surface_state(surface);
	struct weston_buffer *buffer = gs->buffer_ref.buffer;
	bool recording_censor =
		(output->disable_planes > 0) &&
		(surface->desired_protection > WESTON_HDCP_DISABLE);

	bool unprotected_censor =
		(surface->desired_protection > output->current_protection);

	if (buffer->direct_display) {
		censor_override(sconf, output);
		return;
	}

	/* When not in enforced mode, the client is notified of the protection */
	/* change, so content censoring is not required */
	if (surface->protection_mode !=
	    WESTON_SURFACE_PROTECTION_MODE_ENFORCED)
		return;

	if (recording_censor || unprotected_censor)
		censor_override(sconf, output);
}

static void
gl_shader_config_set_input_textures(struct gl_shader_config *sconf,
				    struct gl_surface_state *gs)
{
	struct gl_buffer_state *gb = gs->buffer;
	int i;

	sconf->req.variant = gb->shader_variant;
	sconf->req.input_is_premult =
		gl_shader_texture_variant_can_be_premult(gb->shader_variant);

	for (i = 0; i < 4; i++)
		sconf->unicolor[i] = gb->color[i];

	assert(gb->num_textures <= GL_SHADER_INPUT_TEX_MAX);
	for (i = 0; i < gb->num_textures; i++)
		sconf->input_tex[i] = gb->textures[i];
	for (; i < GL_SHADER_INPUT_TEX_MAX; i++)
		sconf->input_tex[i] = 0;
}

static bool
gl_shader_config_init_for_paint_node(struct gl_shader_config *sconf,
				     struct weston_paint_node *pnode,
				     GLint filter)
{
	struct gl_surface_state *gs = get_surface_state(pnode->surface);
	struct gl_output_state *go = get_output_state(pnode->output);

	if (!pnode->surf_xform_valid)
		return false;

	*sconf = (struct gl_shader_config) {
		.projection = go->output_matrix,
		.view_alpha = pnode->view->alpha,
		.input_tex_filter = filter,
	};

	gl_shader_config_set_input_textures(sconf, gs);

	if (!gl_shader_config_set_color_transform(sconf, pnode->surf_xform.transform)) {
		weston_log("GL-renderer: failed to generate a color transformation.\n");
		return false;
	}

	return true;
}

static void
draw_paint_node(struct weston_paint_node *pnode,
		pixman_region32_t *damage /* in global coordinates */)
{
	struct gl_renderer *gr = get_renderer(pnode->surface->compositor);
	struct gl_surface_state *gs = get_surface_state(pnode->surface);
	struct gl_buffer_state *gb = gs->buffer;
	struct weston_buffer *buffer = gs->buffer_ref.buffer;
	/* repaint bounding region in global coordinates: */
	pixman_region32_t repaint;
	/* opaque region in surface coordinates: */
	pixman_region32_t surface_opaque;
	/* non-opaque region in surface coordinates: */
	pixman_region32_t surface_blend;
	GLint filter;
	struct gl_shader_config sconf;

	if (gb->shader_variant == SHADER_VARIANT_NONE &&
	    !buffer->direct_display)
		return;

	pixman_region32_init(&repaint);
	pixman_region32_intersect(&repaint,
				  &pnode->view->transform.boundingbox, damage);
	pixman_region32_subtract(&repaint, &repaint, &pnode->view->clip);

	if (!pixman_region32_not_empty(&repaint))
		goto out;

	if (ensure_surface_buffer_is_ready(gr, gs) < 0)
		goto out;

	if (pnode->needs_filtering)
		filter = GL_LINEAR;
	else
		filter = GL_NEAREST;

	if (!gl_shader_config_init_for_paint_node(&sconf, pnode, filter))
		goto out;

	/* blended region is whole surface minus opaque region: */
	pixman_region32_init_rect(&surface_blend, 0, 0,
				  pnode->surface->width, pnode->surface->height);
	if (pnode->view->geometry.scissor_enabled)
		pixman_region32_intersect(&surface_blend, &surface_blend,
					  &pnode->view->geometry.scissor);
	pixman_region32_subtract(&surface_blend, &surface_blend,
				 &pnode->surface->opaque);

	/* XXX: Should we be using ev->transform.opaque here? */
	pixman_region32_init(&surface_opaque);
	if (pnode->view->geometry.scissor_enabled)
		pixman_region32_intersect(&surface_opaque,
					  &pnode->surface->opaque,
					  &pnode->view->geometry.scissor);
	else
		pixman_region32_copy(&surface_opaque, &pnode->surface->opaque);

	maybe_censor_override(&sconf, pnode);

	if (pixman_region32_not_empty(&surface_opaque)) {
		struct gl_shader_config alt = sconf;

		if (alt.req.variant == SHADER_VARIANT_RGBA) {
			/* Special case for RGBA textures with possibly
			 * bad data in alpha channel: use the shader
			 * that forces texture alpha = 1.0.
			 * Xwayland surfaces need this.
			 */
			alt.req.variant = SHADER_VARIANT_RGBX;
		}

		if (pnode->view->alpha < 1.0)
			glEnable(GL_BLEND);
		else
			glDisable(GL_BLEND);

		repaint_region(gr, pnode, &repaint, &surface_opaque, &alt);
		gs->used_in_output_repaint = true;
	}

	if (pixman_region32_not_empty(&surface_blend)) {
		glEnable(GL_BLEND);
		repaint_region(gr, pnode, &repaint, &surface_blend, &sconf);
		gs->used_in_output_repaint = true;
	}

	pixman_region32_fini(&surface_blend);
	pixman_region32_fini(&surface_opaque);

out:
	pixman_region32_fini(&repaint);
}

static void
repaint_views(struct weston_output *output, pixman_region32_t *damage)
{
	struct weston_compositor *compositor = output->compositor;
	struct weston_paint_node *pnode;

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	wl_list_for_each_reverse(pnode, &output->paint_node_z_order_list,
				 z_order_link) {
		if (pnode->view->plane == &compositor->primary_plane)
			draw_paint_node(pnode, damage);
	}

	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
}

static int
gl_renderer_create_fence_fd(struct weston_output *output);

/* Updates the release fences of surfaces that were used in the current output
 * repaint. Should only be used from gl_renderer_repaint_output, so that the
 * information in gl_surface_state.used_in_output_repaint is accurate.
 */
static void
update_buffer_release_fences(struct weston_compositor *compositor,
			     struct weston_output *output)
{
	struct weston_paint_node *pnode;

	wl_list_for_each_reverse(pnode, &output->paint_node_z_order_list,
				 z_order_link) {
		struct weston_view *view = pnode->view;
		struct gl_surface_state *gs;
		struct weston_buffer_release *buffer_release;
		int fence_fd;

		if (view->plane != &compositor->primary_plane)
			continue;

		gs = get_surface_state(view->surface);
		buffer_release = gs->buffer_release_ref.buffer_release;

		if (!gs->used_in_output_repaint || !buffer_release)
			continue;

		fence_fd = gl_renderer_create_fence_fd(output);

		/* If we have a buffer_release then it means we support fences,
		 * and we should be able to create the release fence. If we
		 * can't, something has gone horribly wrong, so disconnect the
		 * client.
		 */
		if (fence_fd == -1) {
			linux_explicit_synchronization_send_server_error(
				buffer_release->resource,
				"Failed to create release fence");
			fd_clear(&buffer_release->fence_fd);
			continue;
		}

		/* At the moment it is safe to just replace the fence_fd,
		 * discarding the previous one:
		 *
		 * 1. If the previous fence fd represents a sync fence from
		 *    a previous repaint cycle, that fence fd is now not
		 *    sufficient to provide the release guarantee and should
		 *    be replaced.
		 *
		 * 2. If the fence fd represents a sync fence from another
		 *    output in the same repaint cycle, it's fine to replace
		 *    it since we are rendering to all outputs using the same
		 *    EGL context, so a fence issued for a later output rendering
		 *    is guaranteed to signal after fences for previous output
		 *    renderings.
		 *
		 * Note that the above is only valid if the buffer_release
		 * fences only originate from the GL renderer, which guarantees
		 * a total order of operations and fences.  If we introduce
		 * fences from other sources (e.g., plane out-fences), we will
		 * need to merge fences instead.
		 */
		fd_update(&buffer_release->fence_fd, fence_fd);
	}
}

static void
draw_output_border_texture(struct gl_renderer *gr,
			   struct gl_output_state *go,
			   struct gl_shader_config *sconf,
			   enum gl_renderer_border_side side,
			   int32_t x, int32_t y,
			   int32_t width, int32_t height)
{
	struct gl_border_image *img = &go->borders[side];
	static GLushort indices [] = { 0, 1, 3, 3, 1, 2 };

	if (!img->data) {
		if (img->tex) {
			glDeleteTextures(1, &img->tex);
			img->tex = 0;
		}

		return;
	}

	if (!img->tex) {
		glGenTextures(1, &img->tex);
		glBindTexture(GL_TEXTURE_2D, img->tex);

		glTexParameteri(GL_TEXTURE_2D,
				GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,
				GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else {
		glBindTexture(GL_TEXTURE_2D, img->tex);
	}

	if (go->border_status & (1 << side)) {
		glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, 0);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT,
			     img->tex_width, img->height, 0,
			     GL_BGRA_EXT, GL_UNSIGNED_BYTE, img->data);
	}

	sconf->input_tex_filter = GL_NEAREST;
	sconf->input_tex[0] = img->tex;
	gl_renderer_use_program(gr, sconf);

	GLfloat texcoord[] = {
		0.0f, 0.0f,
		(GLfloat)img->width / (GLfloat)img->tex_width, 0.0f,
		(GLfloat)img->width / (GLfloat)img->tex_width, 1.0f,
		0.0f, 1.0f,
	};

	GLfloat verts[] = {
		x, y,
		x + width, y,
		x + width, y + height,
		x, y + height
	};

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texcoord);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

static int
output_has_borders(struct weston_output *output)
{
	struct gl_output_state *go = get_output_state(output);

	return go->borders[GL_RENDERER_BORDER_TOP].data ||
	       go->borders[GL_RENDERER_BORDER_RIGHT].data ||
	       go->borders[GL_RENDERER_BORDER_BOTTOM].data ||
	       go->borders[GL_RENDERER_BORDER_LEFT].data;
}

static struct weston_geometry
output_get_border_area(const struct gl_output_state *go,
		       enum gl_renderer_border_side side)
{
	const struct weston_size *fb = &go->fb_size;
	const struct weston_geometry *area = &go->area;

	switch (side) {
	case GL_RENDERER_BORDER_TOP:
		return (struct weston_geometry){
			.x = 0,
			.y = 0,
			.width = fb->width,
			.height = area->y
		};
	case GL_RENDERER_BORDER_LEFT:
		return (struct weston_geometry){
			.x = 0,
			.y = area->y,
			.width = area->x,
			.height = area->height
		};
	case GL_RENDERER_BORDER_RIGHT:
		return (struct weston_geometry){
			.x = area->x + area->width,
			.y = area->y,
			.width = fb->width - area->x - area->width,
			.height = area->height
		};
	case GL_RENDERER_BORDER_BOTTOM:
		return (struct weston_geometry){
			.x = 0,
			.y = area->y + area->height,
			.width = fb->width,
			.height = fb->height - area->y - area->height
		};
	}

	assert(0);
	return (struct weston_geometry){};
}

static void
draw_output_borders(struct weston_output *output,
		    enum gl_border_status border_status)
{
	struct gl_shader_config sconf = {
		.req = {
			.variant = SHADER_VARIANT_RGBA,
			.input_is_premult = true,
		},
		.view_alpha = 1.0f,
	};
	struct weston_color_transform *ctransf;
	struct gl_output_state *go = get_output_state(output);
	struct gl_renderer *gr = get_renderer(output->compositor);
	const struct weston_size *fb = &go->fb_size;
	unsigned side;

	if (border_status == BORDER_STATUS_CLEAN)
		return; /* Clean. Nothing to do. */

	ctransf = output->color_outcome->from_sRGB_to_output;
	if (!gl_shader_config_set_color_transform(&sconf, ctransf)) {
		weston_log("GL-renderer: %s failed to generate a color transformation.\n", __func__);
		return;
	}

	glDisable(GL_BLEND);
	glViewport(0, 0, fb->width, fb->height);

	weston_matrix_init(&sconf.projection);
	weston_matrix_translate(&sconf.projection,
				-fb->width / 2.0, -fb->height / 2.0, 0);
	weston_matrix_scale(&sconf.projection,
			    2.0 / fb->width, -2.0 / fb->height, 1);

	glActiveTexture(GL_TEXTURE0);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	for (side = 0; side < 4; side++) {
		struct weston_geometry g;

		if (!(border_status & (1 << side)))
			continue;

		g = output_get_border_area(go, side);
		draw_output_border_texture(gr, go, &sconf, side,
					   g.x, g.y, g.width, g.height);
	}

	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
}

static void
output_get_border_damage(struct weston_output *output,
			 enum gl_border_status border_status,
			 pixman_region32_t *damage)
{
	struct gl_output_state *go = get_output_state(output);
	unsigned side;

	for (side = 0; side < 4; side++) {
		struct weston_geometry g;

		if (!(border_status & (1 << side)))
			continue;

		g = output_get_border_area(go, side);
		pixman_region32_union_rect(damage, damage,
					   g.x, g.y, g.width, g.height);
	}
}

static void
output_get_damage(struct weston_output *output,
		  pixman_region32_t *buffer_damage, uint32_t *border_damage)
{
	struct gl_output_state *go = get_output_state(output);
	struct gl_renderer *gr = get_renderer(output->compositor);
	EGLint buffer_age = 0;
	EGLBoolean ret;
	int i;

	if (gr->has_egl_buffer_age || gr->has_egl_partial_update) {
		ret = eglQuerySurface(gr->egl_display, go->egl_surface,
				      EGL_BUFFER_AGE_EXT, &buffer_age);
		if (ret == EGL_FALSE) {
			weston_log("buffer age query failed.\n");
			gl_renderer_print_egl_error_state();
		}
	} else if (go->swap_behavior_is_preserved) {
		buffer_age = 1;
	}

	if (buffer_age == 0 || buffer_age - 1 > BUFFER_DAMAGE_COUNT) {
		pixman_region32_copy(buffer_damage, &output->region);
		*border_damage = BORDER_ALL_DIRTY;
	} else {
		for (i = 0; i < buffer_age - 1; i++)
			*border_damage |= go->border_damage[(go->buffer_damage_index + i) % BUFFER_DAMAGE_COUNT];

		if (*border_damage & BORDER_SIZE_CHANGED) {
			/* If we've had a resize, we have to do a full
			 * repaint. */
			*border_damage |= BORDER_ALL_DIRTY;
			pixman_region32_copy(buffer_damage, &output->region);
		} else {
			for (i = 0; i < buffer_age - 1; i++)
				pixman_region32_union(buffer_damage,
						      buffer_damage,
						      &go->buffer_damage[(go->buffer_damage_index + i) % BUFFER_DAMAGE_COUNT]);
		}
	}
}

static void
output_rotate_damage(struct weston_output *output,
		     pixman_region32_t *output_damage,
		     enum gl_border_status border_status)
{
	struct gl_output_state *go = get_output_state(output);
	struct gl_renderer *gr = get_renderer(output->compositor);

	if (!gr->has_egl_buffer_age && !gr->has_egl_partial_update)
		return;

	go->buffer_damage_index += BUFFER_DAMAGE_COUNT - 1;
	go->buffer_damage_index %= BUFFER_DAMAGE_COUNT;

	pixman_region32_copy(&go->buffer_damage[go->buffer_damage_index], output_damage);
	go->border_damage[go->buffer_damage_index] = border_status;
}

/**
 * Given a region in Weston's (top-left-origin) global co-ordinate space,
 * translate it to the co-ordinate space used by GL for our output
 * rendering. This requires shifting it into output co-ordinate space:
 * translating for output offset within the global co-ordinate space,
 * multiplying by output scale to get buffer rather than logical size.
 *
 * Finally, if borders are drawn around the output, we translate the area
 * to account for the border region around the outside, and add any
 * damage if the borders have been redrawn.
 *
 * @param output The output whose co-ordinate space we are after
 * @param global_region The affected region in global co-ordinate space
 * @param[out] rects Y-inverted quads in {x,y,w,h} order; caller must free
 * @param[out] nrects Number of quads (4x number of co-ordinates)
 */
static void
pixman_region_to_egl_y_invert(struct weston_output *output,
			      struct pixman_region32 *global_region,
			      EGLint **rects,
			      EGLint *nrects)
{
	struct gl_output_state *go = get_output_state(output);
	pixman_region32_t transformed;
	struct pixman_box32 *box;
	EGLint *d;
	int i;

	/* Translate from global to output co-ordinate space. */
	pixman_region32_init(&transformed);
	weston_region_global_to_output(&transformed,
				       output,
				       global_region);

	/* If we have borders drawn around the output, shift our output damage
	 * to account for borders being drawn around the outside, adding any
	 * damage resulting from borders being redrawn. */
	if (output_has_borders(output)) {
		pixman_region32_translate(&transformed,
					  go->area.x, go->area.y);
		output_get_border_damage(output, go->border_status,
					 &transformed);
	}

	/* Convert from a Pixman region into {x,y,w,h} quads, flipping in the
	 * Y axis to account for GL's lower-left-origin co-ordinate space. */
	box = pixman_region32_rectangles(&transformed, nrects);
	*rects = malloc(*nrects * 4 * sizeof(EGLint));

	d = *rects;
	for (i = 0; i < *nrects; ++i) {
		*d++ = box[i].x1;
		*d++ = go->fb_size.height - box[i].y2;
		*d++ = box[i].x2 - box[i].x1;
		*d++ = box[i].y2 - box[i].y1;
	}

	pixman_region32_fini(&transformed);
}

static void
blit_shadow_to_output(struct weston_output *output,
		      pixman_region32_t *output_damage)
{
	struct gl_output_state *go = get_output_state(output);
	struct gl_shader_config sconf = {
		.req = {
			.variant = SHADER_VARIANT_RGBA,
			.input_is_premult = true,
		},
		.projection = {
			.d = { /* transpose */
				 2.0f,  0.0f, 0.0f, 0.0f,
				 0.0f,  2.0f, 0.0f, 0.0f,
				 0.0f,  0.0f, 1.0f, 0.0f,
				-1.0f, -1.0f, 0.0f, 1.0f
			},
			.type = WESTON_MATRIX_TRANSFORM_SCALE |
				WESTON_MATRIX_TRANSFORM_TRANSLATE,
		},
		.view_alpha = 1.0f,
		.input_tex_filter = GL_NEAREST,
		.input_tex[0] = go->shadow.tex,
	};
	struct gl_renderer *gr = get_renderer(output->compositor);
	double width = go->area.width;
	double height = go->area.height;
	struct weston_color_transform *ctransf;
	pixman_box32_t *rects;
	int n_rects;
	int i;
	pixman_region32_t translated_damage;
	GLfloat verts[4 * 2];

	ctransf = output->color_outcome->from_blend_to_output;
	if (!gl_shader_config_set_color_transform(&sconf, ctransf)) {
		weston_log("GL-renderer: %s failed to generate a color transformation.\n", __func__);
		return;
	}

	pixman_region32_init(&translated_damage);

	gl_renderer_use_program(gr, &sconf);
	glDisable(GL_BLEND);

	/* output_damage is in global coordinates */
	pixman_region32_intersect(&translated_damage, output_damage,
				  &output->region);
	/* Convert to output pixel coordinates in-place */
	weston_region_global_to_output(&translated_damage, output,
				       &translated_damage);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	rects = pixman_region32_rectangles(&translated_damage, &n_rects);
	for (i = 0; i < n_rects; i++) {

		verts[0] = rects[i].x1 / width;
		verts[1] = (height - rects[i].y1) / height;
		verts[2] = rects[i].x2 / width;
		verts[3] = (height - rects[i].y1) / height;

		verts[4] = rects[i].x2 / width;
		verts[5] = (height - rects[i].y2) / height;
		verts[6] = rects[i].x1 / width;
		verts[7] = (height - rects[i].y2) / height;

		/* position: */
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
		/* texcoord: */
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, verts);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);

	glBindTexture(GL_TEXTURE_2D, 0);
	pixman_region32_fini(&translated_damage);
}

/* NOTE: We now allow falling back to ARGB gl visuals when XRGB is
 * unavailable, so we're assuming the background has no transparency
 * and that everything with a blend, like drop shadows, will have something
 * opaque (like the background) drawn underneath it.
 *
 * Depending on the underlying hardware, violating that assumption could
 * result in seeing through to another display plane.
 */
static void
gl_renderer_repaint_output(struct weston_output *output,
			   pixman_region32_t *output_damage,
			   struct weston_renderbuffer *renderbuffer)
{
	struct gl_output_state *go = get_output_state(output);
	struct weston_compositor *compositor = output->compositor;
	struct gl_renderer *gr = get_renderer(compositor);
	EGLBoolean ret;
	static int errored;
	/* areas we've damaged since we last used this buffer */
	pixman_region32_t previous_damage;
	/* total area we need to repaint this time */
	pixman_region32_t total_damage;
	enum gl_border_status border_status = BORDER_STATUS_CLEAN;
	struct weston_paint_node *pnode;
	const int32_t area_inv_y =
		go->fb_size.height - go->area.y - go->area.height;

	assert(output->from_blend_to_output_by_backend ||
	       output->color_outcome->from_blend_to_output == NULL ||
	       shadow_exists(go));

	if (use_output(output) < 0)
		return;

	/* Clear the used_in_output_repaint flag, so that we can properly track
	 * which surfaces were used in this output repaint. */
	wl_list_for_each_reverse(pnode, &output->paint_node_z_order_list,
				 z_order_link) {
		if (pnode->view->plane == &compositor->primary_plane) {
			struct gl_surface_state *gs =
				get_surface_state(pnode->view->surface);
			gs->used_in_output_repaint = false;
		}
	}

	timeline_begin_render_query(gr, go->render_query);

	/* Calculate the global GL matrix */
	go->output_matrix = output->matrix;
	weston_matrix_translate(&go->output_matrix,
				-(go->area.width / 2.0),
				-(go->area.height / 2.0), 0);
	weston_matrix_scale(&go->output_matrix,
			    2.0 / go->area.width,
			    -2.0 / go->area.height, 1);

	/* If using shadow, redirect all drawing to it first. */
	if (shadow_exists(go)) {
		glBindFramebuffer(GL_FRAMEBUFFER, go->shadow.fbo);
		glViewport(0, 0, go->area.width, go->area.height);
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(go->area.x, area_inv_y,
			   go->area.width, go->area.height);
	}

	/* In fan debug mode, redraw everything to make sure that we clear any
	 * fans left over from previous draws on this buffer.
	 * This precludes the use of EGL_EXT_swap_buffers_with_damage and
	 * EGL_KHR_partial_update, since we damage the whole area. */
	if (gr->fan_debug) {
		pixman_region32_t undamaged;
		pixman_region32_init(&undamaged);
		pixman_region32_subtract(&undamaged, &output->region,
					 output_damage);
		gr->fan_debug = false;
		repaint_views(output, &undamaged);
		gr->fan_debug = true;
		pixman_region32_fini(&undamaged);
	}

	/* previous_damage covers regions damaged in previous paints since we
	 * last used this buffer */
	pixman_region32_init(&previous_damage);
	pixman_region32_init(&total_damage); /* total area to redraw */

	/* Update previous_damage using buffer_age (if available), and store
	 * current damaged region for future use. */
	output_get_damage(output, &previous_damage, &border_status);
	output_rotate_damage(output, output_damage, go->border_status);

	/* Redraw both areas which have changed since we last used this buffer,
	 * as well as the areas we now want to repaint, to make sure the
	 * buffer is up to date. */
	pixman_region32_union(&total_damage, &previous_damage, output_damage);
	border_status |= go->border_status;

	if (gr->has_egl_partial_update && !gr->fan_debug) {
		int n_egl_rects;
		EGLint *egl_rects;

		/* For partial_update, we need to pass the region which has
		 * changed since we last rendered into this specific buffer;
		 * this is total_damage. */
		pixman_region_to_egl_y_invert(output, &total_damage,
					      &egl_rects, &n_egl_rects);
		gr->set_damage_region(gr->egl_display, go->egl_surface,
				      egl_rects, n_egl_rects);
		free(egl_rects);
	}

	if (shadow_exists(go)) {
		/* Repaint into shadow. */
		if (compositor->test_data.test_quirks.gl_force_full_redraw_of_shadow_fb)
			repaint_views(output, &output->region);
		else
			repaint_views(output, output_damage);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(go->area.x, area_inv_y,
			   go->area.width, go->area.height);
		blit_shadow_to_output(output, &total_damage);
	} else {
		repaint_views(output, &total_damage);
	}

	pixman_region32_fini(&total_damage);
	pixman_region32_fini(&previous_damage);

	draw_output_borders(output, border_status);

	gl_renderer_do_capture_tasks(gr, output,
				     WESTON_OUTPUT_CAPTURE_SOURCE_FRAMEBUFFER);
	gl_renderer_do_capture_tasks(gr, output,
				     WESTON_OUTPUT_CAPTURE_SOURCE_FULL_FRAMEBUFFER);
	wl_signal_emit(&output->frame_signal, output_damage);

	timeline_end_render_query(gr);

	if (go->render_sync != EGL_NO_SYNC_KHR)
		gr->destroy_sync(gr->egl_display, go->render_sync);
	go->render_sync = create_render_sync(gr);

	if (gr->swap_buffers_with_damage && !gr->fan_debug) {
		int n_egl_rects;
		EGLint *egl_rects;

		/* For swap_buffers_with_damage, we need to pass the region
		 * which has changed since the previous SwapBuffers on this
		 * surface - this is output_damage. */
		pixman_region_to_egl_y_invert(output, output_damage,
					      &egl_rects, &n_egl_rects);
		ret = gr->swap_buffers_with_damage(gr->egl_display,
						   go->egl_surface,
						   egl_rects, n_egl_rects);
		free(egl_rects);
	} else {
		ret = eglSwapBuffers(gr->egl_display, go->egl_surface);
	}

	if (ret == EGL_FALSE && !errored) {
		errored = 1;
		weston_log("Failed in eglSwapBuffers.\n");
		gl_renderer_print_egl_error_state();
	}

	go->border_status = BORDER_STATUS_CLEAN;

	/* We have to submit the render sync objects after swap buffers, since
	 * the objects get assigned a valid sync file fd only after a gl flush.
	 */
	timeline_submit_render_sync(gr, output, go->render_sync,
				    go->render_query);

	update_buffer_release_fences(compositor, output);

	gl_renderer_garbage_collect_programs(gr);
}

static int
gl_renderer_read_pixels(struct weston_output *output,
			const struct pixel_format_info *format, void *pixels,
			uint32_t x, uint32_t y,
			uint32_t width, uint32_t height)
{
	struct gl_output_state *go = get_output_state(output);
	struct gl_renderer *gr = get_renderer(output->compositor);

	x += go->area.x;
	y += go->fb_size.height - go->area.y - go->area.height;

	if (format->gl_format == 0 || format->gl_type == 0)
		return -1;

	if (use_output(output) < 0)
		return -1;

	if (gr->has_pack_reverse)
		glPixelStorei(GL_PACK_REVERSE_ROW_ORDER_ANGLE, GL_FALSE);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(x, y, width, height, format->gl_format,
		     format->gl_type, pixels);

	return 0;
}

static GLenum
gl_format_from_internal(GLenum internal_format)
{
	switch (internal_format) {
	case GL_R8_EXT:
		return GL_RED_EXT;
	case GL_RG8_EXT:
		return GL_RG_EXT;
	case GL_RGBA16_EXT:
	case GL_RGBA16F:
	case GL_RGB10_A2:
		return GL_RGBA;
	default:
		return internal_format;
	}
}

static void
gl_renderer_flush_damage(struct weston_surface *surface,
			 struct weston_buffer *buffer)
{
	const struct weston_testsuite_quirks *quirks =
		&surface->compositor->test_data.test_quirks;
	struct gl_surface_state *gs = get_surface_state(surface);
	struct gl_buffer_state *gb = gs->buffer;
	struct weston_view *view;
	bool texture_used;
	pixman_box32_t *rectangles;
	uint8_t *data;
	int i, j, n;

	assert(buffer && gb);

	pixman_region32_union(&gb->texture_damage,
			      &gb->texture_damage, &surface->damage);

	/* This can happen if a SHM wl_buffer gets destroyed before we flush
	 * damage, because wayland-server just nukes the wl_shm_buffer from
	 * underneath us */
	if (!buffer->shm_buffer)
		return;

	/* Avoid upload, if the texture won't be used this time.
	 * We still accumulate the damage in texture_damage, and
	 * hold the reference to the buffer, in case the surface
	 * migrates back to the primary plane.
	 */
	texture_used = false;
	wl_list_for_each(view, &surface->views, surface_link) {
		if (view->plane == &surface->compositor->primary_plane) {
			texture_used = true;
			break;
		}
	}
	if (!texture_used)
		return;

	if (!pixman_region32_not_empty(&gb->texture_damage) &&
	    !gb->needs_full_upload)
		goto done;

	data = wl_shm_buffer_get_data(buffer->shm_buffer);

	glActiveTexture(GL_TEXTURE0);

	if (gb->needs_full_upload || quirks->gl_force_full_upload) {
		glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);
		wl_shm_buffer_begin_access(buffer->shm_buffer);

		for (j = 0; j < gb->num_textures; j++) {
			int hsub = pixel_format_hsub(buffer->pixel_format, j);
			int vsub = pixel_format_vsub(buffer->pixel_format, j);

			glBindTexture(GL_TEXTURE_2D, gb->textures[j]);
			glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT,
				      gb->pitch / hsub);
			glTexImage2D(GL_TEXTURE_2D, 0,
				     gb->gl_format[j],
				     buffer->width / hsub,
				     buffer->height / vsub,
				     0,
				     gl_format_from_internal(gb->gl_format[j]),
				     gb->gl_pixel_type,
				     data + gb->offset[j]);
		}
		wl_shm_buffer_end_access(buffer->shm_buffer);
		goto done;
	}

	rectangles = pixman_region32_rectangles(&gb->texture_damage, &n);
	wl_shm_buffer_begin_access(buffer->shm_buffer);
	for (i = 0; i < n; i++) {
		pixman_box32_t r;

		r = weston_surface_to_buffer_rect(surface, rectangles[i]);

		for (j = 0; j < gb->num_textures; j++) {
			int hsub = pixel_format_hsub(buffer->pixel_format, j);
			int vsub = pixel_format_vsub(buffer->pixel_format, j);

			glBindTexture(GL_TEXTURE_2D, gb->textures[j]);
			glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT,
				      gb->pitch / hsub);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, r.x1 / hsub);
			glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, r.y1 / vsub);
			glTexSubImage2D(GL_TEXTURE_2D, 0,
					r.x1 / hsub,
					r.y1 / vsub,
					(r.x2 - r.x1) / hsub,
					(r.y2 - r.y1) / vsub,
					gl_format_from_internal(gb->gl_format[j]),
					gb->gl_pixel_type,
					data + gb->offset[j]);
		}
	}
	wl_shm_buffer_end_access(buffer->shm_buffer);

done:
	pixman_region32_fini(&gb->texture_damage);
	pixman_region32_init(&gb->texture_damage);
	gb->needs_full_upload = false;

	weston_buffer_reference(&gs->buffer_ref, buffer,
				BUFFER_WILL_NOT_BE_ACCESSED);
	weston_buffer_release_reference(&gs->buffer_release_ref, NULL);
}

static void
destroy_buffer_state(struct gl_buffer_state *gb)
{
	int i;

	glDeleteTextures(gb->num_textures, gb->textures);

	for (i = 0; i < gb->num_images; i++)
		gb->gr->destroy_image(gb->gr->egl_display, gb->images[i]);

	pixman_region32_fini(&gb->texture_damage);
	wl_list_remove(&gb->destroy_listener.link);

	free(gb);
}

static void
handle_buffer_destroy(struct wl_listener *listener, void *data)
{
	struct weston_buffer *buffer = data;
	struct gl_buffer_state *gb =
		container_of(listener, struct gl_buffer_state, destroy_listener);

	assert(gb == buffer->renderer_private);
	buffer->renderer_private = NULL;

	destroy_buffer_state(gb);
}

static void
ensure_textures(struct gl_buffer_state *gb, GLenum target, int num_textures)
{
	int i;

	assert(gb->num_textures == 0);

	glActiveTexture(GL_TEXTURE0);

	for (i = 0; i < num_textures; i++) {
		glGenTextures(1, &gb->textures[i]);
		glBindTexture(target, gb->textures[i]);
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	gb->num_textures = num_textures;
	glBindTexture(target, 0);
}

static bool
gl_renderer_attach_shm(struct weston_surface *es, struct weston_buffer *buffer)
{
	struct weston_compositor *ec = es->compositor;
	struct gl_renderer *gr = get_renderer(ec);
	struct gl_surface_state *gs = get_surface_state(es);
	struct gl_buffer_state *gb;
	struct wl_shm_buffer *shm_buffer = buffer->shm_buffer;
	struct weston_buffer *old_buffer = gs->buffer_ref.buffer;
	GLenum gl_format[3] = {0, 0, 0};
	GLenum gl_pixel_type;
	enum gl_shader_texture_variant shader_variant;
	int pitch;
	int offset[3] = { 0, 0, 0 };
	unsigned int num_planes;
	unsigned int i;
	bool using_glesv2 = gr->gl_version < gr_gl_version(3, 0);
	const struct yuv_format_descriptor *yuv = NULL;

	/* When sampling YUV input textures and converting to RGB by hand, we
	 * have to bind to each plane separately, with a different format. For
	 * example, YUYV will have a single wl_shm input plane, but be bound as
	 * two planes within gl-renderer, one as GR88 and one as ARGB8888.
	 *
	 * The yuv_formats array gives us this translation.
	 */
	for (i = 0; i < ARRAY_LENGTH(yuv_formats); ++i) {
		if (yuv_formats[i].format == buffer->pixel_format->format) {
			yuv = &yuv_formats[i];
			break;
		}
	}

	if (yuv) {
		unsigned int out;
		unsigned int shm_plane_count;
		int shm_offset[3] = { 0 };
		int bpp = buffer->pixel_format->bpp;

		/* XXX: Pitch here is given in pixel units, whereas offset is
		 * given in byte units. This is fragile and will break with
		 * new formats.
		 */
		if (!bpp)
			bpp = pixel_format_get_info(yuv->plane[0].format)->bpp;
		pitch = wl_shm_buffer_get_stride(shm_buffer) / (bpp / 8);

		/* well, they all are so far ... */
		gl_pixel_type = GL_UNSIGNED_BYTE;
		shader_variant = yuv->shader_variant;

		/* pre-compute all plane offsets in shm buffer */
		shm_plane_count = pixel_format_get_plane_count(buffer->pixel_format);
		assert(shm_plane_count <= ARRAY_LENGTH(shm_offset));
		for (i = 1; i < shm_plane_count; i++) {
			int hsub, vsub;

			hsub = pixel_format_hsub(buffer->pixel_format, i - 1);
			vsub = pixel_format_vsub(buffer->pixel_format, i - 1);
			shm_offset[i] = shm_offset[i - 1] +
				((pitch / hsub) * (buffer->height / vsub));
		}

		num_planes = yuv->output_planes;
		for (out = 0; out < num_planes; out++) {
			const struct pixel_format_info *sub_info =
				pixel_format_get_info(yuv->plane[out].format);

			assert(sub_info);
			assert(yuv->plane[out].plane_index < (int) shm_plane_count);

			gl_format[out] = sub_info->gl_format;
			offset[out] = shm_offset[yuv->plane[out].plane_index];
		}
	} else {
		int bpp = buffer->pixel_format->bpp;

		assert(pixel_format_get_plane_count(buffer->pixel_format) == 1);
		num_planes = 1;

		if (pixel_format_is_opaque(buffer->pixel_format))
			shader_variant = SHADER_VARIANT_RGBX;
		else
			shader_variant = SHADER_VARIANT_RGBA;

		assert(bpp > 0 && !(bpp & 7));
		pitch = wl_shm_buffer_get_stride(shm_buffer) / (bpp / 8);

		gl_format[0] = buffer->pixel_format->gl_format;
		gl_pixel_type = buffer->pixel_format->gl_type;
	}

	for (i = 0; i < ARRAY_LENGTH(gb->gl_format); i++) {
		/* Fall back to GL_RGBA for 10bpc formats on ES2 */
		if (using_glesv2 && gl_format[i] == GL_RGB10_A2) {
			assert(gl_pixel_type == GL_UNSIGNED_INT_2_10_10_10_REV_EXT);
			gl_format[i] = GL_RGBA;
		}

		/* Fall back to old luminance-based formats if we don't have
		 * GL_EXT_texture_rg, which requires different sampling for
		 * two-component formats. */
		if (!gr->has_gl_texture_rg && gl_format[i] == GL_R8_EXT) {
			assert(gl_pixel_type == GL_UNSIGNED_BYTE);
			assert(shader_variant == SHADER_VARIANT_Y_U_V ||
			       shader_variant == SHADER_VARIANT_Y_UV);
			gl_format[i] = GL_LUMINANCE;
		}
		if (!gr->has_gl_texture_rg && gl_format[i] == GL_RG8_EXT) {
			assert(gl_pixel_type == GL_UNSIGNED_BYTE);
			assert(shader_variant == SHADER_VARIANT_Y_UV ||
			       shader_variant == SHADER_VARIANT_Y_XUXV);
			shader_variant = SHADER_VARIANT_Y_XUXV;
			gl_format[i] = GL_LUMINANCE_ALPHA;
		}
	}

	/* If this surface previously had a SHM buffer, its gl_buffer_state will
	 * be speculatively retained. Check to see if we can reuse it rather
	 * than allocating a new one. */
	assert(!gs->buffer ||
	      (old_buffer && old_buffer->type == WESTON_BUFFER_SHM));
	if (gs->buffer &&
	    buffer->width == old_buffer->width &&
	    buffer->height == old_buffer->height &&
	    buffer->pixel_format == old_buffer->pixel_format) {
		gs->buffer->pitch = pitch;
		memcpy(gs->buffer->offset, offset, sizeof(offset));
		return true;
	}

	if (gs->buffer)
		destroy_buffer_state(gs->buffer);
	gs->buffer = NULL;

	gb = zalloc(sizeof(*gb));
	if (!gb)
		return false;
	gb->gr = gr;

	wl_list_init(&gb->destroy_listener.link);
	pixman_region32_init(&gb->texture_damage);

	gb->pitch = pitch;
	gb->shader_variant = shader_variant;
	ARRAY_COPY(gb->offset, offset);
	ARRAY_COPY(gb->gl_format, gl_format);
	gb->gl_pixel_type = gl_pixel_type;
	gb->needs_full_upload = true;

	gs->buffer = gb;
	gs->surface = es;

	ensure_textures(gb, GL_TEXTURE_2D, num_planes);

	return true;
}

static bool
gl_renderer_fill_buffer_info(struct weston_compositor *ec,
			     struct weston_buffer *buffer)
{
	struct gl_renderer *gr = get_renderer(ec);
	struct gl_buffer_state *gb = zalloc(sizeof(*gb));
	EGLint format;
	uint32_t fourcc;
	GLenum target;
	EGLint y_inverted;
	bool ret = true;
	int i;

	if (!gb)
		return false;

	gb->gr = gr;
	pixman_region32_init(&gb->texture_damage);

	buffer->legacy_buffer = (struct wl_buffer *)buffer->resource;
	ret &= gr->query_buffer(gr->egl_display, buffer->legacy_buffer,
			        EGL_WIDTH, &buffer->width);
	ret &= gr->query_buffer(gr->egl_display, buffer->legacy_buffer,
				EGL_HEIGHT, &buffer->height);
	ret &= gr->query_buffer(gr->egl_display, buffer->legacy_buffer,
				EGL_TEXTURE_FORMAT, &format);
	if (!ret) {
		weston_log("eglQueryWaylandBufferWL failed\n");
		gl_renderer_print_egl_error_state();
		goto err_free;
	}

	/* The legacy EGL buffer interface only describes the channels we can
	 * sample from; not their depths or order. Take a stab at something
	 * which might be representative. Pessimise extremely hard for
	 * TEXTURE_EXTERNAL_OES. */
	switch (format) {
	case EGL_TEXTURE_RGB:
		fourcc = DRM_FORMAT_XRGB8888;
		gb->num_images = 1;
		gb->shader_variant = SHADER_VARIANT_RGBA;
		break;
	case EGL_TEXTURE_RGBA:
		fourcc = DRM_FORMAT_ARGB8888;
		gb->num_images = 1;
		gb->shader_variant = SHADER_VARIANT_RGBA;
		break;
	case EGL_TEXTURE_EXTERNAL_WL:
		fourcc = DRM_FORMAT_ARGB8888;
		gb->num_images = 1;
		gb->shader_variant = SHADER_VARIANT_EXTERNAL;
		break;
	case EGL_TEXTURE_Y_XUXV_WL:
		fourcc = DRM_FORMAT_YUYV;
		gb->num_images = 2;
		gb->shader_variant = SHADER_VARIANT_Y_XUXV;
		break;
	case EGL_TEXTURE_Y_UV_WL:
		fourcc = DRM_FORMAT_NV12;
		gb->num_images = 2;
		gb->shader_variant = SHADER_VARIANT_Y_UV;
		break;
	case EGL_TEXTURE_Y_U_V_WL:
		fourcc = DRM_FORMAT_YUV420;
		gb->num_images = 3;
		gb->shader_variant = SHADER_VARIANT_Y_U_V;
		break;
	default:
		assert(0 && "not reached");
	}

	buffer->pixel_format = pixel_format_get_info(fourcc);
	assert(buffer->pixel_format);
	buffer->format_modifier = DRM_FORMAT_MOD_INVALID;

	/* Assume scanout co-ordinate space i.e. (0,0) is top-left
	 * if the query fails */
	ret = gr->query_buffer(gr->egl_display, buffer->legacy_buffer,
			       EGL_WAYLAND_Y_INVERTED_WL, &y_inverted);
	if (!ret || y_inverted)
		buffer->buffer_origin = ORIGIN_TOP_LEFT;
	else
		buffer->buffer_origin = ORIGIN_BOTTOM_LEFT;

	for (i = 0; i < gb->num_images; i++) {
		const EGLint attribs[] = {
			EGL_WAYLAND_PLANE_WL,	 i,
			EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
			EGL_NONE
		};

		gb->images[i] = gr->create_image(gr->egl_display,
						 EGL_NO_CONTEXT,
						 EGL_WAYLAND_BUFFER_WL,
						 buffer->legacy_buffer,
						 attribs);
		if (gb->images[i] == EGL_NO_IMAGE_KHR) {
			weston_log("couldn't create EGLImage for plane %d\n", i);
			goto err_img;
		}
	}

	target = gl_shader_texture_variant_get_target(gb->shader_variant);
	ensure_textures(gb, target, gb->num_images);

	buffer->renderer_private = gb;
	gb->destroy_listener.notify = handle_buffer_destroy;
	wl_signal_add(&buffer->destroy_signal, &gb->destroy_listener);
	return true;

err_img:
	while (--i >= 0)
		gr->destroy_image(gb->gr->egl_display, gb->images[i]);
err_free:
	free(gb);
	return false;
}

static bool
gl_renderer_attach_egl(struct weston_surface *es, struct weston_buffer *buffer)
{
	struct weston_compositor *ec = es->compositor;
	struct gl_renderer *gr = get_renderer(ec);
	struct gl_surface_state *gs = get_surface_state(es);
	struct gl_buffer_state *gb = buffer->renderer_private;
	GLenum target;
	int i;

	assert(gb);

	gs->buffer = gb;

	target = gl_shader_texture_variant_get_target(gb->shader_variant);
	for (i = 0; i < gb->num_images; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(target, gb->textures[i]);
		gr->image_target_texture_2d(target, gb->images[i]);
	}

	return true;
}

static void
gl_renderer_destroy_dmabuf(struct linux_dmabuf_buffer *dmabuf)
{
	struct gl_buffer_state *gb =
		linux_dmabuf_buffer_get_user_data(dmabuf);

	linux_dmabuf_buffer_set_user_data(dmabuf, NULL, NULL);
	destroy_buffer_state(gb);
}

static EGLImageKHR
import_simple_dmabuf(struct gl_renderer *gr,
                     const struct dmabuf_attributes *attributes)
{
	EGLint attribs[52];
	int atti = 0;
	bool has_modifier;

	/* This requires the Mesa commit in
	 * Mesa 10.3 (08264e5dad4df448e7718e782ad9077902089a07) or
	 * Mesa 10.2.7 (55d28925e6109a4afd61f109e845a8a51bd17652).
	 * Otherwise Mesa closes the fd behind our back and re-importing
	 * will fail.
	 * https://bugs.freedesktop.org/show_bug.cgi?id=76188
	 */

	attribs[atti++] = EGL_WIDTH;
	attribs[atti++] = attributes->width;
	attribs[atti++] = EGL_HEIGHT;
	attribs[atti++] = attributes->height;
	attribs[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
	attribs[atti++] = attributes->format;
	attribs[atti++] = EGL_IMAGE_PRESERVED_KHR;
	attribs[atti++] = EGL_TRUE;

	if (attributes->modifier[0] != DRM_FORMAT_MOD_INVALID) {
		if (!gr->has_dmabuf_import_modifiers)
			return NULL;
		has_modifier = true;
	} else {
		has_modifier = false;
	}

	if (attributes->n_planes > 0) {
		attribs[atti++] = EGL_DMA_BUF_PLANE0_FD_EXT;
		attribs[atti++] = attributes->fd[0];
		attribs[atti++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
		attribs[atti++] = attributes->offset[0];
		attribs[atti++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
		attribs[atti++] = attributes->stride[0];
		if (has_modifier) {
			attribs[atti++] = EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT;
			attribs[atti++] = attributes->modifier[0] & 0xFFFFFFFF;
			attribs[atti++] = EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT;
			attribs[atti++] = attributes->modifier[0] >> 32;
		}
	}

	if (attributes->n_planes > 1) {
		attribs[atti++] = EGL_DMA_BUF_PLANE1_FD_EXT;
		attribs[atti++] = attributes->fd[1];
		attribs[atti++] = EGL_DMA_BUF_PLANE1_OFFSET_EXT;
		attribs[atti++] = attributes->offset[1];
		attribs[atti++] = EGL_DMA_BUF_PLANE1_PITCH_EXT;
		attribs[atti++] = attributes->stride[1];
		if (has_modifier) {
			attribs[atti++] = EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT;
			attribs[atti++] = attributes->modifier[1] & 0xFFFFFFFF;
			attribs[atti++] = EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT;
			attribs[atti++] = attributes->modifier[1] >> 32;
		}
	}

	if (attributes->n_planes > 2) {
		attribs[atti++] = EGL_DMA_BUF_PLANE2_FD_EXT;
		attribs[atti++] = attributes->fd[2];
		attribs[atti++] = EGL_DMA_BUF_PLANE2_OFFSET_EXT;
		attribs[atti++] = attributes->offset[2];
		attribs[atti++] = EGL_DMA_BUF_PLANE2_PITCH_EXT;
		attribs[atti++] = attributes->stride[2];
		if (has_modifier) {
			attribs[atti++] = EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT;
			attribs[atti++] = attributes->modifier[2] & 0xFFFFFFFF;
			attribs[atti++] = EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT;
			attribs[atti++] = attributes->modifier[2] >> 32;
		}
	}

	if (gr->has_dmabuf_import_modifiers) {
		if (attributes->n_planes > 3) {
			attribs[atti++] = EGL_DMA_BUF_PLANE3_FD_EXT;
			attribs[atti++] = attributes->fd[3];
			attribs[atti++] = EGL_DMA_BUF_PLANE3_OFFSET_EXT;
			attribs[atti++] = attributes->offset[3];
			attribs[atti++] = EGL_DMA_BUF_PLANE3_PITCH_EXT;
			attribs[atti++] = attributes->stride[3];
			attribs[atti++] = EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT;
			attribs[atti++] = attributes->modifier[3] & 0xFFFFFFFF;
			attribs[atti++] = EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT;
			attribs[atti++] = attributes->modifier[3] >> 32;
		}
	}

	attribs[atti++] = EGL_NONE;

	return gr->create_image(gr->egl_display, EGL_NO_CONTEXT,
				EGL_LINUX_DMA_BUF_EXT, NULL, attribs);
}

static EGLImageKHR
import_dmabuf_single_plane(struct gl_renderer *gr,
                           const struct pixel_format_info *info,
                           int idx,
                           const struct dmabuf_attributes *attributes,
                           struct yuv_plane_descriptor *descriptor)
{
	struct dmabuf_attributes plane;
	EGLImageKHR image;
	char fmt[4];
	int hsub = pixel_format_hsub(info, idx);
	int vsub = pixel_format_vsub(info, idx);

	plane.width = attributes->width / hsub;
	plane.height = attributes->height / vsub;
	plane.format = descriptor->format;
	plane.n_planes = 1;
	plane.fd[0] = attributes->fd[descriptor->plane_index];
	plane.offset[0] = attributes->offset[descriptor->plane_index];
	plane.stride[0] = attributes->stride[descriptor->plane_index];
	plane.modifier[0] = attributes->modifier[descriptor->plane_index];

	image = import_simple_dmabuf(gr, &plane);
	if (image == EGL_NO_IMAGE_KHR) {
		weston_log("Failed to import plane %d as %.4s\n",
		           descriptor->plane_index,
		           dump_format(descriptor->format, fmt));
		return NULL;
	}

	return image;
}

static bool
import_yuv_dmabuf(struct gl_renderer *gr, struct gl_buffer_state *gb,
		  struct dmabuf_attributes *attributes)
{
	unsigned i;
	int j;
	struct yuv_format_descriptor *format = NULL;
	const struct pixel_format_info *info;
	int plane_count;
	GLenum target;
	char fmt[4];

	for (i = 0; i < ARRAY_LENGTH(yuv_formats); ++i) {
		if (yuv_formats[i].format == attributes->format) {
			format = &yuv_formats[i];
			break;
		}
	}

	if (!format) {
		weston_log("Error during import, and no known conversion for format "
		           "%.4s in the renderer\n",
		           dump_format(attributes->format, fmt));
		return false;
	}

	info = pixel_format_get_info(attributes->format);
	assert(info);
	plane_count = pixel_format_get_plane_count(info);

	if (attributes->n_planes != plane_count) {
		weston_log("%.4s dmabuf must contain %d plane%s (%d provided)\n",
		           dump_format(format->format, fmt),
		           plane_count,
		           (plane_count > 1) ? "s" : "",
		           attributes->n_planes);
		return false;
	}

	for (j = 0; j < format->output_planes; ++j) {
		gb->images[j] = import_dmabuf_single_plane(gr, info, j, attributes,
		                                           &format->plane[j]);
		if (gb->images[j] == EGL_NO_IMAGE_KHR) {
			while (--j >= 0) {
				gr->destroy_image(gb->gr->egl_display,
						  gb->images[j]);
				gb->images[j] = NULL;
			}
			return false;
		}
	}

	gb->num_images = format->output_planes;
	gb->shader_variant = format->shader_variant;

	target = gl_shader_texture_variant_get_target(gb->shader_variant);
	ensure_textures(gb, target, gb->num_images);

	return true;
}

static void
gl_renderer_query_dmabuf_modifiers_full(struct gl_renderer *gr, int format,
					uint64_t **modifiers,
					unsigned **external_only,
					int *num_modifiers);

static struct dmabuf_format*
dmabuf_format_create(struct gl_renderer *gr, uint32_t format)
{
	struct dmabuf_format *dmabuf_format;

	dmabuf_format = calloc(1, sizeof(struct dmabuf_format));
	if (!dmabuf_format)
		return NULL;

	dmabuf_format->format = format;

	gl_renderer_query_dmabuf_modifiers_full(gr, format,
			&dmabuf_format->modifiers,
			&dmabuf_format->external_only,
			&dmabuf_format->num_modifiers);

	if (dmabuf_format->num_modifiers == 0) {
		free(dmabuf_format);
		return NULL;
	}

	wl_list_insert(&gr->dmabuf_formats, &dmabuf_format->link);
	return dmabuf_format;
}

static void
dmabuf_format_destroy(struct dmabuf_format *format)
{
	free(format->modifiers);
	free(format->external_only);
	wl_list_remove(&format->link);
	free(format);
}

static GLenum
choose_texture_target(struct gl_renderer *gr,
		      struct dmabuf_attributes *attributes)
{
	struct dmabuf_format *tmp, *format = NULL;

	wl_list_for_each(tmp, &gr->dmabuf_formats, link) {
		if (tmp->format == attributes->format) {
			format = tmp;
			break;
		}
	}

	if (!format)
		format = dmabuf_format_create(gr, attributes->format);

	if (format) {
		int i;

		for (i = 0; i < format->num_modifiers; ++i) {
			if (format->modifiers[i] == attributes->modifier[0]) {
				if (format->external_only[i])
					return GL_TEXTURE_EXTERNAL_OES;
				else
					return GL_TEXTURE_2D;
			}
		}
	}

	switch (attributes->format & ~DRM_FORMAT_BIG_ENDIAN) {
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_YVYU:
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_VYUY:
	case DRM_FORMAT_AYUV:
	case DRM_FORMAT_XYUV8888:
		return GL_TEXTURE_EXTERNAL_OES;
	default:
		return GL_TEXTURE_2D;
	}
}

static struct gl_buffer_state *
import_dmabuf(struct gl_renderer *gr,
	      struct linux_dmabuf_buffer *dmabuf)
{
	EGLImageKHR egl_image;
	struct gl_buffer_state *gb;

	if (!pixel_format_get_info(dmabuf->attributes.format))
		return NULL;

	gb = zalloc(sizeof(*gb));
	if (!gb)
		return NULL;

	gb->gr = gr;
	pixman_region32_init(&gb->texture_damage);
	wl_list_init(&gb->destroy_listener.link);

	egl_image = import_simple_dmabuf(gr, &dmabuf->attributes);
	if (egl_image != EGL_NO_IMAGE_KHR) {
		GLenum target = choose_texture_target(gr, &dmabuf->attributes);

		gb->num_images = 1;
		gb->images[0] = egl_image;

		switch (target) {
		case GL_TEXTURE_2D:
			gb->shader_variant = SHADER_VARIANT_RGBA;
			break;
		default:
			gb->shader_variant = SHADER_VARIANT_EXTERNAL;
		}

		ensure_textures(gb, target, gb->num_images);

		return gb;
	}

	if (!import_yuv_dmabuf(gr, gb, &dmabuf->attributes)) {
		destroy_buffer_state(gb);
		return NULL;
	}

	return gb;
}

static void
gl_renderer_query_dmabuf_formats(struct weston_compositor *wc,
				int **formats, int *num_formats)
{
	struct gl_renderer *gr = get_renderer(wc);
	static const int fallback_formats[] = {
		DRM_FORMAT_ARGB8888,
		DRM_FORMAT_XRGB8888,
		DRM_FORMAT_YUYV,
		DRM_FORMAT_NV12,
		DRM_FORMAT_YUV420,
		DRM_FORMAT_YUV444,
		DRM_FORMAT_XYUV8888,
	};
	bool fallback = false;
	EGLint num;

	assert(gr->has_dmabuf_import);

	if (!gr->has_dmabuf_import_modifiers ||
	    !gr->query_dmabuf_formats(gr->egl_display, 0, NULL, &num)) {
		num = gr->has_gl_texture_rg ? ARRAY_LENGTH(fallback_formats) : 2;
		fallback = true;
	}

	*formats = calloc(num, sizeof(int));
	if (*formats == NULL) {
		*num_formats = 0;
		return;
	}

	if (fallback) {
		memcpy(*formats, fallback_formats, num * sizeof(int));
		*num_formats = num;
		return;
	}

	if (!gr->query_dmabuf_formats(gr->egl_display, num, *formats, &num)) {
		*num_formats = 0;
		free(*formats);
		return;
	}

	*num_formats = num;
}

static void
gl_renderer_query_dmabuf_modifiers_full(struct gl_renderer *gr, int format,
					uint64_t **modifiers,
					unsigned **external_only,
					int *num_modifiers)
{
	int num;

	assert(gr->has_dmabuf_import);

	if (!gr->has_dmabuf_import_modifiers ||
		!gr->query_dmabuf_modifiers(gr->egl_display, format, 0, NULL,
					    NULL, &num) ||
		num == 0) {
		*num_modifiers = 0;
		return;
	}

	*modifiers = calloc(num, sizeof(uint64_t));
	if (*modifiers == NULL) {
		*num_modifiers = 0;
		return;
	}
	if (external_only) {
		*external_only = calloc(num, sizeof(unsigned));
		if (*external_only == NULL) {
			*num_modifiers = 0;
			free(*modifiers);
			return;
		}
	}
	if (!gr->query_dmabuf_modifiers(gr->egl_display, format,
				num, *modifiers, external_only ?
				*external_only : NULL, &num)) {
		*num_modifiers = 0;
		free(*modifiers);
		if (external_only)
			free(*external_only);
		return;
	}

	*num_modifiers = num;
}

static void
gl_renderer_query_dmabuf_modifiers(struct weston_compositor *wc, int format,
					uint64_t **modifiers,
					int *num_modifiers)
{
	struct gl_renderer *gr = get_renderer(wc);

	gl_renderer_query_dmabuf_modifiers_full(gr, format, modifiers, NULL,
			num_modifiers);
}

static bool
gl_renderer_import_dmabuf(struct weston_compositor *ec,
			  struct linux_dmabuf_buffer *dmabuf)
{
	struct gl_renderer *gr = get_renderer(ec);
	struct gl_buffer_state *gb;
	int i;

	assert(gr->has_dmabuf_import);

	for (i = 0; i < dmabuf->attributes.n_planes; i++) {
		/* return if EGL doesn't support import modifiers */
		if (dmabuf->attributes.modifier[i] != DRM_FORMAT_MOD_INVALID)
			if (!gr->has_dmabuf_import_modifiers)
				return false;

		/* return if modifiers passed are unequal */
		if (dmabuf->attributes.modifier[i] !=
		    dmabuf->attributes.modifier[0])
			return false;
	}

	/* reject all flags we do not recognize or handle */
	if (dmabuf->attributes.flags & ~ZWP_LINUX_BUFFER_PARAMS_V1_FLAGS_Y_INVERT)
		return false;

	gb = import_dmabuf(gr, dmabuf);
	if (!gb)
		return false;

	linux_dmabuf_buffer_set_user_data(dmabuf, gb,
		gl_renderer_destroy_dmabuf);

	return true;
}

static struct gl_buffer_state *
ensure_renderer_gl_buffer_state(struct weston_surface *surface,
				struct weston_buffer *buffer)
{
	struct gl_renderer *gr = get_renderer(surface->compositor);
	struct gl_surface_state *gs = get_surface_state(surface);
	struct gl_buffer_state *gb = buffer->renderer_private;

	if (gb) {
		gs->buffer = gb;
		return gb;
	}

	gb = zalloc(sizeof(*gb));
	gb->gr = gr;
	pixman_region32_init(&gb->texture_damage);
	buffer->renderer_private = gb;
	gb->destroy_listener.notify = handle_buffer_destroy;
	wl_signal_add(&buffer->destroy_signal, &gb->destroy_listener);

	gs->buffer = gb;

	return gb;
}

static void
attach_direct_display_censor_placeholder(struct weston_surface *surface,
					 struct weston_buffer *buffer)
{
	struct gl_buffer_state *gb;

	gb = ensure_renderer_gl_buffer_state(surface, buffer);

	/* uses the same color as the content-protection placeholder */
	gb->color[0] = 0.40f;
	gb->color[1] = 0.0f;
	gb->color[2] = 0.0f;
	gb->color[3] = 1.0f;

	gb->shader_variant = SHADER_VARIANT_SOLID;
}


static bool
gl_renderer_attach_dmabuf(struct weston_surface *surface,
			  struct weston_buffer *buffer)
{
	struct gl_renderer *gr = get_renderer(surface->compositor);
	struct gl_surface_state *gs = get_surface_state(surface);
	struct gl_buffer_state *gb;
	struct linux_dmabuf_buffer *dmabuf = buffer->dmabuf;
	GLenum target;
	int i;

	if (buffer->direct_display) {
		attach_direct_display_censor_placeholder(surface, buffer);
		return true;
	}

	/* Thanks to linux-dmabuf being totally independent of libweston,
	 * the first time a dmabuf is attached, the gl_buffer_state will
	 * only be set as userdata on the dmabuf, not on the weston_buffer.
	 * When this happens, steal it away into the weston_buffer. */
	if (!buffer->renderer_private) {
		gb = linux_dmabuf_buffer_get_user_data(dmabuf);
		assert(gb);
		linux_dmabuf_buffer_set_user_data(dmabuf, NULL, NULL);
		buffer->renderer_private = gb;
		gb->destroy_listener.notify = handle_buffer_destroy;
		wl_signal_add(&buffer->destroy_signal, &gb->destroy_listener);
	}

	assert(buffer->renderer_private);
	assert(linux_dmabuf_buffer_get_user_data(dmabuf) == NULL);
	gb = buffer->renderer_private;

	gs->buffer = gb;

	target = gl_shader_texture_variant_get_target(gb->shader_variant);
	for (i = 0; i < gb->num_images; ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(target, gb->textures[i]);
		gr->image_target_texture_2d(target, gb->images[i]);
	}

	return true;
}

static const struct weston_drm_format_array *
gl_renderer_get_supported_formats(struct weston_compositor *ec)
{
	struct gl_renderer *gr = get_renderer(ec);

	return &gr->supported_formats;
}

static int
populate_supported_formats(struct weston_compositor *ec,
			   struct weston_drm_format_array *supported_formats)
{
	struct weston_drm_format *fmt;
	int *formats = NULL;
	uint64_t *modifiers = NULL;
	int num_formats, num_modifiers;
	int i, j;
	int ret = 0;

	/* Use EGL_EXT_image_dma_buf_import_modifiers to query the
	 * list of formats/modifiers of the renderer. */
	gl_renderer_query_dmabuf_formats(ec, &formats, &num_formats);
	if (num_formats == 0)
		return 0;

	for (i = 0; i < num_formats; i++) {
		const struct pixel_format_info *info =
			pixel_format_get_info(formats[i]);

		if (!info || info->hide_from_clients)
			continue;

		fmt = weston_drm_format_array_add_format(supported_formats,
							 formats[i]);
		if (!fmt) {
			ret = -1;
			goto out;
		}

		/* Always add DRM_FORMAT_MOD_INVALID, as EGL implementations
		 * support implicit modifiers. */
		ret = weston_drm_format_add_modifier(fmt, DRM_FORMAT_MOD_INVALID);
		if (ret < 0)
			goto out;

		gl_renderer_query_dmabuf_modifiers(ec, formats[i],
						   &modifiers, &num_modifiers);
		if (num_modifiers == 0)
			continue;

		for (j = 0; j < num_modifiers; j++) {
			/* Skip MOD_INVALID, as it has already been added. */
			if (modifiers[j] == DRM_FORMAT_MOD_INVALID)
				continue;
			ret = weston_drm_format_add_modifier(fmt, modifiers[j]);
			if (ret < 0) {
				free(modifiers);
				goto out;
			}
		}
		free(modifiers);
	}

out:
	free(formats);
	return ret;
}

static bool
gl_renderer_attach_solid(struct weston_surface *surface,
			 struct weston_buffer *buffer)
{
	struct gl_buffer_state *gb;

	gb = ensure_renderer_gl_buffer_state(surface, buffer);

	gb->color[0] = buffer->solid.r;
	gb->color[1] = buffer->solid.g;
	gb->color[2] = buffer->solid.b;
	gb->color[3] = buffer->solid.a;

	gb->shader_variant = SHADER_VARIANT_SOLID;

	return true;
}

static void
gl_renderer_attach(struct weston_surface *es, struct weston_buffer *buffer)
{
	struct gl_surface_state *gs = get_surface_state(es);
	bool ret = false;

	/* SHM buffers are a little special in that they are allocated
	 * per-surface rather than per-buffer, because we keep a shadow
	 * copy of the SHM data in a GL texture; for these we need to
	 * destroy the buffer state when we're switching to another
	 * buffer type. For all the others, the gl_buffer_state comes
	 * from the weston_buffer itself, and will only be destroyed
	 * along with it. */
	if (gs->buffer && gs->buffer_ref.buffer->type == WESTON_BUFFER_SHM) {
		if (!buffer || buffer->type != WESTON_BUFFER_SHM) {
			destroy_buffer_state(gs->buffer);
			gs->buffer = NULL;
		}
	} else {
		gs->buffer = NULL;
	}

	if (!buffer)
		goto out;

	switch (buffer->type) {
	case WESTON_BUFFER_SHM:
		ret = gl_renderer_attach_shm(es, buffer);
		break;
	case WESTON_BUFFER_DMABUF:
		ret = gl_renderer_attach_dmabuf(es, buffer);
		break;
	case WESTON_BUFFER_RENDERER_OPAQUE:
		ret = gl_renderer_attach_egl(es, buffer);
		break;
	case WESTON_BUFFER_SOLID:
		ret = gl_renderer_attach_solid(es, buffer);
		break;
	default:
		break;
	}

	if (!ret) {
		weston_log("unhandled buffer type!\n");
		weston_buffer_send_server_error(buffer,
			"disconnecting due to unhandled buffer type");
		goto out;
	}

	weston_buffer_reference(&gs->buffer_ref, buffer,
				BUFFER_MAY_BE_ACCESSED);
	weston_buffer_release_reference(&gs->buffer_release_ref,
					es->buffer_release_ref.buffer_release);
	return;

out:
	assert(!gs->buffer);
	weston_buffer_reference(&gs->buffer_ref, NULL,
				BUFFER_WILL_NOT_BE_ACCESSED);
	weston_buffer_release_reference(&gs->buffer_release_ref, NULL);
}

static uint32_t
pack_color(pixman_format_code_t format, float *c)
{
	uint8_t r = round(c[0] * 255.0f);
	uint8_t g = round(c[1] * 255.0f);
	uint8_t b = round(c[2] * 255.0f);
	uint8_t a = round(c[3] * 255.0f);

	switch (format) {
	case PIXMAN_a8b8g8r8:
		return (a << 24) | (b << 16) | (g << 8) | r;
	default:
		assert(0);
		return 0;
	}
}

static int
gl_renderer_surface_copy_content(struct weston_surface *surface,
				 void *target, size_t size,
				 int src_x, int src_y,
				 int width, int height)
{
	static const GLfloat verts[4 * 2] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};
	static const GLfloat projmat_normal[16] = { /* transpose */
		 2.0f,  0.0f, 0.0f, 0.0f,
		 0.0f,  2.0f, 0.0f, 0.0f,
		 0.0f,  0.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f, 1.0f
	};
	static const GLfloat projmat_yinvert[16] = { /* transpose */
		 2.0f,  0.0f, 0.0f, 0.0f,
		 0.0f, -2.0f, 0.0f, 0.0f,
		 0.0f,  0.0f, 1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f, 1.0f
	};
	struct gl_shader_config sconf = {
		.view_alpha = 1.0f,
		.input_tex_filter = GL_NEAREST,
	};
	const pixman_format_code_t format = PIXMAN_a8b8g8r8;
	const size_t bytespp = 4; /* PIXMAN_a8b8g8r8 */
	const GLenum gl_format = GL_RGBA; /* PIXMAN_a8b8g8r8 little-endian */
	struct gl_renderer *gr = get_renderer(surface->compositor);
	struct gl_surface_state *gs = get_surface_state(surface);
	struct gl_buffer_state *gb = gs->buffer;
	struct weston_buffer *buffer = gs->buffer_ref.buffer;
	int cw, ch;
	GLuint fbo;
	GLuint tex;
	GLenum status;
	int ret = -1;

	assert(buffer);

	cw = buffer->width;
	ch = buffer->height;

	switch (buffer->type) {
	case WESTON_BUFFER_SOLID:
		*(uint32_t *)target = pack_color(format, gb->color);
		return 0;
	case WESTON_BUFFER_SHM:
		gl_renderer_flush_damage(surface, buffer);
		/* fall through */
	case WESTON_BUFFER_DMABUF:
	case WESTON_BUFFER_RENDERER_OPAQUE:
		break;
	}

	gl_shader_config_set_input_textures(&sconf, gs);

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cw, ch,
		     0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			       GL_TEXTURE_2D, tex, 0);

	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		weston_log("%s: fbo error: %#x\n", __func__, status);
		goto out;
	}

	glViewport(0, 0, cw, ch);
	glDisable(GL_BLEND);
	if (buffer->buffer_origin == ORIGIN_TOP_LEFT)
		ARRAY_COPY(sconf.projection.d, projmat_normal);
	else
		ARRAY_COPY(sconf.projection.d, projmat_yinvert);
	sconf.projection.type = WESTON_MATRIX_TRANSFORM_SCALE |
				WESTON_MATRIX_TRANSFORM_TRANSLATE;

	if (!gl_renderer_use_program(gr, &sconf))
		goto out;

	/* position: */
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, verts);
	glEnableVertexAttribArray(0);

	/* texcoord: */
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, verts);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);

	if (gr->has_pack_reverse)
		glPixelStorei(GL_PACK_REVERSE_ROW_ORDER_ANGLE, GL_FALSE);
	glPixelStorei(GL_PACK_ALIGNMENT, bytespp);
	glReadPixels(src_x, src_y, width, height, gl_format,
		     GL_UNSIGNED_BYTE, target);
	ret = 0;

out:
	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(1, &tex);

	return ret;
}

static void
surface_state_destroy(struct gl_surface_state *gs, struct gl_renderer *gr)
{
	wl_list_remove(&gs->surface_destroy_listener.link);
	wl_list_remove(&gs->renderer_destroy_listener.link);

	gs->surface->renderer_state = NULL;

	if (gs->buffer && gs->buffer_ref.buffer->type == WESTON_BUFFER_SHM)
		destroy_buffer_state(gs->buffer);
	gs->buffer = NULL;

	weston_buffer_reference(&gs->buffer_ref, NULL,
				BUFFER_WILL_NOT_BE_ACCESSED);
	weston_buffer_release_reference(&gs->buffer_release_ref, NULL);

	free(gs);
}

static void
surface_state_handle_surface_destroy(struct wl_listener *listener, void *data)
{
	struct gl_surface_state *gs;
	struct gl_renderer *gr;

	gs = container_of(listener, struct gl_surface_state,
			  surface_destroy_listener);

	gr = get_renderer(gs->surface->compositor);

	surface_state_destroy(gs, gr);
}

static void
surface_state_handle_renderer_destroy(struct wl_listener *listener, void *data)
{
	struct gl_surface_state *gs;
	struct gl_renderer *gr;

	gr = data;

	gs = container_of(listener, struct gl_surface_state,
			  renderer_destroy_listener);

	surface_state_destroy(gs, gr);
}

static int
gl_renderer_create_surface(struct weston_surface *surface)
{
	struct gl_surface_state *gs;
	struct gl_renderer *gr = get_renderer(surface->compositor);

	gs = zalloc(sizeof *gs);
	if (gs == NULL)
		return -1;

	/* A buffer is never attached to solid color surfaces, yet
	 * they still go through texcoord computations. Do not divide
	 * by zero there.
	 */
	gs->surface = surface;

	surface->renderer_state = gs;

	gs->surface_destroy_listener.notify =
		surface_state_handle_surface_destroy;
	wl_signal_add(&surface->destroy_signal,
		      &gs->surface_destroy_listener);

	gs->renderer_destroy_listener.notify =
		surface_state_handle_renderer_destroy;
	wl_signal_add(&gr->destroy_signal,
		      &gs->renderer_destroy_listener);

	if (surface->buffer_ref.buffer) {
		gl_renderer_attach(surface, surface->buffer_ref.buffer);
		if (surface->buffer_ref.buffer->type == WESTON_BUFFER_SHM) {
			gl_renderer_flush_damage(surface,
						 surface->buffer_ref.buffer);
		}
	}

	return 0;
}

void
gl_renderer_log_extensions(struct gl_renderer *gr,
			   const char *name, const char *extensions)
{
	const char *p, *end;
	int l;
	int len;

	if (!weston_log_scope_is_enabled(gr->renderer_scope))
		return;

	l = weston_log_scope_printf(gr->renderer_scope, "%s:", name);
	p = extensions;
	while (*p) {
		end = strchrnul(p, ' ');
		len = end - p;
		if (l + len > 78) {
			l = weston_log_scope_printf(gr->renderer_scope,
						    "\n  %.*s", len, p);
		} else {
			l += weston_log_scope_printf(gr->renderer_scope,
						     " %.*s", len, p);
		}
		for (p = end; isspace(*p); p++)
			;
	}
	weston_log_scope_printf(gr->renderer_scope, "\n");
}

static void
log_egl_info(struct gl_renderer *gr, EGLDisplay egldpy)
{
	const char *str;

	str = eglQueryString(egldpy, EGL_VERSION);
	weston_log("EGL version: %s\n", str ? str : "(null)");

	str = eglQueryString(egldpy, EGL_VENDOR);
	weston_log("EGL vendor: %s\n", str ? str : "(null)");

	str = eglQueryString(egldpy, EGL_CLIENT_APIS);
	weston_log("EGL client APIs: %s\n", str ? str : "(null)");

	str = eglQueryString(egldpy, EGL_EXTENSIONS);
	gl_renderer_log_extensions(gr, "EGL extensions", str ? str : "(null)");
}

static void
log_gl_info(struct gl_renderer *gr)
{
	const char *str;

	str = (char *)glGetString(GL_VERSION);
	weston_log("GL version: %s\n", str ? str : "(null)");

	str = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
	weston_log("GLSL version: %s\n", str ? str : "(null)");

	str = (char *)glGetString(GL_VENDOR);
	weston_log("GL vendor: %s\n", str ? str : "(null)");

	str = (char *)glGetString(GL_RENDERER);
	weston_log("GL renderer: %s\n", str ? str : "(null)");

	str = (char *)glGetString(GL_EXTENSIONS);
	gl_renderer_log_extensions(gr, "GL extensions", str ? str : "(null)");
}

static void
gl_renderer_output_set_border(struct weston_output *output,
			      enum gl_renderer_border_side side,
			      int32_t width, int32_t height,
			      int32_t tex_width, unsigned char *data)
{
	struct gl_output_state *go = get_output_state(output);

	if (go->borders[side].width != width ||
	    go->borders[side].height != height)
		/* In this case, we have to blow everything and do a full
		 * repaint. */
		go->border_status |= BORDER_SIZE_CHANGED | BORDER_ALL_DIRTY;

	if (data == NULL) {
		width = 0;
		height = 0;
	}

	go->borders[side].width = width;
	go->borders[side].height = height;
	go->borders[side].tex_width = tex_width;
	go->borders[side].data = data;
	go->border_status |= 1 << side;
}

static bool
gl_renderer_resize_output(struct weston_output *output,
			  const struct weston_size *fb_size,
			  const struct weston_geometry *area)
{
	struct gl_output_state *go = get_output_state(output);
	const struct pixel_format_info *shfmt = go->shadow_format;
	bool ret;

	check_compositing_area(fb_size, area);

	go->fb_size = *fb_size;
	go->area = *area;

	weston_output_update_capture_info(output,
					  WESTON_OUTPUT_CAPTURE_SOURCE_FRAMEBUFFER,
					  area->width, area->height,
					  output->compositor->read_format);

	weston_output_update_capture_info(output,
					  WESTON_OUTPUT_CAPTURE_SOURCE_FULL_FRAMEBUFFER,
					  fb_size->width, fb_size->height,
					  output->compositor->read_format);

	if (!shfmt)
		return true;

	if (shadow_exists(go))
		gl_fbo_texture_fini(&go->shadow);

	ret = gl_fbo_texture_init(&go->shadow, area->width, area->height,
				  shfmt->gl_format, GL_RGBA, shfmt->gl_type);

	return ret;
}

static int
gl_renderer_setup(struct weston_compositor *ec, EGLSurface egl_surface);

static EGLSurface
gl_renderer_create_window_surface(struct gl_renderer *gr,
				  EGLNativeWindowType window_for_legacy,
				  void *window_for_platform,
				  const struct pixel_format_info *const *formats,
				  unsigned formats_count)
{
	EGLSurface egl_surface = EGL_NO_SURFACE;
	EGLConfig egl_config;

	egl_config = gl_renderer_get_egl_config(gr, EGL_WINDOW_BIT,
						formats, formats_count);
	if (egl_config == EGL_NO_CONFIG_KHR)
		return EGL_NO_SURFACE;

	log_egl_config_info(gr->egl_display, egl_config);

	if (gr->create_platform_window)
		egl_surface = gr->create_platform_window(gr->egl_display,
							 egl_config,
							 window_for_platform,
							 NULL);
	else
		egl_surface = eglCreateWindowSurface(gr->egl_display,
						     egl_config,
						     window_for_legacy, NULL);

	return egl_surface;
}

static int
gl_renderer_output_create(struct weston_output *output,
			  EGLSurface surface,
			  const struct weston_size *fb_size,
			  const struct weston_geometry *area)
{
	struct gl_output_state *go;
	struct gl_renderer *gr = get_renderer(output->compositor);
	const struct weston_testsuite_quirks *quirks;
	int i;

	quirks = &output->compositor->test_data.test_quirks;

	go = zalloc(sizeof *go);
	if (go == NULL)
		return -1;

	go->egl_surface = surface;

	for (i = 0; i < BUFFER_DAMAGE_COUNT; i++)
		pixman_region32_init(&go->buffer_damage[i]);

	if (gr->has_disjoint_timer_query)
		gr->gen_queries(1, &go->render_query);

	wl_list_init(&go->timeline_render_point_list);

	go->render_sync = EGL_NO_SYNC_KHR;

	if ((output->color_outcome->from_blend_to_output != NULL &&
	     output->from_blend_to_output_by_backend == false) ||
	    quirks->gl_force_full_redraw_of_shadow_fb) {
		assert(gr->gl_supports_color_transforms);

		go->shadow_format =
			pixel_format_get_info(DRM_FORMAT_ABGR16161616F);
	}

	output->renderer_state = go;

	if (!gl_renderer_resize_output(output, fb_size, area)) {
		weston_log("Output %s failed to create 16F shadow.\n",
			   output->name);
		output->renderer_state = NULL;
		free(go);
		return -1;
	}

	if (shadow_exists(go)) {
		weston_log("Output %s uses 16F shadow.\n",
			   output->name);
	}

	return 0;
}

static int
gl_renderer_output_window_create(struct weston_output *output,
				 const struct gl_renderer_output_options *options)
{
	struct weston_compositor *ec = output->compositor;
	struct gl_renderer *gr = get_renderer(ec);
	EGLSurface egl_surface = EGL_NO_SURFACE;
	int ret = 0;

	egl_surface = gl_renderer_create_window_surface(gr,
							options->window_for_legacy,
							options->window_for_platform,
							options->formats,
							options->formats_count);
	if (egl_surface == EGL_NO_SURFACE) {
		weston_log("failed to create egl surface\n");
		return -1;
	}

	ret = gl_renderer_output_create(output, egl_surface,
					&options->fb_size, &options->area);
	if (ret < 0)
		weston_platform_destroy_egl_surface(gr->egl_display, egl_surface);

	return ret;
}

static int
gl_renderer_output_pbuffer_create(struct weston_output *output,
				  const struct gl_renderer_pbuffer_options *options)
{
	struct gl_renderer *gr = get_renderer(output->compositor);
	struct gl_output_state *go;
	EGLConfig pbuffer_config;
	EGLSurface egl_surface;
	EGLint value = 0;
	int ret;
	EGLint pbuffer_attribs[] = {
		EGL_WIDTH, options->fb_size.width,
		EGL_HEIGHT, options->fb_size.height,
		EGL_NONE
	};

	pbuffer_config = gl_renderer_get_egl_config(gr, EGL_PBUFFER_BIT,
						    options->formats,
						    options->formats_count);
	if (pbuffer_config == EGL_NO_CONFIG_KHR) {
		weston_log("failed to choose EGL config for PbufferSurface\n");
		return -1;
	}

	log_egl_config_info(gr->egl_display, pbuffer_config);

	egl_surface = eglCreatePbufferSurface(gr->egl_display, pbuffer_config,
					      pbuffer_attribs);
	if (egl_surface == EGL_NO_SURFACE) {
		weston_log("failed to create egl surface\n");
		gl_renderer_print_egl_error_state();
		return -1;
	}

	eglSurfaceAttrib(gr->egl_display, egl_surface,
			 EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED);
	if (!eglQuerySurface(gr->egl_display, egl_surface,
			     EGL_SWAP_BEHAVIOR, &value) ||
	    value != EGL_BUFFER_PRESERVED) {
		weston_log("Error: pbuffer surface does not support EGL_BUFFER_PRESERVED, got 0x%x."
			   " Continuing anyway.\n", value);
	}

	ret = gl_renderer_output_create(output, egl_surface,
					&options->fb_size, &options->area);
	if (ret < 0) {
		eglDestroySurface(gr->egl_display, egl_surface);
	} else {
		go = get_output_state(output);
		go->swap_behavior_is_preserved = true;
	}

	return ret;
}

static void
gl_renderer_output_destroy(struct weston_output *output)
{
	struct gl_renderer *gr = get_renderer(output->compositor);
	struct gl_output_state *go = get_output_state(output);
	struct timeline_render_point *trp, *tmp;
	int i;

	for (i = 0; i < 2; i++)
		pixman_region32_fini(&go->buffer_damage[i]);

	if (shadow_exists(go))
		gl_fbo_texture_fini(&go->shadow);

	eglMakeCurrent(gr->egl_display,
		       gr->dummy_surface, gr->dummy_surface, gr->egl_context);

	weston_platform_destroy_egl_surface(gr->egl_display, go->egl_surface);

	if (!wl_list_empty(&go->timeline_render_point_list))
		weston_log("warning: discarding pending timeline render"
			   "objects at output destruction");

	if (gr->has_disjoint_timer_query)
		gr->delete_queries(1, &go->render_query);

	wl_list_for_each_safe(trp, tmp, &go->timeline_render_point_list, link)
		timeline_render_point_destroy(trp);

	if (go->render_sync != EGL_NO_SYNC_KHR)
		gr->destroy_sync(gr->egl_display, go->render_sync);

	free(go);
}

static int
gl_renderer_create_fence_fd(struct weston_output *output)
{
	struct gl_output_state *go = get_output_state(output);
	struct gl_renderer *gr = get_renderer(output->compositor);
	int fd;

	if (go->render_sync == EGL_NO_SYNC_KHR)
		return -1;

	fd = gr->dup_native_fence_fd(gr->egl_display, go->render_sync);
	if (fd == EGL_NO_NATIVE_FENCE_FD_ANDROID)
		return -1;

	return fd;
}

static void
gl_renderer_destroy(struct weston_compositor *ec)
{
	struct gl_renderer *gr = get_renderer(ec);
	struct dmabuf_format *format, *next_format;

	wl_signal_emit(&gr->destroy_signal, gr);

	if (gr->has_bind_display)
		gr->unbind_display(gr->egl_display, ec->wl_display);

	gl_renderer_shader_list_destroy(gr);
	if (gr->fallback_shader)
		gl_shader_destroy(gr, gr->fallback_shader);

	/* Work around crash in egl_dri2.c's dri2_make_current() - when does this apply? */
	eglMakeCurrent(gr->egl_display,
		       EGL_NO_SURFACE, EGL_NO_SURFACE,
		       EGL_NO_CONTEXT);

	wl_list_for_each_safe(format, next_format, &gr->dmabuf_formats, link)
		dmabuf_format_destroy(format);

	weston_drm_format_array_fini(&gr->supported_formats);

	if (gr->dummy_surface != EGL_NO_SURFACE)
		weston_platform_destroy_egl_surface(gr->egl_display,
						    gr->dummy_surface);

	eglTerminate(gr->egl_display);
	eglReleaseThread();

	wl_array_release(&gr->vertices);
	wl_array_release(&gr->vtxcnt);

	if (gr->fragment_binding)
		weston_binding_destroy(gr->fragment_binding);
	if (gr->fan_binding)
		weston_binding_destroy(gr->fan_binding);

	weston_log_scope_destroy(gr->shader_scope);
	weston_log_scope_destroy(gr->renderer_scope);
	free(gr);
}

static int
gl_renderer_create_pbuffer_surface(struct gl_renderer *gr) {
	EGLConfig pbuffer_config;
	static const EGLint pbuffer_attribs[] = {
		EGL_WIDTH, 10,
		EGL_HEIGHT, 10,
		EGL_NONE
	};

	pbuffer_config = gr->egl_config;
	if (pbuffer_config == EGL_NO_CONFIG_KHR) {
		pbuffer_config =
			gl_renderer_get_egl_config(gr, EGL_PBUFFER_BIT,
						   NULL, 0);
	}
	if (pbuffer_config == EGL_NO_CONFIG_KHR) {
		weston_log("failed to choose EGL config for PbufferSurface\n");
		return -1;
	}

	gr->dummy_surface = eglCreatePbufferSurface(gr->egl_display,
						    pbuffer_config,
						    pbuffer_attribs);

	if (gr->dummy_surface == EGL_NO_SURFACE) {
		weston_log("failed to create PbufferSurface\n");
		return -1;
	}

	return 0;
}

static int
create_default_dmabuf_feedback(struct weston_compositor *ec,
			       struct gl_renderer *gr)
{
	struct stat dev_stat;
	struct weston_dmabuf_feedback_tranche *tranche;
	uint32_t flags = 0;

	if (stat(gr->drm_device, &dev_stat) != 0) {
		weston_log("%s: device disappeared, so we can't recover\n", __func__);
		abort();
	}

	ec->default_dmabuf_feedback =
		weston_dmabuf_feedback_create(dev_stat.st_rdev);
	if (!ec->default_dmabuf_feedback)
		return -1;

	tranche =
		weston_dmabuf_feedback_tranche_create(ec->default_dmabuf_feedback,
						      ec->dmabuf_feedback_format_table,
						      dev_stat.st_rdev, flags,
						      RENDERER_PREF);
	if (!tranche) {
		weston_dmabuf_feedback_destroy(ec->default_dmabuf_feedback);
		ec->default_dmabuf_feedback = NULL;
		return -1;
	}

	return 0;
}

static int
gl_renderer_display_create(struct weston_compositor *ec,
			   const struct gl_renderer_display_options *options)
{
	struct gl_renderer *gr;
	int ret;

	gr = zalloc(sizeof *gr);
	if (gr == NULL)
		return -1;

	gr->compositor = ec;
	wl_list_init(&gr->shader_list);
	gr->platform = options->egl_platform;

	gr->renderer_scope = weston_compositor_add_log_scope(ec, "gl-renderer",
		"GL-renderer verbose messages\n", NULL, NULL, gr);
	if (!gr->renderer_scope)
		goto fail;

	gr->shader_scope = gl_shader_scope_create(gr);
	if (!gr->shader_scope)
		goto fail;

	if (gl_renderer_setup_egl_client_extensions(gr) < 0)
		goto fail;

	gr->base.read_pixels = gl_renderer_read_pixels;
	gr->base.repaint_output = gl_renderer_repaint_output;
	gr->base.resize_output = gl_renderer_resize_output;
	gr->base.flush_damage = gl_renderer_flush_damage;
	gr->base.attach = gl_renderer_attach;
	gr->base.destroy = gl_renderer_destroy;
	gr->base.surface_copy_content = gl_renderer_surface_copy_content;
	gr->base.fill_buffer_info = gl_renderer_fill_buffer_info;
	gr->base.type = WESTON_RENDERER_GL;

	if (gl_renderer_setup_egl_display(gr, options->egl_native_display) < 0)
		goto fail;

	weston_drm_format_array_init(&gr->supported_formats);

	log_egl_info(gr, gr->egl_display);

	ec->renderer = &gr->base;

	if (gl_renderer_setup_egl_extensions(ec) < 0)
		goto fail_with_error;

	if (!gr->has_configless_context) {
		EGLint egl_surface_type = options->egl_surface_type;

		if (!gr->has_surfaceless_context)
			egl_surface_type |= EGL_PBUFFER_BIT;

		gr->egl_config =
			gl_renderer_get_egl_config(gr,
						   egl_surface_type,
						   options->formats,
						   options->formats_count);
		if (gr->egl_config == EGL_NO_CONFIG_KHR) {
			weston_log("failed to choose EGL config\n");
			goto fail_terminate;
		}
	}

	ec->capabilities |= WESTON_CAP_ROTATION_ANY;
	ec->capabilities |= WESTON_CAP_CAPTURE_YFLIP;
	ec->capabilities |= WESTON_CAP_VIEW_CLIP_MASK;
	if (gr->has_native_fence_sync && gr->has_wait_sync)
		ec->capabilities |= WESTON_CAP_EXPLICIT_SYNC;

	if (gr->has_dmabuf_import) {
		gr->base.import_dmabuf = gl_renderer_import_dmabuf;
		gr->base.get_supported_formats = gl_renderer_get_supported_formats;
		ret = populate_supported_formats(ec, &gr->supported_formats);
		if (ret < 0)
			goto fail_terminate;
		if (gr->drm_device) {
			/* We support dma-buf feedback only when the renderer
			 * exposes a DRM-device */
			ec->dmabuf_feedback_format_table =
				weston_dmabuf_feedback_format_table_create(&gr->supported_formats);
			if (!ec->dmabuf_feedback_format_table)
				goto fail_terminate;
			ret = create_default_dmabuf_feedback(ec, gr);
			if (ret < 0)
				goto fail_feedback;
		}
	}
	wl_list_init(&gr->dmabuf_formats);

	if (gr->has_surfaceless_context) {
		gr->dummy_surface = EGL_NO_SURFACE;
	} else {
		if (gl_renderer_create_pbuffer_surface(gr) < 0)
			goto fail_with_error;
	}

	wl_signal_init(&gr->destroy_signal);

	if (gl_renderer_setup(ec, gr->dummy_surface) < 0) {
		if (gr->dummy_surface != EGL_NO_SURFACE)
			weston_platform_destroy_egl_surface(gr->egl_display,
							    gr->dummy_surface);
		goto fail_with_error;
	}

	wl_display_add_shm_format(ec->wl_display, WL_SHM_FORMAT_RGB565);
	wl_display_add_shm_format(ec->wl_display, WL_SHM_FORMAT_YUV420);
	wl_display_add_shm_format(ec->wl_display, WL_SHM_FORMAT_YUV444);
	wl_display_add_shm_format(ec->wl_display, WL_SHM_FORMAT_NV12);
	wl_display_add_shm_format(ec->wl_display, WL_SHM_FORMAT_YUYV);
	wl_display_add_shm_format(ec->wl_display, WL_SHM_FORMAT_XYUV8888);
#if __BYTE_ORDER == __LITTLE_ENDIAN
	if (gr->has_texture_type_2_10_10_10_rev) {
		wl_display_add_shm_format(ec->wl_display, WL_SHM_FORMAT_ABGR2101010);
		wl_display_add_shm_format(ec->wl_display, WL_SHM_FORMAT_XBGR2101010);
	}
	if (gr->gl_supports_color_transforms) {
		wl_display_add_shm_format(ec->wl_display, WL_SHM_FORMAT_ABGR16161616F);
		wl_display_add_shm_format(ec->wl_display, WL_SHM_FORMAT_XBGR16161616F);
	}
	if (gr->has_texture_norm16) {
		wl_display_add_shm_format(ec->wl_display, WL_SHM_FORMAT_ABGR16161616);
		wl_display_add_shm_format(ec->wl_display, WL_SHM_FORMAT_XBGR16161616);
	}
#endif

	if (gr->gl_supports_color_transforms)
		ec->capabilities |= WESTON_CAP_COLOR_OPS;

	return 0;

fail_with_error:
	gl_renderer_print_egl_error_state();
	if (gr->drm_device) {
		weston_dmabuf_feedback_destroy(ec->default_dmabuf_feedback);
		ec->default_dmabuf_feedback = NULL;
	}
fail_feedback:
	if (gr->drm_device) {
		weston_dmabuf_feedback_format_table_destroy(ec->dmabuf_feedback_format_table);
		ec->dmabuf_feedback_format_table = NULL;
	}
fail_terminate:
	weston_drm_format_array_fini(&gr->supported_formats);
	eglTerminate(gr->egl_display);
fail:
	weston_log_scope_destroy(gr->shader_scope);
	weston_log_scope_destroy(gr->renderer_scope);
	free(gr);
	ec->renderer = NULL;
	return -1;
}

static void
fragment_debug_binding(struct weston_keyboard *keyboard,
		       const struct timespec *time,
		       uint32_t key, void *data)
{
	struct weston_compositor *ec = data;
	struct gl_renderer *gr = get_renderer(ec);
	struct weston_output *output;

	gr->fragment_shader_debug = !gr->fragment_shader_debug;

	wl_list_for_each(output, &ec->output_list, link)
		weston_output_damage(output);
}

static void
fan_debug_repaint_binding(struct weston_keyboard *keyboard,
			  const struct timespec *time,
			  uint32_t key, void *data)
{
	struct weston_compositor *compositor = data;
	struct gl_renderer *gr = get_renderer(compositor);

	gr->fan_debug = !gr->fan_debug;
	weston_compositor_damage_all(compositor);
}

static uint32_t
get_gl_version(void)
{
	const char *version;
	int major, minor;

	version = (const char *) glGetString(GL_VERSION);
	if (version &&
	    (sscanf(version, "%d.%d", &major, &minor) == 2 ||
	     sscanf(version, "OpenGL ES %d.%d", &major, &minor) == 2) &&
	    major > 0 && minor >= 0) {
		return gr_gl_version(major, minor);
	}

	weston_log("warning: failed to detect GLES version, defaulting to 2.0.\n");
	return gr_gl_version(2, 0);
}

static int
gl_renderer_setup(struct weston_compositor *ec, EGLSurface egl_surface)
{
	struct gl_renderer *gr = get_renderer(ec);
	const char *extensions;
	EGLBoolean ret;

	EGLint context_attribs[16] = {
		EGL_CONTEXT_CLIENT_VERSION, 0,
	};
	unsigned int nattr = 2;

	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		weston_log("failed to bind EGL_OPENGL_ES_API\n");
		gl_renderer_print_egl_error_state();
		return -1;
	}

	/*
	 * Being the compositor we require minimum output latency,
	 * so request a high priority context for ourselves - that should
	 * reschedule all of our rendering and its dependencies to be completed
	 * first. If the driver doesn't permit us to create a high priority
	 * context, it will fallback to the default priority (MEDIUM).
	 */
	if (gr->has_context_priority) {
		context_attribs[nattr++] = EGL_CONTEXT_PRIORITY_LEVEL_IMG;
		context_attribs[nattr++] = EGL_CONTEXT_PRIORITY_HIGH_IMG;
	}

	assert(nattr < ARRAY_LENGTH(context_attribs));
	context_attribs[nattr] = EGL_NONE;

	/* try to create an OpenGLES 3 context first */
	context_attribs[1] = 3;
	gr->egl_context = eglCreateContext(gr->egl_display, gr->egl_config,
					   EGL_NO_CONTEXT, context_attribs);
	if (gr->egl_context == NULL) {
		/* and then fallback to OpenGLES 2 */
		context_attribs[1] = 2;
		gr->egl_context = eglCreateContext(gr->egl_display,
						   gr->egl_config,
						   EGL_NO_CONTEXT,
						   context_attribs);
		if (gr->egl_context == NULL) {
			weston_log("failed to create context\n");
			gl_renderer_print_egl_error_state();
			return -1;
		}
	}

	if (gr->has_context_priority) {
		EGLint value = EGL_CONTEXT_PRIORITY_MEDIUM_IMG;

		eglQueryContext(gr->egl_display, gr->egl_context,
				EGL_CONTEXT_PRIORITY_LEVEL_IMG, &value);

		if (value != EGL_CONTEXT_PRIORITY_HIGH_IMG) {
			weston_log("Failed to obtain a high priority context.\n");
			/* Not an error, continue on as normal */
		}
	}

	ret = eglMakeCurrent(gr->egl_display, egl_surface,
			     egl_surface, gr->egl_context);
	if (ret == EGL_FALSE) {
		weston_log("Failed to make EGL context current.\n");
		gl_renderer_print_egl_error_state();
		return -1;
	}

	gr->gl_version = get_gl_version();
	log_gl_info(gr);

	gr->image_target_texture_2d =
		(void *) eglGetProcAddress("glEGLImageTargetTexture2DOES");

	extensions = (const char *) glGetString(GL_EXTENSIONS);
	if (!extensions) {
		weston_log("Retrieving GL extension string failed.\n");
		return -1;
	}

	if (!weston_check_egl_extension(extensions, "GL_EXT_texture_format_BGRA8888")) {
		weston_log("GL_EXT_texture_format_BGRA8888 not available\n");
		return -1;
	}

	if (weston_check_egl_extension(extensions, "GL_EXT_read_format_bgra"))
		ec->read_format = pixel_format_get_info_by_pixman(PIXMAN_a8r8g8b8);
	else
		ec->read_format = pixel_format_get_info_by_pixman(PIXMAN_a8b8g8r8);

	if (gr->gl_version < gr_gl_version(3, 0) &&
	    !weston_check_egl_extension(extensions, "GL_EXT_unpack_subimage")) {
		weston_log("GL_EXT_unpack_subimage not available.\n");
		return -1;
	}

	if (gr->gl_version >= gr_gl_version(3, 0) ||
	    weston_check_egl_extension(extensions, "GL_EXT_texture_type_2_10_10_10_REV"))
		gr->has_texture_type_2_10_10_10_rev = true;

	if (weston_check_egl_extension(extensions, "GL_EXT_texture_norm16"))
		gr->has_texture_norm16 = true;

	if (weston_check_egl_extension(extensions, "GL_ANGLE_pack_reverse_row_order"))
		gr->has_pack_reverse = true;

	if (gr->gl_version >= gr_gl_version(3, 0) ||
	    weston_check_egl_extension(extensions, "GL_EXT_texture_rg"))
		gr->has_gl_texture_rg = true;

	if (weston_check_egl_extension(extensions, "GL_OES_EGL_image_external"))
		gr->has_egl_image_external = true;

	if (gr->gl_version >= gr_gl_version(3, 0) &&
	    weston_check_egl_extension(extensions, "GL_OES_texture_float_linear") &&
	    weston_check_egl_extension(extensions, "GL_EXT_color_buffer_half_float") &&
		weston_check_egl_extension(extensions, "GL_OES_texture_3D")) {
		gr->gl_supports_color_transforms = true;
	}

	if (weston_check_egl_extension(extensions, "GL_EXT_disjoint_timer_query")) {
		PFNGLGETQUERYIVEXTPROC get_query_iv =
			(void *) eglGetProcAddress("glGetQueryivEXT");
		int elapsed_bits;

		assert(get_query_iv);
		get_query_iv(GL_TIME_ELAPSED_EXT, GL_QUERY_COUNTER_BITS_EXT,
			     &elapsed_bits);
		if (elapsed_bits != 0) {
			gr->gen_queries =
				(void *) eglGetProcAddress("glGenQueriesEXT");
			gr->delete_queries =
				(void *) eglGetProcAddress("glDeleteQueriesEXT");
			gr->begin_query = (void *) eglGetProcAddress("glBeginQueryEXT");
			gr->end_query = (void *) eglGetProcAddress("glEndQueryEXT");
#if !defined(NDEBUG)
			gr->get_query_object_iv =
				(void *) eglGetProcAddress("glGetQueryObjectivEXT");
#endif
			gr->get_query_object_ui64v =
				(void *) eglGetProcAddress("glGetQueryObjectui64vEXT");
			assert(gr->gen_queries);
			assert(gr->delete_queries);
			assert(gr->begin_query);
			assert(gr->end_query);
			assert(gr->get_query_object_iv);
			assert(gr->get_query_object_ui64v);
			gr->has_disjoint_timer_query = true;
		} else {
			weston_log("warning: Disabling render GPU timeline due "
				   "to lack of support for elapsed counters by "
				   "the GL_EXT_disjoint_timer_query "
				   "extension\n");
		}
	} else if (gr->has_native_fence_sync)  {
		weston_log("warning: Disabling render GPU timeline due to "
			   "missing GL_EXT_disjoint_timer_query extension\n");
	}

	glActiveTexture(GL_TEXTURE0);

	gr->fallback_shader = gl_renderer_create_fallback_shader(gr);
	if (!gr->fallback_shader) {
		weston_log("Error: compiling fallback shader failed.\n");
		return -1;
	}

	gr->fragment_binding =
		weston_compositor_add_debug_binding(ec, KEY_S,
						    fragment_debug_binding,
						    ec);
	gr->fan_binding =
		weston_compositor_add_debug_binding(ec, KEY_F,
						    fan_debug_repaint_binding,
						    ec);

	weston_log("GL ES %d.%d - renderer features:\n",
		   gr_gl_version_major(gr->gl_version),
		   gr_gl_version_minor(gr->gl_version));
	weston_log_continue(STAMP_SPACE "read-back format: %s\n",
			    ec->read_format->drm_format_name);
	weston_log_continue(STAMP_SPACE "glReadPixels supports y-flip: %s\n",
			    yesno(gr->has_pack_reverse));
	weston_log_continue(STAMP_SPACE "wl_shm 10 bpc formats: %s\n",
			    yesno(gr->has_texture_type_2_10_10_10_rev));
	weston_log_continue(STAMP_SPACE "wl_shm 16 bpc formats: %s\n",
			    yesno(gr->has_texture_norm16));
	weston_log_continue(STAMP_SPACE "wl_shm half-float formats: %s\n",
			    yesno(gr->gl_supports_color_transforms));
	weston_log_continue(STAMP_SPACE "internal R and RG formats: %s\n",
			    yesno(gr->has_gl_texture_rg));
	weston_log_continue(STAMP_SPACE "OES_EGL_image_external: %s\n",
			    yesno(gr->has_egl_image_external));

	return 0;
}

WL_EXPORT struct gl_renderer_interface gl_renderer_interface = {
	.display_create = gl_renderer_display_create,
	.output_window_create = gl_renderer_output_window_create,
	.output_pbuffer_create = gl_renderer_output_pbuffer_create,
	.output_destroy = gl_renderer_output_destroy,
	.output_set_border = gl_renderer_output_set_border,
	.create_fence_fd = gl_renderer_create_fence_fd,
};
