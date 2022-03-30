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
- - /developers/design-documents/extensions/proposed-changes/apis-under-development
  - API Proposals (New APIs Start Here)
page_name: browser-keys
title: BrowserKeys API Proposal
---

**Proposal Date**

28 February 2012

**Who is the primary contact for this API?**

Gary Kacmarcik (garykac@chromium.org)

**Who will be responsible for this API? (Team please, not an individual)**

Chromoting team

**Overview**

The browser-keys extension would send the browser shortcut keys to the page for
handling. If the page chooses not to handle the key, then the browser would be
able to handle it.

The most problematic cases are ctrl-n, ctrl-w and ctrl-t, which are accelerators
in Chromium that are often used for other purposes.

Note that OS-level accelerators (like Alt-Tab on Windows) would not be send to
the page. The general idea is that any key event that the browser gets is passed
on to the page, but no special effort is made to grab literally all key events.

**Use cases**

Any page or plugin that requires all the keyboard input would now be able to get
it.

Scenarios:

Remote access or terminals require these keys so they can send them to the
remote host for processing. As a concrete example, programs like emacs are hard
to use without ctrl-w and ctrl-t.

Some games will also benefit from the extra keys. They often use shift and ctrl
in combination with other keys during gameplay.

Pages/apps that were developed initially on a different browser may assume the
availability of these accelerator keys. This extension would help the developer
to get it working on Chrome.

**Do you know anyone else, internal or external, that is also interested in this
API?**

Chromoting is the primary client, but HTerm has a similar need.

Also, the basic idea (get more key events sent to page) has been proposed
before:
<https://www.chromium.org/developers/design-documents/reserved-keys-api>

Other groups interested in this functionality have commented on
<http://crbug.com/84332>.

The W3C Games Community has on their wishlist a request for better keyboard
support. This issue is larger in scope than this extension can address, but it
would be a welcome first step.

See "Keyboard Lock" on
<http://www.w3.org/community/games/2011/11/10/w3c-games-community-group-new-game-summit-november-2011/>

**Could this API be part of the web platform?**

Other browser vendors don't suffer from this problem as much as Chromium because
they send browser accelerator keys to the page before handling them. For
usability/responsiveness reasons, Chromium decided not to send these key events
to plugins (see <http://crbug.com/84332> for discussion) so we are alone with
this particular keyboard problem.

Because this is basically a Chromium-only problem, there is unlikely to be
consensus for this as a web standard.

**Do you expect this API to be fairly stable? How might it be extended or
changed in the future?**

General API would be to simply enable/disable receiving the browser keys.

Earlier proposals have mentioned allowing the developer to specify certain keys
to allow/disallow, but that seems more complicated than needed.

**List every UI surface belonging to or potentially affected by your API:**

No UI elements are involved. Using this API would simply allow more keyboard
events to be passed to the page.

**How could this API be abused?**

Giving the page first crack at browser key events allows a malicious consumer to
ignore keys like ctrl-w to keep the user stuck on the page. Since we don't plan
on trapping OS-level key events, we don't believe this will be a serious
problem.

**Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you
could commit with your API (if you’ve got good ones, feel free to add more):**

**1)** Fullscreen, Pointer Lock + Ignore all keyboard input. With this
extension, Dr. Evil would be able to ignore more keys.

**2)**

**3)**

**Alright Doctor, one last challenge:**

**Could a consumer of your API cause any permanent change to the user’s system
using your API that would not be reversed when that consumer is removed from the
system?**

No.

**How would you implement your desired features if this API didn't exist?**

There is no real alternative. Since Chromium does not pass the key events to the
page, there is nothing that can be done. Chromium's sandbox prevents getting the
key events directly.

**Draft API spec**

API reference: chrome.browserkeys

Methods:

**enableBrowserKeys**

*chrome.experimental.browserkeys.enableBrowserKeys(boolean enable)*

Enables or disables browser accelerator keys being sent to the page.

Parameters:

> **enable ( boolean )**

> True to enable browserkeys, false to disable.

**Open questions**

Note any unanswered questions that require further discussion.
