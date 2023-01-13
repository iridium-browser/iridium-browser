---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: cscope-emacs-example-linux-setup
title: 'cscope/emacs: example Linux  setup'
---

This is a simple recipe for being setting up jump-to-file (for #includes) and
jump-to-declaration (for symbol names) in emacs, on gprecise (should work on
most posix systems).

*   Install cscope: `sudo apt-get install -y cscope cscope-el`
*   Place the attached cscope-update script in the directory containing
            `.gclient` (parent of `src/`).
*   Run `./cscope-update` after each `gclient sync` to update the cscope
            database
*   Load the attached `cscope.el` from your `.emacs`; note you'll
            probably want to edit this with your own client roots in
            `ami-src-root-for-buffer`.
*   While browsing source, hit meta-. (period) to jump to the file
            `#include`d at point or to the declaration of the symbol at point.
