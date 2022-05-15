---
breadcrumbs:
- - /developers
  - For Developers
page_name: web-platform-status
title: Web Platform Status
---

THIS PAGE HAS BEEN DEPRECATED! PLEASE SEE
[CHROMESTATUS.COM](http://CHROMESTATUS.COM) FOR THE LATEST DATA.

Last updated: March 6, 2013

DevRel Contact: Eric Bidelman (ericbidelman), Paul Irish (paulirish)

This page captures the implementation status of HTML5 feature support in Chrome
(desktop) and Chrome for Android. The current release of Chrome for Android
matches m25, any exceptions to this will be indicated below.

You have a few ways to watch these features more closely:

*   ☆ the Chromium bug tickets or add yourself to the CC of WebKit
            tickets.
*   Watch the [Last Week in
            WebKit/Chromium](http://peter.sh/category/last-week/) series for
            weekly updates
*   Subscribe to the [chromium-html5 discussion
            group](https://groups.google.com/a/chromium.org/group/chromium-html5/topics)

Many of these features have their own jumpoff point on
[html5rocks.com](http://html5rocks.com/), with links to solid tutorials and
resources.

The near-daily-updated [Chrome Canary](http://tools.google.com/dlpage/chromesxs)
lands these features much sooner than the stable or beta releases. (Though it's
pre-alpha, so expect bugs!)

Note: In this page there are many references to "m6" and such. This refers to
the stable Chrome release of that version.

[TOC]

## File APIs

Handle file uploads and file manipulation.

Availability: Basic File API support in m5.

Spec: [w3c spec](http://dev.w3.org/2006/webapi/FileAPI/)

Notes: Limited form of drag-and-drop available in Firefox 3.6

#### File System

Availability: started to land in m8, but only available for apps & extensions.
Synchronous APIs are in m9 for web workers.
`[window.resolveLocalFileSystemURL()](http://dev.w3.org/2009/dap/file-system/file-dir-sys.html#solveLocalFileSystemURL)`
and
`[Entry.toURL()](http://dev.w3.org/2009/dap/file-system/file-dir-sys.html#widl-Entry-toURL)`
were added in m11. Stable release for web pages \[not apps/extensions\] was m13.

Spec: [w3c spec](http://dev.w3.org/2009/dap/file-system/file-dir-sys.html)

Notes: [devtools
support](https://bug-45982-attachments.webkit.org/attachment.cgi?id=70320)
coming. Needs flag `--allow-file-access-from-files` for testing from file://
URLs.

Demos:
[layouttests](http://trac.webkit.org/browser/trunk/LayoutTests/fast/filesystem)

Dev Contact: Eric Uhrhane (ericu)

> **Drag and Drop Directories**

> Allows dragging and dropping entire folders using HTML5 Drag and Drop. Extends
> the DataTransferItem with a method to get a FileEntry/DirectoryEntry.

> (`var entry = DataTransferItem.webkitGetAsEntry()`)

> Availability: M21

> Spec: <http://wiki.whatwg.org/wiki/DragAndDropEntries>

> Dev Contact: Kinuko Yasuda (kinuko)

> **HTMLInputElement.webkitEntries**

> Get FileSystem API entries representing user selected files from an `<input
> type="file" multiple>`.

> Availability: M22

> Spec: <http://wiki.whatwg.org/wiki/DragAndDropEntries>

> Dev Contact: Kinuko Yasuda (kinuko)

#### FileWriter

Availability: basic in m8, sync api in m9 for web workers

Spec: [w3c spec](http://www.w3.org/TR/file-writer-api/)

Tickets: [webk.it/44358](https://bugs.webkit.org/show_bug.cgi?id=44358)

Dev Contact: Eric Uhrhane (ericu)

#### FileReader

Can read local and parse local files.

Availability: m7.

Spec: [w3c spec](http://dev.w3.org/2006/webapi/FileAPI/#dfn-filereader)

Demo: [demo](http://slides.html5rocks.com/#drag-in)

Dev Contact: Jian Li (jianli)

#### BlobBuilder

Build blobs (files).

Availability: m8. As of m20, this API is deprecated. Use Blob() constructor
instead (see below).

Demo:
[demo](http://html5-demos.appspot.com/static/html5storage/index.html#slide51)

Spec: [w3c
spec](http://dev.w3.org/2009/dap/file-system/file-writer.html#idl-def-BlobBuilder)

Notes: `window.createBlobURL()` changed to new spec's `window.createObjectURL()`
somewhere around m8. As of M10, the call is prefixed under
`window.webkitURL.createObjectURL()`. Chrome 23 unprefixes `window.URL`.

#### Blob() constructor

Removes the need for BlobBuilder API. Allows you to construct Blobs directly
(`var blob = new Blob(["1234"], {type: 'text/plain'})`)

Availability: M20

Spec: [w3c spec](http://dev.w3.org/2006/webapi/FileAPI/#dfn-Blob)

> #### Blob() constructor ArrayBufferView support

> Blob() constructor takes ArrayBufferView directly rather than constructing a
> blob with ArrayBuffer.

> Availability: M21

#### Unprefixed Blob.slice() support

Blob.slice() is un-prefixed (from webkitSlice()) in M21.

Availability: M22

#### Typed Arrays

Buffers for holding binary data and working with WebGL & Audio API:
`ArrayBuffer`, `Float32Array` , `Int16Array`, `Uint8Array`, etc.)

Availability: m7 for most. `DataView` is m9. `Float64Array` in m13.

Spec:
[Khronos](https://cvs.khronos.org/svn/repos/registry/trunk/public/webgl/doc/spec/TypedArray-spec.html)

Notes: `worker.postMessage(TypedArray|ArrayBuffer)` landed in m13 using
structured cloning. `postMesage(Object|Array)` (e.g. any type) landed in M23.

**a\[download\] attribute**

When used on an &lt;a&gt;, this attribute signifies that the resource it points
to should be downloaded by the browser rather than navigating to it.

See [update.html5rocks.com
post](http://updates.html5rocks.com/2011/08/Downloading-resources-in-HTML5-a-download).

Availability: M14

Spec: [whatwg](http://developers.whatwg.org/links.html#downloading-resources)

Demo: [demo](http://html5-demos.appspot.com/static/a.download.html)

## **Offline and XHR**

**App Cache**

### Enables web pages to work without the user being connected to the internet

### Notes: Chrome's implementation is maxed at 260MB. Individual files max out at 32MB (260MB/8)
### Availability: m5. Dev tools support added in m6
### Dev Contact: Michael Nordman (michaeln)

**XHR supports xhr.send(ArrayBuffer)**

#### Allows for sending a binary byte array using xhr.

#### Availability: targeting m9

#### Dev Contact: Jian Li (jianli)

#### Spec: [w3
draft](http://dev.w3.org/2006/webapi/XMLHttpRequest-2/#dom-xmlhttprequest-send)

#### XHR supports xhr.send(Blob|File)

Allows for sending a Blob or File using xhr.

Availability: m7

Spec: [w3
draft](http://dev.w3.org/2006/webapi/XMLHttpRequest-2/#dom-xmlhttprequest-send)

Dev Contact: Eric Uhrhane (ericu)

Tickets: [crbug.com/51267](http://crbug.com/51267)

#### XHR supports xhr.send(FormData)

Availability: landed m6

Demo: [demo on jsfiddle](http://jsfiddle.net/rUyVq/1/)

#### XHR supports xhr.send(ArrayBufferView)

Allows for sending a typed array directly rather than sending just its
ArrayBuffer.

Availability: m22

**XHR.response, XHR.responseType**

#### Allows reading an xhr response as a blob/arraybuffer (`xhr.responseType = 'arraybuffer'`,` xhr.responseType = 'blob'`)

Availability: m10. m18 adds `xhr.responseType = 'document'` (see [HTML in
XMLHttpRequest](https://developer.mozilla.org/en/HTML_in_XMLHttpRequest)) and

restricts the usage of synchronous XHRs (`xhr.open(..., false);`) by throwing an
error when `.responseType` is set.

Spec: [w3 draft](http://dev.w3.org/2006/webapi/XMLHttpRequest-2/#xmlhttprequest)

Tickets: [crbug.com/52486](http://crbug.com/52486) (arraybuffer works, blob does
not)

Dev Contact: Michael Nordman (michaeln)

#### XMLHttpRequestProgressEvent

Availability: m7

Spec: [w3 draft](http://dev.w3.org/2006/webapi/XMLHttpRequest-2/#xmlhttprequest)

#### navigator.connection

Access the underlying network information (connection info) of the device.

Availability: target unknown

Spec: [w3 draft](http://dev.w3.org/2009/dap/netinfo/)

Tickets: [webk.it/73528](http://webk.it/73528)

Notes: In Android since 2.2, but not implemented yet desktop browsers.

Dev Contact: -

#### navigator.onLine

Allows an application to check if the user has an active internet connection.
Also can register 'online' and 'offline' event handlers.

Availability: m14

Tickets: [~~crbug.com/7469~~](http://crbug.com/7469)

Notes: <https://developer.mozilla.org/en/Online_and_offline_events>

Demo: <http://kinlan-presentations.appspot.com/bleeding-berlin/index.html#16>

Dev Contact: adamk

#### navigator.registerProtocolHandler()

Allow web applications to handle URL protocols like mailto:.
Availability: m13
Tickets:
[~~crbug.com/11359~~](http://code.google.com/p/chromium/issues/detail?id=11359)

Notes: Protocol list: "mailto", "mms", "nntp", "rtsp", "webcal". Custom
protocols require "web+" prefix (e.g. "web+myScheme").

There are no plans to implement registerContentHandler.

Dev Contact: koz

#### Directory upload

Allow specifying a directory to upload (`<input type="file" multiple
webkitdirectory />`), which is just be an extension of existing form
mime-multipart file upload.

Availability: m8. m21 [landed](http://crbug.com/58977) dragging and dropping a
folder onto an &lt;input type="file" webkidirectory&gt; element).

Demo:
[demo](http://html5-demos.appspot.com/static/html5storage/index.html#slide47)

#### Websockets

*   TBD: The return type of send() will be changed to void
*   m16: Update to conform to [RFC 6455 (aka HyBi 13
            version)](http://tools.ietf.org/html/rfc6455) (a few minor changes
            in handshake from [HyBi 08
            version](http://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol-10).
            See also [HTML5 Rocks
            post](http://updates.html5rocks.com/2011/10/WebSockets-updated-to-latest-version-in-Chrome-Canary))
*   m15: Binary message API (Blob and ArrayBuffer). Added close code and
            reason to close() and CloseEvent
*   m14: New handshake and framing introduced at [HyBi 08
            version](http://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol-10)
            excluding binary message API and extension support
*   (m6: New handshake ([Hixie 76
            version](http://tools.ietf.org/html/draft-hixie-thewebsocketprotocol-76)),
            m5: WebSocket in workers, m4: [Hixie 75
            version](http://tools.ietf.org/html/draft-hixie-thewebsocketprotocol-75))
*   m23: ArrayBufferView support

Spec: [W3C editor's draft](http://dev.w3.org/html5/websockets/)

Protocol Spec: [RFC 6455](http://tools.ietf.org/html/rfc6455) +
x-webkit-deflate-frame

Demo: [demo](http://slides.html5rocks.com/#web-sockets)

Tickets: [WebKit protocol ticket](https://bugs.webkit.org/show_bug.cgi?id=50099)

Planned: Implement permessage-compress extension and mux extension

Dev Contact: tyoshino

#### Strict Transport Security

Header to inform the browser to always request a given domain over SSL, reducing
MITM attack surface area.

Availability: m4

Spec: [ietf spec](http://tools.ietf.org/html/draft-hodges-strict-transport-sec)

Notes: [details on STS on chromium.org](/sts)

#### a\[ping\] attribute.

Notify resource on link click.

Availability: Chrome 7.

Spec: [html5 spec](http://developers.whatwg.org/links.html#ping)

Notes: [webkit changeset](http://trac.webkit.org/changeset/68166). Can be
disabled through a command line flag or in about:flags

## Storage

#### **Web SQL Database**

API exposing an SQLite database

Availability: m4.

Notes: No spec progress or other implementations (outside of Webkit/Opera)
expected.

Demo: [demo](http://slides.html5rocks.com/#web-sql-db)

Dev Contact: Dumitru Daniliuc (dumi)

#### Database access from workers

Spec: <http://dev.w3.org/html5/webdatabase/#synchronous-database-api>

Availability: Indeterminate status

#### Indexed Database API

Availability: Landed in m11. (Prefixed as `webkitIndexedDB `pre M24.)

Spec: [Draft w3c
spec](http://dvcs.w3.org/hg/IndexedDB/raw-file/tip/Overview.html)
Notes: formerly called WebSimpleDB.

Demo:
[Demo](http://html5-demos.appspot.com/static/html5storage/index.html#slide34),
[LayoutTest IndexedDB tutorial &
demo](http://trac.webkit.org/export/70913/trunk/LayoutTests/storage/indexeddb/tutorial.html)

Dev Contacts: dgrogan, jsbell, alecflett

Cross-Browser Status:

*   Firefox: FF16+ enabled (through 4beta9 as `moz_indexedDB`; through
            FF15 as `mozIndexedDB)`
*   IE: IE10+ (IE10 Platform Preview through 5 as msIndexedDB)
    *   IE10 will not support Array key paths
*   Opera implementation is in progress

Known differences between Chrome (as of M25) and the draft specification (as of
July 16, 2012):

*   a "`blocked`" event will fire against a request created by an
            `open(name, version)`, a `setVersion(version)` or
            `deleteDatabase(name)` call even if all other connections are closed
            in response to a "`versionchange`" event
*   "`blocked`" events may also fire multiple times against the same
            deleteDatabase(name) request
*   Blobs, Files, and FileLists in values are not supported*.*
*   The Sync API is not supported - but neither IE nor Firefox support
            this either; it is marked as "At Risk" in the spec

Updates in M23 (from M22)

*   The new-style `open(name, version).onupgradeneeded` API is now
            supported; the old `open(name)` followed by `setVersion(version)`
            API may still be used for now, but web applications should migrate
            as soon as possible.

Updates in M22 (from M21)

*   `deleteDatabase()` and `webkitGetDatabaseNames()` can now be called
            from Workers
*   `IDBKeyRange.upperOpen`/`lowerOpen` flags are now reported correctly
            when `lowerBound()`/`upperBound() `used
*   Empty arrays are no longer valid key paths
*   Requests can no longer be issued against transactions when not
            active (i.e. outside of creation context or callbacks, e.g. from a
            timeout or non-IDB event)
*   "`versionchange`" events are no longer fired against connections
            that are closing
*   Multi-entry indexes with invalid/duplicate subkeys now populated per
            spec

Updates in M21 (from M20)

*   Exceptions are now raised when methods are called on deleted objects
*   [Creating a transaction from within a transaction callback now
            fails](https://groups.google.com/a/chromium.org/forum/?fromgroups#!topic/chromium-html5/VlWI87JFKMk)
*   Cursor value modifications are now preserved until cursor iterates
*   Error codes should now match the latest spec
*   Nonstandard enumeration API has now been renamed to
            `IDBFactory.webkitGetDatabaseNames()`
*   `IDBTransaction.error` and `IDBRequest.error` are now implemented
*   Support for Array key paths has been implemented
*   Invalid index keys no longer cause `add()`/`put() `calls to fail -
            indexes are skipped
*   The `IDBObjectStore.autoIncrement` property is now available
*   [`transaction()`, `openCursor()` and `openKeyCursor()` have been
            updated to take strings instead of numbers for enum
            values](https://groups.google.com/a/chromium.org/forum/?fromgroups#!topic/chromium-html5/OhsoAQLj7kc)

#### Web Storage

Availability: localStorage in m4, sessionStorage in m5

Notes: localStorage serves basic use cases. For more comprehensive storage
solution, consider IndexedDB. While the spec indicates anything that structured
clone algorithm can clone can be stored, all browser implementations currently
allow only strings. Chrome's [storage capacity is currently
2.5mb](http://dev-test.nemikor.com/web-storage/support-test/).

Tickets: [webk.it/41645](https://bugs.webkit.org/show_bug.cgi?id=41645)
Dev Contact: Jeremy Orlow (jorlow) -&gt; Michael Nordman (michaeln)

**Quota Management API**

This API can be used to check how much quota an app/origin is using.

Availability: m13: FileSystem (TEMPORARY & PERSISTENT) and WebSQL (temp only).
m14 : AppCache and IndexedDB were added (TEMP only)

Docs: <http://code.google.com/chrome/whitepapers/storage.html>

Dev Contact: Kinuko Yasuda (kinuko)

## **CSS & Presentation**

#### CSS3 3D Transforms

Availability: m12

Spec: [w3 spec](http://www.w3.org/TR/css3-3d-transforms/)

Notes: Associated GPU rendering quirkiness actively being worked on in prep for
beta channel release.

Demo: [poster circle
demo](http://webkit.org/blog-files/3d-transforms/poster-circle.html)

#### new semantic sectioning elements

section, article, aside, nav, header, and `footer` elements

Availability: m5

#### &lt;progress&gt; and &lt;meter&gt; elements

Offer a visual display of progress.

Availability: m6

Spec: [whatwg
spec](http://www.whatwg.org/specs/web-apps/current-work/multipage/the-button-element.html#the-progress-element)

Demo: [demo of elements in
action](http://peter.sh/examples/?/html/meter-progress.html)

#### Ruby

Ruby annotations are short runs of text presented alongside base text, primarily
used in East Asian typography as a guide for pronunciation or to include other
annotations
Availability: m4

Spec: [whatwg html5
spec](http://www.whatwg.org/specs/web-apps/current-work/multipage/text-level-semantics.html#the-ruby-element)

Notes: [description &
demo](http://blog.whatwg.org/implementation-progress-on-the-html5-ruby-element)
on whatwg blog

#### @font-face webfonts

Availability: OTF/TTF support in m4. WOFF in m6.

Spec: [w3c editor's draft](http://dev.w3.org/csswg/css3-fonts/)

Notes: [unicode-range
support](http://dev.w3.org/csswg/css3-fonts/#unicode-range-desc) planned.
Improved handling of text while font asset is being downloaded, also planned.

#### **Gradients**

Availability: w3c [linear-gradient()
syntax](http://webkit.org/blog/1424/css3-gradients/) as of m10. [original webkit
-gradient() syntax](http://webkit.org/blog/175/introducing-css-gradients/) as of
m3.

Spec: [w3c image values](http://dev.w3.org/csswg/css3-images/#gradients)

Notes: The legacy syntax will be retained for the foreseeable future.

#### Background printing

Opt-in ability to specify a background should be printed

Availability: landing in m16 with prefix:` -webkit-print-background`

Discussions: [thread feb
2011](http://lists.w3.org/Archives/Public/www-style/2011Feb/0626.html), [csswg
minutes
august](http://lists.w3.org/Archives/Public/www-style/2011Aug/0645.html),
[thread august
2011](http://lists.w3.org/Archives/Public/www-style/2011Aug/0436.html), [wiki
page](http://wiki.csswg.org/ideas/print-backgrounds)

**Scoped style sheets**

Boolean attribute for the &lt;style&gt; element (&lt;style scoped&gt;). When
present, its styles only apply to the parent element.

Availability: m20 (enable webkit experimental feature in about:flags)

Spec:
[whatwg](http://www.whatwg.org/specs/web-apps/current-work/multipage/semantics.html#attr-style-scoped)

Demo: [jsbin](http://jsbin.com/ozerih/edit#html,live)

#### CSS3 Filters

Apply (SVG-like) filter effects to arbitrary DOM elements.

Availability: m19

Spec: [w3 spec](https://dvcs.w3.org/hg/FXTF/raw-file/tip/filters/index.html)

Info:
<http://updates.html5rocks.com/2011/12/CSS-Filter-Effects-Landing-in-WebKit>

#### #### CSS Shaders

#### Apply OpenGL shaders to arbitrary DOM elements.

#### Availability: m24, enabled via about:flags.

#### Spec: [w3
spec](https://dvcs.w3.org/hg/FXTF/raw-file/tip/filters/index.html)

#### Info: <http://www.adobe.com/devnet/html5/articles/css-shaders.html>

#### Demos: <http://adobe.github.com/web-platform/samples/css-customfilters/>

#### CSS3 Grid Layout

Availability: work is just started behind a flag. No way to enable it.

Spec: [w3c](http://dev.w3.org/csswg/css3-grid-layout/)

Tickets: [webk.it/60731](http://webk.it/60731)

Dev Contact: Tony Chang (tony@chromium), Ojan Vafai (ojan@chromium)

#### CSS3 Flexbox Model (the new one)

Note: The old box model (display: -webkit-box) and spec has been replaced by a
new one (display: -webkit-flex).

For now, chrome supports both. But you should use the new one!

Availability: started landing in m18. m21 has the latest and full implementation
with a bunch of properties renamed.

Spec: [w3 spec](http://www.w3.org/TR/css3-flexbox/)

Tickets: [webk.it/62048](https://bugs.webkit.org/show_bug.cgi?id=62048)

Demo: <http://html5-demos.appspot.com/static/css/flexbox/index.html>

Docs: <https://developer.mozilla.org/en/Using_flexbox>

Dev Contact: Ojan Vafai (ojan)

#### CSS3 Regions

Magazine-like content flow into specified regions.

Availability: Partial support in m19 (enable webkit experimental feature in
about:flags)

Spec: [w3c](http://dev.w3.org/csswg/css3-regions/)

Tickets: [webk.it/57312](https://bugs.webkit.org/show_bug.cgi?id=57312)

#### position: sticky

`position: sticky` is a new way to position elements and is conceptually similar
to position: fixed. The difference is that a stickily positioned element behaves
like position: relative within its parent, until a given offset threshold is
met.

Availability: M23 (enable webkit experimental feature in about:flags)

Proposal:
[www-style](http://lists.w3.org/Archives/Public/www-style/2012Jun/0627.html)

Demo: <http://html5-demos.appspot.com/static/css/sticky.html>

#### #### CSS Intrinsic Sizing

#### `width: fill-available`,` width: min-content`, `width: max-content`, etc.

#### Availability: ~M22 (vendor prefixed)

#### Spec: [w3c](http://dev.w3.org/csswg/css3-writing-modes/#intrinsic-sizing)

#### MDN: <https://developer.mozilla.org/en-US/docs/CSS/width>

#### Other CSS3

**box-shadow**: inset keyword and spread value supported since m4, went
unprefixed in m10

**border-radius:** went unprefixed in m4

**cross-fade():** landing with prefix (-webkit-cross-fade) in m17.
[demo](http://peter.sh/files/examples/cross-fading.html)

**image-resolution()**: [landed](http://trac.webkit.org/changeset/119984) for
m21 [spec](http://www.w3.org/TR/2012/CR-css3-images-20120417/#image-resolution)

**clip-path:**
[landed](https://plus.google.com/118075919496626375791/posts/2n8PTisLztW) for
m24. Prefixed -webkit-clip-path.
[spec](http://dvcs.w3.org/hg/FXTF/raw-file/tip/masking/index.html#the-clip-path)

**@viewport**: [spec](http://dev.w3.org/csswg/css-device-adapt/), bug

**@supports:** [spec](http://www.w3.org/TR/css3-conditional/#at-supports), bug,

**CSS viewport % lengths (vw, vh, vmin, vmax):**
[spec](http://www.w3.org/TR/css3-values/), Availability: m20

## Graphics

#### Canvas

Provides an API to draw 2D graphics

Availability: m1, Safari, Firefox, Opera, IE9

Notes: Accelerated 2D canvas [targeting](http://crbug.com/61526) m14 for
windows/linux; perhaps m15 for mac. Currently, available in about:flags.

webp format: `canvas.toDataURL("image/webp")`
[added](http://code.google.com/p/chromium/issues/detail?id=63221) in m17 (WebKit
535), but m18 on Mac.

In M20, `canvas.getImageData(...).data` returns a `Uint8ClampedArray` instead of
a `CanvasPixelArray`.

Spec: [W3C Last Call Working draft](http://dev.w3.org/html5/2dcontext/)

#### WebGL **(Canvas 3D)**

3D rendering via the &lt;canvas&gt; element.

Availability: m9

Android Availability: TBD.

Spec: [kronos/apple
spec](https://cvs.khronos.org/svn/repos/registry/trunk/public/webgl/doc/spec/WebGL-spec.html)

Notes: Originally project developed by Mozilla as a JavaScript API to make
OpenGL calls - [WebGL Wiki](http://khronos.org/webgl/wiki/Main_Page)

Dev Contact: Vangelis Kokevis (vangelis), Ken Russell (kbr)

#### requestAnimationFrame

Offload animation repainting to browser for optimized performance.

Spec: [webkit draft spec](http://webstuff.nfshost.com/anim-timing/Overview.html)

Availability: m10 , currently prefixed as `window.webkitRequestAnimationFrame`

Notes: As of FF4, Mozilla's implementation differs and offers lower framerates,
but it will be addressed. This method should be used for WebGL, Canvas 2D, and
DOM/CSS animation.

Dev Contact: jamesr

#### Accelerated Video: m10
#### WebP image format support: m9. alpha and lossless added to m22.

## Multimedia

#### **Video and Audio**

Natively play video and audio in the browser
Provides API and built-in playback controls
Availability: m3. Also Safari 4, Firefox 3.5
Codecs: mp4, ogv, and webm ship with Chrome. webm shipped in m6.
Dev Contact: Andrew Scherkus (scherkus)

mediaElem.webkitAudioDecodedByteCount - m11

mediaElem.webkitVideoDecodedByteCount - m11

videoElem.webkitDecodedFrameCount - m11

videoElem.webkitDroppedFrameCount - m11

#### Web Audio API

Availability: dev channel since m12. stable channel in m14. m24 adds the ability
for [live mic
input](http://updates.html5rocks.com/2012/09/Live-Web-Audio-Input-Enabled)
(enable in about:flags). m25 [updated API
calls](https://code.google.com/p/chromium/issues/detail?id=160176) to the latest
spec.

Android availability: TBD ([issue 166003](http://crbug.com/166003))

Spec: [W3C Audio Incubator Group
Proposal](http://chromium.googlecode.com/svn/trunk/samples/audio/specification/specification.html)

Notes: enable via about:flags. Also, Mozilla has an alternative [Audio Data
API](https://wiki.mozilla.org/Audio_Data_API) proposal. M20 included

support for oscillator nodes.

Demo: [spec
samples](http://chromium.googlecode.com/svn/trunk/samples/audio/index.html)

Dev Contact: Chris Rogers (crogers)

#### Media Source API

Allows appending data to an &lt;audio&gt;/&lt;video&gt; element.

Availability: dev channel m23

Spec: [w3c
spec](http://dvcs.w3.org/hg/html-media/raw-file/tip/media-source/media-source.html)

Notes: enable via about:flags or run with `--enable-media-source` flag.

Demo: <http://html5-demos.appspot.com/static/media-source.html>

Dev Contact: Aaron Colwell (acolwell)

#### WebRTC

Real time communication in the browser. (Note that many online resources refer
to 'WebRTC' but just mean getUserMedia!)

Combines the following APIs:

- navigator.getUserMedia (aka MediaStream, cross-browser demo:
[simpl.info/gum](http://simpl.info/gum))

- RTCPeerConnection (demos: [apprtc.appspot.com](http://apprtc.appspot.com),
[simpl.info/pc](http://simpl.info/pc))

- RTCDataChannel (demo [simpl.info/dc](http://simpl.info/dc))

Availability:

- M21 for getUserMedia (prefixed),

- M23 for RTCPeerConnection (prefixed and flagless from M23)

- M26 for RTCDataChannel (prefixed, flagless), requires an RTCPeerConnection {
optional:\[ { RtpDataChannels: true } \]} constraint and only supports
unreliable data channels and text messages.

Spec: [w3c spec](http://dev.w3.org/2011/webrtc/editor/webrtc.html)

Docs: [webrtc.org](http://webrtc.org/)

Google group:
<https://groups.google.com/forum/?fromgroups#!forum/discuss-webrtc>

Notes:

- webkitRTCPeerConnection is the name for RTCPeerConnection implementation
currently in Chrome: other names/implementations are obsolete; prefix will be
removed when APIs stabilise

- for the permission dialog: Chrome only show the "Always allow" for sites using
https, for increased security

- getUserMedia is implemented in Opera and Firefox Stable (video only), and
Chrome and Firefox Nightly/Aurora (audio and video);

- RTCPeerConnection and RTCDataChannel are implemented in Firefox Aurora/Nightly

- getUserMedia can be used as input to Web Audio
([demo](http://webaudiodemos.appspot.com/input/index.html), the implementation
can be enabled in about:flags)

- Chrome &lt;=&gt; Firefox RTCPeerConnection interop demo
([blog](http://blog.chromium.org/2013/02/hello-firefox-this-is-chrome-calling.html))

- [Firefox/Chrome/W3C Standard API differences](http://www.webrtc.org/interop)

**Resolution Constraints** are enabled as of M24:

- [spec and
examples](http://tools.ietf.org/html/draft-alvestrand-constraints-resolution-00#page-4)

- [demo](http://simpl.info/getusermedia/constraints)

**Tab Capture** (i.e. ability to get stream of 'screencast') currently available
in Chrome Dev channel:

-
[proposal](http://www.chromium.org/developers/design-documents/extensions/proposed-changes/apis-under-development/webrtc-tab-content-capture)

- [documentation for
chrome.tabCapture](http://developer.chrome.com/trunk/extensions/tabCapture.html)

**Screen Capture** (i.e. ability to capture the contents of the screen)
currently available in Chrome 26.0.1410.0:

More info: <https://plus.google.com/118075919496626375791/posts/7U9fkCRM4SM>

[Demo](https://html5-demos.appspot.com/static/getusermedia/screenshare.html)

Demos: [Testing WebRTC on
Chrome](http://www.webrtc.org/running-the-demos#TOC-Demos)

Code samples: [web-rtc
samples](http://code.google.com/p/webrtc-samples/source/browse/#svn%2Ftrunk) on
code.google.com

Dev contact: Serge Lachapelle (sergel)

#### Track element

Add subtitles, captions, screen reader descriptions, chapters and other types of
timed metadata to video and audio. Chrome currently supports the WebVTT format
for track data.

Availability: m23

Specs:
[WHATWG](http://www.whatwg.org/specs/web-apps/current-work/multipage/video.html#the-track-element),
[W3C](http://dev.w3.org/html5/spec/Overview.html#the-track-element),
[WebVTT](http://dev.w3.org/html5/webvtt/#webvtt-cue-timings)

Notes: Rendering does not yet include CSS pseudo elements.

Demos: [HTML5 Rocks
article](http://www.html5rocks.com/en/tutorials/track/basics/)

Dev Contact: Andrew Scherkus (scherkus), Silvia Pfeiffer (silviapf)

#### Encrypted Media Extensions

Enables playback of encrypted streams in an &lt;audio&gt;/&lt;video&gt;
elements.

Availability: m25: [WebM
Encryption](http://wiki.webmproject.org/encryption/webm-encryption-rfc) & ISO
BMFF Common Encryption (CENC)

Spec: W3C Editor's Draft:
[v0.1b](http://dvcs.w3.org/hg/html-media/raw-file/eme-v0.1b/encrypted-media/encrypted-media.html)
(m22 and later);
[latest](http://dvcs.w3.org/hg/html-media/raw-file/tip/encrypted-media/encrypted-media.html)
(object-oriented version not yet implemented)

Notes: In m25, enable via about:flags or run with `--enable-encrypted-media`
flag. (Enabled by default in m26 and later on desktop and Chrome OS.)

Demo:
<http://downloads.webmproject.org/adaptive-encrypted-demo/adaptive/index.html>

Dev Contact: David Dorwin (ddorwin)

**JS Web Speech API**

Enables web developers to incorporate speech recognition and synthesis into
their web pages.

Availability: m25

Spec: [w3c](http://dvcs.w3.org/hg/speech-api/raw-file/tip/speechapi.html)

Info:
<http://updates.html5rocks.com/2013/01/Voice-Driven-Web-Apps-Introduction-to-the-Web-Speech-API>

Dev Contact: gshires

## Web Components

#### Shadow DOM

Availability: m18 exposed `WebKitShadowRoot().`Enable Shadow DOM in about:flags.
DevTools support is in m20, behind the DevTools Experiments flag (and setting).
m25 removes constructor and exposes `element.webkitCreateShadowRoot() `by
default.

Notes: As of m13, elements internally ported to the shadow dom:
&lt;progress&gt;, &lt;meter&gt;, &lt;video&gt;, &lt;input type=range&gt;,
&lt;keygen&gt;.

Tickets: [webkit shadow dom
tickets](https://bugs.webkit.org/buglist.cgi?query_format=advanced&short_desc_type=substring&short_desc=shadow+dom&long_desc_type=substring&long_desc=&bug_file_loc_type=allwordssubstr&bug_file_loc=&keywords_type=allwords&keywords=&bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&bug_status=RESOLVED&bug_status=VERIFIED&bug_status=CLOSED&emailassigned_to1=1&emailtype1=substring&email1=&emailassigned_to2=1&emailreporter2=1&emailcc2=1&emailtype2=substring&email2=&bugidtype=include&bug_id=&chfieldfrom=&chfieldto=Now&chfieldvalue=&cmdtype=doit&order=Reuse+same+sort+as+last+time&field0-0-0=noop&type0-0-0=noop&value0-0-0=)

Notes: More details at [What the heck is a Shadow
DOM?](http://glazkov.com/2011/01/14/what-the-heck-is-shadow-dom/), [+Component
Model](https://plus.google.com/103330502635338602217/posts)

Dev Contact: dglazkov

&lt;template&gt;

HTML template element to allow creating fragment of inert HTML as a prototype
for stamping out DOM.

Spec:
[w3c](http://dvcs.w3.org/hg/webcomponents/raw-file/tip/spec/templates/index.html)

Availability: targeting m26

Info: <http://html5-demos.appspot.com/static/webcomponents/index.html#16>

Dev Contact: rafaelw

**document.register()**

Method for registering (creating) custom elements in script.

Availability: [bug](https://bugs.webkit.org/show_bug.cgi?id=100229)

**CSS @host at-rule**

Allows styling of the element hosting shadow dom.

Spec:
[spec](http://dvcs.w3.org/hg/webcomponents/raw-file/tip/spec/shadow/index.html#host-at-rule)

Demo: [demo](http://html5-demos.appspot.com/static/webcomponents/index.html#32)

Availability: m25

**CSS ::distributed() pseudo element**

Allows styling of nodes distributed into an insertion point.

Spec:
[spec](https://dvcs.w3.org/hg/webcomponents/raw-file/tip/spec/shadow/index.html#selecting-nodes-distributed-to-insertion-points)

Demo: [demo](http://jsbin.com/acapeh/1/edit)

Availability: m26

## Other Open Web Platform features

#### Desktop notifications

Availability: m5

Spec: [w3 draft
spec](http://dvcs.w3.org/hg/notifications/raw-file/tip/Overview.html), [design
document](/developers/design-documents/desktop-notifications/)

Notes: Additional functionality added in m6: in-place replacement of
notifications, BiDi support, Worker support, and UI improvement.

In M22, `createHTMLNotification()` was removed because of spec compliance. See
[this
post](https://plus.google.com/u/0/102860501900098846931/posts/8vWo8hq4pDm).
Dev Contact: John Gregg (johnnyg)

#### &lt;details&gt;/&lt;summary&gt;

Interactive widget to show/hide content.

Spec:
[WHATWG](http://www.whatwg.org/specs/web-apps/current-work/multipage/interactive-elements.html)

Availability: m12

Demo: <http://kinlan-presentations.appspot.com/>

#### &lt;datalist&gt;

Predefined data/options for controls

Spec: [W3C](http://www.w3.org/TR/html-markup/datalist.html)

Availability: m20. See [html5rocks.com
update](http://updates.html5rocks.com/2012/04/datalist-landed-in-Chrome-Canary)
for mor info.

Demo: <http://updates.html5rocks.com/2012/04/datalist-landed-in-Chrome-Canary>

#### #### &lt;dialog&gt;

#### The dialog element represents a part of an application that a user
interacts with to perform a task, for example a dialog box, inspector, or
window.

#### Spec:
[WHATWG](http://www.whatwg.org/specs/web-apps/current-work/multipage/commands.html#the-dialog-element)

#### Bug:
[webk.it/](goog_742956014)[84635](https://bugs.webkit.org/show_bug.cgi?id=84635)

#### Availability: Behind a flag in m25. Enable "Experimental WebKit Features"
in about:flags.

#### Dev contact: falken

#### #### Device Orientation

#### Enables real-time events about the 3 dimensional orientation of the
device/laptop

#### Availability: m7, in Intel-based Apple portables ([apple.com
kb](http://support.apple.com/kb/HT1935)), [iOS
4.2](https://developer.apple.com/library/safari/#documentation/SafariDOMAdditions/Reference/DeviceOrientationEventClassRef/DeviceOrientationEvent/DeviceOrientationEvent.html)+

#### Spec: [w3c editors
draft](http://dev.w3.org/geo/api/spec-source-orientation.html)

#### Demo: [demo](http://slides.html5rocks.com/#slide-orientation)

#### #### EventSource

#### Also called Server-sent Events, these are push notifications from the
server received as DOM events.

#### Availability: m6

#### Demo: [html5rocks
article](http://www.html5rocks.com/tutorials/eventsource/basics/), [code and
demos](http://weblog.bocoup.com/chrome-6-server-sent-events-with-new-eventsource)

**Fullscreen API**

Programmatically instruct content on the page to be presented in the browser's
full screen (kiosk) mode.

Availability: m15

Spec: [w3c spec](http://dvcs.w3.org/hg/fullscreen/raw-file/tip/Overview.html)

Demo: [html5rocks
update](http://updates.html5rocks.com/2011/10/Let-Your-Content-Do-the-Talking-Fullscreen-API),
[demo](http://html5-demos.appspot.com/static/fullscreen.html)

Dev contact: jeremya

#### history.pushState

The [pushState, replaceState, and
clearState](http://dev.w3.org/html5/spec-author-view/history.html#dom-history-pushstate)
methods provide applications with programmatic control over session history.

Availability: m5. `history.state` was implemented in m18. popstate firing after
page load: [crbug.com/63040](http://crbug.com/63040)

Documentation:
[MDN](https://developer.mozilla.org/en/DOM/Manipulating_the_browser_history)

#### Geolocation

Enables websites to get geolocation information from browsers

Availability: m5, also Firefox 3.5

Dev Contact: Andrei Popescu (andreip)

#### GamePad API

Gives JS access to a game controller via USB.

Availability: targeting m18

Spec: [W3C Editor's
draft](http://dvcs.w3.org/hg/webevents/raw-file/default/gamepad.html)
Dev contact: scottmg

Notes: [Chromium tracking issue](http://crbug.com/72754). Enabled in about:flags

#### Battery Status

Allows access to see the battery level of the device's battery

Availability: unknown

Spec: [w3c editor's
draft](http://dev.w3.org/2009/dap/system-info/battery-status.html)

Bugs: [webk.it/62698](http://webk.it/62698)

Demo: [here](http://www.smartjava.org/examples/webapi-battery/)

#### #### DOM MutationObservers

#### Provides notifications when DOM nodes are rearranged or modified.

#### Availability: m18, prefixed as WebKitMutationObserver; unprefixed in
current Firefox

#### Spec: [part of dom4
(w3c)](http://dvcs.w3.org/hg/domcore/raw-file/tip/Overview.html#mutation-observers)

#### Related links:
[developer.mozilla.org](https://developer.mozilla.org/en/DOM/DOM_Mutation_Observers)

#### Useful library: [Mutation
Summary](http://code.google.com/p/mutation-summary/)

#### Dev Contact: rafaelw

**&lt;iframe&gt; attributes**

> [MDN docs](https://developer.mozilla.org/en/HTML/Element/iframe)

> **seamless**

> The seamless attribute is used to embed and &lt;iframe&gt; in the calling page
> without scrollbars or borders (e.g. seamlessly). In m26, seamless iframes
> inherit styles from their embedding parent page.

> Availability: m20

> Spec:
> [whatwg](http://www.whatwg.org/specs/web-apps/current-work/multipage/the-iframe-element.html#attr-iframe-seamless)

> Info: <http://benvinegar.github.com/seamless-talk/>

> **srcdoc**

> Gives the content of an iframe as a src context to embed (e.g. `<iframe
> seamless srcdoc="<b>Hello World</b>"></iframe>`).

> Note: If both src and srcdoc are set, the latter takes precedence.

> Availability: m20

> Spec:
> [whatwg](http://www.whatwg.org/specs/web-apps/current-work/multipage/the-iframe-element.html#attr-iframe-srcdoc)

> **sandbox**

> Method of running external site pages with reduced privileges (i.e. no
> JavaScript) in iframes (`<iframe sandbox="allow-same-origin allow-forms"
> src="..."></iframe>`)

> Availability: m19

> Spec:
> [whatwg](http://www.whatwg.org/specs/web-apps/current-work/multipage/the-iframe-element.html#attr-iframe-sandbox)

#### #### matchMedia()

#### API for testing if a given media query will apply.

#### Availability: m9

#### Spec: [w3c cssom
spec](http://dev.w3.org/csswg/cssom-view/#dom-window-matchmedia)

#### Notes: [webkit changeset](http://trac.webkit.org/changeset/72552)

#### Pointer Lock (TAPIFKA Mouse Lock)

Gives access to raw mouse movement, locks the target of mouse events to a single
element, eliminates limits of how far mouse movement can go in a single
direction, and removes the cursor from view. Obvious use cases are for first
person or real time strategy games.
Availability: m18 behind flag. On be default in m22 with a refined API similar
to requestFullScreen.

Spec: [W3C Editor's
draft](http://dvcs.w3.org/hg/webevents/raw-file/default/mouse-lock.html)

Documentation: [HTML5
Rocks](http://updates.html5rocks.com/2012/02/Pointer-Lock-API-Brings-FPS-Games-to-the-Browser)

Notes: [Chromium tracking
issue](http://code.google.com/p/chromium/issues/detail?id=72754)

Dev contact: scheib

Note, late breaking changes:

Pointer lock can only work within one document. If you lock in one iframe, you
can not have another iframe try to lock and transfer the target... it will error
instead. The first iframe has to unlock, then the second iframe can lock.

Also, iframes work by default, but sandboxed iframes block pointer lock. Landed
recently into WebKit the ability to use &lt;iframe
sandbox="allow-pointer-lock"&gt;, but that will not percolate into Chrome until
23.

#### Page Visibility

Provides an API to ask whether the current tab is visibile or not. If you, you
might want to throttle back action or set an idle state.

Availability: m13, prefixed as `document.webkitHidden`

Notes: Relatedly, setTimeout/Interval are clamped to 1000ms when in a background
tab as of [m11](http://code.google.com/p/chromium/issues/detail?id=66078).
Firefox 5 has the
[same](https://developer.mozilla.org/en/DOM/window.setTimeout#Minimum_delay_and_timeout_nesting)
behavior.

Spec: [w3c draft
spec](http://dvcs.w3.org/hg/webperf/raw-file/tip/specs/PageVisibility/Overview.html)

Demo: [samdutton.com/pageVisibility](http://samdutton.com/pageVisibility)

Dev Contact: shishir

#### Workers

Provides a threading API

Availability: Chrome 3, Safari 4, Firefox 3.5. Shared workers available in m4.

Android Availability: Dedicated workers in m16 (0.16). No shared workers, yet.

Spec: [WHATWG](http://www.whatwg.org/specs/web-workers/current-work/)

Notes: Pre-M15, there was a limit to the number of workers that each page can
start, as well as the number of workers that can be running globally across all
pages. This was because a new process was started for each worker. As of M15,
workers are started in-process, meaning less memory footprint and faster
messaging. Transferrable objects landed in m17 (see next)

Dev Contact: Dmitry Lomov (dslomov), Drew Wilson (atwilson)

#### Transferable Object messaging passing

With transferable objects/data, ownership is transferred from one context to
another. It is zero-copy, which vastly improves the performance of sending data
to a Worker or another window.

Availability: m17

Spec: [w3c
spec](http://dev.w3.org/html5/spec/common-dom-interfaces.html#transferable-objects)

Notes: landed as prefixed. Unprefixed in M24.

```none
worker.postMessage(arrayBuffer, [arrayBuffer])
window.postMessage(arrayBuffer, targetOrigin, [arrayBuffer])
```

See: [html5rocks
update](http://updates.html5rocks.com/2011/12/Transferable-Objects-Lightning-Fast),
[demo](http://html5-demos.appspot.com/static/workers/transferables/index.html)

#### &lt;link&gt; load/error events

Specify and onload or onerror event for a stylesheet to load.

Availability: m19?

Spec: [W3C](http://www.w3.org/TR/html5/the-link-element.html#the-link-element)

#### Touch Events

Android Availability: m16 (0.16)

**MathML**

Availability: m24

Spec: [w3c spec](http://www.w3.org/TR/MathML3/)

**JS Internationalization API**

Allows collation (string comparison), number formatting, and date and time
formatting for JavaScript applications. More info
[here](http://norbertlindenberg.com/2012/10/ecmascript-internationalization-api/index.html).

Availability: m24

Spec:
[here](http://www.ecma-international.org/publications/standards/Ecma-402.htm)

**Resource Timing API**

Availability: m25

Spec: [w3c](http://www.w3.org/TR/2011/WD-resource-timing-20110524/)

**User Timing API**

Availability: m25

Spec: [w3c](http://w3c-test.org/webperf/specs/UserTiming/)

**Object.observe()**

Way to observe changes to JS objects.

Availability: m25 (behind Enable Experimental JS Features in about:flags)

Info:
<http://updates.html5rocks.com/2012/11/Respond-to-change-with-Object-observe>

## Webforms

Availability: **See [HTML5 Forms
Status](http://www.chromium.org/developers/web-platform-status/forms) for all
details.** (Updated as recently as Nov 28th, 2012).

Includes details in input types (like date and color), input attributes,
datalist and more.

#### Autocomplete

Provides hints to the autofill implementation to allow Chrome to better provide
suggested values to the user.

Availability: The `x-autocompletetype` attribute was implemented in Chrome 15.
It used [token values that are documented
here](http://wiki.whatwg.org/wiki/Autocomplete_Types#Experimental_Implementation_in_Chrome),
but later revised. Chrome 24 features an implementation of the `autocomplete`
attribute that [matches the HTML
spec](http://www.whatwg.org/specs/web-apps/current-work/multipage/association-of-controls-and-forms.html#autofilling-form-controls:-the-autocomplete-attribute).

## DOM APIs

[Element.matchesSelector](http://www.w3.org/TR/selectors-api2/#matchtesting) -
m4

Element.outerHTML - m1

Element.textContent (faster than innerHTML for plain text) - m1

[Element.classList](http://www.whatwg.org/specs/web-apps/current-work/multipage/elements.html#dom-classlist)
(classList.add/remove methods) - m8

[Element.dataset](http://www.whatwg.org/specs/web-apps/current-work/multipage/elements.html#embedding-custom-non-visible-data-with-the-data-*-attributes)
(Provides easy access to `data-*` attribute values) - m8

window.onerror event - [m10](http://trac.webkit.org/changeset/76216)

crypto.getRandomValues() - m11

selectionchange event - [m11](http://trac.webkit.org/changeset/79208)

## Chrome Extensions

[View the changelog of features landing in Chrome
extensions.](http://code.google.com/chrome/extensions/trunk/whats_new.html)

More resources for going deep with HTML5:

*   [HTML5 Rocks](http://html5rocks.com/)
*   [Mozilla Developer
            Network](https://developer.mozilla.org/en/HTML/HTML5)
*   [StackOverflow:
            html5](http://stackoverflow.com/questions/tagged/html5)
*   [#html5](http://webchat.freenode.net/?channels=html5) on freenode
