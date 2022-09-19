/*
 * Copyright © 2014 Pekka Paalanen <pq@iki.fi>
 * Copyright © 2014, 2019 Collabora, Ltd.
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

#ifndef WESTON_TIMELINE_H
#define WESTON_TIMELINE_H

#include <wayland-util.h>
#include <stdbool.h>

#include <libweston/weston-log.h>
#include <wayland-server-core.h>

enum timeline_type {
	TLT_END = 0,
	TLT_OUTPUT,
	TLT_SURFACE,
	TLT_VBLANK,
	TLT_GPU,
};

/** Timeline subscription created for each subscription
 *
 * Created automatically by weston_log_scope::new_subscription and
 * destroyed by weston_log_scope::destroy_subscription.
 *
 * @ingroup internal-log
 */
struct weston_timeline_subscription {
	unsigned int next_id;
	struct wl_list objects; /**< weston_timeline_subscription_object::subscription_link */
};

/**
 * Created when object is first seen for a particular timeline subscription
 * Destroyed when the subscription got destroyed or object was destroyed
 *
 * @ingroup internal-log
 */
struct weston_timeline_subscription_object {
	void *object;                           /**< points to the object */
	unsigned int id;
	bool force_refresh;
	struct wl_list subscription_link;       /**< weston_timeline_subscription::objects */
	struct wl_listener destroy_listener;
};

#define TYPEVERIFY(type, arg) ({		\
	typeof(arg) tmp___ = (arg);		\
	(void)((type)0 == tmp___);		\
	tmp___; })

/**
 * Should be used as the last argument when using TL_POINT macro
 *
 * @ingroup log
 */
#define TLP_END TLT_END, NULL

#define TLP_OUTPUT(o) TLT_OUTPUT, TYPEVERIFY(struct weston_output *, (o))
#define TLP_SURFACE(s) TLT_SURFACE, TYPEVERIFY(struct weston_surface *, (s))
#define TLP_VBLANK(t) TLT_VBLANK, TYPEVERIFY(const struct timespec *, (t))
#define TLP_GPU(t) TLT_GPU, TYPEVERIFY(const struct timespec *, (t))

/** This macro is used to add timeline points.
 *
 * Use TLP_END when done for the vargs.
 *
 * @param ec weston_compositor instance
 *
 * @ingroup log
 */
#define TL_POINT(ec, ...) do { \
	weston_timeline_point(ec->timeline, __VA_ARGS__); \
} while (0)

void
weston_timeline_point(struct weston_log_scope *timeline_scope,
		      const char *name, ...);

#endif /* WESTON_TIMELINE_H */
