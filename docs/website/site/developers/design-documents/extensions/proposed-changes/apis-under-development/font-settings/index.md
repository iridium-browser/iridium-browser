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
page_name: font-settings
title: Font Settings
---

Font Settings API is now stable! See the
**[chrome.fontSettings](http://developer.chrome.com/stable/extensions/fontSettings.html)
documentation.**

Proposal Date
Jan 17, 2012

Who is the primary contact for this API?
*@)*Matt Falkenhagen (falken
Who will be responsible for this API? (Team please, not an individual)

Chrome Tokyo team / Chrome i18n team

Overview
Expose Chrome/WebKit font preferences to extensions, much like the Proxy Settings API.
Use cases
We want to add a more advanced font settings UI than is currently available, and
it might be decided that it should be an extension in interest of keeping the
Chrome UI simple.

Specifically, Chrome/WebKit recently added per-script font settings, so e.g.,
the "sans-serif" font setting for Simplified Chinese script can be different
than the one for Traditional Chinese, or Japanese, etc. We want users to have
access to these preferences. This is [crbug.com/2685](http://crbug.com/2685).

Furthermore, there are other font settings not currently exposed in the UI, such
as for fantasy and cursive fonts. This API would allow extensions for those
settings.

Do you know anyone else, internal or external, that is also interested in this
API?
No.
Could this API be part of the web platform?
No, this is simply client-side Chrome/WebKit settings. I don't see how it could be part of the web platform.
Do you expect this API to be fairly stable? How might it be extended or changed
in the future?
Yes, it will be fairly stable, only changing when the underlying Chrome/WebKit
settings change. Some reasons it may change are:

*   It could be extended if support for additional scripts or CSS font
            families is needed.
*   The per-script settings may be extended to be per-script+language.
            For example, Farsi/Persian may need to use a different font than the
            one for Arabic language although both are written in Arabic script.
*   Some settings like default font size, default fixed font size,
            minimum font size that are currently global settings may become
            per-script settings in the future.
*   We could also add more parameters like font settings for certain URL
            patterns.

List every UI surface belonging to or potentially affected by your API:
Since it's exposing Chrome/WebKit font settings, it should affect only the web
content renderer. These settings are not used in native UI surfaces to my
knowledge.

How could this API be abused?
Describe any concerns you have with exposing this API. Particular attention
should be given to issues of security, privacy and performance.

Since Chrome observes font settings changes and propagates them to WebKit, which
redraws as necessary, maybe an extension can cause performance disruptions.

Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you
could commit with your API (if you’ve got good ones, feel free to add more):
1) An extension that trashes the font settings. Uninstalling the extension would
undo the damage.

2) An extension that rapidly keeps changing the fonts. It could conceivably be a
DOS attack as WebKit tries to keep up. 3) An extension that reads font settings
and uploads to a server to try to build a database fingerprinting users by the
font settings. Also, if the API exposes a list of fonts installed on the system,
it can also be a source of information for fingerprinting. See:
<http://trac.webkit.org/wiki/Fingerprinting>

Alright Doctor, one last challenge:
Could a consumer of your API cause any permanent change to the user’s system
using your API that would not be reversed when that consumer is removed from the
system?

No. It just uses the standard [extension-controlled
preferences](http://www.chromium.org/developers/design-documents/preferences#TOC-Extension-Controlled-Preferences)
feature which handles cleanup when the extension is uninstalled.

How would you implement your desired features if this API didn't exist?
It could be part of the built-in Settings UI instead of relying on extensions.
Draft API spec
For the appropriate style, please refer to [existing
APIs](http://code.google.com/chrome/extensions/docs.html) as well as the [other
API proposals](/developers/design-documents/extensions/).
For example, note that API calls that affect the browser process must be
asynchronous, to prevent extension JavaScript from blocking on the browser UI.
See the [current API
spec](http://code.google.com/chrome/extensions/trunk/experimental.fontSettings.html)
on trunk documentation.

Originally I planned to use
[ChromeSetting](http://code.google.com/chrome/extensions/types.html#ChromeSetting)
to expose all preferences, but there would be too many of them (about 30 scripts
with 6 font families each...).

Open questions
Note any unanswered questions that require further discussion.

There was a possibility of putting per-script and other advanced font settings
in the built-in UI, but to keep Chrome UI simple this is unlikely to happen.
