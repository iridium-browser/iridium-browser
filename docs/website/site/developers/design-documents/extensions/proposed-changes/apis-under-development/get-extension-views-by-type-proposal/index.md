---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/proposed-changes
  - Proposed & Proposing New Changes
- - /developers/design-documents/extensions/proposed-changes/apis-under-development
  - API Proposals (New APIs Start Here)
page_name: get-extension-views-by-type-proposal
title: Get Views by Type
---

### Problem

Right now we have chrome.extension.getViews() which returns a list of all active
views (toolstrips, tab contents, the background page) in the extension. This is
cool, but you also frequently need to get all views of a certain type. For
example, all toolstrips, or just the background page. Also, people frequently
want to get the toolstrips associated with a particular window.

### Proposal

chrome.extension

// Returns an array of DOMWindows for the toolstrips running in the current
extension.

//

// windowId: optional. If specified only returns toolstrips from that window. If

// omitted, defaults to the current window as defined by
chrome.windows.getCurrent().

// Can also be the string "all", in which case all toolstrips in all windows
will be

// returned.

DOMWindow\[\] **getToolstrips**(\[int-or-string windowId\]);

// Returns an array of DOMWindows for any tabs running in the current extension.

//

// windowId: optional. If specified only returns toolstrips from that window. If

// omitted, defaults to the current window as defined by
chrome.windows.getCurrent().

// Can also be the string "all", in which case all toolstrips in all windows
will be

// returned.

DOMWindow\[\] **getTabs**(\[int-or-string windowId\]);

// Returns the DOMWindow for the extension's background page, or null if the
extension

// doesn't have a background page.

DOMWindow **getBackgroundPage**();

### Example

// Assuming the extension has a background page and has a global function called
"foobar".

chrome.extension.getBackgroundPage().foobar();
