---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/licensing
  - Licensing in Chromium OS
page_name: licensing-for-chromiumos-developers
title: Licensing for Chromium OS Developers
---

User oriented documentation is here: [Licensing for Chromium OS Package
Owners](/chromium-os/licensing/licensing-for-chromiumos-package-owners)

## Goal

Chrome OS ships with an html file that should contain a list of all the licenses
used by packages shipped with Chrome OS. We also chose to include some build
packages in the license output and while most are not required to be listed, we
included them to include some required build packages that provide static
libraries (.a) linked in other projects and therefore part of Chrome OS, but not
part of the package list.

[licences.py](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/licensing/licenses.py)
is currently meant to run inside a cros_sdk chroot after you have built an
x86-alex board, it will ask portage for the list of packages installed by the
build, and then retrieve the list of packages and their licenses. This is done
by parsing the ebuild and if the license is set to something like MIT or BSD,
we’re forced to unpack the source and look for a license file for copyright
attribution, which is required by the BSD and MIT style licenses.

Note that we just happened to pick x86-alex and use its license on all boards as
a stopgap. This is not ideal, just what we’re doing for now.

Once all the licenses have been gathered, the script factors out the licenses
used by multiple packages by listing them at the end and creating pointers to
them, and then generates an HTML file that will be located under
/opt/google/chrome/resources/about_os_credits.html on the generated image.

## Howto

*   get an internal manifest/build of Chrome OS (not external)
*   TODO: add instructions of how to get a release branch if it’s been
            cut, or how to do licensing the day of or the day before the branch
            will be cut (will become obsolete once licensing is part of the
            image build)
*   build it:

    ```none
    $ board=x86-alex
    $ ./build_packages --board=$board --nowithautotest --nowithtest --nowithdev --nowithfactory
    ```

*   read the comments at the top of licenses.py for more instructions

## Pointers

*   `chromite/licensing/` is the code that currently does all the work.
*   Custom licenses (licenses referred by non Gentoo ebuilds) are placed
            in ~/trunk/src/third_party/chromiumos-overlay/licenses
*   The code will look in
            `~/trunk/src/third_party/chromiumos-overlay/licenses/copyright-attribution/<category>/<pkgname>[-ver][-rel]`
            for MIT and BSD packages for a license you may have placed there
            instead of trying to unpack the source and scanning for a license
            file. This is required for packages that have a BSD like license but
            then fail to include a license in their source code, making it
            impossible for the code to fulfill the copyright attribution
            requirement.

## HOWTO

This is how you re-generate licensing for a few packages after fixing their
license:

```none
(cr) ~/trunk/chromite/licensing $ for i in dev-util/libc-bench-0.0.1-r6 x11-libs/libdrm-2.4.52 x11-libs/libdrm-tests-2.4.52-r1 x11-libs/xcb-util-wm-0.3.8 x11-libs/xcb-util-renderutil-0.3.8 x11-libs/xcb-util-image-0.3.8 x11-libs/xcb-util-0.3.8 x11-libs/xcb-util-keysyms-0.3.8; do
  ./licenses --debug --board  $board -p $i --generate -o  out-single.html 2>&1 | tee out-single
done
```

This will regenerate the licensing bits just for those packages, and then you
can generate the license file with:

```none
(cr) ~/trunk/chromite/licensing $ ./licenses --debug --board  $board  --all-packages -o  outall.html 2>&1 | tee outall
```

But we only really want a subset of this (without --all-packages):

```none
(cr) ~/trunk/chromite/licensing $ ./licenses --debug --board  $board -o  out.html 2>&1 | tee out
```

After that, we compare with the last generated file in chrome:

```none
~/chromiumos/chromite/licensing$ cp /usr/local/google2/chromeos/about_os_credits.html .
(cr) ~/trunk/chromite/licensing $ ../bin/diff_license_html about_os_credits.html out.html | tee diff
<lots of output, review that>
```

## How the license generation can fail and exit

The code has multiple asserts. They should be self documenting for us, but they
are not meant to be understandable for the end user, so I’m not going to list
them here. Also, I can’t say how they would happen since the whole point is that
they catch cases that aren’t supposed to happen :)

I’ll list a few special ones.

### raise AssertionError("%s: Can't parse || in the middle of a license: %s"

This shows that the code isn’t smart enough to handle || in the middle of an
ebuild. Doing so would involve more code and a recursive parser or something
more complicated, and currently we haven’t needed it.

If you find this, see if the LICENSE field in the ebuild can be simplified to
remove the complicated syntax (basically analyse what it says, and simplify it
to pick one license that applies to us most instead of the list of licenses in
the || ( lic1 lic2 lic3 ) OR block.

TODO: the code should handle this syntax by importing the gentoo ebuild parser.

### raise AssertionError("license %s could not be found in %s"

This will happen if a license specified in LICENSE= doesn’t exist anywhere in
STOCK_LICENSE_DIRS and CUSTOM_LICENSE_DIRS.

Arguably this should likely be a better error message instead of an assert and
once the presubmit that stops a non existing license to be referred to exists,
this code should never have to trigger.

See <http://crbug.com/318364>

### Assert DO NOT USE OUTPUT!!!

This one runs at the end, but tells the user to scroll back up and find the
relevant message in the logs for that package.

It’s not super user friendly, maybe it should capture all those logs so that it
can print them again at the end?

Note that the script is recommended to be run as

%(prog)s \[--debug\] $board out.html 2&gt;&1 | tee output.sav

so that you can look in output.sav and scroll back up for the errors relevant to
each package.

### raise PackageLicenseError()

This one comes with a suitable logging.error:

logging.error("""
%s: unable to find usable license.
Typically this will happen because the ebuild says it's MIT or BSD, but there
was no license file that this script could find to include along with a
copyright attribution (required for BSD/MIT).
If this is Google source, please change
LICENSE="BSD"
to
LICENSE="BSD-Google"
If not, go investigate the unpacked source in %s,
and find which license to assign. Once you found it, you should copy that
license to a file under %s
(unless you can modify LICENSE_FILENAMES to pick up a license file that isn't
being scraped currently).""",

## TODO for the code

*   See the rollup tracking bug and dependents:
            <http://crbug.com/197970>
*   licenses.py has plenty of TODOs and FIXME in the comments, please
            look at them :)
*   The goal is to run licensing on every new package built and
            submitted. There are a few ways to do this:
    1.  Know which packages will be shipped, generate license bits for
                each package as they are being built (and ignore this for non
                shipping packages). Once the image is built, the license bits
                are concatenated. See <http://crbug.com/207004>
        *   knowing which packages will be shipped is non trivial.
                    Generating licensing for all packages is better, but many
                    dev and test packages will fail licensing and fixing them
                    all will/would take work.
    2.  Run licensing as is, and integrate that at the end of an image
                build. This would still block packages that break licensing as
                part of the pre-CQ
    3.  Require valid licenses for all compiled packages =&gt; this is
                what we picked. Progress report of making all packages licensing
                compliant is here:
                <https://docs.google.com/a/google.com/document/d/1GK9rwdZtc--o5P63TwneOikO7A5Lf2hrQfGSJwXrgrc/edit#heading=h.46xqvmtxyqd5>
*   We will need to have very good instructions of what to do when
            licensing breaks before anything can be part of a package build, or
            added the end of an image build and to pre-CQ. DONE: Document for
            end users is here:
            <https://docs.google.com/a/google.com/document/d/1-p5Sg1m66pLPFYye4d7jiPkcJQVbfcwlBiExqc6tTpc/edit#>
*   The end goal is to have the chrome browser read the license file
            from a location in the Chrome OS tree. <http://crbug.com/271832>
            says it’ll be /opt/google/chrome/resources/about_os_credits.html
*   Obviously running licensing x86-alex and shipping that output for
            all boards is incorrect, but that’s the best we can do before the
            above points are fixed. Once chrome knows to read the license file
            from the OS, it’ll allow us to generate one license file per board.

## TODO for packages

*   All Google authored packages should have their license in ebuild
            changed from BSD to BSD-Google. This will save the code the time to
            unpack them and scan for a license file, as well as stop excluding
            all packages in chromeos-base (see SKIPPED_CATEGORIES in the code)
*   All packages with a license of “Proprietary” or "Proprietary-Binary"
            or "Google-TOS" must be migrated to a proper license.
    *   For Google-authored code, BSD-Google is usually a good fit (and
                make sure the source tree has a LICENSE file).
    *   For partner/closed source code, BSD or MIT are popular choices,
                but has to be made in coordination with the partner
*   It's also about time that we define owners for each packages
            (preferably a group), and allow those owners to do the reviews and
            vet changes to the package they happen to know well. This is what we
            do for prodimage and it works well. Obviously most Linux
            distributions do the same too.