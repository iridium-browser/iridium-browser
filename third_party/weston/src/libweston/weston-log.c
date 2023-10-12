/*
 * Copyright © 2017 Pekka Paalanen <pq@iki.fi>
 * Copyright © 2018 Zodiac Inflight Innovations
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

#include <libweston/weston-log.h>
#include "shared/helpers.h"
#include <libweston/libweston.h>

#include "weston-log-internal.h"
#include "weston-debug-server-protocol.h"

#include <assert.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

/**
 * @defgroup log Public Logging/Debugging API
 * @defgroup internal-log Private/Internal Logging/Debugging API
 * @defgroup debug-protocol weston-debug protocol specific
 */

/** Main weston-log context
 *
 * One per weston_compositor. Stores list of scopes created and a list pending
 * subscriptions.
 *
 * A pending subscription is a subscription to a scope which hasn't been
 * created. When the scope is finally created the pending subscription will be
 * removed from the pending subscription list, but not before was added in the
 * scope's subscription list and that of the subscriber list.
 *
 * Pending subscriptions only make sense for other types of streams, other than
 * those created by weston-debug protocol. In the case of the weston-debug
 * protocol, the subscription processes is done automatically whenever a client
 * connects and subscribes to a scope which was previously advertised by the
 * compositor.
 *
 * @ingroup internal-log
 */
struct weston_log_context {
	struct wl_global *global;
	struct wl_listener compositor_destroy_listener;
	struct wl_list scope_list; /**< weston_log_scope::compositor_link */
	struct wl_list pending_subscription_list; /**< weston_log_subscription::source_link */
};

/** weston-log message scope
 *
 * This is used for scoping logging/debugging messages. Clients can subscribe
 * to only the scopes they are interested in. A scope is identified by its name
 * (also referred to as debug stream name).
 *
 * @ingroup log
 */
struct weston_log_scope {
	char *name;
	char *desc;
	weston_log_scope_cb new_subscription;
	weston_log_scope_cb destroy_subscription;
	void *user_data;
	struct wl_list compositor_link;
	struct wl_list subscription_list;  /**< weston_log_subscription::source_link */
};

/** Ties a subscriber to a scope
 *
 * A subscription is created each time we'd want to subscribe to a scope. From
 * the stream type we can retrieve the subscriber and from the subscriber we
 * reach each of the streams callbacks. See also weston_log_subscriber object.
 *
 * When a subscription has been created we store it in the scope's subscription
 * list and in the subscriber's subscription list. The subscription might be a
 * pending subscription until the scope for which there's was a subscribe has
 * been created. The scope creation will take of looking through the pending
 * subscription list.
 *
 * A subscription can reached from a subscriber subscription list by using the
 * streams base class.
 *
 * @ingroup internal-log
 */
struct weston_log_subscription {
	struct weston_log_subscriber *owner;
	struct wl_list owner_link;      /**< weston_log_subscriber::subscription_list */

	char *scope_name;
	struct weston_log_scope *source;
	struct wl_list source_link;     /**< weston_log_scope::subscription_list  or
					  weston_log_context::pending_subscription_list */

	void *data;
};

static struct weston_log_subscription *
find_pending_subscription(struct weston_log_context *log_ctx,
			  const char *scope_name)
{
	struct weston_log_subscription *sub;

	wl_list_for_each(sub, &log_ctx->pending_subscription_list, source_link)
		if (!strcmp(sub->scope_name, scope_name))
			return sub;

	return NULL;
}

/** Create a pending subscription and add it the list of pending subscriptions
 *
 * @param owner a subscriber represented by weston_log_subscriber object
 * @param scope_name the name of the scope (which we don't have in the list of scopes)
 * @param log_ctx the logging context used to add the pending subscription
 *
 * @memberof weston_log_subscription
 */
static void
weston_log_subscription_create_pending(struct weston_log_subscriber *owner,
				       const char *scope_name,
				       struct weston_log_context *log_ctx)
{
	assert(owner);
	assert(scope_name);
	struct weston_log_subscription *sub = zalloc(sizeof(*sub));

	if (!sub)
		return;

	sub->scope_name = strdup(scope_name);
	sub->owner = owner;

	wl_list_insert(&log_ctx->pending_subscription_list, &sub->source_link);
}

/** Destroys the pending subscription created previously with
 * weston_log_subscription_create_pending()
 *
 * @param sub the weston_log_subscription object to remove from the list
 * of subscriptions and to destroy the subscription
 *
 * @memberof weston_log_subscription
 */
static void
weston_log_subscription_destroy_pending(struct weston_log_subscription *sub)
{
	assert(sub);
	/* pending subsriptions do not have a source */
	wl_list_remove(&sub->source_link);
	free(sub->scope_name);
	free(sub);
}

/** Write to the stream's subscription
 *
 * @memberof weston_log_subscription
 */
static void
weston_log_subscription_write(struct weston_log_subscription *sub,
			      const char *data, size_t len)
{
	if (sub->owner && sub->owner->write)
		sub->owner->write(sub->owner, data, len);
}

/** Write a formatted string to the stream's subscription
 *
 * @memberof weston_log_subscription
 */
static void
weston_log_subscription_vprintf(struct weston_log_subscription *sub,
				const char *fmt, va_list ap)
{
	static const char oom[] = "Out of memory";
	char *str;
	int len;

	if (!weston_log_scope_is_enabled(sub->source))
		return;

	len = vasprintf(&str, fmt, ap);
	if (len >= 0) {
		weston_log_subscription_write(sub, str, len);
		free(str);
	} else {
		weston_log_subscription_write(sub, oom, sizeof oom - 1);
	}
}

void
weston_log_subscription_set_data(struct weston_log_subscription *sub, void *data)
{
	/* don't allow data to already be set */
	assert(!sub->data);
	sub->data = data;
}

void *
weston_log_subscription_get_data(struct weston_log_subscription *sub)
{
	return sub->data;
}

/** Creates a new subscription using the subscriber by \c owner.
 *
 * The subscription created is added to the \c owner subscription list.
 * Destroying the subscription using weston_log_subscription_destroy() will
 * remove the link from the subscription list and free storage alloc'ed.
 *
 * Note that this adds the subscription to the scope's subscription list
 * hence the \c scope required argument.
 *
 * @param owner the subscriber owner, must be created before creating a
 * subscription
 * @param scope the scope in order to add the subscription to the scope's
 * subscription list
 *
 * @sa weston_log_subscription_destroy, weston_log_subscription_remove,
 * weston_log_subscription_add
 * @memberof weston_log_subscription
 */
void
weston_log_subscription_create(struct weston_log_subscriber *owner,
			       struct weston_log_scope *scope)
{
	struct weston_log_subscription *sub;
	assert(owner);
	assert(scope);
	assert(scope->name);

	sub = zalloc(sizeof(*sub));
	if (!sub)
		return;

	sub->owner = owner;
	sub->scope_name = strdup(scope->name);

	wl_list_insert(&sub->owner->subscription_list, &sub->owner_link);

	weston_log_subscription_add(scope, sub);
	weston_log_run_cb_new_subscription(sub);
}

/** Destroys the subscription
 *
 * Removes the subscription from the scopes subscription list and from
 * subscriber's subscription list. It destroys the subscription afterwads.
 *
 * @memberof weston_log_subscription
 */
void
weston_log_subscription_destroy(struct weston_log_subscription *sub)
{
	assert(sub);

	if (sub->owner->destroy_subscription)
		sub->owner->destroy_subscription(sub->owner);

	if (sub->source->destroy_subscription)
		sub->source->destroy_subscription(sub, sub->source->user_data);

	if (sub->owner)
		wl_list_remove(&sub->owner_link);

	weston_log_subscription_remove(sub);
	free(sub->scope_name);
	free(sub);
}

/** Adds the subscription \c sub to the subscription list of the
 * scope.
 *
 * This should used when the scope has been created, and the subscription \c
 * sub has be created before calling this function.
 *
 * @param scope the scope
 * @param sub the subscription, it must be created before, see
 * weston_log_subscription_create()
 *
 * @memberof weston_log_subscription
 */
void
weston_log_subscription_add(struct weston_log_scope *scope,
			    struct weston_log_subscription *sub)
{
	assert(scope);
	assert(sub);
	/* don't allow subscriptions to have a source already! */
	assert(!sub->source);

	sub->source = scope;
	wl_list_insert(&scope->subscription_list, &sub->source_link);
}

/** Removes the subscription from the scope's subscription list
 *
 * @memberof weston_log_subscription
 */
void
weston_log_subscription_remove(struct weston_log_subscription *sub)
{
	assert(sub);
	if (sub->source)
		wl_list_remove(&sub->source_link);
	sub->source = NULL;
}

/** Look-up the scope from the scope list  stored in the log context, by
 * matching against the \c name.
 *
 * @param log_ctx
 * @param name the scope name, see weston_log_ctx_add_log_scope() and
 * weston_compositor_add_log_scope()
 * @returns NULL if none found, or a pointer to a weston_log_scope
 *
 * @ingroup internal-log
 */
struct weston_log_scope *
weston_log_get_scope(struct weston_log_context *log_ctx, const char *name)
{
	struct weston_log_scope *scope;
	wl_list_for_each(scope, &log_ctx->scope_list, compositor_link)
		if (strcmp(name, scope->name) == 0)
			return scope;
	return NULL;
}

/** Wrapper to invoke the weston_log_scope_cb. Allows to call the cb
 * new_subscription of a log scope.
 *
 * @ingroup internal-log
 */
void
weston_log_run_cb_new_subscription(struct weston_log_subscription *sub)
{
	if (sub->source->new_subscription)
		sub->source->new_subscription(sub, sub->source->user_data);
}

/** Advertise the log scope name and the log scope description
 *
 * This is only used by the weston-debug protocol!
 *
 * @ingroup internal-log
 */
void
weston_debug_protocol_advertise_scopes(struct weston_log_context *log_ctx,
				       struct wl_resource *res)
{
	struct weston_log_scope *scope;
	wl_list_for_each(scope, &log_ctx->scope_list, compositor_link)
		weston_debug_v1_send_available(res, scope->name, scope->desc);
}

/** Disable debug-protocol
 *
 * @param log_ctx The log context where the debug-protocol is linked
 *
 * @ingroup internal-log
 */
static void
weston_log_ctx_disable_debug_protocol(struct weston_log_context *log_ctx)
{
	if (!log_ctx->global)
		return;

	wl_global_destroy(log_ctx->global);
	log_ctx->global = NULL;
}

/** Creates  weston_log_context structure
 *
 * \return NULL in case of failure, or a weston_log_context object in case of
 * success
 *
 * weston_log_context is a singleton for each weston_compositor.
 * @ingroup log
 *
 */
WL_EXPORT struct weston_log_context *
weston_log_ctx_create(void)
{
	struct weston_log_context *log_ctx;

	log_ctx = zalloc(sizeof *log_ctx);
	if (!log_ctx)
		return NULL;

	wl_list_init(&log_ctx->scope_list);
	wl_list_init(&log_ctx->pending_subscription_list);
	wl_list_init(&log_ctx->compositor_destroy_listener.link);

	return log_ctx;
}

/** Destroy weston_log_context structure
 *
 * \param log_ctx The log context to destroy.
 *
 * @ingroup log
 *
 */
WL_EXPORT void
weston_log_ctx_destroy(struct weston_log_context *log_ctx)
{
	struct weston_log_scope *scope;
	struct weston_log_subscription *pending_sub, *pending_sub_tmp;

	/* We can't destroy the log context if there's still a compositor
	 * that depends on it. This is an user error */
	 assert(wl_list_empty(&log_ctx->compositor_destroy_listener.link));

	weston_log_ctx_disable_debug_protocol(log_ctx);

	wl_list_for_each(scope, &log_ctx->scope_list, compositor_link)
		fprintf(stderr, "Internal warning: debug scope '%s' has not been destroyed.\n",
			   scope->name);

	/* Remove head to not crash if scope removed later. */
	wl_list_remove(&log_ctx->scope_list);

	/* Remove any pending subscription(s) which nobody subscribed to */
	wl_list_for_each_safe(pending_sub, pending_sub_tmp,
			      &log_ctx->pending_subscription_list, source_link) {
		weston_log_subscription_destroy_pending(pending_sub);
	}

	/* pending_subscription_list should be empty at this point */

	free(log_ctx);
}

static void
compositor_destroy_listener(struct wl_listener *listener, void *data)
{
	struct weston_log_context *log_ctx =
		wl_container_of(listener, log_ctx, compositor_destroy_listener);

	/* We have to keep this list initialized as weston_log_ctx_destroy() has
	 * to check if there's any compositor destroy listener registered */
	wl_list_remove(&log_ctx->compositor_destroy_listener.link);
	wl_list_init(&log_ctx->compositor_destroy_listener.link);

	weston_log_ctx_disable_debug_protocol(log_ctx);
}

/** Enable weston-debug protocol extension
 *
 * \param compositor The libweston compositor where to enable.
 *
 * This enables the weston_debug_v1 Wayland protocol extension which any client
 * can use to get debug messages from the compositor.
 *
 * WARNING: This feature should not be used in production. If a client
 * provides a file descriptor that blocks writes, it will block the whole
 * compositor indefinitely.
 *
 * There is no control on which client is allowed to subscribe to debug
 * messages. Any and all clients are allowed.
 *
 * The debug extension is disabled by default, and once enabled, cannot be
 * disabled again.
 *
 * @ingroup debug-protocol
 */
WL_EXPORT void
weston_compositor_enable_debug_protocol(struct weston_compositor *compositor)
{
	struct weston_log_context *log_ctx = compositor->weston_log_ctx;
	assert(log_ctx);
	if (log_ctx->global)
		return;

	log_ctx->global = wl_global_create(compositor->wl_display,
				       &weston_debug_v1_interface, 1,
				       log_ctx, weston_log_bind_weston_debug);
	if (!log_ctx->global)
		return;

	log_ctx->compositor_destroy_listener.notify = compositor_destroy_listener;
	wl_signal_add(&compositor->destroy_signal, &log_ctx->compositor_destroy_listener);

	fprintf(stderr, "WARNING: debug protocol has been enabled. "
		   "This is a potential denial-of-service attack vector and "
		   "information leak.\n");
}

/** Determine if the debug protocol has been enabled
 *
 * \param wc The libweston compositor to verify if debug protocol has been
 * enabled
 *
 * @ingroup debug-protocol
 */
WL_EXPORT bool
weston_compositor_is_debug_protocol_enabled(struct weston_compositor *wc)
{
	return wc->weston_log_ctx->global != NULL;
}

/** Register a new stream name, creating a log scope.
 *
 * @param log_ctx The weston_log_context where to add.
 * @param name The debug stream/scope name; must not be NULL.
 * @param description The log scope description for humans; must not be NULL.
 * @param new_subscription Optional callback when a client subscribes to this
 * scope.
 * @param destroy_subscription Optional callback when a client destroys the
 * subscription.
 * @param user_data Optional user data pointer for the callback.
 * @returns A valid pointer on success, NULL on failure.
 *
 * This function is used to create a log scope. All debug message printing
 * happens for a scope, which allows clients to subscribe to the kind of debug
 * messages they want by \c name. For the weston-debug protocol,
 * subscription for the scope will happen automatically but for other types of
 * streams, weston_log_subscribe() should be called as to create a subscription
 * and tie it to the scope created by this function.
 *
 * \p name must be unique in the weston_compositor instance. \p name
 * and \p description must both be provided. In case of the weston-debug
 * protocol, the description is printed when a client asks for a list of
 * supported log scopes.
 *
 * \p new_subscription, if not NULL, is called when a client subscribes to the log
 * scope creating a debug stream. This is for log scopes that need to print
 * messages as a response to a client appearing, e.g. printing a list of
 * windows on demand or a static preamble. The argument \p user_data is
 * passed in to the callback and is otherwise unused.
 *
 * For one-shot debug streams, \c new_subscription should finally call
 * weston_log_subscription_complete() to close the stream and tell the client
 * the printing is complete. Otherwise the client expects more data to be
 * written.  The complete callback in weston_log_subscriber should be installed
 * to trigger it and it is set-up automatically for the weston-debug protocol.
 *
 * As subscription can take place before creating the scope, any pending
 * subscriptions to scope added by weston_log_subscribe(), will be checked
 * against the scope being created and if found will be added to the scope's
 * subscription list.
 *
 * The log scope must be destroyed using weston_log_scope_destroy()
 * before destroying the weston_compositor.
 *
 * @memberof weston_log_scope
 * @sa weston_log_scope_cb, weston_log_subscribe
 */
WL_EXPORT struct weston_log_scope *
weston_log_ctx_add_log_scope(struct weston_log_context *log_ctx,
			     const char *name,
			     const char *description,
			     weston_log_scope_cb new_subscription,
			     weston_log_scope_cb destroy_subscription,
			     void *user_data)
{
	struct weston_log_scope *scope;
	struct weston_log_subscription *pending_sub = NULL;

	if (!name || !description) {
		fprintf(stderr, "Error: cannot add a debug scope without name or description.\n");
		return NULL;
	}

	if (!log_ctx) {
		fprintf(stderr, "Error: cannot add debug scope '%s', infra not initialized.\n",
			   name);
		return NULL;
	}

	if (weston_log_get_scope(log_ctx, name)){
		fprintf(stderr, "Error: debug scope named '%s' is already registered.\n",
			   name);
		return NULL;
	}

	scope = zalloc(sizeof *scope);
	if (!scope) {
		fprintf(stderr, "Error adding debug scope '%s': out of memory.\n",
			   name);
		return NULL;
	}

	scope->name = strdup(name);
	scope->desc = strdup(description);
	scope->new_subscription = new_subscription;
	scope->destroy_subscription = destroy_subscription;
	scope->user_data = user_data;
	wl_list_init(&scope->subscription_list);

	if (!scope->name || !scope->desc) {
		fprintf(stderr, "Error adding debug scope '%s': out of memory.\n",
			   name);
		free(scope->name);
		free(scope->desc);
		free(scope);
		return NULL;
	}

	wl_list_insert(log_ctx->scope_list.prev, &scope->compositor_link);

	/* check if there are any pending subscriptions to this scope */
	while ((pending_sub = find_pending_subscription(log_ctx, scope->name)) != NULL) {
		weston_log_subscription_create(pending_sub->owner, scope);

		/* remove it from pending */
		weston_log_subscription_destroy_pending(pending_sub);
	}


	return scope;
}

/** Register a new stream name, creating a log scope.
 *
 * @param compositor The compositor that contains the log context where the log
 * scope will be linked.
 * @param name The debug stream/scope name; must not be NULL.
 * @param description The log scope description for humans; must not be NULL.
 * @param new_subscription Optional callback when a client subscribes to this
 * scope.
 * @param destroy_subscription Optional callback when a client destroys the
 * subscription.
 * @param user_data Optional user data pointer for the callback.
 * @returns A valid pointer on success, NULL on failure.
 *
 * This function works like weston_log_ctx_add_log_scope(), but the log scope
 * created is linked to the log context of \c compositor.
 *
 * @memberof weston_compositor
 * @sa weston_log_ctx_add_log_scope
 */
WL_EXPORT struct weston_log_scope *
weston_compositor_add_log_scope(struct weston_compositor *compositor,
				const char *name,
				const char *description,
				weston_log_scope_cb new_subscription,
				weston_log_scope_cb destroy_subscription,
				void *user_data)
{
	struct weston_log_scope *scope;
	scope = weston_log_ctx_add_log_scope(compositor->weston_log_ctx,
					     name, description,
 					     new_subscription,
					     destroy_subscription,
					     user_data);
	return scope;
}

/** Destroy a log scope
 *
 * @param scope The log scope to destroy; may be NULL.
 *
 * Destroys the log scope, calling each stream's destroy callback if one was
 * installed/created.
 *
 * @memberof weston_log_scope
 */
WL_EXPORT void
weston_log_scope_destroy(struct weston_log_scope *scope)
{
	struct weston_log_subscription *sub, *sub_tmp;

	if (!scope)
		return;

	wl_list_for_each_safe(sub, sub_tmp, &scope->subscription_list, source_link)
		weston_log_subscription_destroy(sub);

	wl_list_remove(&scope->compositor_link);
	free(scope->name);
	free(scope->desc);
	free(scope);
}

/** Are there any active subscriptions to the scope?
 *
 * \param scope The log scope to check; may be NULL.
 * \return True if any streams are open for this scope, false otherwise.
 *
 * As printing some debugging messages may be relatively expensive, one
 * can use this function to determine if there is a need to gather the
 * debugging information at all. If this function returns false, all
 * printing for this scope is dropped, so gathering the information is
 * pointless.
 *
 * The return value of this function should not be stored, as new clients
 * may subscribe to the debug scope later.
 *
 * If the given scope is NULL, this function will always return false,
 * making it safe to use in teardown or destroy code, provided the
 * scope is initialized to NULL before creation and set to NULL after
 * destruction.
 *
 * \memberof weston_log_scope
 */
WL_EXPORT bool
weston_log_scope_is_enabled(struct weston_log_scope *scope)
{
	if (!scope)
		return false;

	return !wl_list_empty(&scope->subscription_list);
}

/** Close the stream's complete callback if one was installed/created.
 *
 * @ingroup log
 */
WL_EXPORT void
weston_log_subscription_complete(struct weston_log_subscription *sub)
{
	if (sub->owner && sub->owner->complete)
		sub->owner->complete(sub->owner);
}

/** Close the log scope.
 *
 * @param scope The log scope to complete; may be NULL.
 *
 * Complete the log scope, calling each stream's complete callback if one was
 * installed/created. This can be useful to signal the reading end that the
 * data has been transmitted and should no longer expect that written over the
 * stream. Particularly useful for the weston-debug protocol.
 *
 * @memberof weston_log_scope
 * @sa weston_log_ctx_add_log_scope, weston_compositor_add_log_scope,
 * weston_log_scope_destroy
 */
WL_EXPORT void
weston_log_scope_complete(struct weston_log_scope *scope)
{
	struct weston_log_subscription *sub;

	if (!scope)
		return;

	wl_list_for_each(sub, &scope->subscription_list, source_link)
		weston_log_subscription_complete(sub);
}

/** Write log data for a scope
 *
 * \param scope The debug scope to write for; may be NULL, in which case
 *              nothing will be written.
 * \param[in] data Pointer to the data to write.
 * \param len Number of bytes to write.
 *
 * Writes the given data to all subscribed clients' streams.
 *
 * \memberof weston_log_scope
 */
WL_EXPORT void
weston_log_scope_write(struct weston_log_scope *scope,
		       const char *data, size_t len)
{
	struct weston_log_subscription *sub;

	if (!scope)
		return;

	wl_list_for_each(sub, &scope->subscription_list, source_link)
		weston_log_subscription_write(sub, data, len);
}

/** Write a formatted string for a scope (varargs)
 *
 * \param scope The log scope to write for; may be NULL, in which case
 *              nothing will be written.
 * \param fmt Printf-style format string.
 * \param ap Formatting arguments.
 *
 * Writes to formatted string to all subscribed clients' streams.
 *
 * The behavioral details for each stream are the same as for
 * weston_debug_stream_write().
 *
 * \memberof weston_log_scope
 */
WL_EXPORT int
weston_log_scope_vprintf(struct weston_log_scope *scope,
			 const char *fmt, va_list ap)
{
	static const char oom[] = "Out of memory";
	char *str;
	int len = 0;

	if (!weston_log_scope_is_enabled(scope))
		return len;

	len = vasprintf(&str, fmt, ap);
	if (len >= 0) {
		weston_log_scope_write(scope, str, len);
		free(str);
	} else {
		weston_log_scope_write(scope, oom, sizeof oom - 1);
	}

	return len;
}

/** Write a formatted string for a scope
 *
 * \param scope The log scope to write for; may be NULL, in which case
 *              nothing will be written.
 * \param fmt Printf-style format string and arguments.
 *
 * Writes to formatted string to all subscribed clients' streams.
 *
 * The behavioral details for each stream are the same as for
 * weston_debug_stream_write().
 *
 * \memberof weston_log_scope
 */
WL_EXPORT int
weston_log_scope_printf(struct weston_log_scope *scope,
			  const char *fmt, ...)
{
	va_list ap;
	int len;

	va_start(ap, fmt);
	len = weston_log_scope_vprintf(scope, fmt, ap);
	va_end(ap);

	return len;
}

/** Write a formatted string for a subscription
 *
 * \param sub The subscription to write for; may be NULL, in which case
 *              nothing will be written.
 * \param fmt Printf-style format string and arguments.
 *
 * Writes to formatted string to the stream that created the subscription.
 *
 * @ingroup log
 */
WL_EXPORT void
weston_log_subscription_printf(struct weston_log_subscription *sub,
			       const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	weston_log_subscription_vprintf(sub, fmt, ap);
	va_end(ap);
}

/** Write debug scope name and current time into string
 *
 * \param[in] scope debug scope; may be NULL
 * \param[out] buf Buffer to store the string.
 * \param len Available size in the buffer in bytes.
 * \return \c buf
 *
 * Reads the current local wall-clock time and formats it into a string.
 * and append the debug scope name to it, if a scope is available.
 * The string is NUL-terminated, even if truncated.
 *
 * @memberof weston_log_scope
 */
WL_EXPORT char *
weston_log_scope_timestamp(struct weston_log_scope *scope,
			   char *buf, size_t len)
{
	struct timeval tv;
	struct tm *bdt;
	char string[128];
	size_t ret = 0;

	gettimeofday(&tv, NULL);

	bdt = localtime(&tv.tv_sec);
	if (bdt)
		ret = strftime(string, sizeof string,
			       "%Y-%m-%d %H:%M:%S", bdt);

	if (ret > 0) {
		snprintf(buf, len, "[%s.%03ld][%s]", string,
			 tv.tv_usec / 1000,
			 (scope) ? scope->name : "no scope");
	} else {
		snprintf(buf, len, "[?][%s]",
			 (scope) ? scope->name : "no scope");
	}

	return buf;
}

/** Returns a timestamp useful for adding it to a log scope.
 *
 * @example
 * char timestr[128];
 * static int cached_dm = -1;
 * char *time_buff = weston_log_timestamp(timestr, sizeof(timestr),  &cached_dm);
 * weston_log_scope_printf(log_scope, "%s %s", time_buff, other_data);
 *
 * @param buf a user-supplied buffer
 * @param len user-supplied length of the buffer
 * @param cached_tm_mday a cached day of the month, as an integer. Setting this
 * pointer different from NULL, to an integer value other than was retrieved as
 * current day of the month, would add an additional line under the form of
 * 'Date: Y-m-d Z\n'. Setting the pointer to NULL would not print any date, nor
 * if the value matches the current day of month. Helps identify logs that
 * spawn multiple days, while still having a shorter time stamp format.
 * @ingroup log
 */
WL_EXPORT char *
weston_log_timestamp(char *buf, size_t len, int *cached_tm_mday)
{
       struct timeval tv;
       struct tm *brokendown_time;
       char datestr[128];
       char timestr[128];

       gettimeofday(&tv, NULL);

       brokendown_time = localtime(&tv.tv_sec);
       if (brokendown_time == NULL) {
               snprintf(buf, len, "%s", "[(NULL)localtime] ");
               return buf;
       }

       memset(datestr, 0, sizeof(datestr));
       if (cached_tm_mday && brokendown_time->tm_mday != *cached_tm_mday) {
               strftime(datestr, sizeof(datestr), "Date: %Y-%m-%d %Z\n",
                        brokendown_time);
               *cached_tm_mday = brokendown_time->tm_mday;
       }

       strftime(timestr, sizeof(timestr), "%H:%M:%S", brokendown_time);
       /* if datestr is empty it prints only timestr*/
       snprintf(buf, len, "%s[%s.%03li]", datestr,
                timestr, (tv.tv_usec / 1000));

       return buf;
}


void
weston_log_subscriber_release(struct weston_log_subscriber *subscriber)
{
	struct weston_log_subscription *sub, *sub_tmp;

	wl_list_for_each_safe(sub, sub_tmp, &subscriber->subscription_list, owner_link)
		weston_log_subscription_destroy(sub);
}

/** Destroy a file type or a flight-rec type subscriber.
 *
 * They are created, respectively, with weston_log_subscriber_create_log()
 * and weston_log_subscriber_create_flight_rec()
 *
 * @param subscriber the weston_log_subscriber object to destroy
 *
 * @ingroup log
 */
WL_EXPORT void
weston_log_subscriber_destroy(struct weston_log_subscriber *subscriber)
{
	subscriber->destroy(subscriber);
}

/** Subscribe to a scope
 *
 * Creates a subscription which is used to subscribe the \p subscriber
 * to the scope \c scope_name.
 *
 * If \c scope_name has already been created (using
 * weston_log_ctx_add_log_scope or weston_compositor_add_log_scope) the
 * subscription will take place immediately, otherwise we store the
 * subscription into a pending list. See also weston_log_ctx_add_log_scope()
 * and weston_compositor_add_log_scope()
 *
 * @param log_ctx the log context, used for accessing pending list
 * @param subscriber the subscriber, which has to be created before
 * @param scope_name the scope name. In case the scope is not created
 * we temporarily store the subscription in the pending list.
 *
 * @ingroup log
 */
WL_EXPORT void
weston_log_subscribe(struct weston_log_context *log_ctx,
		     struct weston_log_subscriber *subscriber,
		     const char *scope_name)
{
	assert(log_ctx);
	assert(subscriber);
	assert(scope_name);

	struct weston_log_scope *scope;

	scope = weston_log_get_scope(log_ctx, scope_name);
	if (scope)
		weston_log_subscription_create(subscriber, scope);
	else
		/*
		 * if we don't have already as scope for it, add it to pending
		 * subscription list
		 */
		weston_log_subscription_create_pending(subscriber, scope_name, log_ctx);
}

/** Iterate over all subscriptions in a scope
 *
 * @param scope the scope for which you want to iterate
 * @param sub_iter the iterator, use NULL to start from the 'head'
 * @returns the next subscription from the log scope
 *
 * This is (quite) useful when 'log_scope' and 'log_subscription' are opaque. Do note
 * that \c sub_iter needs to be NULL-initialized before calling this function.
 *
 */
WL_EXPORT struct weston_log_subscription *
weston_log_subscription_iterate(struct weston_log_scope *scope,
				struct weston_log_subscription *sub_iter)
{
	struct wl_list *list = &scope->subscription_list;
	struct wl_list *node;

	/* go to the next item in the list or if not set starts with the head */
	if (sub_iter)
		node = sub_iter->source_link.next;
	else
		node = list->next;

	assert(node);
	assert(!sub_iter || node != &sub_iter->source_link);

	/* if we're at the end */
	if (node == list)
		return NULL;

	return container_of(node, struct weston_log_subscription, source_link);
}
