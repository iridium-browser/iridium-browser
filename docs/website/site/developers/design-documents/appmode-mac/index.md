---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: appmode-mac
title: Mac App Mode (draft)
---

### Overview

Chromium provides a way to create a "custom" browser shortcut for any web site
(not yet implemented on Mac). Our ultimate goal for the Mac implementation is to
make such a shortcut appear to be first-class application. In particular, its UI
should consist of a browser displaying the specified web site, but with many
"normal" browser features removed (no bookmarks, navigation, a custom menu bar,
etc.). Usage examples include Gmail, Reader, Facebook, and so on.

### User experience requirements

The following are absolutely essential:

*   A shortcut should appear to be an application to the operating
            system (Spotlight, Finder, etc.). This also includes it working in
            the Dock like any other application: click to launch/raise,
            indicates when running, provides a window list, and present in
            Cmd-tab list for quick application switching.
*   The user should be able to treat a shortcut like any other
            Application, e.g., install it wherever he/she wishes.
*   A shortcut must have a customizable name and icon (and of course web
            site); the name must be displayed in the menu bar.
*   The window displayed by a shortcut must not display the "usual"
            gadgets, such as the toolbar, bookmark bar, etc.
*   A shortcut must be robust and maintenance-free with respect to
            browser updates, etc. (This presumably also means that shortcuts
            should be designed to be upgradable, at least in some way, if
            necessary.)

The following would be nice to have:

*   A shortcut should function properly with Exposé and Spaces (e.g., a
            user should be able to "pin" a shortcut to any workspace, in
            particular to a workspace different from the browser's workspace).
            \[This is not expected to be a problem; see below.\]
*   A shortcut should have fully customized menus, including in
            particular the main menu. This means, in particular, that many of
            the normal browser's menus would simply not be present. \[This is
            somewhat difficult, but do-able in the near future.\]
*   A shortcut should optionally be able to share cookies, local
            storage, cache, etc. with their regular Chromium browser.

### Technical decisions and implementation

*   We have decided to implement each shortcut as a "shim app", that is,
            a small application bundle which finds and dynamically loads the
            Chromium framework.
*   With this design, a shortcut truly is a first-class application,
            albeit one with a tiny binary, and minimal binary contact with
            Chromium.
*   Nearly all nontrivial functionality will be contained in the
            Chromium framework proper.
*   As such, we do not anticipate much of a need to update the shortcut
            application bundles; however, if necessary, this could be done by
            code in the Chromium framework.
*   The hard technical work for this has already been done.
*   The only nontrivial task for the shim app would be to locate
            Chromium. We plan to do this in several ways (in order of priority):
            (1) Whenever Chromium runs, it records its location in the defaults
            system; a shim app can look this up. (2) Look in various standard
            locations, such as /Applications, ~/Applications, etc. (3) Consult
            Launch Services or other system databases.
*   Note that if all the methods fail, the shim app can simply ask the
            user to run Chromium to re-record its location.
*   Q. When cookies, etc. aren't shared, where should the profile data
            be stored?

\[*work in progress, more to come* -Trung\]

### Implementation plan

*   Phase 1: Design/code the shim app and basic interface between the
            shim app and the framework. Put the shim app into the build system.
            Make the shim app display an appropriate (specifiable) name in the
            main menu. At completion, one should be able to manually create
            shortcuts which satisfy the essential requirements. \[started\]
*   Phase 2: Design/code the (normal) browser-side code to produce
            shortcuts (with appropriate URL, name, icon) to be saved in a
            user-specifiable location (or perhaps leverage drag-and-drop?). It
            should also have the appropriate behavior when a shortcut is produce
            from a "live" web page. At completion, shortcuts should be fully
            usable by the average user.
*   Phase 3: Fix up the main menu, etc., and generally tighten up any
            other behavior. At completion, shortcuts should be very polished and
            truly look like a first-class application.
*   Phase X: Make cookies, local storage, cache, etc. shareable, and
            make this an optional capability.

\[*sorry for the mess, work in progress* -Trung\]

Old stuff:

### This is just a draft proposal at this time, to figure out options, and see what might apply to other platforms.

### Background

Gears provides an way to create desktop shortcuts to web sites
([Desktop.createShortcut()](http://code.google.com/apis/gears/api_desktop.html)).
Chromium expands on this to try and make a "custom" browser for the site. Our
ultimate goal for the Mac implementation is to have the shortcut appear to the
OS as a first-class application. Its UI consists of a browser displaying the
website but with many of the browser features removed (ie-no bookmarks,
navigation, etc.). Examples would be "applications" such as Gmail, Reader, etc.

### Expectations

The desired user experience is:

*   The shortcuts should appear to be Applications to the OS (Spotlight,
            Finder, etc.).
*   The shortcut should work in the Dock like any other application:
            click to launch/raise, indicates when running, provides a window
            list, and present in cmd-tab list for quick process switching.
*   The menu bar and window appearance are different from "normal"
            Chromium
    *   App name is related to the website the shortcut is for (created
                by the user? or taken from the shortcut name?)
    *   No bookmarks or navigation menus/items.

The desired technical details are:

*   Share the HTML5 storage so any site/domain storage data is available
            in both the AppMode and w/in the main browser. ie, offline Gmail is
            in the same state regardless if the user is using a Gmail "app" or
            w/in Chromium.
*   Share the cache so common html/js/images don't have to be
            re-downloaded, cached by each AppMode the user has.
*   Make cookie sharing something that is optional or controlled on a
            per shortcut basis. This would allow some users to have the Gmail
            App share its auth cookie with Chromium, but others might chose not
            to so they could have a Gmail shortcut for one account, and use a
            different account w/in Chromium.

### Possible Approaches

#### Shim Apps to sub-load Chromium

Make each AppMode shortcut a little Cocoa app bundle. The binary itself would be
extremely small. When launched, it would find Chromium.app, load it as a bundle,
and invoke an exported system to relay the shortcut info. The vended API from
w/in Chromium.app would start up Cocoa, WebKit, etc. This would keep the "binary
contact" between the shortcuts and Chromium as small as possible so they don't
have to be updated w/ new versions of Chromium. The shortcut app would
effectively be another instance of Chromium in "browser" mode, maintaining an
independent set of renderers, plugin hosts, etc.

#### Launcher Apps

Make each AppMode shortcut a little app that simply launches the Chromium
application (if not already running), sends over the shortcut info, and exits.

#### Shortcut Document

Rather than creating a separate application, create a new document type that the
Finder recognizes as a Chromium document to store the site location. When the
user double-clicks the a saved shortcut document, the Finder (through
LaunchServices) automatically sends a message to Chromium, launching it first if
it's not already running. Chromium responds to the open event by creating a new
window with AppMode chrome and disabling the navigation menu items when that
window is in the foreground. This window will co-exist in the same browser
process as other non-AppMode windows, thus can share the profile and data
stores.

#### Make A Full-blown Copy

Rather than trying to load and sub-load parts from another application bundle,
have the shortcut be a full copy of the Chromium application.

### Limitations / Issues

#### None of these approaches is a obvious win, they all meet different parts of the expectations or have different things that would need to be resolved:

<table>
<tr>
<td colspan=4><b>UI</b></td>
</tr>
<tr>
<td>*Proposal*</td>
<td>*Apps to Spotlight*</td>
<td>*Work as app from Dock*</td>
<td>*AppMode menubar and window*</td>
</tr>
<tr>
<td>Shim Apps to sub-load Chromium</td>
<td>✓</td>
<td>✓</td>
<td>✓</td>
</tr>
<tr>
<td>Launcher Apps</td>
<td>✓</td>
</tr>
<tr>
<td>Shortcut Document</td>
</tr>
<tr>
<td>Make a Full-blown Copy</td>
<td>✓</td>
<td>✓</td>
<td>✓</td>
</tr>
</table>

<table>
<tr>
<td colspan=4><b>Technical</b></td>
</tr>
<tr>
<td>*Proposal*</td>
<td>*Share HTML5 Storage*</td>
<td>*Share Cache*</td>
<td>*Cookie Control*</td>
</tr>
<tr>
<td>Shim Apps to sub-load Chromium</td>
<td>TBD - Would require the storage layers support multiple processes actively modifying the on disk storage</td>
<td>TBD - Same requirement around multiple process support</td>
<td>TBD - Private cookie store would be easy, opting to share has the same multiple process issue</td>
</tr>
<tr>
<td>Launcher Apps</td>
<td>✓</td>
<td>✓</td>
<td>TBD - Might be able to use a different profile for the window to have different cookies</td>
</tr>
<tr>
<td>Shortcut Document</td>
<td>✓</td>
<td>✓</td>
<td>TBD - Might be able to use a different profile for the window to have different cookies</td>
</tr>
<tr>
<td>Make a Full-blown Copy</td>
<td>TBD - Would require the storage layers support multiple processes actively modifying the on disk storage</td>
<td>TBD - Same requirement around multiple process support</td>
<td>TBD - Private cookie store would be easy, opting to share has the same multiple process issue</td>
</tr>
</table>

### Special Notes

Shim Apps to sub-load Chromium does require a long term contract around the
interface used to find, load, and invoke the real browser. If we ever needed
changes to the Cocoa info.plist or different arguments, we'd have to find ways
to handle this without the AppMode stub being updated (as the running user might
not have privledges to rewrite it).
Make a Full-blown Copy has a major drawback; the inability to update the
shortcut applications. Keystone/Omaha requires a one-to-one mapping from GUID to
installed application, and self-generated GUIDs would receive no updates as they
don't match the GUID on the server. So once a copy was made for a short cut,
there wouldn't be a way to target it for updates (feature, security, etc).

### Additional Notes from Trung

**Hybrid solutions:** There are a number of possible hybrid solutions. For
example, one could make a persistent Launcher App which, instead of quitting
immediately after launching, persists and performs tasks such as "forwarding"
Dock activations to the main browser process. One should also be able to switch
out the main menu on the demand, and such an model would allow a better
imitation of a full-fledged application. (One could also imagine other hybrids,
e.g., a Launcher App which just opens an embedded Shortcut Document. This has
certain *flexibility* advantages; see below.)

**Launcher/Copy: Another possible hybrid is a Launcher App which simply
launches, for each specific App Mode shortcut, a copy of Chromium with special
settings/command-line arguments. This would avoid the major drawback of a true
copy, yet provide most of the benefits. (It would still have the drawback of
making sharing storage/cache/etc. difficult.) This is very similar to a Shim
App, but with no binary contact. It would be lightweight on disk space
requirements (though lack of cache sharing might make increase disk space
usage); memory usage would hopefully be similar to a Shim App, in that perhaps
text pages could be shared, and hopefully lighter than that of a Full-blown
Copy.**

**Flexibility and upgradeability:** Regardless of model, it is worth considering
how App Mode capabilities may be modified/upgraded (possibly changing models) in
the future. In general, Shim and Launcher Apps seem most flexible; upgrading a
Shortcut Document might be troublesome. A possible upgrade model is as follows:
Upon loading, the Shim/Launcher App locates the primary Chromium application
bundle and checks if this bundle contains a more up-to-date Shim/Launcher App.
If necessary, it can then update itself, and relaunch. (This assumes that the
App Mode data is well-segregated inside the Shim/Launcher App's bundle.)

**"Captivity":** A problem with a web app posing as a normal application is that
web apps are typically not *captive*, in the sense that there's usually no clear
delineation of where the application ends. For example, in a native mail client,
clicking on a link will take one outside the mail program. Likewise, it either
has calendar functionality built-in, or it doesn't. With a "Gmail App", things
are not so clear. What should happen if one clicks on "Calendar"? If one has a
separate "Calendar App" set up, perhaps one would like it to launch or switch to
that. Perhaps one would view it as integral to the "Gmail App". Or perhaps one
would prefer that it be opened in a regular browser window. (Worse, one might
prefer different things for different links, even within the same "site",
however sites are delineated. For example, one may want "Docs" to be a separate
app, but keep "Calendar" in the same app.) With web apps, of course, there are
also usually many external links. Probably one would usually want to open
external links in a regular browser window, but it is not clear how to
distinguish between the different cases (open in current app, open in different
app, open in a regular browser window).

Shortcut Documents, Launcher Apps, and variants skirt this problem by not truly
distinguishing between different App Mode windows and the regular browser. The
distinction made is very weak: an "application" is just a window in the regular
browser with a few special properties. It is then very easy to open normal
browser windows running with one's normal browsing profile. Depending on the
approach, there may be no real distinction between an App Mode window running,
e.g., Gmail from one running Calendar or Docs.

The Shim App, etc. approaches which provide more full, separate applications
suffer here. For example, if one is in Gmail and one clicks on a link to, say,
Flickr, not only does one probably want Flickr to open in a normal browser
window (unless one has a Flickr App set up!), but one probably also wants it to
open using the profile associated with one's normal browser -- one doesn't
typically want to maintain have separate Flickr logins between one's Gmail App
and one's normal browser. (On the other hand, one might want distinct Gmail
logins!)

**Other considerations:** In addition to making the Dock icon, menus, etc. work,
it would be good if an App Mode shortcut worked properly with Spaces. In
particular, one can assign a particular space to each application (e.g., one
space for the "Gmail App" windows and a different space for normal Chromium
browser windows). The general lack of API for Spaces means that only Shim Apps,
full-blown copies, and perhaps the Launcher/Copy hybrid could work properly with
Spaces.

### Questions from Trung

1.  The question of "captivity" is essentially the following: What
            should happen when one clicks on a link from within an App?
2.  Technical problem: When and how does OS X determine what application
            is being run? (To what extent can this be set at run time?)
3.  Technical: Is there any way of convincing Spaces that a given window
            really belongs to a different application?
