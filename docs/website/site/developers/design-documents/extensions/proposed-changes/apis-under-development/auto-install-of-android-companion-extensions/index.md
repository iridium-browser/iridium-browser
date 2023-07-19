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
page_name: auto-install-of-android-companion-extensions
title: Auto-install of android companion extensions
---

Proposal Date
12/12/2012
Who is the primary contact for this API?
Francois Dermu ([mwd743@chromium.org](mailto:mwd743@chromium.org))
Who will be responsible for this API? (Team please, not an individual)

Apps Extension API Team

Overview
A way for third parties that have both an Android app and a Chrome extension to
get their companion chrome extensions automatically installed on the user's
chrome browser without having to ask them to go to their browser and type in a
URL to download it or to do a search in the Chrome web store.
Use cases
My use case is that I have an app that let the user transfer informations to a
chrome extension. My problem is that the chrome extension is not easily
discoverable and there is no easy way for me to send the user from his phone
over to the right page on his chrome browser. I can create a short URL for him
to remember and manually type in his browser but this is far from the ease of
use I had in mind. Also the user may not be in front of his computer when he
installs my app.
Do you know anyone else, internal or external, that is also interested in this
API?
A good example would be for Evernote. They have an android app and an extension
that lets the user take snippets of web pages and save them in their notes that
they can then view on their phone. Wouldn't it be great if once the user
installed the android app he is presented with the option to also automatically
install the chrome extension so that when he goes to his computer it's already
there for him to use.
Could this API be part of the web platform?
I don't think so since this is dealing with chrome extensions specifically.
Do you expect this API to be fairly stable? How might it be extended or changed
in the future?
Unless you change the way extensions are installed this API shouldn't ever
change.

**If multiple extensions used this API at the same time, could they conflict
with each others? If so, how do you propose to mitigate this problem?**
If multiple 3rd party call this API the browser should just queue up the
installations. I don't foresee any issues with that. The only issue I could
imagine is if you are trying to install an extension that is incompatible with
your version of chrome or other already installed extensions but is that even
possible ?

List every UI surface belonging to or potentially affected by your API:
This API shouldn't affect any UI.
**Actions taken with extension APIs should be obviously attributable to an
extension. Will users be able to tell when this new API is being used? How?**

We could simply use the existing way that Chrome tells users that there is a new
extension (Chrome shows a balloon next to the new button saying that the
extension is installed)

**How could this API be abused?**
**Ill-intentioned app developers could try to abuse this api by making you
install thing you don't want.**

**Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you could commit with your API (if you’ve got good ones, feel free to add more):**
**1) try to purposely send installation requests for thousands of extensions
(**Denial of Service type of attach by sending an insane amount of requests**)**

**2) the same one multiple times
3) make you install paid apps or extensions without your knowledge**

4) **making you install extensions that they didn't create for a profit**
What security UI or other mitigations do you propose to limit evilness made possible by this new API?**
These abuses would obviously need to be adressed by limiting the amount of
extension an app can install, the delay between installation and probably
handling paid apps differently like for example opening the store page but not
pursuing the purchase or simple forbidding this API for paid apps. Performance
shouldn't be degraded any further than when the user installs extensions
manually and there is already a synching mechanism in place to auto-install
extensions and apps to match it across all your chrome instances. On the android
side an app that calls that API should have that functionality listed in the
same place where they put all the warnings about accessing your data during
installation.**

**If the API is called for an extension that is already installed nothing would
happen.**

The extensions would have to be registered to the same author than the android
app to prevent bloatware installation.

Since the user would have to be logged in to the same Google account on both the
phone and chrome there shouldn't be any security risks.
**Alright Doctor, one last challenge:**
**Could a consumer of your API cause any permanent change to the user’s system using your API that would not be reversed when that consumer is removed from the system?**
**Doesn't apply here.
How would you implement your desired features if this API didn't exist?
One could make an extension that essentially allows other third parties to
install extensions remotely. The user would have to manually install that
extension but once he has it and the API is public the door would be open for
anyone to remotely install extensions on that browser.
Draft Manifest Changes

**Doesn't apply here.**

Draft API spec
There would be an android API that once called the android system would then
transfer the installation request to Chrome's already existing mechanism of
extension installation (as part of the sync mechanism)

The Chrome API itself wouldn't be disclosed to the public (only the android one)

The android API could be apart of the android version of Chrome (that the user
would have logged in with the same account as the one on his computer)
Open questions
