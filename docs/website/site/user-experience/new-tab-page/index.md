---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: new-tab-page
title: New Tab Page
---

[TOC]

The new tab page is the default starting point for all tabs - it is designed to
get the user where they want to go, and is not meant to be an information
resource like the user's home page; that is, the new tab page is not intended to
be a destination, but rather a jumping-off point to other destinations - we
strongly want to avoid cognitive load and distractions for the user, especially
those creating new tabs for other purposes.

## Sections

The new tab page is made up of several sections; these sections will vary in
size and presentation method as we figure out which sections are and are not
useful.

*   **Most Visited**: A grid of thumbnails showing the user's nine most
            frequently visited sites.
*   **Searches**: Like most visited, the searches section shows your
            most frequent searches. Currently, this is a list of the most
            frequently used keyword searches, but will be expanded to include
            all form-field based searches.
*   **Recent Bookmarks**: A list of the user's nine most recently
            created bookmarks.
*   **Recently Closed Tabs**: A list of up to three tabs that have been
            closed within the last X minutes. Clicking an item here will restore
            the tab to the same state it was in when it was closed.

## Future Work

*   Show the end-points of navigations (if you always click on the same
            link after going to a root page, why not show the linked page
            instead).
*   Allow control over the contents of the grid
*   Vary content depending on context (time of day, IP block, etc)

## Experimenting with the New Tab Page

It is relatively easy to make edits to the new tab page as its frontend is
constructed with HTML.

To edit the HTML, look for browser\\resources\\new_tab.html - the backend for
this page is defined in browser\\dom_ui\\new_tab_ui.cc. After making HTML
changes, you'll need to recompile the resources:

> [<img alt="image" src="/user-experience/new-tab-page/new_tab_resources.png"
> height=420 width=142>](/user-experience/new-tab-page/new_tab_resources.png)

1.  Right-click new_tab.html and select **Compile**.
2.  Right-click browser_resources.rc and select **Compile**.
3.  Press F5 to run Chromium.

You must follow this process for all HTML changes.
