Running Weston
==============

libweston uses the concept of a *back-end* to abstract the interface to the
underlying environment where it runs on. Ultimately, the back-end is
responsible for handling the input and generate an output. Weston, as a
libweston user, can be run on different back-ends, including nested, by using
the wayland backend, but also on X11 or on a stand-alone back-end like
DRM/KMS.

In most cases, people should allow Weston to choose the backend automatically
as it will produce the best results. That happens for instance when running
Weston on a machine that already has another graphical environment running,
being either another wayland compositor (e.g.  Weston) or on a X11 server.
You should only specify the backend manually if you know that what Weston picks
is not the best, or the one you intended to use is different than the one
loaded.  In that case, the backend can be selected by using ``-B [backend]``
command line option.  As each back-end uses a different way to get input and
produce output, it means that the most suitable back-end depends on the
environment being used.

Available back-ends:

* **drm** -- run stand-alone on DRM/KMS and evdev (recommend)
  (`DRM kernel doc <https://www.kernel.org/doc/html/latest/gpu/index.html>`_)
* **wayland** -- run as a Wayland application, nested in another Wayland compositor
  instance
* **x11** -- run as a x11 application, nested in a X11 display server instance
* **rdp** -- run as an RDP server without local input or output
* **headless** -- run without input or output, useful for test suite
* **pipewire** -- run without input, output into a PipeWire node

The job of gathering all the surfaces (windows) being displayed on an output and
stitching them together is performed by a *renderer*. By doing so, it is
compositing all surfaces into a single image, which is being handed out to a
back-end, and finally, displayed on the screen.

libweston provides two useful renderers. One uses
`OpenGL ES <https://www.khronos.org/opengles/>`_, which will often be accelerated
by your GPU when suitable drivers are installed. The other uses the
`Pixman <http://www.pixman.org>`_ library which is entirely CPU (software)
rendered. You can select between these with the ``--renderer=gl`` and
``--renderer=pixman`` arguments when starting Weston.

Additional set-up steps
-----------------------

Depending on your distribution some additional set-up parts might be required,
before actually launching Weston, although any fairly modern distribution
should have it already set-up for you. Weston creates its unix socket file (for
example, wayland-1) in the directory specified by the required
environment variable ``$XDG_RUNTIME_DIR``. Clients use the same variable to
find that socket. Normally this should already be provided by systemd.  If you
are using a distribution that does not set-up ``$XDG_RUNTIME_DIR``, you
must set it using your shell profile capability. More info about how to
set-up that up, which depends to some extent on your shell, can be found at
`Building/Running Weston <https://wayland.freedesktop.org/building.html>`_

Running Weston in a graphical environment
-----------------------------------------

As stated previously, if you are already in a graphical environment, Weston
would infer and attempt to load up the correct back-end.  Either running
in a Wayland compositor instance, or a X11 server, you should be able to run
Weston from a X terminal or a Wayland one.

Running Weston on a stand-alone back-end
----------------------------------------

Now that we are aware of the concept of a back-end and a renderer, it is time to
introduce the concept of a seat, as stand-alone back-ends require one.  A *seat*
is a collection of input devices like a keyboard and a mouse, and output
devices (monitors), forming the work or entertainment place for one person. It
could also include sound cards or cameras.  A single computer could be serving
multiple seats.

.. note::

        A graphics card is **required** to be a part of the seat, as among
        other things, it effectively drives the monitor.

By default Weston will use the default seat named ``seat0``, but there's an
option to specify which seat Weston must use by passing ``--seat`` argument.

You can start Weston from a VT assuming that there's a seat manager supported by
`libseat <https://sr.ht/~kennylevinsen/seatd>`_ running, such as ``seatd`` or
`logind <https://www.freedesktop.org/wiki/Software/systemd/logind/>`_.  The
backend to be used by ``libseat`` can optionally be selected with
``$LIBSEAT_BACKEND``.  If ``libseat`` and ``seatd`` are both installed, but
``seatd`` is not already running, it can be started with ``sudo -- seatd -g
video``.

Launching Weston via ssh or a serial terminal is best with the ``libseat``
launcher and ``seatd``. Logind will refuse to give access to local seats from
remote connections directly. The process for
setting that up is identical to the one described above, where one just need to
ensure that ``seatd`` is running with the appropriate arguments, after which one
can just run ``weston``. ``seatd`` will lend out the current VT, and if you want
to run on a different VT you need to ``chvt`` first. Make sure nothing will try
to take over the seat or VT via logind at the same time in case logind is
running.

If you want to rely on logind, you can start weston as a systemd user service:
:ref:`weston-user-service`.

Running Weston on a different seat on a stand-alone back-end
------------------------------------------------------------

While Weston can be tested on top of an already running Wayland compositor or
an X11 server, another option might be to have an unused GPU card which can
be solely used by Weston.  So, instead of having a dedicated machine to run
Weston for trying out the DRM-backend, by just having an extra GPU, one can
create a new seat that could access the unused GPU on the same machine (and
potentialy other inputs) and assign it to that seat. All of the
happening while you already have your graphical environment running.

In order to have that set-up, the requirements/steps would be:

* have an extra GPU card -- you could also use integrated GPUs, while your
  other GPU is in use by another graphical environment
* create a udev file that assigns the card (and inputs) to another seat
* start Weston on that seat

Start by creating a udev file, under ``/etc/udev/rules.d/`` adding something
similar to the following:

::

        ACTION=="remove", GOTO="id_insecure_seat_end"

        SUBSYSTEM=="drm", KERNEL=="card*", KERNELS=="0000:00:02.0", ENV{ID_SEAT}="seat-insecure"

        SUBSYSTEM=="input", ATTRS{idVendor}=="222a", ATTRS{idProduct}=="004d", OWNER="your_user_id", ENV{ID_SEAT}="seat-insecure", ENV{WL_OUTPUT}="HDMI-A-1"
        SUBSYSTEM=="input", ATTRS{idVendor}=="03f0", ATTRS{idProduct}=="1198", OWNER="your_user_id", ENV{ID_SEAT}="seat-insecure"

        LABEL="id_insecure_seat_end"

By using the above udev file, devices assigned to that particular seat
will be skipped by your normal display environment. Follow the naming scheme
when creating the file (``man 7 udev``). For instance you could use
``63-insecure-seat.rules`` as a filename, but take note that other udev rules
might also be present and could potentially affect the way in which they get
applied. Check that no other rules might take precedence before adding
this new one.

.. warning::

        This seat uses on purpose the name ``seat-insecure``, to warn users
        that the input devices can be eavesdropped. Futher more, if you attempt
        doing this on a VT, without being already in a graphical environment
        (and although the udev rules do apply), there will be nothing stopping
        the events from input devices reaching the virtual terminal.

In the example above, there are two input devices, one of which is a
touch panel that is being assigned to a specific output (`HDMI-A-1`) and
another input which a mouse.  Notice how ``ENV{ID_SEAT}`` and
``ENV{WL_OUTPUT}`` specify the name of the seat, respectively the input that
should be assign to a specific output.

Resolving or extracting the udev key/value pair names, can be easily done with
the help of ``udevadm`` command, for instance issuing ``udevadm info -a
/dev/dri/cardX`` would give you the entire list of key values names for that
particular card.  Archaically, one would might also use ``lsusb`` and ``lspci``
commands to retrieve the PCI vendor and device codes associated with it.

If there are no input devices the DRM-backend can be started by appending
``--continue-without-input`` or by editing ``weston.ini`` and adding to the
``core`` section ``require-input=false``.

Then, weston can be run by selecting the DRM-backend and the seat ``seat-insecure``:

::

        SEATD_VTBOUND=0 ./weston -Bdrm --seat=seat-insecure

This assumes you are using the libseat launcher of Weston with the "builtin"
backend of libseat. Libseat automatically falls back to the builtin backend if
``seatd`` is not running and a ``logind`` service is not running or refuses.
You can also force it with ``LIBSEAT_BACKEND=builtin`` if needed.
``SEATD_VTBOUND=0`` tells libseat that there is no VT associated with the
chosen seat.

If everything went well you should see weston be up-and-running on an output
connected to that DRM device.

.. _weston-user-service:

Running weston from a systemd service
-------------------------------------

Weston could also be started, as a systemd user `service
<https://www.freedesktop.org/software/systemd/man/systemd.service.html>`_,
rather than as systemd system service, still relying on logind launcher.  In
order to do that we would need two
`unit <https://man7.org/linux/man-pages/man5/systemd.unit.5.html>`_ files,
a ``.service`` and a ``.socket`` one.

On a Debian system, the systemd user units are under ``/etc/systemd/user/``
directory.

* ``weston.socket``

::

        [Unit]
        Description=Weston, a Wayland compositor
        Documentation=man:weston(1) man:weston.ini(5)
        Documentation=https://wayland.freedesktop.org/

        [Socket]
        ListenStream=%t/wayland-0


* ``weston.service``

::

        [Unit]
        Description=Weston, a Wayland compositor, as a user service
        Documentation=man:weston(1) man:weston.ini(5)
        Documentation=https://wayland.freedesktop.org/

        # Activate using a systemd socket
        Requires=weston.socket
        After=weston.socket

        # Since we are part of the graphical session, make sure we are started before
        Before=graphical-session.target

        [Service]
        Type=notify
        TimeoutStartSec=60
        WatchdogSec=20
        # Defaults to journal
        #StandardOutput=journal
        StandardError=journal

        # add a ~/.config/weston.ini and weston will pick-it up
        ExecStart=/usr/bin/weston

        [Install]
        WantedBy=graphical-session.target

After creating those two files, make sure systemd is aware of the changes:

::

        systemctl --user daemon-reload

If nothing creates a login session on the machine, one would actually need to
log-in physically (over VT). Starting weston then would be as simple as
doing:

::

        systemctl --user start weston


Alternatively to logging in over a VT, one can create an equivalent systemd
system service. Replacing the need to log-in physically at a keyboard when one
might not exist is a real possibility, but this approach can also work while
being logged in over a ssh connection, and run weston as a regular user.


In order to do that, create a systemd system service (for Debian that is under
``/etc/systemd/system`` directory) called for instance
``mysession.service``, and add the following:

::

        [Unit]
        Description=My graphical session

        # Make sure we are started after logins are permitted.
        After=systemd-user-sessions.service

        # if you want you can make it part of the graphical session
        #Before=graphical.target

        # not necessary but just in case
        #ConditionPathExists=/dev/tty7

        [Service]
        Type=simple
        Environment=XDG_SESSION_TYPE=wayland
        ExecStart=/usr/bin/systemctl --wait --user start mysession.target

        # The user to run the session as. Pick one!
        User=user
        Group=user

        # Set up a full user session for the user, required by Weston.
        PAMName=login

        # A virtual terminal is needed.
        TTYPath=/dev/tty7
        TTYReset=yes
        TTYVHangup=yes
        TTYVTDisallocate=yes

        # Fail to start if not controlling the tty.
        StandardInput=tty-fail

        # Defaults to journal, in case it doesn't adjust it accordingly
        #StandardOutput=journal
        StandardError=journal

        # Log this user with utmp, letting it show up with commands 'w' and 'who'.
        UtmpIdentifier=tty7
        UtmpMode=user

        [Install]
        WantedBy=graphical.target


Make sure that you're using a valid ``user`` for both ``User`` and ``Group``
entries.  Create also system user ``.target``, named ``mysession.target`` that
contains:

::

        [Unit]
        Description=My session

        BindsTo=mysession.target
        Before=mysession.target

Perform both a system, but also a user ``daemon-reload``, to make sure all
changes have been applied. Afterwards, start ``mysession`` and then ``weston``
user service. Checking if that worked could be done by verifying with loginctl
that there's an active login with the default `seat0` assigned on that
particular tty.

So, as a user one can do the following:

::

        systemctl start mysession # systemd will ask for passowrd
        loginctl # verify if mysession was able to perform the session login
        systemctl --user start weston

Finally, if one would not want to create such a systemd service, one could also
use `systemd-run <https://www.freedesktop.org/software/systemd/man/systemd-run.html>`_
which would allow to create a temporary service unit and ultimately achieve
something similar to the systemd service above:

::

        systemd-run  --collect -E XDG_SESSION_TYPE=wayland --uid=1000 -p PAMName=login -p TTYPath=/dev/tty7 sleep 1d
        systemctl --user start weston
