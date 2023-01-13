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
page_name: gleam-api
title: Gleam API
---

## Overview

Extension developers often want a way to offer contextual actions with different
parts of a web page. For example:

*   The Skype extension would like to offer to make calls to phone
            numbers in the text of a page
*   Many extensions would like to offer to download or edit embedded
            images and video
*   Annotation extensions would like to offer to act on the current
            selection, and to highlight passages of text in a web page

The Gleam API would allow extension developers to do these things, in a way
that:

*   Cannot slow down page load
*   Cannot break the rendering of web pages
*   Simplifies handling all the tricky edge cases in dynamic pages that
            add, edit, and remove nodes at runtime
*   Offers a nice, consistent UI that users will understand and that
            allows multiple extensions to interact nicely without conflict

## Status

Proposal

## Details

Extensions can register interest in several different types of objects:

*   Images
*   Plugins
*   Video (the &lt;video&gt; tag)
*   Links
*   Text
*   Selection

Gleams are registered in the manifest, and multiple gleams can be registered
per-extension. If an extension is only interested in a subset of objects of a
given type on the page, they can be filtered with optional filter declarations
in the manifest or a filter function.

Once an object has been selected by at least one extension, it is visually
highlighted by Chrome. For most types, the available actions wouldn't be visible
on an object by default, only a highlight indicating that some actions are
available. When the user hovers over the object, buttons for the available
actions are rendered. When a Gleam is invoked, a callback is fired in the
extension with details of the invoked Gleam and the relevant object.

&lt;todo: need mocks!&gt;

## Manifest

{

...

"gleam": \[

{

"type": "image" | "plugin" | "video" | "link" | "text" | "selection",

"id": &lt;string&gt;, /\* a unique ID (within this extension) for the gleam \*/

"name": &lt;string&gt;, /\* the name to display in the UI \*/

"icon": &lt;path&gt;, /\* A path to an icon to display in the UI \*/

"regexp": &lt;string&gt;, /\* optional, only valid for type "text". a regexp
that filters text objects before invoking the filter function \*/

"content-types": &lt;string\[\]&gt; /\* optional, only valid for types "image",
"plugin", and "video". an array of content-types to filter to \*/

"filter": &lt;string&gt;, /\* optional, an expression that evaluates to a
function to use to filter objects to select \*/

"callback": &lt;string&gt; /\* an expression that evaluates to a function to
call when the gleam is invoked \*/

}

\]

...

}

## Filters

Each page action can specify an optional filter. The filter is a string that
should evaluate to a JavaScript function. The function is called for each object
found that matches the specified type and any applicable declarative filters.

The filter callback receives an object with the following properties:

String id // The ID of the gleam that a matching object was found for.

String documentUrl // The URL of the document the object was found on.

int tabId // The ID of the tab the object was found on.

String contentType // The content type of the matching object. Only valid for
image, video, and plugin.

String url // The URL of the object. Only valid for image, video, and link

String text // The text of the object. Only valid for text and link.

int width // The width of the object, if known. Only valid for image, video, and
plugin

int height // The height of the object, if known. Only valid for image, video,
and plugin

For all types except text, the filter callback should return a boolean
indicating whether the object should be selected. For type text, the filter
callback should return an array of begin/end pairs indicating the ranges inside
the text that should be selected.

Note that extension developers may choose to never select any objects if they
are only interested in knowing about certain types of objects on the page, not
performing actions on them.

## Callbacks

Each action must specify a callback. The callback is a string that should
evaluate to a JavaScript function. The function is called when the gleam is
invoked by the user. The parameters to the callback are the same as the
parameters to the filter (see above), and there is no return value.

## Extra Details

One of the neat things about this API is that we describe "objects" at a higher
level than DOM nodes. So Chrome gets to decide how to translate the DOM into
these "objects". We have an opportunity to really reduce complexity for
developers by doing these translations smartly.

*   For type "image", we shouldn't send images that are very small, or
            off screen. These are layout-related, most likely, and not useful to
            users.
*   For type "text", we should construct a text string for entire
            paragraphs at a time, removing all inline formatting tags, links,
            etc. This is very difficult to do correctly and we can save
            developer lots of work and bugginess by doing it for them. When the
            extension returns ranges, we translate these back into the DOM tree.
            This is hard, but we have all the data necessary to do it. Extension
            developers don't because it isn't surfaced through DOM APIs. This
            also means that we need to track tree changes so that we can
            invalidate filters requests that are outstanding.
*   We could add other high-level objects in the future, like
            "phone-number" and "address". (thanks erikkay for the idea!)
