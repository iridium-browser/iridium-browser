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
- - /developers/design-documents/extensions/how-the-extension-system-works/docs
  - Extension Documentation System
page_name: how-docs-are-served
title: How Extension Docs Are Served
---

The documentation for Chrome extensions and apps is served at
[developer.chrome.com/extensions](https://developer.chrome.com/extensions) and
[developer.chrome.com/apps](https://developer.chrome.com/apps) by Google App
Engine.

See
<https://cs.>[chromium.org/chrome/common/extensions/docs/README](http://chromium.org/chrome/common/extensions/docs/README)
to learn how the extension/app docs are generated.

## Contributing documentation

Have you found an error in the documentation? Either submit a patch (see below)
or file a bug with label `Cr-Platform-Extensions` at the [the issue
tracker](https://code.google.com/p/chromium/issues/list).

1.  Check out Chromium's source code, see [Get the Code: Checkout,
            Build, Run & Submit](/developers/how-tos/get-the-code) (`gclient
            runhooks` is optional for this purpose).
2.  Find the relevant file that you need to edit (e.g. by visiting
            [cs.chromium.org](http://cs.chromium.org) and pasting a quoted
            snippet from the page that you want to edit).
3.  To view the documentation locally, run
            `src/chrome/common/extensions/docs/server2/preview.py` (this will
            serve the documentation at http://localhost:8888, add `-p [port]` to
            serve on a different port).
4.  After updating the documentation, upload the patch to
            codereview.chromium.org (see also: [Contributing
            Code](/developers/contributing-code)).
5.  Others can preview your changes by visiting
            `https://chrome-apps-doc.appspot.com/_patch/*[issue
            id]*/extensions/` (or /apps/).
