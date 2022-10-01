---
breadcrumbs:
- - /user-experience
  - User Experience
page_name: infobars
title: Infobars
---

# **## **## **## **## Infobars are non-modal, informational messages that display as an extension of the chrome. They are displayed when something happens in the Tab that the user should be aware of, e.g. a plugin has crashed, or the user can save a password they just entered. An infobar will push the website down so as to not block any content.Variants********

# **#### **#### **#### **## Usage********

# **## **There are two main scenarios for dispalying an infobar:****
# **## **1) Page-related actions - to message the user about an action they can take related to the page they are on****

# **## ******## 2) Confirmations / warnings - to provide a notification or alert about a problem or a completed action********
# **## ******## Infobars should never be purely informational; they should always have an action the user can take. Additionally, actions should be optional. If they are blocking the user from doing something else, a dialog should be used instead.********

# **#### **## Types****

# **## **## 1) Page-related actions****

# **## **## Permissions - getting the user to agree to or permit an action (e.g. sharing their location so it can be used with the site they are on)****

# **## **## **## **## Page action - asking the user if they want to perform an action on a page that the user is currently on (e.g. translating a page)********

# **## **## **## **## **## **## Feature setup - promoting a feature that's related to content on the current page; additionally giving them a way to set it up (e.g. setting up Autofill feature by saving current page's form info)************

# **## **## **## **## 2) Confirmations/Warnings********

# **## **## Confirmation - a success confirmation of action/installation (e.g. confirmation of an extension or theme installation with ability to undo)****

# **## **## Problem - informing the user of a problem that occurred with a way for them to fix/troubleshoot (e.g. notification of extension crashing with option to restore; notification of missing plug-in with a way to install)****

# **## **## #### **## **## When & Where to display********

# **## In most cases, infobars should not be displayed in direct response to a user action (i.e. they should be system-initiated). If a user performs an action that directly responds with a message of the types listed above, a lightweight dialog should be used instead. The exception to this is the confirmation infobar which is usually in direct response to the user choosing to install something.**
# **## Infobars with page-related actions should only show on the pages to which they are relevant. Confirmation/warning infobars should only show on the page that they first appeared. The infobar can persist on that tab/page as long as it makes sense to stay open or until the user closes it out.**

# ****#### ****Visual********

# ****Colors****

# **From Chrome/3rd party (PC)**

# **[<img alt="image"
src="/user-experience/infobars/blue-gradient.png">](/user-experience/infobars/blue-gradient.png)**

# **From Chrome/3rd party (Mac)**

# **[<img alt="image"
src="/user-experience/infobars/grey-gradient.png">](/user-experience/infobars/grey-gradient.png)**

# ****Confirmation/warning****

# ****[<img alt="image"
src="/user-experience/infobars/yellow-gradient.png">](/user-experience/infobars/yellow-gradient.png)****

# ****Controls****

# ****### [<img alt="image"
src="/user-experience/infobars/controls.png">](/user-experience/infobars/controls.png)****

# **Note: The Options menu on an infobar only shows the encasing button on hover
and press, similar to the page and wrench menus in Chrome.**

# ****Spacing****

# **infobar height = 36 px**

# **left and right padding = 6 px**

# **spacing after icon = 6 px**

# **spacing between buttons = 10 px**

# **spacing after message (and before buttons) = 16 px**

# **spacing to left of x button (between x and learn more or Options) = 12 px**

# **### **### **Examples******

# **### ****### ****### **e.g. Page action infobar from************

# **### ****### ****### ****### ****### **Translate********************

# **### ****### ****### ****### ****### ****### **[<img alt="image"
src="/user-experience/infobars/example-translate.png">](/user-experience/infobars/example-translate.png)************************

# **### **e.g. Confirmation./page action infobar with more options****

# **### **<img alt="image"
src="/user-experience/infobars/example-translate-confirm.png">****

# **### ****### ****### **e.g. Problem infobar************

# **### ****### ****### ****### **[<img alt="image"
src="/user-experience/infobars/example-ext-crash.png">](/user-experience/infobars/example-ext-crash.png)****************

# ****#### ****Content********

# ****Font****

# **\[system font\]**

# **(Tahoma 13px shown in mocks)**

# ****Message wording****

# **Infobar messages should be clear and succinct. If possible, the message
should be able to fit on one line of the infobar and still retain enough space
for controls, options/learn more and a close button. There may be cases where
multiple infobars get stacked so messages should always be clear about where
(which feature/app) the message is coming from.**

# ****Button wording****

# **The button that performs the main action should convey in wording what
action will take place. The button that declines the action should simply say no
if the preceding text contains a question. If the infobar message does not
contain a question, then the button should convey what they are refusing.**

# ****Examples by type:****

# **Permissions/Page Actions - explanation of who/what is asking and what they
are asking permission for**

> # **e.g. Flickr wishes to use your location. \[Share my location\] \[Don't
> share\]**

> # **e.g. This page is in French. Would you like to translate it? \[Translate\]
> \[No thanks\]**

# **Feature setup - explanation of what feature is and/or value + question
asking if user wants to perform action**

# **e.g. Chrome's Autofill feature can help you fill out web forms faster. Would you like to save this form info? \[Set up Autofill...\] \[Nope\]**

# **Confirmation - confirmation of completed action**

> # **e.g. The "Mappy" extension was successfully installed. \[Undo\]**

> # **e.g. This page has been translated from \[French\] to \[English\]. *Note:
> The controls are dropdowns in this example.***

# **Problem - explanation of problem**

# **e.g. The "Dictionary" extension crashed. \[Restore\]**

# **### ****Options / Learn more / Help******
# **### **If an infobar is substantial enough to have an options menu, put the
Help or Learn more link within that menu. Otherwise, any infobar that can
provide additional explanation of the feature or message should give more
information in a "learn more" or "About \[feature name\]" link.****

# **#### ****Persistence and Crossfading******

# ******Persistence******

# ****In general, to avoid the up and down bouncing of new infobar instances,
infobars related to the same action instance should persist.****

# ****For example, translating a page involves multiple infobars. The first
allow the user to translate the page, and the second informs the user that the
page has been translated AND offers the ability to do another translation. When
the user clicks on the affirmative button in the first infobar, the infobar will
not bounce up and return with the second infobar. Instead, the infobar will
persist, and the content on the infobar will change.****

# **In a case where there is a persistent infobar and the color of the infobar
must change, the first color will crossfade into the second color over 500 ms.**

# **For example, if a user tries to translate a page using the blue translation
bar and an error occurs, the blue infobar will crossfade into a yellow infobar
outlining the issue and an ability to try translating again.**

# ****#### ****Dialogs********

# ******A dialog should be used instead of an infobar if...******

# ******1) it will block the user from doing something unless he/she takes
action******

# **2) it is a direct response to a user action**

# **e.g.**

# **[<img alt="image"
src="/user-experience/infobars/dialog.png">](/user-experience/infobars/dialog.png)**

# ****#### ****## **Variants**********

# ****## ****## [Geolocation](/user-experience/infobars/geolocation)********
