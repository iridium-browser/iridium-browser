---
breadcrumbs: []
page_name: chrome-release-channels
title: Chrome Release Channels
---

[TOC]

Chrome supports a number of different release channels. We use these channels to
slowly roll out updates to users, starting with our close to daily Canary
channel builds, all the way up to our Stable channel releases that happen every
6 weeks roughly.

### Channels

#### Windows

All channels may be run in parallel -- they install into distinct directories
and use dedicated User Data directories.

*   [Stable channel for
            Windows](http://www.google.com/chrome?platform=win)
*   [Beta channel for
            Windows](http://www.google.com/chrome?platform=win)
*   [Dev channel for
            Windows](http://www.google.com/chrome/eula.html?extra=devchannel&platform=win)
*   [Canary build for
            Windows](http://tools.google.com/dlpage/chromesxs?platform=win)

#### Mac

*   [Stable channel for Mac](http://google.com/chrome?platform=mac)
*   [Beta channel for Mac](http://google.com/chrome?platform=mac)
*   [Dev channel for
            Mac](http://www.google.com/chrome/intl/en/eula_dev.html?dl=mac)
*   [Canary build for
            Mac](http://tools.google.com/dlpage/chromesxs?platform=mac) (Note,
            this will run in parallel to any other Chrome channel you have
            installed, it will not use the same profile)

#### Linux

[Stable channel](http://www.google.com/chrome?platform=linux)
**32-bit Ubuntu/Debian**

*   [Beta channel for 32-bit
            Debian/Ubuntu](http://www.google.com/chrome/intl/en/eula_beta.html?dl=beta_i386_deb)
*   [Dev channel for 32-bit
            Debian/Ubuntu](http://www.google.com/chrome/intl/en/eula_dev.html?dl=unstable_i386_deb)

**32-bit Fedora/OpenSUSE**

*   [Beta channel for 32-bit
            Fedora/OpenSUSE](http://www.google.com/chrome/intl/en/eula_beta.html?dl=beta_i386_rpm)
*   [Dev channel for 32-bit
            Fedora/OpenSUSE](http://www.google.com/chrome/intl/en/eula_dev.html?dl=unstable_i386_rpm)

**64-bit Ubuntu/Debian**

*   [Beta channel for 64-bit
            Debian/Ubuntu](http://www.google.com/chrome/intl/en/eula_beta.html?dl=beta_amd64_deb)
*   [Dev channel for 64-bit
            Debian/Ubuntu](http://www.google.com/chrome/intl/en/eula_dev.html?dl=unstable_amd64_deb)

**64-bit Fedora/Red Hat/OpenSUSE**

*   [Beta channel for 64-bit
            Fedora/OpenSUSE](http://www.google.com/chrome/intl/en/eula_beta.html?dl=beta_amd64_rpm)
*   [Dev channel for 64-bit
            Fedora/OpenSUSE](http://www.google.com/chrome/intl/en/eula_dev.html?dl=unstable_amd64_dev)

### How do I choose which channel to use?

The release channels for chrome range from the most stable and tested (Stable
channel) to completely untested and likely least stable (Canary channel). Note,
you can run the Canary channel builds alongside any other channel, as they do
not share profiles with other channels. This allows you to play with our latest
code, while still keeping a tested version of Chrome around.

*   **Stable channel:** This channel has gotten the full testing and
            blessing of the Chrome test team, and is the best bet to avoid
            crashes and other issues. It's updated roughly every two-three weeks
            for minor releases, and every 6 weeks for major releases.
*   **Beta channel:** If you are interested in seeing what's next, with
            minimal risk, Beta channel is the place to be. It's updated every
            week roughly, with major updates coming ever six weeks, more than a
            month before the Stable channel will get them.
*   **Dev channel:** Want to see what's happening quickly, then you want
            the Dev channel. The Dev channel gets updated once or twice weekly,
            and it shows what we're working on right now. There's no lag between
            major versions, whatever code we've got, you will get. While this
            build does get tested, it is still subject to bugs, as we want
            people to see what's new as soon as possible.
*   **Canary build:** Canary builds are the bleeding edge. Released
            daily, this build has not been tested or used, it's released as soon
            as it's built. Because there's no guarantee that it will even run in
            some cases, it uses it's own profile and settings, and can be run
            side by side another Chrome channel. By default, it also reports
            crashes and usage statistics to Google (you can disable this on the
            download page).

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

> *   Stable, beta, and dev channels: ~/Library/Application
              Support/Google/Chrome/Default
> *   Canary builds: ~/Library/Application Support/Google/Chrome
              Canary/Default

> Linux:

> *   ~/.config/google-chrome/Default

Note:If you're using Explorer to find the folder, you might need to set **Show
hidden files and folders** in **Tools &gt; Folder Options... &gt; View**.

#### Enable anonymous usage statistics

Please configure Google Chrome to send anonymous usage stats to Google. The
statistics we gather have no personally identifiable information. The aggregate
of all the stats for all users in a release channel really help us understand
how stable the release is and how people are using any new features.

Choose **\[Wrench menu\] &gt; Options** (Windows and Linux) or **Chrome &gt;
Preferences…** (Mac), go to the **Under the Hood** tab, and check **Help make
Google Chrome better by automatically sending usage statistics and crash reports
to Google**.

### Reporting Dev channel and Canary build problems

Remember, Dev channel browsers and Canary builds may still crash frequently.
Before reporting bugs, consult the following pages:

*   [Bug Life Cycle and Reporting
            Guidelines](/for-testers/bug-reporting-guidelines)
*   See [
            bug-reporting-guidlines-for-the-mac-linux-builds](/for-testers/bug-reporting-guidlines-for-the-mac-linux-builds)
            before reporting problems in Mac or Linux Dev channel builds

If after reading the above, you think you have a real bug, file it at
<http://crbug.com/new>

### Going back to a more stable channel

*   If you decide to switch from Dev to Beta or from Beta to Stable, the
            new channel will be on an earlier version of Google Chrome. You
            won't get automatic updates on that channel until it reaches a
            version later than what you're already running.
*   You can uninstall Google Chrome and re-install from
            <http://www.google.com/chrome> to go back to an earlier version.
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
            Chrome Frame. After doing so, the newer version of Chrome should
            install without difficulty.
