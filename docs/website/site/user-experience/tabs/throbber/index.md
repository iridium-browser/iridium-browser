---
breadcrumbs:
- - /user-experience
  - User Experience
- - /user-experience/tabs
  - Tabs
page_name: throbber
title: Throbber
---

[<img alt="image"
src="/user-experience/tabs/throbber/throbber.png">](/user-experience/tabs/throbber/throbber.png)

We use throbber in the favicon area of the tab to indicate that a page is
loading.

A single-state throbber could make Chromium feel slow because it would hide the
not-our-fault steps in the load process - it would lump DNS wait, server contact
wait, data transfer, and rendering into one animation blob. Server/network
slowness would therefore affect perception of Chromium's overall speed.
