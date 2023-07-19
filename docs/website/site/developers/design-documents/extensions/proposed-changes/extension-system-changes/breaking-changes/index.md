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
- - /developers/design-documents/extensions/proposed-changes/extension-system-changes
  - Extension System
page_name: breaking-changes
title: Breaking Changes
---

Our general philosophy for the Chrome extensions system is to try hard to avoid
breaking (ie backwards-incompatible) changes. Over time we've discussed a number
of ideas for improvements based on actual experience with the extensions system
that would require breaking backwards compatibility. Ideally we'd do them all
together in one big batch to minimize developer pain.

In no particular order, here's a list of these ideas. Some require changes to
the manifest format, and some change the behavior of existing APIs.

*   We want to reduce the need for background pages to help save memory,
            and some ideas for how to do this require breaking changes. See [a
            list of related
            bugs](http://code.google.com/p/chromium/issues/list?q=TaskForce%3DBackgroundPagesMustDie).
*   Split out host permissions and API permissions into separate fields
            ([crbug.com/62898](http://crbug.com/62898))
    *   We might want to design for extensibility in these fields, for
                example to allow things like adding an optional
                "why"/"justification" field for each permission.
    *   Also, for some API's such as spell checking, you want to grant
                permissions at the host level to get events on input into text
                boxes but don't necessarily want the ability to do cross-origin
                XHR or chrome.tabs.executeScript.
        *   (aa says:) In that case, the permission should be different.
                    A host permission means authenticated access to the host. We
                    shouldn't twist the meaning per-API.
*   Improve security in content scripts by either forcing the following
            behaviors, or making them the default with explicit developer
            opt-out:
    *   forbid eval
    *   turn on javascript strict mode
                (<https://developer.mozilla.org/en/JavaScript/Strict_mode>)
    *   turn on CSP
                (<http://www.w3.org/Security/wiki/Content_Security_Policy>)
    *   If a content script injects a &lt;script&gt; tag into the DOM,
                only allow it if the src is an https url
*   Split permissions into read and write (Context: captureVisibleTab
            really requires &lt;all_urls&gt; because iframes can have content
            from any site, but we don't want to require screenshot extensions to
            get write-all permissions when they don't need/want it. There may be
            other cases like this.)
*   Move tabs.captureVisibleTab into its own top-level permission to
            simplify concerns with iframe permissions
*   Rationalize naming conventions. Excepting special circumstances, we
            probably want to stick to the [Google javascript style
            guide](http://google-styleguide.googlecode.com/svn/trunk/javascriptguide.xml):
    *   functionNamesLikeThis, variableNamesLikeThis,
                ClassNamesLikeThis, EnumNamesLikeThis, methodNamesLikeThis, and
                SYMBOLIC_CONSTANTS_LIKE_THIS
    *   Unify our handling for abbreviations like URL - we have
                functions named getURL, setUpdateUrlData, addUrl, etc. We should
                standardize on URL or Url.
*   Have a "tabs_simple" or "tabs_light" permission for things like
            chrome.tabs.{create,update,remove,onRemoved}
*   Allow extensions to have opt-in "public" pages that can be loaded in
            iframes, window.open, etc. from within other origins, and make pages
            private by default.
*   Fix the port madness in URLPattern. Can we make all URLPatterns
            support ports? I believe right now, most cases of URLPatterns make
            ports an error, which means we can make it work without a breaking
            change. But in a few cases, ports are silently allowed for backward
            compatibility. Fixing this would be the breaking change.
*   Change chrome://favicon/ to a new scheme chrome-favicon:// (this was
            the root of a security bug, <http://crbug.com/83010>)
    *   We should just make this a separate top-level API permission. We
                don't need or want authenticated access to these origins - we
                just want the icons.
    *   Some work on this is in-progress: crbug.com/172375
*   Allow better URL pattern matching (to differentiate between path,
            query and fragments): <http://crbug.com/84024>
*   Change the manifest format in any ways needed to let us use [JSON
            Schema](http://json-schema.org/) for parsing it.
    *   Note: we already have an internal C++ implementation of JSON
                Schema that is used for other things in the extension system
*   Scan extension.cc for DEPRECATED and the like. Remove old stuff like
            the support for multiple pageActions.
