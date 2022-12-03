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
page_name: power-management-api
title: Power Management API
---

**Proposal Date**

2013-03-11
Who is the primary contact for this API?
derat@
Who will be responsible for this API?

The Chrome OS UI team.

Overview
This API is intended to provide a way for extensions and apps to temporarily
disable aspects of system power management on Chrome OS. It is a modified
version of [an earlier proposed API](/system/errors/NodeNotFound) (see also
[additional implementation
details](https://docs.google.com/a/google.com/document/d/1CrZJRH5Eoh8A_6o8PwERzVSDSwEr2DDOOn14gekVCb0/edit#)).
Use cases
When the user is inactive, Chrome OS dims the screen, turns it off, and
eventually suspends the system. It attempts to avoid some or all of these
actions when the user is watching video or listening to audio. Some apps may
wish to trigger similar behavior on their own; imagine e-book or presentation
apps that need to prevent the screen from being dimmed or turned off, or a
communication app that needs to prevent the system from suspending so that
incoming calls may be received.
Do you know anyone else, internal or external, that is also interested in this
API?
Yes, there is an internal need for this API (<http://crbug.com/178944>).
External users have requested the ability to e.g. disable power management
entirely on desktop machines; this API makes it possible to write a simple
extension that does so.
Could this API be part of the web platform?
Likely, although the level of control over system power management available to
Chrome may vary between different OSes.
Do you expect this API to be fairly stable? How might it be extended or changed
in the future?
Yes, it should be stable. I don't foresee any need to add additional levels of
power-management-disabling.

**If multiple extensions used this API at the same time, could they conflict with each others? If so, how do you propose to mitigate this problem?**
Requests will be tracked per-extension. Since requests are additive, conflicts
should not be possible.
List every UI surface belonging to or potentially affected by your API:
No UI changes are planned.
**Actions taken with extension APIs should be obviously attributable to an
extension. Will users be able to tell when this new API is being used? How?**

This isn't currently planned. It would be possible to add UI that notifies the
user when an extension creates a request, though.

How could this API be abused?
Extensions could use this API to disable power management features indefinitely.
Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you
could commit with your API (if you’ve got good ones, feel free to add more):
1) Disable power management features indefinitely while the extension is
running, increasing power consumption, decreasing battery life, and possibly
confusing the user.
2) N/A
3) N/A
What security UI or other mitigations do you propose to limit evilness made
possible by this new API?
Extensions will only be able to override power management while the system's lid
is open; closing the lid will still suspend the system.
**Alright Doctor, one last challenge:**
**Could a consumer of your API cause any permanent change to the user’s system using your API that would not be reversed when that consumer is removed from the system?**
**No. Any outstanding requests will be abandoned when an extension is stopped.
How would you implement your desired features if this API didn't exist?
An extension could perform actions that trigger Chrome OS's existing automated
power-management-disabling behavior. For example, playing a silent audio file
would prevent the system from suspending.
Draft API spec

*Manifest changes*

**power** permission

> A new permission that is required in order to call this API's methods.

***Methods***

**requestKeepAwake**

> Registers a request to disable power management. Each extension may hold at
> most one request; when multiple sequential calls are made, only the final one
> will be honored. When an extension is unloaded, its request will be
> automatically released.

> **Parameter:**

> *level (enum \["system", "display"\])*

> > Specifies the degree to which power management should be disabled.

> > "system": The system will not suspend due to user inactivity, but the screen
> > may still be dimmed or turned off.

> > "display": The screen will not be dimmed or turned off due to user
> > inactivity. Additionally, the system will not suspend due to user
> > inactivity. If the screen is currently dimmed or turned off due to
> > inactivity, it will be turned on immediately.

**releaseKeepAwake**

> Unregisters an outstanding request to disable power management. Does nothing
> if no request exists. Previously-deferred power-management-related actions may
> be performed immediately.

> **Parameters**

> *none*

Open questions

1.  ~~Can the API initially be Chrome-OS-only?~~ This probably won't be
            an issue; content::PowerSaveBlocker should provide cross-platform
            support.
2.  ~~Should a "power" permission be added to the manifest specification
            or should extensions be able to use this API unconditionally?~~ Yes;
            added.
3.  ~~Can the *callback* parameter be omitted from both methods?~~ Yes;
            removed.
4.  ~~The initial implementation of this API never made it out of
            experimental, although [an extension which uses
            it](https://chrome.google.com/webstore/detail/keep-awake-extension/bijihlabcfdnabacffofojgmehjdielb/reviews?hl=en)
            was published. Can the new version of the API use the same method
            names?~~ Yes. I'm updating the existing extension to use the
            chrome.power API if it's present and chrome.experimental.power
            otherwise.
