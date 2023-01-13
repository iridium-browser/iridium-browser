---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: applescript
title: AppleScript Support
---

## Introduction

AppleScript is a widely used scripting language on Mac OS X, used for
inter-process communication (IPC). It enables end-users who have no programming
skills to easily create workflows and perform repetitive tasks. Supporting
AppleScript also provides a consistent interface for communication with other
applications.

If you are unfamiliar with AppleScript, you can read more about it at
[AppleScript Language
Guide](http://developer.apple.com/mac/library/documentation/AppleScript/Conceptual/AppleScriptLangGuide/introduction/ASLR_intro.html#//apple_ref/doc/uid/TP40000983).

For implementation of AppleScript in your application, you might want to look at
[Cocoa Scripting
Guide](http://developer.apple.com/mac/library/documentation/cocoa/conceptual/ScriptableCocoaApplications/SApps_intro/SAppsIntro.html).

## Terminology

*   SDEF - scripting definition file, contains all the classes,
            properties, commands that scripters can use.
*   Automator - an application bundled with Mac OS X allowing the user
            to graphically build workflows on top of AppleScript.

## Details

AppleScript support involves five classes, each representing the classes defined
in the sdef file:

ApplicationApplescript

*   Not a separate class but a set of methods added in a category of
            BrowserCrApplication
*   The name, frontmost, and version properties are all obtained by work
            done in NSApplication
*   An application has windows as its elements
    *   The windows are obtained by traversing through BrowserList,
                wrapping each Browser in a WindowApplescript class and returning
                it as an NSArray
    *   A new window is created by creating a new Browser with a single
                blank tab
    *   A window is closed by calling browser-&gt;window()-&gt;Close()
*   Bookmarks are represented as two folders elements, corresponding to
            the bookmarks bar and the "Other Bookmarks" folder

### WindowApplescript

*   Encapsulates a Browser object
*   ID of a browser is obtained via browser-&gt;session_id().id()
*   mode indicates whether a browser window is a normal or an incognito
            window. The mode is set only during creation and cannot be changed
            at a later time. The default is a normal window.
*   The NSWindow associated with a browser can be used to get and set
            certain properties such as bounds, closeable, miniaturizable,
            miniaturized, resizable, visible, zoomable, and zoomed.
*   A browser window has tabs as its elements
    *   The tabs comprising a browser window are obtained by getting
                each tab using browser-&gt;GetTabContentsAt(), wrapping it
                inside a TabApplescript class, and returning them as an NSArray
    *   New tabs are made by using browser-&gt;AddTabWithURL()
    *   Tabs are deleted using
                browser-&gt;tabstrip_model()-&gt;DetachTabContentsAt()

### TabApplescript

*   Encapsulates a TabContents object
*   ID of a tab is obtained via
            tab_contents-&gt;controller().session_id().id()
*   The url and title associated with a tab can be mutated and accessed
            through the TabContents' controller.
*   Standard text-manipulation commands such as cut, copy, and paste are
            achieved by calling methods on the RenderViewHost associated with a
            particular tab
*   Standard navigation commands such as go back, go forward, and reload
            are achieved by calling methods on the controller associated with a
            particular tab
*   Printing a tab is done by calling tabContents-&gt;PrintNow()
*   Saving a tab is done by calling tabContents-&gt;SavePage(). The
            caller has to provide parameters such as file name and directory
            path, otherwise the user is prompted for these values

### BookmarkFolderApplescript

*   Encapsulates a BookmarkNode object which represents a folder
*   ID is obtained via bookmarkNode-&gt;id()
*   The title can be mutated and accessed by calling methods on the
            BookmarkModel, which takes care of obtaining the correct locks and
            other housekeeping details.
*   A bookmark folder has other bookmark folders and bookmark items as
            its elements
    *   Any child bookmark folders are obtained by getting all the child
                nodes of type folder, wrapping each up inside a
                BookmarkFolderApplescript class, and returning them as an
                NSArray
    *   Adding and deleting bookmark folders and items is done by
                calling appropriate methods on the BookmarkModel which takes
                care of all internal details like obtaining locks etc
    *   The bookmark items are obtained by getting all child nodes with
                type as url, wrapping each up in a BookmarkItemApplescript
                class, and returning them as an NSArray

### BookmarkItemApplescript

*   Encapsulates a BookmarkNode object which represents a single url
*   ID is obtained via bookmarkNode-&gt;id()
*   Each bookmark item consists of a title and url and can be
            manipulated by calling methods on BookmarkModel
