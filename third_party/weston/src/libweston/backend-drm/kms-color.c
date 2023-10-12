/*
 * Copyright 2021-2022 Collabora, Ltd.
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
#include <libweston/libweston.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "drm-internal.h"

static inline uint16_t
color_xy_to_u16(float v)
{
	assert(v >= 0.0f);
	assert(v <= 1.0f);
	/*
	 * CTA-861-G
	 * 6.9.1 Static Metadata Type 1
	 * chromaticity coordinate encoding
	 */
	return (uint16_t)round(v * 50000.0);
}

static inline uint16_t
nits_to_u16(float nits)
{
	assert(nits >= 1.0f);
	assert(nits <= 65535.0f);
	/*
	 * CTA-861-G
	 * 6.9.1 Static Metadata Type 1
	 * max display mastering luminance, max content light level,
	 * max frame-average light level
	 */
	return (uint16_t)round(nits);
}

static inline uint16_t
nits_to_u16_dark(float nits)
{
	assert(nits >= 0.0001f);
	assert(nits <= 6.5535f);
	/*
	 * CTA-861-G
	 * 6.9.1 Static Metadata Type 1
	 * min display mastering luminance
	 */
	return (uint16_t)round(nits * 10000.0);
}

static void
weston_hdr_metadata_type1_to_kms(struct hdr_metadata_infoframe *dst,
				 const struct weston_hdr_metadata_type1 *src)
{
	if (src->group_mask & WESTON_HDR_METADATA_TYPE1_GROUP_PRIMARIES) {
		unsigned i;

		for (i = 0; i < 3; i++) {
			dst->display_primaries[i].x = color_xy_to_u16(src->primary[i].x);
			dst->display_primaries[i].y = color_xy_to_u16(src->primary[i].y);
		}
	}

	if (src->group_mask & WESTON_HDR_METADATA_TYPE1_GROUP_WHITE) {
		dst->white_point.x = color_xy_to_u16(src->white.x);
		dst->white_point.y = color_xy_to_u16(src->white.y);
	}

	if (src->group_mask & WESTON_HDR_METADATA_TYPE1_GROUP_MAXDML)
		dst->max_display_mastering_luminance = nits_to_u16(src->maxDML);

	if (src->group_mask & WESTON_HDR_METADATA_TYPE1_GROUP_MINDML)
		dst->min_display_mastering_luminance = nits_to_u16_dark(src->minDML);

	if (src->group_mask & WESTON_HDR_METADATA_TYPE1_GROUP_MAXCLL)
		dst->max_cll = nits_to_u16(src->maxCLL);

	if (src->group_mask & WESTON_HDR_METADATA_TYPE1_GROUP_MAXFALL)
		dst->max_fall = nits_to_u16(src->maxFALL);
}

int
drm_output_ensure_hdr_output_metadata_blob(struct drm_output *output)
{
	struct drm_device *device = output->device;
	const struct weston_hdr_metadata_type1 *src;
	struct hdr_output_metadata meta;
	uint32_t blob_id = 0;
	int ret;

	if (output->hdr_output_metadata_blob_id &&
	    output->ackd_color_outcome_serial == output->base.color_outcome_serial)
		return 0;

	src = weston_output_get_hdr_metadata_type1(&output->base);

	/*
	 * Set up the data for Dynamic Range and Mastering InfoFrame,
	 * CTA-861-G, a.k.a the static HDR metadata.
	 */

	memset(&meta, 0, sizeof meta);

	meta.metadata_type = 0; /* Static Metadata Type 1 */

	/* Duplicated field in UABI struct */
	meta.hdmi_metadata_type1.metadata_type = meta.metadata_type;

	switch (output->base.eotf_mode) {
	case WESTON_EOTF_MODE_NONE:
		assert(0 && "bad eotf_mode: none");
		return -1;
	case WESTON_EOTF_MODE_SDR:
		/*
		 * Do not send any static HDR metadata. Video sinks should
		 * respond by switching to traditional SDR mode. If they
		 * do not, the kernel should fix that up.
		 */
		assert(output->hdr_output_metadata_blob_id == 0);
		return 0;
	case WESTON_EOTF_MODE_TRADITIONAL_HDR:
		meta.hdmi_metadata_type1.eotf = 1; /* from CTA-861-G */
		break;
	case WESTON_EOTF_MODE_ST2084:
		meta.hdmi_metadata_type1.eotf = 2; /* from CTA-861-G */
		weston_hdr_metadata_type1_to_kms(&meta.hdmi_metadata_type1, src);
		break;
	case WESTON_EOTF_MODE_HLG:
		meta.hdmi_metadata_type1.eotf = 3; /* from CTA-861-G */
		break;
	}

	if (meta.hdmi_metadata_type1.eotf == 0) {
		assert(0 && "bad eotf_mode");
		return -1;
	}

	ret = drmModeCreatePropertyBlob(device->drm.fd,
					&meta, sizeof meta, &blob_id);
	if (ret != 0) {
		weston_log("Error: failed to create KMS blob for HDR metadata on output '%s': %s\n",
			   output->base.name, strerror(-ret));
		return -1;
	}

	drmModeDestroyPropertyBlob(device->drm.fd,
				   output->hdr_output_metadata_blob_id);

	output->hdr_output_metadata_blob_id = blob_id;
	output->ackd_color_outcome_serial = output->base.color_outcome_serial;

	return 0;
}
