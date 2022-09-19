/*
 * Copyright © 2013 Intel Corporation
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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <X11/Xcursor/Xcursor.h>

#include <libweston/libweston.h>
#include "xwayland.h"

#include "hash.h"

struct dnd_data_source {
	struct weston_data_source base;
	struct weston_wm *wm;
	int version;
	uint32_t window;
};

static void
data_source_accept(struct weston_data_source *base,
		   uint32_t time, const char *mime_type)
{
	struct dnd_data_source *source = (struct dnd_data_source *) base;
	xcb_client_message_event_t client_message;
	struct weston_wm *wm = source->wm;

	weston_log("got accept, mime-type %s\n", mime_type);

	/* FIXME: If we rewrote UTF8_STRING to
	 * text/plain;charset=utf-8 and the source doesn't support the
	 * mime-type, we'll have to rewrite the mime-type back to
	 * UTF8_STRING here. */

	client_message.response_type = XCB_CLIENT_MESSAGE;
	client_message.format = 32;
	client_message.window = wm->dnd_window;
	client_message.type = wm->atom.xdnd_status;
	client_message.data.data32[0] = wm->dnd_window;
	client_message.data.data32[1] = 2;
	if (mime_type)
		client_message.data.data32[1] |= 1;
	client_message.data.data32[2] = 0;
	client_message.data.data32[3] = 0;
	client_message.data.data32[4] = wm->atom.xdnd_action_copy;

	xcb_send_event(wm->conn, 0, wm->dnd_owner,
			       XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
			       (char *) &client_message);
}

static void
data_source_send(struct weston_data_source *base,
		 const char *mime_type, int32_t fd)
{
	struct dnd_data_source *source = (struct dnd_data_source *) base;
	struct weston_wm *wm = source->wm;

	weston_log("got send, %s\n", mime_type);

	/* Get data for the utf8_string target */
	xcb_convert_selection(wm->conn,
			      wm->selection_window,
			      wm->atom.xdnd_selection,
			      wm->atom.utf8_string,
			      wm->atom.wl_selection,
			      XCB_TIME_CURRENT_TIME);

	xcb_flush(wm->conn);

	fcntl(fd, F_SETFL, O_WRONLY | O_NONBLOCK);
	wm->data_source_fd = fd;
}

static void
data_source_cancel(struct weston_data_source *source)
{
	weston_log("got cancel\n");
}

static void
handle_enter(struct weston_wm *wm, xcb_client_message_event_t *client_message)
{
	struct dnd_data_source *source;
	struct weston_seat *seat = weston_wm_pick_seat(wm);
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);
	char **p;
	const char *name;
	uint32_t *types;
	int i, length, has_text;
	xcb_get_property_cookie_t cookie;
	xcb_get_property_reply_t *reply;

	source = zalloc(sizeof *source);
	if (source == NULL)
		return;

	wl_signal_init(&source->base.destroy_signal);
	source->base.accept = data_source_accept;
	source->base.send = data_source_send;
	source->base.cancel = data_source_cancel;
	source->wm = wm;
	source->window = client_message->data.data32[0];
	source->version = client_message->data.data32[1] >> 24;

	if (client_message->data.data32[1] & 1) {
		cookie = xcb_get_property(wm->conn,
					  0, /* delete */
					  source->window,
					  wm->atom.xdnd_type_list,
					  XCB_ATOM_ANY, 0, 2048);
		reply = xcb_get_property_reply(wm->conn, cookie, NULL);
		types = xcb_get_property_value(reply);
		length = reply->value_len;
	} else {
		reply = NULL;
		types = &client_message->data.data32[2];
		length = 3;
	}

	wl_array_init(&source->base.mime_types);
	has_text = 0;
	for (i = 0; i < length; i++) {
		if (types[i] == XCB_ATOM_NONE)
			continue;

		name = get_atom_name(wm->conn, types[i]);
		if (types[i] == wm->atom.utf8_string ||
		    types[i] == wm->atom.text_plain_utf8 ||
		    types[i] == wm->atom.text_plain) {
			if (has_text)
				continue;

			has_text = 1;
			p = wl_array_add(&source->base.mime_types, sizeof *p);
			if (p)
				*p = strdup("text/plain;charset=utf-8");
		} else if (strchr(name, '/')) {
			p = wl_array_add(&source->base.mime_types, sizeof *p);
			if (p)
				*p = strdup(name);
		}
	}

	free(reply);
	weston_pointer_start_drag(pointer, &source->base, NULL, NULL);
}

int
weston_wm_handle_dnd_event(struct weston_wm *wm,
			   xcb_generic_event_t *event)
{
	xcb_xfixes_selection_notify_event_t *xfixes_selection_notify =
		(xcb_xfixes_selection_notify_event_t *) event;
	xcb_client_message_event_t *client_message =
		(xcb_client_message_event_t *) event;

	switch (event->response_type - wm->xfixes->first_event) {
	case XCB_XFIXES_SELECTION_NOTIFY:
		if (xfixes_selection_notify->selection != wm->atom.xdnd_selection)
			return 0;

		weston_log("XdndSelection owner: %d!\n",
			   xfixes_selection_notify->owner);
		return 1;
	}

	switch (EVENT_TYPE(event)) {
	case XCB_CLIENT_MESSAGE:
		if (client_message->type == wm->atom.xdnd_enter) {
			handle_enter(wm, client_message);
			return 1;
		} else if (client_message->type == wm->atom.xdnd_leave) {
			weston_log("got leave!\n");
			return 1;
		} else if (client_message->type == wm->atom.xdnd_drop) {
			weston_log("got drop!\n");
			return 1;
		} else if (client_message->type == wm->atom.xdnd_drop) {
			weston_log("got enter!\n");
			return 1;
		}
		return 0;
	}

	return 0;
}

void
weston_wm_dnd_init(struct weston_wm *wm)
{
	uint32_t values[1], version = 4, mask;

	mask =
		XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER |
		XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY |
		XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE;
	xcb_xfixes_select_selection_input(wm->conn, wm->selection_window,
					  wm->atom.xdnd_selection, mask);

	wm->dnd_window = xcb_generate_id(wm->conn);
	values[0] =
		XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
		XCB_EVENT_MASK_PROPERTY_CHANGE;

	xcb_create_window(wm->conn,
			  XCB_COPY_FROM_PARENT,
			  wm->dnd_window,
			  wm->screen->root,
			  0, 0,
			  8192, 8192,
			  0,
			  XCB_WINDOW_CLASS_INPUT_ONLY,
			  wm->screen->root_visual,
			  XCB_CW_EVENT_MASK, values);
	xcb_change_property(wm->conn,
			    XCB_PROP_MODE_REPLACE,
			    wm->dnd_window,
			    wm->atom.xdnd_aware,
			    XCB_ATOM_ATOM,
			    32, /* format */
			    1, &version);
}
