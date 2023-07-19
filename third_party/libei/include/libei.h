/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2020 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @addtogroup libei EI - The client API
 *
 * libei is the client-side module. This API should be used by processes
 * that need to emulate devices.
 *
 * libei clients come in "sender" and "receiver" modes, depending on whether
 * the client sends or receives events. A libeis context however may accept
 * both sender and receiver clients, the EIS implementation works as
 * corresponding receiver or sender for this client. It is up to the
 * implementation to disconnect clients that it does not want to allow. See
 * eis_client_is_sender() for details.
 *
 * @{
 */

/**
 * @struct ei
 *
 * The main context to interact with libei. A libei context is a single
 * connection to an EIS implementation and may contain multiple devices, see
 * @ref ei_device.
 *
 * An @ref ei context is refcounted, see ei_unref().
 */
struct ei;

/**
 * @struct ei_device
 *
 * A single device to generate input events from. A device may have multiple
 * capabilities. For example, a single device may be a pointer and a keyboard
 * and a touch device. It is up to the EIS implementation on how to handle
 * this case, some implementations may split a single device up into
 * multiple virtual devices, others may not.
 *
 * An @ref ei_device is refcounted, see ei_device_unref().
 */
struct ei_device;

/**
 * @struct ei_seat
 *
 * A logical seat for a group of devices. Seats are provided by the EIS
 * implementation, devices may be added to a seat. The hierarchy of objects
 * looks like this:
 * <pre>
 *    ei ---- ei_seat one ---- ei_device 1
 *       \                \
 *        \                --- ei device 2
 *         --- ei_seat two --- ei device 3
 * </pre>
 */
struct ei_seat;

/**
 * @struct ei_event
 *
 * An event received from the EIS implementation. See @ref ei_event_type
 * for the list of possible event types.
 *
 * An @ref ei_event is refcounted, see ei_event_unref().
 */
struct ei_event;

/**
 * @struct ei_keymap
 *
 * An keymap for a device with the @ref EI_DEVICE_CAP_KEYBOARD capability.
 *
 * An @ref ei_keymap is refcounted, see ei_keymap_unref().
 */
struct ei_keymap;

/**
 * @struct ei_region
 *
 * A rectangular region, defined by an x/y offset and a width and a height.
 * A region defines the area on an EIS desktop layout that is accessible by
 * this device - this region may not be the full area of the desktop.
 * Input events may only be sent for points within the regions.
 *
 * The use of regions is private to the EIS compositor and coordinates may not
 * match the size of the actual desktop. For example, a compositor may set a
 * 1920x1080 region to represent a 4K monitor and transparently map input
 * events into the respective true pixels.
 *
 * Absolute devices may have different regions, it is up to the libei client
 * to send events through the correct device to target the right pixel. For
 * example, a dual-head setup my have two absolute devices, the first with a
 * zero offset region spanning the first screen, the second with a nonzero
 * offset spanning the second screen.
 */
struct ei_region;

/**
 * @enum ei_device_type
 *
 * The device type determines what the device represents.
 *
 * If the device type is @ref EI_DEVICE_TYPE_VIRTUAL, the device is a
 * virtual device representing input as applied on the EIS implementation's
 * screen. A relative virtual device generates input events in logical pixels,
 * an absolute virtual device generates input events in logical pixels on one
 * of the device's regions. Virtual devices do not have a size.
 *
 * If the device type is @ref EI_DEVICE_TYPE_PHYSICAL, the device is a
 * representation of a physical device as if connected to the EIS
 * implementation's host computer. A relative physical device generates input
 * events in mm, an absolute physical device generates input events in mm
 * within the device's specified physical size. Physical devices do not have
 * regions.
 *
 * @see ei_device_get_width
 * @see ei_device_get_height
 */
enum ei_device_type {
	EI_DEVICE_TYPE_VIRTUAL = 1,
	EI_DEVICE_TYPE_PHYSICAL
};

/**
 * @enum ei_device_capability
 *
 * The set of supported capabilities. A device may have zero or more
 * capabilities, a device with perceived zero capabilities is typically a
 * device with capabilities unsupported by the client.
 *
 * Capabilities are initialized by the EIS implementation but the client may
 * further reduce the requested capabilities, see ei_seat_confirm_capability().
 * For example, a client may bind to a seat with the pointer and keyboard
 * capability but only the former is permitted by the EIS implementation.
 * Keyboard events sent through such a device will be treated as client bug
 * and discarded.
 *
 * See ei_device_has_capability().
 *
 */
enum ei_device_capability {
	EI_DEVICE_CAP_POINTER = 1,
	EI_DEVICE_CAP_POINTER_ABSOLUTE = 2,
	EI_DEVICE_CAP_KEYBOARD = 4,
	EI_DEVICE_CAP_TOUCH = 8,
};

/**
 * @enum ei_keymap_type
 *
 * The set of supported keymap types for a struct @ref ei_keymap.
 */
enum ei_keymap_type {
	/**
	 * A libxkbcommon-compatible XKB keymap.
	 */
	EI_KEYMAP_TYPE_XKB = 1,
};

enum ei_event_type {
	/**
	 * The server has approved the connection to this client. Where the
	 * server does not approve the connection, @ref EI_EVENT_DISCONNECT is
	 * sent instead.
	 *
	 * This event is only sent once after the initial connection
	 * request.
	 */
	EI_EVENT_CONNECT = 1,

	/**
	 * The server has disconnected this client - all resources left to
	 * reference this server are now obsolete. Once this event has been
	 * received, the struct @ref ei and all its associated resources
	 * should be released.
	 *
	 * This event may occur at any time after the connection has been
	 * made and is the last event to be received by this ei instance.
	 *
	 * libei guarantees that a @ref EI_EVENT_DISCONNECT is provided to
	 * the caller even where the server does not send one.
	 */
	EI_EVENT_DISCONNECT,

	/**
	 * The server has added a seat available to this client.
	 *
	 * libei guarantees that any seat added has a corresponding @ref
	 * EI_EVENT_SEAT_REMOVED event before @ref EI_EVENT_DISCONNECT.
	 * libei guarantees that any device in this seat generates a @ref
	 * EI_EVENT_DEVICE_REMOVED event before the @ref
	 * EI_EVENT_SEAT_REMOVED event.
	 */
	EI_EVENT_SEAT_ADDED,

	/**
	 * The server has removed a seat previously available to this
	 * client. The caller should release the struct @ref ei_seat and
	 * all its associated resources. No devices will be added to this seat
	 * anymore.
	 *
	 * libei guarantees that any device in this seat generates a @ref
	 * EI_EVENT_DEVICE_REMOVED event before the @ref
	 * EI_EVENT_SEAT_REMOVED event.
	 */
	EI_EVENT_SEAT_REMOVED,

	/**
	 * The server has added a device for this client. The capabilities
	 * of the device may be a subset of the seat capabilities - it is up
	 * to the client to verify the minimum required capabilities are
	 * indeed set.
	 *
	 * libei guarantees that any device added has a corresponding @ref
	 * EI_EVENT_DEVICE_REMOVED event before @ref EI_EVENT_DISCONNECT.
	 */
	EI_EVENT_DEVICE_ADDED,

	/**
	 * The server has removed a device belonging to this client. The
	 * caller should release the struct @ref ei_device and all its
	 * associated resources. Any events sent through a removed device
	 * are discarded.
	 *
	 * When this event is received, the device is already removed. A
	 * caller does not need to call ei_device_close() event on this
	 * device.
	 */
	EI_EVENT_DEVICE_REMOVED,

	/**
	 * Any events sent from this device will be discarded until the next
	 * resume. The state of a device is not expected to change between
	 * pause/resume - for any significant state changes the server is
	 * expected to remove the device instead.
	 */
	EI_EVENT_DEVICE_PAUSED,
	/**
	 * The client may send events.
	 */
	EI_EVENT_DEVICE_RESUMED,

	/**
	 * The server has changed the modifier state on the device's
	 * keymap. See
	 * ei_event_keyboard_get_xkb_mods_depressed(),
	 * ei_event_keyboard_get_xkb_mods_latched(),
	 * ei_event_keyboard_get_xkb_mods_locked(), and
	 * ei_event_keyboard_get_xkb_group().
	 *
	 * This event is sent in response to an external modifier state
	 * change. Where the client triggers a modifier state change in
	 * response to ei_device_keyboard_key(), no such event is sent.
	 *
	 * This event may arrive while a device is paused.
	 */
	EI_EVENT_KEYBOARD_MODIFIERS,

	/**
	 * "Hardware" frame event. This event **must** be sent by the server
	 * and notifies the client that the previous set of events belong to
	 * the same logical hardware event.
	 *
	 * These events are only generated on a receiver ei context.
	 *
	 * This event is most commonly used to implement multitouch (multiple
	 * touches may update within the same hardware scanout cycle).
	 */
	EI_EVENT_FRAME = 100,

	/**
	 * The server is about to send events for a device. This event should
	 * be used by the client to clear the logical state of the emulated
	 * devices and/or provide UI to the user.
	 *
	 * These events are only generated on a receiver ei context.
	 *
	 * Note that a server start multiple emulating sequences
	 * simultaneously, depending on the devices available.
	 * For example, in a synergy-like situation, the server
	 * may start sending pointer and keyboard once the remote device
	 * logically entered the screen.
	 */
	EI_EVENT_DEVICE_START_EMULATING = 200,
	EI_EVENT_DEVICE_STOP_EMULATING,

	/* These events are only generated on a receiver ei context. */
	EI_EVENT_POINTER_MOTION = 300,
	EI_EVENT_POINTER_MOTION_ABSOLUTE,
	EI_EVENT_POINTER_BUTTON,
	EI_EVENT_POINTER_SCROLL,
	EI_EVENT_POINTER_SCROLL_STOP,
	EI_EVENT_POINTER_SCROLL_CANCEL,
	EI_EVENT_POINTER_SCROLL_DISCRETE,

	EI_EVENT_KEYBOARD_KEY = 400,

	EI_EVENT_TOUCH_DOWN = 500,
	EI_EVENT_TOUCH_UP,
	EI_EVENT_TOUCH_MOTION,
};

/**
 * This is an alias for @ref ei_new_sender.
 */
struct ei *
ei_new(void *user_data);

/**
 * Create a new sender ei context. The context is refcounted and must be
 * released with ei_unref().
 *
 * A sender ei context sends events to the EIS implementation but cannot
 * receive events.
 *
 * A context supports exactly one backend, set up with one of
 * ei_setup_backend_socket() or ei_setup_backend_fd().
 *
 * @param user_data An opaque pointer to be returned with ei_get_user_data()
 *
 * @see ei_set_user_data
 * @see ei_get_user_data
 * @see ei_setup_backend_fd
 * @see ei_setup_backend_socket
 */
struct ei *
ei_new_sender(void *user_data);

/**
 * Create a new receiver ei context. The context is refcounted and must be
 * released with ei_unref().
 *
 * A receiver ei context receives events from the EIS implementation but cannot
 * send events.
 *
 * A context supports exactly one backend, set up with one of
 * ei_setup_backend_socket() or ei_setup_backend_fd().
 *
 * @param user_data An opaque pointer to be returned with ei_get_user_data()
 *
 * @see ei_set_user_data
 * @see ei_get_user_data
 * @see ei_setup_backend_fd
 * @see ei_setup_backend_socket
 */
struct ei *
ei_new_receiver(void *user_data);

bool
ei_is_sender(struct ei *ei);

enum ei_log_priority {
	EI_LOG_PRIORITY_DEBUG = 10,
	EI_LOG_PRIORITY_INFO = 20,
	EI_LOG_PRIORITY_WARNING = 30,
	EI_LOG_PRIORITY_ERROR = 40,
};

struct ei_log_context;

/**
 * @return the line number (``__LINE__``) for a given log message context.
 */
unsigned int
ei_log_context_get_line(struct ei_log_context *ctx);

/**
 * @return the file name (``__FILE__``) for a given log message context.
 */
const char *
ei_log_context_get_file(struct ei_log_context *ctx);

/**
 * @return the function name (``__func__``) for a given log message context.
 */
const char *
ei_log_context_get_func(struct ei_log_context *ctx);

/**
 * The log handler for library logging. This handler is only called for
 * messages with a log level equal or greater than than the one set in
 * ei_log_set_priority().
 *
 * The context passed to this function contains auxilary information about
 * this log message such as the line number, file name and function name
 * this message occured in. The log context is valid only within the current
 * invocation of the log handler.
 *
 * @param ei The EI context
 * @param priority The log priority
 * @param message The log message as a null-terminated string
 * @param context A log message context for this message
 */
typedef void (*ei_log_handler)(struct ei *ei,
			       enum ei_log_priority priority,
			       const char *message,
			       struct ei_log_context *context);
/**
 * Change the log handler for this context. If the log handler is NULL, the
 * built-in default log function is used.
 *
 * @param ei The EI context
 * @param log_handler The log handler or NULL to use the default log
 * handler.
 */
void
ei_log_set_handler(struct ei *ei, ei_log_handler log_handler);

void
ei_log_set_priority(struct ei *ei, enum ei_log_priority priority);

enum ei_log_priority
ei_log_get_priority(const struct ei *ei);

/**
 * Set the name for this client. This is a suggestion to the
 * server only and may not be honored.
 *
 * The client name may be used for display to the user, for example in
 * an authorization dialog that requires the user to approve a connection to
 * the EIS implementation.
 *
 * This function must be called immediately after ei_new() and before
 * setting up a backend with ei_setup_backend_socket() or
 * ei_setup_backend_fd().
 */
void
ei_configure_name(struct ei * ei, const char *name);

/**
 * Set this ei context to use the socket backend. The ei context will
 * connect to the socket at the given path and initiate the conversation
 * with the EIS server listening on that socket.
 *
 * If @a socketpath is `NULL`, the value of the environment variable
 * `LIBEI_SOCKET` is used. If @a socketpath does not start with '/', it is
 * relative to `$XDG_RUNTIME_DIR`. If `XDG_RUNTIME_DIR` is not set, this
 * function fails.
 *
 * If the connection was successful, an event of type @ref EI_EVENT_CONNECT
 * or @ref EI_EVENT_DISCONNECT will become available after a future call to
 * ei_dispatch().
 *
 * If the connection failed, use ei_unref() to release the data allocated
 * for this context.
 *
 * @return zero on success or a negative errno on failure
 */
int
ei_setup_backend_socket(struct ei *ei, const char *socketpath);

/**
 * Initialize the ei context on the given socket. The ei context will
 * initiate the conversation with the EIS server listening on the other end
 * of this socket.
 *
 * If the connection was successful, an event of type @ref EI_EVENT_CONNECT
 * or @ref EI_EVENT_DISCONNECT will become available after a future call to
 * ei_dispatch().
 *
 * If the connection failed, use ei_unref() to release the data allocated
 * for this context.
 *
 * This function takes ownership of the file descriptor, and will close it
 * when tearing down.
 *
 * @return zero on success or a negative errno on failure
 */
int
ei_setup_backend_fd(struct ei *ei, int fd);

/**
 * Increase the refcount of this struct by one. Use ei_unref() to decrease
 * the refcount.
 *
 * @return the argument passed into the function
 */
struct ei *
ei_ref(struct ei *ei);

/**
 * Decrease the refcount of this struct by one. When the refcount reaches
 * zero, the context disconnects from the server and all allocated resources
 * are released.
 *
 * @return always NULL
 */
struct ei *
ei_unref(struct ei *ei);

/**
 * Set a custom data pointer for this context. libei will not look at or
 * modify the pointer. Use ei_get_user_data() to retrieve a previously set
 * user data.
 */
void
ei_set_user_data(struct ei *ei, void *user_data);

/**
 * Return the custom data pointer for this context. libei will not look at or
 * modify the pointer. Use ei_set_user_data() to change the user data.
 */
void *
ei_get_user_data(struct ei *ei);

/**
 * libei keeps a single file descriptor for all events. This fd should be
 * monitored for events by the caller's mainloop, e.g. using select(). When
 * events are available on this fd, call ei_dispatch() immediately to
 * process.
 */
int
ei_get_fd(struct ei *ei);

/**
 * Main event dispatching function. Reads events of the file descriptors
 * and processes them internally. Use ei_get_event() to retrieve the
 * events.
 *
 * Dispatching does not necessarily queue events. This function
 * should be called immediately once data is available on the file
 * descriptor returned by libei_get_fd().
 */
void
ei_dispatch(struct ei *ei);

/**
 * Return the next event from the event queue, removing it from the queue.
 *
 * The returned object must be released by the caller with ei_event_unref()
 */
struct ei_event *
ei_get_event(struct ei *ei);

/**
 * Returns the next event in the internal event queue (or `NULL`) without
 * removing that event from the queue; the next call to ei_get_event()
 * will return that same event.
 *
 * This call is useful for checking whether there is an event and/or what
 * type of event it is.
 *
 * Repeated calls to ei_peek_event() return the same event.
 *
 * The returned event is refcounted, use ei_event_unref() to drop the
 * reference.
 *
 * A caller must not call ei_get_event() while holding a ref to an event
 * returned by ei_peek_event(). Doing so is undefined behavior.
 */
struct ei_event *
ei_peek_event(struct ei *ei);

/**
 * Release resources associated with this event. This function always
 * returns NULL.
 *
 * The caller cannot increase the refcount of an event. Events should be
 * considered transient data and not be held longer than required.
 * ei_event_unref() is provided for consistency (as opposed to, say,
 * ei_event_free()).
 */
struct ei_event *
ei_event_unref(struct ei_event *event);

const char *
ei_seat_get_name(struct ei_seat *seat);

/**
 * Return true if the capabilitiy is available on this seat or false
 * otherwise. The return value of this function is not affected by
 * ei_seat_confirm_capability().
 */
bool
ei_seat_has_capability(struct ei_seat *seat,
		       enum ei_device_capability cap);

/**
 * Bind this client to the given seat capability. Once bound, the server may
 * create devices for the requested capability and send the respective @ref
 * EI_EVENT_DEVICE_ADDED events. To undo, call ei_seat_unbind_capability().
 *
 * Note that binding to a capability does not guarantee a device for that
 * capability becomes available. Devices may be added and removed at any time.
 *
 * It is an application bug to call this function for a capability already
 * bound - call ei_seat_unbind_capability() first.
 *
 * Calling this function for a capability that does not exist on the seat is
 * permitted (but obviously a noop)
 */
void
ei_seat_bind_capability(struct ei_seat *seat,
		        enum ei_device_capability cap);

/**
 * Same as ei_seat_bind_capability() but takes multiple capabilities,
 * terminated by NULL.
 */
__attribute__((sentinel))
void
ei_seat_bind_capabilities(struct ei_seat *seat, ...);

/**
 * Unbind a seat's capability. This function indicates the the application is
 * no longer interested in devices with the given capability.
 *
 * If any devices with the given capability are present, libei automatically
 * calls ei_device_close() on those devices (and thus the server will send
 * @ref EI_EVENT_DEVICE_REMOVED for those devices).
 */
void
ei_seat_unbind_capability(struct ei_seat *seat,
			  enum ei_device_capability cap);

/**
 * Same as ei_seat_unbind_capability() but takes multiple capabilities, terminated
 * by NULL.
 */
__attribute__((sentinel))
void
ei_seat_unbind_capabilities(struct ei_seat *seat, ...);


struct ei_seat *
ei_seat_ref(struct ei_seat *seat);

struct ei_seat *
ei_seat_unref(struct ei_seat *seat);

/**
 * Return the struct @ref ei context this seat is associated with.
 */
struct ei *
ei_seat_get_context(struct ei_seat *seat);

/**
 * @return the type of this event
 */
enum ei_event_type
ei_event_get_type(struct ei_event *event);

/**
 * Return the device from this event.
 *
 * For events of type @ref EI_EVENT_CONNECT and @ref EI_EVENT_DISCONNECT,
 * this function returns NULL.
 *
 * This does not increase the refcount of the device. Use eis_device_ref()
 * to keep a reference beyond the immediate scope.
 */
struct ei_device *
ei_event_get_device(struct ei_event *event);

/**
 * Return the time for the event of type @ref EI_EVENT_FRAME in microseconds.
 *
 * @note: Only events of type @ref EI_EVENT_FRAME carry a timestamp. For
 * convenience, the timestamp for other device events is retrofitted by this
 * library.
 *
 * @return the event time in microseconds
 */
uint64_t
ei_event_get_time(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_KEYBOARD_MODIFIERS, get the
 * mask of currently logically pressed-down modifiers.
 * See ei_device_get_keymap() for the corresponding keymap.
 */
uint32_t
ei_event_keyboard_get_xkb_mods_depressed(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_KEYBOARD_MODIFIERS, get the
 * mask of currently logically latched modifiers.
 * See ei_device_get_keymap() for the corresponding keymap.
 */
uint32_t
ei_event_keyboard_get_xkb_mods_latched(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_KEYBOARD_MODIFIERS, get the
 * mask of currently logically locked modifiers.
 * See ei_device_get_keymap() for the corresponding keymap.
 */
uint32_t
ei_event_keyboard_get_xkb_mods_locked(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_KEYBOARD_MODIFIERS, get the
 * logical group state.
 * See ei_device_get_keymap() for the corresponding keymap.
 */
uint32_t
ei_event_keyboard_get_xkb_group(struct ei_event *event);

/**
 * Increase the refcount of this struct by one. Use ei_device_unref() to
 * decrease the refcount.
 *
 * @return the argument passed into the function
 */
struct ei_device *
ei_device_ref(struct ei_device *device);

/**
 * Decrease the refcount of this struct by one. When the refcount reaches
 * zero, the context disconnects from the server and all allocated resources
 * are released.
 *
 * @return always NULL
 */
struct ei_device *
ei_device_unref(struct ei_device *device);

struct ei_seat *
ei_device_get_seat(struct ei_device *device);

/**
 * Set a custom data pointer for this context. libei will not look at or
 * modify the pointer. Use ei_seat_get_user_data() to retrieve a
 * previously set user data.
 */
void
ei_seat_set_user_data(struct ei_seat *seat, void *user_data);

/**
 * Return the custom data pointer for this context. libei will not look at or
 * modify the pointer. Use ei_seat_get_user_data() to change the user data.
 */
void *
ei_seat_get_user_data(struct ei_seat *seat);

/**
 * Set a custom data pointer for this context. libei will not look at or
 * modify the pointer. Use ei_device_get_user_data() to retrieve a
 * previously set user data.
 */
void
ei_device_set_user_data(struct ei_device *device, void *user_data);

/**
 * Return the custom data pointer for this context. libei will not look at or
 * modify the pointer. Use ei_device_get_user_data() to change the user data.
 */
void *
ei_device_get_user_data(struct ei_device *device);

/**
 * Return the width of the device in mm if the device is of type @ref
 * EI_DEVICE_TYPE_PHYSICAL, otherwise zero.
 */
uint32_t
ei_device_get_width(struct ei_device *device);

/**
 * Return the height of the device in mm if the device is of type @ref
 * EI_DEVICE_TYPE_PHYSICAL, otherwise zero.
 */
uint32_t
ei_device_get_height(struct ei_device *device);


struct ei_keymap *
ei_device_get_keymap(struct ei_device *device);

/**
 * @return the size of the keymap in bytes
 */
size_t
ei_keymap_get_size(struct ei_keymap *keymap);

/**
 * Returns the type for this keymap. The type specifies how to interpret the
 * data at the file descriptor returned by ei_keymap_get_fd().
 */
enum ei_keymap_type
ei_keymap_get_type(struct ei_keymap *keymap);

/**
 * Return a memmap-able file descriptor pointing to the keymap used by the
 * device. The keymap is constant for the lifetime of the device and
 * assigned to this device individually.
 */
int
ei_keymap_get_fd(struct ei_keymap *keymap);

/**
 * Return the device this keymap belongs to, or `NULL` if it has not yet
 * been assigned to a device.
 *
 * After processing and if the server changed the keymap or set the keymap
 * to NULL, this keymap may no longer be in use by the device and future
 * calls to this function return `NULL`.
 */
struct ei_device *
ei_keymap_get_device(struct ei_keymap *keymap);

/**
 * Increase the refcount of this struct by one. Use ei_keymap_unref() to
 * decrease the refcount.
 *
 * @return the argument passed into the function
 */
struct ei_keymap *
ei_keymap_ref(struct ei_keymap *keymap);

/**
 * Decrease the refcount of this struct by one. When the refcount reaches
 * zero, the context disconnects from the server and all allocated resources
 * are released.
 *
 * @return always NULL
 */
struct ei_keymap *
ei_keymap_unref(struct ei_keymap *keymap);

void
ei_keymap_set_user_data(struct ei_keymap *keymap, void *user_data);

void *
ei_keymap_get_user_data(struct ei_keymap *keymap);

/**
 * Notify the server that the client is no longer interested in
 * this device.
 *
 * Due to the asynchronous nature of the client-server interaction,
 * events for this device may still be in transit. The server will send an
 * @ref EI_EVENT_DEVICE_REMOVED event for this device. After that event,
 * device is considered removed by the server.
 *
 * A client can assume that an @ref EI_EVENT_DEVICE_REMOVED event is sent
 * for any device for which ei_device_close() was called before the @ref
 * EI_EVENT_DISCONNECT event. Where a client gets
 * disconnected libei will emulate that event.
 *
 * This does not release any resources associated with this device, use
 * ei_device_unref() for any references held by the client.
 */
void
ei_device_close(struct ei_device *device);

/**
 * @return the name of the device (if any) or NULL
 */
const char *
ei_device_get_name(struct ei_device *device);

/**
 * The device type of the device is determined by the type of the ei
 * context. If the client context was created with ei_new_sender(), the device
 * type defaults to @ref EI_DEVICE_TYPE_VIRTUAL. If the client context was
 * created with ei_new_receiver(), the device type defaults to @ref
 * EI_DEVICE_TYPE_PHYSICAL.
 */
enum ei_device_type
ei_device_get_type(struct ei_device *device);

/**
 * Return true if the device has the requested capability. Device
 * capabilities are constant.
 */
bool
ei_device_has_capability(struct ei_device *device,
			 enum ei_device_capability cap);


/**
 * Obtain a region from a device of type @ref EI_DEVICE_TYPE_VIRTUAL. The
 * number of regions is constant for a device and the indices of any region
 * remains the same for the lifetime of the device.
 *
 * Regions are shared between all capabilities. Where two capabilities need
 * different regions, the EIS implementation must create multiple devices with
 * individual capabilities and regions. For example, two touchscreens that are
 * mapped to two screens would typically show up as two separate devices with
 * one region each.
 *
 * This function returns the given region or NULL if the index is larger than
 * the number of regions available.
 *
 * This does not increase the refcount of the region. Use ei_region_ref() to
 * keep a reference beyond the immediate scope.
 *
 * Devices of type @ref EI_DEVICE_TYPE_PHYSICAL do not have regions.
 */
struct ei_region *
ei_device_get_region(struct ei_device *device, size_t index);

struct ei_region *
ei_region_ref(struct ei_region *region);

struct ei_region *
ei_region_unref(struct ei_region *region);

void
ei_region_set_user_data(struct ei_region *region, void *user_data);

void *
ei_region_get_user_data(struct ei_region *region);

uint32_t
ei_region_get_x(struct ei_region *region);

uint32_t
ei_region_get_y(struct ei_region *region);

uint32_t
ei_region_get_width(struct ei_region *region);

uint32_t
ei_region_get_height(struct ei_region *region);

/**
 * Return true if the point x/y (in desktop-wide coordinates) is within @a
 * region.
 */
bool
ei_region_contains(struct ei_region *region, double x, double y);

/**
 * Convert the point x/y in a desktop-wide coordinate system into the
 * corresponding point relative to the offset of the given region.
 * If the point is inside the region, this function returns true and @a x and @a
 * y are set to the points with the region offset subtracted.
 * If the point is outside the region, this function returns false and @a x
 * and @a y are left unmodified.
 */
bool
ei_region_convert_point(struct ei_region *region, double *x, double *y);

/**
 * Return the physical scale for this region. The default scale is 1.0.
 *
 * The regions' coordinate space is in logical pixels in the EIS range. The
 * logical pixels may or may not match the physical pixels on the output
 * range but the mapping from logical pixels to physical pixels is performed
 * by the EIS implementation.
 *
 * In some use-cases though, relative data from a remote input source needs
 * to be converted by the libei client into an absolute movement on an EIS
 * region. In that case, the physical scale provides the factor to multiply
 * the relative logical input to provide the expected physical relative
 * movement.
 *
 * For example consider the following dual-monitor setup comprising a 2k and
 * a 4k monitor **of the same physical size**:
 * The physical layout of the monitors appears like this:
 * @code
 *        2k            4k
 *  +-------------++-------------+
 *  |             ||             |
 *  |  a   b      ||   c   d     |
 *  |             ||             |
 *  +-------------++-------------+
 * @endcode
 *
 * The physical distance `ab` is the same as the physical distance `cd`.
 * Where the EIS implementation supports high-dpi screens, the logical
 * distance (in pixels) are identical too.
 *
 * Where the EIS implementation does not support high-dpi screens, the
 * logical layout of these two monitors appears like this:
 *
 * @code
 *        2k            4k
 *  +-------------++--------------------------+
 *  |             ||                          |
 *  |  a   b      ||                          |
 *  |             ||                          |
 *  +-------------+|     c       d            |
 *                 |                          |
 *                 |                          |
 *                 |                          |
 *                 +--------------------------+
 * @endcode
 *
 * While the two physical distances `ab` and `cd` are still identical, the
 * logical distance `cd` (in pixels) is twice that of `ab`.
 * Where a libei client receives relative deltas from an input source and
 * converts that relative input into an absolute position on the screen, it
 * needs to take this into account.
 *
 * For example, if a remote input source moves by relative 100 logical
 * pixels, the libei client would convert this as `a + 100 = b` on the
 * region for the 2k screen and send the absolute events to logically change
 * the position from `a` to `b`. If the same remote input source moves by
 * relative 100 logical pixels, the libei client would convert this as
 * `c + 100 * scale = d` on the region for the 4k screen to logically
 * change the position from `c` to `d`. While the pixel movement differs,
 * the physical movement as seen by the user is thus identical.
 *
 * A second possible use-case for the physical scale is to match pixels from
 * one region to their respective counterpart on a different region.
 * For example, if the bottom-right corner of the 2k screen in the
 * illustration above has a coordinate of ``(x, y)``, the neighbouring pixel on
 * the **physical** 4k screen is ``(0, y * scale)``.
 */
double
ei_region_get_physical_scale(struct ei_region *region);

/**
 * Return the keymap for this device or `NULL`. The keymap is constant for
 * the lifetime of the device and applies to this device individually.
 *
 * If this function returns `NULL`, this device does not have
 * an individual keymap assigned. What keymap applies to the device in this
 * case is a server implementation detail.
 *
 * This does not increase the refcount of the keymap. Use ei_keymap_ref() to
 * keep a reference beyond the immediate scope.
 *
 */
/* FIXME: the current API makes it impossible to know when the keymap has
 * been consumed so the file stays open forever.
 */
struct ei_keymap *
ei_device_keyboard_get_keymap(struct ei_device *device);

/**
 * Return the struct @ref ei_device this keymap is associated with.
 */
struct ei_device *
ei_keymap_get_context(struct ei_keymap *keymap);

/**
 * Return the struct @ref ei context this device is associated with.
 */
struct ei *
ei_device_get_context(struct ei_device *device);

/**
 * Notify the EIS implementation that the given device is about to start
 * sending events. This should be seen more as a transactional boundary than a
 * time-based boundary. The primary use-cases for this are to allow for setup on
 * the EIS implementation side and/or UI updates to indicate that a device is
 * sending events now and for out-of-band information to sync with a given event
 * sequence.
 *
 * There is no actual requirement that events start immediately once emulation
 * starts and there is no requirement that a client calls
 * ei_device_stop_emulating() after the most recent events.
 *
 * For example, in a synergy-like use-case the client would call
 * ei_device_start_emulating() once the pointer moves into the the screen and
 * ei_device_stop_emulating() once the pointer moves out of the screen.
 *
 * Sending events before ei_device_start_emulating() or after
 * ei_device_stop_emulating() is a client bug.
 *
 * The sequence number identifies this transaction between start/stop emulating.
 * It must go up by at least 1 on each call to
 * ei_device_start_emulating(). Wraparound must be handled by the EIS
 * implementation but Callers must ensure that detection of wraparound is
 * reasonably.
 */
void
ei_device_start_emulating(struct ei_device *device, uint32_t sequence);

/**
 * Notify the EIS implementation that the given device is no longer sending
 * events. See ei_device_start_emulating() for details.
 */
void
ei_device_stop_emulating(struct ei_device *device);

/**
 * Generate a frame event to group the current set of events
 * into a logical hardware event. This function **must** be called after any
 * other event has been generated.
 *
 * The given timestamp applies to all events in the current frame.
 * The timestamp must be in microseconds of CLOCK_MONOTONIC, use the return
 * value of ei_now() to get a compatible timestamp.
 *
 * @note libei does not prevent a caller from passing in a future time but it
 * is strongly recommended that this is avoided by the caller.
 */
void
ei_device_frame(struct ei_device *device, uint64_t time);

/**
 * Generate a relative motion event on a device with
 * the @ref EI_DEVICE_CAP_POINTER capability.
 *
 * @param device The EI device
 * @param x The x movement in logical pixels
 * @param y The y movement in logical pixels
 */
void
ei_device_pointer_motion(struct ei_device *device, double x, double y);

/**
 * Generate an absolute motion event on a device with
 * the @ref EI_DEVICE_CAP_POINTER_ABSOLUTE capability.
 *
 * The x/y coordinate must be within the device's regions or the event is
 * silently discarded.
 *
 * @param device The EI device
 * @param x The x position in logical pixels
 * @param y The y position in logical pixels
 */
void
ei_device_pointer_motion_absolute(struct ei_device *device,
				  double x, double y);

/**
 * Generate a button event on a device with
 * the @ref EI_DEVICE_CAP_POINTER_ABSOLUTE or
 * @ref EI_DEVICE_CAP_POINTER capability.
 *
 * Button codes must match the defines in ``linux/input-event-codes.h``
 *
 * @param device The EI device
 * @param button The button code
 * @param is_press true for button press, false for button release
 */
void
ei_device_pointer_button(struct ei_device *device,
			 uint32_t button, bool is_press);

/**
 * Generate a smooth (pixel-precise) scroll event on a device with
 * the @ref EI_DEVICE_CAP_POINTER_ABSOLUTE or
 * @ref EI_DEVICE_CAP_POINTER capability.
 *
 * @note The server is responsible for emulating discrete scrolling based
 * on the pixel value, do not call ei_device_pointer_scroll_discrete() for
 * the same input event.
 *
 * @param device The EI device
 * @param x The x scroll distance in logical pixels
 * @param y The y scroll distance in logical pixels
 *
 * @see ei_device_pointer_scroll_discrete
 */
void
ei_device_pointer_scroll(struct ei_device *device, double x, double y);

/**
 * Generate a discrete scroll event on a device with
 * the @ref EI_DEVICE_CAP_POINTER_ABSOLUTE or
 * @ref EI_DEVICE_CAP_POINTER capability.
 *
 * A discrete scroll event is based logical scroll units (equivalent to one
 * mouse wheel click). The value for one scroll unit is 120, a fraction or
 * multiple thereof represents a fraction or multiple of a wheel click.
 *
 * @note The server is responsible for emulating pixel-based scrolling based
 * on the discrete value, do not call ei_device_pointer_scroll() for the
 * same input event.
 *
 * @param device The EI device
 * @param x The x scroll distance in fractions or multiples of 120
 * @param y The y scroll distance in fractions or multiples of 120
 *
 * @see ei_device_pointer_scroll
 */
void
ei_device_pointer_scroll_discrete(struct ei_device *device, int32_t x, int32_t y);

/**
 * Generate a scroll stop event on a device with the @ref
 * EI_DEVICE_CAP_POINTER_ABSOLUTE or @ref EI_DEVICE_CAP_POINTER capability.
 *
 * A scroll stop event notifies the server that the interaction causing a
 * scroll motion previously triggered with ei_device_pointer_scroll() or
 * ei_device_pointer_scroll_discrete() has stopped. For example, if all
 * fingers are lifted off a touchpad, two-finger scrolling has logically
 * stopped.
 *
 * The server may use this information to e.g. start kinetic scrolling
 * previously based on the previous finger speed.
 *
 * Use ei_device_pointer_scroll_cancel() to signal that the scroll motion has
 * completely stopped.
 *
 * Calling ei_device_pointer_scroll_stop() after
 * ei_device_pointer_scroll_cancel() without any of ei_device_pointer_scroll()
 * or ei_device_pointer_scroll_discrete() in between indicates a client logic bug.
 *
 * libei keeps track of the scroll axis and filters duplicate calls to
 * ei_device_pointer_scroll_stop() for the same axis. A nonzero scroll or
 * scroll-discrete value is required for the given axis to re-start scrolling
 * for that axis.
 */
void
ei_device_pointer_scroll_stop(struct ei_device *device, bool stop_x, bool stop_y);

/**
 * Generate a scroll cancel event on a device with the @ref
 * EI_DEVICE_CAP_POINTER_ABSOLUTE or @ref EI_DEVICE_CAP_POINTER capability.
 *
 * A scroll cancel event notifies the server that a scroll motion previously
 * triggered with ei_device_pointer_scroll() or
 * ei_device_pointer_scroll_discrete() has ceased and no further events should
 * be sent.
 *
 * This event indicates that the interaction has stopped to the point where
 * further (server-emulated) scroll events from this device are wrong.
 *
 * Use ei_device_pointer_scroll_stop() to signal that the interaction has
 * stopped but a server may emulate further scroll events.
 *
 * Calling ei_device_pointer_scroll_cancel() after
 * ei_device_pointer_scroll_stop() without any of ei_device_pointer_scroll()
 * or ei_device_pointer_scroll_discrete() in between iis permitted.
 *
 * libei keeps track of the scroll axis and filters duplicate calls to
 * ei_device_pointer_scroll_cancel() for the same axis. A nonzero scroll or
 * scroll-discrete value is required for the given axis to re-start scrolling
 * for that axis.
 */
void
ei_device_pointer_scroll_cancel(struct ei_device *device, bool cancel_x, bool cancel_y);

/**
 * Generate a key event on a device with
 * the @ref EI_DEVICE_CAP_KEYBOARD capability.
 *
 * Keys use the evdev scan codes as defined in
 * ``linux/input-event-codes.h``.
 *
 * Note that this is a keymap-independent key code, equivalent to the scancode
 * a physical keyboard would produce. To generate a specific key symbol, a
 * client must look at the keymap returned by ei_device_get_keymap() and
 * generate the appropriate keycodes.
 *
 * @param device The EI device
 * @param keycode The key code
 * @param is_press true for key down, false for key up
 */
void
ei_device_keyboard_key(struct ei_device *device, uint32_t keycode, bool is_press);

/**
 * Initiate a new touch on a device with the @ref EI_DEVICE_CAP_TOUCH
 * capability. This touch does not immediately send events, use
 * ei_touch_down(), ei_touch_motion(), and ei_touch_up().
 *
 * The returned touch has a refcount of at least 1, use ei_touch_unref() to
 * release resources associated with this touch
 */
struct ei_touch *
ei_device_touch_new(struct ei_device *device);

/**
 * This function can only be called once on an ei_touch object. Further
 * calls to ei_touch_down() on the same object are silently ignored.
 *
 * The x/y coordinate must be within the device's regions or the event is
 * silently discarded.
 *
 * @param touch A newly created touch
 * @param x The x position in logical pixels
 * @param y The y position in logical pixels
 */
void
ei_touch_down(struct ei_touch *touch, double x, double y);

/**
 * Move this touch to the new coordinates.
 */
void
ei_touch_motion(struct ei_touch *touch, double x, double y);

/**
 * Release this touch. After this call, the touch event becomes inert and
 * no longer responds to either ei_touch_down(), ei_touch_motion() or
 * ei_touch_up() and the caller should call ei_touch_unref().
 */
void
ei_touch_up(struct ei_touch *touch);

/**
 * Increase the refcount of this struct by one. Use ei_touch_unref() to
 * decrease the refcount.
 *
 * @return the argument passed into the function
 */
struct ei_touch *
ei_touch_ref(struct ei_touch *touch);

/**
 * Decrease the refcount of this struct by one. When the refcount reaches
 * zero, the context disconnects from the server and all allocated resources
 * are released.
 *
 * @return always NULL
 */
struct ei_touch *
ei_touch_unref(struct ei_touch *touch);

/**
 * Return the custom data pointer for this context. libei will not look at or
 * modify the pointer. Use ei_touch_set_user_data() to change the user data.
 */
void
ei_touch_set_user_data(struct ei_touch *touch, void *user_data);

/**
 * Set a custom data pointer for this context. libei will not look at or
 * modify the pointer. Use ei_touch_get_user_data() to retrieve a previously
 * set user data.
 */
void *
ei_touch_get_user_data(struct ei_touch *touch);

/**
 * @return the device this touch originates on
 */
struct ei_device *
ei_touch_get_device(struct ei_touch *touch);

/**
 * Return the seat from this event.
 *
 * For events of type @ref EI_EVENT_CONNECT and @ref EI_EVENT_DISCONNECT,
 * this function returns NULL.
 *
 * This does not increase the refcount of the seat. Use eis_seat_ref()
 * to keep a reference beyond the immediate scope.
 */
struct ei_seat *
ei_event_get_seat(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_DEVICE_START_EMULATING, return the
 * sequence number set by the EIS implementation.
 *
 * See eis_device_start_emulating() for details.
 */
uint32_t
ei_event_emulating_get_sequence(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_POINTER_MOTION return the relative x
 * movement in logical pixels or mm, depending on the device type.
 */
double
ei_event_pointer_get_dx(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_POINTER_MOTION return the relative y
 * movement in logical pixels or mm, depending on the device type.
 */
double
ei_event_pointer_get_dy(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_POINTER_MOTION_ABSOLUTE return the x
 * position in logical pixels or mm, depending on the device type.
 */
double
ei_event_pointer_get_absolute_x(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_POINTER_MOTION_ABSOLUTE return the y
 * position in logical pixels or mm, depending on the device type.
 */
double
ei_event_pointer_get_absolute_y(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_POINTER_BUTTON return the button
 * code as defined in linux/input-event-codes.h
 */
uint32_t
ei_event_pointer_get_button(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_POINTER_BUTTON return true if the
 * event is a button press, false for a release.
 */
bool
ei_event_pointer_get_button_is_press(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_POINTER_SCROLL return the x scroll
 * distance in logical pixels or mm, depending on the device type.
 */
double
ei_event_pointer_get_scroll_x(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_POINTER_SCROLL return the y scroll
 * distance in logical pixels or mm, depending on the device type.
 */
double
ei_event_pointer_get_scroll_y(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_POINTER_SCROLL_CANCEL return whether the
 * x axis has cancelled scrolling.
 */
bool
ei_event_pointer_get_scroll_stop_x(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_POINTER_SCROLL_STOP return whether the
 * y axis has stopped scrolling.
 */
bool
ei_event_pointer_get_scroll_stop_y(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_POINTER_SCROLL_DISCRETE return the x
 * scroll distance in fractions or multiples of 120.
 */
int32_t
ei_event_pointer_get_scroll_discrete_x(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_POINTER_SCROLL_DISCRETE return the y
 * scroll distance in fractions or multiples of 120.
 */
int32_t
ei_event_pointer_get_scroll_discrete_y(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_KEYBOARD_KEY return the key code (as
 * defined in include/linux/input-event-codes.h).
 */
uint32_t
ei_event_keyboard_get_key(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_KEYBOARD_KEY return true if the
 * event is a key down, false for a release.
 */
bool
ei_event_keyboard_get_key_is_press(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_TOUCH_DOWN, @ref
 * EI_EVENT_TOUCH_MOTION, or @ref EI_EVENT_TOUCH_UP, return the tracking
 * ID of the touch.
 *
 * The tracking ID is a unique identifier for a touch and is valid from
 * touch down through to touch up but may be re-used in the future.
 * The tracking ID is randomly assigned to a touch, a client
 * must not expect any specific value.
 */
uint32_t
ei_event_touch_get_id(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_TOUCH_DOWN, or @ref
 * EI_EVENT_TOUCH_MOTION, return the x coordinate of the touch
 * in logical pixels or mm, depending on the device type.
 */
double
ei_event_touch_get_x(struct ei_event *event);

/**
 * For an event of type @ref EI_EVENT_TOUCH_DOWN, or @ref
 * EI_EVENT_TOUCH_MOTION, return the y coordinate of the touch
 * in logical pixels or mm, depending on the device type.
 */
double
ei_event_touch_get_y(struct ei_event *event);

/**
 * @returns a timestamp in microseconds for the current time to pass into
 * ei_device_frame().
 *
 * In the current implementation, the returned timestamp is CLOCK_MONOTONIC
 * for compatibility with evdev and libinput.
 */
uint64_t
ei_now(struct ei *ei);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
