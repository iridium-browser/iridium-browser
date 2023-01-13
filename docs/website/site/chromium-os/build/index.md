---
breadcrumbs:
- - /chromium-os
  - Chromium OS
page_name: build
title: Chromium OS Build
---

<div class="two-column-container">
<div class="column">

Developer/User documentation pertaining to the Chrome OS build system.

Google-internal documentation can be found linked from the [internal build team
page](http://goto.google.com/cros-build).

#### For Developers

*   [Developer
            guide](http://www.chromium.org/chromium-os/developer-guide)
*   Imaging your device with [Cros
            Flash](https://chromium.googlesource.com/chromiumos/docs/+/master/cros_flash.md)
*   Install packages to your device with [Cros
            Deploy](https://chromium.googlesource.com/chromiumos/docs/+/master/cros_deploy.md)
*   [Bypassing tests on a per-project
            basis](/chromium-os/build/bypassing-tests-on-a-per-project-basis)
*   [Adding a Package to the SDK](/chromium-os/build/add-sdk-package)

#### For Sheriffs

*   <http://www.chromium.org/developers/tree-sheriffs/sheriff-details-chromium-os>

#### For Build Contributors and other resources

*   Help writing unittests using [python-mock](/chromium-os/python-mock)
*   [Python style
            guide](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/styleguide/python.md)
*   [Chromium OS Developer
            Guide](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/developer_guide.md)
*   [Developing Chromium on Chromium
            OS](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/simple_chrome_workflow.md)
            ("Simple Chrome")
*   Misc. [developer helper
            scripts](http://www.chromium.org/chromium-os/how-tos-and-troubleshooting/helper-scripts)

*   [Licensing for Chromium OS Package
            Owners](/chromium-os/licensing/licensing-for-chromiumos-package-owners)
*   [Licensing for Chromium OS
            Developers](/chromium-os/licensing/licensing-for-chromiumos-developers)

</div>
<div class="column">

#### Build System Documentation

*   [Portage Build and
            FAQ](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/portage/ebuild_faq.md)
*   [Portage Package Upgrade
            Initiative](http://www.chromium.org/chromium-os/obsolete/portage-package-status)
    *   [Portage Package Status
                Spreadsheet](https://docs.google.com/a/chromium.org/spreadsheet/ccc?key=0AsXDKtaHikmcdEp1dVN1SG1yRU1xZEw1Yjhka2dCSUE#gid=0)
*   Build hacking
    *   [Chroot versioning](/chromium-os/build/chroot_version_hooks)
                (chroot version hooks)
    *   [Clearing all
                binaries](https://sites.google.com/a/google.com/chromeos/for-team-members/build/clear_binaries)
                (e.g. for a toolchain revert)

*   [cbuildbot
            Overview](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/remote_trybots.md)
*   Buildbot [Configure/Set
            up](/developers/testing/chromium-build-infrastructure/getting-the-buildbot-source/configuring-your-buildbot)
            (Chrome Infra guide)
*   [Commit Queue overview](/system/errors/NodeNotFound)
*   [Local Trybot](http://www.chromium.org/chromium-os/build/local-trybot-documentation)
*   [Remote trybot](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/remote_trybots.md)

#### Build labels

*   [Build-Tools](https://code.google.com/p/chromium/issues/list?can=2&q=Build%3DTools+OS%3DChrome&colspec=ID+Pri+M+Iteration+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles)
*   [Build-Tools-Cbuildbot](https://code.google.com/p/chromium/issues/list?can=2&q=Build%3DTools-Cbuildbot&colspec=ID+Pri+M+Iteration+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles)
            (file [new issue](http://goto.google.com/cros-cbuildbot-ticket))
*   [Build-Tools-Paygen](https://code.google.com/p/chromium/issues/list?can=2&q=Build%3DTools-Paygen&colspec=ID+Pri+M+Iteration+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles)
            (file [new issue](http://goto.google.com/cros-paygen-ticket))
*   [Build-Tools-Portage](https://code.google.com/p/chromium/issues/list?can=2&q=Build%3DTools-Portage&colspec=ID+Pri+M+Iteration+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles)
*   [Build-Tools-Pushlive](https://code.google.com/p/chromium/issues/list?can=2&q=Build%3DTools-Pushlive&colspec=ID+Pri+M+Iteration+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles)
            (file [new issue](http://goto.google.com/cros-pushlive-ticket))
*   [Build-Tools-SimpleChrome](https://code.google.com/p/chromium/issues/list?can=2&q=Build%3DTools-SimpleChrome&colspec=ID+Pri+M+Iteration+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles)
*   [Build-Tools-Trybot](https://code.google.com/p/chromium/issues/list?can=2&q=Build%3DTools-Trybot&colspec=ID+Pri+M+Iteration+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles)
*   [Build-Tools
            label=stats](http://goto.google.com/cros-build-stats-tickets) (file
            [new issue](http://goto.google.com/cros-build-stats-ticket))
*   [Cr-OS-Packages](https://code.google.com/p/chromium/issues/list?can=2&q=Cr%3DOS-Packages&colspec=ID+Pri+M+Iteration+ReleaseBlock+Cr+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=tiles)

Or file a generic Build issue to be triaged by the build team at
[goto.google.com/cros-build-ticket](https://code.google.com/p/chromium/issues/entry?template=Build%20Infrastructure&labels=Build,OS-Chrome,Pri-2&summary=your%20words%20here).

#### Other Resources

*   Build [FAQ](/chromium-os/build/faq)

</div>
</div>
