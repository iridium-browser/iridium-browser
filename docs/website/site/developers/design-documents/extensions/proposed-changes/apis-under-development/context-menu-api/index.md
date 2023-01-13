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
page_name: context-menu-api
title: Context Menu API Proposal
---

****NOTE: This document is now out of date. The API evolved some while behind the experimental flag, and is becoming fully supported in chrome 6. For details on the current implementation you can see**

******<http://code.google.com/chrome/extensions/trunk/contextMenus.html>********

********---********

****Status****

**Being implemented behind the --enable-experimental-extension-apis flag so we
can collect developer feedback. Should be landing in the tree relatively soon.**

**Overview**

Provide a way for extensions to add items to the context (aka "right click")
menu for web pages.

**Use cases**

Many features that extensions want to expose are only applicable to certain
kinds of content. Some examples include saving selected text or link urls to a
extension-local or web based notepad, removing particular images from a page,
filling in form fields, and looking up words in an online dictionary.

**Could this API be part of the web platform?**

Probably not, since many actions people would like to add are logically
"browser-level" functionality.

**Do you expect this API to be fairly stable?**

Yes, once we get some experience with an experimental version.

**What UI does this API expose?**

New menu items & submenus in context menus.

****How could this API be abused?****

Someone could create menu items with innocuous-sounding or misleading names (or
even duplicates of built-in menu items) to trick users to click on them.

**How would you implement your desired features if this API didn't exist?**

You could (and perhaps some extensions do?) inject script into all pages to
intercept onmousedown and create your own custom html menu implementation. But
that is a terrible user experience since it prevents the browser-native menu
from appearing.

**Draft API spec**

Initially these functions will only be callable from an extension's background
page. This will likely not be sufficient, since a frequent use case for context
menus is to operate on the specific link/image/etc. that the menu was activated
on. See the "Future Improvements" section below for ideas about this.

**Note: because this will be initially available as an experiment, the API
methods will at first be chrome.experimental.contextMenu.methodname**

**chrome.contextMenu.create**(object properties, function onCreatedCallback);

> Creates a new context menu item. The onCreatedCallback function should have a
> signature like function(id) {}, and will be passed the integer id of the newly
> created menu item.

> The properties parameter can contain:

> \* **title** (optional string) - Text for this menu item. This is required for
> all types except 'SEPARATOR'. The special string %s in a title will be
> replaced with the currently selected text, if any.

> \* **type** (optional string) - One of the strings 'NORMAL', 'CHECKBOX',
> 'RADIO', or 'SEPARATOR'. Defaults to 'NORMAL' if not specified.

> \* **checked** (optional boolean) - For items of type CHECKBOX or RADIO,
> should this be selected (RADIO) or checked (CHECKBOX)? Only one RADIO item can
> be selected at a time in a given group of RADIO items, with the last one to
> have checked == true winning.

> \* **contexts** (string) - Which contexts should this menu item appear for? An
> array of one or more of the following strings:

> > > 'ALL', 'PAGE', 'SELECTION', 'LINK', 'EDITABLE', 'IMAGE', 'VIDEO', 'AUDIO'.

> > Defaults to PAGE, which means "none of the rest" are selected (no link, no
> > image, etc.). If 'ALL' is in the array, the item will appear in all contexts
> > regardless of the rest of the items in the array.

> \* **enabledContexts** (string) - An array of strings similar to the contexts
> property. This serves to limit the contexts where this item will be enabled
> (ie not greyed out). If omitted, it defaults to the same set as the contexts
> property.

> \* **parentId** (integer) - If this should be a child item of another item,
> this is the id

> \* **onclick** (function(object info, Tab tab)) - A function that will be
> called when this menu item is clicked on. The info parameter is an object
> containing the following properties:

> > \* **menuItemId** (integer) - The id of the menu item that was clicked.

> > \* **parentMenuItemId** (optional integer) - The parent id, if any, for the
> > item clicked.

> > \* **mediaType** (optional string) - One of 'IMAGE', 'AUDIO', or 'VIDEO' if
> > the context item was brought up on one of these types of elements.

> > \* **linkUrl** (optional string) - If the element is a link, the url it
> > points to.

> > \* **srcUrl** (optional string) - Will be present for elements with a 'src'
> > url.

> > \* **pageUrl** (optional string) - The url of the page where the context
> > menu was clicked.

> > \* **frameUrl** (optional string) - The url of the frame of the element
> > where the context menu was clicked.

> > \* **selectionText** (optional string) - The text for the context selection,
> > if any.

> > \* **editable** (boolean) - A flag indicating whether the element is
> > editable (text input, textarea, etc.)

> > The tab parameter is the tab where the click happened, of the same form as
> > that returned by chrome.tabs.get.

**chrome.contextMenu.remove**(id);

> Removes an extension menu item with the given id.

**Examples**

​==Define the currently selected word(s)==

​The following code would live in your background page, and you would need to
have "tabs" specified in the permissions section of your manifest.

​function getDefinition(info, tab) {

if (!info.selectionText || info.selectionText.length == 0) {

return;

}

var maxLength = 1024;

var searchText = (info.selectionText.length &lt;= maxLength) ?

info.selectionText : info.selectionText.substr(0, maxLength);

var url = "http://www.google.com/search?q=define:" + escape(searchText);

chrome.tabs.create({"url": url});

}

chrome.contextMenu.create({"title": "Define '%s'", "onclick": getDefinition,

"contexts":\["SELECTION"\]});

​==Remove an image from a page==

The following code would live in your background page, and you would need to
have an entry in the permissions section of your manifest which allowed you to
run chrome.tabs.executeScript on the page where the image had its context menu
activated. This example highlights one of the limitations of the initial
proposal, which is that you don't have any way to determine the actual unique
node that was clicked on, so it removes any instances of the image from the
page.

chrome.contextMenu.create({"title": "Remove This Image", "contexts":
\["IMAGE"\],

"onclick": function(info, tab) {

var frameUrl = info.frameUrl ? info.frameUrl : "";

var code =

"var imgs = document.getElementsByTagName('img');" +

"var srcUrl = unescape('" + escape(info.srcUrl) + "');" +

"for (var i in imgs) {" +

" var img = imgs\[i\];" +

" if (img.src == srcUrl) {" +

" img.parentNode.removeChild(img);" +

" }" +

"}";

chrome.tabs.executeScript(tab.id, {"allFrames": true, "code": code});

}});

****UI Design****

**-Extension items appear towards the bottom of the context menu, above "Inspect
element", sorted by extension name.**

**-We will show at most one top-level menu item per extension. If an extension
adds more than 1 item, we automatically push the items into a submenu, with the
extension name as the top-level item.**

**-The extension's icon will appear to the left of the top-level item to help
the user understand which extension added what items, and help to mitigate the
spoofing concerns raised above.**

**[<img alt="image"
src="/developers/design-documents/extensions/proposed-changes/apis-under-development/context-menu-api/context_menu_api_4.png"
height=200
width=151>](/developers/design-documents/extensions/proposed-changes/apis-under-development/context-menu-api/context_menu_api_4.png)**

**Future Improvements**

- Provide a mechanism for getting a handle to the precise node in the DOM where
the menu was activated. This could mean one of the following ideas:

*   Allow registration of context menus from content scripts, with a
            callback function that receives a handle to the node.
*   Pass the id of the node in the callback to the background page, and
            if the node didn't have an id we synthesize one.
*   Provide something like chrome.tabs.executeScript, where you can pass
            a file or string of code to execute but also specify a function that
            will be called with the node

- Add an update(int id, object properties) method so you can change your menu
items in place.

- Add a removeAll() method so you don't have to keep track of id's if you don't
want to.

-Add a way to limit your menu items to specific tabs or frame urls (by a given
match pattern, or perhaps automatically limit to only sites you have content
script permissions for)

-Some people have asked for a onBeforeShow() event to fire before the menu items
are displayed, but this may be infeasible given chrome's multiprocess
architecture and desire to keep the UI very responsive.
