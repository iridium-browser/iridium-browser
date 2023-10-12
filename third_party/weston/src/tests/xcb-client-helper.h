/*
 * Copyright 2022 Collabora, Ltd.
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

#pragma once

#include "config.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "shared/xcb-xwayland.h"
#include <pixman.h>
#include <wayland-client.h>

#include <xcb/xcb.h>
#include <xcb/xcb_cursor.h>

enum w_state {
	CREATED			= 1 << 0,
	MAPPED			= 1 << 1,
	UNMAPPED		= 1 << 2,
	PROPERTY_NAME		= 1 << 3,
	DESTROYED		= 1 << 4,
	EXPOSE			= 1 << 5,
	REPARENT		= 1 << 6,
};

struct window_state {
	uint8_t event;
	enum w_state pending_state;
	xcb_drawable_t wid;
	struct wl_list link;	/** window_x11.tentative_state::pending_events_list */
};

struct connection_x11 {
	struct atom_x11 *atoms;
	struct xcb_connection_t *connection;
};

struct window_x11 {
	struct window_x11 *parent;
	struct xcb_screen_t *screen;
	struct connection_x11 *conn;
	bool handle_in_progress;

	xcb_drawable_t root_win_id;	/* screen root */
	xcb_drawable_t win_id;		/* this window */
	xcb_drawable_t parent_win_id;	/* the parent, if set */

	xcb_gcontext_t background;

	xcb_cursor_context_t *ctx;
	xcb_cursor_t cursor;

	int width;
	int height;

	int pos_x;
	int pos_y;

	pixman_color_t bg_color;

	/* these track what the X11 client does */
	struct {
		/* pending queue events */
		struct wl_list pending_events_list;	/** window_state::link */
	} tentative_state;

	/* these track what we got back from the server */
	struct {
		/* applied, received event */
		uint32_t win_state;
	} state;

	struct wl_list window_list;
	struct wl_list window_link;

	xcb_window_t frame_id;
};

void
window_x11_map(struct window_x11 *window);

void
window_x11_unmap(struct window_x11 *window);


struct connection_x11 *
create_x11_connection(void);

void
destroy_x11_connection(struct connection_x11 *conn);

struct window_x11 *
create_x11_window(int width, int height, int pos_x, int pos_y, struct connection_x11 *conn,
		  pixman_color_t bg_color, struct window_x11 *parent);
void
destroy_x11_window(struct window_x11 *window);

void
window_x11_set_win_name(struct window_x11 *window, const char *name);

xcb_get_property_reply_t *
window_x11_dump_prop(struct window_x11 *window, xcb_drawable_t win, xcb_atom_t atom);

void
handle_event_set_pending(struct window_x11 *window, uint8_t event,
			 enum w_state pending_state, xcb_drawable_t wid);

void
handle_event_remove_pending(struct window_state *wstate);

int
handle_events_x11(struct window_x11 *window);

struct atom_x11 *
window_get_atoms(struct window_x11 *win);

struct xcb_connection_t *
window_get_connection(struct window_x11 *win);

void
window_x11_notify_for_root_events(struct window_x11 *window);

/* note that the flag is already bitshiftted */
static inline bool
window_state_has_flag(struct window_x11 *win, enum w_state flag)
{
	return (win->state.win_state & flag) == flag;
}

static inline void
window_state_set_flag(struct window_x11 *win, enum w_state flag)
{
	win->state.win_state |= flag;
}

static inline void
window_state_clear_flag(struct window_x11 *win, enum w_state flag)
{
	win->state.win_state &= ~flag;
}

static inline void
window_state_clear_all_flags(struct window_x11 *win)
{
	win->state.win_state = 0;
}

/**
 * A wrapper over handle_events_x11() to check if the pending flag has been
 * set, that waits for events calling handle_events_x11() and that verifies
 * afterwards if the flag has indeed been applied.
 */
static inline void
handle_events_and_check_flags(struct window_x11 *win, enum w_state flag)
{
	struct wl_list *pending_events =
		&win->tentative_state.pending_events_list;
	struct window_state *wstate;
	bool found_pending_flag = false;

	wl_list_for_each(wstate, pending_events, link) {
		if ((wstate->pending_state & flag) == flag)
			found_pending_flag = true;
	}
	assert(found_pending_flag);

	handle_events_x11(win);
	assert(window_state_has_flag(win, flag));
}
