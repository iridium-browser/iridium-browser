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
page_name: offscreen-tabs
title: Offscreen Tabs (experimental)
---

[TOC]

## Introduction

The chrome.experimental.offscreenTabs.\* API module allows for interacting with
multiple web pages on the screen at the same time. Example applications might
look like the [Safari’s “Top
Sites”](http://decoding.files.wordpress.com/2009/02/safari_40_beta_1.jpg) or
[Chrome’s “Most
Visited”](http://www.revthatup.com/wp-content/uploads/2011/04/chrome.jpg) page
augmented with real-time display updates and user interaction. In combination
with WebGL and its texture API capabilities the offscreen tabs module could be
used to create true 3D browser environments on the web!

### Display

The real-time display could be achieved by updating standard HTML &lt;img&gt; or
&lt;canvas&gt; elements while capturing snapshots of offscreen tabs with the
***toDataUrl*** method.

### Interaction

For interaction, we propose the ***sendMouse*** and ***sendKeyboard*** methods.
These methods could pass standard JavaScript mouse and keyboard events along
with the offscreen tab ID specifying the offscreen tab these events should be
applied to. In the case of mouse events appropriate x and y coordinates should
be passed as well. Why? Because when an application or extension is responsible
for rendering an offscreen tab's contents, only that application knows exactly
where a click against the tab's canvas is supposed to be dispatched. Once Chrome
picks up the JavaScript events new synthesized events will be sent to the
appropriate offscreen tab. Most of the fields of the synthesized events will be
the same as the ones of the incoming JavaScript events. One exception, for
example, are the x and y coordinates for mouse events as explained above.

### Window Size

Since the offscreen tabs will not be displayed by the browser in separate tabs
the developer has to explicitly specify width and height of each offscreen tab's
window.

## Manifest

The module requires the specification of an *offscreenTabs* permission type.
This permission would enable the API to see all web sites' user data. The
developer will have to declare the *offscreenTabs* permission in the [extension
manifest](http://code.google.com/chrome/extensions/manifest.html). For example:
**{**
**"name": "My extension",**
**...**
**"permissions": \[**
**"offscreenTabs"**
**\],**
**...**
**}**

## Example Extensions

Above you can see a video of a Chrome extension we have developed with the
Offscreen Tab API and WebGL. Here are some additional screenshots:

[<img alt="image"
src="/developers/design-documents/extensions/proposed-changes/apis-under-development/offscreen-tabs/book_screenshot.png"
height=220
width=400>](/developers/design-documents/extensions/proposed-changes/apis-under-development/offscreen-tabs/book_screenshot.png)

[<img alt="image"
src="/developers/design-documents/extensions/proposed-changes/apis-under-development/offscreen-tabs/coverflow_screenshot.png"
height=211
width=400>](/developers/design-documents/extensions/proposed-changes/apis-under-development/offscreen-tabs/coverflow_screenshot.png)

## **Design Review Slides**

[Slides from API design
review](https://docs.google.com/a/chromium.org/present/edit?id=dcz7497p_9gzwfsfgs)

## Spec (outdated)

Note: the information below is outdated. The current API is in the process of
being checked in [here](http://codereview.chromium.org/7720002/).

### Types

#### OffscreenTab

***id (int)***
The ID of the offscreen tab. Tab IDs are unique within a browser session.
***url (string)***
The URL the offscreen tab is displaying.
***width (int)***
Width of the window.
***height (int)***
Height of the window.

### Methods

#### create

create (object createProperties, function callback)
Creates a new offscreen tab.
**Parameters**
**createProperties (object)**
***url (string)***
The URL the offscreen tab should be displaying.
***width (optional int)***
Width of the visible window. Defaults to width of the current window.
***height (****optional** **int)***
Height of the visible window. Defaults to height of the current window.
***callback (****optional** **function)***
The callback parameter should specify a function that looks like this:
function (OffscreenTab offscreenTab) {...};
Details of the offscreen tab.

#### get

get (int offscreenTabId, function callback)
Gets details about the specified offscreen tabs.
**Parameters**
*offscreenTabId (int)***
ID of the offscreen tab.
***callback (function)***
The callback parameter should specify a function that looks like this:
function (OffscreenTab offscreenTab) {...};
Details of the offscreen tab.

#### getAll

getAll (function callback)
Gets an array of all offscreen tabs.
**Parameters**
***callback (function)***
The callback parameter should specify a function that looks like this:
function (array of OffscreenTab offscreenTabs) {...};

#### remove

remove (int offscreenTabId, function callback)
Removes an offscreen tab.
**Parameters**
***offscreenTabId (int)***
ID of the offscreen tab.
***callback (****optional** **function)***
The callback parameter should specify a function that looks like this:
function () {...};

#### sendMouseEvent

sendMouseEvent (int offscreenTabId, int x, int y, MouseEvent mouseEvent,
function callback)
Dispatches a mouse event in the offscreen tab.
**Parameters**
***offscreenTabId (int)***
ID of the offscreen tab.
***mouseEvent (MouseEvent)***
A JavaScript MouseEvent object. Supported event types: *mousedown*, *mouseup*,
*click*, *mousemove*, *mousewheel*.
***x (****optional*** ***int)***
X position of where the mouse event should be dispatched on the offscreen web
page. Not required in the case of a *mousewheel* event.
***y (****optional*** ***int)***
Y position of where the mouse event should be dispatched on the offscreen web
page. Not required in the case of a *mousewheel* event.
***callback (****optional** **function)***
The callback parameter should specify a function that looks like this:
function (OffscreenTab offscreenTab) {...};
Details of the offscreen tab.

#### sendKeyboardEvent

sendKeyboardEvent (int offscreenTabId, KeyboardEvent event, function callback)
Dispatches a keyboard event in the offscreen tab.
**Parameters**
***offscreenTabId (int)***
ID of the offscreen tab.
***keyboardEvent (KeyboardEvent)***
A JavaScript KeyboardEvent object. Supported event types: *keydown*, *keyup*,
*keypress*. Only *keypress* produces a visible result on screen.
***callback (****optional** **function)***
The callback parameter should specify a function that looks like this:
function (OffscreenTab offscreenTab) {...};
Details of the offscreen tab.

#### toDataUrl

toDataUrl (int offscreenTabId, object options, function callback)
Captures the visible area of an offscreen tab.
**Parameters**
***offscreenTabId (int)***
The ID of the offscreen tab.
***options (****optional** **object)***
Set parameters of image capture, such as the format of the resulting image.
***format (****optional** **enumerated string \[“jpeg”, “png”\])***
The format of the resulting image. Default is jpeg.
***quality (****optional** **int)***
When format is 'jpeg', controls the quality of the resulting image. This value
is ignored for PNG images. As quality is decreased, the resulting image will
have more visual artifacts, and the number of bytes needed to store it will
decrease.
***callback (****function)***
The callback parameter should specify a function that looks like this:
function (string dataUrl) {...};
***dataUrl (string)***
A data URL which encodes an image of the visible area of the captured offscreen
tab. May be assigned to the 'src' property of an HTML Image element for display.

#### update

update (int offscreenTabId, object updateProperties, function callback)
Modifies the properties of an offscreen tab. Properties that are not specified
in updateProperties are not modified.
**Parameters**
***offscreenTabId (int)***
The ID of the offscreen tab.
***updateProperties (object)***
***url (****optional** **string)***
The URL the offscreen tab is displaying.
***width (****optional** **int)***
Width of the window.
***height (****optional** **int)***
Height of the window.
***callback (****optional** **function)***
The callback parameter should specify a function that looks like this:
function (OffscreenTab offscreenTab) {...};
Details of the offscreen tab.

### Events

#### onUpdated

onUpdated.addListener (function (int offscreenTabId, object chnageInfo,
OffscreenTab offscreenTab) {...})
Fires when an offscreen tab is updated.
**Parameters**
***offscreenTabId (int)***
ID of the updated offscreen tab
***changeInfo (object)***
Lists the changes to the state of the offscreen tab that was updated.
***url (****optional** **string)***
The offscreen tab’s URL if it has changed.
***offscreenTab (OffscreenTab)***
Details of the offscreen tab
