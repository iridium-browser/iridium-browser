Weston kiosk-shell
==================

Weston's kiosk-shell is a simple shell targeted at single-app/kiosk use cases.
It makes all top-level application windows fullscreen, and supports defining
which applications to place on particular outputs. This is achieved with the
``app-ids=`` field in the corresponding output section in weston.ini. For
example:

.. code-block:: ini

    [output]
    name=screen0
    app-ids=org.domain.app1,com.domain.app2

To run weston with kiosk-shell set ``shell=kiosk-shell.so`` in weston.ini, or
use the ``--shell=kiosk-shell.so`` command-line option.
