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
page_name: executecontentscript-proposal
title: executeScript() and executeCSS()
---

### Problem

Sometimes, extension developers don't need to execute content script in a page
until some event happens in the extension. It is wasteful to execute content
script in every web page on the off chance that this event will happen.

For example, consider an extension that wants to translate web pages. There will
be a button in the UI that enables translation. When this button is pressed, the
DOM needs to be crawled, collecting text nodes, and concatenating them for
translation. However, it would be nice to avoid running content scripts until
this button is pressed.

### Proposal

chrome.tabs

// Executes JavaScript in the root frame of a tab. The script is executed in the
same type of

// sandbox that content scripts are.

//

// Each time this method is called a new sandbox is created to run the script
and the sandbox

// persists following the usual rules of GC. This isn't particularly
heavyweight, but it also

// isn't something you'd want to do in a loop.

//

// If you need lots of small communication with a tab, consider executing script
once and

// then using content script messaging to communicate between that script and
the extension.

//

// tabId: Optional, defaults to selected tab of the current window (as
determined by

// chrome.windows.getCurrent()).

// js: Optional, string of JavaScript to execute.

// js_files: Optional, array of JavaScript files to execute, in order.

// callback: Optional, callback to invoke with result of execution (result of
last statement).

//

// If the extension does not have access to interact with the URL in the
specified tab, or if a

// js file is specified that does not exist, an error is reported to the console
associated with the

// view that called the API. Execution errors in the script are reported to the
console associated

// with the tab.

void **executeScript**(\[int tabId\], {\[string js\], \[string\[\] js_files\]},
\[callback onSuccess(Object result)\]);

// Applies CSS to the root frame of a tab.

//

// tabId: Optional, defaults to selected tab of the current window (as
determined by

// chrome.windows.getCurrent()).

// css: Optional, string of CSS to execute.

// css_files: Optional, array of CSS files to execute, in order.

void **insertCSS**(\[int tabId\], {\[string css\], \[string\[\] css_files\]});

Example

chrome.tabs.executeScript(null, {js_files: \["jquery/min.js", "woo.js"\]});

### Details

*   When we implement permissions, this API should fail if the extension
            doesn't have access to the page loaded into the tab.
*   No attempt is made to dedupe these. Each time you call the API, the
            script gets executed again. Callers need to maintain state if they
            don't want to execute duplicate scripts.
*   Executing script in subframes is not supported.
