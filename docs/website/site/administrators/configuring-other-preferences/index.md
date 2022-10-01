---
breadcrumbs:
- - /administrators
  - Documentation for Administrators
page_name: configuring-other-preferences
title: Configuring Other Preferences
---

Not all user preferences are managed through policy, typically because they do
not need to be managed centrally.

**Preferences vs. Policies**

Preferences and policies are two different methods for controlling the browser's
behavior. They have different purposes, however:

Preferences:

*   Keep state of a user's personal browsing experience.
*   Are usually unique to each user.
*   Are writable by the user, as they live in the user's directory in a
            text file.
*   Are often under-the-hood, technical settings that don't necessarily
            make sense to pre-set per user or lock in with policy.
*   Use the master_preferences file as a template if no Preferences
            already exist. **Preferences are copied from master_preferences
            ==only once==; changes to the master_preferences file made after
            that are not respected!**

Policies:

*   Are rules that the user's browsing experience must abide by.
*   Do not keep state of the user's experience.
*   Are usually applied to groups of users; they are not necessarily
            unique to each user.
*   Are not (typically) writable by the user.
*   Are clearly enumerated and are intended for the admin to use & set.
*   Are set only by the admin in special locations (registry, MCX,
            /etc/...).
*   Policies that should be editable by the user are called "recommended
            policies" and offer a better alternative than the master_preferences
            file. Their contents can be changed and are respected as long as the
            user has not modified the value of that preference themselves.

There are several notable preferences that are also policies, "homepage" being
the most common.

Policies take precedence to preferences and preferences take precedence to
recommended policies. If "homepage" is specified in both the policies, and in
the "master_preferences" file, policies will always override.

**Should I use a Preference, or a Policy?**

Prefer policies to configure Chrome on managed computers. Use recommended
policies for settings that users should be able to change, and use mandatory
policies for settings that users should not be able to change. A
master_preferences file may be used to make default settings for new users,
though doing so has some drawbacks:

*   The contents of a master_preferences file are copied once per
            profile at profile creation. As a result, it is not possible to
            automatically propagate modifications to a master_preferences file
            to users.
*   The preferences that may be set via master_preferences are not
            guaranteed to work in the future. It is possible that preferences
            may change or be removed such that values in master_preferences
            files would be ignored.

### The Gritty Details

Preferences are kept in a file named "Preferences", which every Chromium /
Google Chrome user will have in their own user directory. This Preferences file
is a text file that contains JSON markup. Going through and editing every user's
Preferences file to deploy a behavior change is really cumbersome and some
preferences are protected by cryptographic hashes and can not be manually
edited, so there are easier ways to manage this:

*   When users start Chromium / Google Chrome for the first time, they
            don't yet have any Preferences file.
*   A file named "master_preferences" located next to chrome.exe or
            chromium executable, is used as a template for what becomes users'
            Preferences file. On a system with Chrome installed from an MSI,
            this will be C:\\Program Files
            (x86)\\Google\\Chrome\\Application\\master_preferences
    *   On **Mac OS X**, for **Google Chrome**, this file is either of
                ~/Library/Application Support/Google/Chrome/Google Chrome Master
                Preferences or /Library/Google/Google Chrome Master Preferences
    *   On **Mac OS X**, for **Chromium**, this file is either of
                ~/Library/Application Support/Chromium/Chromium Master
                Preferences or /Library/Application Support/Chromium/Chromium
                Master Preferences
*   You only need to create and populate the master_preferences file
            when you deploy Google Chrome, and all users on that machine will
            get those settings when they first start Chromium / Google Chrome.

The master_preferences file, like each user's Preferences file, is simply a text
file that contains JSON markup, and will look something like this:

```none
{
  "homepage" : "http://www.chromium.org/",
  "homepage_is_newtabpage" : false,
  "distribution" : {
      ...more stuff here ...
  }
}
```

Some of the preferences should be obvious, but some are not entirely clear --
they are described at the end of this document.

Moreover, you'll notice that some of these preferences are managed by policy.
Note that **no matter what is in the master_preferences or Preferences files,
policy always takes precedence.** Setting the home page in both the Preferences
file and policy means that the home page in policy will be the one that Chromium
/ Google Chrome uses, and the user will not be able to edit it.

**Preferences List**

So, what preferences should you actually use? There are actually lots and lots
of preferences, most of which you won't really care about.

Here is a sample master_preferences list that may be of interest (this is a
fully-functional master_preferences file):

{

"homepage": "http://www.google.com",

"homepage_is_newtabpage": false,

"browser": {

"show_home_button": true

},

"session": {

"restore_on_startup": 4,

"startup_urls": \[

"http://www.google.com/ig"

\]

},

"bookmark_bar": {

"show_on_all_tabs": true

},

"sync_promo": {

"show_on_first_run_allowed": false

},

"distribution": {

"import_bookmarks_from_file": "bookmarks.html",

"import_bookmarks": true,

"import_history": true,

"import_home_page": true,

"import_search_engine": true,

"ping_delay": 60,

"do_not_create_desktop_shortcut": true,

"do_not_create_quick_launch_shortcut": true,

"do_not_create_taskbar_shortcut": true,

"do_not_launch_chrome": true,

"do_not_register_for_update_launch": true,

"make_chrome_default": true,

"make_chrome_default_for_user": true,

"system_level": true,

"verbose_logging": true,

"browser": {

"confirm_to_quit": true,

}

},

"first_run_tabs": \[

"http://www.example.com",

"http://welcome_page",

"http://new_tab_page"

\]

}

Most of these settings should be self-explanatory. The most interesting settings
are:

*   import_bookmarks_from_file: silently imports bookmarks from the
            given HTML file.
*   import_\*: each of these import parameters will trigger automatic
            imports of settings on first run.
*   ping_delay: RLZ ping delay in seconds.
*   do_not_create_any_shortcuts: suppress creation of all shortcuts
            (including the Start Menu shortcut)
*   do_not_create_taskbar_shortcut: only supported on Windows 8 and
            below -- TaskBar shortcuts are never created on Windows 10
*   do_not_launch_chrome: doesn't launch chrome after the first install.
*   do_not_register_for_update_launch: does not register with Google
            Update to have Chrome launched after install.
*   make_chrome_default: makes chrome the default browser.
*   make_chrome_default_for_user: makes chrome the default browser for
            the current user.
*   system_level: install chrome to system-wide location.
*   verbose_logging: emit extra details to the installer's log file to
            diagnose install or update failures.
*   first_run_tabs: these are the tabs & URLs shown on the first launch
            (and only on first launch) of the browser.
*   sync_promo.show_on_first_run_allowed: prevents the sign-in page from
            appearing on first run.
*   browser/confirm_to_quit: Supported only on MacOS can be used to
            prevent the confirmation prompt on quitting the browser. Note that
            it needs to be in the "distribution" section of the file.

### Pre-installed Bookmarks

To add pre-installed bookmarks, you have to create a file that contains all of
your bookmarks, then give the right signals for a Chrome install to import them
when a user runs Chrome for the first time.

1.  First, set up bookmarks in Chrome as you'd like them to appear to
            the end-user
2.  Go to the Wrench Menu -&gt; Bookmark Manager -&gt; Organize
            Bookmarks -&gt; Export Bookmarks
3.  The file that is saved/exported contains all of the bookmark data
            that will be imported.

To instruct an end-user's Chrome to import these bookmarks, include these
elements in your master_preferences:

{ "distribution": { "import_bookmarks": false, "import_bookmarks_from_file":
"c:\\\\path\\\\to\\\\your\\\\bookmarks.html" }, "bookmark_bar": {
"show_on_all_tabs": true } }

The relevant entries are:

*   "import_bookmarks_from_file": needs to have the path to bookmark
            file. **The backslashes in the path must be escaped by a backslash;
            use double-backslashes. Also be sure that this file exists at the
            point that the user first runs Chrome.**
*   "import_bookmarks" should probably be false, so your imported
            bookmarks don't get overwritten.
*   "show_on_all_tabs": can either be true or false, whether we've
            promised the partner to show the bookmarks bar on by default or not.
