Libweston
=========

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   libweston/compositor.rst
   libweston/output-management.rst
   libweston/log.rst

`Libweston` is an effort to separate the re-usable parts of Weston into a
library. `Libweston` provides most of the boring and tedious bits of correctly
implementing core Wayland protocols and interfacing with input and output
systems, so that people who just want to write a new "Wayland window manager"
(WM) or a small desktop environment (DE) can focus on the WM part.

Libweston was first introduced in Weston 1.12, and is expected to continue
evolving through many Weston releases before it achieves a stable API and
feature completeness.

`Libweston`'s primary purpose is exporting an API for creating Wayland
compositors. Libweston's secondary purpose is to export the weston_config API
so that third party plugins and helper programs can read :file:`weston.ini` if
they want to. However, these two scopes are orthogonal and independent. At no
point will the compositor functionality use or depend on the weston_config
functionality.

Further work
------------

In current form, `libweston` is an amalgam of various APIs mashed together and
currently it needs a large clean-up and re-organization and possibly, a split
into class-specific files. The documentation only provide the public
API and not the private API used inside `libweston` or other functionality
required in the core internals of the library.

With that in mind we see the following steps needed to achieve that:

- migrate everything that should not reside in the public API (for instance,
  the doxygen **\\internal** command is a clear indication that that symbol
  should not be present in the public API) to private headers.
- if needed be, create class-specific files, like **input** and **output**
  which should tackle specific functionality, and allows to write the
  documentation parts much easier, and provides clarity for `libweston`
  users when they'd read it.
