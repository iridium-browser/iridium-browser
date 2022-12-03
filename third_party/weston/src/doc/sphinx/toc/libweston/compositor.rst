Compositor
==========

:type:`weston_compositor` represents the core object of the library, which
aggregates all the other objects and maintains their state. You can create it
using :func:`weston_compositor_create`, while for releasing all the resources
associated with it and then destroy it, you should use
:func:`weston_compositor_destroy`.

Compositor API
--------------

.. doxygengroup:: compositor
   :content-only:
