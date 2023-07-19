Logging/Debugging
=================

Weston's printf
---------------

Logging in weston takes place through :func:`weston_log()` function, which
calls a log handler (:var:`log_handler`) that has to be installed before
actually calling :func:`weston_log()`.  In weston, the log handler makes use of
the logging framework which is (mostly) comprised of :ref:`log scopes` (produces
of data) and :ref:`subscribers`.

Logging context
---------------

Management of the logging framework in weston happens under the
:type:`weston_log_context` object and is entirely separated from the main
compositor instance (:type:`weston_compositor`). The compositor
instance can be brought up much more later, but in the same time logging can
take place much earlier without the need of a compositor instance.

Instantiation of the :type:`weston_log_context` object takes place using
:func:`weston_log_ctx_create()` and clean-up/destroy with
:func:`weston_log_ctx_destroy()` or :func:`weston_log_ctx_compositor_destroy()`.

Log scopes
----------

A scope represents a source for a data stream (i.e., a producer). You'll
require one as a way to generate data. Creating a log scope is done using
:func:`weston_log_ctx_add_log_scope()` or
:func:`weston_compositor_add_log_scope()`. You can customize the scope
behaviour and you'll require at least a name and a description for the scope.

.. note::

   A scope **name** identifies that scope. Scope retrieval from the
   :type:`weston_log_context` is done using the scope name. The name is
   important for the subscription part, detailed bit later.

Log scopes are managed **explicitly**, and destroying the scope is done using
:func:`weston_log_scope_destroy`.

Available scopes in weston
~~~~~~~~~~~~~~~~~~~~~~~~~~

Weston has a few scopes worth mentioning:

- **log** - a debug scope for generic logging, upon which :func:`weston_log`
  re-routes its data.
- **proto** - debug scope that displays the protocol communication. It is
  similar to WAYLAND_DEBUG=server environmental variable but has the ability to
  distinguish multiple clients.
- **scene-graph** - an one-shot debug scope which describes the current scene
  graph comprising of layers (containers of views), views (which represent a
  window), their surfaces, sub-surfaces, buffer type and format, both in
  :samp:`DRM_FOURCC` type and human-friendly form.
- **drm-backend** - Weston uses DRM (Direct Rendering Manager) as one of its
  backends and this debug scope display information related to that: details
  the transitions of a view as it takes before being assigned to a hardware
  plane or to a renderer, current assignments of views, the compositing mode
  Weston is using for rendering the scene-graph, describes the current hardware
  plane properties like CRTC_ID, FB_ID, FORMAT when doing a commit or a
  page-flip. It incorporates the scene-graph scope as well.
- **xwm-wm-x11** - a scope for the X11 window manager in Weston for supporting
  Xwayland, printing some X11 protocol actions.
- **content-protection-debug** - scope for debugging HDCP issues.
- **timeline** - see more at :ref:`timeline points`

.. note::

   Besides 'log' scope, which is a generic scope, intended for usage through
   :func:`weston_log`, all the others scopes listed above could suffer various
   modifications and might not represent a current list on which one should
   rely upon.


Subscribers
-----------

Besides creating a creating a scope, a subscriber (:type:`weston_log_subscriber`)
object needs to be created. The subscriber object is an opaque
object (private) and allows customization of the back-end side of libweston.
The subscriber object can define its own methods. Users wanting to define
a new data stream should extend this :type:`weston_log_subscriber`.

For example libweston make uses of several type of subscribers, specific to the
data streams they will be generating:

- a **'logger'** type created by :func:`weston_log_subscriber_create_log()`
- a **'flight-recoder'** type created by :func:`weston_log_subscriber_destroy_flight_rec()`
- for the **'weston-debug'** protocol, which is private/hidden created whenever a
  client connects

Like log scopes, the subscribers are also manged **explicitly** and both of the
subscriber types above have their destroy counter-parts. `weston-debug`
protocol is a bit special in this regard as the destroying part is handled
implicitly using wayland protocol specifics.

Once the subscriber has been created there needs to be a subscription process
in which we establish a relationship between the subscriber and the scope.

To create a subscription we use :func:`weston_log_subscribe` which uses the
subscriber created previously and the scope name. If the scope was not created
at the time, the subscription will be (at least for a time) a *pending
subscription*. Once the scope is created the *pending subscription* is
destroyed, not before creating a new subscription to accommodate the
initial/original one.

.. note::

   The subscription process is (an) internal API and is managed implictly.

When a scope is being destroyed the subscriptions for this scope will be
destroyed as well.

Logger
~~~~~~

weston uses a logger type of a subscriber for logging everyhing in the code
(through the help of :func:`weston_log()`).  The subscriber method
(:func:`weston_log_subscriber_create_log()`) takes an :samp:`FILE *` as an
argument in case the std :samp:`stdout` file-descriptor is not where the data
should be sent to.

Additionally, specifying which scopes to subscribe to can be done using
:samp:`--logger-scopes` command line option. As log scopes are already created
in the code, this merely subscribes to them. Default, the 'log' scope is being
subscribr to the logger subscriber.

Flight recorder
~~~~~~~~~~~~~~~

The flight recorder acts like a black box found in airplanes: it accumulates
data until the user wants to display its contents. The backed up storage is a
simple ring-buffer of a compiled-time fixed size value, and the memory is
forcibly-mapped such that we make sure the kernel allocated storage for it.

The user can use the debug keybinding :samp:`KEY_D` (shift+mod+space-d) to
force the contents to be printed on :samp:`stdout` file-descriptor.
The user has first to specify which log scope to subscribe to.

Specifying which scopes to subscribe for the flight-recorder can be done using
:samp:`--flight-rec-scopes`. By default, the 'log' scope and 'drm-backend' are
the scopes subscribed to.

weston-debug protocol
~~~~~~~~~~~~~~~~~~~~~

Weston-debug protocol is only present in the weston compositor (i.e., a weston
specific compositor). It make uses of the the logging framework presented
above, with the exception that the subscription happens automatically rather
than manually with :func:`weston_log_subscribe()` in case of the other two
types of subscribers.  Also the subscriber is created once the client has
connected and requested data from a log scope.  This means that each time a
client connects a new subscriber will be created.  For each stream subscribed a
subscription will be created.  Enabling the debug-protocol happens using the
:samp:`--debug` command line.

Timeline points
---------------

A special log scope is the 'timeline' scope which, together with
`wesgr <https://github.com/ppaalanen/wesgr>`_ tool, helps diagnose latency issues.
Timeline points write to this 'timeline' scope in different parts of the
compositor, including the GL renderer.

As with all other scopes this scope is available over the debug protocol, or by
using the others :ref:`subscribers`. By far the easiest way to get data out
of this scope would be to use the debug protocol.
Then use `wesgr <https://github.com/ppaalanen/wesgr>`_ to process the data which
will transform it into a SVG that can be rendered by any web browser.

The following illustrates how to use it:

.. code-block:: console

   ./weston-debug timeline > log.json
   ./wesgr -i log.json -o log.svg

Inserting timeline points
~~~~~~~~~~~~~~~~~~~~~~~~~

Timline points can be inserted using :c:macro:`TL_POINT` macro. The macro will
take the :type:`weston_compositor` instance, followed by the name of the
timeline point. What follows next is a variable number of arguments, which
**must** end with the macro :c:macro:`TLP_END`.

Debug protocol API
------------------

.. doxygengroup:: debug-protocol
   :content-only:

Weston Log API
--------------

.. doxygengroup:: wlog
   :content-only:

Logging API
-----------

.. doxygengroup:: log
   :content-only:

Internal logging API
--------------------

.. note::

   The following is mean to be internal API and aren't exposed in libweston!

.. doxygengroup:: internal-log
   :content-only:
