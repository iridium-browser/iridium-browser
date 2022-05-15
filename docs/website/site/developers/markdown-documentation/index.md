---
breadcrumbs:
- - /developers
  - For Developers
page_name: markdown-documentation
title: Markdown Documentation guide
---

New documentation related to code structure should be put in Markdown. The best
practices recommendation is to put a README.md file in the code directory
closest to your component or code.

To preview changes to Markdown files, see
[//src/docs/README.md](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/README.md).

Please write your Markdown in accordance with Google's [style
guide](https://github.com/google/styleguide/tree/gh-pages/docguide), and
[attempt to limit lines to 80 characters where
possible](https://groups.google.com/a/chromium.org/d/msg/chromium-dev/KECdEn562vY/sqRor1frEgAJ);
unfortunately, **git cl format** will not do this for you.

After committing the patch, you can view the rendered markdown using a URL like
https://chromium.googlesource.com/chromium/src/+/HEAD/&lt;my
path&gt;/README.md
([example](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/WebKit/Source/core/paint/README.md)).

[Here are some more
examples](https://cs.chromium.org/search/?q=file:readme.md+-file:/third_party/&type=cs)
to learn from.