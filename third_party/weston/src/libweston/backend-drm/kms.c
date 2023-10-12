/*
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2011 Intel Corporation
 * Copyright © 2017, 2018 Collabora, Ltd.
 * Copyright © 2017, 2018 General Electric Company
 * Copyright (c) 2018 DisplayLink (UK) Ltd.
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

#include <stdint.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <libweston/libweston.h>
#include <libweston/backend-drm.h>
#include "shared/helpers.h"
#include "shared/weston-drm-fourcc.h"
#include "drm-internal.h"
#include "pixel-formats.h"
#include "presentation-time-server-protocol.h"

#ifndef DRM_CAP_ATOMIC_ASYNC_PAGE_FLIP
#define DRM_CAP_ATOMIC_ASYNC_PAGE_FLIP 0x15
#endif

struct drm_property_enum_info plane_type_enums[] = {
	[WDRM_PLANE_TYPE_PRIMARY] = {
		.name = "Primary",
	},
	[WDRM_PLANE_TYPE_OVERLAY] = {
		.name = "Overlay",
	},
	[WDRM_PLANE_TYPE_CURSOR] = {
		.name = "Cursor",
	},
};

struct drm_property_enum_info plane_rotation_enums[] = {
	[WDRM_PLANE_ROTATION_0] = {
		.name = "rotate-0",
	},
	[WDRM_PLANE_ROTATION_90] = {
		.name = "rotate-90",
	},
	[WDRM_PLANE_ROTATION_180] = {
		.name = "rotate-180",
	},
	[WDRM_PLANE_ROTATION_270] = {
		.name = "rotate-270",
	},
	[WDRM_PLANE_ROTATION_REFLECT_X] = {
		.name = "reflect-x",
	},
	[WDRM_PLANE_ROTATION_REFLECT_Y] = {
		.name = "reflect-y",
	},
};

const struct drm_property_info plane_props[] = {
	[WDRM_PLANE_TYPE] = {
		.name = "type",
		.enum_values = plane_type_enums,
		.num_enum_values = WDRM_PLANE_TYPE__COUNT,
	},
	[WDRM_PLANE_SRC_X] = { .name = "SRC_X", },
	[WDRM_PLANE_SRC_Y] = { .name = "SRC_Y", },
	[WDRM_PLANE_SRC_W] = { .name = "SRC_W", },
	[WDRM_PLANE_SRC_H] = { .name = "SRC_H", },
	[WDRM_PLANE_CRTC_X] = { .name = "CRTC_X", },
	[WDRM_PLANE_CRTC_Y] = { .name = "CRTC_Y", },
	[WDRM_PLANE_CRTC_W] = { .name = "CRTC_W", },
	[WDRM_PLANE_CRTC_H] = { .name = "CRTC_H", },
	[WDRM_PLANE_FB_ID] = { .name = "FB_ID", },
	[WDRM_PLANE_CRTC_ID] = { .name = "CRTC_ID", },
	[WDRM_PLANE_IN_FORMATS] = { .name = "IN_FORMATS" },
	[WDRM_PLANE_IN_FENCE_FD] = { .name = "IN_FENCE_FD" },
	[WDRM_PLANE_FB_DAMAGE_CLIPS] = { .name = "FB_DAMAGE_CLIPS" },
	[WDRM_PLANE_ZPOS] = { .name = "zpos" },
	[WDRM_PLANE_ROTATION] = {
		.name = "rotation",
		.enum_values = plane_rotation_enums,
		.num_enum_values = WDRM_PLANE_ROTATION__COUNT,
	 },
	[WDRM_PLANE_ALPHA] = { .name = "alpha" },
};

struct drm_property_enum_info dpms_state_enums[] = {
	[WDRM_DPMS_STATE_OFF] = {
		.name = "Off",
	},
	[WDRM_DPMS_STATE_ON] = {
		.name = "On",
	},
	[WDRM_DPMS_STATE_STANDBY] = {
		.name = "Standby",
	},
	[WDRM_DPMS_STATE_SUSPEND] = {
		.name = "Suspend",
	},
};

struct drm_property_enum_info content_protection_enums[] = {
	[WDRM_CONTENT_PROTECTION_UNDESIRED] = {
		.name = "Undesired",
	},
	[WDRM_CONTENT_PROTECTION_DESIRED] = {
		.name = "Desired",
	},
	[WDRM_CONTENT_PROTECTION_ENABLED] = {
		.name = "Enabled",
	},
};

struct drm_property_enum_info hdcp_content_type_enums[] = {
	[WDRM_HDCP_CONTENT_TYPE0] = {
		.name = "HDCP Type0",
	},
	[WDRM_HDCP_CONTENT_TYPE1] = {
		.name = "HDCP Type1",
	},
};

struct drm_property_enum_info panel_orientation_enums[] = {
	[WDRM_PANEL_ORIENTATION_NORMAL] = { .name = "Normal", },
	[WDRM_PANEL_ORIENTATION_UPSIDE_DOWN] = { .name = "Upside Down", },
	[WDRM_PANEL_ORIENTATION_LEFT_SIDE_UP] = { .name = "Left Side Up", },
	[WDRM_PANEL_ORIENTATION_RIGHT_SIDE_UP] = { .name = "Right Side Up", },
};

struct drm_property_enum_info content_type_enums[] = {
	[WDRM_CONTENT_TYPE_NO_DATA] = { .name = "No Data", },
	[WDRM_CONTENT_TYPE_GRAPHICS] = { .name = "Graphics", },
	[WDRM_CONTENT_TYPE_PHOTO] = { .name = "Photo", },
	[WDRM_CONTENT_TYPE_CINEMA] = { .name = "Cinema", },
	[WDRM_CONTENT_TYPE_GAME] = { .name = "Game", },
};

const struct drm_property_info connector_props[] = {
	[WDRM_CONNECTOR_EDID] = { .name = "EDID" },
	[WDRM_CONNECTOR_DPMS] = {
		.name = "DPMS",
		.enum_values = dpms_state_enums,
		.num_enum_values = WDRM_DPMS_STATE__COUNT,
	},
	[WDRM_CONNECTOR_CRTC_ID] = { .name = "CRTC_ID", },
	[WDRM_CONNECTOR_WRITEBACK_PIXEL_FORMATS] = { .name = "WRITEBACK_PIXEL_FORMATS", },
	[WDRM_CONNECTOR_WRITEBACK_FB_ID] = { .name = "WRITEBACK_FB_ID", },
	[WDRM_CONNECTOR_WRITEBACK_OUT_FENCE_PTR] = { .name = "WRITEBACK_OUT_FENCE_PTR", },
	[WDRM_CONNECTOR_NON_DESKTOP] = { .name = "non-desktop", },
	[WDRM_CONNECTOR_CONTENT_PROTECTION] = {
		.name = "Content Protection",
		.enum_values = content_protection_enums,
		.num_enum_values = WDRM_CONTENT_PROTECTION__COUNT,
	},
	[WDRM_CONNECTOR_HDCP_CONTENT_TYPE] = {
		.name = "HDCP Content Type",
		.enum_values = hdcp_content_type_enums,
		.num_enum_values = WDRM_HDCP_CONTENT_TYPE__COUNT,
	},
	[WDRM_CONNECTOR_PANEL_ORIENTATION] = {
		.name = "panel orientation",
		.enum_values = panel_orientation_enums,
		.num_enum_values = WDRM_PANEL_ORIENTATION__COUNT,
	},
	[WDRM_CONNECTOR_HDR_OUTPUT_METADATA] = {
		.name = "HDR_OUTPUT_METADATA",
	},
	[WDRM_CONNECTOR_MAX_BPC] = { .name = "max bpc", },
	[WDRM_CONNECTOR_CONTENT_TYPE] = {
		.name = "content type",
		.enum_values = content_type_enums,
		.num_enum_values = WDRM_CONTENT_TYPE__COUNT,
	},
};

const struct drm_property_info crtc_props[] = {
	[WDRM_CRTC_MODE_ID] = { .name = "MODE_ID", },
	[WDRM_CRTC_ACTIVE] = { .name = "ACTIVE", },
	[WDRM_CRTC_CTM] = { .name = "CTM", },
	[WDRM_CRTC_DEGAMMA_LUT] = { .name = "DEGAMMA_LUT", },
	[WDRM_CRTC_DEGAMMA_LUT_SIZE] = { .name = "DEGAMMA_LUT_SIZE", },
	[WDRM_CRTC_GAMMA_LUT] = { .name = "GAMMA_LUT", },
	[WDRM_CRTC_GAMMA_LUT_SIZE] = { .name = "GAMMA_LUT_SIZE", },
	[WDRM_CRTC_VRR_ENABLED] = { .name = "VRR_ENABLED", },
};


/**
 * Mode for drm_pending_state_apply and co.
 */
enum drm_state_apply_mode {
	DRM_STATE_APPLY_SYNC, /**< state fully processed */
	DRM_STATE_APPLY_ASYNC, /**< state pending event delivery */
	DRM_STATE_TEST_ONLY, /**< test if the state can be applied */
};

/**
 * Get the current value of a KMS property
 *
 * Given a drmModeObjectGetProperties return, as well as the drm_property_info
 * for the target property, return the current value of that property,
 * with an optional default. If the property is a KMS enum type, the return
 * value will be translated into the appropriate internal enum.
 *
 * If the property is not present, the default value will be returned.
 *
 * @param info Internal structure for property to look up
 * @param props Raw KMS properties for the target object
 * @param def Value to return if property is not found
 */
uint64_t
drm_property_get_value(struct drm_property_info *info,
		       const drmModeObjectProperties *props,
		       uint64_t def)
{
	unsigned int i;

	if (info->prop_id == 0)
		return def;

	for (i = 0; i < props->count_props; i++) {
		unsigned int j;

		if (props->props[i] != info->prop_id)
			continue;

		/* Simple (non-enum) types can return the value directly */
		if (info->num_enum_values == 0)
			return props->prop_values[i];

		/* Map from raw value to enum value */
		for (j = 0; j < info->num_enum_values; j++) {
			if (!info->enum_values[j].valid)
				continue;
			if (info->enum_values[j].value != props->prop_values[i])
				continue;

			return j;
		}

		/* We don't have a mapping for this enum; return default. */
		break;
	}

	return def;
}

/**
 * Get the current range values of a KMS property
 *
 * Given a drmModeObjectGetProperties return, as well as the drm_property_info
 * for the target property, return the current range values of that property,
 *
 * If the property is not present, or there's no it is not a prop range then
 * NULL will be returned.
 *
 * @param info Internal structure for property to look up
 * @param props Raw KMS properties for the target object
 */
uint64_t *
drm_property_get_range_values(struct drm_property_info *info,
			      const drmModeObjectProperties *props)
{
	unsigned int i;

	if (info->prop_id == 0)
		return NULL;

	for (i = 0; i < props->count_props; i++) {

		if (props->props[i] != info->prop_id)
			continue;

		if (!(info->flags & DRM_MODE_PROP_RANGE) &&
		    !(info->flags & DRM_MODE_PROP_SIGNED_RANGE))
			continue;

		return info->range_values;
	}

	return NULL;
}

/* We use the fact that 0 is not a valid rotation here - if we return 0,
 * the plane doesn't support the rotation requested. Otherwise the correct
 * value to achieve the requested rotation on this plane is returned.
 */
uint64_t
drm_rotation_from_output_transform(struct drm_plane *plane,
				   enum wl_output_transform ot)
{
	struct drm_property_info *info = &plane->props[WDRM_PLANE_ROTATION];
	enum wdrm_plane_rotation drm_rotation;
	enum wdrm_plane_rotation drm_reflection = 0;
	uint64_t out = 0;

	if (info->prop_id == 0) {
		if (ot == WL_OUTPUT_TRANSFORM_NORMAL)
			return 1;

		return 0;
	}

	switch (ot) {
	case WL_OUTPUT_TRANSFORM_NORMAL:
		drm_rotation = WDRM_PLANE_ROTATION_0;
		break;
	case WL_OUTPUT_TRANSFORM_90:
		drm_rotation = WDRM_PLANE_ROTATION_90;
		break;
	case WL_OUTPUT_TRANSFORM_180:
		drm_rotation = WDRM_PLANE_ROTATION_180;
		break;
	case WL_OUTPUT_TRANSFORM_270:
		drm_rotation = WDRM_PLANE_ROTATION_270;
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED:
		drm_rotation = WDRM_PLANE_ROTATION_0;
		drm_reflection = WDRM_PLANE_ROTATION_REFLECT_X;
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_90:
		drm_rotation = WDRM_PLANE_ROTATION_90;
		drm_reflection = WDRM_PLANE_ROTATION_REFLECT_X;
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_180:
		drm_rotation = WDRM_PLANE_ROTATION_180;
		drm_reflection = WDRM_PLANE_ROTATION_REFLECT_X;
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_270:
		drm_rotation = WDRM_PLANE_ROTATION_270;
		drm_reflection = WDRM_PLANE_ROTATION_REFLECT_X;
		break;
	default:
		assert(0 && "bad output transform");
	}

	if (!info->enum_values[drm_rotation].valid)
		return 0;

	out |= 1 << info->enum_values[drm_rotation].value;

	if (drm_reflection) {
		if (!info->enum_values[drm_reflection].valid)
			return 0;
		out |= 1 << info->enum_values[drm_reflection].value;
	}

	return out;
}

/**
 * Cache DRM property values
 *
 * Update a per-object array of drm_property_info structures, given the
 * DRM properties of the object.
 *
 * Call this every time an object newly appears (note that only connectors
 * can be hotplugged), the first time it is seen, or when its status changes
 * in a way which invalidates the potential property values (currently, the
 * only case for this is connector hotplug).
 *
 * This updates the property IDs and enum values within the drm_property_info
 * array.
 *
 * DRM property enum values are dynamic at runtime; the user must query the
 * property to find out the desired runtime value for a requested string
 * name. Using the 'type' field on planes as an example, there is no single
 * hardcoded constant for primary plane types; instead, the property must be
 * queried at runtime to find the value associated with the string "Primary".
 *
 * This helper queries and caches the enum values, to allow us to use a set
 * of compile-time-constant enums portably across various implementations.
 * The values given in enum_names are searched for, and stored in the
 * same-indexed field of the map array.
 *
 * @param device DRM device object
 * @param src DRM property info array to source from
 * @param info DRM property info array to copy into
 * @param num_infos Number of entries in the source array
 * @param props DRM object properties for the object
 */
void
drm_property_info_populate(struct drm_device *device,
		           const struct drm_property_info *src,
			   struct drm_property_info *info,
			   unsigned int num_infos,
			   drmModeObjectProperties *props)
{
	drmModePropertyRes *prop;
	unsigned i, j;

	for (i = 0; i < num_infos; i++) {
		unsigned int j;

		info[i].name = src[i].name;
		info[i].prop_id = 0;
		info[i].num_enum_values = src[i].num_enum_values;

		if (src[i].num_enum_values == 0)
			continue;

		info[i].enum_values =
			malloc(src[i].num_enum_values *
			       sizeof(*info[i].enum_values));
		assert(info[i].enum_values);
		for (j = 0; j < info[i].num_enum_values; j++) {
			info[i].enum_values[j].name = src[i].enum_values[j].name;
			info[i].enum_values[j].valid = false;
		}
	}

	for (i = 0; i < props->count_props; i++) {
		unsigned int k;

		prop = drmModeGetProperty(device->drm.fd, props->props[i]);
		if (!prop)
			continue;

		for (j = 0; j < num_infos; j++) {
			if (!strcmp(prop->name, info[j].name))
				break;
		}

		/* We don't know/care about this property. */
		if (j == num_infos) {
#ifdef DEBUG
			weston_log("DRM debug: unrecognized property %u '%s'\n",
				   prop->prop_id, prop->name);
#endif
			drmModeFreeProperty(prop);
			continue;
		}

		if (info[j].num_enum_values == 0 &&
		    (prop->flags & DRM_MODE_PROP_ENUM)) {
			weston_log("DRM: expected property %s to not be an"
			           " enum, but it is; ignoring\n", prop->name);
			drmModeFreeProperty(prop);
			continue;
		}

		info[j].prop_id = props->props[i];
		info[j].flags = prop->flags;

		if (prop->flags & DRM_MODE_PROP_RANGE ||
		    prop->flags & DRM_MODE_PROP_SIGNED_RANGE) {
			info[j].num_range_values = prop->count_values;
			for (int i = 0; i < prop->count_values; i++)
				info[j].range_values[i] = prop->values[i];
		}


		if (info[j].num_enum_values == 0) {
			drmModeFreeProperty(prop);
			continue;
		}

		if (!(prop->flags & DRM_MODE_PROP_ENUM) &&
		    !(prop->flags & DRM_MODE_PROP_BITMASK)) {
			weston_log("DRM: expected property %s to be an enum"
				   " or bitmask, but it is not; ignoring\n",
				   prop->name);
			drmModeFreeProperty(prop);
			info[j].prop_id = 0;
			continue;
		}

		for (k = 0; k < info[j].num_enum_values; k++) {
			int l;

			for (l = 0; l < prop->count_enums; l++) {
				if (!strcmp(prop->enums[l].name,
					    info[j].enum_values[k].name))
					break;
			}

			if (l == prop->count_enums)
				continue;

			info[j].enum_values[k].valid = true;
			info[j].enum_values[k].value = prop->enums[l].value;
		}

		drmModeFreeProperty(prop);
	}

#ifdef DEBUG
	for (i = 0; i < num_infos; i++) {
		if (info[i].prop_id == 0)
			weston_log("DRM warning: property '%s' missing\n",
				   info[i].name);
	}
#endif
}

/**
 * Free DRM property information
 *
 * Frees all memory associated with a DRM property info array and zeroes
 * it out, leaving it usable for a further drm_property_info_update() or
 * drm_property_info_free().
 *
 * @param info DRM property info array
 * @param num_props Number of entries in array to free
 */
void
drm_property_info_free(struct drm_property_info *info, int num_props)
{
	int i;

	for (i = 0; i < num_props; i++)
		free(info[i].enum_values);

	memset(info, 0, sizeof(*info) * num_props);
}

/**
 * Populates the plane's formats array, using either the IN_FORMATS blob
 * property (if available), or the plane's format list if not.
 */
int
drm_plane_populate_formats(struct drm_plane *plane, const drmModePlane *kplane,
			   const drmModeObjectProperties *props,
			   const bool use_modifiers)
{
	struct drm_device *device = plane->device;
	uint32_t i, blob_id, fmt_prev = DRM_FORMAT_INVALID;
	drmModeFormatModifierIterator drm_iter = {0};
	struct weston_drm_format *fmt = NULL;
	drmModePropertyBlobRes *blob = NULL;
	int ret = 0;

	if (!use_modifiers)
		goto fallback;

	blob_id = drm_property_get_value(&plane->props[WDRM_PLANE_IN_FORMATS],
				         props,
				         0);
	if (blob_id == 0)
		goto fallback;

	blob = drmModeGetPropertyBlob(device->drm.fd, blob_id);
	if (!blob)
		goto fallback;

	while (drmModeFormatModifierBlobIterNext(blob, &drm_iter)) {
		if (fmt_prev != drm_iter.fmt) {
			fmt = weston_drm_format_array_add_format(&plane->formats,
								 drm_iter.fmt);
			if (!fmt) {
				ret = -1;
				goto out;
			}

			fmt_prev = drm_iter.fmt;
		}

		ret = weston_drm_format_add_modifier(fmt, drm_iter.mod);
		if (ret < 0)
			goto out;

	}

out:
	drmModeFreePropertyBlob(blob);
	return ret;

fallback:
	/* No IN_FORMATS blob available, so just use the old. */
	for (i = 0; i < kplane->count_formats; i++) {
		fmt = weston_drm_format_array_add_format(&plane->formats,
							 kplane->formats[i]);
		if (!fmt)
			return -1;
		ret = weston_drm_format_add_modifier(fmt, DRM_FORMAT_MOD_INVALID);
		if (ret < 0)
			return -1;
	}
	return 0;
}

void
drm_output_set_gamma(struct weston_output *output_base,
		     uint16_t size, uint16_t *r, uint16_t *g, uint16_t *b)
{
	int rc;
	struct drm_output *output = to_drm_output(output_base);
	struct drm_device *device = output->device;

	assert(output);

	/* check */
	if (output_base->gamma_size != size)
		return;

	output->deprecated_gamma_is_set = true;
	rc = drmModeCrtcSetGamma(device->drm.fd,
				 output->crtc->crtc_id,
				 size, r, g, b);
	if (rc)
		weston_log("set gamma failed: %s\n", strerror(errno));
}

/**
 * Mark an output state as current on the output, i.e. it has been
 * submitted to the kernel. The mode argument determines whether this
 * update will be applied synchronously (e.g. when calling drmModeSetCrtc),
 * or asynchronously (in which case we wait for events to complete).
 */
static void
drm_output_assign_state(struct drm_output_state *state,
			enum drm_state_apply_mode mode)
{
	struct drm_output *output = state->output;
	struct drm_device *device = output->device;
	struct drm_backend *b = device->backend;
	struct drm_plane_state *plane_state;
	struct drm_head *head;

	assert(!output->state_last);

	if (mode == DRM_STATE_APPLY_ASYNC)
		output->state_last = output->state_cur;
	else
		drm_output_state_free(output->state_cur);

	wl_list_remove(&state->link);
	wl_list_init(&state->link);
	state->pending_state = NULL;

	output->state_cur = state;

	if (device->atomic_modeset && mode == DRM_STATE_APPLY_ASYNC) {
		drm_debug(b, "\t[CRTC:%u] setting pending flip\n",
			  output->crtc->crtc_id);
		output->atomic_complete_pending = true;
	}

	if (device->atomic_modeset &&
	    state->protection == WESTON_HDCP_DISABLE)
		wl_list_for_each(head, &output->base.head_list, base.output_link)
			weston_head_set_content_protection_status(&head->base,
							   WESTON_HDCP_DISABLE);

	/* Replace state_cur on each affected plane with the new state, being
	 * careful to dispose of orphaned (but only orphaned) previous state.
	 * If the previous state is not orphaned (still has an output_state
	 * attached), it will be disposed of by freeing the output_state. */
	wl_list_for_each(plane_state, &state->plane_list, link) {
		struct drm_plane *plane = plane_state->plane;

		if (plane->state_cur && !plane->state_cur->output_state)
			drm_plane_state_free(plane->state_cur, true);
		plane->state_cur = plane_state;

		if (mode != DRM_STATE_APPLY_ASYNC) {
			plane_state->complete = true;
			continue;
		}

		if (device->atomic_modeset)
			continue;

		assert(plane->type != WDRM_PLANE_TYPE_OVERLAY);
		if (plane->type == WDRM_PLANE_TYPE_PRIMARY)
			output->page_flip_pending = true;
	}
}

static void
drm_output_set_cursor(struct drm_output_state *output_state)
{
	struct drm_output *output = output_state->output;
	struct drm_device *device = output->device;
	struct drm_crtc *crtc = output->crtc;
	struct drm_plane *plane = output->cursor_plane;
	struct drm_plane_state *state;
	uint32_t handle;

	if (!plane)
		return;

	state = drm_output_state_get_existing_plane(output_state, plane);
	if (!state)
		return;

	if (!state->fb) {
		pixman_region32_fini(&plane->base.damage);
		pixman_region32_init(&plane->base.damage);
		drmModeSetCursor(device->drm.fd, crtc->crtc_id, 0, 0, 0);
		return;
	}

	assert(state->fb == output->gbm_cursor_fb[output->current_cursor]);
	assert(!plane->state_cur->output || plane->state_cur->output == output);

	handle = output->gbm_cursor_handle[output->current_cursor];
	if (plane->state_cur->fb != state->fb) {
		if (drmModeSetCursor(device->drm.fd, crtc->crtc_id, handle,
				     device->cursor_width, device->cursor_height)) {
			weston_log("failed to set cursor: %s\n",
				   strerror(errno));
			goto err;
		}
	}

	pixman_region32_fini(&plane->base.damage);
	pixman_region32_init(&plane->base.damage);

	if (drmModeMoveCursor(device->drm.fd, crtc->crtc_id,
	                      state->dest_x, state->dest_y)) {
		weston_log("failed to move cursor: %s\n", strerror(errno));
		goto err;
	}

	return;

err:
	device->cursors_are_broken = true;
	drmModeSetCursor(device->drm.fd, crtc->crtc_id, 0, 0, 0);
}

static void
drm_output_reset_legacy_gamma(struct drm_output *output)
{
	uint32_t len = output->base.gamma_size;
	uint16_t *lut;
	uint32_t i;
	int ret;

	if (len == 0)
		return;

	if (output->legacy_gamma_not_supported)
		return;

	lut = calloc(len, sizeof(uint16_t));
	if (!lut)
		return;

	/* Identity curve */
	for (i = 0; i < len; i++)
		lut[i] = 0xffff * i / (len - 1);

	ret = drmModeCrtcSetGamma(output->device->drm.fd,
				  output->crtc->crtc_id,
				  len, lut, lut, lut);
	if (ret == -EOPNOTSUPP || ret == -ENOSYS)
		output->legacy_gamma_not_supported = true;
	else if (ret < 0) {
		weston_log("%s failed for %s: %s\n", __func__,
			   output->base.name, strerror(-ret));
	}

	free(lut);
}

static int
drm_output_apply_state_legacy(struct drm_output_state *state)
{
	struct drm_output *output = state->output;
	struct drm_device *device = output->device;
	struct drm_backend *backend = device->backend;
	struct drm_plane *scanout_plane = output->scanout_plane;
	struct drm_crtc *crtc = output->crtc;
	struct drm_property_info *dpms_prop;
	struct drm_plane_state *scanout_state;
	struct drm_mode *mode;
	struct drm_head *head;
	const struct pixel_format_info *pinfo = NULL;
	uint32_t connectors[MAX_CLONED_CONNECTORS];
	int n_conn = 0;
	struct timespec now;
	int ret = 0;

	wl_list_for_each(head, &output->base.head_list, base.output_link) {
		assert(n_conn < MAX_CLONED_CONNECTORS);
		connectors[n_conn++] = head->connector.connector_id;
	}

	/* If disable_planes is set then assign_planes() wasn't
	 * called for this render, so we could still have a stale
	 * cursor plane set up.
	 */
	if (output->base.disable_planes) {
		drm_output_set_cursor_view(output, NULL);
		if (output->cursor_plane) {
			output->cursor_plane->base.x = INT32_MIN;
			output->cursor_plane->base.y = INT32_MIN;
		}
	}

	if (state->dpms != WESTON_DPMS_ON) {
		if (output->cursor_plane) {
			ret = drmModeSetCursor(device->drm.fd, crtc->crtc_id,
					       0, 0, 0);
			if (ret)
				weston_log("drmModeSetCursor failed disable: %s\n",
					   strerror(errno));
		}

		ret = drmModeSetCrtc(device->drm.fd, crtc->crtc_id, 0, 0, 0,
				     NULL, 0, NULL);
		if (ret)
			weston_log("drmModeSetCrtc failed disabling: %s\n",
				   strerror(errno));

		drm_output_assign_state(state, DRM_STATE_APPLY_SYNC);
		weston_compositor_read_presentation_clock(output->base.compositor, &now);
		drm_output_update_complete(output,
		                           WP_PRESENTATION_FEEDBACK_KIND_HW_COMPLETION,
					   now.tv_sec, now.tv_nsec / 1000);

		return 0;
	}

	scanout_state =
		drm_output_state_get_existing_plane(state, scanout_plane);

	/* The legacy SetCrtc API doesn't allow us to do scaling, and the
	 * legacy PageFlip API doesn't allow us to do clipping either. */
	assert(scanout_state->src_x == 0);
	assert(scanout_state->src_y == 0);
	assert(scanout_state->src_w ==
		(unsigned) (output->base.current_mode->width << 16));
	assert(scanout_state->src_h ==
		(unsigned) (output->base.current_mode->height << 16));
	assert(scanout_state->dest_x == 0);
	assert(scanout_state->dest_y == 0);
	assert(scanout_state->dest_w == scanout_state->src_w >> 16);
	assert(scanout_state->dest_h == scanout_state->src_h >> 16);
	/* The legacy SetCrtc API doesn't support fences */
	assert(scanout_state->in_fence_fd == -1);

	mode = to_drm_mode(output->base.current_mode);
	if (device->state_invalid ||
	    !scanout_plane->state_cur->fb ||
	    scanout_plane->state_cur->fb->strides[0] !=
	    scanout_state->fb->strides[0]) {

		ret = drmModeSetCrtc(device->drm.fd, crtc->crtc_id,
				     scanout_state->fb->fb_id,
				     0, 0,
				     connectors, n_conn,
				     &mode->mode_info);
		if (ret) {
			weston_log("set mode failed: %s\n", strerror(errno));
			goto err;
		}

		if (!output->deprecated_gamma_is_set)
			drm_output_reset_legacy_gamma(output);
	}

	pinfo = scanout_state->fb->format;
	drm_debug(backend, "\t[CRTC:%u, PLANE:%u] FORMAT: %s\n",
			   crtc->crtc_id, scanout_state->plane->plane_id,
			   pinfo ? pinfo->drm_format_name : "UNKNOWN");

	if (drmModePageFlip(device->drm.fd, crtc->crtc_id,
			    scanout_state->fb->fb_id,
			    DRM_MODE_PAGE_FLIP_EVENT, output) < 0) {
		weston_log("queueing pageflip failed: %s\n", strerror(errno));
		goto err;
	}

	assert(!output->page_flip_pending);

	if (output->pageflip_timer)
		wl_event_source_timer_update(output->pageflip_timer,
		                             backend->pageflip_timeout);

	drm_output_set_cursor(state);

	if (state->dpms != output->state_cur->dpms) {
		wl_list_for_each(head, &output->base.head_list, base.output_link) {
			dpms_prop = &head->connector.props[WDRM_CONNECTOR_DPMS];
			if (dpms_prop->prop_id == 0)
				continue;

			ret = drmModeConnectorSetProperty(device->drm.fd,
						head->connector.connector_id,
						dpms_prop->prop_id,
						state->dpms);
			if (ret) {
				weston_log("DRM: DPMS: failed property set for %s\n",
					   head->base.name);
			}
		}
	}

	drm_output_assign_state(state, DRM_STATE_APPLY_ASYNC);

	return 0;

err:
	drm_output_set_cursor_view(output, NULL);
	drm_output_state_free(state);
	return -1;
}

static int
crtc_add_prop(drmModeAtomicReq *req, struct drm_crtc *crtc,
	      enum wdrm_crtc_property prop, uint64_t val)
{
	struct drm_device *device = crtc->device;
	struct drm_backend *b = device->backend;
	struct drm_property_info *info = &crtc->props_crtc[prop];
	int ret;

	drm_debug(b, "\t\t\t[CRTC:%lu] %lu (%s) -> %llu (0x%llx)\n",
		  (unsigned long) crtc->crtc_id,
		  (unsigned long) info->prop_id, info->name,
		  (unsigned long long) val, (unsigned long long) val);

	if (info->prop_id == 0)
		return -1;

	ret = drmModeAtomicAddProperty(req, crtc->crtc_id, info->prop_id,
				       val);
	return (ret <= 0) ? -1 : 0;
}

/** Set a CRTC property, allowing zero value for non-existing property
 *
 * \param req The atomic KMS request to append to.
 * \param crtc The CRTC whose property to set.
 * \param prop Which CRTC property to set.
 * \param val The value, cast to u64, to set to the CRTC property.
 * \return 0 on succcess, -1 on failure.
 *
 * If the property does not exist, attempting to set it to value
 * zero is ok, because the property with value zero has the same
 * KMS effect as the property not existing.
 *
 * However, trying to set a non-existing property to a non-zero value
 * must fail, because that would not achieve the desired KMS effect.
 *
 * It is up to the caller to understand which KMS properties work
 * like this and which do not.
 */
static int
crtc_add_prop_zero_ok(drmModeAtomicReq *req, struct drm_crtc *crtc,
		      enum wdrm_crtc_property prop, uint64_t val)
{
	struct drm_property_info *info = &crtc->props_crtc[prop];

	if (info->prop_id == 0 && val == 0)
		return 0;

	return crtc_add_prop(req, crtc, prop, val);
}

static int
connector_add_prop(drmModeAtomicReq *req, struct drm_connector *connector,
		   enum wdrm_connector_property prop, uint64_t val)
{
	struct drm_device *device = connector->device;
	struct drm_backend *b = device->backend;
	struct drm_property_info *info = &connector->props[prop];
	uint32_t connector_id = connector->connector_id;
	int ret;

	drm_debug(b, "\t\t\t[CONN:%lu] %lu (%s) -> %llu (0x%llx)\n",
		  (unsigned long) connector_id,
		  (unsigned long) info->prop_id, info->name,
		  (unsigned long long) val, (unsigned long long) val);

	if (info->prop_id == 0)
		return -1;

	ret = drmModeAtomicAddProperty(req, connector_id, info->prop_id, val);
	return (ret <= 0) ? -1 : 0;
}

static int
plane_add_prop(drmModeAtomicReq *req, struct drm_plane *plane,
	       enum wdrm_plane_property prop, uint64_t val)
{
	struct drm_device *device = plane->device;
	struct drm_backend *b = device->backend;
	struct drm_property_info *info = &plane->props[prop];
	int ret;

	drm_debug(b, "\t\t\t[PLANE:%lu] %lu (%s) -> %llu (0x%llx)\n",
		  (unsigned long) plane->plane_id,
		  (unsigned long) info->prop_id, info->name,
		  (unsigned long long) val, (unsigned long long) val);

	if (info->prop_id == 0)
		return -1;

	ret = drmModeAtomicAddProperty(req, plane->plane_id, info->prop_id,
				       val);
	return (ret <= 0) ? -1 : 0;
}

static bool
drm_connector_has_prop(struct drm_connector *connector,
		       enum wdrm_connector_property prop)
{
	if (connector->props[prop].prop_id != 0)
		return true;

	return false;
}

/*
 * This function converts the protection requests from weston_hdcp_protection
 * corresponding drm values. These values can be set in "Content Protection"
 * & "HDCP Content Type" connector properties.
 */
static void
get_drm_protection_from_weston(enum weston_hdcp_protection weston_protection,
			       enum wdrm_content_protection_state *drm_protection,
			       enum wdrm_hdcp_content_type *drm_cp_type)
{

	switch (weston_protection) {
	case WESTON_HDCP_DISABLE:
		*drm_protection = WDRM_CONTENT_PROTECTION_UNDESIRED;
		*drm_cp_type = WDRM_HDCP_CONTENT_TYPE0;
		break;
	case WESTON_HDCP_ENABLE_TYPE_0:
		*drm_protection = WDRM_CONTENT_PROTECTION_DESIRED;
		*drm_cp_type = WDRM_HDCP_CONTENT_TYPE0;
		break;
	case WESTON_HDCP_ENABLE_TYPE_1:
		*drm_protection = WDRM_CONTENT_PROTECTION_DESIRED;
		*drm_cp_type = WDRM_HDCP_CONTENT_TYPE1;
		break;
	default:
		assert(0 && "bad weston_hdcp_protection");
	}
}

static void
drm_connector_set_hdcp_property(struct drm_connector *connector,
				enum weston_hdcp_protection protection,
				drmModeAtomicReq *req)
{
	int ret;
	enum wdrm_content_protection_state drm_protection;
	enum wdrm_hdcp_content_type drm_cp_type;
	struct drm_property_enum_info *enum_info;
	uint64_t prop_val;
	struct drm_property_info *props = connector->props;

	get_drm_protection_from_weston(protection, &drm_protection,
				       &drm_cp_type);

	if (!drm_connector_has_prop(connector, WDRM_CONNECTOR_CONTENT_PROTECTION))
		return;

	/*
	 * Content-type property is not exposed for platforms not supporting
	 * HDCP2.2, therefore, type-1 cannot be supported. The type-0 content
	 * still can be supported if the content-protection property is exposed.
	 */
	if (!drm_connector_has_prop(connector, WDRM_CONNECTOR_HDCP_CONTENT_TYPE) &&
	    drm_cp_type != WDRM_HDCP_CONTENT_TYPE0)
			return;

	enum_info = props[WDRM_CONNECTOR_CONTENT_PROTECTION].enum_values;
	prop_val = enum_info[drm_protection].value;
	ret = connector_add_prop(req, connector,
				 WDRM_CONNECTOR_CONTENT_PROTECTION, prop_val);
	assert(ret == 0);

	if (!drm_connector_has_prop(connector, WDRM_CONNECTOR_HDCP_CONTENT_TYPE))
		return;

	enum_info = props[WDRM_CONNECTOR_HDCP_CONTENT_TYPE].enum_values;
	prop_val = enum_info[drm_cp_type].value;
	ret = connector_add_prop(req, connector,
				 WDRM_CONNECTOR_HDCP_CONTENT_TYPE, prop_val);
	assert(ret == 0);
}

static int
drm_connector_set_max_bpc(struct drm_connector *connector,
			  struct drm_output *output,
			  drmModeAtomicReq *req)
{
	const struct drm_property_info *info;
	struct drm_head *head;
	struct drm_backend *backend = output->device->backend;
	uint64_t max_bpc;
	uint64_t a, b;

	if (!drm_connector_has_prop(connector, WDRM_CONNECTOR_MAX_BPC))
		return 0;

	if (output->max_bpc == 0) {
		/* A value of 0 means that the current max_bpc must be programmed. */
		head = drm_head_find_by_connector(backend, connector->connector_id);
		max_bpc = head->inherited_max_bpc;
	} else {
		info = &connector->props[WDRM_CONNECTOR_MAX_BPC];
		assert(info->flags & DRM_MODE_PROP_RANGE);
		assert(info->num_range_values == 2);
		a = info->range_values[0];
		b = info->range_values[1];
		assert(a <= b);

		max_bpc = MAX(a, MIN(output->max_bpc, b));
	}

	return connector_add_prop(req, connector,
				  WDRM_CONNECTOR_MAX_BPC, max_bpc);
}

static int
drm_connector_set_content_type(struct drm_connector *connector,
			       enum wdrm_content_type content_type,
			       drmModeAtomicReq *req)
{
	struct drm_property_enum_info *enum_info;
	uint64_t prop_val;
	struct drm_property_info *props = connector->props;

	if (!drm_connector_has_prop(connector, WDRM_CONNECTOR_CONTENT_TYPE))
		return 0;

	enum_info = props[WDRM_CONNECTOR_CONTENT_TYPE].enum_values;
	prop_val = enum_info[content_type].value;
	return connector_add_prop(req, connector,
				  WDRM_CONNECTOR_CONTENT_TYPE, prop_val);
}

static int
drm_output_apply_state_atomic(struct drm_output_state *state,
			      drmModeAtomicReq *req,
			      uint32_t *flags)
{
	struct drm_output *output = state->output;
	struct drm_device *device = output->device;
	struct drm_backend *b = device->backend;
	struct drm_crtc *crtc = output->crtc;
	struct drm_plane_state *plane_state;
	struct drm_mode *current_mode = to_drm_mode(output->base.current_mode);
	struct drm_head *head;
	struct drm_head *tmp;
	struct drm_writeback_state *wb_state = output->wb_state;
	enum writeback_screenshot_state wb_screenshot_state =
		drm_output_get_writeback_state(output);
	int ret = 0;

	drm_debug(b, "\t\t[atomic] %s output %lu (%s) state\n",
		  (*flags & DRM_MODE_ATOMIC_TEST_ONLY) ? "testing" : "applying",
		  (unsigned long) output->base.id, output->base.name);

	if (state->dpms != output->state_cur->dpms) {
		drm_debug(b, "\t\t\t[atomic] DPMS state differs, modeset OK\n");
		*flags |= DRM_MODE_ATOMIC_ALLOW_MODESET;
	}

	if (wb_screenshot_state == DRM_OUTPUT_WB_SCREENSHOT_PREPARE_COMMIT) {
		drm_debug(b, "\t\t\t[atomic] Writeback connector screenshot requested, modeset OK\n");
		*flags |= DRM_MODE_ATOMIC_ALLOW_MODESET;
	}

	if (state->dpms == WESTON_DPMS_ON) {
		ret = drm_mode_ensure_blob(device, current_mode);
		if (ret != 0)
			return ret;

		ret |= crtc_add_prop(req, crtc, WDRM_CRTC_MODE_ID,
				     current_mode->blob_id);
		ret |= crtc_add_prop(req, crtc, WDRM_CRTC_ACTIVE, 1);

		if (!output->deprecated_gamma_is_set) {
			ret |= crtc_add_prop_zero_ok(req, crtc,
						     WDRM_CRTC_GAMMA_LUT, 0);
			ret |= crtc_add_prop_zero_ok(req, crtc,
						     WDRM_CRTC_DEGAMMA_LUT, 0);
		}
		ret |= crtc_add_prop_zero_ok(req, crtc, WDRM_CRTC_CTM, 0);
		ret |= crtc_add_prop_zero_ok(req, crtc, WDRM_CRTC_VRR_ENABLED, 0);

		/* No need for the DPMS property, since it is implicit in
		 * routing and CRTC activity. */
		wl_list_for_each(head, &output->base.head_list, base.output_link) {
			ret |= connector_add_prop(req, &head->connector,
						  WDRM_CONNECTOR_CRTC_ID,
						  crtc->crtc_id);
		}

		if (wb_screenshot_state == DRM_OUTPUT_WB_SCREENSHOT_PREPARE_COMMIT) {
			ret |= connector_add_prop(req, &wb_state->wb->connector,
						  WDRM_CONNECTOR_CRTC_ID,
						  crtc->crtc_id);
			ret |= connector_add_prop(req, &wb_state->wb->connector,
						  WDRM_CONNECTOR_WRITEBACK_FB_ID,
						  wb_state->fb->fb_id);
			ret |= connector_add_prop(req, &wb_state->wb->connector,
						  WDRM_CONNECTOR_WRITEBACK_OUT_FENCE_PTR,
						  (uintptr_t)&wb_state->out_fence_fd);
			if (!(*flags & DRM_MODE_ATOMIC_TEST_ONLY))
				wb_state->state = DRM_OUTPUT_WB_SCREENSHOT_CHECK_FENCE;
		}
	} else {
		ret |= crtc_add_prop(req, crtc, WDRM_CRTC_MODE_ID, 0);
		ret |= crtc_add_prop(req, crtc, WDRM_CRTC_ACTIVE, 0);

		if (wb_screenshot_state == DRM_OUTPUT_WB_SCREENSHOT_PREPARE_COMMIT) {
			drm_debug(b, "\t\t\t[atomic] Writeback connector screenshot requested but CRTC is off\n");
			drm_writeback_fail_screenshot(wb_state, "drm: CRTC is off");
		}

		/* No need for the DPMS property, since it is implicit in
		 * routing and CRTC activity. */
		wl_list_for_each(head, &output->base.head_list, base.output_link)
			ret |= connector_add_prop(req, &head->connector,
						  WDRM_CONNECTOR_CRTC_ID, 0);

		wl_list_for_each_safe(head, tmp, &output->disable_head,
				      disable_head_link) {
			ret |= connector_add_prop(req, &head->connector,
						  WDRM_CONNECTOR_CRTC_ID, 0);
			wl_list_remove(&head->disable_head_link);
			wl_list_init(&head->disable_head_link);
		}
	}

	wl_list_for_each(head, &output->base.head_list, base.output_link) {
		drm_connector_set_hdcp_property(&head->connector,
						state->protection, req);
		ret |= drm_connector_set_content_type(&head->connector,
						      output->content_type, req);

		if (drm_connector_has_prop(&head->connector,
					   WDRM_CONNECTOR_HDR_OUTPUT_METADATA)) {
			ret |= connector_add_prop(req, &head->connector,
						  WDRM_CONNECTOR_HDR_OUTPUT_METADATA,
						  output->hdr_output_metadata_blob_id);
		}

		ret |= drm_connector_set_max_bpc(&head->connector, output, req);
	}

	if (ret != 0) {
		weston_log("couldn't set atomic CRTC/connector state\n");
		return ret;
	}

	wl_list_for_each(plane_state, &state->plane_list, link) {
		struct drm_plane *plane = plane_state->plane;
		const struct pixel_format_info *pinfo = NULL;

		ret |= plane_add_prop(req, plane, WDRM_PLANE_FB_ID,
				      plane_state->fb ? plane_state->fb->fb_id : 0);
		ret |= plane_add_prop(req, plane, WDRM_PLANE_CRTC_ID,
				      plane_state->fb ? crtc->crtc_id : 0);
		ret |= plane_add_prop(req, plane, WDRM_PLANE_SRC_X,
				      plane_state->src_x);
		ret |= plane_add_prop(req, plane, WDRM_PLANE_SRC_Y,
				      plane_state->src_y);
		ret |= plane_add_prop(req, plane, WDRM_PLANE_SRC_W,
				      plane_state->src_w);
		ret |= plane_add_prop(req, plane, WDRM_PLANE_SRC_H,
				      plane_state->src_h);
		ret |= plane_add_prop(req, plane, WDRM_PLANE_CRTC_X,
				      plane_state->dest_x);
		ret |= plane_add_prop(req, plane, WDRM_PLANE_CRTC_Y,
				      plane_state->dest_y);
		ret |= plane_add_prop(req, plane, WDRM_PLANE_CRTC_W,
				      plane_state->dest_w);
		ret |= plane_add_prop(req, plane, WDRM_PLANE_CRTC_H,
				      plane_state->dest_h);
		if (plane->props[WDRM_PLANE_FB_DAMAGE_CLIPS].prop_id != 0)
			ret |= plane_add_prop(req, plane, WDRM_PLANE_FB_DAMAGE_CLIPS,
					      plane_state->damage_blob_id);

		if (plane_state->fb && plane_state->fb->format)
			pinfo = plane_state->fb->format;

		drm_debug(b, "\t\t\t[PLANE:%lu] FORMAT: %s\n",
			  (unsigned long) plane->plane_id,
			  pinfo ? pinfo->drm_format_name : "UNKNOWN");

		if (plane_state->in_fence_fd >= 0) {
			ret |= plane_add_prop(req, plane,
					      WDRM_PLANE_IN_FENCE_FD,
					      plane_state->in_fence_fd);
		}

		if (plane->props[WDRM_PLANE_ROTATION].prop_id != 0)
			ret |= plane_add_prop(req, plane, WDRM_PLANE_ROTATION,
					      plane_state->rotation);

		/* do note, that 'invented' zpos values are set as immutable */
		if (plane_state->zpos != DRM_PLANE_ZPOS_INVALID_PLANE &&
		    plane_state->plane->zpos_min != plane_state->plane->zpos_max)
			ret |= plane_add_prop(req, plane,
					      WDRM_PLANE_ZPOS,
					      plane_state->zpos);

		/*Plane-alpha support */
		if (plane->alpha_max != plane->alpha_min)
			ret |= plane_add_prop(req, plane,
					      WDRM_PLANE_ALPHA,
					      plane_state->alpha);

		if (ret != 0) {
			weston_log("couldn't set plane state\n");
			return ret;
		}
	}

	return 0;
}

static void
drm_pending_state_clear_tearing(struct drm_pending_state *pending_state)
{
	struct drm_output_state *output_state;

	wl_list_for_each(output_state, &pending_state->output_list, link) {
		if (output_state->output->virtual)
			continue;
		output_state->tear = false;
	}
}

/**
 * Helper function used only by drm_pending_state_apply, with the same
 * guarantees and constraints as that function.
 */
static int
drm_pending_state_apply_atomic(struct drm_pending_state *pending_state,
			       enum drm_state_apply_mode mode)
{
	struct drm_device *device = pending_state->device;
	struct drm_backend *b = device->backend;
	struct drm_output_state *output_state, *tmp;
	struct drm_plane *plane;
	drmModeAtomicReq *req = drmModeAtomicAlloc();
	uint32_t flags, tear_flag = 0;
	bool may_tear = true;
	int ret = 0;

	if (!req)
		return -1;

	switch (mode) {
	case DRM_STATE_APPLY_SYNC:
		flags = 0;
		break;
	case DRM_STATE_APPLY_ASYNC:
		flags = DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK;
		break;
	case DRM_STATE_TEST_ONLY:
		flags = DRM_MODE_ATOMIC_TEST_ONLY;
		break;
	}

	if (device->state_invalid) {
		struct weston_head *head_base;
		struct drm_head *head;
		struct drm_crtc *crtc;
		uint32_t connector_id;
		int err;

		drm_debug(b, "\t\t[atomic] previous state invalid; "
			     "starting with fresh state\n");

		/* If we need to reset all our state (e.g. because we've
		 * just started, or just been VT-switched in), explicitly
		 * disable all the CRTCs and connectors we aren't using. */
		wl_list_for_each(head_base,
				 &b->compositor->head_list, compositor_link) {
			struct drm_property_info *info;
			head = to_drm_head(head_base);
			if (!head)
				continue;

			if (weston_head_is_enabled(head_base))
				continue;

			connector_id = head->connector.connector_id;
			if (head->connector.device != device)
				continue;

			drm_debug(b, "\t\t[atomic] disabling inactive head %s\n",
				  head_base->name);

			info = &head->connector.props[WDRM_CONNECTOR_CRTC_ID];
			err = drmModeAtomicAddProperty(req, connector_id,
						       info->prop_id, 0);
			drm_debug(b, "\t\t\t[CONN:%lu] %lu (%s) -> 0\n",
				  (unsigned long) connector_id,
				  (unsigned long) info->prop_id,
				  info->name);
			if (err <= 0)
				ret = -1;
		}

		wl_list_for_each(crtc, &device->crtc_list, link) {
			struct drm_property_info *info;
			drmModeObjectProperties *props;
			uint64_t active;

			/* Ignore CRTCs that are in use */
			if (crtc->output)
				continue;

			/* We can't emit a disable on a CRTC that's already
			 * off, as the kernel will refuse to generate an event
			 * for an off->off state and fail the commit.
			 */
			props = drmModeObjectGetProperties(device->drm.fd,
							   crtc->crtc_id,
							   DRM_MODE_OBJECT_CRTC);
			if (!props) {
				ret = -1;
				continue;
			}

			info = &crtc->props_crtc[WDRM_CRTC_ACTIVE];
			active = drm_property_get_value(info, props, 0);
			drmModeFreeObjectProperties(props);
			if (active == 0)
				continue;

			drm_debug(b, "\t\t[atomic] disabling unused CRTC %lu\n",
				  (unsigned long) crtc->crtc_id);

			ret |= crtc_add_prop(req, crtc, WDRM_CRTC_ACTIVE, 0);
			ret |= crtc_add_prop(req, crtc, WDRM_CRTC_MODE_ID, 0);
		}

		/* Disable all the planes; planes which are being used will
		 * override this state in the output-state application. */
		wl_list_for_each(plane, &device->plane_list, link) {
			drm_debug(b, "\t\t[atomic] starting with plane %lu disabled\n",
				  (unsigned long) plane->plane_id);
			plane_add_prop(req, plane, WDRM_PLANE_CRTC_ID, 0);
			plane_add_prop(req, plane, WDRM_PLANE_FB_ID, 0);
		}

		flags |= DRM_MODE_ATOMIC_ALLOW_MODESET;
	}

	wl_list_for_each(output_state, &pending_state->output_list, link) {
		if (output_state->output->virtual)
			continue;
		if (mode == DRM_STATE_APPLY_SYNC)
			assert(output_state->dpms == WESTON_DPMS_OFF);
		may_tear &= output_state->tear;
		ret |= drm_output_apply_state_atomic(output_state, req, &flags);
	}

	if (ret != 0) {
		weston_log("atomic: couldn't compile atomic state\n");
		goto out;
	}
	if (may_tear)
		tear_flag = DRM_MODE_PAGE_FLIP_ASYNC;

	ret = drmModeAtomicCommit(device->drm.fd, req, flags | tear_flag,
				  device);
	drm_debug(b, "[atomic] drmModeAtomicCommit\n");
	if (ret != 0 && may_tear && mode == DRM_STATE_TEST_ONLY) {
		/* If we failed trying to set up a tearing commit, try again
		 * without tearing. If that succeeds, knock the tearing flag
		 * out of our state in case we were testing for a later commit.
		 */
		drm_debug(b, "[atomic] drmModeAtomicCommit (no tear fallback)\n");
		ret = drmModeAtomicCommit(device->drm.fd, req, flags, device);
		if (ret == 0)
			drm_pending_state_clear_tearing(pending_state);
	}
	/* Test commits do not take ownership of the state; return
	 * without freeing here. */
	if (mode == DRM_STATE_TEST_ONLY) {
		drmModeAtomicFree(req);
		return ret;
	}

	if (ret != 0) {
		wl_list_for_each(output_state, &pending_state->output_list, link)
			if (drm_output_get_writeback_state(output_state->output) != DRM_OUTPUT_WB_SCREENSHOT_OFF)
				drm_writeback_fail_screenshot(output_state->output->wb_state,
							      "drm: atomic commit failed");
		weston_log("atomic: couldn't commit new state: %s\n",
			   strerror(errno));
		goto out;
	}

	wl_list_for_each_safe(output_state, tmp, &pending_state->output_list,
			      link)
		drm_output_assign_state(output_state, mode);

	device->state_invalid = false;

	assert(wl_list_empty(&pending_state->output_list));

out:
	drmModeAtomicFree(req);
	drm_pending_state_free(pending_state);
	return ret;
}

/**
 * Tests a pending state, to see if the kernel will accept the update as
 * constructed.
 *
 * Using atomic modesetting, the kernel performs the same checks as it would
 * on a real commit, returning success or failure without actually modifying
 * the running state. It does not return -EBUSY if there are pending updates
 * in flight, so states may be tested at any point, however this means a
 * state which passed testing may fail on a real commit if the timing is not
 * respected (e.g. committing before the previous commit has completed).
 *
 * Without atomic modesetting, we have no way to check, so we optimistically
 * claim it will work.
 *
 * Unlike drm_pending_state_apply() and drm_pending_state_apply_sync(), this
 * function does _not_ take ownership of pending_state, nor does it clear
 * state_invalid.
 */
int
drm_pending_state_test(struct drm_pending_state *pending_state)
{
	struct drm_device *device = pending_state->device;

	if (device->atomic_modeset)
		return drm_pending_state_apply_atomic(pending_state,
						      DRM_STATE_TEST_ONLY);

	/* We have no way to test state before application on the legacy
	 * modesetting API, so just claim it succeeded. */
	return 0;
}

/**
 * Applies all of a pending_state asynchronously: the primary entry point for
 * applying KMS state to a device. Updates the state for all outputs in the
 * pending_state, as well as disabling any unclaimed outputs.
 *
 * Unconditionally takes ownership of pending_state, and clears state_invalid.
 */
int
drm_pending_state_apply(struct drm_pending_state *pending_state)
{
	struct drm_device *device = pending_state->device;
	struct drm_backend *b = device->backend;
	struct drm_output_state *output_state, *tmp;
	struct drm_crtc *crtc;

	if (wl_list_empty(&pending_state->output_list)) {
		drm_pending_state_free(pending_state);
		return 0;
	}

	if (device->atomic_modeset)
		return drm_pending_state_apply_atomic(pending_state,
						      DRM_STATE_APPLY_ASYNC);

	if (device->state_invalid) {
		/* If we need to reset all our state (e.g. because we've
		 * just started, or just been VT-switched in), explicitly
		 * disable all the CRTCs we aren't using. This also disables
		 * all connectors on these CRTCs, so we don't need to do that
		 * separately with the pre-atomic API. */
		wl_list_for_each(crtc, &device->crtc_list, link) {
			if (crtc->output)
				continue;
			drmModeSetCrtc(device->drm.fd, crtc->crtc_id, 0, 0, 0,
				       NULL, 0, NULL);
		}
	}

	wl_list_for_each_safe(output_state, tmp, &pending_state->output_list,
			      link) {
		struct drm_output *output = output_state->output;
		int ret;

		if (output->virtual) {
			drm_output_assign_state(output_state,
						DRM_STATE_APPLY_ASYNC);
			continue;
		}

		ret = drm_output_apply_state_legacy(output_state);
		if (ret != 0) {
			weston_log("Couldn't apply state for output %s\n",
				   output->base.name);
			weston_output_repaint_failed(&output->base);
			drm_output_state_free(output->state_cur);
			output->state_cur = drm_output_state_alloc(output, NULL);
			device->state_invalid = true;
			if (b->compositor->renderer->type == WESTON_RENDERER_GL) {
				drm_output_fini_egl(output);
				drm_output_init_egl(output, b);
			}
		}
	}

	device->state_invalid = false;

	assert(wl_list_empty(&pending_state->output_list));

	drm_pending_state_free(pending_state);

	return 0;
}

/**
 * The synchronous version of drm_pending_state_apply. May only be used to
 * disable outputs. Does so synchronously: the request is guaranteed to have
 * completed on return, and the output will not be touched afterwards.
 *
 * Unconditionally takes ownership of pending_state, and clears state_invalid.
 */
int
drm_pending_state_apply_sync(struct drm_pending_state *pending_state)
{
	struct drm_device *device = pending_state->device;
	struct drm_output_state *output_state, *tmp;
	struct drm_crtc *crtc;

	if (device->atomic_modeset)
		return drm_pending_state_apply_atomic(pending_state,
						      DRM_STATE_APPLY_SYNC);

	if (device->state_invalid) {
		/* If we need to reset all our state (e.g. because we've
		 * just started, or just been VT-switched in), explicitly
		 * disable all the CRTCs we aren't using. This also disables
		 * all connectors on these CRTCs, so we don't need to do that
		 * separately with the pre-atomic API. */
		wl_list_for_each(crtc, &device->crtc_list, link) {
			if (crtc->output)
				continue;
			drmModeSetCrtc(device->drm.fd, crtc->crtc_id, 0, 0, 0,
				       NULL, 0, NULL);
		}
	}

	wl_list_for_each_safe(output_state, tmp, &pending_state->output_list,
			      link) {
		int ret;

		assert(output_state->dpms == WESTON_DPMS_OFF);
		ret = drm_output_apply_state_legacy(output_state);
		if (ret != 0) {
			weston_log("Couldn't apply state for output %s\n",
				   output_state->output->base.name);
		}
	}

	device->state_invalid = false;

	assert(wl_list_empty(&pending_state->output_list));

	drm_pending_state_free(pending_state);

	return 0;
}

void
drm_output_update_msc(struct drm_output *output, unsigned int seq)
{
	uint32_t msc_hi = output->base.msc >> 32;

	if (seq < (output->base.msc & 0xffffffff))
		msc_hi++;

	output->base.msc = u64_from_u32s(msc_hi, seq);
}

static void
page_flip_handler(int fd, unsigned int frame,
		  unsigned int sec, unsigned int usec, void *data)
{
	struct drm_output *output = data;
	struct drm_device *device = output->device;
	uint32_t flags = WP_PRESENTATION_FEEDBACK_KIND_VSYNC |
			 WP_PRESENTATION_FEEDBACK_KIND_HW_COMPLETION |
			 WP_PRESENTATION_FEEDBACK_KIND_HW_CLOCK;

	drm_output_update_msc(output, frame);

	assert(!device->atomic_modeset);
	assert(output->page_flip_pending);
	output->page_flip_pending = false;

	drm_output_update_complete(output, flags, sec, usec);
}

static void
atomic_flip_handler(int fd, unsigned int frame, unsigned int sec,
		    unsigned int usec, unsigned int crtc_id, void *data)
{
	struct drm_device *device = data;
	struct drm_backend *b = device->backend;
	struct weston_compositor *ec = b->compositor;
	struct drm_crtc *crtc;
	struct drm_output *output;
	struct timespec now;
	uint32_t flags = WP_PRESENTATION_FEEDBACK_KIND_VSYNC |
			 WP_PRESENTATION_FEEDBACK_KIND_HW_COMPLETION |
			 WP_PRESENTATION_FEEDBACK_KIND_HW_CLOCK;

	crtc = drm_crtc_find(device, crtc_id);
	assert(crtc);

	output = crtc->output;

	/* During the initial modeset, we can disable CRTCs which we don't
	 * actually handle during normal operation; this will give us events
	 * for unknown outputs. Ignore them. */
	if (!output || !output->base.enabled)
		return;

	drm_output_update_msc(output, frame);

	if (output->state_cur->tear) {
		/* When tearing we might not get accurate timestamps from
		 * the driver, so just use whatever time it is now.
		 * Note: This could actually be after a vblank that occured
		 * after entering this function.
		 */
		weston_compositor_read_presentation_clock(ec, &now);
		sec = now.tv_sec;
		usec = now.tv_nsec / 1000;
	}

	drm_debug(b, "[atomic][CRTC:%u] flip processing started\n", crtc_id);
	assert(device->atomic_modeset);
	assert(output->atomic_complete_pending);
	output->atomic_complete_pending = false;

	drm_output_update_complete(output, flags, sec, usec);
	drm_debug(b, "[atomic][CRTC:%u] flip processing completed\n", crtc_id);
}

int
on_drm_input(int fd, uint32_t mask, void *data)
{
	struct drm_device *device = data;
	struct drm_writeback_state *state;
	struct drm_crtc *crtc;
	bool wait_wb_completion = false;
	drmEventContext evctx;

	/* If we have a pending writeback job for this output, we can't continue
	 * with the repaint loop. The KMS UAPI docs says that we need to wait
	 * until the writeback is over before we send a new atomic commit that
	 * uses the KMS objects (CRTC, planes, etc) in use by the writeback. */
	wl_list_for_each(crtc, &device->crtc_list, link) {
		state = crtc->output ? crtc->output->wb_state : NULL;
		if (state && drm_writeback_should_wait_completion(state))
			wait_wb_completion = true;
	}
	if (wait_wb_completion)
		return 1;

	memset(&evctx, 0, sizeof evctx);
	evctx.version = 3;
	if (device->atomic_modeset)
		evctx.page_flip_handler2 = atomic_flip_handler;
	else
		evctx.page_flip_handler = page_flip_handler;
	drmHandleEvent(fd, &evctx);

	return 1;
}

int
init_kms_caps(struct drm_device *device)
{
	struct drm_backend *b = device->backend;
	struct weston_compositor *compositor = b->compositor;
	uint64_t cap;
	int ret;

	weston_log("using %s\n", device->drm.filename);

	ret = drmGetCap(device->drm.fd, DRM_CAP_TIMESTAMP_MONOTONIC, &cap);
	if (ret != 0 || cap != 1) {
		weston_log("Error: kernel DRM KMS does not support DRM_CAP_TIMESTAMP_MONOTONIC.\n");
		return -1;
	}

	if (weston_compositor_set_presentation_clock(compositor, CLOCK_MONOTONIC) < 0) {
		weston_log("Error: failed to set presentation clock to CLOCK_MONOTONIC.\n");
		return -1;
	}

	ret = drmGetCap(device->drm.fd, DRM_CAP_CURSOR_WIDTH, &cap);
	if (ret == 0)
		device->cursor_width = cap;
	else
		device->cursor_width = 64;

	ret = drmGetCap(device->drm.fd, DRM_CAP_CURSOR_HEIGHT, &cap);
	if (ret == 0)
		device->cursor_height = cap;
	else
		device->cursor_height = 64;

	ret = drmSetClientCap(device->drm.fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if (ret) {
		weston_log("Error: drm card doesn't support universal planes!\n");
		return -1;
	}

	if (!getenv("WESTON_DISABLE_ATOMIC")) {
		ret = drmGetCap(device->drm.fd, DRM_CAP_CRTC_IN_VBLANK_EVENT, &cap);
		if (ret != 0)
			cap = 0;
		ret = drmSetClientCap(device->drm.fd, DRM_CLIENT_CAP_ATOMIC, 1);
		device->atomic_modeset = ((ret == 0) && (cap == 1));
	}
	weston_log("DRM: %s atomic modesetting\n",
		   device->atomic_modeset ? "supports" : "does not support");

	if (!getenv("WESTON_DISABLE_GBM_MODIFIERS")) {
		ret = drmGetCap(device->drm.fd, DRM_CAP_ADDFB2_MODIFIERS, &cap);
		if (ret == 0)
			device->fb_modifiers = cap;
	}
	weston_log("DRM: %s GBM modifiers\n",
		   device->fb_modifiers ? "supports" : "does not support");

	drmSetClientCap(device->drm.fd, DRM_CLIENT_CAP_WRITEBACK_CONNECTORS, 1);

	ret = drmGetCap(device->drm.fd, DRM_CAP_ATOMIC_ASYNC_PAGE_FLIP, &cap);
	if (ret != 0)
		cap = 0;
	device->tearing_supported = cap;

	/*
	 * KMS support for hardware planes cannot properly synchronize
	 * without nuclear page flip. Without nuclear/atomic, hw plane
	 * and cursor plane updates would either tear or cause extra
	 * waits for vblanks which means dropping the compositor framerate
	 * to a fraction. For cursors, it's not so bad, so they are
	 * enabled.
	 */
	if (!device->atomic_modeset || getenv("WESTON_FORCE_RENDERER"))
		device->sprites_are_broken = true;

	ret = drmSetClientCap(device->drm.fd, DRM_CLIENT_CAP_ASPECT_RATIO, 1);
	device->aspect_ratio_supported = (ret == 0);
	weston_log("DRM: %s picture aspect ratio\n",
		   device->aspect_ratio_supported ? "supports" : "does not support");

	return 0;
}
