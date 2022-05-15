---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: first-run-customizations
title: First Run Customizations
---

Chromium can be customized to perform certain behaviors during its first run.
The key to it is a special file named ***master_preferences*** that must exist
before chrome first run in the same folder as the chrome.exe binary.

The master_preferences file is a text file of JSON format, which needs to
contain some specific name/value pairs. A prototypical file is shown here (for a
more exhaustive list consult the [source
code](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/installer/util/master_preferences.h?view=markup)
directly):

{

"homepage" : "http://www.chromium.org",

"homepage_is_newtabpage" : false,

"distribution": {

"create_all_shortcuts": true,

"import_history": false,

"import_bookmarks": false,

"import_home_page": false,

"import_search_engine": false,

"do_not_launch_chrome": false

},

"first_run_tabs": \[

"http://example.com",

"http://welcome_page",

"http://new_tab_page"

\]

}

There is one dictionary, called "distribution" and one list, called
"first_run_tabs". Setting the home page behavior is done outside of these.

The home page settings control the following behaviors:

*   homepage: when set, this is the URL opened as the home page (see
            setting below.)
*   homepage_is_newtabpage: if set to false, clicking the 'home' button
            or taking actions that send the user 'home' will open the home page
            instead of the new tab page.

The distribution dictionary controls the following behaviors:

*   create_all_shortcuts: if set to true, the desktop, quick-launch and
            start menu shortcuts are created without user intervention.
*   import_history: if set to true it automatically imports the browser
            history from the current default browser.
*   import_bookmarks: if set to true it automatically imports the user's
            bookmarks from the current default browser.
*   import_home_page: if set to true it automatically imports the user's
            home page from the current default browser.
*   import_search_engine: if set to true it automatically imports the
            user's search engine setting from the current default browser.
*   do_not_launch_chrome: if set to true, per-user Chromium will not
            automatically launch after it is installed (per-machine Chromium
            never auto-launches after install).

The first_run_tabs list controls the set of urls that are displayed in chromium
when first run. If you want the default behavior the entire section can be
removed. Each entry can be a standard url or you can use these special names to
display special tabs:

*   "http://welcome_page" : shows the default welcome page for the
            current locale.
*   "http://new_tab_page" : shows the new tab page. Which contains the
            thumbnails of the favorite pages.
