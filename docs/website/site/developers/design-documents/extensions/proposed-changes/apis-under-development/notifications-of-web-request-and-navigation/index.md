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
page_name: notifications-of-web-request-and-navigation
title: Notifications of Web Request and Navigation
---

## Overview

This document describes the following API:

*   [chrome.webRequest](http://code.google.com/chrome/extensions/trunk/experimental.webRequest.html):
            Monitors events of HTTP requests.
*   [chrome.webNavigation](http://code.google.com/chrome/extensions/trunk/experimental.webNavigation.html):
            Monitors events of navigations happening in tab content windows as
            well as subframes (iframe / frame elements).

## Use cases

An API of detailed Web progress notifications can be used to log traffic data,
measure browser performance, etc. Please note that this requirement has been
listed in the [API Wish
List](/developers/design-documents/extensions/apiwishlist) (the Network item).

*   Configure proxy for each request.
*   Modify headers (e.g. change User-Agent:).
*   Block requests to certain servers.
*   Log traffic data and measure performance.

## Could this API be part of the web platform?

To the best of my knowledge, it could not.

## Do you expect this API to be fairly stable?

I think the API will be fairly stable, since the way how HTTP works, as well as
how the browser handles Web page navigations, is mature and stable. However, as
described in the Open questions section, there may be some improvements in the
future.

## What UI does this API expose?

The API doesn’t expose any UI.

## How could this API be abused?

There may be privacy issues since extensions can collect detailed information
about network traffic via this API.

## How would you implement your desired features if this API didn't exist?

Without this API,

*   I probably cannot monitor HTTP requests.
*   To monitor Web navigation events, I would use chrome.tabs.onUpdated,
            chrome.{tabs,windows}.onCreated and inject content scripts to
            monitor subframe navigations. But there are still things I cannot
            know about, e.g., when a navigation is about to occur, accurate
            timestamp of events, transition type of a navigation, etc.

## Draft API spec

### Manifest

In order to use chrome.webRequest or chrome.webNavigation, the "web_progress"
permission has to be declared in the permissions section of the extension
manifest.

Moreover, if you would like to receive extra information along with
chrome.webRequest.\* (please see the description of extraInfoSpec), you need to
add hosts (or host match patterns) to the permissions section as well. You can
only request extra information for URLs whose hosts are specified in the
permissions section. For example:

```none
{
    "name": "My extension",
    ...
    "permissions": [
        "web_progress",
        "http://www.google.com/"
    ],
    ...
}
// This isn’t allowed since it requests extra information for any URL.
chrome.webRequest.onBeforeRequest.addListener(
    function(object details) { ... }, null, ["requestHeaders"]);
// This is allowed.
chrome.webRequest.onBeforeRequest.addListener(
    function(object details) { ... }, {urls: ["http://www.google.com/foo*bar"]},
    ["requestHeaders"]);
// This is allowed too, since it doesn’t request any extra information.
chrome.webRequest.onBeforeRequest.addListener(
    function(object details) { ... }, {urls: ["http://www.youtube.com/*"]});
```

### chrome.webRequest

#### **Events**

#### onBeforeRequest

```none
chrome.webRequest.onBeforeRequest.addListener(
    function(object details) { ... }, RequestFilter filter,
    array of string extraInfoSpec); 
```

Fires when a request is about to occur. This is sent before any TCP connection
is made.

**Parameters**

***details (object)***

> ***requestId (integer)***

> > The ID of the request. Request IDs are unique within a browser session. As a
> > result, they could be used to relate different events of the same request.

> ***url (string)***

> ***method (string)***

> > Standard HTTP method.

> ***tabId (integer)***

> > The ID of the tab in which the request takes place. Set to null if the
> > request isn’t related to a tab.

> ***type (string)***

> > Please refer to the request types below for detailed explanation.

> ***timeStamp (Date)***

> > The time when the browser was about to make the request.

***filter (optional RequestFilter)***

> Please refer to RequestFilter in the Types section for detailed explanation.

***extraInfoSpec (optional array of string)***

> Extra information that needs to be passed into the callback function. Each
> element of extraInfoSpec requires a corresponding string property being added
> to the details object. Array elements can be:

    *   **"blocking"**

**#### onBeforeSendHeaders**

**```none
chrome.webRequest.onBeforeSendHeaders.addListener(
    function(object details) { ... }, RequestFilter filter,
    array of string extraInfoSpec); 
```**

**Fires when an HTTP request is about to occur. This may occur after a TCP connection is made to the server, but before any HTTP data is sent. The HTTP request headers are available at this point.**

****Parameters****

*****details (object)*****

> *****requestId (integer)*****

> > **The ID of the request. Request IDs are unique within a browser session. As
> > a result, they could be used to relate different events of the same
> > request.**

> *****url (string)*****

> *****timeStamp (Date)*****

> > **The time when the browser was about to make the request.**

*****filter (optional RequestFilter)*****

> **Please refer to RequestFilter in the Types section for detailed
> explanation.**

*****extraInfoSpec (optional array of string)*****

> **Extra information that needs to be passed into the callback function. Each
> element of extraInfoSpec requires a corresponding string property being added
> to the details object. Array elements can be:**

    *   ****"requestLine"****
    *   ****"requestHeaders"****
    *   ****"blocking"****

#### onSendHeaders

```none
chrome.webRequest.onSendHeaders.addListener(
    function(object details) { ... }, RequestFilter filter,
    array of string extraInfoSpec);
```

Fires just before the headers are sent to the server (after all users of the
webrequest api had a chance to modify the request in onBeforeSendHeaders).

**Parameters**

***details (object)***

> ***requestId (integer)***

> > The ID of the request.

> ***url (string)***

> ***timeStamp (Date)***

> > The time when the browser finished sending the request.

***filter (optional RequestFilter)***

> Please refer to RequestFilter in the Types section for detailed explanation.

***extraInfoSpec (optional array of string)***

> Extra information that needs to be passed into the callback function. Each
> element of extraInfoSpec requires a corresponding string property being added
> to the details object. Array elements can be:

    *   **"requestLine"**
    *   **"requestHeaders"**

#### onResponseStarted

```none
chrome.webRequest.onResponseStarted.addListener(
    function(object details) { ... }, RequestFilter filter,
    array of string extraInfoSpec);
```

Fires when the first byte of the response body is received. For HTTP requests,
this means that the status line and response headers are available.

**Parameters**

***details (object)***

> ***requestId (integer)***

> > The ID of the request.

> ***url (string)***

> ***ip (string)***

> > The server IP address that is actually connected to. Note that it may be a
> > literal IPv6 address.

> ***statusCode (integer)***

> > Standard HTTP status code returned by the server.

> ***timeStamp (Date)***

> > The time when the status line and response headers were received.

***filter (optional RequestFilter)***

> Please refer to RequestFilter in the Types section for detailed explanation.

***extraInfoSpec (optional array of string)***

> Extra information that needs to be passed into the callback function. Each
> element of extraInfoSpec requires a corresponding string property being added
> to the details object. Array elements can be:

    *   **"statusLine"**
    *   **"responseHeaders"**

#### onBeforeRedirect

```none
chrome.webRequest.onBeforeRedirect.addListener(
    function(object details) { ... }, RequestFilter filter,
    array of string extraInfoSpec); 
```

Fires when a server initiated redirect is about to occur.

**Parameters**

***details (object)***

> ***requestId (integer)***

> > The ID of the request.

> ***url (string)***

> > The URL of the current request.

> ***ip (string)***

> > The server IP address that is actually connected to. Note that it may be a
> > literal IPv6 address.

> ***statusCode (integer)***

> > Standard HTTP status code returned by the server.

> ***redirectUrl (string)***

> > The new URL.

> ***timeStamp (Date)***

> > The time when the browser was about to make the redirect.

***filter (optional RequestFilter)***

> Please refer to RequestFilter in the Types section for detailed explanation.

***extraInfoSpec (optional array of string)***

> Extra information that needs to be passed into the callback function. Each
> element of extraInfoSpec requires a corresponding string property being added
> to the details object. Array elements can be:

    *   **"statusLine"**
    *   **"responseHeaders"**

#### onCompleted

```none
chrome.webRequest.onCompleted.addListener(
    function(object details) { ... }, RequestFilter filter,
    array of string extraInfoSpec); 
```

Fires when a request is completed.

**Parameters**

***details (object)***

> ***requestId (integer)***

> > The ID of the request.

> ***url (string)***

> ***ip (string)***

> > The server IP address that is actually connected to. Note that it may be a
> > literal IPv6 address.

> ***statusCode (integer)***

> > Standard HTTP status code returned by the server.

> ***timeStamp (Date)***

> > The time when the response was received completely.

***filter (optional RequestFilter)***

> Please refer to RequestFilter in the Types section for detailed explanation.

***extraInfoSpec (optional array of string)***

> Extra information that needs to be passed into the callback function. Each
> element of extraInfoSpec requires a corresponding string property being added
> to the details object. Array elements can be:

    *   **"statusLine"**
    *   **"responseHeaders"**

#### onErrorOccurred

```none
chrome.webRequest.onErrorOccurred.addListener(
    function(object details) { ... }, RequestFilter filter);
```

Fires when an error occurs.

**Parameters**

***details (object)***

> ***requestId (integer)***

> > The ID of the request.

> ***url (string)***

> ***ip (string)***

> > The server IP address that is actually connected to. Note that it may be a
> > literal IPv6 address.

> ***error (string)***

> > The error description.

> ***timeStamp (Date)***

> > The time when the error occurred.

***filter (optional RequestFilter)***

> Please refer to RequestFilter in the Types section for detailed explanation.

#### **Event order**

For any successful request, events are fired in the following order:

```none
onBeforeRequest -> onBeforeSendHeaders[1] -> onSendHeaders ->
    (onBeforeRedirect -> onBeforeRequest -> onBeforeSendHeaders[1] -> onSendHeaders ->)* ->
    onResponseStarted -> onCompleted
```

\[1\] Note: onBeforeSendHeaders is only fired for HTTP requests. It will not be
fired for e.g. file:/// URLs.

The part enclosed in parentheses indicates that there may be zero or more server
initiated redirects.

Any error that occurs during the process results in an onErrorOccurred event.
For a specific request, there is no further events fired after onErrorOccurred.

### chrome.webNavigation

#### **Events**

#### onBeforeNavigate

```none
chrome.webNavigation.onBeforeNavigate.addListener(function(object details) { ... }); 
```

Fires when a navigation is about to occur.

**Parameters**

***details (object)***

> ***tabId (integer)***

> > The ID of the tab in which the navigation is about to occur.

> ***url (string)***

> ***frameId (integer)***

> > 0 indicates the navigation happens in the tab content window; positive value
> > indicates navigation in a subframe. Frame IDs are unique within a tab.

> ***requestId (integer)***

> > The ID of the request to retrieve the document of this navigation. Note that
> > this event is fired prior to the corresponding
> > chrome.webRequest.onBeforeRequest.

> ***timeStamp (Date)***

> > The time when the browser was about to start the navigation.

#### onCommitted

```none
chrome.webNavigation.onCommitted.addListener(function(object details) { ... });
```

Fires when a navigation is committed. The document (and the resources it refers
to, such as images and subframes) might still be downloading, but at least part
of the document has been received from the server and the browser has decided to
switch to the new document.

**Parameters**

***details (object)***

> ***tabId (integer)***

> > The ID of the tab in which the navigation occurs.

> ***url (string)***

> ***frameId (integer)***

> > 0 indicates the navigation happens in the tab content window; positive value
> > indicates navigation in a subframe.

> ***transitionType (string)***

> > Cause of the navigation. Use the same transition types as those defined for
> > [chrome.history
> > API](http://code.google.com/chrome/extensions/history.html#transition_types).

> ***transitionQualifiers (string)***

> Zero or more transition qualifiers delimited by "|". Please refer to the
> qualifier values below for detailed explanation.

> ***timeStamp (Date)***

> > The time when the navigation was committed.

#### onDOMContentLoaded

```none
chrome.webNavigation.onDOMContentLoaded.addListener(function(object details) { ... });
```

Fires when the page’s DOM is fully constructed, but the referenced resources may
not finish loading.

**Parameters**

***details (object)***

> ***tabId (integer)***

> > The ID of the tab in which the navigation occurs.

> ***url (string)***

> ***frameId (integer)***

> > 0 indicates the navigation happens in the tab content window; positive value
> > indicates navigation in a subframe.

> ***timeStamp (Date)***

> > The time when the page’s DOM was fully constructed.

#### onCompleted

```none
chrome.webNavigation.onCompleted.addListener(function(object details) { ... });
```

Fires when a document, including the resources it refers to, is completely
loaded and initialized.

**Parameters**

***details (object)***

> ***tabId (integer)***

> > The ID of the tab in which the navigation occurs.

> ***url (string)***

> ***frameId (integer)***

> > 0 indicates the navigation happens in the tab content window; positive value
> > indicates navigation in a subframe.

> ***timeStamp (Date)***

> > The time when the document finished loading.

#### onErrorOccurred

```none
chrome.webNavigation.onErrorOccurred.addListener(function(object details) { ... });
```

Fires when an error occurs.

**Parameters**

***details (object)***

> ***tabId (integer)***

> > The ID of the tab in which the navigation occurs.

> ***url (string)***

> ***frameId (integer)***

> > 0 indicates the navigation happens in the tab content window; positive value
> > indicates navigation in a subframe.

> ***error (string)***

> > The error description.

> ***timeStamp (Date)***

> > The time when the error occurred.

#### onBeforeRetarget

```none
chrome.webNavigation.onBeforeRetarget.addListener(function(object details) { ... });
```

Fires when a new window, or a new tab in an existing window, is about to be
created to host a navigation.

**Parameters**

***details (object)***

> ***sourceTabId (integer)***

> > The ID of the tab in which the navigation is triggered.

> ***sourceUrl (string)***

> > The URL of the document that is opening the new window.

> ***targetUrl (string)***

> > The URL to be opened in the new window.

> ***timeStamp (Date)***

> > The time when the browser was about to create a new view.

#### **Event order**

For a navigation that is successfully completed, events are fired in the
following order:

```none
onBeforeNavigate -> onCommitted -> onDOMContentLoaded -> onCompleted 
```

Any error that occurs during the process results in an onErrorOccurred event.
For a specific navigation, there is no further events fired after
onErrorOccurred.

Note that it is possible that a new navigation occurs in the same frame before
the former navigation completes. As a result, even if a navigation proceeds
successfully, the event sequence doesn’t necessarily end with an onCompleted.

If a navigating frame contains subframes, its onCommitted is fired before any of
its children’s onBeforeNavigate; while its onCompleted is fired after any of its
children’s onCompleted.

### Types

#### Request types

How the requested resource will be used.

*   **"main_frame"**: The resource is the document of a tab content
            window.
*   **"sub_frame"**: The resource is the document of a subframe (iframe
            or frame).
*   **"stylesheet"**: The resource is a CSS stylesheet.
*   **"script"**: The resource is an external script.
*   **"image"**: The resource is an image (jpg/gif/png/etc).
*   **"object"**: The resource is an object (or embed) tag for a plugin,
            or a resource that a plugin requested.
*   **"other"**: Other kinds of resource.

#### Transition qualifier values

Any transition type can be augmented by qualifiers, which further define the
navigation.

*   **"client_redirect"**: Redirects caused by JavaScript or a meta
            refresh tag on the page.
*   **"server_redirect"**: Redirects sent from the server by HTTP
            headers.
*   **"forward_back"**: Users use Forward or Back button to navigate
            among browsing history.

#### RequestFilter

***(object)***

> ***urls (optional array of string)***

> > Each element is a URL or URL pattern. Please refer to [content script match
> > patterns](http://code.google.com/chrome/extensions/dev/match_patterns.html)
> > for URL pattern definition. Requests that cannot match any of the URLs will
> > be filtered out.

> ***types (optional array of string)***

> > Each element is a request type described above. Requests that cannot match
> > any of the types will be filtered out.

> ***tabId (optional integer)***

> > The ID of the tab in which requests takes place.

> ***windowId (optional integer)***

> > The ID of the window in which requests takes place.

## Open questions

### Accessing request and response bodies from extensions?

We could support extensions to specify "requestBody" element in the
extraInfoSpec array of chrome.webRequest.onBeforeRequest (or onRequestSent), so
that an extra property, requestBody, will be added to the details object passed
into the event handler. Similarly, we could support extensions to ask for
responseBody, too.

Both requestBody and responseBody are EntityBody objects.

#### EntityBody

#### **Methods**

```none
readBytes(integer length, boolean wait, function callback) 
```

**Parameters**

***length (integer)***

> How many bytes to read.

***wait (boolean)***

> If wait is set to false, the browser will respond as soon as possible,
> returning the bytes it has got currently; otherwise, it won’t invoke the
> callback function until it collects enough bytes or reaches the end of the
> entity body.

***callback (function)***

**Callback function**

It should specify a function that looks like this:

```none
function (integer actualLength, string base64EncodedData)
```

***actualLength (integer)***

> The actual number of bytes that are returned. -1 indicates the end of the
> entity body has been reached or error occurred. Please note that 0 doesn’t
> indicate the end of the entity body, if the wait parameter of readBytes() is
> set to false.

***base64EncodedData (string)***

> Base64 encoded bytes. Please note that normally the string length of
> base64EncodedData is different from actualLength.

Some issues need to be considered:

*   An important issue is how to manage the lifetime of request bodies
            and response bodies properly.

Please consider the following scenario: an extension requests responseBody to be
passed into the handler of chrome.webRequest.onCompleted. In the handler, it
calls responseBody.readBytes() to read the content. However, since events are
asynchronous, the browser fires onCompleted and proceeds immediately. If it
finishes using the response body and discards it, the call of
responseBody.readBytes() at the extension side may fail, depending on the
timing. This is undesired.
As a result, the browser has to properly manage the lifetime of request bodies
and response bodies. One possible approach is to keep track of references to
each EntityBody object (either requestBody or responseBody). An entity body
maybe be accessed by several EntityBody objects, which are passed to different
extensions. If all these EntityBody objects are no longer referenced, the
browser knows that the entity body is no long needed by any extension, and it is
safe to remove it.

*   It may be inconvenient to use an EntityBody object directly. We
            could also provide helper classes. For example, define
            TextEntityBody class to convert entity body to a text stream:

```none
// entityBody is an EntityBody object, which deals with bytes.
var textEntityBody = new TextEntityBody(entityBody, ‘UTF-8’);
// textEntityBody calls entityBody.readBytes() and converts the result to text.
textEntityBody.readText(10, true, function (text, reachEnd) {
  // Handle text directly.
});
```

### Synchronous (blocking) notifications?

The API proposed above is supposed to be asynchronous (non-blocking)
notifications. This is consistent with other chrome.\* APIs. More importantly,
notification publishers / consumers live in different processes. If we block the
publisher side for sending each notifications, it may hurt browser performance.

However, in some cases we would like to affect the control flow in event
handlers. (Please note that this has been listed as a requirement on the [API
Wish List](/developers/design-documents/extensions/apiwishlist), the Navigation
Interception item.)

Consider the following use case: in the callback function of
chrome.webRequest.onBeforeRequest we may want to cancel the request or modify
the request headers. If the notification is asynchronous, the browser fires the
notification and immediately makes the request. There is no way for extensions
to make a change.

One possible solution is to design some functions like "installSyncOnXyzHandler"
that installs a short snippet of JavaScript to run synchronously within the
browser process for the notifications that may need synchronous responses.
However, this may not fit with the overall Chrome architecture where we want to
avoid running javascript in the browser process; and the JavaScript snippet in
the browser process might need to communicate back to the extension for some of
its decisions as well.

Another feasible solution is to add support for browser to wait for a response
from an extension. Take chrome.webRequest.onBeforeRequest as an example, we
could reform it the extraInfoSpec parameter to accept an optional string
**"blocking".** If present, then the browser needs to be blocked and wait for a
response from the callback function.

If blocking is present, the callback function could optionally return an object
to cancel or modify the request:

**result (optional object)**

> ***requestLine (optional string)***

> > Modified request line.

> ***reqeustHeaders (optional string)***

> > Modified request headers.

> ***cancel (optional boolean)***

> > Set to true to cancel the request.

> ***redirectUrl (optional string)***

> > Redirects the request to a different URL (only available in onBeforeRequest
> > and onBeforeRedirect).

The biggest concern is the impact on browser performance. We have to measure the
performance impact very carefully if we really want to make the move.

## References

*   [DWebBrowserEvents2 Interface of
            IE](http://msdn.microsoft.com/en-us/library/aa768283%28VS.85%29.aspx)
*   [WinINet Status
            Callback](http://msdn.microsoft.com/en-us/library/aa385121%28v=VS.85%29.aspx)
*   [nsIWebProgressListener Interface of
            Firefox](https://developer.mozilla.org/en/nsIWebProgressListener)
