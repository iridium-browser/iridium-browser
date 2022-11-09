---
breadcrumbs: []
page_name: embeddedsearch
title: Embedded Search API
---

**This document describes a replacement for the [SearchBox API](/searchbox)
designed for search providers included with Chromium to allow them to integrate
more tightly with Chrome’s Omnibox and New Tab Page. This document will be kept
up to date with the changes in trunk Chromium and is to be considered the
authoritative reference on this new API.**

**Overview**
**The Embedded Search API provides a point of integration for a default search
provider:** The provider is responsible for rendering the New Tab Page (NTP).

**Search providers who wish to try EmbeddedSearch NewTabPage API can do so by
modifying the TemplateURL data for their provider (in
[src/components/search_engines/prepopulated_engines.json](https://code.google.com/p/chromium/codesearch#chromium/src/components/search_engines/prepopulated_engines.json))
to add a "new_tab_url" field pointing to an URL that will render the NTP.
Implementers can find an example in the chromium sources at
[src/chrome/browser/resources/local_ntp/local_ntp.js](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/browser/resources/local_ntp/local_ntp.js).**

**Reference**

**// This interface is exposed as window.chrome.embeddedSearch to the embedded search page.**
**interface EmbeddedSearch {**

**// Interface to allow the embedded search page to interact with and control the**
**// navigator searchbox. Exposed as window.chrome.embeddedSearch.searchBox.**
**interface SearchBox {**

// Indicates whether the browser is rendering its UI in rigth-to-left mode.

readonly attribute unsigned boolean rtl;

// Requests the searchbox to enter (or exit) a mode where it is not visibly
focused

**// but if the user presses a key will capture it and focus itself. Conceptually**
**// similar to an invisible focus() call.**
**void startCapturingKeyStrokes();**
**void stopCapturingKeyStrokes();**

**// Attribute and listener to detect if the searchbox is currently capturing user key**
**// strokes.**
readonly attribute boolean isFocused;
**readonly attribute boolean isKeyCaptureEnabled;**
attribute Function onfocuschange;
**attribute Function onkeycapturechange;**
**}; // interface Searchbox**
**// Interface to allow the embedded search page to interact with and control
the**

**// New Tab Page. Exposed as window.chrome.embeddedSearch.newTabPage.**
**interface NewTabPage {**

// Attribute and listeners that indicate to the new tab page whether user input

// is in progress in the omnibox.

readonly attribute boolean **isInputInProgress**;

attribute Function **oninputstart**;

attribute Function **oninputcancel**;

// List of user's most visited items.

readonly attribute Array&lt;MostVisitedItem&gt; **mostVisited**;

attribute Function **onmostvisitedchange**;

// Deletes the most visited item corresponding to |restrictedID|.

function **deleteMostVisitedItem(**restrictedID**)**;

// Undoes the deletion of the item for |restrictedID|.

function **undoMostVisitedDeletion(**restrictedID**)**;

// Undoes the deletion of all most visited items.

function **undoAllMostVisitedDeletions()**;

// Information necessary to render the user's theme background on the NTP.

**readonly attribute ThemeBackgroundInfo themeBackgroundInfo;**

attribute Function **onthemechange**;

**}; // interface NewTabPage**
**};**
// Format for a most visited object supplied by the browser.

//

// Most visited items are represented by an opaque identifier, the "rid". After
M72, Chrome

// provides one special URL that the page can render as &lt;iframe&gt;s to
display

// most visited data:

//

// chrome-search://most-visited/title.html?rid=&lt;RID&gt;

//

// Prior to M72, Chrome also provided a second URL which has now been
deprecated:

//

// \[deprecated\] chrome-search://most-visited/thumbnail.html?rid=&lt;RID&gt;

//

// The URL also accepts the following query parameters to modify the style of
the

// content inside the iframe:

//

// 'c' : Color of the text in the iframe.

// 'f' : Font family for the text in the iframe.

// 'fs' : Font size for the text in the iframe.

// 'pos' : The position of the iframe in the UI.

//

// For example, the page could create the following iframe

//

// &lt;iframe
src="chrome-search://most-visited/title.html?rid=5&c=777777&f=arial%2C%20sans-serif&fs=11&pos=0"&gt;

//

// to render the title for the first most visited item.

MostVisitedItem {

// The restricted ID (an opaque identifier) to refer to this item.

readonly attribute unsigned long rid;

// URL to load the favicon for the most visited item as an image.

//

// NOTE: the URL is particular to the current instance of the New Tab page and

// cannot be reused.

readonly attribute string faviconUrl;

}

**// Format of the ThemeBackgroundInfo object which indicates information about the user's**
**// chosen custom UI theme necessary for correct rendering of the new tab page.**
**ThemeBackgroundInfo {**

// NOTE: The following fields are always present.

// True if the default theme is selected.

readonly attribute boolean **usingDefaultTheme**;

// Theme background color in RGBA format.

readonly attribute Array backgroundColorRgba;

// Theme text color in RGBA format.

readonly attribute Array **textColorRgba**;

// Theme text color light in RGBA format.

readonly attribute Array **textColorLightRgba**;

// True if the theme has an alternate logo.

readonly attribute boolean **logoAlternate**;

**// NOTE: The following fields are all optional.**

**// Theme background-position x component.**
**readonly attribute String imageHorizontalAlignment;**
**// Theme background-position y component.**
**readonly attribute String imageVerticalAlignment;**
**// Theme background-repeat.**
**readonly attribute String imageTiling;**

// True if the theme has an attribution logo.

readonly attribute boolean **hasAttribution**;

**};**

Last edited: Wednesday, November 12, 2018, 10:45 PM PST
