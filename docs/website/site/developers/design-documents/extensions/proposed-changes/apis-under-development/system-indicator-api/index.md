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
page_name: system-indicator-api
title: System Indicator API
---

**Proposal Date**
**14 November 2012**
**Who is the primary contact for this API?**
**dewittj@**
**Who will be responsible for this API? (Team please, not an individual)**

**dimich@, dewittj@**

**Overview**
**This API would serve two main purposes: first, to provide a system-wide status indication, appearing as an icon in the system tray area. Alongside this API, we would extend the chrome.contextMenus API to also control the system indicator context menu.**
**There is a class of applications that require the ability to let the user know, at a glance, the status of the application, regardless of what else is running on the system. These applications tend to fall into a few classes:**

1.  **Apps that wait for incoming events but do not have their own
            windows in the common case - e.g. chat, where a user needs to know
            that everything is OK with the application (online, available
            status) at a glance even when the app is otherwise idle.**
2.  **System status apps - e.g. a WiFi indicator app, which a user will
            check the app when she needs to either determine if a connection has
            been made, or troubleshoot her system**
3.  **Resource usage indicators - e.g. a battery indicator, that inform
            a user’s behavior over a long period of time (do I have x enough
            battery to write this document before finding an outlet? Is my
            battery draining more quickly than usual?)**

**Many applications can use such an API to make it easier for users to interact with their app. An app that requires frequent short interaction, such as a chat app, can make use of this to both create a visual cue that action needs to be taken (such as reading an incoming chat message), and to allow the user to bring into focus any existing chat conversations, or the contact roster panel.**
**This API exposes an icon whose behavior and support varies across operating systems. Frequently, users are allowed to hide icons; in some cases, such as ChromeOS, there is no appropriate area for such an icon. Accordingly, this API will expose functionality on a best-effort basis. Apps should provide all functionality exposed through this API in other places as well.**
**[Example usage and
images](https://docs.google.com/document/d/1QhhfR33Y28Yqnnoa_Sl3fnZK_mKtwt4dZe6kNyJ_MjU/edit)**

**Do you know anyone else, internal or external, that is also interested in this API?**
**Currently the “Chat for Google” application installs an NPAPI plugin to achieve this functionality. Already, NPAPI plugins are disabled on some systems and their use going forward is being discouraged. This API would allow them to remove their system indicator binary plugin and rely on Chrome APIs instead.**
**Could this API be part of the web platform?**
**This API is fundamental to long-running offline applications. As such it is not appropriate for the OWP.**
**Do you expect this API to be fairly stable? How might it be extended or changed in the future?**
**This API itself is minimal - only enabling apps to show and hide icons. Because the most complex portions of this feature will be within the chrome.contextMenus API, this API is expected to remain quite stable.**
**If multiple extensions used this API at the same time, could they conflict with each others? If so, how do you propose to mitigate this problem?**
**Multiple apps could indeed use this API at the same time. The system tray has historically been a place that every app wants to have a presence in, despite its extremely limited real estate. This will be mitigated by restricting apps to one indicator icon (or zero).**
**List every UI surface belonging to or potentially affected by your API:**
**This API would affect the system indicator area appropriate to each platform (OSX: the menu bar, Windows and Linux: the system tray), by adding an icon and exposing a menu on click.**
**Actions taken with extension APIs should be obviously attributable to an extension. Will users be able to tell when this new API is being used? How?**
**Icons used for this menu will be distributed with the extension, and there will be hovertext over the icon that shows the app’s name.**
**How could this API be abused?**
**This API could be abused by creating functionality that is only exposed through the menus or click event of the system tray. Since not all platforms support it, this would create a highly inaccessible and inconsistent experience across platforms. Additionally this API could be used to incessantly attract user attention.**
**Imagine you’re Dr. Evil Extension Writer, list the three worst evil deeds you could commit with your API (if you’ve got good ones, feel free to add more):**
**1) I could pretend to be an application that I’m not**
**2) I could write a bunch of extensions that all take icon space in the indicator area**
**3) I could create distracting icons and constantly draw the user’s attention to my app.**
**What security UI or other mitigations do you propose to limit evilness made possible by this new API?**

*   **We will require that the hovertext of this icon be the app’s name
            to prevent spoofing**
*   **Other evil acts would best be mitigated by uninstalling an
            offending app.**

**Alright Doctor, one last challenge:**
**Could a consumer of your API cause any permanent change to the user’s system using your API that would not be reversed when that consumer is removed from the system?**
**This API should not leave anything behind. The indicator icon will only be visible when the app is listening for messages, so removing the app will prevent any indicator from being shown.**
**How would you implement your desired features if this API didn't exist?**
**Currently the way to do this is via NPAPI plugins.**
**Draft Manifest Changes**
**This will require a new ‘systemIndicator’ permission so that users know that they’re volunteering access to the system indicator area.**
**Draft API spec**
*// Manages an app's system indicator icon, an image displayed in the system's
// menubar, system tray, or other visible area provided by the OS. namespace
systemIndicator {*

*dictionary SetIconDetails {*

*// Path should be either a string representing an icon loaded with the app
manifest,*

*// or a dictionary representing multiple resolutions of the same icon that are
to*

*// be used on systems with different icon sizes, or DPI scaling factors.*

*any? path;*

*// ImageData should be either a single ImageData object (typically from a
canvas*

*// element) or a dictionary representing multiple resolutions of the same icon.
any? imageData; }; callback DoneCallback = void (); interface Functions { // Set
the image to be used as an indicator icon, using a set of ImageData // objects.
These objects should have multiple resolutions so that an // appropriate size
can be selected for the given icon size and DPI scaling // settings. Only square
ImageData objects are accepted. static void setIcon(SetIconDetails details,
optional DoneCallback callback); // Show the icon in the status tray. static
void enable(optional DoneCallback callback); // Hide the icon from the status
tray. static void disable(optional DoneCallback callback); }; interface Events {
// Fired only when a click on the icon does not result in a menu being // shown.
static void onClicked(); }; };*

Open questions

*   System indicator areas are slowly being removed from each operating
            system. Win8 Metro doesn’t have one at all, and ChromeOS’ area is
            restricted to a few explicitly allowed native applications. How will
            the functionality from this API be exposed if this trend continues?
            Currently we are treating this API as providing an optional service
            that should not break functionality of the app if the icon and menu
            fail to display on a user’s system.
*   Different operating systems have different human interface
            guidelines for icons appearing in the status area. For example, Mac
            OS icons in the title bar area are typically black and white. How
            should this difference be exposed to the API consumer?
