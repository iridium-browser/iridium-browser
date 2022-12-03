---
breadcrumbs:
- - /developers
  - For Developers
page_name: creating-and-using-profiles
title: Creating and Using Profiles
---

*(This page deals with establishing entirely separate data directories for
running parallel instances of the Chrome browser. To create separate user
accounts within the same Chrome browser instance, please read about the
[multiple accounts feature](/user-experience/multi-profiles).)*

By creating and using multiple profiles, you can do development — creating
extensions, modifying the browser, or testing the browser — while still being
able to use Google Chrome as your default browser. We recommend that you use the
default profile for your everyday browsing.
The details of how to create and use a profile vary by platform, but here's the
basic process:

*   Create a folder to hold data for the new profile.
*   Create a shortcut or alias that launches the browser, using the
            `--user-data-dir` command-line argument to specify the profile's
            location.
*   Whenever you launch the browser, use the shortcut or alias that's
            associated with the profile. If the profile folder is empty, the
            browser creates initial data for it.

### Instructions: Windows

#### To create a profile

1.  Create a folder on your computer, say **C:\\chrome-dev-profile**.
            This folder will hold the data for the new profile.
2.  Make a copy of the Google Chrome shortcut on your desktop.
            (Right-click, then Copy.) Name the new shortcut something like
            **Chrome Development**.
3.  Right-click the **Chrome Development** shortcut, choose
            **Properties**, and paste `--user-data-dir=C:\\chrome-dev-profile`
            at the end of the **Target** field. The result might look like this:
    `"C:\Documents and Settings\me\Local Settings\Application
    Data\Google\Chrome\Application\chrome.exe"
    --user-data-dir=C:\chrome-dev-profile`
4.  Start Google Chrome by double-clicking the **Chrome Development**
            shortcut. This creates the profile data.

#### To use the profile

Just start Google Chrome by double-clicking the **Chrome Development** shortcut.
Yep, it's that easy.

#### To clear the profile

You might want to clear the profile if you're testing, and you want to start
from scratch. Here's how:

1.  Close Google Chrome.
2.  Delete the contents of your profile folder — the folder you
            specified with `--user-data-dir` (for example,
            **C:\\chrome-dev-profile**).

### Instructions: Mac

To be provided. See [User Data Directory](/user-experience/user-data-directory)
for the default location.

### Instructions: Linux

To be provided. See [User Data Directory](/user-experience/user-data-directory)
for the default location.
