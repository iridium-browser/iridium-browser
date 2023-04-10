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
page_name: docs
title: Extension Documentation System
---

[TOC]

If you're interested in contributing to the developer docs for extensions (or
you're just curious how it works), you've found the right place.

## How you can contribute to the doc

You can help improve the extension docs for Google Chrome in a couple of ways:

*   [File a bug](http://crbug.com/) about each doc
            mistake/omission/improvement
    *   If possible, tell us the exact text you'd like to see
*   Fix the doc yourself
    *   See [Contributing
                Code](http://www.chromium.org/developers/contributing-code) for
                general rules
    *   Note that pages could take months to show up in the default doc.
                To get around this, someone needs to merge your changes into the
                various branches.

## Overview of the doc system

Although the doc appears to be on code.google.com, it isn't. It's really in the
Chromium project and served up by an AppEngine app. To modify the doc, you need
to build it.

For details on where doc content comes from (from the server's point of view),
see [How Extension Docs Are
Served](/developers/design-documents/extensions/how-the-extension-system-works/docs/how-docs-are-served).
