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

#ifndef WESTON_SIGNAL_H
#define WESTON_SIGNAL_H

#include <wayland-server-core.h>

#ifdef  __cplusplus
extern "C" {
#endif

/* A safer version of wl_signal_emit() which can gracefully handle additions
 * and deletions of any signal listener from within listener notification
 * callbacks.
 *
 * Listeners deleted during a signal emission and which have not already been
 * notified at the time of deletion are not notified by that emission.
 *
 * Listeners added (or readded) during signal emission are ignored by that
 * emission.
 *
 * Note that repurposing a listener without explicitly removing it and readding
 * it is not supported and can lead to unexpected behavior.
 */

void
weston_signal_emit_mutable(struct wl_signal *signal, void *data);

#ifdef  __cplusplus
}
#endif

#endif /* WESTON_SIGNAL_H */
