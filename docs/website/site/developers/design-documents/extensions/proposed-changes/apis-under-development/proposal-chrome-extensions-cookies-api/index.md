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
page_name: proposal-chrome-extensions-cookies-api
title: Cookies API
---

### Overview

The proposed API allows Chrome extensions to access the browser's cookies,
without having to send HTTP requests to obtain those cookies.

### Use Cases

We are developing a Chrome extension that interacts with known services
requiring authentication. We would like the extension to be able to detect
authentication status changes as they occur in an inexpensive way, by checking
for the existence of known cookies associated with that service.
In general, any Chrome extension wishing to interact with services that set
browser cookies could benefit from this API. The API could also support Chrome
extensions which don't care about specific known cookies, but can for instance
help the user gain more visibility or control into what cookies are being set on
their browser.

### Could this API be part of the web platform?

No. The web platform relies on the same origin model, and sites can read their
own cookies. This API is for elevated privilege entities (i.e. extensions) to
read and write cookies across origins.

### Do you expect this API to be fairly stable?

Yes. The general mechanism of browser cookies is mature and likely stable,
making this API equally stable.

### What UI does this API expose?

None.

### How could this API be abused?

A malicious extension could steal cookies from sites the user visits. The API
also exposes HttpOnly cookie data to the extension. However, as documented
above, it is already possible to obtain cookies and to generate HTTP requests
from a Chrome extension by using content scripts, so these new APIs don't expose
any new vulnerabilities.

### How would you implement your desired features if this API didn't exist?

One technique is to use content scripts: loading a URL matching a particular
cookie's domain, and inspecting the value of document.cookie in a content
script. Another method could involve sending an XmlHttpRequest to a service that
will return authentication status information for a particular resource. Both of
these techniques require network connectivity and communication, making them
undesirable in a real-world application. Loading extra URLs in new windows or
hidden iframes in order to access cookies via content scripts poses further
potential security, privacy and usability risks.

### Are you willing and able to develop and maintain this API?

Yes.

### ******Draft API spec******

### ************Use the chrome.experimental.cookies module to access cookies from your extension.************

### ************#### ************Manifest************************

### ************You must declare the "cookies" permission in your extension's manifest to use the cookies API. You also must request cross-origin permissions for the domains you wish to access cookies for, using the same match-pattern syntax used for cross-origin XHR permissions. For example:************

### ************`{`************
### ************` "name": "My extension",`************
### ************` ...`************
### ************` "permissions": [`************
### ************` "cookies",`************
### ************` "http://*.google.com/"`************
### ************` ],`************
### ************` ...`************
### ************`}`************

#### ************Methods************

### ************get************

### ************`chrome.experimental.cookies.get(object details, function callback)`************

### ************Retrieves information about a single cookie. If more than one cookie of the same name exists for the given URL, the one with the longest domain property, longest path length, or earliest creation time will be returned.************

> ### ************#### ************Parameters************************

> ### *************details (object)*************

> > ### *************************Details to identify the cookie being retrieved.*************************

> > ### *************url (string)*************

> > > ### ************The URL with which the cookie to retrieve is associated. This argument may be a full URL, in which case any data following the URL path (e.g. the query string) is simply ignored. If host permissions for this URL are not specified in the manifest file, the API call will fail.************

> > *************name (string)*************

> > > ### **************************************The name of the cookie to retrieve.**************************************

> > ### *************storeId (optional string)*************

> > > ### ************The ID of the cookie store in which to look for the cookie. By default, the current execution context's cookie store will be used.************

> ### *************************callback (function)*************************

> #### ************************************Callback Function************************************

> > #### ************************************************The callback parameter should specify a function that looks like this:************************************************

> > #### ************************************************************`function(Cookie cookie) {...});`************************************************************

> > ### *************cookie (optional Cookie)*************

> > > ### *************************Contains details about the cookie. This parameter is null if no such cookie was found.*************************

### **************getAll**************

### ************`chrome.experimental.cookies.getAll(object `************

### ************``************``### ************`details`************``************``************

### ************``**********`, function callback)`************``************

### Retrieves all cookies from a single cookie store that match the given information.

> #### ************Parameters************

> #### *************************details (object)*************************

> > #### *************************************Information to filter the cookies being retrieved.*************************************

> > ### *************url (optional string)*************

> > > ### Restricts the retrieved cookies to those that would match the given URL.

> > ### *### *************name (optional string)**************

> > > ### *### Filters the cookies by name.*

> > ### *### *************domain (optional string)**************

> > > ### *### Restricts the retrieved cookies to those whose domains match or are subdomains of this one.*

> > ### *### *************path (optional string)**************

> > > ### *### Restricts the retrieved cookies to those whose path exactly matches this string.*

> > ### *### *************secure (optional boolean)**************

> > > ### *### Filters the cookies by their Secure property.*

> > ### *### *************session (optional**************

> > ### *### **************### **************### *************boolean******************************************

> > ### *### *************### *************)****************************

> > > ### *### Filters out session vs. persistent cookies.*

> > ### *### *### **************************storeId (optional****************************

> > ### *### *### ***************************### *### ***************************### *************string**********************************************************************

> > ### *### *### **************************### *### **************************)********************************************************

> > > ### ### **************************The cookie store to retrieve cookies from. If omitted, the current execution context's cookie store will be used.**************************

> ### *callback (function)*

> #### Callback Function

> > #### The callback parameter should specify a function that looks like this:

> > #### `function(array of Cookie cookies) {...});`

> > #### `*cookies (array of Cookie)*`

> > > #### `*All the existing, unexpired cookies that match the given cookie info.*`

****### set****

****### *******************************`chrome.experimental.cookies.set(object
details)`***********************************

****### #### `*********************Sets a cookie with the given cookie data; may overwrite equivalent cookies if they exist.*********************`****

> ****#### ************Parameters****************

> ****#### *details (object)*****

> > ****#### Details about the cookie being set.****

> > ********### *************url (string)*********************

> > > ********### The request-URI to associate with the setting of the cookie. This value can affect the default domain and path values of the created cookie. If host permissions for this URL are not specified in the manifest file, the API call will fail.********

> > *******### *************name (optional string)*********************

> > > *******### The name of the cookie. Empty by default if omitted.********

> > *******### *************value (optional string)*********************

> > > *******### The value of the cookie. Empty by default if omitted.********

> > *******### *************domain (optional string)*********************

> > > *******### The domain of the cookie. If omitted, the cookie becomes a host-only cookie.********

> > *******### *************path (optional string)*********************

> > > *******### The path of the cookie. Defaults to the path portion of the url parameter.********

> > *******### *************secure (optional boolean)*********************

> > > *******### Whether the cookie should be marked as Secure. Defaults to false.********

> > *******### *************httpOnly (optional boolean)*********************

> > > *******### Whether the cookie should be marked as HttpOnly. Defaults to false.********

> > *******### *************expirationDate (optional number)*********************

> > > *******### The expiration date of the cookie as the number of seconds since the UNIX epoch. If omitted, the cookie becomes a session cookie.********

> > ****### *************storeId (optional string)*****************

> > > ***### The ID of the cookie store in which to set the cookie. By default, the cookie is set in the current execution context's cookie store.****

******remove******

****### ****### ****###
*******************************`chrome.experimental.cookies.remove(object
details)`*******************************************

****### ****### ****### *******************************`****#### *Deletes a cookie by name.*****`*******************************************

> ****#### **### ### ### `#### #### Parameters`******

> ****#### *details (object)*****

> > ****#### Information to identify the cookie to remove.****

> > ****#### ****### *************url (string)*********************

> > > ****### The URL associated with the cookie. If host permissions for this URL are not specified in the manifest file, the API call will fail.****

> > ********### ****name (string)************

> > > ****### The name of the cookie to remove.****

> > ****### storeId (optional string)****

> > > ****The ID of the cookie store to look in for the cookie. If unspecified,
> > > the cookie is looked for by default in the current execution context's
> > > cookie store.****

****### ********************### ********************### ****### getAllCookieStores************************************************

****### ****### ****###
*******************************`chrome.experimental.cookies.getAllCookieStores(function
callback)`*******************************************

****### ****### ****### *******************************`****#### *Lists all existing cookie stores.*****`*******************************************

> ****#### **### ### ### `#### #### Parameters`******

> ****#### *callback (function)*****

> ****#### **#### Callback Function******

> > ****#### The callback parameter should specify a function that looks like this:****

> > ****#### `function(array of CookieStore cookieStores) {...});`****

> > ****#### cookieStores (array of CookieStore)****

> > > ****#### All the existing cookie stores.****

#### **#### Events**

#### **#### ****#### onChanged******

#### **#### ****#### `chrome.experimental.cookies.onChanged.addListener(function(object changeInfo) {...});`******

#### **#### ****#### `Fired when a cookie is set or removed.`******

> #### Parameters

> #### **#### ****#### `changeInfo (object)`******

> > #### **#### ****#### `removed (boolean)`******

> > > #### **#### ****#### `True if a cookie was removed.`******

> > #### **#### ****#### `cookie (Cookie) `******

> > > #### **#### ****#### `Information about the cookie that was set or removed.`******

#### Types

#### **#### ****#### ``**#### ****#### `Cookie (object)`******``******

#### **#### ``#### **#### `Represents information about an HTTP cookie.`**``**

> #### **#### ****#### ``**#### ****#### `name (string)`******``******

> > #### **#### ****#### ``**#### ****#### `The name of the cookie.`******``******

> #### **#### ****#### ``**#### ****#### `value (string)`******``******

> > #### **#### ****#### ``**#### ****#### `The value of the cookie.`******``******

> #### **#### ****#### ``**#### ****#### `domain (string)`******``******

> > #### **#### ****#### ``**#### ****#### `The domain of the cookie.`******``******

> #### **#### ****#### ``**#### ****#### `hostOnly (boolean) `******``******

> > #### **#### ****#### ``**#### ****#### `True if the cookie is a host-only cookie (i.e. a request's host must exactly match the domain of the cookie).`******``******

> #### **#### ****#### ``**#### ****#### `path (string)`******``******

> > #### **#### ****#### ``**#### ****#### `The path of the cookie.`******``******

> #### **#### ****#### ``**#### ****#### `secure (boolean)`******``******

> > #### **#### ****#### ``**#### ****#### `True if the cookie is marked as Secure (i.e. its scope is limited to secure channels, typically HTTPS).`******``******

> #### **#### ****#### ``**#### ****#### `httpOnly (boolean)`******``******

> > #### **#### ****#### ``**#### ****#### `True if the cookie is marked as HttpOnly (i.e. the cookie is inaccessible to client-side scripts).`******``******

> *session (boolean)*

> > #### #### #### ``#### #### `True if the cookie is a session cookie, as opposed to a persistent cookie with an expiration date.` ``

> #### **#### ****#### ``**#### ****#### `expirationDate (optional number) `******``******

> > #### **#### ****#### ``**#### ****#### `The expiration date of the cookie as the number of seconds since the UNIX epoch. Not provided for session cookies. `******``******

> #### **#### ****#### ``**#### ****#### `storeId (string)`******``******

> > #### **#### ****#### ``**#### ****#### `The ID of the cookie store containing this cookie, as provided in getAllCookieStores().`******``******

#### **#### ****#### ``**#### ****#### `CookieStore (object) `******``******

#### **#### ****#### ``**#### ****#### `A session cookie store in the browser. An incognito mode window, for instance, has a separate session cookie store from other windows.`******``******

> #### **#### ****#### ``**#### ****#### `id (string)`******``******

> > #### **#### ****#### ``**#### ****#### `The unique identifier for the cookie store.`******``******

> #### **#### ****#### ``**#### ****#### `tabIds (array of integer)`******``******

> > #### **#### ****#### ``**#### ****#### `Identifiers of all the browser tabs that share this cookie store.`******``******

#### **#### ****#### ``**#### ****#### `A more formal description of the API and some sample code usage:`******``******

`[`
` {`

` "namespace": "experimental.cookies",`

` "types": \[`

` {`

` "id": "Cookie",`

` "type": "object",`

` "description": "Represents information about an HTTP cookie.",`

` "properties": {`

` "name": {"type": "string", "description": "The name of the cookie."},`

` "value": {"type": "string", "description": "The value of the cookie."},`

` "domain": {"type": "string", "description": "The domain of the cookie."},`

` "hostOnly": {"type": "boolean", "description": "True if the cookie is a
host-only cookie (i.e. a request's host must exactly match the domain of the
cookie)."},`

` "path": {"type": "string", "description": "The path of the cookie."},`

` "secure": {"type": "boolean", "description": "True if the cookie is marked as
Secure (i.e. its scope is limited to secure channels, typically HTTPS)."},`

` "httpOnly": {"type": "boolean", "description": "True if the cookie is marked
as HttpOnly (i.e. the cookie is inaccessible to client-side scripts)."},`

` "session": {"type": "boolean", "description": "True if the cookie is a session
cookie, as opposed to a persistent cookie with an expiration date."},`

` "expirationDate": {"type": "number", "optional": true, "description": "The
expiration date of the cookie as the number of seconds since the UNIX epoch. Not
provided for session cookies."},`

` "storeId": {"type": "string", "description": "The ID of the cookie store
containing this cookie, as provided in getAllCookieStores()."}`

` }`

` },`

` {`

` "id": "CookieStore",`

` "type": "object",`

` "description": "Represents a cookie store in the browser. An incognito mode
window, for instance, uses a separate cookie store from a non-incognito
window.",`

` "properties": {`

` "id": {"type": "string", "description": "The unique identifier for the cookie
store."},`

` "tabIds": {"type": "array", "items": {"type": "integer"}, "description":
"Identifiers of all the browser tabs that share this cookie store."}`

` }`

` }`

` \],`

` "functions": \[`

` {`

` "name": "get",`

` "type": "function",`

` "description": "Retrieves information about a single cookie. If more than one
cookie of the same name exists for the given URL, the one with the longest
domain property, longest path length, or earliest creation time will be
returned.",`

` "parameters": \[`

` {`

` "type": "object",`

` "name": "details",`

` "description": "Details to identify the cookie being retrieved.",`

` "properties": {`

` "url": {"type": "string", "description": "The URL with which the cookie to
retrieve is associated. This argument may be a full URL, in which case any data
following the URL path (e.g. the query string) is simply ignored. If host
permissions for this URL are not specified in the manifest file, the API call
will fail."},`

` "name": {"type": "string", "description": "The name of the cookie to
retrieve."},`

` "storeId": {"type": "string", "optional": true, "description": "The ID of the
cookie store in which to look for the cookie. By default, the current execution
context's cookie store will be used."}`

` }`

` },`

` {`

` "type": "function",`

` "name": "callback",`

` "parameters": \[`

` {`

` "name": "cookie", "$ref": "Cookie", "optional": true, "description": "Contains
details about the cookie. This parameter is null if no such cookie was found."`

` }`

` \]`

` }`

` \]`

` },`

` {`

` "name": "getAll",`

` "type": "function",`

` "description": "Retrieves all cookies from a single cookie store that match
the given information.",`

` "parameters": \[`

` {`

` "type": "object",`

` "name": "details",`

` "description": "Information to filter the cookies being retrieved.",`

` "properties": {`

` "url": {"type": "string", "optional": true, "description": "Restricts the
retrieved cookies to those that would match the given URL."},`

` "name": {"type": "string", "optional": true, "description": "Filters the
cookies by name."},`

` "domain": {"type": "string", "optional": true, "description": "Restricts the
retrieved cookies to those whose domains match or are subdomains of this
one."},`

` "path": {"type": "string", "optional": true, "description": "Restricts the
retrieved cookies to those whose path exactly matches this string."},`

` "secure": {"type": "boolean", "optional": true, "description": "Filters the
cookies by their Secure property."},`

` "session": {"type": "boolean", "optional": true, "description": "Filters out
session vs. persistent cookies."},`

` "storeId": {"type": "string", "optional": true, "description": "The cookie
store to retrieve cookies from. If omitted, the current execution context's
cookie store will be used."}`

` }`

` },`

` {`

` "type": "function",`

` "name": "callback",`

` "parameters": \[`

` {`

` "name": "cookies", "type": "array", "items": {"$ref": "Cookie"},
"description": "All the existing, unexpired cookies that match the given cookie
info."`

` }`

` \]`

` }`

` \]`

` },`

` {`

` "name": "set",`

` "type": "function",`

` "description": "Sets a cookie with the given cookie data; may overwrite
equivalent cookies if they exist.",`

` "parameters": \[`

` {`

` "type": "object",`

` "name": "details",`

` "description": "Details about the cookie being set.",`

` "properties": {`

` "url": {"type": "string", "description": "The request-URI to associate with
the setting of the cookie. This value can affect the default domain and path
values of the created cookie. If host permissions for this URL are not specified
in the manifest file, the API call will fail."},`

` "name": {"type": "string", "optional": true, "description": "The name of the
cookie. Empty by default if omitted."},`

` "value": {"type": "string", "optional": true, "description": "The value of the
cookie. Empty by default if omitted."},`

` "domain": {"type": "string", "optional": true, "description": "The domain of
the cookie. If omitted, the cookie becomes a host-only cookie."},`

` "path": {"type": "string", "optional": true, "description": "The path of the
cookie. Defaults to the path portion of the url parameter."},`

` "secure": {"type": "boolean", "optional": true, "description": "Whether the
cookie should be marked as Secure. Defaults to false."},`

` "httpOnly": {"type": "boolean", "optional": true, "description": "Whether the
cookie should be marked as HttpOnly. Defaults to false."},`

` "expirationDate": {"type": "number", "optional": true, "description": "The
expiration date of the cookie as the number of seconds since the UNIX epoch. If
omitted, the cookie becomes a session cookie."},`

` "storeId": {"type": "string", "optional": true, "description": "The ID of the
cookie store in which to set the cookie. By default, the cookie is set in the
current execution context's cookie store."}`

` }`

` }`

` \]`

` },`

` {`

` "name": "remove",`

` "type": "function",`

` "description": "Deletes a cookie by name.",`

` "parameters": \[`

` {`

` "type": "object",`

` "name": "details",`

` "description": "Information to identify the cookie to remove.",`

` "properties": {`

` "url": {"type": "string", "description": "The URL associated with the cookie.
If host permissions for this URL are not specified in the manifest file, the API
call will fail."},`

` "name": {"type": "string", "description": "The name of the cookie to
remove."},`

` "storeId": {"type": "string", "optional": true, "description": "The ID of the
cookie store to look in for the cookie. If unspecified, the cookie is looked for
by default in the current execution context's cookie store."}`

` }`

` }`

` \]`

` },`

` {`

` "name": "getAllCookieStores",`

` "type": "function",`

` "description": "Lists all existing cookie stores.",`

` "parameters": \[`

` {`

` "type": "function",`

` "name": "callback",`

` "parameters": \[`

` {`

` "name": "cookieStores", "type": "array", "items": {"$ref": "CookieStore"},
"description": "All the existing cookie stores."`

` }`

` \]`

` }`

` \]`

` }`

` \],`

` "events": \[`

` {`

` "name": "onChanged",`

` "type": "function",`

` "description": "Fired when a cookie is set or removed.",`

` "parameters": \[`

` {`

` "type": "object",`

` "name": "changeInfo",`

` "properties": {`

` "removed": {"type": "boolean", "description": "True if a cookie was
removed."},`

` "cookie": {"$ref": "Cookie", "description": "Information about the cookie that
was set or removed."}`

` }`

` }`

` \]`

` }`

` \]`

` }`

`]`

#### Sample code

`function updateUserLoginStatusUi() {`
` chrome.experimental.cookies.get({`
` url: "http://www.mysite.com",`
` name: "MY_COOKIE"`
` }, function(cookie) {`
` if (cookie) {`
` displayLoggedInUi();`
` } else {`
` displayLoggedOutUi();`
` }`
` });`
`}`
`chrome.experimental.cookies.onChanged.addListener(function(changeInfo) {`
` updateUserLoginStatusUi();`
`});`

`// Remove a cookie`

`chrome.experimental.cookies.remove(`

` {url: "http://www.mysite.com", name: "MY_COOKIE"});`

`// Manually expire a cookie`

`chrome.experimental.cookies.set(`

` {`

` url: "http://www.mysite.com",`

` name: "MY_COOKIE",`

` expirationDate: 0,`

` }); `
`function setSessionCookieForTab(tabId, url, name, value) { `
` chrome.experimental.cookies.getAllCookieStores(function(cookieStores) { `
` var i;`

` for (i = 0; i < cookieStores.length; i++) {`

` if (cookieStores[i].tabIds.indexOf(tabId) != -1) {`

` chrome.experimental.cookies.set(`

` {url: url, name: name, value: value, storeId: cookieStores[i].id});`

` }`
` }`
` });`
`}`
