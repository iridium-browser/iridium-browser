---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/diagnostics
  - Diagnostics
page_name: gcf_troubleshooting
title: Chrome Frame Troubleshooting
---

## Google Chrome Frame is no longer supported and retired as of February 25, 2014. **For guidance on what you need to know as a developer or IT administrator, please read our [developer FAQs](https://developers.google.com/chrome/chrome-frame/) for Chrome Frame.**

[TOC]

## Introduction

This document describes the troubleshooting flow for Google Chrome Frame users.

## Basic Troubleshooting Actions

### Filing a bug

If the problem has serious security implications please report it using
guidelines posted here:
<http://www.chromium.org/Home/chromium-security/reporting-security-bugs>

Before you file any bug, please search Google Chrome Frame bug tracker:
<http://code.google.com/p/chromium/issues/list?q=Area%3DChromeFrame> If your
problem is a known issue then it’s likely that a solution is waiting for you. If
you find the bug is already there, feel free to star it so you can find it later
and indicate to us another person is interested in its resolution.

if you can't find the bug in the database should you consider filing a new one
by filling out the form here:
<http://code.google.com/p/chromium/issues/entry?template=ChromeFrame%20Issue> If
possible, provide a test page that demonstrates the problem. Test pages help
quickly fix the problem and allow us to catch regressions in future.

Asking a Question on the ChromeFrame Discussion Group

Join / post your question to
<http://groups.google.com/group/google-chrome-frame> This forum is answered by
Chrome Frame engineers, and is publicly-viewable, so don’t ask any
sensitive/proprietary questions here.

### Checking if a Page is Running in Chrome Frame

Right-click the page (make sure to click on the page, not on any embedded
controls e.g., flash). You should see a (small-ish) context menu that shows
“About Chrome Frame...” as the last option in the list.

If you don’t see this, then ChromeFrame is not running this page.

### Check if Chrome Frame is installed

Websites are not automatically triggered to load in Chrome Frame -- the site has
to explicitly trigger it. In order for a web site to know that chrome frame is
available the following should work:

1.  Chrome Frame is properly installed: typing “gcf:about:version” into
            IE address bar should display a page with version information.
2.  Chrome Frame is enabled: you can verify that it is enabled by
            accessing ‘Manage Addons’ dialog via Tools menu in IE.
3.  To make sure that web sites can see chrome frame, visit
            [whatsmyuseragent.com](http://www.google.com/url?q=http%3A%2F%2Fwhatsmyuseragent.com&sa=D&sntz=1&usg=AFQjCNFVJf0_Aui3AZpzlWQ1o7wZEHbkeg)
            and see if “chromeframe/...” appears in the user-agent string.

## Common Issues

### I just installed Chrome Frame but it is not working

Make sure that Chrome Frame is installed and enabled. Chrome Frame plugs into
Internet Explorer as a
[BHO](http://www.google.com/url?q=http%3A%2F%2Fmsdn.microsoft.com%2Fen-us%2Flibrary%2Fbb250436(v%3Dvs.85).aspx%23bho_whatare&sa=D&sntz=1&usg=AFQjCNFTOgksXW5aAjZ0PIjbS5dDJiKVsw).
When a new BHO is installed, it needs a browser restart to get loaded. However,
after a fresh install, a site can use a script on the landing page that makes
sure the current browser instance gets Chrome Frame activated.

If a browser restart or reboot fixes this problem then this is very likely a BHO
load after install issue.

Example support thread::
<https://groups.google.com/group/google-chrome-frame/browse_thread/thread/517755cfbe6d31db>

### Is IE restart required after installing Chrome Frame for it to work?

1.  No, you shouldn’t need to restart after installing for it to work.
2.  If it isn’t working, please file a bug.

Example support
thread:<http://groups.google.com/group/google-chrome-frame/browse_thread/thread/438390134edaf4fd#>

### Chrome Frame Is Enabled but not working

You have installed Chrome Frame and even after the browser restart or reboot it
does not work. Then you can first check if Chrome Frame is installed:

1.  Verify that “third party browser extensions” are not disabled in IE
            (‘Tools’ -&gt; ‘Manage Add-ons’)
2.  This can also be controlled via Group Policy, check the setting
            here: Computer Configuration/Administrative Templates/Windows
            Components/Internet Explorer/Internet Control Panel/Advanced
            Page/Alllow third-party browser extensions

Example support
thread:<http://groups.google.com/group/google-chrome-frame/browse_thread/thread/6ef50d50cb09db9f/d6d0a1bc9fad0725?pli=1>

### Can a server tell if GCF is both Installed \*and\* Enabled?

1.  Check the user-agent string. If the “chromeframe/...” token is
            present, then it is installed and enabled.
2.  If Chrome Frame is installed but the BHO is disabled then it can be
            detected using script: var gcf = new
            ActiveXObject('ChromeTab.ChromeFrame')

Example support
thread:<http://groups.google.com/group/google-chrome-frame/browse_thread/thread/21cb36205d1b79a2#>

### Google Chrome Frame doesn't work for site XYZ

1.  Make sure that Chrome Frame is installed and enabled.
2.  Sites have to opt-in to turning on Chrome Frame, so it doesn’t
            automatically turn on for every site.
3.  Make sure that the site XYZ is sending the ChromeFrame &lt;meta&gt;
            tag, or the HTTP headers as described here:
            <http://www.chromium.org/developers/how-tos/chrome-frame-getting-started#TOC-Making-Your-Pages-Work-With-Google->
4.  If the user wants Chrome Frame on for every site, see
            <http://www.chromium.org/developers/how-tos/chrome-frame-getting-started#TOC-Testing-Your-Sites>

Example support threads:

<http://groups.google.com/group/google-chrome-frame/browse_thread/thread/6914c2edd167503c#><http://groups.google.com/group/google-chrome-frame/browse_thread/thread/021af37c507512cd#>

### Chrome frame works, but gcf: prefix doesn't, Why?

1.  gcf: doesn’t work on all sites by default, for security reasons.
2.  To enable this testing feature, open regedit.exe and add a DWORD
            value AllowUnsafeURLs=1 under HKCU\\Software\\Google\\ChromeFrame

Example support
thread:<http://groups.google.com/group/google-chrome-frame/browse_thread/thread/38c53f04d1f6af13#>

### Chrome Frame Is Not active unless I use the gcf: prefix

1.  Make sure the page is only sending one X-UA-Compatible &lt;meta&gt;
            tag.
2.  Make sure that the meta tag in in the first 1K of the page contents.

Example support thread:

<http://groups.google.com/group/google-chrome-frame/browse_thread/thread/04f133762065d471#>

### CFInstall.js’s Chrome Frame Detection doesn't work

1.  Make sure the calls to CFInstall.js’s CFInstall.check() are
            correctly formed.
2.  Make sure the CFInstall.check() is performed after the &lt;body&gt;
            is loaded.

Example support
thread:<http://groups.google.com/group/google-chrome-frame/browse_thread/thread/a49b91141fc84a10#>

## Advanced Troubleshooting

### Detecting crashes

Google Chrome Frame plugin is loaded as npchrome_frame.dll in Internet Explorer
processes. It uses Chrome’s multi process architecture so when a web page is
loaded using this plugin, it will launch a Chrome Browser process, which in turn
launches a Chrome Renderer process.

The following sections describe how to identify crashes in these processes.

#### Internet Explorer Crash

In this case Internet Explorer will detect a crashing plugin and put up a dialog
like this:

[<img alt="image"
src="/developers/diagnostics/gcf_troubleshooting/image2.png">](/developers/diagnostics/gcf_troubleshooting/image2.png)

More detailed information about the crash can be added by clicking on the
‘Advanced...’ button to bring up the crash details dialog and inserting the
details in the bug report .

#### Chrome Browser Crash

If for some reason the Chrome Browser process crashed or lost connection with
the plugin running in Internet Explorer, then it looks like this:

[<img alt="image"
src="/developers/diagnostics/gcf_troubleshooting/image1.png">](/developers/diagnostics/gcf_troubleshooting/image1.png)

To figure out if this is a crash use the following steps:

1.  Make sure that there are no stand alone Chrome browser windows open.
2.  Make sure that this is the only tab or window loaded with Chrome
            Frame.
3.  Open up task manager and look for chrome.exe processes.
4.  If there are one or more chrome.exe processes in the task manager
            then this is a sign of a connection loss between Chrome Frame plugin
            and Chrome Browser process.
5.  If there are no chrome.exe processes then it’s most likely a crash.
            If this is happening at launch time then it could be a failure to
            launch Chrome.exe.

#### Chrome Renderer Crash

If a Chrome Renderer process crashed with Chrome Frame then it will look like
this:

[<img alt="image"
src="/developers/diagnostics/gcf_troubleshooting/image4.png">](/developers/diagnostics/gcf_troubleshooting/image4.png)

It’s very rare to have Chrome Frame specific renderer crashes. So the first
thing to verify is see if the same steps lead to a crash in stand alone Chrome
browser. In either case, a test page that demonstrates the crash will be really
helpful in the report.

### Reporting crashes

While reporting a crash, having a reproducible test case is very important. If
the crash is happening at random then having a crash dump is quite useful.

#### How to generate a crash dump

A freely available tool called ‘Debugging Tools For Windows’ ([32 bit
download](http://www.google.com/url?q=http%3A%2F%2Fwww.microsoft.com%2Fwhdc%2Fdevtools%2Fdebugging%2Finstallx86.mspx&sa=D&sntz=1&usg=AFQjCNFhPKf3h5kZ1oBJmIEdrRMkPigeZA)
[64 bit
download](http://www.google.com/url?q=http%3A%2F%2Fwww.microsoft.com%2Fwhdc%2Fdevtools%2Fdebugging%2Finstall64bit.mspx%2520&sa=D&sntz=1&usg=AFQjCNEZnFyqlcxNJW3A16kQXP4ZSSj_1w))
or ‘WinDbg’ in short, can be used to collect crash dumps. Once you have
downloaded and installed the correct flavor of WinDbg then run Internet Explorer
under the debugger using the command line:

`"c:\Program Files\Debugging Tools For Windows (x86)\windbg.exe" -g -G -o
"c:\program files\Internet Explorer\iexplore.exe"`

or for 64 bit OS use:

`"c:\Program Files\Debugging Tools For Windows (x64)\windbg.exe" -g -G -o
"c:\program files (x86)\Internet Explorer\iexplore.exe"`

Once Internet Explorer is launched, WinDbg should look like this:

[<img alt="image"
src="/developers/diagnostics/gcf_troubleshooting/image3.png">](/developers/diagnostics/gcf_troubleshooting/image3.png)

Now switch to Internet Explorer and follow steps to reproduce the crash. **Note
that once the crash happens, you will not see usual dialog or ‘sad tab’ bitmap.
Instead, Internet Explorer will appear unresponsive**. When that happens, come
back to WinDbg and it should look like this:

[<img alt="image"
src="/developers/diagnostics/gcf_troubleshooting/image0.png">](/developers/diagnostics/gcf_troubleshooting/image0.png)

Type the following command into the debugger prompt as shown in the picture
above and press Enter:

`.dump c:\temp\crash.dmp`

After the crash dump is successfully written to the disk please attach it to the
bug.

### Detecting stability issues

Sometimes Chrome Frame may not work as expected due to spyware or other buggy
Internet Explorer Add-ons. To make sure that is not the case, it’s best to test
on a clean Windows image. Also, for the purpose of testing, any other installed
Add-ons can be disabled by going into ‘Tools’ -&gt; ‘Manage Add-ons’ dialog in
Internet Explorer.
