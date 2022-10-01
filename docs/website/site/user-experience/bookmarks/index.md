---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: bookmarks
title: Bookmarks
---

[TOC]

## Adding

[<img alt="image"
src="/user-experience/bookmarks/bookmark_add.png">](/user-experience/bookmarks/bookmark_add.png)

Bookmarks can be added using the star in the location bar. The dialog that
appears can be dismissed by focusing elsewhere, otherwise user actions take
place immediately.

The star may also be dragged onto the bookmarks bar in order to add a bookmark
to a specific location.

Bookmarks Bar

[<img alt="image"
src="/user-experience/bookmarks/bookmarks_bar.png">](/user-experience/bookmarks/bookmarks_bar.png)

The bookmarks bar is the home of all of the bookmarks the user creates. We
wished to avoid creating multiple separate locations for bookmarks as each
additional section is yet another location to lose things in. While this reduces
the categorization and access options, it covers the most frequent uses of
bookmarks.

The 'Other Bookmarks' folder is a special, right-aligned always-visible folder
that is a dumping ground for bookmarks that the user doesn't want to show in
their prime bookmarks area (the left side).
The bookmarks bar is not visible by default, but is shown as part of the new tab
page so that people who dislike the use of real estate can still easily access
their bookmarks at the start of a navigational task. It can be permanently shown
by user option, and toggled with Ctrl+B.

## Basic UX Issues

*   The toolbar has some interaction quirks:
    *   Cannot roll horizontally across menus (this also applies to
                Page/Chromium menus)
    *   Cannot open a menu by dragging downwards (we can't let this
                detract from drag to rearrange)
*   History view needs a 'show only starred pages' checkbox. This should
            be a simple fix as the backend supports this.
*   We need better filing controls in the add bookmark case.

## Common Feature Requests

Bookmarks management
Tagging
We can implement tagging as a layer on top of folders. More research needs to be
done here.
Multiple-location bookmarks
A consequence of having a 'star' toggle for a given URL is that the bookmark
location becomes a property of the URL, making it hard to locate a bookmark in
multiple places - if a bookmark existed in two folders, which folder would you
see if you went to that page and clicked on the star to edit the bookmark?
Other browsers have a similar model, with the following approach:

*   You can only create duplicate entries by dragging the favicon, or
            CTRL-dragging an existing bookmark.
*   The bookmark bubble only shows the folder of the last-created
            bookmark with the same URL.
*   Editing the name of a bookmark edits the visible entry of that
            bookmark and the name attribute of the bookmark properties, but not
            the visible names of the bookmark in other locations.
*   Creating a new duplicate (by dragging the favicon) will overwrite
            the name attribute, but not the tags and not the toolbar names of
            any bookmark.
*   Clicking **Remove bookmark** in the bookmark bubble removes **all**
            instances of the bookmark from the bookmarking system.

Multiple access points for 'add bookmark' functionality
We could add **Bookmark this page** to the context and page menus, though it's a
touchy issue given the extra space required.
