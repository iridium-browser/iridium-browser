/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2013 Vasily Khoruzhick <anarsoul@gmail.com>
 * Copyright © 2015 Collabora, Ltd.
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
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include "pixman-renderer.h"
#include "shared/helpers.h"

#include <linux/input.h>

struct pixman_output_state {
	void *shadow_buffer;
	pixman_image_t *shadow_image;
	pixman_image_t *hw_buffer;
	pixman_region32_t *hw_extra_damage;
};

struct pixman_surface_state {
	struct weston_surface *surface;

	pixman_image_t *image;
	struct weston_buffer_reference buffer_ref;
	struct weston_buffer_release_reference buffer_release_ref;

	struct wl_listener buffer_destroy_listener;
	struct wl_listener surface_destroy_listener;
	struct wl_listener renderer_destroy_listener;
};

struct pixman_renderer {
	struct weston_renderer base;

	int repaint_debug;
	pixman_image_t *debug_color;
	struct weston_binding *debug_binding;

	struct wl_signal destroy_signal;
};

static inline struct pixman_output_state *
get_output_state(struct weston_output *output)
{
	return (struct pixman_output_state *)output->renderer_state;
}

static int
pixman_renderer_create_surface(struct weston_surface *surface);

static inline struct pixman_surface_state *
get_surface_state(struct weston_surface *surface)
{
	if (!surface->renderer_state)
		pixman_renderer_create_surface(surface);

	return (struct pixman_surface_state *)surface->renderer_state;
}

static inline struct pixman_renderer *
get_renderer(struct weston_compositor *ec)
{
	return (struct pixman_renderer *)ec->renderer;
}

static int
pixman_renderer_read_pixels(struct weston_output *output,
			       pixman_format_code_t format, void *pixels,
			       uint32_t x, uint32_t y,
			       uint32_t width, uint32_t height)
{
	struct pixman_output_state *po = get_output_state(output);
	pixman_image_t *out_buf;

	if (!po->hw_buffer) {
		errno = ENODEV;
		return -1;
	}

	out_buf = pixman_image_create_bits(format,
		width,
		height,
		pixels,
		(PIXMAN_FORMAT_BPP(format) / 8) * width);

	pixman_image_composite32(PIXMAN_OP_SRC,
				 po->hw_buffer, /* src */
				 NULL /* mask */,
				 out_buf, /* dest */
				 x, y, /* src_x, src_y */
				 0, 0, /* mask_x, mask_y */
				 0, 0, /* dest_x, dest_y */
				 pixman_image_get_width (po->hw_buffer), /* width */
				 pixman_image_get_height (po->hw_buffer) /* height */);

	pixman_image_unref(out_buf);

	return 0;
}

static void
region_global_to_output(struct weston_output *output, pixman_region32_t *region)
{
	if (output->zoom.active) {
		weston_matrix_transform_region(region, &output->matrix, region);
	} else {
		pixman_region32_translate(region, -output->x, -output->y);
		weston_transformed_region(output->width, output->height,
					  output->transform,
					  output->current_scale,
					  region, region);
	}
}

#define D2F(v) pixman_double_to_fixed((double)v)

static void
weston_matrix_to_pixman_transform(pixman_transform_t *pt,
				  const struct weston_matrix *wm)
{
	/* Pixman supports only 2D transform matrix, but Weston uses 3D, *
	 * so we're omitting Z coordinate here. */
	pt->matrix[0][0] = pixman_double_to_fixed(wm->d[0]);
	pt->matrix[0][1] = pixman_double_to_fixed(wm->d[4]);
	pt->matrix[0][2] = pixman_double_to_fixed(wm->d[12]);
	pt->matrix[1][0] = pixman_double_to_fixed(wm->d[1]);
	pt->matrix[1][1] = pixman_double_to_fixed(wm->d[5]);
	pt->matrix[1][2] = pixman_double_to_fixed(wm->d[13]);
	pt->matrix[2][0] = pixman_double_to_fixed(wm->d[3]);
	pt->matrix[2][1] = pixman_double_to_fixed(wm->d[7]);
	pt->matrix[2][2] = pixman_double_to_fixed(wm->d[15]);
}

static void
pixman_renderer_compute_transform(pixman_transform_t *transform_out,
				  struct weston_view *ev,
				  struct weston_output *output)
{
	struct weston_matrix matrix;

	/* Set up the source transformation based on the surface
	   position, the output position/transform/scale and the client
	   specified buffer transform/scale */
	matrix = output->inverse_matrix;

	if (ev->transform.enabled) {
		weston_matrix_multiply(&matrix, &ev->transform.inverse);
	} else {
		weston_matrix_translate(&matrix,
					-ev->geometry.x, -ev->geometry.y, 0);
	}

	weston_matrix_multiply(&matrix, &ev->surface->surface_to_buffer_matrix);

	weston_matrix_to_pixman_transform(transform_out, &matrix);
}

static bool
view_transformation_is_translation(struct weston_view *view)
{
	if (!view->transform.enabled)
		return true;

	if (view->transform.matrix.type <= WESTON_MATRIX_TRANSFORM_TRANSLATE)
		return true;

	return false;
}

static void
region_intersect_only_translation(pixman_region32_t *result_global,
				  pixman_region32_t *global,
				  pixman_region32_t *surf,
				  struct weston_view *view)
{
	float view_x, view_y;

	assert(view_transformation_is_translation(view));

	/* Convert from surface to global coordinates */
	pixman_region32_copy(result_global, surf);
	weston_view_to_global_float(view, 0, 0, &view_x, &view_y);
	pixman_region32_translate(result_global, (int)view_x, (int)view_y);

	pixman_region32_intersect(result_global, result_global, global);
}

static void
composite_whole(pixman_op_t op,
		pixman_image_t *src,
		pixman_image_t *mask,
		pixman_image_t *dest,
		const pixman_transform_t *transform,
		pixman_filter_t filter)
{
	int32_t dest_width;
	int32_t dest_height;

	dest_width = pixman_image_get_width(dest);
	dest_height = pixman_image_get_height(dest);

	pixman_image_set_transform(src, transform);
	pixman_image_set_filter(src, filter, NULL, 0);

	/* bilinear filtering needs the equivalent of OpenGL CLAMP_TO_EDGE */
	if (filter == PIXMAN_FILTER_NEAREST)
		pixman_image_set_repeat(src, PIXMAN_REPEAT_NONE);
	else
		pixman_image_set_repeat(src, PIXMAN_REPEAT_PAD);

	pixman_image_composite32(op, src, mask, dest,
				 0, 0, /* src_x, src_y */
				 0, 0, /* mask_x, mask_y */
				 0, 0, /* dest_x, dest_y */
				 dest_width, dest_height);
}

static void
composite_clipped(pixman_image_t *src,
		  pixman_image_t *mask,
		  pixman_image_t *dest,
		  const pixman_transform_t *transform,
		  pixman_filter_t filter,
		  pixman_region32_t *src_clip)
{
	int n_box;
	pixman_box32_t *boxes;
	int32_t dest_width;
	int32_t dest_height;
	int src_stride;
	int bitspp;
	pixman_format_code_t src_format;
	void *src_data;
	int i;

	/*
	 * Hardcoded to use PIXMAN_OP_OVER, because sampling outside of
	 * a Pixman image produces (0,0,0,0) instead of discarding the
	 * fragment.
	 *
	 * Also repeat mode must be PIXMAN_REPEAT_NONE (the default) to
	 * actually sample (0,0,0,0). This may cause issues for clients that
	 * expect OpenGL CLAMP_TO_EDGE sampling behavior on their buffer.
	 * Using temporary 'boximg' it is not possible to apply CLAMP_TO_EDGE
	 * correctly with bilinear filter. Maybe trapezoid rendering could be
	 * the answer instead of source clip?
	 */

	dest_width = pixman_image_get_width(dest);
	dest_height = pixman_image_get_height(dest);
	src_format = pixman_image_get_format(src);
	src_stride = pixman_image_get_stride(src);
	bitspp = PIXMAN_FORMAT_BPP(src_format);
	src_data = pixman_image_get_data(src);

	assert(src_format);

	/* This would be massive overdraw, except when n_box is 1. */
	boxes = pixman_region32_rectangles(src_clip, &n_box);
	for (i = 0; i < n_box; i++) {
		uint8_t *ptr = src_data;
		pixman_image_t *boximg;
		pixman_transform_t adj = *transform;

		ptr += boxes[i].y1 * src_stride;
		ptr += boxes[i].x1 * bitspp / 8;
		boximg = pixman_image_create_bits_no_clear(src_format,
					boxes[i].x2 - boxes[i].x1,
					boxes[i].y2 - boxes[i].y1,
					(uint32_t *)ptr, src_stride);

		pixman_transform_translate(&adj, NULL,
					   pixman_int_to_fixed(-boxes[i].x1),
					   pixman_int_to_fixed(-boxes[i].y1));
		pixman_image_set_transform(boximg, &adj);

		pixman_image_set_filter(boximg, filter, NULL, 0);
		pixman_image_composite32(PIXMAN_OP_OVER, boximg, mask, dest,
					 0, 0, /* src_x, src_y */
					 0, 0, /* mask_x, mask_y */
					 0, 0, /* dest_x, dest_y */
					 dest_width, dest_height);

		pixman_image_unref(boximg);
	}

	if (n_box > 1) {
		static bool warned = false;

		if (!warned)
			weston_log("Pixman-renderer warning: %dx overdraw\n",
				   n_box);
		warned = true;
	}
}

/** Paint an intersected region
 *
 * \param ev The view to be painted.
 * \param output The output being painted.
 * \param repaint_output The region to be painted in output coordinates.
 * \param source_clip The region of the source image to use, in source image
 *                    coordinates. If NULL, use the whole source image.
 * \param pixman_op Compositing operator, either SRC or OVER.
 */
static void
repaint_region(struct weston_view *ev, struct weston_output *output,
	       pixman_region32_t *repaint_output,
	       pixman_region32_t *source_clip,
	       pixman_op_t pixman_op)
{
	struct pixman_renderer *pr =
		(struct pixman_renderer *) output->compositor->renderer;
	struct pixman_surface_state *ps = get_surface_state(ev->surface);
	struct pixman_output_state *po = get_output_state(output);
	struct weston_buffer_viewport *vp = &ev->surface->buffer_viewport;
	pixman_image_t *target_image;
	pixman_transform_t transform;
	pixman_filter_t filter;
	pixman_image_t *mask_image;
	pixman_color_t mask = { 0, };

	if (po->shadow_image)
		target_image = po->shadow_image;
	else
		target_image = po->hw_buffer;

 	/* Clip rendering to the damaged output region */
	pixman_image_set_clip_region32(target_image, repaint_output);

	pixman_renderer_compute_transform(&transform, ev, output);

	if (ev->transform.enabled || output->current_scale != vp->buffer.scale)
		filter = PIXMAN_FILTER_BILINEAR;
	else
		filter = PIXMAN_FILTER_NEAREST;

	if (ps->buffer_ref.buffer)
		wl_shm_buffer_begin_access(ps->buffer_ref.buffer->shm_buffer);

	if (ev->alpha < 1.0) {
		mask.alpha = 0xffff * ev->alpha;
		mask_image = pixman_image_create_solid_fill(&mask);
	} else {
		mask_image = NULL;
	}

	if (source_clip)
		composite_clipped(ps->image, mask_image, target_image,
				  &transform, filter, source_clip);
	else
		composite_whole(pixman_op, ps->image, mask_image,
				target_image, &transform, filter);

	if (mask_image)
		pixman_image_unref(mask_image);

	if (ps->buffer_ref.buffer)
		wl_shm_buffer_end_access(ps->buffer_ref.buffer->shm_buffer);

	if (pr->repaint_debug)
		pixman_image_composite32(PIXMAN_OP_OVER,
					 pr->debug_color, /* src */
					 NULL /* mask */,
					 target_image, /* dest */
					 0, 0, /* src_x, src_y */
					 0, 0, /* mask_x, mask_y */
					 0, 0, /* dest_x, dest_y */
					 pixman_image_get_width (target_image), /* width */
					 pixman_image_get_height (target_image) /* height */);

	pixman_image_set_clip_region32(target_image, NULL);
}

static void
draw_view_translated(struct weston_view *view, struct weston_output *output,
		     pixman_region32_t *repaint_global)
{
	struct weston_surface *surface = view->surface;
	/* non-opaque region in surface coordinates: */
	pixman_region32_t surface_blend;
	/* region to be painted in output coordinates: */
	pixman_region32_t repaint_output;

	pixman_region32_init(&repaint_output);

	/* Blended region is whole surface minus opaque region,
	 * unless surface alpha forces us to blend all.
	 */
	pixman_region32_init_rect(&surface_blend, 0, 0,
				  surface->width, surface->height);

	if (!(view->alpha < 1.0)) {
		pixman_region32_subtract(&surface_blend, &surface_blend,
					 &surface->opaque);

		if (pixman_region32_not_empty(&surface->opaque)) {
			region_intersect_only_translation(&repaint_output,
							  repaint_global,
							  &surface->opaque,
							  view);
			region_global_to_output(output, &repaint_output);

			repaint_region(view, output, &repaint_output, NULL,
				       PIXMAN_OP_SRC);
		}
	}

	if (pixman_region32_not_empty(&surface_blend)) {
		region_intersect_only_translation(&repaint_output,
						  repaint_global,
						  &surface_blend, view);
		region_global_to_output(output, &repaint_output);

		repaint_region(view, output, &repaint_output, NULL,
			       PIXMAN_OP_OVER);
	}

	pixman_region32_fini(&surface_blend);
	pixman_region32_fini(&repaint_output);
}

static void
draw_view_source_clipped(struct weston_view *view,
			 struct weston_output *output,
			 pixman_region32_t *repaint_global)
{
	struct weston_surface *surface = view->surface;
	pixman_region32_t surf_region;
	pixman_region32_t buffer_region;
	pixman_region32_t repaint_output;

	/* Do not bother separating the opaque region from non-opaque.
	 * Source clipping requires PIXMAN_OP_OVER in all cases, so painting
	 * opaque separately has no benefit.
	 */

	pixman_region32_init_rect(&surf_region, 0, 0,
				  surface->width, surface->height);
	if (view->geometry.scissor_enabled)
		pixman_region32_intersect(&surf_region, &surf_region,
					  &view->geometry.scissor);

	pixman_region32_init(&buffer_region);
	weston_surface_to_buffer_region(surface, &surf_region, &buffer_region);

	pixman_region32_init(&repaint_output);
	pixman_region32_copy(&repaint_output, repaint_global);
	region_global_to_output(output, &repaint_output);

	repaint_region(view, output, &repaint_output, &buffer_region,
		       PIXMAN_OP_OVER);

	pixman_region32_fini(&repaint_output);
	pixman_region32_fini(&buffer_region);
	pixman_region32_fini(&surf_region);
}

static void
draw_view(struct weston_view *ev, struct weston_output *output,
	  pixman_region32_t *damage) /* in global coordinates */
{
	struct pixman_surface_state *ps = get_surface_state(ev->surface);
	/* repaint bounding region in global coordinates: */
	pixman_region32_t repaint;

	/* No buffer attached */
	if (!ps->image)
		return;

	pixman_region32_init(&repaint);
	pixman_region32_intersect(&repaint,
				  &ev->transform.boundingbox, damage);
	pixman_region32_subtract(&repaint, &repaint, &ev->clip);

	if (!pixman_region32_not_empty(&repaint))
		goto out;

	if (view_transformation_is_translation(ev)) {
		/* The simple case: The surface regions opaque, non-opaque,
		 * etc. are convertible to global coordinate space.
		 * There is no need to use a source clip region.
		 * It is possible to paint opaque region as PIXMAN_OP_SRC.
		 * Also the boundingbox is accurate rather than an
		 * approximation.
		 */
		draw_view_translated(ev, output, &repaint);
	} else {
		/* The complex case: the view transformation does not allow
		 * converting opaque etc. regions into global coordinate space.
		 * Therefore we need source clipping to avoid sampling from
		 * unwanted source image areas, unless the source image is
		 * to be used whole. Source clipping does not work with
		 * PIXMAN_OP_SRC.
		 */
		draw_view_source_clipped(ev, output, &repaint);
	}

out:
	pixman_region32_fini(&repaint);
}
static void
repaint_surfaces(struct weston_output *output, pixman_region32_t *damage)
{
	struct weston_compositor *compositor = output->compositor;
	struct weston_view *view;

	wl_list_for_each_reverse(view, &compositor->view_list, link)
		if (view->plane == &compositor->primary_plane)
			draw_view(view, output, damage);
}

static void
copy_to_hw_buffer(struct weston_output *output, pixman_region32_t *region)
{
	struct pixman_output_state *po = get_output_state(output);
	pixman_region32_t output_region;

	pixman_region32_init(&output_region);
	pixman_region32_copy(&output_region, region);

	region_global_to_output(output, &output_region);

	pixman_image_set_clip_region32 (po->hw_buffer, &output_region);
	pixman_region32_fini(&output_region);

	pixman_image_composite32(PIXMAN_OP_SRC,
				 po->shadow_image, /* src */
				 NULL /* mask */,
				 po->hw_buffer, /* dest */
				 0, 0, /* src_x, src_y */
				 0, 0, /* mask_x, mask_y */
				 0, 0, /* dest_x, dest_y */
				 pixman_image_get_width (po->hw_buffer), /* width */
				 pixman_image_get_height (po->hw_buffer) /* height */);

	pixman_image_set_clip_region32 (po->hw_buffer, NULL);
}

static void
pixman_renderer_repaint_output(struct weston_output *output,
			       pixman_region32_t *output_damage)
{
	struct pixman_output_state *po = get_output_state(output);
	pixman_region32_t hw_damage;

	if (!po->hw_buffer) {
		po->hw_extra_damage = NULL;
 		return;
	}

	pixman_region32_init(&hw_damage);
	if (po->hw_extra_damage) {
		pixman_region32_union(&hw_damage,
				      po->hw_extra_damage, output_damage);
		po->hw_extra_damage = NULL;
	} else {
		pixman_region32_copy(&hw_damage, output_damage);
	}

	if (po->shadow_image) {
		repaint_surfaces(output, output_damage);
		copy_to_hw_buffer(output, &hw_damage);
	} else {
		repaint_surfaces(output, &hw_damage);
	}
	pixman_region32_fini(&hw_damage);

	wl_signal_emit(&output->frame_signal, output_damage);

	/* Actual flip should be done by caller */
}

static void
pixman_renderer_flush_damage(struct weston_surface *surface)
{
	/* No-op for pixman renderer */
}

static void
buffer_state_handle_buffer_destroy(struct wl_listener *listener, void *data)
{
	struct pixman_surface_state *ps;

	ps = container_of(listener, struct pixman_surface_state,
			  buffer_destroy_listener);

	if (ps->image) {
		pixman_image_unref(ps->image);
		ps->image = NULL;
	}

	ps->buffer_destroy_listener.notify = NULL;
}

static void
pixman_renderer_attach(struct weston_surface *es, struct weston_buffer *buffer)
{
	struct pixman_surface_state *ps = get_surface_state(es);
	struct wl_shm_buffer *shm_buffer;
	pixman_format_code_t pixman_format;

	weston_buffer_reference(&ps->buffer_ref, buffer);
	weston_buffer_release_reference(&ps->buffer_release_ref,
					es->buffer_release_ref.buffer_release);

	if (ps->buffer_destroy_listener.notify) {
		wl_list_remove(&ps->buffer_destroy_listener.link);
		ps->buffer_destroy_listener.notify = NULL;
	}

	if (ps->image) {
		pixman_image_unref(ps->image);
		ps->image = NULL;
	}

	if (!buffer)
		return;

	shm_buffer = wl_shm_buffer_get(buffer->resource);

	if (! shm_buffer) {
		weston_log("Pixman renderer supports only SHM buffers\n");
		weston_buffer_reference(&ps->buffer_ref, NULL);
		weston_buffer_release_reference(&ps->buffer_release_ref, NULL);
		return;
	}

	switch (wl_shm_buffer_get_format(shm_buffer)) {
	case WL_SHM_FORMAT_XRGB8888:
		pixman_format = PIXMAN_x8r8g8b8;
		es->is_opaque = true;
		break;
	case WL_SHM_FORMAT_ARGB8888:
		pixman_format = PIXMAN_a8r8g8b8;
		es->is_opaque = false;
		break;
	case WL_SHM_FORMAT_RGB565:
		pixman_format = PIXMAN_r5g6b5;
		es->is_opaque = true;
		break;
	default:
		weston_log("Unsupported SHM buffer format 0x%x\n",
			wl_shm_buffer_get_format(shm_buffer));
		weston_buffer_reference(&ps->buffer_ref, NULL);
		weston_buffer_release_reference(&ps->buffer_release_ref, NULL);
                weston_buffer_send_server_error(buffer,
			"disconnecting due to unhandled buffer type");
		return;
	break;
	}

	buffer->shm_buffer = shm_buffer;
	buffer->width = wl_shm_buffer_get_width(shm_buffer);
	buffer->height = wl_shm_buffer_get_height(shm_buffer);

	ps->image = pixman_image_create_bits(pixman_format,
		buffer->width, buffer->height,
		wl_shm_buffer_get_data(shm_buffer),
		wl_shm_buffer_get_stride(shm_buffer));

	ps->buffer_destroy_listener.notify =
		buffer_state_handle_buffer_destroy;
	wl_signal_add(&buffer->destroy_signal,
		      &ps->buffer_destroy_listener);
}

static void
pixman_renderer_surface_state_destroy(struct pixman_surface_state *ps)
{
	wl_list_remove(&ps->surface_destroy_listener.link);
	wl_list_remove(&ps->renderer_destroy_listener.link);
	if (ps->buffer_destroy_listener.notify) {
		wl_list_remove(&ps->buffer_destroy_listener.link);
		ps->buffer_destroy_listener.notify = NULL;
	}

	ps->surface->renderer_state = NULL;

	if (ps->image) {
		pixman_image_unref(ps->image);
		ps->image = NULL;
	}
	weston_buffer_reference(&ps->buffer_ref, NULL);
	weston_buffer_release_reference(&ps->buffer_release_ref, NULL);
	free(ps);
}

static void
surface_state_handle_surface_destroy(struct wl_listener *listener, void *data)
{
	struct pixman_surface_state *ps;

	ps = container_of(listener, struct pixman_surface_state,
			  surface_destroy_listener);

	pixman_renderer_surface_state_destroy(ps);
}

static void
surface_state_handle_renderer_destroy(struct wl_listener *listener, void *data)
{
	struct pixman_surface_state *ps;

	ps = container_of(listener, struct pixman_surface_state,
			  renderer_destroy_listener);

	pixman_renderer_surface_state_destroy(ps);
}

static int
pixman_renderer_create_surface(struct weston_surface *surface)
{
	struct pixman_surface_state *ps;
	struct pixman_renderer *pr = get_renderer(surface->compositor);

	ps = zalloc(sizeof *ps);
	if (ps == NULL)
		return -1;

	surface->renderer_state = ps;

	ps->surface = surface;

	ps->surface_destroy_listener.notify =
		surface_state_handle_surface_destroy;
	wl_signal_add(&surface->destroy_signal,
		      &ps->surface_destroy_listener);

	ps->renderer_destroy_listener.notify =
		surface_state_handle_renderer_destroy;
	wl_signal_add(&pr->destroy_signal,
		      &ps->renderer_destroy_listener);

	return 0;
}

static void
pixman_renderer_surface_set_color(struct weston_surface *es,
		 float red, float green, float blue, float alpha)
{
	struct pixman_surface_state *ps = get_surface_state(es);
	pixman_color_t color;

	color.red = red * 0xffff;
	color.green = green * 0xffff;
	color.blue = blue * 0xffff;
	color.alpha = alpha * 0xffff;

	if (ps->image) {
		pixman_image_unref(ps->image);
		ps->image = NULL;
	}

	ps->image = pixman_image_create_solid_fill(&color);
}

static void
pixman_renderer_destroy(struct weston_compositor *ec)
{
	struct pixman_renderer *pr = get_renderer(ec);

	wl_signal_emit(&pr->destroy_signal, pr);
	weston_binding_destroy(pr->debug_binding);
	free(pr);

	ec->renderer = NULL;
}

static void
pixman_renderer_surface_get_content_size(struct weston_surface *surface,
					 int *width, int *height)
{
	struct pixman_surface_state *ps = get_surface_state(surface);

	if (ps->image) {
		*width = pixman_image_get_width(ps->image);
		*height = pixman_image_get_height(ps->image);
	} else {
		*width = 0;
		*height = 0;
	}
}

static int
pixman_renderer_surface_copy_content(struct weston_surface *surface,
				     void *target, size_t size,
				     int src_x, int src_y,
				     int width, int height)
{
	const pixman_format_code_t format = PIXMAN_a8b8g8r8;
	const size_t bytespp = 4; /* PIXMAN_a8b8g8r8 */
	struct pixman_surface_state *ps = get_surface_state(surface);
	pixman_image_t *out_buf;

	if (!ps->image)
		return -1;

	out_buf = pixman_image_create_bits(format, width, height,
					   target, width * bytespp);

	pixman_image_set_transform(ps->image, NULL);
	pixman_image_composite32(PIXMAN_OP_SRC,
				 ps->image,    /* src */
				 NULL,         /* mask */
				 out_buf,      /* dest */
				 src_x, src_y, /* src_x, src_y */
				 0, 0,         /* mask_x, mask_y */
				 0, 0,         /* dest_x, dest_y */
				 width, height);

	pixman_image_unref(out_buf);

	return 0;
}

static void
debug_binding(struct weston_keyboard *keyboard, const struct timespec *time,
	      uint32_t key, void *data)
{
	struct weston_compositor *ec = data;
	struct pixman_renderer *pr = (struct pixman_renderer *) ec->renderer;

	pr->repaint_debug ^= 1;

	if (pr->repaint_debug) {
		pixman_color_t red = {
			0x3fff, 0x0000, 0x0000, 0x3fff
		};

		pr->debug_color = pixman_image_create_solid_fill(&red);
	} else {
		pixman_image_unref(pr->debug_color);
		weston_compositor_damage_all(ec);
	}
}

WL_EXPORT int
pixman_renderer_init(struct weston_compositor *ec)
{
	struct pixman_renderer *renderer;

	renderer = zalloc(sizeof *renderer);
	if (renderer == NULL)
		return -1;

	renderer->repaint_debug = 0;
	renderer->debug_color = NULL;
	renderer->base.read_pixels = pixman_renderer_read_pixels;
	renderer->base.repaint_output = pixman_renderer_repaint_output;
	renderer->base.flush_damage = pixman_renderer_flush_damage;
	renderer->base.attach = pixman_renderer_attach;
	renderer->base.surface_set_color = pixman_renderer_surface_set_color;
	renderer->base.destroy = pixman_renderer_destroy;
	renderer->base.surface_get_content_size =
		pixman_renderer_surface_get_content_size;
	renderer->base.surface_copy_content =
		pixman_renderer_surface_copy_content;
	ec->renderer = &renderer->base;
	ec->capabilities |= WESTON_CAP_ROTATION_ANY;
	ec->capabilities |= WESTON_CAP_VIEW_CLIP_MASK;

	renderer->debug_binding =
		weston_compositor_add_debug_binding(ec, KEY_R,
						    debug_binding, ec);

	wl_display_add_shm_format(ec->wl_display, WL_SHM_FORMAT_RGB565);

	wl_signal_init(&renderer->destroy_signal);

	return 0;
}

WL_EXPORT void
pixman_renderer_output_set_buffer(struct weston_output *output,
				  pixman_image_t *buffer)
{
	struct pixman_output_state *po = get_output_state(output);

	if (po->hw_buffer)
		pixman_image_unref(po->hw_buffer);
	po->hw_buffer = buffer;

	if (po->hw_buffer) {
		output->compositor->read_format = pixman_image_get_format(po->hw_buffer);
		pixman_image_ref(po->hw_buffer);
	}
}

WL_EXPORT void
pixman_renderer_output_set_hw_extra_damage(struct weston_output *output,
					   pixman_region32_t *extra_damage)
{
	struct pixman_output_state *po = get_output_state(output);

	po->hw_extra_damage = extra_damage;
}

WL_EXPORT int
pixman_renderer_output_create(struct weston_output *output,
			      const struct pixman_renderer_output_options *options)
{
	struct pixman_output_state *po;
	int w, h;

	po = zalloc(sizeof *po);
	if (po == NULL)
		return -1;

	if (options->use_shadow) {
		/* set shadow image transformation */
		w = output->current_mode->width;
		h = output->current_mode->height;

		po->shadow_buffer = malloc(w * h * 4);

		if (!po->shadow_buffer) {
			free(po);
			return -1;
		}

		po->shadow_image =
			pixman_image_create_bits(PIXMAN_x8r8g8b8, w, h,
						 po->shadow_buffer, w * 4);

		if (!po->shadow_image) {
			free(po->shadow_buffer);
			free(po);
			return -1;
		}
	}

	output->renderer_state = po;

	return 0;
}

WL_EXPORT void
pixman_renderer_output_destroy(struct weston_output *output)
{
	struct pixman_output_state *po = get_output_state(output);

	if (po->shadow_image)
		pixman_image_unref(po->shadow_image);

	if (po->hw_buffer)
		pixman_image_unref(po->hw_buffer);

	free(po->shadow_buffer);

	po->shadow_buffer = NULL;
	po->shadow_image = NULL;
	po->hw_buffer = NULL;

	free(po);
}
