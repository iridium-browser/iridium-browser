/*
 * Copyright © 2020 Microsoft
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include <stdio.h>
#include <wchar.h>
#include <strings.h>

#include "rdp.h"

#include "shared/xalloc.h"

static bool
match_primary(struct rdp_backend *rdp, rdpMonitor *a, rdpMonitor *b)
{
	if (a->is_primary && b->is_primary)
		return true;

	return false;
}

static bool
match_dimensions(struct rdp_backend *rdp, rdpMonitor *a, rdpMonitor *b)
{
	int scale_a = a->attributes.desktopScaleFactor;
	int scale_b = b->attributes.desktopScaleFactor;


	if (a->width != b->width ||
	    a->height != b->height ||
	    scale_a != scale_b)
		return false;

	return true;
}

static bool
match_position(struct rdp_backend *rdp, rdpMonitor *a, rdpMonitor *b)
{
	if (a->x != b->x ||
	    a->y != b->y)
		return false;

	return true;
}

static bool
match_exact(struct rdp_backend *rdp, rdpMonitor *a, rdpMonitor *b)
{
	if (match_dimensions(rdp, a, b) &&
	    match_position(rdp, a, b))
		return true;

	return false;
}

static bool
match_any(struct rdp_backend *rdp, rdpMonitor *a, rdpMonitor *b)
{
	return true;
}

static void
update_head(struct rdp_backend *rdp, struct rdp_head *head, rdpMonitor *config)
{
	struct weston_mode mode = {};
	int scale;
	bool changed = false;

	head->matched = true;
	scale = config->attributes.desktopScaleFactor / 100;
	scale = scale ? scale : 1;

	if (!match_position(rdp, &head->config, config))
		changed = true;

	if (!match_dimensions(rdp, &head->config, config)) {
		mode.flags = WL_OUTPUT_MODE_PREFERRED;
		mode.width = config->width;
		mode.height = config->height;
		mode.refresh = rdp->rdp_monitor_refresh_rate;
		weston_output_mode_set_native(head->base.output,
					      &mode, scale);
		changed = true;
	}

	if (changed) {
		weston_head_set_device_changed(&head->base);
	}
	head->config = *config;
}

static void
match_heads(struct rdp_backend *rdp, rdpMonitor *config, uint32_t count,
	    int *done,
	    bool (*cmp)(struct rdp_backend *rdp, rdpMonitor *a, rdpMonitor *b))
{
	struct weston_head *iter;
	struct rdp_head *current;
	uint32_t i;

	wl_list_for_each(iter, &rdp->compositor->head_list, compositor_link) {
		current = to_rdp_head(iter);
		if (!current || current->matched)
			continue;

		for (i = 0; i < count; i++) {
			if (*done & (1 << i))
				continue;

			if (cmp(rdp, &current->config, &config[i])) {
				*done |= 1 << i;
				update_head(rdp, current, &config[i]);
				break;
			}
		}
	}
}

static void
disp_layout_change(freerdp_peer *client, rdpMonitor *config, UINT32 monitorCount)
{
	RdpPeerContext *peerCtx = (RdpPeerContext *)client->context;
	struct rdp_backend *b = peerCtx->rdpBackend;
	struct rdp_head *current;
	struct weston_head *iter, *tmp;
	pixman_region32_t desktop;
	int done = 0;

	assert_compositor_thread(b);

	pixman_region32_init(&desktop);

	/* Prune heads that were never enabled, and flag heads as unmatched  */
	wl_list_for_each_safe(iter, tmp, &b->compositor->head_list, compositor_link) {
		current = to_rdp_head(iter);
		if (!current)
			continue;

		if (!iter->output) {
			rdp_head_destroy(iter);
			continue;
		}
		current->matched = false;
	}

	/* We want the primary head to remain primary - it
	 * should always be rdp-0.
	  */
	match_heads(b, config, monitorCount, &done, match_primary);

	/* Look for any exact match */
	match_heads(b, config, monitorCount, &done, match_exact);

	/* Match first head with the same dimensions */
	match_heads(b, config, monitorCount, &done, match_dimensions);

	/* Match head with the same position */
	match_heads(b, config, monitorCount, &done, match_position);

	/* Pick any available head */
	match_heads(b, config, monitorCount, &done, match_any);

	/* Destroy any heads we won't be using */
	wl_list_for_each_safe(iter, tmp, &b->compositor->head_list, compositor_link) {
		current = to_rdp_head(iter);
		if (!current)
			continue;

		if (!current->matched)
			rdp_head_destroy(iter);
	}


	for (uint32_t i = 0; i < monitorCount; i++) {
		/* accumulate monitor layout */
		pixman_region32_union_rect(&desktop, &desktop,
					   config[i].x,
					   config[i].y,
					   config[i].width,
					   config[i].height);

		/* Create new heads for any without matches */
		if (!(done & (1 << i)))
			rdp_head_create(b, &config[i]);
	}
	peerCtx->desktop_left = desktop.extents.x1;
	peerCtx->desktop_top = desktop.extents.y1;
	peerCtx->desktop_width = desktop.extents.x2 - desktop.extents.x1;
	peerCtx->desktop_height = desktop.extents.y2 - desktop.extents.y1;
	pixman_region32_fini(&desktop);
}

static bool
disp_sanity_check_layout(RdpPeerContext *peerCtx, rdpMonitor *config, uint32_t count)
{
	struct rdp_backend *b = peerCtx->rdpBackend;
	uint32_t primaryCount = 0;
	uint32_t i;

	/* dump client monitor topology */
	rdp_debug(b, "%s:---INPUT---\n", __func__);
	for (i = 0; i < count; i++) {
		int scale = config[i].attributes.desktopScaleFactor / 100;

		rdp_debug(b, "	rdpMonitor[%d]: x:%d, y:%d, width:%d, height:%d, is_primary:%d\n",
			i, config[i].x, config[i].y,
			   config[i].width, config[i].height,
			   config[i].is_primary);
		rdp_debug(b, "	rdpMonitor[%d]: physicalWidth:%d, physicalHeight:%d, orientation:%d\n",
			i, config[i].attributes.physicalWidth,
			   config[i].attributes.physicalHeight,
			   config[i].attributes.orientation);
		rdp_debug(b, "	rdpMonitor[%d]: desktopScaleFactor:%d, deviceScaleFactor:%d\n",
			i, config[i].attributes.desktopScaleFactor,
			   config[i].attributes.deviceScaleFactor);

		rdp_debug(b, "	rdpMonitor[%d]: scale:%d\n",
			i, scale);
	}

	for (i = 0; i < count; i++) {
		/* make sure there is only one primary and its position at client */
		if (config[i].is_primary) {
			/* count number of primary */
			if (++primaryCount > 1) {
				weston_log("%s: RDP client reported unexpected primary count (%d)\n",
					   __func__, primaryCount);
				return false;
			}
			/* primary must be at (0,0) in client space */
			if (config[i].x != 0 || config[i].y != 0) {
				weston_log("%s: RDP client reported primary is not at (0,0) but (%d,%d).\n",
					   __func__, config[i].x, config[i].y);
				return false;
			}
		}
	}
	return true;
}

bool
handle_adjust_monitor_layout(freerdp_peer *client, int monitor_count, rdpMonitor *monitors)
{
	RdpPeerContext *peerCtx = (RdpPeerContext *)client->context;

	if (!disp_sanity_check_layout(peerCtx, monitors, monitor_count))
		return true;

	disp_layout_change(client, monitors, monitor_count);

	return true;
}

static bool
rect_contains(int32_t px, int32_t py,  int32_t rx, int32_t ry,
	      int32_t width, int32_t height)
{
	if (px < rx)
		return false;
	if (py < ry)
		return false;
	if (px >= rx + width)
		return false;
	if (py >= ry + height)
		return false;

	return true;
}

static bool
rdp_head_contains(struct rdp_head *rdp_head, int32_t x, int32_t y)
{
	rdpMonitor *config = &rdp_head->config;

	/* If we're forcing RDP desktop size then we don't have
	 * useful information in the monitor structs, but we
	 * can rely on the output settings in that case.
	 */
	if (config->width == 0) {
		struct weston_head *head = &rdp_head->base;
		struct weston_output *output = head->output;

		if (!output)
			return false;

		return rect_contains(x, y, output->x, output->y,
				     output->width * output->scale,
				     output->height * output->scale);
	}

	return rect_contains(x, y, config->x, config->y,
			     config->width, config->height);
}

/* Input x/y in client space, output x/y in weston space */
struct weston_output *
to_weston_coordinate(RdpPeerContext *peerContext, int32_t *x, int32_t *y)
{
	struct rdp_backend *b = peerContext->rdpBackend;
	int sx = *x, sy = *y;
	struct weston_head *head_iter;

	/* First, find which monitor contains this x/y. */
	wl_list_for_each(head_iter, &b->compositor->head_list, compositor_link) {
		struct rdp_head *head = to_rdp_head(head_iter);

		if (!head)
			continue;

		if (rdp_head_contains(head, sx, sy)) {
			struct weston_output *output = head->base.output;
			float scale = 1.0f / head->base.output->scale;

			sx -= head->config.x;
			sy -= head->config.y;
			/* scale x/y to client output space. */
			sx *= scale;
			sy *= scale;
			/* translate x/y to offset of this output in weston space. */
			sx += output->x;
			sy += output->y;
			rdp_debug_verbose(b, "%s: (x:%d, y:%d) -> (sx:%d, sy:%d) at head:%s\n",
					  __func__, *x, *y, sx, sy, head->base.name);
			*x = sx;
			*y = sy;
			return output;
		}
	}
	/* x/y is outside of any monitors. */
	return NULL;
}
