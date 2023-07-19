---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: run-chromium-with-flags
title: Run Chromium with command-line switches
---

[TOC]

There are command-line switches that Chromium (and Chrome) accept in order
to enable particular features or modify otherwise default functionality.

There is no list of all switches, but most of the existing switches
can be found at <https://peter.sh/examples/?/chromium-switches.html>.

**Note**: [Chrome switches](https://chromium.googlesource.com/chromium/src/+/main/docs/configuration.md#switches)
(e.g., `--incognito`)
and [Chrome flags](https://chromium.googlesource.com/chromium/src/+/main/docs/configuration.md#flags)
(e.g., `chrome://flags/#ignore-gpu-blocklist`) are separate configurations.
Some features can be enabled by a command-line switch, flag or both.

It is important to note that some switches are intended for development and
temporary cases. They may break, change, or be removed in the future without
notice. IT admins looking to manage Chrome for their organization should
instead use [enterprise policies](https://chromeenterprise.google/policies/).

Note that if you look at `chrome://flags` to see if the command-line option is
active, the state might not be accurately reflected. Check `chrome://version`
for the complete command-line used in the current instance.

## Windows

1.  Exit any running-instance of Chrome.
2.  Right click on your "Chrome" shortcut.
3.  Choose properties.
4.  At the end of your "Target:" line add the command-line switch. For
            example:
    *   `--disable-gpu-vsync`
5.  With that example flag, it should look like below (replacing
            "--disable-gpu-vsync" with any other command-line switch you want to
            use):
    `chrome.exe --disable-gpu-vsync`
6.  Launch Chrome like normal with the shortcut.

## macOS

1.  Quit any running instance of Chrome.
2.  Run your favorite Terminal application.
3.  In the terminal, run commands like below (replacing
            "--remote-debugging-port=9222" with any other command-line switch you
            want to use):

    ```none
    /Applications/Chromium.app/Contents/MacOS/Chromium --remote-debugging-port=9222
    # For Google Chrome you'll need to escape spaces like so:
    /Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome --remote-debugging-port=9222
    ```

## Linux

1.  Quit any running instance of Chrome.
2.  Run your favorite terminal emulator.
3.  In the terminal, run commands like below (replacing
            "--remote-debugging-port=9222" with any other command-line switch you
            want to use):

    ```none
    chromium-browser --remote-debugging-port=9222
    google-chrome --foo --bar=2
    ```

## iOS

If you are building Chromium from the source, you can run it with command-line
switches by adding them in the Experimental Settings.

1.  Open the Settings app
2.  Go to Chromium/Experimental Settings
3.  Add your switches in the "Extra flags (one per line)". Don't forget to
            switch on the "Append Extra Flags" setting.

It is not possible to run with flags the official releases (Google Chrome from
App Store or Testflight).

## V8 Flags

V8 can take a number of flags (command-line switches) as well,
via Chrome's `js-flags` flag. For example, this traces V8 optimizations:

```none
chrome.exe --js-flags="--trace-opt --trace-deopt --trace-bailout"
```

To get a listing of all possible V8 flags:

```none
chrome.exe --js-flags="--help"
```

Browse [the V8 docs](https://v8.dev/docs) for more flags for V8.

## Android

Visit `about:version` to review the command-line switches that are effective
in the app.

If you are running on a rooted device or using a debug build of Chromium, then
you can set switches like so:

```none
out/Default/bin/chrome_public_apk argv  # Show existing switches
out/Default/bin/content_shell_apk argv --args='--foo --bar'  # Set new switches
```

You can also install, set switches, and launch with a single command:

```none
out/Default/bin/chrome_public_apk run --args='--foo --bar'
out/Default/bin/content_shell_apk run  # Clears any existing switches
```

For production build on a non-rooted device, you need to enable "Enable command
line on non-rooted devices" in chrome://flags, then set command-line in
/data/local/tmp/chrome-command-line. When doing that, mind that the first
command-line item should be a "_" (underscore) followed by the ones you actually
need. Finally, manually restart Chrome ("Relaunch" from chrome://flags page
might no be enough to trigger reading this file). See
<https://crbug.com/784947>.

### ContentShell on Android

There's an alternative method for setting command-line switches with ContentShell
that doesn't require building yourself:

1.  Download a [LKGR build of
            Android](https://download-chromium.appspot.com/?platform=Android&type=continuous).
2.  This will include both ChromePublic.apk and ContentShell.apk
3.  Install ContentShell APK to your device.
4.  Run this magic incantation

```none
adb shell am start \
  -a android.intent.action.VIEW \
  -n org.chromium.content_shell_apk/.ContentShellActivity \
  --es activeUrl "http://chromium.org" \
  --esa commandLineArgs --show-paint-rects,--show-property-changed-rects
```

This will launch contentshell with the supplied switches. You can apply whatever
commandLineArgs you want in that syntax.

### Android WebView

This is documented [in the chromium
tree](https://chromium.googlesource.com/chromium/src/+/HEAD/android_webview/docs/commandline-flags.md).

## Chrome OS

1.  Put the device into [dev mode, disable rootfs verification, and
            bring up a command
            prompt](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/developer_mode.md).
2.  Modify /etc/chrome_dev.conf (read the comments in the file for more
            details).
3.  Restart the UI via:
    `sudo restart ui`
