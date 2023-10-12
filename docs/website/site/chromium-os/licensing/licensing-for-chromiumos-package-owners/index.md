---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/licensing
  - Licensing in Chromium OS
page_name: licensing-for-chromiumos-package-owners
title: Licensing for Chromium OS Package Owners
---

[TOC]

Chrome OS Build team oriented info with more details can be found here:
[Licensing for Chromium OS
Developers](/chromium-os/licensing/licensing-for-chromiumos-developers)

## I am creating a new package, what license do I set in the Ebuild?

Google-authored open source packages typically use BSD-Google as the default
license. It is "BSD-Google" and not "BSD" because BSD licenses require the
licensing code to find a license file in the code with copyright attribution.
For Google packages, we simply use BSD-Google to skip the step that looks for
the copyright to assign (the BSD-Google contains both the BSD text and the
copyright assignment to Google). You can find this information in the LICENSE
file in the respective source repository.

Obviously you need to clear new proprietary packages with the vendor they came
from and legal before you can include them in Chrome OS. At that time, the
proper license to use should be sorted out. The main requirement is to allow for
free redistribution by anyone (Google, various partners, end users, etc...).

While the license of package is being reviewed by the lawyers, you may set
license in ebuild to "TAINTED". This special placeholder value will allow you to
keep working on the package, and will prevent the image from being signed with
MP keys.

For firmware packages (where the file is installed into /lib/firmware/ and
loaded into devices at runtime), you should push for a simple license that would
allow for inclusion into
[linux-firmware](https://chromium.googlesource.com/chromiumos/third_party/linux-firmware/+/HEAD/).
If you view the various license files in there, you should be able to get a gist
of what will work. Main goal is to allow for free redistribution.

If you are creating a package that does not exist in Gentoo, you are responsible
for scanning the source code of that package and setting the LICENSE field in
the ebuild to the correct value. Note that some open source software may require
that you list more than one license, or may be dual or tripled licensed. Feel
free to ask [chromeos-build-discuss@](http://g/chromeos-build-discuss) for
guidance if you have questions.

## You need to check in a new license

### shared licenses

Shared licenses are in one of these trees:

*   `src/third_party/chromiumos-overlay/licenses/` (custom licenses from
            us)
*   `src/third_party/portage-stable/licenses/` (stock licenses synced
            from Gentoo upstream)

### single use licenses

As explained in more details in "I got an error about BSD/MIT copyright
attribution" below, single use licenses should either be in a license file
checked in the source code tree for source we maintain ourselves, or can be
applied in src/third_party/chromiumos-overlay/licenses/copyright-attribution/

Please make sure you use the correct one.

## I got an error about BSD/MIT copyright attribution

### Fixing copyright attribution for pristine upstream source

Code under portage-stable is synced unmodified and therefore it's not desirable
to add a license file there because the next sync could remove it.

In this case you don't want to modify the source tree, you can put a license
mapping in src/third_party/chromiumos-overlay/licenses/copyright-attribution
(look for README and other examples there).

### Fixing copyright attribution for source we forked and modified

Is the source under a directory that has overlay in its name?

If this case, simply add a file called LICENSE with copyright attribution in the
package's source tree.

### Discussion on why and how

The licensing script will look for a license in many different places.

Usually, the license specified in the ebuild is good enough. There are however,
exceptions. Currently, they are:

```none
# Any license listed list here found in the ebuild will make the code look for
# license files inside the package source code in order to get copyright
# attribution from them.
COPYRIGHT_ATTRIBUTION_LICENSES = [
    'BSD',    # requires distribution of copyright notice
    'BSD-2',  # so does BSD-2 http://opensource.org/licenses/BSD-2-Clause
    'BSD-3',  # and BSD-3? http://opensource.org/licenses/BSD-3-Clause
    'BSD-4',  # and 4?
    'BSD-with-attribution',
    'MIT',
    'MIT-with-advertising',
    'Old-MIT',
]
```

If the code sees any of these licenses, and the LICENSE field in the ebuild
doesn't say something like LICENSE="|| ( BSD GPL-2 )" (if it does say that, it
will ignore the BSD license and use the GPL-2 one that does not require
copyright attribution), the licensing code must now look for a license that
contains a copyright for that package.

Running the licensing code with --debug shows how this is done. The logs below
show the details but basically it will first look in
`src/third_party/chromiumos-overlay/licenses/copyright-attribution`to see if it
can find a license file based on the package name. Use this for packages that we
mirror directly from gentoo and for which we don't want to modify the source
tree.

For packages that we've forked, you can instead simply add a file called LICENSE
in the package's source tree and the licensing script will pick it up.

```none
12:11:43: INFO: RunCommand: equery-x86-alex which dev-lang/python-exec-0.3.1
12:11:43: DEBUG: equery-x86-alex which dev-lang/python-exec-0.3.1 -> /mnt/host/source/src/third_party/portage-stable/dev-lang/python-exec/python-exec-0.3.1.ebuild
12:11:43: INFO: RunCommand: portageq-x86-alex metadata /build/x86-alex ebuild dev-lang/python-exec-0.3.1 HOMEPAGE LICENSE DESCRIPTION
12:11:44: INFO: Read licenses from ebuild for dev-lang/python-exec-0.3.1: BSD
12:11:44: INFO: dev-lang/python-exec-0.3.1: can't use BSD, will scan source code for copyright
12:11:44: DEBUG: Looking for override copyright attribution license in /mnt/host/source/src/third_party/chromiumos-overlay/licenses/copyright-attribution/dev-lang/python-exec-0.3.1
12:11:44: DEBUG: Looking for override copyright attribution license in /mnt/host/source/src/third_party/chromiumos-overlay/licenses/copyright-attribution/dev-lang/python-exec-0.3.1
12:11:44: DEBUG: Looking for override copyright attribution license in /mnt/host/source/src/third_party/chromiumos-overlay/licenses/copyright-attribution/dev-lang/python-exec
12:11:44: INFO: RunCommand: ebuild-x86-alex /mnt/host/source/src/third_party/portage-stable/dev-lang/python-exec/python-exec-0.3.1.ebuild clean fetch
12:11:44: INFO: RunCommand: ebuild-x86-alex /mnt/host/source/src/third_party/portage-stable/dev-lang/python-exec/python-exec-0.3.1.ebuild unpack
12:11:46: INFO: >>> Unpacking python-exec-0.3.1.tar.bz2 to /build/x86-alex/tmp/portage/dev-lang/python-exec-0.3.1/work
12:11:46: INFO: >>> Source unpacked in /build/x86-alex/tmp/portage/dev-lang/python-exec-0.3.1/work
12:11:46: INFO: RunCommand: portageq-x86-alex envvar PORTAGE_TMPDIR
12:11:46: INFO: RunCommand: find /build/x86-alex/tmp/portage/dev-lang/python-exec-0.3.1/work -type f
12:11:47: ERROR: 
dev-lang/python-exec-0.3.1: unable to find usable license.
Typically this will happen because the ebuild says it's MIT or BSD, but there
was no license file that this script could find to include along with a
copyright attribution (required for BSD/MIT).
If this is Google source, please change
LICENSE="BSD"
to
LICENSE="BSD-Google"
If not, go investigate the unpacked source in /build/x86-alex/tmp/portage/dev-lang/python-exec-0.3.1/work,
and find which license to assign.  Once you found it, you should copy that
license to a file under /mnt/host/source/src/third_party/chromiumos-overlay/licenses/copyright-attribution
(or you can modify LICENSE_FILENAMES to pickup a license file that isn't
being scraped currently).
```

To fix this error, I looked at the ebuild, found that the source is in
<https://bitbucket.org/mgorny/python-exec/src/> and found a copyright notice in
python-exec.c.

I then created a BSD file from another one:

```none
third_party/chromiumos-overlay/licenses/copyright-attribution$ cp ./dev-util/bsdiff dev-lang/python-exec
```

I then edited that new file and put the proper name for the copyright.

## How will licensing work?

The license file is currently generated after the fact with
<https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/licensing/licenses.py>

Once all packages are fixed to pass licensing (we have some left to fix, and
it's time consuming when we don't have easy to Email owners for each package,
hence request #4 above), licensing will be run for each package after it is
built and error out if the license isn't valid or can't be retrieved.

The end state is explained here:
<https://code.google.com/p/chromium/issues/detail?id=207004>
<https://code.google.com/p/chromium/issues/detail?id=271832>
<https://code.google.com/p/chromium/issues/detail?id=271851>
