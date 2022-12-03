---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: about-conflicts
title: Design document for about:conflicts
---

    ### Objective

    This feature is intended to provide the user with a good overview of what 3rd party modules (ie. DLLs on Windows) get loaded into their Google Chrome installation and to call out suspected and known bad modules. Hopefully this will lead to increased awareness of the adverse effect that 3rd party modules can have on the stability of Google Chrome and make troubleshooting easier. In turn, this should have a positive effect on Google Chrome stability.

    Background
    Windows provides developers with an array of options for hooking into both the operating system as well as other applications on the system. Windows provides APIs to, for example, extend the functions of the shell, context menus and to hook into the network stack, not to mention methods to simply inject DLLs into random processes and start executing. There is even a registry key that lists DLLs to get automatically injected into all processes.
    These techniques are often used for legitimate purposes, such as security software (VPN, firewalls, anti-virus, anti-spyware), accessibility (IME) or for general utilities (download managers). Of course this can also be used by malware (viruses, spyware, trojans, etc). To facilitate discussion, we'll refer to the functions these modules provide as 'extensions' (not to be confused with Chrome Extensions).
    Unfortunately, most software vendors don't have the resources to test their extension for compatibility problems with all the applications in use in the wild (and most malware writers wouldn't even if they had the resources). Therefore, the code injected in this way often causes major stability problems for the target applications. Chromium developer Eric Roman analyzed all crash reports for a particular version of Google Chrome and found that over 1 in 4 crashes reported had 3rd party code on the call stack at the time of the crash. It's hard to tell whether the 3rd-party code is directly to blame, but conversely if the extension module causes memory corruption, the crash will often happen when the extension module is no longer on the stack, leaving a weird crash in some code that can be entirely unrelated to the task at hand.
    As far as the user is concerned, there is hardly any correlation between the application crash and the extensions installed, so users have little means to differentiate between a buggy application and a buggy extension. Naturally, the tendency is to blame the application developers and if they can't pinpoint the extension that caused the problem the crashes will simply go unnoticed by the extension developers.

    ### Goals

    The goals of this feature are to:

    *   Identify all modules running inside Google Chrome.
    *   Call out suspected and known bad 3rd party modules (using a list
                provided by Support).
    *   Introduce a flag that allows the user to opt-in to a background
                thread check for known bad modules and show an indicator on the
                wrench menu when incompatibilities are detected.

    Possible goals for future versions:
    *   Port the feature to non-Windows platforms.
        *   Historical note: The name of this feature got changed from
                    about:dlls to about:conflicts (to be platform neutral).
    *   Provide backend support for a safe-browsing type of database
                that can be auto-updated in the background.
        *   To keep things simple for v1 and to test the concept the
                    module blocklist will be baked into Chrome.
    *   Provide functionality for the user to block certain modules.
        *   This is non-essential for v1 but might be implemented later.
    *   Integrate with --diagnostics to list bad modules. *\[Done:
                Implemented for v10\]*

    ### Non-goals

    *   Identify modules that actively try to hide their existence or
                for some reason or other won't show up in the module list for
                the running process.
    *   Ensure access to about:conflicts even in the event of a module
                that crashes Chrome at every startup (except maybe in
                conjunction with --diagnostics).

Use Cases

    The main use case of this feature is simply the user typing in about:conflicts into the Omnibox. This will show a simple web page, similar to about:version, except that the page will enumerate all modules loaded into the browser process and cross-reference each module with a list of known or suspected bad actors. It should also enumerate modules that have not loaded yet, but are likely to (ie. shell extension registrations and Winsock LSPs).
    The other use case would be for the user to opt-in to have the modules periodically enumerated and, on finding known bad actors, show a red dot (see below) over the wrench menu that promotes the about:conflicts page.

    Detailed Design
    The about:conflicts page
    about:conflicts is simply an HTML resource, much like about:version, loaded from the resource bundle and populated with the needed data at runtime. See The detection mechanism below.
    For a proposed screenshot of the page see comment 5 of issue 51105:
    <http://code.google.com/p/chromium/issues/detail?id=51105#c5>
    *\[Update: The look of the about:conflicts page no longer resembles the image in the bug\]*

    The red dot

    *\[Update: The wrench image has since changed and the badging as well -- now
    it is a 'hotdog' menu that changes colors (instead of being badged). This
    doc will still refer to the original design, though: the red dot on the
    wrench menu.\]*

    The red dot is the equivalent to the badge we show when there is an update for Chrome available, except that instead it notifies you of confirmed incompatibilities (not suspected ones). It might look something like this:

[<img alt="image"
src="/developers/design-documents/about-conflicts/incompatibility_thumb.png">](/developers/design-documents/about-conflicts/incompatibility_thumb.png)

    Similarly, the wrench menu that pops up will have a menu item 'View incompatibilities', which, when clicked, will take the user to the about:conflicts page.
    If both an incompatibility and a new version are detected, the update dot will show on the Wrench icon (not the red dot) but both menu items will appear in the Wrench menu (both the 'update available' menu item and the 'incompatibility detected' menu item).

    The list of bad actors
    A given module has many properties that can be used to try to uniquely identify it. The intent is not to get into the pattern matching business but to use some of the properties that we can extract at run time to try to identify modules. The information that is readily available is the module file name, a location (path) and its file size. Modules often (but not always) contain version information, a description, information about the product it belongs to (name and version) and it may also be digitally signed).
    To start with, we'll use the normalized path (described below) + filename, the description (if any) and a version range (if version is available to us and we need to match a certain version only).
    Paths need to be normalized to account for differences in OS installation and profile location. This means that we'll need to convert local paths from c:\\user\\foo\\bar to %USERPROFILE%\\foo and from short names to long names before we use them for comparison. Furthermore, we should also lower-case all the strings before converting them to hashes (SHA-256?) and use only the hashes to match modules on the client. This will prevent the list from overly increasing the size of the compiled Google Chrome binary.
    Every entry in the list of bad actors will have an associated bitmask of help tips to display along with a confirmed/suspected match.
    Possible bitmasks:
    NONE -- This means we have no help tip. Shows: "We are currently investigating this issue"
    UNINSTALL -- We know uninstall is possible and has resolved the issue for some: "You may try uninstalling this product to see if it resolves the issue".
    DISABLE -- Same as uninstall except "uninstalling" -&gt; "disabling".
    UPDATE -- Same as uninstall except "uninstalling" -&gt; "updating".
    SEE_LINK -- If this is provided, we'll append the link "Learn more" to the end.
    The goal here is to avoid having to ship the help strings down to the client. Also to be generic enough to be included in the resource files, localized to each language we support and reused whenever we find a match. If more details are required, such as a link to a hotfix, we'll need to provide this in the help center article.

    If uninstall, disable and/or update appear together, we'll collapse the string into one, such as: "You may try uninstalling, updating or disabling this product to see if it resolves the issue". 'None' can not be used if Uninstall, Disable and Update is specified.
    If SEE_LINK is provided, we'll use the hashes to construct a link to a help center article passing as parameters the hashes we constructed for the list, so we can customize the help messages shown to the user.
    The list would be constructed as follows:
    Filename | location | Signer (if available, otherwise module Description) | version lower | version upper | help tip
    An example hypothetical entry in the list:
    mshtml.dll | %windir%\\system32 | Microsoft (R) HTML Viewer | 1.0 | 1.1 | UPGRADE or SEE_LINK
    This would translate into:
    32bithash1 | 32bithash2 | 32bithash3 | string | string | bitmask
    If the link is clicked, it would (hypothetically) navigate to something like:

    http://www.google.com/support/chrome/bin/answer.py?answer=&lt;someid&gt;&topic=&lt;sometopic&gt;&hl=&lt;language&gt;&n=&lt;hash1&gt;&l=&lt;hash2&gt;&d=&lt;hash3&gt;&s=&lt;hash3&gt;

    The detection mechanism
    The client code will enumerate all modules loaded into the browser process and all registered modules of interest (ie. shell extensions). We'll need to enumerate the registered modules as well because some modules load on-demand only (and might even crash the browser while doing so).
    A module is considered a suspected match if its file name matches a file name on the list. If either an Authenticode signature signer or *description* is provided in the blocklist, then we can possibly upgrade the status to confirmed match. Specifically, a module is considered a confirmed match if (in addition to the file name):
    *   The module (DLL) is Authenticode signed and the digital
                signature signer matches the signer in the blocklist.
        *   _or_
    *   Both the location of the module and the description matches the
                information provided in the list of bad actors.
        *   _or_
    *   The *description* field and the *signer* field in the blocklist
                is left blank.
    Note: If a version range is provided in the list, it must match for the module to be considered either a suspected/confirmed match. The same applies to location.
    To begin with, at least, the red dot will only show for confirmed bad modules (for those opting in). We can later experiment on the Dev channel with showing it also for suspected bad modules.
    Data collection
    For those users who opt-in to data collection (UMA) we'd want to know...
    *   How many bad modules did we detect?
    *   How often are we displaying the red dot on the Wrench menu?
    *   How long does it take to go through the list of all modules?
    FAQ
    Most of this is constructed from discussions among various Chromium members regarding this feature/different approaches.
    Why not block all 3rd party modules in the browser process?
    Because that penalizes the good extension developers that provide properly written useful extension functionality. **Update**: See <https://crbug.com/690166> and <https://crbug.com/846953>
    How about introducing a list of allowed modules?
    Such a list would likely be too hard/time consuming to construct and maintain due to the number of items that users would want to be allowed.
    How about automatically blocking all known bad modules?
    Concerns have been raised that automatically blocking modules introduces code that adversely affects startup time for people who have no bad modules installed. In the future we might look into the possibility of checking on a background thread for known bad modules and set a flag for next startup to eject that module. That's beyond the scope of this feature though (at least the first version).
    Where does the block list come from?
    The block list is constructed by a collaboration between Support and people working on browser stability.
    Won't the red dot on the Wrench menu be annoying for false positives (ie. if just names of modules matching)?
    The red dot notification is opt-in by a flag, but it won't in any case be shown for suspected matches where only the names matches. It has to be a confirmed match (where either the Authenticode signer matches or the location and description match).
