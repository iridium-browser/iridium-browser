---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: immersive-fullscreen
title: Immersive fullscreen
---

<div class="two-column-container">
<div class="column">

jamescook, pkotwicz

August 2013

User Experience

Immersive fullscreen is a fullscreen mode for the Chrome OS Ash window manager.
It allows users with small screens to use almost the entire screen for web
content, but still access the tab strip and app shelf when needed.

<img alt="image"
src="https://lh6.googleusercontent.com/4mTHvcSoE3eOhCTWLh3yziRiq67Bfgo0qKZHxC1UXpxRKxQmhT7sJuGhlNeNCnwZvV3ZDk0fI-JoMm4Slp35zJapaYx1vUMktvArbGa760VdXp51DdoFodat">

The tabs (and location bar) are collapsed into a small strip at the top of the
screen. The shelf is similarly collapsed at the bottom of the screen. Showing
the strip gives the user a visual target for the mouse.

<img alt="image"
src="https://lh3.googleusercontent.com/saLdLHVhQarR82y2Z9zIK9Qg12aYVX41LxYBUbOs5ro4jTgejhv60Q2Flkt0UNasfy42_CGaMOUcVl8Bc_DA-uMMb1MC3gjd-rZ5He0m4Q0OhdXd8NTShkce"
height=120px; width=500px;>

Mousing to the top of the screen slides in the top controls, which are fully
interactive.

For a detailed description of the user experience, see Ben’s [original
proposal](https://docs.google.com/a/google.com/document/d/1dequH51rwf8YpU6s5v6htmEozk8VBB5FbtM1jFDOwt4/edit).

Top of screen implementation

We introduce a TopContainerView that holds the tab strip and location bar.
BrowserView continues to hold the page contents and download shelf.

TopContainerView sometimes contains the bookmark bar. If the bookmark bar is
visible (“attached” under the location bar) it should slide down over the page.
However, on the new tab page the bookmark bar is visually part of the page
contents, so the top controls must slide down over it. Therefore the bookmark
bar is reparented between the BrowserView and the TopContainerView as needed.

Infobars (“Save this password?”) are considered part of the page contents. The
TopContainerView always slides over infobars.

These are the states for the top controls. “Closed” is the steady state.

<img alt="image"
src="https://docs.google.com/a/google.com/drawings/d/s2NLSYoYsxuc8Z5YxLbWGyg/image?w=512&h=258&rev=644&ac=1"
height=258px; width=512px;>

During the sliding and revealed states the TopContainerView paints to a
compositor layer to allow it to be stacked above the web contents layer.

Revealing the top controls

The top controls need to feel like they slide open when the user presses the top
of the screen, but not trigger accidentally when the user is mousing past web
content at the top of the page. A reveal is triggered when the mouse is at the
top of screen past a time delay and is moving slowly (or not at all) in a
horizontal direction.

In addition to revealing when the user mouses to the top of the screen there are
a variety of situations in the UI when the top controls need to be visible. For
example:

    When focus is in the location bar (so the user can see what he types)

    Making a bookmark (the dialog relies on the location bar to show the URL)

    Opening the tools menu (so the menu is visually attached to the 3-lines
    button)

In these situations the top controls need to stay visible even if the mouse
moves away from the screen edge. Getting these situations correct was one of the
hardest parts of this feature.

pkotwicz@ introduced a clever ImmersiveRevealedLock class to deal with the
visibility issue. Various parts of Chrome code can acquire a lock and hold it
until their UI is dismissed. The ImmersiveModeController handles sliding closed
the top controls when all locks are destroyed and the mouse is away from the
screen edge.

The top controls also stay visible when any child control has focus, which
handles the location bar and several other situations.

Painting the collapsed controls

The white strips in the collapsed controls (the “light bar”) help reinforce to
the user that their tabs are still present. It also provides a place to show
loading animations.

The light-bar uses the existing TabStrip for painting, but with a special
painting style flag set. This allows the existing tab layout code to be used for
the light bar, so the tab positions exactly match. We also recycle most of the
loading animation code.

The disadvantaged of this approach is that it requires resizing, relayout and
repainting of the tab strip whenever the reveal state changes. However, we
decided the benefits of reusing the layout and animation code outweighed this
cost.

Browser and tab fullscreen

Chrome supports two internal fullscreen concepts “browser fullscreen” and “tab
fullscreen”. Browser fullscreen is entered via UI commands (e.g. Tools menu &gt;
Fullscreen). Tab fullscreen is entered when a web page requests fullscreen (e.g.
Docs presentations).

Immersive fullscreen is only tied to browser fullscreen. This has two
advantages:

1. From the user’s perspective, immersive fullscreen is the only fullscreen
state in the Ash window manager. In particular we avoid the confusion Mac OS
creates with its two separate “fullscreen” and “presentation mode” commands.

2. Fullscreen videos, presentations and games don’t display the light-bars at
the top and bottom of the screen, and the user doesn’t need to do anything
special for this to happen.

See chrome/browser/ui/fullscreen/fullscreen_controller.h for details on the two
fullscreen modes.

Test coverage

Immersive fullscreen is covered by both unit tests and browser tests. The
browser tests are used primarily to verify integration with the rest of the
browser’s fullscreen mechanisms. See
c/b/ui/views/frame/immersive_mode_controller_ash_unittest.cc and
c/b/ui/views/frame/immersive_mode_controller_ash_browsertest.cc.

Appendix

During the development of immersive fullscreen the instant-extended project
(“1993”) experimented with using web content to display the auto-complete
suggetsions under the location bar. This presented challenges for layout in
immersive fullscreen, as the suggestions were sometimes in a separate web view
and sometimes (on the new tab page) part of the primary web content.

The suggestions list has been reverted back to a native UI widget. However, if
it becomes web content again this is how the layout code will need to change:

Tab contents position, without 1993:

<table>
<tr>

<td>Regular window</td>

<td>Immersive fullscreen</td>

</tr>
<tr>

<td>Bookmark bar hidden</td>

<td>bottom of omnibox (top_container)</td>

<td>3 pixels from top (omnibox hidden, bookmarks hidden)</td>

</tr>
<tr>

<td>Bookmark bar attached</td>

<td>bottom of bookmark bar (top_container)</td>

<td>3 pixels from top (omnibox hidden, bookmarks bar shown during reveals)</td>

</tr>
<tr>

<td>New tab page with bookmark bar detached</td>

<td>bottom of bookmark bar (not top_container)</td>

<td>below bookmarks</td>

</tr>
</table>

Tab contents position, with 1993:

<table>
<tr>

<td>Regular window</td>

<td>Immersive fullscreen</td>

</tr>
<tr>

<td>No suggestions</td>

<td>bottom of omnibox</td>

<td>3 pixels from top</td>

</tr>
<tr>

<td>Suggestions (in overlay)</td>

<td>bottom of omnibox</td>

<td>3 pixels from top, overlay contents below top_container</td>

</tr>
<tr>

<td>NTP suggestions (in active)</td>

<td>bottom of omnibox (unless suggestions won’t cover bookmarks, then at top)</td>

<td>bottom of omnibox (unless suggestions won’t cover bookmarks, then at top)</td>

</tr>
</table>

</div>
<div class="column">

</div>
</div>
