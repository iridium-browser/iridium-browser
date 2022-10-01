---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/network-stack
  - Network Stack
- - /developers/design-documents/network-stack/network-bug-triage
  - Network bug triage
page_name: downloads-bug-triage
title: Triaging Downloads Bugs
---

[Downloads
bugs](https://code.google.com/p/chromium/issues/list?can=2&q=cr-ui-browser%3Ddownloads&sort=&groupby=&colspec=ID+Pri+Status+Modified+Owner+Summary&nobtn=Update)
are automatically cc'd to the mailing list download-bugs@chromium.org. People on
the bugs rotation should subscribe to that list. The suggested frequency for
handling incoming bugs is 2-3 days, but that's just a suggestion.

Triagers should

1.  Attempt to reproduce and set the correct Status for [untriaged and
            unconfirmed
            bugs](http://crbug.com/?q=cr-ui-browser%3Ddownloads+status%3Auntriaged%2Cunconfirmed+-blocking%3A96604+-needs%3Dfeedback&colspec=ID+Summary+Modified).
            (That search ignores Needs-Feedback and External bugs.)
    *   Engage with users as they report bugs to get all the information
                we need for eventual resolution.
    *   Set Type-Feature Available for feature requests or mark WontFix
                and explain why this feature is unlikely to be implemented.
    *   Crash reports frequently do not contain crash ids. Send
                reporters to [Reporting a Crash
                Bug](http://www.chromium.org/for-testers/bug-reporting-guidelines/reporting-crash-bug)
                and set Needs-Feedback.
    *   [Providing Network Details for bug
                reports](/for-testers/providing-network-details)
    *   Ensure high priority bugs receive appropriate attention. Consult
                benjhayden or asanka if necessary in order to ensure timely
                resolution. High priority bugs include
        *   Regressions: Mark as Type-Bug-Regression Pri-1
        *   Possible security problems: Request security review and lock
                    down (Restrict-View-Commit). If security review shows
                    severity medium or higher, mark Pri-1
        *   Crashers: Mark as Pri-1 if it's on the top crash list or
                    looks likely to happen in the wild. If it looks like a
                    use-after-free that might be reproducible in the wild, it's
                    a security issue.
            *   [chromecrash
                        query](https://chromecrash.corp.google.com/browse?q=int32(extract_regexp(product.version%2C%20r'%5Cd%2B%5C.%5Cd%2B%5C.(%5Cd%2B)%5C.%5Cd%2B'))%20%3E%3D%200%20AND%0Acustom_data.ChromeCrashProto.ptype%3D'browser'%20AND%0A(custom_data.ChromeCrashProto.magic_signature_1.component%20CONTAINS%20'src%2Fcontent%2Fbrowser%2Fdownload'%20OR%0A%20custom_data.ChromeCrashProto.magic_signature_1.component%20CONTAINS%20'src%2Fchrome%2Fbrowser%2Fdownload'%20OR%0A%20custom_data.ChromeCrashProto.magic_signature_1.file_path%20CONTAINS%20'src%2Fchrome%2Fbrowser%2Fextensions%2Fapi%2Fdownloads'%20OR%0A%20custom_data.ChromeCrashProto.magic_signature_1.file_path%20CONTAINS%20'src%2Fchrome%2Fbrowser%2Fui%2Fwebui%2Fdownload')):
                        fiddle with the literal 0 to find crashes
                        before/after/at a specific branch.
        *   Badly broken features
        *   Failing or disabled tests
    *   Mark duplicates as such. [Frequently duplicated
                bugs](/developers/design-documents/network-stack/network-bug-triage/downloads-bug-triage/salient-bug-list)
        *   Some issues on windows are due to bad shell extensions.
                    Point reporters to
                    [ShellMenuView](http://www.nirsoft.net/utils/shell_menu_view.html)
2.  Categorize [uncategorized
            bugs](http://crbug.com?q=cr-ui-browser%3Ddownloads+-blocking%3A68191+-blocking%3A68195+-blocking%3A68196+-blocking%3A68197+-blocking%3A68198+-blocking%3A68200+-blocking%3A68201+-blocking%3A68204+-blocking%3A68206+-blocking%3A68208+-blocking%3A68209+-blocking%3A68276+-blocking%3A68356+-blocking%3A68358+-blocking%3A68359+-blocking%3A68361+-blocking%3A69298+-blocking%3A78147+-blocking%3A78148+-blocking%3A96604+-blocking%3A133971+-blocking%3A133960&colspec=ID+Summary+Modified)
            by adding them to the "Blocked on" list of one of [these category
            bugs](http://crbug.com/?q=Cr-Ui-Browser%3DDownloads+blocking%3A133960&colspec=ID+Summary).
3.  Sweep
            [needs=feedback](https://code.google.com/p/chromium/issues/list?can=2&q=cr-ui-browser=downloads%20needs=feedback&sort=modified&colspec=ID%20Status%20Owner%20Summary%20Modified):
            if feedback has been provided, remove needs-feedback and continue
            debugging; if feedback has not been provided after 2 weeks, Archive
            the bug.
4.  When major changes such as file moves happen, either update the
            below documentation or delete it if nobody has found it useful.

FAQ, Bug-hunting tips

Where to begin fixing different kinds of bugs. Please feel free extend this
liberally.

*   Main entry point from the ResourceDispatcherHost to the downloads
            system:
            [BufferedResourceHandler](https://code.google.com/p/chromium/codesearch#chromium/src/content/browser/loader/buffered_resource_handler.cc)
            decides whether RDH should create
            [DownloadResourceHandler](https://code.google.com/p/chromium/codesearch#chromium/src/content/browser/download/download_resource_handler.cc)
*   The download state machine is managed on the UI thread in
            [DownloadItemImpl](https://code.google.com/p/chromium/codesearch#chromium/src/content/browser/download/download_item_impl.cc),
            managed by
            [DownloadManagerImpl](https://code.google.com/p/chromium/codesearch#chromium/src/content/browser/download/download_manager_impl.cc).
*   chrome://downloads
            [WebUI](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/browser/ui/webui/downloads_dom_handler.cc)
            [HTML/CSS/JS](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/browser/resources/downloads/)
*   [Target filename
            determiner](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/browser/download/download_target_determiner.cc)
*   [DownloadHistory](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/browser/download/download_history.cc)
            observes the DownloadManager and DownloadItems and posts changes to
            [DownloadDatabase](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/browser/history/download_database.cc)
*   chrome.downloads Extension API
            [IDL](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/common/extensions/api/downloads.idl),
            [implementation](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/browser/extensions/api/downloads/downloads_api.cc)
*   Multiple automatic download throttling infobar
            [DownloadRequestLimiter](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/browser/download/download_request_limiter.cc)
