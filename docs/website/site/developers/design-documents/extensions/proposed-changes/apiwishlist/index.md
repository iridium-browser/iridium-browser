---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/proposed-changes
  - Proposed & Proposing New Changes
page_name: apiwishlist
title: API Wish List
---

An early design document containing an unsorted list of things we've want to do
as extension APIs. Some of these ideas have now been implemented.

<table>
  <tr>
    <th>Name</th>
    <th>Notes</th>
    <th>Use cases</th>
    <th>Priority</th>
  </tr>
  <tr>
    <td>Launch external programs</td>
    <td>Just a simple command line, for more interaction with native code use NPAPI</td>
    <td>Download accelerators</td>
    <td></td>
  </tr>
  <tr>
    <td>History</td>
    <td>APIs to the built-in history and bookmark system. For example, to be notified when the star button is pressed, or to push items into either database.</td>
    <td>Synchronization</td>
    <td></td>
  </tr>
  <tr>
    <td>Internationalization</td>
    <td>This is particularly necessary in cases where UI is declarative. For example, if we have declarative content gleam or tool buttons. It is also useful for internationalizing HTML without having to use JS at runtime.</td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>Read-write user scripts</td>
    <td>Inject javascript into pages.</td>
    <td>Like Greasemonkey</td>
    <td></td>
  </tr>
  <tr>
    <td>Thumbnail</td>
    <td>Generate thumbnails from open tabs.  Access read and write to the thumbnail database.</td>
    <td>CtrlTab Preview</td>
    <td></td>
  </tr>
  <tr>
    <td>Sidebars</td>
    <td></td>
    <td>Delicious</td>
    <td></td>
  </tr>
  <tr>
    <td>Content layers</td>
    <td>A separate DOM for use by extensions that renders on top of a content DOM. Extensions (user scripts) can access both, but the top layer's CSS is isolated from the bottom layer.</td>
    <td>Drawing selection UI, like FlashGot</td>
    <td></td>
  </tr>
  <tr>
    <td>Keyboard and mouse handlers</td>
    <td>Extensions should be able to listen to events over the entire browser UI.</td>
    <td>CtrlTab Preview, Gestures</td>
    <td></td>
  </tr>
  <tr>
    <td>Async DOM</td>
    <td>An idea to allow manipulation of the DOM asynchronously from the browser process.</td>
    <td>This would allow Greasemonkey-type use cases without having to require authors split apart their code into separate processes. It should also be more efficient.</td>
    <td></td>
  </tr>
  <tr>
    <td>Settings</td>
    <td>Interact with various Chromium settings, like what pages JavaScript is enabled for, what pages cookies are sent to, user agent settings, proxy settings, etc</td>
    <td>NoScript, privacy filters, proxy switchers</td>
    <td></td>
  </tr>
  <tr>
    <td>Network</td>
    <td>Listen for requests, watch data go by, etc.</td>
    <td>Cloud-based history</td>
    <td></td>
  </tr>
  <tr>
    <td>Omnibox</td>
    <td>Take part in omnibox sessions.</td>
    <td>Make cloud-based bookmark and history first-class in Chromium UI</td>
    <td></td>
  </tr>
  <tr>
    <td>Read-only user scripts</td>
    <td>Read-only javascript access to the DOM.</td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>Pop-out areas</td>
    <td>Ability to render web content outside the viewport</td>
    <td>Pull out areas from toolbars, shelves</td>
    <td></td>
  </tr>
  <tr>
    <td>Shelves and bottom bars</td>
    <td>They may be able to be the same thing. Shelves have a different default presentation than bottom bars (constrained height, like a toolbar, different background and edges).</td>
    <td>Download status, Firebug</td>
    <td></td>
  </tr>
  <tr>
    <td>Element-specific actions</td>
    <td>Tell Chromium to somehow highlight a DOM element and advertise that an action is possible.</td>
    <td>Send this image to a friend on Facebook</td>
    <td></td>
  </tr>
  <tr>
    <td>Menus</td>
    <td>add items to in-page context menus and system menus</td>
    <td>bookmarking and recommendation systems, many more</td>
    <td></td>
  </tr>
  <tr>
    <td>Downloads</td>
    <td>APIs to manipulate the download system.</td>
    <td>DownThemAll</td>
    <td></td>
  </tr>
  <tr>
    <td>New Tab Page</td>
    <td>Replace it entirely, or add modules (iframes?) to it.</td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>Themes</td>
    <td>Customizing the look and feel of the interface. We might surface them to users as different than extensions, but they will use a lot of the underlying infrastructure.</td>
    <td>It turns out people like to decorate.</td>
    <td></td>
  </tr>
  <tr>
    <td>Storage</td>
    <td>Should come for free via HTML5 APIs.</td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>Navigation Interception</td>
    <td>APIs for an extension to intercept a navigation event and modify it.</td>
    <td>Automated file format converters, auto-incognito mode on certain website URLs</td>
    <td></td>
  </tr>
</table>
