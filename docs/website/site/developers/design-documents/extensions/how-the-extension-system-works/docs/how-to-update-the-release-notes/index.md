---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/extensions
  - Extensions
- - /developers/design-documents/extensions/how-the-extension-system-works
  - How the Extension System Works
- - /developers/design-documents/extensions/how-the-extension-system-works/docs
  - Extension Documentation System
page_name: how-to-update-the-release-notes
title: Updating the Release Notes
---

This page has some tips on figuring out what's changed between two releases.
I've used this technique to update the [What's
New](http://code.google.com/chrome/extensions/whats_new.html) page in the
extensions doc, but the technique is more generally useful.

To see what's changed between two releases (e.g. M9 and M10):

1.  Go to <https://omahaproxy.appspot.com/viewer>, and get the current
            versions of the two releases. E.g.:
    1.  M9 (win stable): 9.0.597.107
    2.  M10 (win beta): 10.0.648.119
2.  Go to <https://omahaproxy.appspot.com/changelog>, enter the versions
            in the fields, and click **Get SVN logs**. E.g.:
    1.  Old Version: 9.0.597.107
    2.  New Version: 10.0.648.119
    3.  &lt;click!&gt;
        You'll see a big page of revisions, with a revision range (e.g.
        67679:72316).
    4.  Search for interesting strings in this page, and open relevant
                links to check whether they're worth talking about.
        **Note:** This tells you the differences between these branches at the
        time the branches were created. If you substitute 1 for the last tuple
        of each version #, you get exactly the same information.
3.  Do #2 again, but this time change the old version to be the same as
            the new version, but with a 1 as the last tuple. E.g.:
    1.  Old Version: 10.0.648.1
    2.  New Version: 10.0.648.119
    3.  &lt;click!&gt;
        You'll see another page of revisions with a different (higher) revision
        range (e.g. 72323:75907).
    4.  Search for interesting strings in this page.
4.  Once you have a list of probable changes, make sure they weren't
            merged into the previous release. One way to do this is to do #3
            again, using the previous release's numbers. E.g.:
    1.  Old Version: 9.0.597.1
    2.  New Version: 9.0.597.107
    3.  &lt;click!&gt;
    4.  Search for the revision #s you found interesting in steps 2 & 3.
                If you find something that *was* backported, make sure it's in
                the release notes for the old release (if it's worthy).

The most useful strings strings to search for (for extensions/apps release
notes):

*   src/chrome/common/extensions/
*   content script
*   theme \[haven't done this in the past, but we really should\]

I used to also/instead search for these:

*   extension_api.json
*   permission
*   manifest
*   extensions/docs
*   packaged app
*   hosted app
*   apps
*   theme
*   extension

Don't bother searching for "api".
