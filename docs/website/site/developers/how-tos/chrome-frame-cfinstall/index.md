---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
page_name: chrome-frame-cfinstall
title: Chrome Frame CFInstall script
---

## Google Chrome Frame is no longer supported and retired as of February 25, 2014. For guidance on what you need to know as a developer or IT administrator, please read our [developer FAQs](https://developers.google.com/chrome/chrome-frame/) for Chrome Frame.

## Introduction

This page describes the latest version of the Chrome Frame CFInstall script that
allows easy prompting for Chrome Frame installation within your own site.

This version supports a new streamlined and improved API, though it is expected
to be backwards compatible with previous versions of the API as well.

## User Experience

This version of the CFInstall script is designed to optimize the user's
installation experience and get them onto your site, with Chrome Frame
installed, as fast as possible.

To use this install flow, simply:

1.  Provide the user with a prompt to install Chrome Frame, whether it
            is optional or required for your site to function.
2.  If the user chooses to install Chrome Frame, invoke
            CFInstall.require(). You may pass optional success and failure
            callbacks, in that order.

CFInstall.require will check the user's browser for compatibility and, if Chrome
Frame is supported, launch a modal dialog with the Chrome Frame EULA and
installation flow. The appropriate callback will be invoked upon installation
success, failure, or cancellation.

By default, the current page will be reloaded after installation completes, with
the Chrome Frame plugin active. Naturally, your site must be sending the Chrome
Frame header or meta tag in order to be activated in Chrome Frame.

See
<http://src.chromium.org/viewvc/chrome/trunk/src/chrome_frame/cfinstall/examples/simple.html>
for an example of this integration.

## Custom Look & Feel

The default look & feel of the installation flow dialog can be replaced quite
easily to match the look of your site. See
<http://src.chromium.org/viewvc/chrome/trunk/src/chrome_frame/cfinstall/examples/jquery.html>
for an example using the JQuery library.

## **Hosting the Scripts**

You can access the CFInstall script directly off of Google's servers or host it
yourself. If you choose to host it yourself, please note that the script
consists of both the small initial script (the "stub") and a second, larger
script (the "implementation") that is only downloaded if the install flow is
launched. If you move the script elsewhere, you must either specify the
implementation location or recompile the stub with your intended hosting path
(see below).

### Specifying a Custom Implementation URL

The implementation is located at
<http://google.com/tools/dlpage/res/chromeframe/script/cf-xd-install-impl.js> .
If you wish to access it from elsewhere, you must make the following call before
CFInstall.require:

```none
CFInstall.setImplementationUrl('/path/to/cf-xd-install-impl.js');
```

Note that the path supplied may be absolute or scheme, host, or path relative
(i.e. 'http://host/path/to/...', '//host/path/to/...', '/path/to',
'../../path/to'). It will be resolved relative to the HTML document in which the
script is executed.

## Compiling CFInstall from Source

CFInstall uses the [Closure Library](http://code.google.com/closure/library/)
and is optimized using the [Closure
Compiler](http://code.google.com/closure/compiler/). The source code for
CFInstall is included in the Chromium source repository and may be checked out
using the following command:

```none
svn co http://src.chromium.org/chrome/trunk/src/chrome_frame/cfinstall/
```

Once you have checked out the code, you may simply run build.sh to download the
required dependencies and build the optimized scripts (see the 'out' directory).
The Closure Compiler requires Python and Java in order to run. If you are unable
to execute build.sh (due to a lack of a shell interpreter, for example), you
should find it pretty straightforward to manually invoke the compiler using the
script as an example.

The build.sh script accepts two options for customizing the optimized output:

```none
Usage: ./build.sh [-l //host.com/path/to/scripts/dir/] [-p] [-d]
       -l <URL>   The path under which the generated
                  scripts will be deployed.
       -d         Disable obfuscating optimizations.
```
