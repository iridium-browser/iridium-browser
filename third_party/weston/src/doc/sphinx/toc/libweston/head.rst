.. _libweston-head:

Heads
=====

A head is represented by a :type:`weston_head` object.

A head refers to a monitor when driving hardware, but it can also be a window
in another window system, or a virtual concept. Essentially a head is a place
where you could present an image. The image will be produced by a weston_output
where the head is attached to.

In display hardware, a head represents a display connector in a computer
system, not the actual monitor connected to the connector. A head carries
monitor information, if present, like make and model, EDID and possible video
modes. Other properties are DPMS mode and backlight control.

In terms of Wayland protocol, a head corresponds to a wl_output. If one
:type:`weston_output` has several heads, meaning that the heads are cloned,
each head is represented as a separate wl_output global in wl_registry. Only
the heads of an enabled output are exposed as wl_outputs.

Heads can appear and disappear dynamically, mainly because of DisplayPort
Multi-Stream Transport where connecting a new monitor may expose new
connectors. Window and virtual outputs are often dynamic as well.

Heads are always owned by libweston which dictates their lifetimes. Some
backends may offer specific API to create and destroy heads, but hardware
backends like DRM-backend create and destroy heads on their own.

.. note::

   :func:`weston_head_init` and :func:`weston_head_release` belong to the
   private/internal backend API and should be moved accordingly once that
   section has been created. There are many other functions as well that are
   intended only for backends.

A :type:`weston_head` must be attached/detached from a :type:`weston_output`.
To that purpose you can use :func:`weston_output_attach_head`, respectively
:func:`weston_head_detach`.


.. doxygengroup:: head
   :content-only:
