/*
 * Copyright (C) 2013 DENSO CORPORATION
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

/**
 * Implementation of ivi-layout library. The actual view on ivi_screen is
 * not updated until ivi_layout_commit_changes is called. An overview from
 * calling API for updating properties of ivi_surface/ivi_layer to asking
 * compositor to compose them by using weston_view_schedule_repaint,
 * 0/ initialize this library by ivi_layout_init
 *    with (struct weston_compositor *ec) from ivi-shell.
 * 1/ When an API for updating properties of ivi_surface/ivi_layer, it updates
 *    pending prop of ivi_surface/ivi_layer/ivi_screen which are structure to
 *    store properties.
 * 2/ Before calling commitChanges, in case of calling an API to get a property,
 *    return current property, not pending property.
 * 3/ At the timing of calling ivi_layout_commitChanges, pending properties
 *    are applied to properties.
 *
 *    *) ivi_layout_commitChanges is also called by transition animation
 *    per each frame. See ivi-layout-transition.c in details. Transition
 *    animation interpolates frames between previous properties of ivi_surface
 *    and new ones.
 *    For example, when a property of ivi_surface is changed from invisible
 *    to visible, it behaves like fade-in. When ivi_layout_commitChange is
 *    called during transition animation, it cancels the transition and
 *    re-start transition to new properties from current properties of final
 *    frame just before the cancellation.
 *
 * 4/ According properties, set transformation by using weston_matrix and
 *    weston_view per ivi_surfaces and ivi_layers in while loop.
 * 5/ Set damage and trigger transform by using weston_view_geometry_dirty.
 * 6/ Schedule repaint for each view by using weston_view_schedule_repaint.
 * 7/ Notify update of properties.
 *
 */
#include "config.h"

#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "compositor/weston.h"
#include <libweston/libweston.h>
#include "ivi-shell.h"
#include "ivi-layout-export.h"
#include "ivi-layout-private.h"
#include "ivi-layout-shell.h"

#include "shared/helpers.h"
#include "shared/os-compatibility.h"
#include "shared/signal.h"
#include "shared/xalloc.h"

#define max(a, b) ((a) > (b) ? (a) : (b))

struct ivi_layout;

struct ivi_layout_screen {
	struct wl_list link;	/* ivi_layout::screen_list */

	struct ivi_layout *layout;
	struct weston_output *output;

	struct {
		struct wl_list layer_list;	/* ivi_layout_layer::pending.link */
	} pending;

	struct {
		int dirty;
		struct wl_list layer_list;	/* ivi_layout_layer::order.link */
	} order;
};

struct ivi_rectangle
{
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
};

static struct ivi_layout ivilayout = {0};

struct ivi_layout *
get_instance(void)
{
	return &ivilayout;
}

/**
 * Internal API to add/remove an ivi_layer to/from ivi_screen.
 */
static struct ivi_layout_surface *
get_surface(struct wl_list *surf_list, uint32_t id_surface)
{
	struct ivi_layout_surface *ivisurf;

	wl_list_for_each(ivisurf, surf_list, link) {
		if (ivisurf->id_surface == id_surface) {
			return ivisurf;
		}
	}

	return NULL;
}

static struct ivi_layout_layer *
get_layer(struct wl_list *layer_list, uint32_t id_layer)
{
	struct ivi_layout_layer *ivilayer;

	wl_list_for_each(ivilayer, layer_list, link) {
		if (ivilayer->id_layer == id_layer) {
			return ivilayer;
		}
	}

	return NULL;
}

static bool
ivi_view_is_rendered(struct ivi_layout_view *view)
{
	return !wl_list_empty(&view->order_link);
}

static void
ivi_view_destroy(struct ivi_layout_view *ivi_view)
{
	wl_list_remove(&ivi_view->transform.link);
	wl_list_remove(&ivi_view->link);
	wl_list_remove(&ivi_view->surf_link);
	wl_list_remove(&ivi_view->pending_link);
	wl_list_remove(&ivi_view->order_link);

	if (weston_surface_is_desktop_surface(ivi_view->ivisurf->surface))
		weston_desktop_surface_unlink_view(ivi_view->view);
	weston_view_destroy(ivi_view->view);

	free(ivi_view);
}

static struct ivi_layout_view*
ivi_view_create(struct ivi_layout_layer *ivilayer,
		struct ivi_layout_surface *ivisurf)
{
	struct ivi_layout_view *ivi_view;

	ivi_view = xzalloc(sizeof *ivi_view);

	if (weston_surface_is_desktop_surface(ivisurf->surface)) {
		ivi_view->view = weston_desktop_surface_create_view(
				ivisurf->weston_desktop_surface);
	} else {
		ivi_view->view = weston_view_create(ivisurf->surface);
	}

	if (ivi_view->view == NULL) {
		weston_log("fails to allocate memory\n");
		free(ivi_view);
		return NULL;
	}

	ivisurf->ivi_view = ivi_view;

	weston_matrix_init(&ivi_view->transform.matrix);
	wl_list_init(&ivi_view->transform.link);

	ivi_view->ivisurf = ivisurf;
	ivi_view->on_layer = ivilayer;
	wl_list_insert(&ivilayer->layout->view_list,
		       &ivi_view->link);
	wl_list_insert(&ivisurf->view_list,
		       &ivi_view->surf_link);

	wl_list_init(&ivi_view->pending_link);
	wl_list_init(&ivi_view->order_link);

	return ivi_view;
}

static struct ivi_layout_view *
get_ivi_view(struct ivi_layout_layer *ivilayer,
		struct ivi_layout_surface *ivisurf)
{
	struct ivi_layout_view *ivi_view;

	assert(ivisurf->surface != NULL);

	wl_list_for_each(ivi_view, &ivisurf->view_list, surf_link) {
		if (ivi_view->on_layer == ivilayer)
			return ivi_view;
	}

	return NULL;
}

static struct ivi_layout_screen *
get_screen_from_output(struct weston_output *output)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_screen *iviscrn = NULL;

	wl_list_for_each(iviscrn, &layout->screen_list, link) {
		if (iviscrn->output == output)
			return iviscrn;
	}

	return NULL;
}

/**
 * Called at destruction of wl_surface/ivi_surface
 */
void
ivi_layout_surface_destroy(struct ivi_layout_surface *ivisurf)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_view *ivi_view ,*next;

	if (ivisurf == NULL) {
		weston_log("%s: invalid argument\n", __func__);
		return;
	}

	wl_list_remove(&ivisurf->link);

	wl_list_for_each_safe(ivi_view, next, &ivisurf->view_list, surf_link) {
		ivi_view_destroy(ivi_view);
	}

	wl_signal_emit(&layout->surface_notification.removed, ivisurf);

	ivi_layout_remove_all_surface_transitions(ivisurf);

	free(ivisurf);
}

static void
destroy_screen(struct ivi_layout_screen *iviscrn)
{
	struct ivi_layout_layer *ivilayer, *layer_next;

	wl_list_for_each_safe(ivilayer, layer_next,
			      &iviscrn->pending.layer_list, pending.link) {
		wl_list_remove(&ivilayer->pending.link);
		wl_list_init(&ivilayer->pending.link);
	}

	assert(wl_list_empty(&iviscrn->pending.layer_list));

	wl_list_for_each_safe(ivilayer, layer_next,
			      &iviscrn->order.layer_list, order.link) {
		wl_list_remove(&ivilayer->order.link);
		wl_list_init(&ivilayer->order.link);
		ivilayer->on_screen = NULL;
	}

	assert(wl_list_empty(&iviscrn->order.layer_list));

	wl_list_remove(&iviscrn->link);
	free(iviscrn);
}

static void
output_destroyed_event(struct wl_listener *listener, void *data)
{
	struct weston_output *destroyed_output = data;
	struct ivi_layout_screen *iviscrn;

	iviscrn = get_screen_from_output(destroyed_output);
	assert(iviscrn != NULL);
	destroy_screen(iviscrn);

}

static void
add_screen(struct weston_output *output)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_screen *iviscrn = NULL;

	iviscrn = xzalloc(sizeof *iviscrn);

	iviscrn->layout = layout;
	iviscrn->output = output;

	wl_list_init(&iviscrn->pending.layer_list);
	wl_list_init(&iviscrn->order.layer_list);
	wl_list_insert(&layout->screen_list, &iviscrn->link);
}

static void
output_created_event(struct wl_listener *listener, void *data)
{
	struct weston_output *created_output = data;
	add_screen(created_output);
}

/**
 * Internal API to initialize ivi_screens found from output_list of weston_compositor.
 * Called by ivi_layout_init.
 */
static void
create_screen(struct weston_compositor *ec)
{
	struct weston_output *output = NULL;

	wl_list_for_each(output, &ec->output_list, link)
		add_screen(output);
}

/**
 * Internal APIs to initialize properties of ivi_surface/ivi_layer when they are created.
 */
static void
init_layer_properties(struct ivi_layout_layer_properties *prop,
		      int32_t width, int32_t height)
{
	memset(prop, 0, sizeof *prop);
	prop->opacity = wl_fixed_from_double(1.0);
	prop->source_width = width;
	prop->source_height = height;
	prop->dest_width = width;
	prop->dest_height = height;
}

static void
init_surface_properties(struct ivi_layout_surface_properties *prop)
{
	memset(prop, 0, sizeof *prop);
	prop->opacity = wl_fixed_from_double(1.0);
	/*
	 * FIXME: this shall be fixed by ivi-layout-transition.
	 */
	prop->dest_width = 1;
	prop->dest_height = 1;
}

/**
 * Internal APIs to be called from ivi_layout_commit_changes.
 */
static void
update_opacity(struct ivi_layout_layer *ivilayer,
	       struct ivi_layout_surface *ivisurf,
	       struct weston_view *view)
{
	double layer_alpha = wl_fixed_to_double(ivilayer->prop.opacity);
	double surf_alpha  = wl_fixed_to_double(ivisurf->prop.opacity);

	view->alpha = layer_alpha * surf_alpha;
}

static void
calc_transformation_matrix(struct ivi_rectangle *source_rect,
			   struct ivi_rectangle *dest_rect,
			   struct weston_matrix *m)
{
	float source_center_x;
	float source_center_y;
	float scale_x;
	float scale_y;
	float translate_x;
	float translate_y;

	source_center_x = source_rect->x + source_rect->width * 0.5f;
	source_center_y = source_rect->y + source_rect->height * 0.5f;
	weston_matrix_translate(m, -source_center_x, -source_center_y, 0.0f);

	if ((dest_rect->width != source_rect->width) ||
	    (dest_rect->height != source_rect->height))
	{
		scale_x = (float) dest_rect->width / (float) source_rect->width;
		scale_y = (float) dest_rect->height / (float) source_rect->height;
		weston_matrix_scale(m, scale_x, scale_y, 1.0f);
	}

	translate_x = dest_rect->width * 0.5f + dest_rect->x;
	translate_y = dest_rect->height * 0.5f + dest_rect->y;
	weston_matrix_translate(m, translate_x, translate_y, 0.0f);
}

/*
 * This computes intersected rect_output from two ivi_rectangles
 */
static void
ivi_rectangle_intersect(const struct ivi_rectangle *rect1,
		        const struct ivi_rectangle *rect2,
		        struct ivi_rectangle *rect_output)
{
	int32_t rect1_right = rect1->x + rect1->width;
	int32_t rect1_bottom = rect1->y + rect1->height;
	int32_t rect2_right = rect2->x + rect2->width;
	int32_t rect2_bottom = rect2->y + rect2->height;

	rect_output->x = max(rect1->x, rect2->x);
	rect_output->y = max(rect1->y, rect2->y);
	rect_output->width = rect1_right < rect2_right ?
			     rect1_right - rect_output->x :
			     rect2_right - rect_output->x;
	rect_output->height = rect1_bottom < rect2_bottom ?
			      rect1_bottom - rect_output->y :
			      rect2_bottom - rect_output->y;

	if (rect_output->width < 0 || rect_output->height < 0) {
		rect_output->width = 0;
		rect_output->height = 0;
	}
}

/*
 * Transform rect_input by the inverse of matrix, intersect with boundingbox,
 * and store the result in rect_output.
 * The boundingbox must be given in the same coordinate space as rect_output.
 * Additionally, there are the following restrictions on the matrix:
 * - no projective transformations
 * - no skew
 * - only multiples of 90-degree rotations supported
 *
 * In failure case of weston_matrix_invert, rect_output is set to boundingbox
 * as a fail-safe with log.
 */
static void
calc_inverse_matrix_transform(const struct weston_matrix *matrix,
			      const struct ivi_rectangle *rect_input,
			      const struct ivi_rectangle *boundingbox,
			      struct ivi_rectangle *rect_output)
{
	struct weston_matrix m;
	struct weston_vector top_left;
	struct weston_vector bottom_right;

	assert(boundingbox != rect_output);

	if (weston_matrix_invert(&m, matrix) < 0) {
		weston_log("ivi-shell: calc_inverse_matrix_transform fails to invert a matrix.\n");
		weston_log("ivi-shell: boundingbox is set to the rect_output.\n");
		rect_output->x = boundingbox->x;
		rect_output->y = boundingbox->y;
		rect_output->width = boundingbox->width;
		rect_output->height = boundingbox->height;
	}

	/* The vectors and matrices involved will always produce f[3] == 1.0. */
	top_left.f[0] = rect_input->x;
	top_left.f[1] = rect_input->y;
	top_left.f[2] = 0.0f;
	top_left.f[3] = 1.0f;

	bottom_right.f[0] = rect_input->x + rect_input->width;
	bottom_right.f[1] = rect_input->y + rect_input->height;
	bottom_right.f[2] = 0.0f;
	bottom_right.f[3] = 1.0f;

	weston_matrix_transform(&m, &top_left);
	weston_matrix_transform(&m, &bottom_right);

	if (top_left.f[0] < bottom_right.f[0]) {
		rect_output->x = floorf(top_left.f[0]);
		rect_output->width = ceilf(bottom_right.f[0] - rect_output->x);
	} else {
		rect_output->x = floorf(bottom_right.f[0]);
		rect_output->width = ceilf(top_left.f[0] - rect_output->x);
	}

	if (top_left.f[1] < bottom_right.f[1]) {
		rect_output->y = floorf(top_left.f[1]);
		rect_output->height = ceilf(bottom_right.f[1] - rect_output->y);
	} else {
		rect_output->y = floorf(bottom_right.f[1]);
		rect_output->height = ceilf(top_left.f[1] - rect_output->y);
	}

	ivi_rectangle_intersect(rect_output, boundingbox, rect_output);
}

/**
 * This computes the whole transformation matrix:m from surface-local
 * coordinates to multi-screen coordinates, which are global coordinates.
 * It is assumed that weston_view::geometry.{x,y} are zero.
 *
 * Additionally, this computes the mask on surface-local coordinates as an
 * ivi_rectangle. This can be set to weston_view_set_mask.
 *
 * The mask is computed by following steps
 * - destination rectangle of layer is transformed to multi-screen coordinates,
 *   global coordinates. This is done by adding weston_output.{x,y} in simple
 *   because there is no scaled and rotated transformation.
 * - destination rectangle of layer in multi-screen coordinates needs to be
 *   intersected inside of a screen the layer is assigned to. This is because
 *   overlapped region of weston surface in another screen shall not be
 *   displayed according to ivi use case.
 * - destination rectangle of layer
 *   - in multi-screen coordinates,
 *   - and intersected inside of an assigned screen,
 *   is inversed to surface-local coordinates by inversed matrix:m.
 * - the area is intersected by intersected area between weston_surface and
 *   source rectangle of ivi_surface.
 */
static void
calc_surface_to_global_matrix_and_mask_to_weston_surface(
	struct ivi_layout_screen  *iviscrn,
	struct ivi_layout_layer *ivilayer,
	struct ivi_layout_surface *ivisurf,
	struct weston_matrix *m,
	struct ivi_rectangle *result)
{
	const struct ivi_layout_surface_properties *sp = &ivisurf->prop;
	const struct ivi_layout_layer_properties *lp = &ivilayer->prop;
	struct weston_output *output = iviscrn->output;
	struct ivi_rectangle surface_source_rect = { sp->source_x,
						     sp->source_y,
						     sp->source_width,
						     sp->source_height };
	struct ivi_rectangle surface_dest_rect =   { sp->dest_x,
						     sp->dest_y,
						     sp->dest_width,
						     sp->dest_height };
	struct ivi_rectangle layer_source_rect =   { lp->source_x,
						     lp->source_y,
						     lp->source_width,
						     lp->source_height };
	struct ivi_rectangle layer_dest_rect =     { lp->dest_x,
						     lp->dest_y,
						     lp->dest_width,
						     lp->dest_height };
	struct ivi_rectangle screen_dest_rect =    { output->x,
						     output->y,
						     output->width,
						     output->height };
	struct ivi_rectangle layer_dest_rect_in_global =
						   { lp->dest_x + output->x,
						     lp->dest_y + output->y,
						     lp->dest_width,
						     lp->dest_height };
	struct ivi_rectangle layer_dest_rect_in_global_intersected;

	/*
	 * the whole transformation matrix:m from surface-local
	 * coordinates to global coordinates, which is computed by
	 * two steps,
	 * - surface-local coordinates to layer-local coordinates
	 * - layer-local coordinates to single screen-local coordinates
	 * - single screen-local coordinates to multi-screen coordinates,
	 *   which are global coordinates.
	 */
	calc_transformation_matrix(&surface_source_rect, &surface_dest_rect, m);
	calc_transformation_matrix(&layer_source_rect, &layer_dest_rect, m);

	weston_matrix_translate(m, output->x, output->y, 0.0f);

	/*
	 * destination rectangle of layer in multi screens coordinate
	 * is intersected to avoid displaying outside of an assigned screen.
	 */
	ivi_rectangle_intersect(&layer_dest_rect_in_global, &screen_dest_rect,
				&layer_dest_rect_in_global_intersected);

	/* calc masking area of weston_surface from m */
	calc_inverse_matrix_transform(m,
				      &layer_dest_rect_in_global_intersected,
				      &surface_source_rect,
				      result);
}

static void
update_prop(struct ivi_layout_view *ivi_view)
{
	struct ivi_layout_surface *ivisurf = ivi_view->ivisurf;
	struct ivi_layout_layer *ivilayer = ivi_view->on_layer;
	struct ivi_layout_screen *iviscrn = ivilayer->on_screen;
	struct ivi_rectangle r;
	bool can_calc = true;

	/*In case of no prop change, this just returns*/
	if (!ivilayer->prop.event_mask && !ivisurf->prop.event_mask)
		return;

	update_opacity(ivilayer, ivisurf, ivi_view->view);

	if (ivisurf->prop.source_width == 0 || ivisurf->prop.source_height == 0) {
		weston_log("ivi-shell: source rectangle is not yet set by ivi_layout_surface_set_source_rectangle\n");
		can_calc = false;
	}

	if (ivisurf->prop.dest_width == 0 || ivisurf->prop.dest_height == 0) {
		weston_log("ivi-shell: destination rectangle is not yet set by ivi_layout_surface_set_destination_rectangle\n");
		can_calc = false;
	}

	if (can_calc) {
		wl_list_remove(&ivi_view->transform.link);
		weston_matrix_init(&ivi_view->transform.matrix);

		calc_surface_to_global_matrix_and_mask_to_weston_surface(
			iviscrn, ivilayer, ivisurf, &ivi_view->transform.matrix, &r);

		weston_view_set_mask(ivi_view->view, r.x, r.y, r.width, r.height);
		wl_list_insert(&ivi_view->view->geometry.transformation_list,
			       &ivi_view->transform.link);

		weston_view_set_transform_parent(ivi_view->view, NULL);
		weston_view_geometry_dirty(ivi_view->view);
		weston_view_update_transform(ivi_view->view);
	}

	ivisurf->update_count++;

	weston_view_schedule_repaint(ivi_view->view);
}

static bool
ivi_view_is_mapped(struct ivi_layout_view *ivi_view)
{
	return (!wl_list_empty(&ivi_view->order_link) &&
		ivi_view->on_layer->on_screen &&
		ivi_view->on_layer->prop.visibility &&
		ivi_view->ivisurf->prop.visibility);
}

static void
commit_changes(struct ivi_layout *layout)
{
	struct ivi_layout_view *ivi_view  = NULL;

	wl_list_for_each(ivi_view, &layout->view_list, link) {
		/*
		 * If the view is not on the currently rendered scenegraph,
		 * we do not need to update its properties.
		 */
		if (!ivi_view_is_mapped(ivi_view))
			continue;

		update_prop(ivi_view);
	}
}

static void
commit_surface_list(struct ivi_layout *layout)
{
	struct ivi_layout_surface *ivisurf = NULL;
	int32_t dest_x = 0;
	int32_t dest_y = 0;
	int32_t dest_width = 0;
	int32_t dest_height = 0;
	int32_t configured = 0;

	wl_list_for_each(ivisurf, &layout->surface_list, link) {
		if (ivisurf->pending.prop.transition_type == IVI_LAYOUT_TRANSITION_VIEW_DEFAULT) {
			dest_x = ivisurf->prop.dest_x;
			dest_y = ivisurf->prop.dest_y;
			dest_width = ivisurf->prop.dest_width;
			dest_height = ivisurf->prop.dest_height;

			ivi_layout_transition_move_resize_view(ivisurf,
							       ivisurf->pending.prop.dest_x,
							       ivisurf->pending.prop.dest_y,
							       ivisurf->pending.prop.dest_width,
							       ivisurf->pending.prop.dest_height,
							       ivisurf->pending.prop.transition_duration);

			if (ivisurf->pending.prop.visibility) {
				ivi_layout_transition_visibility_on(ivisurf, ivisurf->pending.prop.transition_duration);
			} else {
				ivi_layout_transition_visibility_off(ivisurf, ivisurf->pending.prop.transition_duration);
			}

			ivisurf->prop = ivisurf->pending.prop;
			ivisurf->prop.dest_x = dest_x;
			ivisurf->prop.dest_y = dest_y;
			ivisurf->prop.dest_width = dest_width;
			ivisurf->prop.dest_height = dest_height;
			ivisurf->prop.transition_type = IVI_LAYOUT_TRANSITION_NONE;
			ivisurf->pending.prop.transition_type = IVI_LAYOUT_TRANSITION_NONE;

		} else if (ivisurf->pending.prop.transition_type == IVI_LAYOUT_TRANSITION_VIEW_DEST_RECT_ONLY) {
			dest_x = ivisurf->prop.dest_x;
			dest_y = ivisurf->prop.dest_y;
			dest_width = ivisurf->prop.dest_width;
			dest_height = ivisurf->prop.dest_height;

			ivi_layout_transition_move_resize_view(ivisurf,
							       ivisurf->pending.prop.dest_x,
							       ivisurf->pending.prop.dest_y,
							       ivisurf->pending.prop.dest_width,
							       ivisurf->pending.prop.dest_height,
							       ivisurf->pending.prop.transition_duration);

			ivisurf->prop = ivisurf->pending.prop;
			ivisurf->prop.dest_x = dest_x;
			ivisurf->prop.dest_y = dest_y;
			ivisurf->prop.dest_width = dest_width;
			ivisurf->prop.dest_height = dest_height;

			ivisurf->prop.transition_type = IVI_LAYOUT_TRANSITION_NONE;
			ivisurf->pending.prop.transition_type = IVI_LAYOUT_TRANSITION_NONE;

		} else if (ivisurf->pending.prop.transition_type == IVI_LAYOUT_TRANSITION_VIEW_FADE_ONLY) {
			configured = 0;
			if (ivisurf->pending.prop.visibility) {
				ivi_layout_transition_visibility_on(ivisurf, ivisurf->pending.prop.transition_duration);
			} else {
				ivi_layout_transition_visibility_off(ivisurf, ivisurf->pending.prop.transition_duration);
			}

			if (ivisurf->prop.dest_width  != ivisurf->pending.prop.dest_width ||
			    ivisurf->prop.dest_height != ivisurf->pending.prop.dest_height) {
				configured = 1;
			}

			ivisurf->prop = ivisurf->pending.prop;
			ivisurf->prop.transition_type = IVI_LAYOUT_TRANSITION_NONE;
			ivisurf->pending.prop.transition_type = IVI_LAYOUT_TRANSITION_NONE;

			if (configured && !is_surface_transition(ivisurf)) {
				ivi_layout_surface_set_size(ivisurf,
							    ivisurf->prop.dest_width,
							    ivisurf->prop.dest_height);
			}
		} else {
			configured = 0;
			if (ivisurf->prop.dest_width  != ivisurf->pending.prop.dest_width ||
			    ivisurf->prop.dest_height != ivisurf->pending.prop.dest_height) {
				configured = 1;
			}

			ivisurf->prop = ivisurf->pending.prop;
			ivisurf->prop.transition_type = IVI_LAYOUT_TRANSITION_NONE;
			ivisurf->pending.prop.transition_type = IVI_LAYOUT_TRANSITION_NONE;

			if (configured && !is_surface_transition(ivisurf)) {
				ivi_layout_surface_set_size(ivisurf,
							    ivisurf->prop.dest_width,
							    ivisurf->prop.dest_height);
			}
		}
	}
}

static void
commit_layer_list(struct ivi_layout *layout)
{
	struct ivi_layout_view *ivi_view = NULL;
	struct ivi_layout_layer   *ivilayer = NULL;
	struct ivi_layout_view *next     = NULL;

	wl_list_for_each(ivilayer, &layout->layer_list, link) {
		if (ivilayer->pending.prop.transition_type == IVI_LAYOUT_TRANSITION_LAYER_MOVE) {
			ivi_layout_transition_move_layer(ivilayer, ivilayer->pending.prop.dest_x, ivilayer->pending.prop.dest_y, ivilayer->pending.prop.transition_duration);
		} else if (ivilayer->pending.prop.transition_type == IVI_LAYOUT_TRANSITION_LAYER_FADE) {
			ivi_layout_transition_fade_layer(ivilayer,ivilayer->pending.prop.is_fade_in,
							 ivilayer->pending.prop.start_alpha,ivilayer->pending.prop.end_alpha,
							 NULL, NULL,
							 ivilayer->pending.prop.transition_duration);
		}
		ivilayer->pending.prop.transition_type = IVI_LAYOUT_TRANSITION_NONE;

		ivilayer->prop = ivilayer->pending.prop;

		if (!ivilayer->order.dirty) {
			continue;
		}

		wl_list_for_each_safe(ivi_view, next, &ivilayer->order.view_list,
					 order_link) {
			wl_list_remove(&ivi_view->order_link);
			wl_list_init(&ivi_view->order_link);
			ivi_view->ivisurf->prop.event_mask |= IVI_NOTIFICATION_REMOVE;
		}

		assert(wl_list_empty(&ivilayer->order.view_list));

		wl_list_for_each(ivi_view, &ivilayer->pending.view_list,
					 pending_link) {
			wl_list_remove(&ivi_view->order_link);
			wl_list_insert(&ivilayer->order.view_list, &ivi_view->order_link);
			ivi_view->ivisurf->prop.event_mask |= IVI_NOTIFICATION_ADD;
		}

		ivilayer->order.dirty = 0;
	}
}

static void
commit_screen_list(struct ivi_layout *layout)
{
	struct ivi_layout_screen  *iviscrn  = NULL;
	struct ivi_layout_layer   *ivilayer = NULL;
	struct ivi_layout_layer   *next     = NULL;

	wl_list_for_each(iviscrn, &layout->screen_list, link) {
		if (iviscrn->order.dirty) {
			wl_list_for_each_safe(ivilayer, next,
					      &iviscrn->order.layer_list, order.link) {
				ivilayer->on_screen = NULL;
				wl_list_remove(&ivilayer->order.link);
				wl_list_init(&ivilayer->order.link);
				ivilayer->prop.event_mask |= IVI_NOTIFICATION_REMOVE;
			}

			assert(wl_list_empty(&iviscrn->order.layer_list));

			wl_list_for_each(ivilayer, &iviscrn->pending.layer_list,
					 pending.link) {
				/* FIXME: avoid to insert order.link to multiple screens */
				wl_list_remove(&ivilayer->order.link);

				wl_list_insert(&iviscrn->order.layer_list,
					       &ivilayer->order.link);
				ivilayer->on_screen = iviscrn;
				ivilayer->prop.event_mask |= IVI_NOTIFICATION_ADD;
			}

			iviscrn->order.dirty = 0;
		}
	}
}

static void
build_view_list(struct ivi_layout *layout)
{
	struct ivi_layout_screen  *iviscrn;
	struct ivi_layout_layer   *ivilayer;
	struct ivi_layout_view   *ivi_view;

	/* If ivi_view is not part of the scenegrapgh, we have to unmap
	 * weston_views
	 */
	wl_list_for_each(ivi_view, &layout->view_list, link) {
		if (!ivi_view_is_mapped(ivi_view))
			weston_view_unmap(ivi_view->view);
	}

	/* Clear view list of layout ivi_layer */
	wl_list_init(&layout->layout_layer.view_list.link);

	wl_list_for_each(iviscrn, &layout->screen_list, link) {
		wl_list_for_each(ivilayer, &iviscrn->order.layer_list, order.link) {
			if (ivilayer->prop.visibility == false)
				continue;

			wl_list_for_each(ivi_view, &ivilayer->order.view_list, order_link) {
				if (ivi_view->ivisurf->prop.visibility == false)
					continue;

				weston_layer_entry_insert(&layout->layout_layer.view_list,
							  &ivi_view->view->layer_link);

				weston_surface_map(ivi_view->ivisurf->surface);
				ivi_view->view->is_mapped = true;
			}
		}
	}
}

static void
commit_transition(struct ivi_layout* layout)
{
	if (wl_list_empty(&layout->pending_transition_list)) {
		return;
	}

	wl_list_insert_list(&layout->transitions->transition_list,
			    &layout->pending_transition_list);

	wl_list_init(&layout->pending_transition_list);

	wl_event_source_timer_update(layout->transitions->event_source, 1);
}

static void
send_surface_prop(struct ivi_layout_surface *ivisurf)
{
	wl_signal_emit(&ivisurf->property_changed, ivisurf);
	ivisurf->pending.prop.event_mask = 0;
}

static void
send_layer_prop(struct ivi_layout_layer *ivilayer)
{
	wl_signal_emit(&ivilayer->property_changed, ivilayer);
	ivilayer->pending.prop.event_mask = 0;
}

static void
send_prop(struct ivi_layout *layout)
{
	struct ivi_layout_layer   *ivilayer = NULL;
	struct ivi_layout_surface *ivisurf  = NULL;

	wl_list_for_each_reverse(ivilayer, &layout->layer_list, link) {
		if (ivilayer->prop.event_mask)
			send_layer_prop(ivilayer);
	}

	wl_list_for_each_reverse(ivisurf, &layout->surface_list, link) {
		if (ivisurf->prop.event_mask)
			send_surface_prop(ivisurf);
	}
}

static void
clear_view_pending_list(struct ivi_layout_layer *ivilayer)
{
	struct ivi_layout_view *view_link = NULL;
	struct ivi_layout_view *view_next = NULL;

	wl_list_for_each_safe(view_link, view_next,
			      &ivilayer->pending.view_list, pending_link) {
		wl_list_remove(&view_link->pending_link);
		wl_list_init(&view_link->pending_link);
	}
}

/**
 * Exported APIs of ivi-layout library are implemented from here.
 * Brief of APIs is described in ivi-layout-export.h.
 */
static void
ivi_layout_add_listener_create_layer(struct wl_listener *listener)
{
	struct ivi_layout *layout = get_instance();

	assert(listener);

	wl_signal_add(&layout->layer_notification.created, listener);
}

static void
ivi_layout_add_listener_remove_layer(struct wl_listener *listener)
{
	struct ivi_layout *layout = get_instance();

	assert(listener);

	wl_signal_add(&layout->layer_notification.removed, listener);
}

static void
ivi_layout_add_listener_create_surface(struct wl_listener *listener)
{
	struct ivi_layout *layout = get_instance();

	assert(listener);

	wl_signal_add(&layout->surface_notification.created, listener);
}

static void
ivi_layout_add_listener_remove_surface(struct wl_listener *listener)
{
	struct ivi_layout *layout = get_instance();

	assert(listener);

	wl_signal_add(&layout->surface_notification.removed, listener);
}

static void
ivi_layout_add_listener_configure_surface(struct wl_listener *listener)
{
	struct ivi_layout *layout = get_instance();

	assert(listener);

	wl_signal_add(&layout->surface_notification.configure_changed, listener);
}

static void
ivi_layout_add_listener_configure_desktop_surface(struct wl_listener *listener)
{
	struct ivi_layout *layout = get_instance();

	assert(listener);

	wl_signal_add(&layout->surface_notification.configure_desktop_changed, listener);
}

static int32_t
ivi_layout_shell_add_destroy_listener_once(struct wl_listener *listener, wl_notify_func_t destroy_handler)
{
	struct ivi_layout *layout = get_instance();

	assert(listener);
	assert(destroy_handler);

	if (wl_signal_get(&layout->shell_notification.destroy_signal, destroy_handler))
		return IVI_FAILED;

	listener->notify = destroy_handler;
	wl_signal_add(&layout->shell_notification.destroy_signal, listener);
	return IVI_SUCCEEDED;
}

uint32_t
ivi_layout_get_id_of_surface(struct ivi_layout_surface *ivisurf)
{
	return ivisurf->id_surface;
}

static uint32_t
ivi_layout_get_id_of_layer(struct ivi_layout_layer *ivilayer)
{
	return ivilayer->id_layer;
}

static struct ivi_layout_layer *
ivi_layout_get_layer_from_id(uint32_t id_layer)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_layer *ivilayer = NULL;

	wl_list_for_each(ivilayer, &layout->layer_list, link) {
		if (ivilayer->id_layer == id_layer) {
			return ivilayer;
		}
	}

	return NULL;
}

struct ivi_layout_surface *
ivi_layout_get_surface_from_id(uint32_t id_surface)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_surface *ivisurf = NULL;

	wl_list_for_each(ivisurf, &layout->surface_list, link) {
		if (ivisurf->id_surface == id_surface) {
			return ivisurf;
		}
	}

	return NULL;
}

static void
ivi_layout_surface_add_listener(struct ivi_layout_surface *ivisurf,
				    struct wl_listener *listener)
{
	assert(ivisurf);
	assert(listener);

	wl_signal_add(&ivisurf->property_changed, listener);
}

static const struct ivi_layout_layer_properties *
ivi_layout_get_properties_of_layer(struct ivi_layout_layer *ivilayer)
{
	assert(ivilayer);

	return &ivilayer->prop;
}

static void
ivi_layout_get_screens_under_layer(struct ivi_layout_layer *ivilayer,
				   int32_t *pLength,
				   struct weston_output ***ppArray)
{
	int32_t length = 0;
	int32_t n = 0;

	assert(ivilayer);
	assert(pLength);
	assert(ppArray);

	if (ivilayer->on_screen != NULL)
		length = 1;

	if (length != 0) {
		/* the Array must be free by module which called this function */
		*ppArray = xcalloc(length, sizeof(struct weston_output *));

		(*ppArray)[n++] = ivilayer->on_screen->output;
	}

	*pLength = length;
}

static void
ivi_layout_get_layers(int32_t *pLength, struct ivi_layout_layer ***ppArray)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_layer *ivilayer = NULL;
	int32_t length = 0;
	int32_t n = 0;

	assert(pLength);
	assert(ppArray);

	length = wl_list_length(&layout->layer_list);

	if (length != 0) {
		/* the Array must be freed by module which called this function */
		*ppArray = xcalloc(length, sizeof(struct ivi_layout_layer *));

		wl_list_for_each(ivilayer, &layout->layer_list, link) {
			(*ppArray)[n++] = ivilayer;
		}
	}

	*pLength = length;
}

static void
ivi_layout_get_layers_on_screen(struct weston_output *output,
				int32_t *pLength,
				struct ivi_layout_layer ***ppArray)
{
	struct ivi_layout_screen *iviscrn = NULL;
	struct ivi_layout_layer *ivilayer = NULL;
	int32_t length = 0;
	int32_t n = 0;

	assert(output);
	assert(pLength);
	assert(ppArray);

	iviscrn = get_screen_from_output(output);
	length = wl_list_length(&iviscrn->order.layer_list);

	if (length != 0) {
		/* the Array must be freed by module which called this function */
		*ppArray = xcalloc(length, sizeof(struct ivi_layout_layer *));

		wl_list_for_each(ivilayer, &iviscrn->order.layer_list, order.link) {
			(*ppArray)[n++] = ivilayer;
		}
	}

	*pLength = length;
}

static void
ivi_layout_get_layers_under_surface(struct ivi_layout_surface *ivisurf,
				    int32_t *pLength,
				    struct ivi_layout_layer ***ppArray)
{
	struct ivi_layout_view *ivi_view;
	int32_t length = 0;
	int32_t n = 0;

	assert(ivisurf);
	assert(pLength);
	assert(ppArray);

	if (!wl_list_empty(&ivisurf->view_list)) {
		/* the Array must be free by module which called this function */
		length = wl_list_length(&ivisurf->view_list);
		*ppArray = xcalloc(length, sizeof(struct ivi_layout_layer *));

		wl_list_for_each_reverse(ivi_view, &ivisurf->view_list, surf_link) {
			if (ivi_view_is_rendered(ivi_view))
				(*ppArray)[n++] = ivi_view->on_layer;
			else
				length--;
		}
		if (length == 0) {
			free(*ppArray);
			*ppArray = NULL;
		}
	}

	*pLength = length;
}

static void
ivi_layout_get_surfaces(int32_t *pLength, struct ivi_layout_surface ***ppArray)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_surface *ivisurf = NULL;
	int32_t length = 0;
	int32_t n = 0;

	assert(pLength);
	assert(ppArray);

	length = wl_list_length(&layout->surface_list);

	if (length != 0) {
		/* the Array must be freed by module which called this function */
		*ppArray = xcalloc(length, sizeof(struct ivi_layout_surface *));

		wl_list_for_each(ivisurf, &layout->surface_list, link) {
			(*ppArray)[n++] = ivisurf;
		}
	}

	*pLength = length;
}

static void
ivi_layout_get_surfaces_on_layer(struct ivi_layout_layer *ivilayer,
				 int32_t *pLength,
				 struct ivi_layout_surface ***ppArray)
{
	struct ivi_layout_view *ivi_view = NULL;
	int32_t length = 0;
	int32_t n = 0;

	assert(ivilayer);
	assert(pLength);
	assert(ppArray);

	length = wl_list_length(&ivilayer->order.view_list);

	if (length != 0) {
		/* the Array must be freed by module which called this function */
		*ppArray = xcalloc(length, sizeof(struct ivi_layout_surface *));

		wl_list_for_each(ivi_view, &ivilayer->order.view_list, order_link) {
			(*ppArray)[n++] = ivi_view->ivisurf;
		}
	}

	*pLength = length;
}

static struct ivi_layout_layer *
ivi_layout_layer_create_with_dimension(uint32_t id_layer,
				       int32_t width, int32_t height)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_layer *ivilayer = NULL;

	ivilayer = get_layer(&layout->layer_list, id_layer);
	if (ivilayer != NULL) {
		weston_log("id_layer is already created\n");
		++ivilayer->ref_count;
		return ivilayer;
	}

	ivilayer = xzalloc(sizeof *ivilayer);

	ivilayer->ref_count = 1;
	wl_signal_init(&ivilayer->property_changed);
	ivilayer->layout = layout;
	ivilayer->id_layer = id_layer;

	init_layer_properties(&ivilayer->prop, width, height);

	wl_list_init(&ivilayer->pending.view_list);
	wl_list_init(&ivilayer->pending.link);
	ivilayer->pending.prop = ivilayer->prop;

	wl_list_init(&ivilayer->order.view_list);
	wl_list_init(&ivilayer->order.link);

	wl_list_insert(&layout->layer_list, &ivilayer->link);

	wl_signal_emit(&layout->layer_notification.created, ivilayer);

	return ivilayer;
}

static void
ivi_layout_layer_destroy(struct ivi_layout_layer *ivilayer)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_view *ivi_view, *next;

	assert(ivilayer);

	if (--ivilayer->ref_count > 0)
		return;

	/*Destroy all ivi_views*/
	wl_list_for_each_safe(ivi_view, next, &layout->view_list, link) {
		if (ivi_view->on_layer == ivilayer)
			ivi_view_destroy(ivi_view);
	}

	wl_signal_emit(&layout->layer_notification.removed, ivilayer);

	wl_list_remove(&ivilayer->pending.link);
	wl_list_remove(&ivilayer->order.link);
	wl_list_remove(&ivilayer->link);

	free(ivilayer);
}

void
ivi_layout_layer_set_visibility(struct ivi_layout_layer *ivilayer,
				bool newVisibility)
{
	struct ivi_layout_layer_properties *prop = NULL;

	assert(ivilayer);

	prop = &ivilayer->pending.prop;
	prop->visibility = newVisibility;

	if (ivilayer->prop.visibility != newVisibility)
		prop->event_mask |= IVI_NOTIFICATION_VISIBILITY;
	else
		prop->event_mask &= ~IVI_NOTIFICATION_VISIBILITY;
}

int32_t
ivi_layout_layer_set_opacity(struct ivi_layout_layer *ivilayer,
			     wl_fixed_t opacity)
{
	struct ivi_layout_layer_properties *prop = NULL;

	assert(ivilayer);

	if (opacity < wl_fixed_from_double(0.0) ||
	    wl_fixed_from_double(1.0) < opacity) {
		weston_log("ivi_layout_layer_set_opacity: invalid argument\n");
		return IVI_FAILED;
	}

	prop = &ivilayer->pending.prop;
	prop->opacity = opacity;

	if (ivilayer->prop.opacity != opacity)
		prop->event_mask |= IVI_NOTIFICATION_OPACITY;
	else
		prop->event_mask &= ~IVI_NOTIFICATION_OPACITY;

	return IVI_SUCCEEDED;
}

static void
ivi_layout_layer_set_source_rectangle(struct ivi_layout_layer *ivilayer,
				      int32_t x, int32_t y,
				      int32_t width, int32_t height)
{
	struct ivi_layout_layer_properties *prop = NULL;

	assert(ivilayer);

	prop = &ivilayer->pending.prop;
	prop->source_x = x;
	prop->source_y = y;
	prop->source_width = width;
	prop->source_height = height;

	if (ivilayer->prop.source_x != x || ivilayer->prop.source_y != y ||
	    ivilayer->prop.source_width != width ||
	    ivilayer->prop.source_height != height)
		prop->event_mask |= IVI_NOTIFICATION_SOURCE_RECT;
	else
		prop->event_mask &= ~IVI_NOTIFICATION_SOURCE_RECT;
}

void
ivi_layout_layer_set_destination_rectangle(struct ivi_layout_layer *ivilayer,
					   int32_t x, int32_t y,
					   int32_t width, int32_t height)
{
	struct ivi_layout_layer_properties *prop = NULL;

	assert(ivilayer);

	prop = &ivilayer->pending.prop;
	prop->dest_x = x;
	prop->dest_y = y;
	prop->dest_width = width;
	prop->dest_height = height;

	if (ivilayer->prop.dest_x != x || ivilayer->prop.dest_y != y ||
	    ivilayer->prop.dest_width != width ||
	    ivilayer->prop.dest_height != height)
		prop->event_mask |= IVI_NOTIFICATION_DEST_RECT;
	else
		prop->event_mask &= ~IVI_NOTIFICATION_DEST_RECT;
}

void
ivi_layout_layer_set_render_order(struct ivi_layout_layer *ivilayer,
				  struct ivi_layout_surface **pSurface,
				  int32_t number)
{
	int32_t i = 0;
	struct ivi_layout_view * ivi_view;

	assert(ivilayer);

	clear_view_pending_list(ivilayer);

	for (i = 0; i < number; i++) {
		ivi_view = get_ivi_view(ivilayer, pSurface[i]);
		if (!ivi_view)
			ivi_view = ivi_view_create(ivilayer, pSurface[i]);

		assert(ivi_view != NULL);

		wl_list_remove(&ivi_view->pending_link);
		wl_list_insert(&ivilayer->pending.view_list, &ivi_view->pending_link);
	}

	ivilayer->order.dirty = 1;
}

void
ivi_layout_surface_set_visibility(struct ivi_layout_surface *ivisurf,
				  bool newVisibility)
{
	struct ivi_layout_surface_properties *prop = NULL;

	assert(ivisurf);

	prop = &ivisurf->pending.prop;
	prop->visibility = newVisibility;

	if (ivisurf->prop.visibility != newVisibility)
		prop->event_mask |= IVI_NOTIFICATION_VISIBILITY;
	else
		prop->event_mask &= ~IVI_NOTIFICATION_VISIBILITY;
}

int32_t
ivi_layout_surface_set_opacity(struct ivi_layout_surface *ivisurf,
			       wl_fixed_t opacity)
{
	struct ivi_layout_surface_properties *prop = NULL;

	assert(ivisurf);

	if (opacity < wl_fixed_from_double(0.0) ||
	    wl_fixed_from_double(1.0) < opacity) {
		weston_log("ivi_layout_surface_set_opacity: invalid argument\n");
		return IVI_FAILED;
	}

	prop = &ivisurf->pending.prop;
	prop->opacity = opacity;

	if (ivisurf->prop.opacity != opacity)
		prop->event_mask |= IVI_NOTIFICATION_OPACITY;
	else
		prop->event_mask &= ~IVI_NOTIFICATION_OPACITY;

	return IVI_SUCCEEDED;
}

void
ivi_layout_surface_set_destination_rectangle(struct ivi_layout_surface *ivisurf,
					     int32_t x, int32_t y,
					     int32_t width, int32_t height)
{
	struct ivi_layout_surface_properties *prop = NULL;

	assert(ivisurf);

	prop = &ivisurf->pending.prop;
	prop->start_x = prop->dest_x;
	prop->start_y = prop->dest_y;
	prop->dest_x = x;
	prop->dest_y = y;
	prop->start_width = prop->dest_width;
	prop->start_height = prop->dest_height;
	prop->dest_width = width;
	prop->dest_height = height;

	if (ivisurf->prop.dest_x != x || ivisurf->prop.dest_y != y ||
	    ivisurf->prop.dest_width != width ||
	    ivisurf->prop.dest_height != height)
		prop->event_mask |= IVI_NOTIFICATION_DEST_RECT;
	else
		prop->event_mask &= ~IVI_NOTIFICATION_DEST_RECT;
}

void
ivi_layout_surface_set_size(struct ivi_layout_surface *ivisurf,
			    int32_t width, int32_t height)
{
	switch (ivisurf->prop.surface_type) {
	case IVI_LAYOUT_SURFACE_TYPE_DESKTOP:
		weston_desktop_surface_set_size(ivisurf->weston_desktop_surface,
						width, height);
		return;
	case IVI_LAYOUT_SURFACE_TYPE_IVI:
		shell_surface_send_configure(ivisurf->surface,
					     width, height);
		return;
	case IVI_LAYOUT_SURFACE_TYPE_INPUT_PANEL:
		return;
	}
	/* there should be no other surface type */
	assert(0);
}

static void
ivi_layout_screen_add_layer(struct weston_output *output,
			    struct ivi_layout_layer *addlayer)
{
	struct ivi_layout_screen *iviscrn;

	assert(output);
	assert(addlayer);

	iviscrn = get_screen_from_output(output);

	/*if layer is already assigned to screen make order of it dirty
	 * we are going to remove it (in commit_screen_list)*/
	if (addlayer->on_screen)
		addlayer->on_screen->order.dirty = 1;

	wl_list_remove(&addlayer->pending.link);
	wl_list_insert(&iviscrn->pending.layer_list, &addlayer->pending.link);

	iviscrn->order.dirty = 1;
}

static void
ivi_layout_screen_remove_layer(struct weston_output *output,
			    struct ivi_layout_layer *removelayer)
{
	struct ivi_layout_screen *iviscrn;

	assert(output);
	assert(removelayer);

	iviscrn = get_screen_from_output(output);

	wl_list_remove(&removelayer->pending.link);
	wl_list_init(&removelayer->pending.link);

	iviscrn->order.dirty = 1;
}

static void
ivi_layout_screen_set_render_order(struct weston_output *output,
				   struct ivi_layout_layer **pLayer,
				   const int32_t number)
{
	struct ivi_layout_screen *iviscrn;
	struct ivi_layout_layer *ivilayer = NULL;
	struct ivi_layout_layer *next = NULL;
	int32_t i = 0;

	assert(output);

	iviscrn = get_screen_from_output(output);

	wl_list_for_each_safe(ivilayer, next,
			      &iviscrn->pending.layer_list, pending.link) {
		wl_list_remove(&ivilayer->pending.link);
		wl_list_init(&ivilayer->pending.link);
	}

	assert(wl_list_empty(&iviscrn->pending.layer_list));

	for (i = 0; i < number; i++) {
		wl_list_remove(&pLayer[i]->pending.link);
		wl_list_insert(&iviscrn->pending.layer_list,
				&pLayer[i]->pending.link);
	}

	iviscrn->order.dirty = 1;
}

/**
 * This function is used by the additional ivi-module because of dumping ivi_surface sceenshot.
 * The ivi-module, e.g. ivi-controller.so, is in wayland-ivi-extension of Genivi's Layer Management.
 * This function is used to get the result of drawing by clients.
 */
static struct weston_surface *
ivi_layout_surface_get_weston_surface(struct ivi_layout_surface *ivisurf)
{
	return ivisurf != NULL ? ivisurf->surface : NULL;
}

static void
ivi_layout_surface_get_size(struct ivi_layout_surface *ivisurf,
			    int32_t *width, int32_t *height,
			    int32_t *stride)
{
	int32_t w;
	int32_t h;
	const size_t bytespp = 4; /* PIXMAN_a8b8g8r8 */

	assert(ivisurf);

	weston_surface_get_content_size(ivisurf->surface, &w, &h);

	if (width != NULL)
		*width = w;

	if (height != NULL)
		*height = h;

	if (stride != NULL)
		*stride = w * bytespp;
}

static void
ivi_layout_layer_add_listener(struct ivi_layout_layer *ivilayer,
				  struct wl_listener *listener)
{
	assert(ivilayer);
	assert(listener);

	wl_signal_add(&ivilayer->property_changed, listener);
}

static const struct ivi_layout_surface_properties *
ivi_layout_get_properties_of_surface(struct ivi_layout_surface *ivisurf)
{
	assert(ivisurf);

	return &ivisurf->prop;
}

static void
ivi_layout_layer_add_surface(struct ivi_layout_layer *ivilayer,
			     struct ivi_layout_surface *addsurf)
{
	struct ivi_layout_view *ivi_view;

	assert(ivilayer);
	assert(addsurf);

	ivi_view = get_ivi_view(ivilayer, addsurf);
	if (!ivi_view)
		ivi_view = ivi_view_create(ivilayer, addsurf);

	wl_list_remove(&ivi_view->pending_link);
	wl_list_insert(&ivilayer->pending.view_list, &ivi_view->pending_link);

	ivilayer->order.dirty = 1;
}

static void
ivi_layout_layer_remove_surface(struct ivi_layout_layer *ivilayer,
				struct ivi_layout_surface *remsurf)
{
	struct ivi_layout_view *ivi_view;

	if (ivilayer == NULL || remsurf == NULL) {
		weston_log("ivi_layout_layer_remove_surface: invalid argument\n");
		return;
	}

	ivi_view = get_ivi_view(ivilayer, remsurf);
	if (ivi_view) {
		wl_list_remove(&ivi_view->pending_link);
		wl_list_init(&ivi_view->pending_link);

		ivilayer->order.dirty = 1;
	}
}

static void
ivi_layout_surface_set_source_rectangle(struct ivi_layout_surface *ivisurf,
					int32_t x, int32_t y,
					int32_t width, int32_t height)
{
	struct ivi_layout_surface_properties *prop = NULL;

	assert(ivisurf);

	prop = &ivisurf->pending.prop;
	prop->source_x = x;
	prop->source_y = y;
	prop->source_width = width;
	prop->source_height = height;

	if (ivisurf->prop.source_x != x || ivisurf->prop.source_y != y ||
	    ivisurf->prop.source_width != width ||
	    ivisurf->prop.source_height != height)
		prop->event_mask |= IVI_NOTIFICATION_SOURCE_RECT;
	else
		prop->event_mask &= ~IVI_NOTIFICATION_SOURCE_RECT;
}

int32_t
ivi_layout_commit_changes(void)
{
	struct ivi_layout *layout = get_instance();

	commit_surface_list(layout);
	commit_layer_list(layout);
	commit_screen_list(layout);
	build_view_list(layout);

	commit_transition(layout);

	commit_changes(layout);
	send_prop(layout);

	return IVI_SUCCEEDED;
}

static int32_t
ivi_layout_commit_current(void)
{
	struct ivi_layout *layout = get_instance();
	build_view_list(layout);
	commit_changes(layout);
	send_prop(layout);
	return IVI_SUCCEEDED;
}

static void
ivi_layout_layer_set_transition(struct ivi_layout_layer *ivilayer,
				enum ivi_layout_transition_type type,
				uint32_t duration)
{
	assert(ivilayer);

	ivilayer->pending.prop.transition_type = type;
	ivilayer->pending.prop.transition_duration = duration;
}

static void
ivi_layout_layer_set_fade_info(struct ivi_layout_layer* ivilayer,
			       uint32_t is_fade_in,
			       double start_alpha, double end_alpha)
{
	assert(ivilayer);

	ivilayer->pending.prop.is_fade_in = is_fade_in;
	ivilayer->pending.prop.start_alpha = start_alpha;
	ivilayer->pending.prop.end_alpha = end_alpha;
}

static void
ivi_layout_surface_set_transition_duration(struct ivi_layout_surface *ivisurf,
					   uint32_t duration)
{
	struct ivi_layout_surface_properties *prop;

	assert(ivisurf);

	prop = &ivisurf->pending.prop;
	prop->transition_duration = duration*10;
}

/*
 * This interface enables e.g. an id agent to set the id of an ivi-layout
 * surface, that has been created by a desktop application. This can only be
 * done once as long as the initial surface id equals IVI_INVALID_ID. Afterwards
 * two events are emitted, namely surface_created and surface_configured.
 */
static int32_t
ivi_layout_surface_set_id(struct ivi_layout_surface *ivisurf,
			  uint32_t id_surface)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_surface *search_ivisurf = NULL;

	assert(ivisurf);

	if (ivisurf->id_surface != IVI_INVALID_ID) {
		weston_log("surface id can only be set once\n");
		return IVI_FAILED;
	}

	search_ivisurf = get_surface(&layout->surface_list, id_surface);
	if (search_ivisurf) {
		weston_log("id_surface(%d) is already created\n", id_surface);
		return IVI_FAILED;
	}

	ivisurf->id_surface = id_surface;

	wl_signal_emit(&layout->surface_notification.configure_changed,
		       ivisurf);

	return IVI_SUCCEEDED;
}

static void
ivi_layout_surface_set_transition(struct ivi_layout_surface *ivisurf,
				  enum ivi_layout_transition_type type,
				  uint32_t duration)
{
	struct ivi_layout_surface_properties *prop;

	assert(ivisurf);

	prop = &ivisurf->pending.prop;
	prop->transition_type = type;
	prop->transition_duration = duration;
}

static int32_t
ivi_layout_surface_dump(struct weston_surface *surface,
			void *target, size_t size,int32_t x, int32_t y,
			int32_t width, int32_t height)
{
	int result = 0;

	assert(surface);

	result = weston_surface_copy_content(
		surface, target, size,
		x, y, width, height);

	return result == 0 ? IVI_SUCCEEDED : IVI_FAILED;
}

/**
 * methods of interaction between ivi-shell with ivi-layout
 */

static struct ivi_layout_surface*
surface_create(struct weston_surface *wl_surface, uint32_t id_surface,
	       enum ivi_layout_surface_type surface_type)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_surface *ivisurf = NULL;

	ivisurf = xzalloc(sizeof *ivisurf);

	wl_signal_init(&ivisurf->property_changed);
	ivisurf->id_surface = id_surface;
	ivisurf->layout = layout;

	ivisurf->surface = wl_surface;

	ivisurf->surface->width_from_buffer  = 0;
	ivisurf->surface->height_from_buffer = 0;

	init_surface_properties(&ivisurf->prop);
	ivisurf->prop.surface_type = surface_type;

	ivisurf->pending.prop = ivisurf->prop;

	wl_list_init(&ivisurf->view_list);

	wl_list_insert(&layout->surface_list, &ivisurf->link);

	return ivisurf;
}

void
ivi_layout_desktop_surface_configure(struct ivi_layout_surface *ivisurf,
				 int32_t width, int32_t height)
{
	struct ivi_layout *layout = get_instance();
	ivisurf->prop.event_mask |= IVI_NOTIFICATION_CONFIGURE;

	/* emit callback which is set by ivi-layout api user */
	wl_signal_emit(&layout->surface_notification.configure_desktop_changed,
		       ivisurf);
}

struct ivi_layout_surface*
ivi_layout_desktop_surface_create(struct weston_surface *wl_surface,
				  struct weston_desktop_surface *surface)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_surface *ivisurf;

	ivisurf =  surface_create(wl_surface, IVI_INVALID_ID,
				  IVI_LAYOUT_SURFACE_TYPE_DESKTOP);

	ivisurf->weston_desktop_surface = surface;
	wl_signal_emit(&layout->surface_notification.created, ivisurf);

	return ivisurf;
}

struct ivi_layout_surface*
ivi_layout_input_panel_surface_create(struct weston_surface *wl_surface)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_surface *ivisurf;

	ivisurf = surface_create(wl_surface, IVI_INVALID_ID,
				 IVI_LAYOUT_SURFACE_TYPE_INPUT_PANEL);

	weston_signal_emit_mutable(&layout->surface_notification.created,
				   ivisurf);

	return ivisurf;
}

void
ivi_layout_input_panel_surface_configure(struct ivi_layout_surface *ivisurf,
					 int32_t width, int32_t height)
{
	struct ivi_layout *layout = get_instance();

	weston_signal_emit_mutable(&layout->input_panel_notification.configure_changed,
				   ivisurf);
}

void
ivi_layout_update_text_input_cursor(pixman_box32_t *cursor_rectangle)
{
	struct ivi_layout *layout = get_instance();

	memcpy(&layout->text_input.cursor_rectangle, cursor_rectangle,
	       sizeof(pixman_box32_t));
}

void
ivi_layout_show_input_panel(struct ivi_layout_surface *ivisurf,
			    struct ivi_layout_surface *target_ivisurf,
			    bool overlay_panel)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_text_input_state state = {
		.overlay_panel = overlay_panel,
		.input_panel = ivisurf,
		.surface = target_ivisurf,
		.cursor_rectangle = layout->text_input.cursor_rectangle
	};
	layout->text_input.ivisurf = target_ivisurf;

	weston_signal_emit_mutable(&layout->input_panel_notification.show,
				   &state);
}

void
ivi_layout_hide_input_panel(struct ivi_layout_surface *ivisurf)
{
	struct ivi_layout *layout = get_instance();

	weston_signal_emit_mutable(&layout->input_panel_notification.hide,
				   ivisurf);
}

void
ivi_layout_update_input_panel(struct ivi_layout_surface *ivisurf,
			      bool overlay_panel)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_text_input_state state = {
		.overlay_panel = overlay_panel,
		.input_panel = ivisurf,
		.surface =layout->text_input.ivisurf,
		.cursor_rectangle = layout->text_input.cursor_rectangle
	};

	weston_signal_emit_mutable(&layout->input_panel_notification.update,
				   &state);
}

static void
ivi_layout_add_listener_configure_input_panel_surface(struct wl_listener *listener)
{
	struct ivi_layout *layout = get_instance();

	assert(listener);

	wl_signal_add(&layout->input_panel_notification.configure_changed, listener);
	shell_ensure_text_input(layout->shell);
}

static void
ivi_layout_add_listener_show_input_panel(struct wl_listener *listener)
{
	struct ivi_layout *layout = get_instance();

	assert(listener);

	wl_signal_add(&layout->input_panel_notification.show, listener);
	shell_ensure_text_input(layout->shell);
}

static void
ivi_layout_add_listener_hide_input_panel(struct wl_listener *listener)
{
	struct ivi_layout *layout = get_instance();

	assert(listener);

	wl_signal_add(&layout->input_panel_notification.hide, listener);
	shell_ensure_text_input(layout->shell);
}

static void
ivi_layout_add_listener_update_input_panel(struct wl_listener *listener)
{
	struct ivi_layout *layout = get_instance();

	assert(listener);

	wl_signal_add(&layout->input_panel_notification.update, listener);
	shell_ensure_text_input(layout->shell);
}

void
ivi_layout_surface_configure(struct ivi_layout_surface *ivisurf,
			     int32_t width, int32_t height)
{
	struct ivi_layout *layout = get_instance();
	ivisurf->prop.event_mask |= IVI_NOTIFICATION_CONFIGURE;

	/* emit callback which is set by ivi-layout api user */
	wl_signal_emit(&layout->surface_notification.configure_changed,
		       ivisurf);
}

struct ivi_layout_surface*
ivi_layout_surface_create(struct weston_surface *wl_surface,
			  uint32_t id_surface)
{
	struct ivi_layout *layout = get_instance();
	struct ivi_layout_surface *ivisurf = NULL;

	ivisurf = get_surface(&layout->surface_list, id_surface);
	if (ivisurf) {
		weston_log("id_surface(%d) is already created\n", id_surface);
		return NULL;
	}

	ivisurf = surface_create(wl_surface, id_surface,
				 IVI_LAYOUT_SURFACE_TYPE_IVI);

	if (ivisurf)
		wl_signal_emit(&layout->surface_notification.created, ivisurf);

	return ivisurf;
}

void
ivi_layout_ivi_shell_destroy(void)
{
	struct ivi_layout *layout = get_instance();

	/* emit callback which is set by ivi-layout api user */
	weston_signal_emit_mutable(&layout->shell_notification.destroy_signal, NULL);
}

static struct ivi_layout_interface ivi_layout_interface;

void
ivi_layout_init(struct weston_compositor *ec, struct ivi_shell *shell)
{
	struct ivi_layout *layout = get_instance();

	layout->shell = shell;

	wl_list_init(&layout->surface_list);
	wl_list_init(&layout->layer_list);
	wl_list_init(&layout->screen_list);
	wl_list_init(&layout->view_list);

	wl_signal_init(&layout->layer_notification.created);
	wl_signal_init(&layout->layer_notification.removed);

	wl_signal_init(&layout->surface_notification.created);
	wl_signal_init(&layout->surface_notification.removed);
	wl_signal_init(&layout->surface_notification.configure_changed);
	wl_signal_init(&layout->surface_notification.configure_desktop_changed);

	wl_signal_init(&layout->input_panel_notification.configure_changed);
	wl_signal_init(&layout->input_panel_notification.show);
	wl_signal_init(&layout->input_panel_notification.hide);
	wl_signal_init(&layout->input_panel_notification.update);

	wl_signal_init(&layout->shell_notification.destroy_signal);

	/* Add layout_layer at the last of weston_compositor.layer_list */
	weston_layer_init(&layout->layout_layer, ec);
	weston_layer_set_position(&layout->layout_layer,
				  WESTON_LAYER_POSITION_NORMAL);

	create_screen(ec);

	layout->output_created.notify = output_created_event;
	wl_signal_add(&ec->output_created_signal, &layout->output_created);

	layout->output_destroyed.notify = output_destroyed_event;
	wl_signal_add(&ec->output_destroyed_signal, &layout->output_destroyed);

	layout->transitions = ivi_layout_transition_set_create(ec);
	wl_list_init(&layout->pending_transition_list);

	weston_plugin_api_register(ec, IVI_LAYOUT_API_NAME,
				   &ivi_layout_interface,
				   sizeof(struct ivi_layout_interface));
}

void
ivi_layout_fini(void)
{
	struct ivi_layout *layout = get_instance();

	weston_layer_fini(&layout->layout_layer);

	/* XXX: tear down everything else */
	wl_list_remove(&layout->output_created.link);
	wl_list_remove(&layout->output_destroyed.link);
}

static struct ivi_layout_interface ivi_layout_interface = {
	/**
	 * commit all changes
	 */
	.commit_changes = ivi_layout_commit_changes,
	.commit_current = ivi_layout_commit_current,

	/**
	 * surface controller interfaces
	 */
	.add_listener_create_surface	= ivi_layout_add_listener_create_surface,
	.add_listener_remove_surface	= ivi_layout_add_listener_remove_surface,
	.add_listener_configure_surface	= ivi_layout_add_listener_configure_surface,
	.add_listener_configure_desktop_surface	= ivi_layout_add_listener_configure_desktop_surface,
	.get_surface				= shell_get_ivi_layout_surface,
	.get_surfaces				= ivi_layout_get_surfaces,
	.get_id_of_surface			= ivi_layout_get_id_of_surface,
	.get_surface_from_id			= ivi_layout_get_surface_from_id,
	.get_properties_of_surface		= ivi_layout_get_properties_of_surface,
	.get_surfaces_on_layer			= ivi_layout_get_surfaces_on_layer,
	.surface_set_visibility			= ivi_layout_surface_set_visibility,
	.surface_set_opacity			= ivi_layout_surface_set_opacity,
	.surface_set_source_rectangle		= ivi_layout_surface_set_source_rectangle,
	.surface_set_destination_rectangle	= ivi_layout_surface_set_destination_rectangle,
	.surface_add_listener			= ivi_layout_surface_add_listener,
	.surface_get_weston_surface		= ivi_layout_surface_get_weston_surface,
	.surface_set_transition			= ivi_layout_surface_set_transition,
	.surface_set_transition_duration	= ivi_layout_surface_set_transition_duration,
	.surface_set_id				= ivi_layout_surface_set_id,

	/**
	 * layer controller interfaces
	 */
	.add_listener_create_layer			= ivi_layout_add_listener_create_layer,
	.add_listener_remove_layer		= ivi_layout_add_listener_remove_layer,
	.layer_create_with_dimension		= ivi_layout_layer_create_with_dimension,
	.layer_destroy				= ivi_layout_layer_destroy,
	.get_layers				= ivi_layout_get_layers,
	.get_id_of_layer			= ivi_layout_get_id_of_layer,
	.get_layer_from_id			= ivi_layout_get_layer_from_id,
	.get_properties_of_layer		= ivi_layout_get_properties_of_layer,
	.get_layers_under_surface		= ivi_layout_get_layers_under_surface,
	.get_layers_on_screen			= ivi_layout_get_layers_on_screen,
	.layer_set_visibility			= ivi_layout_layer_set_visibility,
	.layer_set_opacity			= ivi_layout_layer_set_opacity,
	.layer_set_source_rectangle		= ivi_layout_layer_set_source_rectangle,
	.layer_set_destination_rectangle	= ivi_layout_layer_set_destination_rectangle,
	.layer_add_surface			= ivi_layout_layer_add_surface,
	.layer_remove_surface			= ivi_layout_layer_remove_surface,
	.layer_set_render_order			= ivi_layout_layer_set_render_order,
	.layer_add_listener			= ivi_layout_layer_add_listener,
	.layer_set_transition			= ivi_layout_layer_set_transition,

	/**
	 * screen controller interfaces
	 */
	.get_screens_under_layer	= ivi_layout_get_screens_under_layer,
	.screen_add_layer		= ivi_layout_screen_add_layer,
	.screen_remove_layer		= ivi_layout_screen_remove_layer,
	.screen_set_render_order	= ivi_layout_screen_set_render_order,

	/**
	 * animation
	 */
	.transition_move_layer_cancel	= ivi_layout_transition_move_layer_cancel,
	.layer_set_fade_info		= ivi_layout_layer_set_fade_info,

	/**
	 * surface content dumping for debugging
	 */
	.surface_get_size		= ivi_layout_surface_get_size,
	.surface_dump			= ivi_layout_surface_dump,

	/**
	 * shell interfaces
	 */
	.shell_add_destroy_listener_once = ivi_layout_shell_add_destroy_listener_once,

	/**
	 * input panel
	 */
	.add_listener_configure_input_panel_surface	= ivi_layout_add_listener_configure_input_panel_surface,
	.add_listener_show_input_panel			= ivi_layout_add_listener_show_input_panel,
	.add_listener_hide_input_panel			= ivi_layout_add_listener_hide_input_panel,
	.add_listener_update_input_panel		= ivi_layout_add_listener_update_input_panel,
};
