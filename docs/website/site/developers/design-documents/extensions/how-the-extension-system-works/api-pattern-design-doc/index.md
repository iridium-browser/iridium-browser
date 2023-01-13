---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/how-the-extension-system-works
  - How the Extension System Works
page_name: api-pattern-design-doc
title: APIs as stateless service calls
---

**Note:** This is the original proposal and is kept for historical reasons. It
may be made obsolete by more recent documentation.

---

We want to give extensions programmatic APIs to interact with many of the
browsers' subsystems. Some examples include: tabs, history, bookmarks,
downloads, and the omnibar.

An interesting wrinkle in Chrome is that in reality, these data structures exist
in another process. Because of this, it's impossible to keep a perfectly
consistent view of the world in the extension process. There will at time be
conflicts between what the extension thinks the current state of the system is,
and what it actually is. We need to design APIs that makes this fundamental
problem as easy to deal with as possible.

## Proposal

We could model these APIs as stateless service calls, similar to the way AJAX
applications work. Each call will be asynchronous, and just dumb data gets
passed back and forth across the void.

For example:

window.onload = populate;

function populate() {

extension.bookmarks.getAllBookmarks(function(results) {

// Assuming something MochiKit-ish is present.

forEach(results, function(bookmark) {

var div = DIV(A(bookmark.title, bookmark.url), " ", A("edit"));

div.id = "b" + result.id;

div.lastChild.onclick = bind(edit, result.id);

return div;

});

});

}

function edit(id) {

var newTitle = prompt("Enter new title");

extension.bookmarks.updateBookmark({id: id, title: newTitle});

}

The advantages of this approach:

*   Chrome doesn't have to send copies of browser state to any extension
            process until it is requested.
*   Chrome doesn't have to have complex machinery to keep multiple
            copies of browser state synchronized across processes.
*   The API reflects the underlying reality -- that the interaction with
            the browser is asynchronous.
*   Fits a wide variety of applications, so many APIs can follow a
            similar pattern, making new APIs easier for chrome-team to build and
            easier for developers to learn.
*   This feels like idiomatic JavaScript.

The disadvantages:

*   Not object-oriented. The verbs are at the top level and accept nouns
            as arguments. This could get ugly in cases where a subsystem has
            many nouns.
*   More coupled to JavaScript (bad if we ever wanted to make these APIs
            available to other languages).
*   May encourage us to be lazy with API design by trying to wedge
            everything into this CRUD idiom.

## Details

#### Conflicts

Without cross-process locking, conflicts can occur in any of our APIs. For
example, you can attempt to navigate a tab that has since been closed. My
feeling is that conflicts should \*not\* be treated as exceptions. For simple
things, we should just let the last change win. For more complex cases that make
no sense, we should print a warning to the console, but not treat it as a
fully-fledged error. In the limit, there is nothing the developer can really do
about these cases. We can propagate the real state back to the extension via
events and the extension can do something about it if it wants (I think in most
cases, it won't care).

#### Constraints

Many of these data models have constraints and side-effects. For example, it
doesn't make sense to set a window's zIndex negative, and changing the index of
a tab affects the index of other tabs in the same window. Constraints and
side-effects in this system would typically be handled by the server side and
propagated back to the client by events, if the extension developer had
registered to receive them.

#### Non-data APIs

Some APIs don't fit this model. For example, interacting with the omnibar or
receiving requests from content scripts. We'd just have to handle these on a
case-by-cases basis. I've outlined some ideas below.

## Alternative Ideas

The main alternative is to have a stateful DOM-style API, like this:

foreach(window.extensions.bookmarks, function(bookmark) {

var div = DIV(A(bookmark.title, bookmark.url), " ", A("edit"));

div.id = "b" + result.id;

div.lastChild.onclick = bind(edit, bookmark);

return div;

});

function edit(bookmark) {

var newTitle = prompt("Enter new title");

bookmark.title = newTitle;

bookmark.save();

}

The main downside of this approach is that it requires synchronizing the state
of the relevant object models between the extension and browser processes, and
resolving conflicts. Also, as in my example here, we still might want to have an
explicit update step if the operation would require writing to disk in the
browser process.

## API Hierarchy Proposal

chromium.

// An event is a way to register to be notified when something happens. In the
rest of this

// documentation, Events are referred to as first-class, with the following
notation:

//

// event name(Arg1Type arg1, Arg2Type arg2, ...);

//

// What this means is that the Event in variable "name" wants handlers with the
signature:

//

// void handler(Arg1Type arg1, Arg2Type arg2, ...)

class Event

void addListener(object callback(...))

void removeListener(object callback(...))

bool hasListener(object callback(...))

// Dispatches the event. Mainly this would be used by the framework, but it
might be useful

// to expose to developers.

void dispatch(...)

// A class that is essentially a client to an extension "server" process.
Provides ways to

// interact with the extension server, usually from a content script, but
theoretically from

// content or another extension.

class Extension

constructor(int id)

Port connect()

string getURL(string path)

// One end of a connection between an extension process and a content script (or
content, or

// another extension).

class Port

event onmessage

void postMessage(object args) // args is any json-compatible type

[browser.](/system/errors/NodeNotFound)

[downloads.](/developers/design-documents/extensions/proposed-changes/apis-under-development/downloads-api)

[bookmarks.](/system/errors/NodeNotFound)

[history.](/developers/design-documents/extensions/proposed-changes/apis-under-development/history-api)

// TODO:

thumbnails.

// I think these all fit the basic CRUD pattern.

[omnibox.](/developers/design-documents/extensions/proposed-changes/apis-under-development/omnibox-api)

// This one should be really fun to figure out.

// The "self" module is defined differently depending on whether you are inside
a content

// script or not.

// When in an extension, 'self' is a module, just like 'tabs', 'history', etc,
that

// provides access to some meta features.

self.

// For access to other parts of an extension.

DOMWindow background // Gets the single background process if there is one

DOMWindow\[\] toolstrips // Gets a list of live toolstrip instances

// Fired when somebody first creates a channel to this extension

Event onconnect(Port port)

// When in a content script, 'self' is an Extension instance referring to the
parent

// extension. Content scripts can use this to talk to their parent without
having to

// know its ID, which should be handy during development.

Extension self

## Examples

Move all google tabs into a new window:

chromium.tabs.getAllTabs(function(tabs) {

var tabsToMove = \[\];

tabs.forEach(function(tab) {

// Too bad the URL isn't a parsed representation :(

if (tab.url.indexOf("google.com")) {

tabsToMove.push(tab);

}

}

if (tabsToMove.length &gt; 0) {

chromium.tabs.createWindow(null, function(newWindow) {

tabsToMove.forEach(function(tab) {

chromium.tabs.updateTab({

id: tab.id,

windowId: newWindow.id

});

});

});

}

});

Write out navigation entries to console:

chromium.tabs.ontabchange.addListener(function(old, new) {

if (old.url != new.url) {

console.log("navigated to: " + newTab.url);

}

});

Communication between content scripts and extension:

// Content script

var port = chromium.self.connect();

port.onmessage.addListener(function(message) {

console.log("got message: " + message);

});

port.postMessage({pageUrl: location.href});

// Extension

chromium.self.onconnect.addListener(function(port) {

console.log("received message from: " + port.pageUrl);

port.postMessage("ack");

});
