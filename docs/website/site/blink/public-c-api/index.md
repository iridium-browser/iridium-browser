---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: public-c-api
title: Public C++ API
---

[TOC]

## Overview

The Blink public API (formerly known as the WebKit API) is the C++ embedding
layer by which Blink and its embedder (Chromium) communicate with each other.
The API is divided into two parts - the web API, located in
[public/web](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/blink/public/web/)/,
and the platform API located in
[public/platform/](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/blink/public/platform/).
The embedder uses the web API to talk to Blink and Blink uses the platform API
to ask the embedder to perform lower-level tasks. The platform API also provides
common types and functionality used in both the web and platform APIs.

This shows the overall dependency layers. Things higher up in the diagram depend
only on things below them.

+-------------------------------------------+

| Embedder (Chromium) |

+-------------------------------------------+

| Blink public web API |

+---------------------+ |

| Blink internals | |

+-------------------------------------------+

| Blink public platform API |

+-------------------------------------------+

| Embedder-provided platform implementation |

+-------------------------------------------+

The web API covers concepts like the DOM, frames, widgets and input handling.
The embedder typically owns a
[WebView](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/blink/public/web/web_view.h)
which represents a page and navigates
[WebFrame](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/blink/public/web/web_frame.h)s
within the page. Many objects in the web API are actually thin wrappers around
internal Blink objects.

The platform API covers concepts like graphics, networking, database access and
other low-level primitives. The platform API is accessible to nearly all of
Blink's internals. The platform API is provided through an implementation of the
pure virtual blink::Platform interface provided when Blink is initialized. Most
of the platform API is expressed by pure virtual interfaces implemented by the
embedder.

### Conventions and common idioms

#### Naming

The Blink API follows the [Blink coding
conventions](https://chromium.googlesource.com/chromium/src/+/HEAD/styleguide/c++/blink-c++.md).

All public Blink API types and classes are in the namespace `blink` and have the
prefix `Web`, both mostly for historical reasons. Types that are thin wrappers
around Blink internal classes have the same name as the internal class except
for the prefix. For instance, `blink::WebFrame` is a wrapper around
`blink::Frame`.

Enums in the public Blink API prefix their values with the name of the enum. For
example:

```none
enum WebMyEnum {
  kWebMyEnumValueOne,
  kWebMyEnumValueTwo,
    ...
};
```

No prefixes are needed for "`enum class`".

```none
enum class WebMyEnum {
  kValueOne,
  kValueTwo,
  ...
};
```

#### WebFoo / WebFooClient

Many classes in the Blink API are associated with a client. This is typically
done when the API type is a wrapper around a Blink internals class and the
client is provided by the embedder. In this pattern, the `WebFooClient` and the
`WebFoo` are created by the embedder and the embedder is responsible for
ensuring that the lifetime of the `WebFooClient` is longer than that of the
`WebFoo`. For example, the embedder creates a `WebView` with a `WebViewClient*`
that it owns (see
[`WebView::create`](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/blink/public/web/web_view.h&q=WebView::create&sq=package:chromium&type=cs)).

#### Wrapping a RefCounted Blink class

Many public API types are value types that contain references to Blink objects
that are reference counted. For example,
[WebNode](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/public/web/WebNode.h)
contains a reference to a
[Node](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/core/dom/Node.h).
To do this, `web_node.h` has a forward declaration of `Node` and `WebNode` has a
single member of type
[`WebPrivatePtr`](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/public/platform/WebPrivatePtr.h)`<Node>`.
`WebNode` also exports a `WebNode::reset()` that can dereference this member
(and thus potentially invoke the `Node` destructor). There's also a
[`WebPrivateOwnPtr`](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/public/platform/WebPrivateOwnPtr.h)`<T>`
for wrapping types that are single ownership.
