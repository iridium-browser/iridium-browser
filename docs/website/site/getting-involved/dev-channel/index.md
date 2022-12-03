---
breadcrumbs:
- - /getting-involved
  - Getting Involved
page_name: dev-channel
title: Chrome Release Channels
---

[TOC]

Chrome supports a number of different release channels. We use these channels to
slowly roll out updates to users, starting with our close to daily Canary
channel builds, all the way up to our Stable channel releases that happen every
6 weeks roughly.

### Channels

#### Windows 64-bit

All channels may be run in parallel -- they install into distinct directories
and use dedicated User Data directories.

*   [Stable channel for Windows
            (64-bit)](https://www.google.com/chrome/browser/?platform=win64)
*   [Beta channel for Windows
            (64-bit)](https://www.google.com/chrome/browser/beta.html?platform=win64)
*   [Dev channel for Windows
            (64-bit)](https://www.google.com/chrome/browser/index.html?extra=devchannel&platform=win64)
*   [Canary build for Windows
            (64-bit)](https://www.google.com/chrome/browser/canary.html?platform=win64)

#### Windows 32-bit

All channels may be run in parallel -- they install into distinct directories
and use dedicated User Data directories.

*   [Stable channel for Windows
            (32-bit)](https://www.google.com/chrome/browser/?platform=win)
*   [Beta channel for Windows
            (32-bit)](https://www.google.com/chrome/browser/beta.html?platform=win)
*   [Dev channel for Windows
            (32-bit)](https://www.google.com/chrome/browser/index.html?extra=devchannel&platform=win)
*   [Canary build for Windows
            (32-bit)](https://www.google.com/chrome/browser/canary.html?platform=win)

#### Mac

All channels may be run in parallel -- they install into distinct directories
and use dedicated User Data directories.

*   [Stable channel for
            Mac](https://www.google.com/chrome/browser/?platform=mac)
*   [Beta channel for
            Mac](https://www.google.com/chrome/browser/beta.html?platform=mac&extra=betachannel)
*   [Dev channel for
            Mac](https://www.google.com/chrome/browser/?platform=mac&extra=devchannel)
*   [Canary build for
            Mac](https://www.google.com/chrome/browser/canary.html?platform=mac)

#### Android

*   [Stable channel for
            Android](https://play.google.com/store/apps/details?id=com.android.chrome)
*   [Beta channel for
            Android](https://play.google.com/store/apps/details?id=com.chrome.beta)
*   [Dev channel for
            Android](https://play.google.com/store/apps/details?id=com.chrome.dev)
*   [Canary channel for
            Android](https://play.google.com/store/apps/details?id=com.chrome.canary)

#### iOS

*   [Stable channel for
            iOS](https://itunes.apple.com/us/app/chrome-web-browser-by-google/id535886823?mt=8)
*   [Beta channel for iOS](https://testflight.apple.com/join/LPQmtkUs)

#### Linux

*   [Stable
            channel](https://www.google.com/chrome/browser/?platform=linux)
*   [Beta
            channel](https://www.google.com/chrome/browser/beta.html?platform=linux)
*   [Dev
            channel](https://www.google.com/chrome/browser/?platform=linux&extra=devchannel)

### How do I choose which channel to use?

The release channels for chrome range from the most stable and tested (Stable
channel) to completely untested and likely least stable (Canary channel). You
can run all channels alongside all others, as they do not share profiles with
one another. This allows you to play with our latest code, while still keeping a
tested version of Chrome around.

*   **Stable channel:** This channel has gotten the full testing and
            blessing of the Chrome test team, and is the best bet to avoid
            crashes and other issues. It's updated roughly every two-three weeks
            for minor releases, and every 6 weeks for major releases.
*   **Beta channel:** If you are interested in seeing what's next, with
            minimal risk, Beta channel is the place to be. It's updated every
            week roughly, with major updates coming every six weeks, more than a
            month before the Stable channel will get them.
*   **Dev channel:** If you want to see what's happening quickly, then
            you want the Dev channel. The Dev channel gets updated once or twice
            weekly, and it shows what we're working on right now. There's no lag
            between major versions, whatever code we've got, you will get. While
            this build does get tested, it is still subject to bugs, as we want
            people to see what's new as soon as possible.
*   **Canary build:** Canary builds are the bleeding edge. Released
            daily, this build has not been tested or used, it's released as soon
            as it's built.
*   **Other builds:** If you're extra brave, you can download the latest
            working (and that's a very loose definition of working) build from
            [download-chromium.appspot.com](https://download-chromium.appspot.com/).
            You can also look for a more specific recent build by going to [the
            Chromium continuous build waterfall](http://build.chromium.org),
            looking at the number near the top under "LKGR", and then going to
            [this Google Storage
            bucket](http://commondatastorage.googleapis.com/chromium-browser-continuous/index.html)
            and downloading the corresponding build.

**Note**: Early access releases (Canary builds and Dev and Beta channels) will
be only partly translated into languages other than English. Text related to new
features may not get translated into all languages until the feature is released
in the Stable channel.

### What should I do before I change my channel?

#### Back up your data!

Before you switch, you should make a backup of your profile (bookmarks, most
visited pages, history, cookies, etc). If you ever want to switch back to a more
stable channel, your updated profile data might not be compatible with the older
version.
Make a copy of the User Data\\Default directory (for example, copy it to
'Default Backup' in the same location). The location depends on your operating
system:
> Windows XP:

> *   Stable, beta, and dev channels: \\Documents and
              Settings\\%USERNAME%\\Local Settings\\Application
              Data\\Google\\Chrome\\User Data\\Default
> *   Canary builds: \\Documents and Settings\\%USERNAME%\\Local
              Settings\\Application Data\\Google\\Chrome SxS\\User Data\\Default

> Windows Vista, 7, 8 or 10:

> *   Stable channel:
              \\Users\\%USERNAME%\\AppData\\Local\\Google\\Chrome\\User
              Data\\Default
> *   Beta channel: \\Users\\%USERNAME%\\AppData\\Local\\Google\\Chrome
              Beta\\User Data\\Default
> *   Dev channel: \\Users\\%USERNAME%\\AppData\\Local\\Google\\Chrome
              Dev\\User Data\\Default
> *   Canary builds: \\Users\\%USERNAME%\\AppData\\Local\\Google\\Chrome
              SxS\\User Data\\Default

> Mac OS X:

> *   Stable channels, as well as older beta and dev channels:
              ~/Library/Application Support/Google/Chrome/Default
> *   Beta channel: ~/Library/Application Support/Google/Chrome
              Beta/Default
> *   Dev channel: ~/Library/Application Support/Google/Chrome
              Dev/Default
> *   Canary builds: ~/Library/Application Support/Google/Chrome
              Canary/Default

> Linux:

> *   ~/.config/google-chrome/Default

Note:If you're using Explorer to find the folder, you might need to set **Show
hidden files and folders** in **Tools &gt; Folder Options... &gt; View**.

### Reporting Dev channel and Canary build problems

Remember, Dev channel browsers and Canary builds may still crash frequently.
Before reporting bugs, consult the following pages:

*   [Bug Life Cycle and Reporting
            Guidelines](/for-testers/bug-reporting-guidelines)
*   See [
            bug-reporting-guidlines-for-the-mac-linux-builds](/for-testers/bug-reporting-guidlines-for-the-mac-linux-builds)
            before reporting problems in Mac or Linux Dev channel builds

<https://crbug.com/new> If after reading the above, you think you have a real bug, file it -

### Going back to a more stable channel

*   If you decide to switch from Dev to Beta or from Beta to Stable, the
            new channel will be on an earlier version of Google Chrome. You
            won't get automatic updates on that channel until it reaches a
            version later than what you're already running.
*   You can uninstall Google Chrome and re-install from
            <https://www.google.com/chrome> to go back to an earlier version.
*   If you re-install an older version, you might find that your profile
            is not compatible (because the data formats changed in the newer
            version you had been running). You'll have to delete your profile
            data. Delete the User Data\\Default folder (see the Before You
            Change Channels section above for the location). If you made a back
            up of your Default directory, you can then rename it to Default so
            that you at least restore some of your previous bookmarks, most
            visited pages, etc.
*   If the installer fails when you attempt to install an older version
            with a message indicating that your computer already has a more
            recent version of Chrome or Chrome Frame, you must also uninstall
            Chrome Frame. After doing so, the older version of Chrome should
            install without difficulty.
