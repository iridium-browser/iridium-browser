---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/androidwebview
  - Android WebView
page_name: webview-ct-bug
title: WebView FAQ for Symantec Certificate Transparency Issue
---

The Issue:

Starting with version 53,
[WebView](https://developer.android.com/reference/android/webkit/WebView.html)
required that all certificates issued by Symantec Corporation after 1 June 2016
must comply with Chrome's Certificate Transparency policy. This was [announced
last
year](https://security.googleblog.com/2015/10/sustaining-digital-certificate-security.html)
and implemented in Chrome and WebView 53 which was released at the beginning of
September 2016. More information about Certificate transparency is available
[here](https://www.certificate-transparency.org/).

Due to a bug in Chrome’s implementation of Certificate Transparency, pages
served with Symantec certificates will not load in WebView versions 53 or 54,
starting 10 weeks after each WebView release. More details are available [in
this public bug](https://bugs.chromium.org/p/chromium/issues/detail?id=664177).

The chromium-dev group also includes extra information
[here.](https://groups.google.com/a/chromium.org/forum/#!topic/chromium-dev/_ZILH704AVU)

General Questions and Answers

    Which versions of WebView are impacted?
    The impacted versions are 53 and 54 builds of Android WebView. Versions 52
    and earlier, or 55 and later, do not exhibit the problem. For affected
    versions, all channels including stable, beta, dev, canary releases are
    impacted.

    When does the issue start occurring?
    The issue manifests 10 weeks after the build date. For 53 stable builds, 10
    weeks have already passed. For 54, builds will expire as follows:

<table>
<tr>

<td>Build ID</td>

<td>Expiration Date</td>

</tr>
<tr>

<td>54.0.2840.68</td>

<td>12/27/2016</td>

</tr>
<tr>

<td>54.0.2840.85</td>

<td>1/7/2017</td>

</tr>
</table>

    Does this issue occur if WebView is up to date?
    Webview is released approximately every 6 weeks. Since the bug takes effect
    10 weeks after a new version is built, up-to-date versions of WebView are
    not impacted.

    Which Android versions are impacted?
    All versions of Android L and above are impacted. This is true even when
    WebView is provided by Chrome (Android N and above).

    How can users check their WebView version?
    This is different for Android versions. For Android L-M, users can check the
    version using:
    Settings-&gt;Apps-&gt;Show system (top right 3 dots)-&gt;Android System
    WebView
    For Android N, by default WebView is provided by Chrome. Users can check it
    in Settings-&gt;Apps -&gt; Chrome
    For Android TV devices, there is no Chrome, so WebView version should be
    checked just like Android L-M devices. However, the settings menu is
    slightly different. Use
    Settings-&gt;Apps-&gt;scroll down to Android System WebView
    For Android N devices, where users disable Chrome, WebView is provided by
    Android WebView package. So version should be checked similar to L-M
    devices.
    There are users who enable developer settings and use Canary/Beta channels
    of Chrome as WebView provider. These users can get the version using
    Settings-&gt;Apps and their choice of provider.
    Android watch devices don’t have WebView so are not impacted.

    What is required for users to update Webview?
    Users who are signed in to their devices can update WebView through the Play
    Store. Most users will be updated automatically.

    The version numbers in the Play Store for both Android WebView and Chrome
    start with 54 for me. Is the issue only in WebView, not Chrome?
    This issue is only in WebView. However WebView can be provided by Chrome in
    Android N and above depending on the type of the device. Please see Q#5 for
    how to find the WebView version for different devices.

    How is Android login process impacted?
    Setup Wizard uses Webview while handling Captive portals. This may be broken
    for portals signed with Symantec certs.
    Enterprise dasher accounts that are hosted on 3rd party WebSites will be
    broken if they are signed with Symantec certs.

    Can users change system clock to an earlier date to workaround Captive
    portal issues or for setting Dasher account during initial setup?
    If blocked by a captive portal, the system clock may still be unset (for
    example if the device does not have a SIM). If either the date/time is
    unset, the user will be shown the date/time screen in Setup Wizard UI, where
    the default date will be Jan 1 of the build year. If the date is earlier
    than the date for the Certificate Transparency deadline, the issue may not
    be seen.
    We do not suggest changing the time/date as it may cause many other things
    to be broken.

    What happens to Android devices that are not in Google Play ecosystem? Are
    they impacted?
    Android WebView is used to provide WebView functionality in non-play
    ecosystem devices too (for example Amazon Fire devices). These devices may
    be impacted if they have WebViews built from M53 and M54 releases of
    Chromium.
    However, WebView implementations and updates in such non Play Store devices
    are beyond Google’s control. OEMs may prefer to fix broken implementations
    using OTAs for example.

    I have Android N and My WebView using app (Amazon, Paytm, JD Lite, ... ) is
    broken. The app is suggesting me to update WebView but the link I was
    referred shows WebView as disabled. What should I do? In Android N and
    above, WebView is distributed using Chrome apk. Please update Chrome.
    Alternatively, if you don't want to use Chrome, then you can disable it.
    This will enable WebView.

Developer Questions and Answers

    Are there solutions based on user agent sniffing?
    Affected versions of WebView will fail to connect at the TLS layer, before a
    user agent is sent. It might be possible to sniff the client at the TLS
    layer, but it is complex and may not be reliable. For example, CloudFlare
    has a tool to sniff the clients based on unique patterns that different
    clients exhibit (such as Safari vs. WebView). We are not aware of a tool
    that can sniff a particular version of WebView (M53 vs. M55).

    How can developers suggest updating WebView properly? Is there a sample
    code?
    Requesting users to update WebView is fairly complicated as WebView on
    Android N is provided by Chrome APK for phones and tablets and further there
    can be multiple WebView providers for a given device. See below.

    Can app developers take measures to fix the issue?
    The only action WebView developers can take is to ask the user to update
    WebView. WebView provides an onReceivedSslError() callback, but this does
    not provide enough information to correctly address the problem. Using this
    callback to bypass the error is never recommended and exposes the app to
    numerous security vulnerabilities.

    For apps, is this just a simple matter of prompting users to take the Chrome
    update? Or, do the apps themselves need to be rebuilt and republished?
    WebView is a system library. Users only need to update WebView, or Chrome
    (Android N), depending on how WebView is provided. Apps cannot do anything
    except in #2 above.

Sample Code For Developers to Check WebView Version

Please refer to the source code
[here](https://github.com/ntfschr-chromium/ct_workaround) for an example app
that demonstrates how to get the WebView version and ask user to update.
