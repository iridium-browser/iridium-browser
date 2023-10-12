.. _libweston-output:

Outputs
=======

A :type:`weston_output` determines what part of the global compositor
coordinate space will be composited into an image and when. That image is
presented on the attached heads (weston_head).

An output object is responsible for the framebuffer management, damage tracking,
display timings, and the repaint state machine. Video mode, output scale and
output transform are properties of an output.

In display hardware, a weston_output represents a CRTC, but only if it has been
successfully enabled. The CRTC may be switched to another during an output's
lifetime.

The lifetime of a weston_output is controlled by the libweston user.

With at least a :type:`weston_head` attached, you can construct a
:type:`weston_output` object which can be used by the compositor, by enabling
the output using :func:`weston_output_enable`. The output **must not** be
already enabled.

The reverse operation, :func:`weston_output_disable`, should be used when there's
a need to reconfigure the output or it will be destroyed.


.. doxygengroup:: output
   :content-only:
