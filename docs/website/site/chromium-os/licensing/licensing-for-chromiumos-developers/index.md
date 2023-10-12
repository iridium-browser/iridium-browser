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

During the build, as part of emerging the packages,
[licences.py](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/licensing/licenses.py)
will create a `license.yaml` file in `$SYSROOT/var/db/pkg/$CATEGORY/$PACKAGE`
that will contain the information, needed for final post-processing.

This is done by parsing the ebuild and if the license is set to something like
MIT or BSD, we’re forced to unpack the source and look for a license file for
copyright attribution, which is required by the BSD and MIT style licenses.

Later, during creation of the ChromeOS image, `licenses.py` will be invoked with
`--output` argument to use the generated `license.yaml` files to create the
final HTML page, which will be compressed and put under
/opt/google/chrome/resources/about_os_credits.html.gz on the generated image.

Read the comments at the top of licenses.py for more instructions.

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


You don't usually need to re-generate licensing for individual packages,
as it will be generated during emerge, but you can do so with this command:

```none
(cr) pkg_name=dev-util/meson-0.61.1
(cr) ~/chromiumos/chromite/licensing $ ./licenses --debug --board $BOARD -p $pkg_name --generate -o out-single.html
```

You can generate the HTML license file with:

```none
(cr) ~/chromiumos/chromite/licensing $ ./licenses --debug --board $BOARD --generate-licenses --output outall.html

```

## How the license generation can fail and exit

The code has multiple asserts. They should be self documenting for us, but they
are not meant to be understandable for the end user, so I’m not going to list
them here. Also, I can’t say how they would happen since the whole point is that
they catch cases that aren’t supposed to happen :)

I’ll list a few special ones.

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

*   licenses.py has plenty of TODOs and FIXME in the comments, please
            look at them :)
