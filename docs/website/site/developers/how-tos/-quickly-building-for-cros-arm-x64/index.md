---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: -quickly-building-for-cros-arm-x64
title: (Quickly!) Building for CrOS (ARM & x64)
---

Googlers note: you might want go/simplechrome instead of this page.

Assuming you have a local chromium checkout you've been building natively, and
you want to be able to hack in it and build it for a CrOS device, see
instructions below.

Following
<http://dev.chromium.org/chromium-os/how-tos-and-troubleshooting/building-chromium-browser>,
a one-time setup step is:
> $ cros_sdk --enter --chrome_root=.../path/to/chrome/dir \[
> --chroot=../../which_chroot \]

(where the *chrome/dir* part is the parent of *src/*).

Subsequent cros_sdk invocations will have the chrome source directory mounted at
*~/chrome_root/src* inside the chroot.

Make sure you've run *build_packages* and *build_image* at least one
successfully in the repo in question, or it's likely you'll be missing
packages/binaries/includes/libs needed to build chrome! See [CrOS dev
guide](/chromium-os/developer-guide) for details on those commands.

Inside the parent of the chrome src/ dir, I have two scripts: "ninja" and
"goma-ninja" (attached to this page):

*   ninja: wraps regular ninja assuming a 32-core machine (z620) and
            emitting build output depending on $GYP_GENERATOR_FLAGS so that
            different builds (e.g.: native, lumpy, and daisy) can coexist
*   (**google-internal only)** goma-ninja: wraps the above ninja wrapper
            to compile on [goma](http://go/ma) with a parallelism of -j5000.

Having these scripts in the parent of the src/ dir means they're available both
inside the chroot and outside it, so regardless of where I am, I can always say
"*../goma-ninja chrome*" for example and the right thing happens.

Finally, I have a file named bashrc-tail which I source inside the chroot with:

> *. ~/chrome_root/cros-chroot-homedir/bashrc-tail*

at the bottom of my chroot's *~/.bashrc*. This defines two shell functions
("lumpy" and "daisy") which set up environment variables facilitating building
for different CrOS boards, as well as shell functions that help run chrome and
other ninja outputs **on** the boards, without having to explicitly copy things
around, by using sshfs. After a build like *../goma-ninja chrome* from above is
done, I'll run *initcrosdevice* which will ssh into the device (**note:** make
sure to update the crosip() functions in your copy of bashrc-tail to point to
the right IP addresses of your boards, as well as the username/hostname/path in
the definition of *initcrosdevice*!) and echo a helpful message useful for
copy/paste to sshfs-mount the ninja output directory on the board. At that point
I can run a unittest like:

> DISPLAY=:0 ./Debug/video_decode_accelerator_unittest

or run a full chrome. Beware that running chrome on a board usually requires a
pile of switches (depending on what features are launched on CrOS at a given
time, and the state of the drivers on any given board at any given time), which
you are best off extracting from the "real" chrome running on the device. I
usually keep these switches in a script at *out_daisy/Debug/go*, so that it's
available via the sshfs mount above and I can just run it on the device as
*./Debug/go*.

**Good luck!**