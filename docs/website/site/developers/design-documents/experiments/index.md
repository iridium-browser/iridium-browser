---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: experiments
title: Experiments
---

Here's how to hide a feature behind a flag that can be changed from the
`chrome://flags` page in Chrome, or with a command line flag. We'll use an
example from the following revision
[84971](https://src.chromium.org/viewvc/chrome?view=rev&revision=84971) which
hides the compact navigation bar behind such a flag.

*   First, you need to add a string for your command line switch. For example:
    [chrome_switches.h](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/common/chrome_switches.h?r1=84971&r2=84970&pathrev=84971)
    &
    [chrome_switches.cc](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/common/chrome_switches.cc?r1=84971&r2=84970&pathrev=84971)
*   Then you need strings to be displayed on the `chrome://flags` page for the
    name and description of your experiment. For example, the two strings with
    IDs starting with IDS_FLAGS_ENABLE in:
    [generated_resources.grd](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/app/generated_resources.grd?r1=84971&r2=84970&pathrev=84971)
*   And then add all this info to a new entry in the Experiments array. For
    example:
    [about_flags.cc](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/browser/about_flags.cc?r1=84971&r2=84970&pathrev=84971)
    *   The first string (e.g., "compact-navigation") is an identifier for the
        preference that will be saved in the user profile based on the settings
        chosen by the user on the `chrome://flags` page.
    *   The the resource IDs for the name and description strings.
    *   Then which OS should expose it.
    *   And finally, the access to the command line flag, using one of the
        following macros:
    *   SINGLE_VALUE_TYPE_AND_VALUE, SINGLE_VALUE_TYPE, or MULTI_VALUE_TYPE.
*   And finally, use the
    CommandLine::ForCurrentProcess()->[HasSwitch](https://source.chromium.org/chromium/chromium/src/+/main:base/command_line.cc?q=CommandLine::HasSwitch)(<*your
    flag*>) to check its state. For example:
    [tab_menu_model.cc](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/browser/ui/tabs/tab_menu_model.cc?r1=84971&r2=84970&pathrev=84971)
    uses it to decide whether we show the Hide toolbar option in the tab
    contextual menu or not.
