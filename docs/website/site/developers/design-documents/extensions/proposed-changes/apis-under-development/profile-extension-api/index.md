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
page_name: profile-extension-api
title: Profile Extension API
---

**Proposal Date**
**December 27, 2012**
**Who is the primary contact for this API?**
**rlp@**
**Who will be responsible for this API? (Team please, not an individual)**
**Profiles team (sub team of Frontend team)**
**Overview**
**Access to profile information for extensions. This requires adding one method and extending another.**
**The addition would be a method which returns a list of all existing users on a machine. Users would be an object consisting of to two pieces of data: profile name and icon.**
**The second part of this proposal would be adding an optional profile parameter to the chrome.window.create method, allowing extensions to open a window for another user.**
**Use cases**
**An API to open windows in new profiles has been requested by the marketing team. Furthermore, providing access to the list of profiles would allow extensions to open windows in a selected profile. We've had several profile bugs about not being able to open a link or otherwise in the expected and/or desired profile.**
**Do you know anyone else, internal or external, that is also interested in this API?**
**The marketing team has requested access to multiple users for demos. This API would allow for writing an extension to give them the needed access.**
**Could this API be part of the web platform?**
**No, because profiles are specific to Chrome.**
**Do you expect this API to be fairly stable? How might it be extended or changed in the future?**
**Yes, this API should be stable due to its simplicity. Even if the underlying structure of profiles change, this should not affect this API.**
**Additional access to profiles could be added in the future.**
**If multiple extensions used this API at the same time, could they conflict with each others? If so, how do you propose to mitigate this problem?**
**This API is read-only. It cannot change or create new profiles. So there can be no conflict between multiple extensions.**
**List every UI surface belonging to or potentially affected by your API:**
**This would not add any new UI.**
**Actions taken with extension APIs should be obviously attributable to an extension. Will users be able to tell when this new API is being used? How?**
**For the extension to creating a new window, we would be piggybacking on existing methods for indicating a new window was opened by an extension. For getting the list of users, there is no external action which would be visible to users.**
**How could this API be abused?**
**This would provide the extension access to know which users are on a given instance of Chrome. Some may view this as as security concern although this information is already available.**
**By allowing the extension to open new windows, you could potentially switch users on someone.**
**Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you could commit with your API (if you’ve got good ones, feel free to add more):**
**1) Open up a website requiring a password with a different user. The original user doesn't realize the newly opened window is from a different user, enters their password which is then stored for the other user.**
**2) Get a list of the users on a computer and send that information back to a nefarious server (though I don't know what they'd do with that information).**
**3) Share information about one user with another.**
**What security UI or other mitigations do you propose to limit evilness made possible by this new API?**
**User icons from the extension API could be tagged to indicate they are from the extension api.**
**Alright Doctor, one last challenge:**
**Could a consumer of your API cause any permanent change to the user’s system using your API that would not be reversed when that consumer is removed from the system?**
**No they could not. All information is read-only so they could not make any permanent changes to the state of a user's Chrome.**
**How would you implement your desired features if this API didn't exist?**
**Currently there is no way to get this information or allow someone to open a window with a different profile.**
**Draft Manifest Changes**
**None.**
**Draft API spec**
**Types**
**Profile (object)**
**Properties of Profile**
**name (string)**
**The name of the profile.**
**icon (ImageData)**
**The icon of the profile.**
**Methods**
**getProfileList**
**chrome.users.getProfileList**

**Returns a list of current users known for this instance of Chrome. If the current user is in managed mode or on ChromeOS, this list will only contain the current user.**

**result (array of Profile)**
**Additional Parameter for chrome.windows.create**
**profileName (string)**
**The name of the profile which the window should be created for. If the current user is in managed mode or on ChromeOS, this parameter will have no effect as they can only open windows for the current user.**
**Open questions**
**Could this be a function under chrome.windows?**
