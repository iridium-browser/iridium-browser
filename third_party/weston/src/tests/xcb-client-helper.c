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
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <poll.h>

#include <time.h>

#include <wayland-client.h>
#include "test-config.h"
#include "shared/os-compatibility.h"
#include "shared/helpers.h"
#include "shared/xalloc.h"
#include "shared/xcb-xwayland.h"
#include <libweston/zalloc.h>
#include "xcb-client-helper.h"

#define DEBUG

#ifdef DEBUG
#define printfd(fmt, args...) do {	\
	fprintf(stderr, fmt, ##args);	\
} while (0)
#else
#define printfd(fmt, args...)	{}
#endif

struct event_response {
    uint8_t response_type;
    bool (*eventcb)(xcb_generic_event_t *e, struct window_x11 *win);
    const char *name;
};

const char *to_event_name(uint8_t event);

static xcb_drawable_t
handle_event_to_wid(xcb_generic_event_t *ev)
{
	xcb_drawable_t wid;

	switch (EVENT_TYPE(ev)) {
	case XCB_CREATE_NOTIFY: {
		xcb_create_notify_event_t *ce = (xcb_create_notify_event_t *) ev;
		wid = ce->window;
		break;
	}
	case XCB_DESTROY_NOTIFY: {
		xcb_destroy_notify_event_t *de = (xcb_destroy_notify_event_t *) ev;
		wid = de->window;
		break;
	}
	case XCB_MAP_NOTIFY: {
		xcb_map_notify_event_t *mn = (xcb_map_notify_event_t *) ev;
		wid = mn->window;
		break;
	}
	case XCB_UNMAP_NOTIFY: {
		xcb_unmap_notify_event_t *un = (xcb_unmap_notify_event_t *) ev;
		wid = un->window;
		break;
	}
	case XCB_PROPERTY_NOTIFY: {
		xcb_property_notify_event_t *pn = (xcb_property_notify_event_t *) ev;
		wid = pn->window;
		break;
	}
	case XCB_CONFIGURE_NOTIFY: {
		xcb_configure_notify_event_t *cn = (xcb_configure_notify_event_t *) ev;
		wid = cn->window;
		break;
	}
	case XCB_EXPOSE: {
		xcb_expose_event_t *ep = (xcb_expose_event_t *) ev;
		wid = ep->window;
		break;
	}
	case XCB_REPARENT_NOTIFY: {
		xcb_reparent_notify_event_t *re = (xcb_reparent_notify_event_t *) ev;
		wid = re->window;
		break;
	}
	default:
		wid = 0;
	}

	return wid;
}

void
handle_event_remove_pending(struct window_state *wstate)
{
	wl_list_remove(&wstate->link);
	free(wstate);
}

/* returns true if all events in the pending_list has been accounted for */
static bool
handle_event_check_pending(struct window_x11 *window, xcb_generic_event_t *ev)
{

	struct window_state *wstate, *wstate_next;
	struct wl_list *pending_events =
		&window->tentative_state.pending_events_list;
	xcb_drawable_t wid;
	uint8_t event;
	bool found = false;

	event = EVENT_TYPE(ev);
	wid = handle_event_to_wid(ev);

	wl_list_for_each_safe(wstate, wstate_next, pending_events, link) {
		if (wstate->event != event)
			continue;

		if (wstate->wid == wid) {
			handle_event_remove_pending(wstate);
			found = true;
			printfd("%s: removed event %d - %s\n", __func__,
				event, to_event_name(event));
			break;
		}
	}

	if (!found) {
		printfd("%s(): event id %d, name %s not found\n", __func__,
				event, to_event_name(event));
		return false;
	}

	/* still need to get events? -> wait one more round */
	if (!wl_list_empty(pending_events)) {
		printfd("%s(): still have %d events to handle!\n", __func__,
				wl_list_length(pending_events));
		return false;
	}

	return true;
}

/** In case you need to wait for a notify event call use this function to do so
 * and then call handle_events_x11() at the end, to wait for the events to be
 * delivered/handled. Note that handle_events_x11() will wait forever if
 * handle_event_set_pending() was called for an event which never arrives.
 *
 * You can call this function multiple times for the same wid, in case you
 * expect multiple events to be delivered (i.e., a map notify event and a
 * expose one). All functions in this XCB wrapper library calls this function,
 * with the user only needing to call handle_events_x11() at the end. This
 * function is only needed if the test itself requires to wait for additional
 * events, or when expanding this library with other states changes
 * (max/fullscreen).
 *
 * \param window the window_x11 in question
 * \param event the event to wait for, like XCB_MAP_NOTIFY, XCB_EXPOSE, etc.
 * \param pending_state the pending event to wait for, similar to the XCB ones
 *  but defined in enum w_state
 * \param wid the window id, could be different than that of the window itself
 */
void
handle_event_set_pending(struct window_x11 *window, uint8_t event,
			 enum w_state pending_state, xcb_drawable_t wid)
{
	struct window_state *wstate = xzalloc(sizeof(*wstate));

	wstate->event = event;
	wstate->wid = wid;
	wstate->pending_state = pending_state;
	wl_list_insert(&window->tentative_state.pending_events_list,
		       &wstate->link);

	printfd("%s: Added pending event id %d - name %s, wid %d\n",
		__func__, event, to_event_name(event), wid);
}

static bool
handle_map_notify(xcb_generic_event_t *e, struct window_x11 *window)
{
	xcb_map_notify_event_t *ce = (xcb_map_notify_event_t *) e;

	if (ce->window != window->win_id)
		return false;

	window_state_set_flag(window, MAPPED);

	return true;
}

static bool
handle_unmap_notify(xcb_generic_event_t *e, struct window_x11 *window)
{
	xcb_unmap_notify_event_t *ce = (xcb_unmap_notify_event_t*) e;

	if (ce->window != window->win_id && ce->window != window->frame_id)
		return false;

	assert(window_state_has_flag(window, MAPPED));
	window_state_set_flag(window, UNMAPPED);

	return true;
}

static bool
handle_create_notify(xcb_generic_event_t *e, struct window_x11 *window)
{
	xcb_create_notify_event_t *ce = (xcb_create_notify_event_t*) e;

	if (ce->window != window->win_id)
		return false;

	window_state_set_flag(window, CREATED);

	return true;
}


static bool
handle_destroy_notify(xcb_generic_event_t *e, struct window_x11 *window)
{
	xcb_destroy_notify_event_t *dn = (xcb_destroy_notify_event_t*) e;

	if (window->win_id != dn->window)
		return false;

	assert(window_state_has_flag(window, CREATED));
	window_state_set_flag(window, DESTROYED);

	return true;
}

static bool
handle_property_notify(xcb_generic_event_t *e, struct window_x11 *window)
{
	xcb_property_notify_event_t *pn = (xcb_property_notify_event_t *) e;
	struct atom_x11 *atoms = window->conn->atoms;

	if (pn->window != window->win_id)
		return false;

	if (pn->atom == atoms->net_wm_name) {
		window_state_set_flag(window, PROPERTY_NAME);
		return true;
	}

	return false;
}

static bool
handle_expose(xcb_generic_event_t *e, struct window_x11 *window)
{
	xcb_expose_event_t *ep = (xcb_expose_event_t *) e;

	if (ep->window != window->win_id)
		return false;

	window_state_set_flag(window, EXPOSE);
	return true;
}

static bool
handle_configure_notify(xcb_generic_event_t *e, struct window_x11 *window)
{
	xcb_configure_notify_event_t *cn = (xcb_configure_notify_event_t*) e;

	/* we're not interested into other's windows */
	if (cn->window != window->win_id)
		return false;

	return true;
}

static bool
handle_reparent_notify(xcb_generic_event_t *e, struct window_x11 *window)
{
	xcb_reparent_notify_event_t *re = (xcb_reparent_notify_event_t *) e;

	if (re->window == window->win_id && window->frame_id == 0) {
		window->frame_id = re->parent;
		window_state_set_flag(window, REPARENT);
		printfd("Window reparent frame id %d\n", window->frame_id);
		return true;
	}

	return false;
}

/* the event handlers should return a boolean that denotes that fact
 * they've been handled. One can customize that behaviour such that
 * it forces handle_events_x11() to wait for additional events, in case
 * that's needed. */
static const struct event_response events[] = {
	{ XCB_CREATE_NOTIFY, handle_create_notify, "CREATE_NOTIFY" },
	{ XCB_MAP_NOTIFY, handle_map_notify, "MAP_NOTIFY" },
	{ XCB_UNMAP_NOTIFY, handle_unmap_notify, "UNMAP_NOTIFY" },
	{ XCB_EXPOSE, handle_expose, "EXPOSE_NOTIFY" },
	{ XCB_PROPERTY_NOTIFY, handle_property_notify, "PROPERTY_NOTIFY" },
	{ XCB_CONFIGURE_NOTIFY, handle_configure_notify, "CONFIGURE_NOTIFY" },
	{ XCB_DESTROY_NOTIFY, handle_destroy_notify, "DESTROY_NOTIFY" },
	{ XCB_REPARENT_NOTIFY, handle_reparent_notify, "REPARENT_NOTIFY" },
};

const char *
to_event_name(uint8_t event)
{
	size_t i;
	for (i = 0; i < ARRAY_LENGTH(events); i++)
		if (events[i].response_type == event)
			return events[i].name;

	return "(unknown event)";
}

/**
 * Tells the X server to display the window. Call handle_events_x11() after
 * calling this function to wait for events to be delivered. Note there's no
 * need to include additional events, they're already added.
 *
 * \sa handle_events_x11().
 *
 * \param window the window in question
 */
void
window_x11_map(struct window_x11 *window)
{
	handle_event_set_pending(window, XCB_MAP_NOTIFY, MAPPED, window->win_id);
	handle_event_set_pending(window, XCB_EXPOSE, EXPOSE, window->win_id);

	/* doing a synchronization for the frame wid helps with other potential
	 * states changes like max/fullscreen or if we try to map the window
	 * from the beginning with a max/fullscreen state rather than normal
	 * state (!max && !fullscreen)
	 */
	handle_event_set_pending(window, XCB_REPARENT_NOTIFY, REPARENT, window->win_id);

	xcb_map_window(window->conn->connection, window->win_id);
	xcb_flush(window->conn->connection);
}

/**
 * \sa window_x11_map, handle_events_x11.  Tells the X server to unmap the
 * window. Call handle_events_x11() to wait for the events to be delivered.
 *
 * \param window the window in question
 */
void
window_x11_unmap(struct window_x11 *window)
{
	handle_event_set_pending(window, XCB_UNMAP_NOTIFY, UNMAPPED, window->win_id);

	xcb_unmap_window(window->conn->connection, window->win_id);
	xcb_flush(window->conn->connection);
}

static xcb_generic_event_t *
poll_for_event(xcb_connection_t *conn)
{
	int fd = xcb_get_file_descriptor(conn);
	struct pollfd pollfds = {};
	int rpol;

	pollfds.fd = fd;
	pollfds.events = POLLIN;

	rpol = ppoll(&pollfds, 1, NULL, NULL);
	if (rpol > 0 && (pollfds.revents & POLLIN))
		return xcb_wait_for_event(conn);

	return NULL;
}

static void
window_x11_set_cursor(struct window_x11 *window, const char *cursor_name)
{
	assert(window);
	assert(window->ctx == NULL);

	if (xcb_cursor_context_new(window->conn->connection,
				   window->screen, &window->ctx) < 0) {
		fprintf(stderr, "Error creating context!\n");
		return;
	}

	window->cursor = xcb_cursor_load_cursor(window->ctx, cursor_name);

	xcb_change_window_attributes(window->conn->connection,
				     window->root_win_id, XCB_CW_CURSOR,
				     &window->cursor);
	xcb_flush(window->conn->connection);
}

static bool
handle_event(xcb_generic_event_t *ev, struct window_x11 *window)
{
	uint8_t event = EVENT_TYPE(ev);
	bool events_handled = false;
	size_t i;

	for (i = 0; i < ARRAY_LENGTH(events); i++) {
		if (event == events[i].response_type) {
			bool ev_cb_handled = events[i].eventcb(ev, window);
			if (!ev_cb_handled)
				return false;

			events_handled =
				handle_event_check_pending(window, ev);
		}
	}

	return events_handled;
}

/**
 * Each operation on 'window_x11' requires calling handle_events_x11() to a) flush
 * out the connection, b) poll for a xcb_generic_event_t and c) to call the
 * appropriate event handler;
 *
 * This function should never block to allow a programmatic way of applying
 * different operations/states to the window. If that happens, running it
 * under meson test will cause a test fail (with a timeout).
 *
 * Before calling this function, one *shall* use handle_event_set_pending() to
 * explicitly set which events to wait for. Not doing so will effectively
 * deadlock the test, as it will wait for pending events never being set.
 *
 * Note that all state change functions, including map and unmap, would
 * implicitly, if not otherwise stated, set the pending events to wait for.
 *
 * \sa handle_event_set_pending()
 * \param window the X11 window in question
 */
int
handle_events_x11(struct window_x11 *window)
{
	bool running = true;
	xcb_generic_event_t *ev;
	int ret = 0;

	assert(window->handle_in_progress == false);
	window->handle_in_progress = true;

	do {
		bool events_handled = false;

		xcb_flush(window->conn->connection);

		if (xcb_connection_has_error(window->conn->connection)) {
			fprintf(stderr, "X11 connection got interrupted\n");
			ret = -1;
			break;
		}

		ev = poll_for_event(window->conn->connection);
		if (!ev) {
			fprintf(stderr, "Error, no event received, "
					"although we requested for one!\n");
			break;
		}

		events_handled = handle_event(ev, window);

		/* signals that we've done processing all the pending events */
		if (events_handled) {
			running = false;
		}

		free(ev);
	} while (running);

	window->handle_in_progress = false;
	return ret;
}

/* Might be useful in case you'd want to receive create notify for our own window. */
void
window_x11_notify_for_root_events(struct window_x11 *window)
{
	int mask_values =
		XCB_EVENT_MASK_STRUCTURE_NOTIFY |
		XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;

	xcb_change_window_attributes(window->conn->connection,
				     window->root_win_id,
				     XCB_CW_EVENT_MASK, &mask_values);
	xcb_flush(window->conn->connection);
}

/**
 * Sets the x11 window a property name.  Call handle_events_x11() to
 * wait for the events to be delivered.
 *
 * \param window the window in question
 * \param name the name, as string
 *
 */
void
window_x11_set_win_name(struct window_x11 *window, const char *name)
{
	struct atom_x11 *atoms = window->conn->atoms;

	handle_event_set_pending(window, XCB_PROPERTY_NOTIFY, PROPERTY_NAME, window->win_id);

	xcb_change_property(window->conn->connection, XCB_PROP_MODE_REPLACE,
			    window->win_id, atoms->net_wm_name,
			    atoms->string, 8,
			    strlen(name), name);
	xcb_flush(window->conn->connection);
}

/** Create a x11 connection.
 *
 * \sa window_get_connection() to retrieve it in the same tests to avoid
 * creating a new connection
 * \sa create_x11_window(), where you need to pass this connection_x11 object.
 *
 * \return a struct connection_x11 which defines a x11 connection.
 */
struct connection_x11 *
create_x11_connection(void)
{
	struct connection_x11 *conn;

	if (access(XSERVER_PATH, X_OK) != 0)
		return NULL;

	conn = xzalloc(sizeof(*conn));
	conn->connection = xcb_connect(NULL, NULL);
	if (!conn->connection)
		return NULL;

	conn->atoms = xzalloc(sizeof(struct atom_x11));

	/* retrieve atoms */
	x11_get_atoms(conn->connection, conn->atoms);

	return conn;
}

/** Destroys a x11 connection. Use this at the end (of the test) to destroy the
 * x11 connection.
 *
 * \param conn the x11 connection in question.
 */
void
destroy_x11_connection(struct connection_x11 *conn)
{
	xcb_disconnect(conn->connection);

	free(conn->atoms);
	free(conn);
}


/**
 * creates a X window, based on the initial supplied values. All operations
 * performed will work on this window_x11 object.
 *
 * The creation and destruction of the window_x11 object is handled implictly
 * so there's no need wait for (additional) events, like it is required for
 * all other change state operations.
 *
 * The window is not mapped/displayed so that needs to happen explictly, by
 * calling window_x11_map() and then waiting for events using
 * handle_events_x11().
 *
 * \param width initial size, width value
 * \param height initial size, height value
 * \param pos_x initial position, x value
 * \param pos_y initial position, y value
 * \param conn X11 connection
 * \param bg_color a background color
 * \param parent the window_x11 parent
 * \return a pointer to window_x11, which gets destroyed with destroy_x11_window()
 */
struct window_x11 *
create_x11_window(int width, int height, int pos_x, int pos_y,
		  struct connection_x11 *conn, pixman_color_t bg_color,
		  struct window_x11 *parent)
{
	uint32_t colorpixel = 0x0;
	uint32_t values[2];
	uint32_t mask = 0;

	xcb_colormap_t colormap;
	struct window_x11 *window;
	xcb_window_t parent_win_id;
	xcb_alloc_color_cookie_t cookie;
	xcb_alloc_color_reply_t *reply;
	xcb_void_cookie_t cookie_create;
	xcb_generic_error_t *error_create;
	const struct xcb_setup_t *xcb_setup;

	assert(conn);
	window = xzalloc(sizeof(*window));

	window->conn = conn;
	xcb_setup = xcb_get_setup(window->conn->connection);
	window->screen = xcb_setup_roots_iterator(xcb_setup).data;

	wl_list_init(&window->window_list);
	wl_list_init(&window->tentative_state.pending_events_list);

	window->root_win_id = window->screen->root;
	window->parent = parent;
	if (window->parent) {
		parent_win_id = window->parent->win_id;
		wl_list_insert(&parent->window_list, &window->window_link);
	} else {
		parent_win_id = window->root_win_id;
	}

	colormap = window->screen->default_colormap;
	cookie = xcb_alloc_color(window->conn->connection, colormap,
				 bg_color.red, bg_color.blue, bg_color.green);
	reply = xcb_alloc_color_reply(window->conn->connection, cookie, NULL);
	assert(reply);

	colorpixel = reply->pixel;
	free(reply);

	window->background = xcb_generate_id(window->conn->connection);
	mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
	values[0]  = colorpixel;
	values[1] = 0;
	window->bg_color = bg_color;

	xcb_create_gc(window->conn->connection, window->background,
		      window->root_win_id, mask, values);

	/* create the window */
	window->win_id = xcb_generate_id(window->conn->connection);
	mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	values[0] = colorpixel;
	values[1] = XCB_EVENT_MASK_EXPOSURE |
		    XCB_EVENT_MASK_KEY_PRESS |
		    XCB_EVENT_MASK_VISIBILITY_CHANGE |
		    XCB_EVENT_MASK_STRUCTURE_NOTIFY |
		    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
		    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
		    XCB_EVENT_MASK_PROPERTY_CHANGE;

	window->pos_x = pos_x;
	window->pos_y = pos_y;

	window->width = width;
	window->height = height;

	cookie_create = xcb_create_window_checked(window->conn->connection,
						  XCB_COPY_FROM_PARENT,
						  window->win_id, parent_win_id,
						  window->pos_x, window->pos_y,
						  window->width, window->height,
						  0,
						  XCB_WINDOW_CLASS_INPUT_OUTPUT,
						  window->screen->root_visual,
						  mask, values);
	error_create = xcb_request_check(window->conn->connection, cookie_create);
	assert(error_create == NULL);

	window_state_set_flag(window, CREATED);
	window_x11_set_cursor(window, "left_ptr");

	return window;
}

static void
kill_window(struct window_x11 *window)
{
	handle_event_set_pending(window, XCB_DESTROY_NOTIFY, DESTROYED, window->win_id);

	xcb_destroy_window(window->conn->connection, window->win_id);
	xcb_flush(window->conn->connection);
}

/**
 * \sa create_x11_window(). The creation and destruction of the window_x11 is
 * handled implicitly so there's no wait for (additional) events.
 *
 * This function would wait for destroy notify event and will disconnect from
 * the server. No further operation can happen on the window_x11, except
 * for destroying the x11 connection using destroy_x11_connection().
 *
 * \param window the window in question
 */
void
destroy_x11_window(struct window_x11 *window)
{
	struct window_state *wstate, *wstate_next;

	xcb_free_cursor(window->conn->connection, window->cursor);
	xcb_cursor_context_free(window->ctx);
	xcb_flush(window->conn->connection);

	kill_window(window);
	handle_events_x11(window);

	/* in case we're called before any events have been handled */
	wl_list_for_each_safe(wstate, wstate_next,
			      &window->tentative_state.pending_events_list, link)
		handle_event_remove_pending(wstate);

	free(window);
}

/**
 * Return the reply_t for an atom
 *
 * \param window the window in question
 * \param win the handle for the window; could be different from the window itself!
 * \param atom the atom in question
 *
 */
xcb_get_property_reply_t *
window_x11_dump_prop(struct window_x11 *window, xcb_drawable_t win, xcb_atom_t atom)
{
	xcb_get_property_cookie_t prop_cookie;
	xcb_get_property_reply_t *prop_reply;

	prop_cookie = xcb_get_property(window->conn->connection, 0, win, atom,
				      XCB_GET_PROPERTY_TYPE_ANY, 0, 2048);

	prop_reply = xcb_get_property_reply(window->conn->connection, prop_cookie, NULL);

	/* callers needs to free it */
	return prop_reply;
}

/** Retrieve the atoms
 *
 * \param win the window in question from which to retrieve the atoms
 *
 */
struct atom_x11 *
window_get_atoms(struct window_x11 *win)
{
	return win->conn->atoms;
}

/** Retrive the connection_x11 from the window_x11
 *
 * \param win the window in question from which to retrieve the connection
 *
 */
struct xcb_connection_t *
window_get_connection(struct window_x11 *win)
{
	return win->conn->connection;
}
