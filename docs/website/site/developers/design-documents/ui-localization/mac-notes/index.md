---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/ui-localization
  - Add & translate strings (aka 'Localization' or 'Translations')
page_name: mac-notes
title: Mac Notes
---

[TOC]

keywords: i18n, internationalization, l10n, localization

## Mac Chromium UI Localization

In a nutshell, Apple's model for localizing a UI is to use .lproj directories
and localize your xib files for each language. This way the UI can be tweaked
for sizing to make sure everything fits, including changing layouts as needed
for the locales where the normal layout just doesn't work ([Apple's Localization
Documentation](http://developer.apple.com/internationalization/localization/)).
At the moment (August 2009), the tree has 21 xib files and the GRD files list 50
languages; that means maintaining and shipping over 1000 nib files! A tool could
be written to automate pulling the strings from the pak files and putting them
into the xib files, but that still leaves the sizing/layout adjustments. Within
Google, there are projects that automate driving every part of a product's UI,
in every language, and taking screen shots. Someone could inspect all of these,
tweak the layouts to get a working UI; but to keep the UI working, there would
need to be automation to continually do this to detect when any UI changes, and
manually re-inspect each time any part of the UI changes to catch new layout
problems, but that means signing up for that work plus the shipping overhead of
the nibs.

Some of the cross platform model code already generates content for the UI out
of pak files. We're following that model with our xib based UI, and doing as
much as possible at runtime. The nibs compiled out of xibs are landing in the
Resources directory of the app directly, ie-they aren't language specific. The
actual strings in them are markers so a helper can fetch the right strings from
the pak files. We've also got some helpers that will do their best to *tweak*
the sizes of objects based on the new string values. The good news is this looks
like it is going to be able to cover a lot of the needs we have; but it also
means, if we really have to, we can also fall back to creating a locale specific
xib for any specific locales that need in (put it in the right .lproj subfolder
and Cocoa will pick that up over the more generic one).

These next sections cover some of what is available and how to use it for making
sure any Mac UI being built is localized.

## String Constants

### Context of the String

Since the UI is similar on all platforms there is a good chance the string you
need already exists in the GRD files, but make sure you are using it from the
right context. Sometimes multiple IDS_\* values have the same English text, in
many cases these string values won't be the same in all languages. To figure out
the right one to use, read the description to see how they are intended to be
used (e.g., one might be a menu item, and another a window title). The
differences are usually things like one use is as a Noun (a title, label) and
the other is a Verb/Action (menu item, button)–so using the right one is
important.

### New Strings

If you do need to add new constants to generated_resources.grd, make sure both
the IDS_\* value and the description on it are clear. When it reaches the
Translation Console they will only have the description to understand what it
is/how it's used; and if someone is looking at a code/XIB, they might only have
the IDS_\* value to go by. Remember to put the new entry into a conditional
block so it is only defined on the Mac, we don't want to cause other platforms
to carry strings they don't actually need. There doesn't appear to be a solid
convention on the naming of the IDS_\* constants, but TVL (following what
Pinkerton did) has been putting _MAC at the end of the new ones he's created to
help make it clear they are specific to one platform; it can't hurt to follow
this convention.

## Building/Updating UI from Code

Chrome already includes some helpers for fetching resource strings and
assembling them (src/ui/base/l10n/l10n_util.h). There are some Mac additions
(src/ui/base/l10n/l10n_util_mac.h) to help get things into the form needed for
Cocoa UI:

*   Fetch resource strings directly into NSStrings via
            l10n_util::GetNSString() and l10n_util::GetNSStringF().
*   Some of the resources strings include the Windows keyboard
            accelerator marker (*"&Cut"*, Alt-C would jump the focus to that UI
            item), and they also tend to have three periods (...) instead of
            ellipses (…). l10n_util::FixUpWindowsStyleLabel() will clean these
            up and return an NSString.
*   If you are fetching a resource string that is meant for the UI
            (i.e.-the combination of the two above), you can do it in one call
            with l10n_util::GetNSStringWithFixup() and
            l10n_util::GetNSStringFWithFixup().

The one to remember here is l10n_util::FixUpWindowsStyleLabel(). If you are
getting a list of menu items, button title, etc. from cross platform code, there
is a good chance you will need to run it though there before putting it into
Cocoa UI objects.

*Example:* See src/chrome/browser/tab_contents/render_view_context_menu_mac.mm.

## UI from XIB Files

The l10n support is built on GTM's
[UILocalizer](http://code.google.com/p/google-toolbox-for-mac/source/browse/trunk/AppKit/GTMUILocalizer.h)
and
[UILocalizerAndLayoutTweaker](http://code.google.com/p/google-toolbox-for-mac/source/browse/trunk/AppKit/GTMUILocalizerAndLayoutTweaker.h).
For more detail than the steps below, please see [GTM's
docs](http://code.google.com/p/google-toolbox-for-mac/wiki/UILocalization) and
source.

### Menus Only

For places where you need to manually load a menu from a XIB file (like the
menubar or the menus for the bookmark bar), build the menus in the XIB file like
normal, then once you have them wired up go back back and make it localizable as
follows:

1.  Check whether the string you need already exists. If you find a
            matching string, read the description to see how it's intended to be
            used, and leverage it if you can. Otherwise, continue.
2.  Open chrome_nibs.gypi and add your XIB to the max_translated_xibs
            list.
3.  Update the titles to be ^IDS_CONSTANT_NAME (yes, put a caret
            followed by the IDS constant name into the XIB). You can even do
            things like ^IDS_CONSTANT$IDS_PRODUCT_NAME, and use the
            l10n_util::GetStringF API to insert one string into another (say for
            "*About \[product name\]*").
    *   See tips on [writing better user-facing
                strings](/user-experience/ui-strings).
    *   Include
                [screenshots](/developers/design-documents/ui-localization#TOC-Add-a-screenshot),
                [meanings](/developers/design-documents/ui-localization#TOC-Use-message-meanings-to-disambiguate-strings),
                and
                [descriptions](/developers/design-documents/ui-localization#TOC-Write-good-descriptions)
                for all strings. This is crucial for high-quality translations.
    *   Use [ICU message
                syntax](http://userguide.icu-project.org/formatparse/messages)
                to accommodate plurals and gender.
4.  In your XIB, and add an NSObject.
5.  In the Inspector, change the class of this NSObject to the
            ChromeUILocalizer class.
6.  Still in the Inspector, switch over to the outlets for this object,
            where you'll have three outlets. Wire any of them to the different
            menus you need in your XIB file. You don't need to wire owner_ to
            the file owner; any menu will work here, and owner_ can have extra
            meaning (see the UI section).

That's it. During the awakeFromNib the Localizer will run through any referenced
object and change any strings that started with ^IDS_ to the values that are
looked up.

*Examples:* See src/chrome/app/nibs/BookmarkBar.xib,
src/chrome/app/nibs/MainMenu.xib

### Windows and Views

Even for the most basic of UIs, like the bookmark editor or new bookmark folder,
it can get complicated as the names of labels and button can change with
different languages. Just how much work you need to do depends on how complex
your UI is.

#### Very Simple UIs

If you are very lucky, you just need strings replaced and nothing will ever get
clipped, then...

Build your UI as you normally would and get things working, then (and these are
very similar to what menu's listed):

1.  Check whether the string you need already exists. If you find a
            matching string, read the description to see how it's intended to be
            used, and leverage it if you can. Otherwise, continue.
2.  Open chrome_nibs.gypi and add your XIB to the max_translated_xibs
            list.
3.  Update the UI strings to be ^IDS_CONSTANT_NAME (yes, put a caret
            followed by the IDS constant name into the XIB). You can even do
            things like ^IDS_CONSTANT$IDS_PRODUCT_NAME, and use the
            l10n_util::GetStringF API to insert one string into another (say for
            "*About \[product name\]*").
    *   See tips on [writing better user-facing
                strings](/user-experience/ui-strings).
    *   Include
                [screenshots](/developers/design-documents/ui-localization#TOC-Add-a-screenshot),
                [meanings](/developers/design-documents/ui-localization#TOC-Use-message-meanings-to-disambiguate-strings),
                and
                [descriptions](/developers/design-documents/ui-localization#TOC-Write-good-descriptions)
                for all strings. This is crucial for high-quality translations.
    *   Use [ICU message
                syntax](http://userguide.icu-project.org/formatparse/messages)
                to accommodate plurals and gender.
4.  In your XIB, and add an NSObject.
5.  In the Inspector, change the class of this NSObject to the
            ChromeUILocalizer class.
6.  Still in the Inspector, switch over to the outlets for the Localizer
            object:
    *   If the owner for this XIB is a subclass of NSViewController or
                NSWindowController, you can just wire owner_ to the XIB owner,
                and the Localizer will run through the controller's view/window
                (going into view, subviews, view's menus, etc.) processing all
                strings that start with ^IDS_\*. This will not only handle
                titles, but also altTitles for VoiceOver, tooltips, etc.
    *   If the owner of the XIB isn't one of these and/or you have
                another view/window in the XIB, you can use the
                otherObjectToLocalize_ and yetAnotherObjectToLocalize_ to wire
                those up also (if you have NSMenus you don't need to directly
                wire them if they are wired into the view hierarchy that is
                already being walked).

*Example:* See ???

#### Simple Entry UIs

If you have a UI with labeled text fields and/or checkboxes/radios, then odds
are you need some of the object sizes to be tweaked so text isn't truncated and
the clickable areas don't extend miles to the right. GTM's
UILocalizerAndLayoutTweaker can handle some of this for you. The GTM docs
include a page with examples showing some of what can be done, [GTM's
UILocalization
docs](http://code.google.com/p/google-toolbox-for-mac/wiki/UILocalization).

Build your UI as normal, then:

1.  Do steps 1-4 of the Very Simple UI case (but do NOT wire up any
            outlets on the Localizer).
2.  Go through your UI putting things that need alignment into
            CustomViews and change the view's class to be a
            GTMWidthBasedTweaker. See the [GTM
            docs](http://code.google.com/p/google-toolbox-for-mac/wiki/UILocalization)
            for some of what can be done.
3.  Add an NSObject to your XIB file.
4.  In the Inspector, change the class of the new object to be a
            GTMUILocalizerAndLayoutTweaker, and wire:
    1.  The localizer_ outlet to the Localizer you created before.
    2.  The uiObject_ to the window/view you need fixed up.

*Example:* See src/chrome/app/nibs/BookmarkEditor.xib.

Notes for the curious:

*   For anything in Chrome you wire up the LocalizerAndLayoutTweaker's
            localizer_ rather than the owner_, because we aren't using the stock
            GTM behavior of localizing via .strings files.
*   You don't wire any of the outlets on the Localizer because there is
            no defined order on when the two objects' awakeFromNib will be
            called, so the LocalizerAndTweaker drives the Localizer to define
            the order.

#### Complex UIs

Preferences was sorta the best example of this, but that has since moved to
domui, so the relevant info isn't in the tree any more. If you are curious, you
can look at the history to dig up the sources/xibs to see how it was done.

#### Dynamic UIs

There are cases where the data that ends up being displayed is dynamic, in that
it includes data that comes from webpages, servers, etc. For these, we will
probably always need to do custom code to help with the layout.

*Examples:* See src/chrome/browser/cocoa/infobar\*.
