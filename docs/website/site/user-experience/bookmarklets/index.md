---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: bookmarklets
title: Bookmarklets
---

Many sites implement sharing/saving features through
[bookmarklets](http://www.bookmarklets.com/about/). Examples include Delicious,
Facebook, Digg, and Google Reader. However, bookmarklets are hard to install,
since they require dragging a link to the bookmarks bar. The following proposal
builds on bookmarklets to enable simple sharing of pages in Chrome.

## **Basic state**

We move the star to the right-side of the Omnibox, and add a drop-down menu:

[<img alt="image"
src="/user-experience/bookmarklets/default.png">](/user-experience/bookmarklets/default.png)

... that contains a list of bookmarklets:

[<img alt="image"
src="/user-experience/bookmarklets/menu.png">](/user-experience/bookmarklets/menu.png)

In this case, the "Email this page" action is basically a mailto: link with the
subject and content prefilled.

### Installing bookmarklets

Just like in other browsers, users can install a bookmarklet by dragging it onto a drop-target (in this case, the drop-down arrows). This dragging action will be interpreted as an implicit permission to install the bookmarklet, and thus won't require a permissions dialog.
However, to make it easier to install bookmarklets, developers can also add
rel="bookmarklet" to their bookmarklet &lt;a&gt; tags. This won't affect their
behavior in other browsers. However, when the user clicks on such a link in
Chrome, they will be presented with the following permissions dialog (exact copy
TBD):

[<img alt="image"
src="/user-experience/bookmarklets/install_dialog.png">](/user-experience/bookmarklets/install_dialog.png)

### **Managing bookmarklets**

Bookmarklets can be managed by right-clicking on them in the drop-down menu:

[<img alt="image"
src="/user-experience/bookmarklets/menu_context.png">](/user-experience/bookmarklets/menu_context.png)

We may also provide an option to promote bookmarklets to the top level, in which
case they will appear next to the star. Only bookmarklets with a favicon can be
promoted in this way.
