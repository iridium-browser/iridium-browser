# Releasing

To make a release of Weston, follow these steps.

0. Verify the test suites and codebase checks pass.  All of the tests should
   either pass or skip.

       ninja -C build/ test

1. Verify that the wayland and wayland-protocols version dependencies are
   correct, and that wayland-protocols has had a release with any needed
   protocol updates.

2. Update the first stanza of `meson.build` to the intended version.

   If the ABI has been broken, make sure `libweston_major` has been bumped since
   the last release.

   Then commit your changes:

       RELEASE_NUMBER="x.y.z"
       RELEASE_NAME="[alpha|beta|RC1|RC2|official|point]"
       git status
       git commit meson.build -m "build: bump to version $RELEASE_NUMBER for the $RELEASE_NAME release"
       git push

3. Run the `release.sh` script to generate the tarballs, sign and upload them,
   and generate a release announcement template.  This script can be obtained
   from X.org's modular package:

   https://gitlab.freedesktop.org/xorg/util/modular/blob/master/release.sh

   The script supports a `--dry-run` option to test it without actually doing a
   release.  If the script fails on the distcheck step due to a test suite error
   that can't be fixed for some reason, you can skip testsuite by specifying
   the `--dist` argument.  Pass `--help` to see other supported options.

       release.sh .

5. Compose the release announcements.  The script will generate *.x.y.z.announce
   files with a list of changes and tags.  Prepend these with a human-readable
   listing of the most notable changes.  For x.y.0 releases, indicate the
   schedule for the x.y+1.0 release.

6. PGP sign the release announcement and send it to
   <wayland-devel@lists.freedesktop.org>.

7. Update `releases.html` in wayland.freedesktop.org with links to tarballs and
   the release email URL. Copy tarballs produced by `release.sh` to `releases/`.

   Once satisfied:

       git commit -am "releases: add ${RELEASE_NUMBER} release"
       git push

For x.y.0 releases, also create the release series x.y branch.  The x.y branch
is for bug fixes and conservative changes to the x.y.0 release, and is where we
create x.y.z releases from.  Creating the x.y branch opens up master for new
development and lets new development move on.  We've done this both after the
x.y.0 release (to focus development on bug fixing for the x.y.1 release for a
little longer) or before the x.y.0 release (like we did with the 1.5.0 release,
to unblock master development early).

    git branch x.y [sha]
    git push origin x.y

The master branch's `meson.build` version should always be (at least) x.y.90,
with x.y being the most recent stable branch.  The stable branch's `meson.build`
version is just whatever was most recently released from that branch.

For stable branches, we commit fixes to master first, then `git cherry-pick -x`
them back to the stable branch.
