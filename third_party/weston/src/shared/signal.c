/*
 * Copyright 2018 Simon Ser
 * Copyright 2021 Collabora, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
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

/* Implementation copied from wlroots util/signal.c file */

#include "signal.h"

static void
handle_noop(struct wl_listener *listener, void *data)
{
	/* Do nothing */
}

void
weston_signal_emit_mutable(struct wl_signal *signal, void *data)
{
	struct wl_listener cursor;
	struct wl_listener end;

	/* Add two special markers: one cursor and one end marker. This way, we
	 * know that we've already called listeners on the left of the cursor
	 * and that we don't want to call listeners on the right of the end
	 * marker. The 'it' function can remove any element it wants from the
	 * list without troubles.
	 *
	 * There was a previous attempt that used to steal the whole list of
	 * listeners but then that broke wl_signal_get().
	 *
	 * wl_list_for_each_safe tries to be safe but it fails: it works fine
	 * if the current item is removed, but not if the next one is. */
	wl_list_insert(&signal->listener_list, &cursor.link);
	cursor.notify = handle_noop;
	wl_list_insert(signal->listener_list.prev, &end.link);
	end.notify = handle_noop;

	while (cursor.link.next != &end.link) {
		struct wl_list *pos = cursor.link.next;
		struct wl_listener *l = wl_container_of(pos, l, link);

		wl_list_remove(&cursor.link);
		wl_list_insert(pos, &cursor.link);

		l->notify(l, data);
	}

	wl_list_remove(&cursor.link);
	wl_list_remove(&end.link);
}
