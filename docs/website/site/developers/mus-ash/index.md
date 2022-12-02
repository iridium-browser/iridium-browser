---
breadcrumbs:
- - /developers
  - For Developers
page_name: mus-ash
title: mus+ash
---

mus+ash (pronounced "mustash") is a project to separate the window management
and shell functionality of ash from the chrome browser process. The benefit is
performance isolation for system components outside of the browser, and a
sharper line between what components are considered part of the ChromeOS UX and
what are part of the browser. mus+ash is built on top of Mandoline UI Service
("Mus") and the Ash system UI and window manager.

**Contact**

[mustash-dev@chromium.org](mailto:mustash-dev@chromium.org)

**Schedule/Milestone**

<table>
<tr>

<td>Milestone</td>

<td>Description</td>

<td>Crbug Label</td>

<td>Target Date</td>

</tr>
<tr>

<td>Window Service 2 </td>

<td>The window service runs as part of the ash process, instead of in a separate process. This allows us to write small apps the use the window service mojo APIs, as preparation for making the very large chrome browser use those APIs..</td>

<td><a href="https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Proj%3DMash-WS2&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids">Proj-Mash-WS2</a></td>

<td>Q2/3 2018</td>

</tr>
<tr>

<td>Keyboard Shortcut Viewer </td>

<td>Migrating KSV into a separate app using the window service mojo APIs from above.</td>

<td><a href="https://bugs.chromium.org/p/chromium/issues/list?can=2&q=+Proj%3DMash-KSV&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids">Proj-Mash-KSV</a></td>

<td>Q3 2018 (M69 Beta, M70 Stable)</td>

</tr>
<tr>

<td>Single Process Mash </td>

<td>Ash header refactoring to support making single process Mash work. Allows chrome browser to use the window service APIs, but still has ash and browser in a single process. This lets us exercise very complex window APIs without having to finish all the ash / browser decoupling.</td>

<td><a href="https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Proj%3DMash-SingleProcess+&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids">Proj-Mash-SingleProcess</a></td>

<td>Q4 2018</td>

</tr>
<tr>

<td>Multi Process Mash</td>

<td>Ash header refactoring to support making ash and the browser run in separate processes. This is the end goal of the mustash project.</td>

<td><a href="https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Proj%3DMash-MultiProcess&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids">Proj-Mash-MultiProcess</a></td>

<td>Q2/Q3 2019</td>

</tr>
</table>

**Bug Tracking**

*   [Bug Tracking
            Process](https://docs.google.com/document/d/1wiTlRVvSsINYa61An1XywU9TuewWCLupyDV_u9Z4jkw/edit#heading=h.iupkzhopjke8)

    <table>
    <tr>

    <td>Label Name</td>

    <td>Use</td>

    </tr>
    <tr>

    <td><a href="https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Proj%3DMash&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids">Proj-Mash</a></td>

    <td>Label for tracking all Mustash related issues (bugs, features)</td>

    </tr>
    <tr>

    <td><a href="https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Proj%3DMash-WS2&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids">Proj-Mash-WS2</a></td>

    <td>Tracking Window Service 2 work</td>

    </tr>
    <tr>

    <td><a href="https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Proj%3DMash-KSV&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids">Proj-Mash-KSV</a></td>

    <td>Tracking KSV migration</td>

    </tr>
    <tr>

    <td><a href="https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Proj%3DMash-SingleProcess&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids">Proj-Mash-SingleProcess</a></td>

    <td>Tracking ash refactoring support mash in single process</td>

    </tr>
    <tr>

    <td><a href="https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Proj%3DMash-MultiProcess&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids">Proj-Mash-MultiProcess</a></td>

    <td>Tracking ash refactoring support mash in multi process</td>

    </tr>
    <tr>

    <td><a href="https://bugs.chromium.org/p/chromium/issues/list?can=2&q=Proj%3DMash-Cleanup&colspec=ID+Pri+M+Stars+ReleaseBlock+Component+Status+Owner+Summary+OS+Modified&x=m&y=releaseblock&cells=ids">Proj-Mash-Cleanup</a></td>

    <td>Label for tracking clean up tasks.</td>

    </tr>
    </table>

**Links**

*   [Intent to
            Implement](https://groups.google.com/a/chromium.org/d/msg/chromium-dev/stof4wmbEDg/bhvWa-PrFQAJ)
            on chromium-dev with high level tactical details
*   [Bugs](https://code.google.com/p/chromium/issues/list?can=2&q=mustash)
*   Notes in
            [//ash/README.md](https://chromium.googlesource.com/chromium/src/+/HEAD/ash/README.md)
*   Googlers: See go/mustash and linked documents

**Build Instructions**

mus+ash builds only for Chrome OS (either for device or on Linux with
target_os="chromeos"). It requires aura and toolkit_views. There are no special
build flags other than target_os.

*Build chrome:*

autoninja -C out/foo chrome

*Run*

out/foo/chrome --enable-features=Mash

By default, each service will run in its own process, including chrome and all
of chrome's renderers.
