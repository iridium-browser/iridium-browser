---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/getting-started
  - '1: Getting Started'
page_name: getting-started-background-and-basics
title: 'Getting Started: Background and Basics'
---

This document gives a broad high-level overview of what Native Client is and how
it fits into the world of web browsers and plugins (Chrome in particular). It is
intended for anybody who wants to learn about the background and concepts behind
Native Client, from browser technology fans, to pontential project contributors,
to NaCl module developers.

[TOC]

### Chrome

Chrome is a
[multi-process](http://www.chromium.org/developers/design-documents/multi-process-architecture)
browser. It uses multiple processes to provide increased security comparing to
other single-process browsers like Firefox.
The main process is called "browser". It runs the UI (including the
[Omnibox](/user-experience/omnibox)) and manages tabs and plugin processes. It
has full permissions of the current user for accessing resources (files,
network, etc) and can fork-exec other processes.
Tabs are allocated into separate processes, typically shared per domain. These
are called “renderer” processes. Renderer interprets the HTML layout and handles
the bitmap for displaying the page. It runs in a sandbox (known as Chrome or
outer sandbox) and has limited access permissions. It cannot open files or
network connections and can only respond to communication requests by the
browser. Communication is done via a combination of
[IPC](http://www.chromium.org/developers/design-documents/inter-process-communication)
techniques. Using sandboxed renderers ensures that if one tab misbehaves or
crashes, the rest of the tabs and the browser are isolated. It also limits the
ability of malicious software running in one tab from accessing activity in
another tab or interacting with the rest of the system.

### Plugins

[Plugins](http://en.wikipedia.org/wiki/Plug-in_%28computing%29) are external
binaries that add new capabilities to a web browser and are loaded when content
of the type they declare is embedded into a page. They either come bundled with
the browser or get downloaded and installed by the user. The most common plugins
are [Adobe Flash](http://en.wikipedia.org/wiki/Adobe_flash), [Adobe
Reader](http://en.wikipedia.org/wiki/Acrobat_reader) and
[Java](http://en.wikipedia.org/wiki/Java_plugin).
In general, existing plugins cannot be sandboxed like the render process because
they rely on file system and network access as well as use of native fonts.
Therefore, Chrome supports [out of process
plugins](http://www.chromium.org/developers/design-documents/plugin-architecture)
that run in a separate process with full privileges (i.e. no sandbox) and
communicate with the renderer and browser via
[IPC](http://www.chromium.org/developers/design-documents/inter-process-communication).
Chrome also supports [in process
plugins](http://www.chromium.org/developers/design-documents/plugin-architecture).
They run within a render process and can use faster direct access for
communication. They have also been used as an integration mechanism for adding
new statically linked functionality to the browser.

### Netscape Plugin API (NPAPI) (No longer used by Native Client)

[NPAPI](https://wiki.mozilla.org/NPAPI) is a common cross-browser plugin
framework used by plugins for exchanging data with the browser. It is
implemented by Chrome, Firefox and most other web browsers, excluding MS
Internet Explorer, which stopped supporting it in favor of
[ActiveX](http://en.wikipedia.org/wiki/ActiveX_control).
Contrary to other single-process browsers, Chrome
[supports](http://code.google.com/chrome/extensions/npapi.html) NPAPI plugins
out of process.
While this improves stability, security is still an issue - even in Chrome,
NPAPI plugins have full permissions of the current user to access resources and
fork-exec new processes.
Although NPAPI is intended to be platform independent, in reality it is not
fully so. NPAPI is a weak "standard", and every browser implements it somewhat
differently. Moreover, NPAPI plugins end up relying on OS and browser specifics
for certain capabilities, such as 2D or 3D graphics events. Even keyboard and
mouse events typically use the native OS implementation. It can also be
difficult to achieve similar rendering of the plugin area within a page across
different systems.

### Pepper Plugin API (PPAPI)

[Pepper](https://wiki.mozilla.org/NPAPI:Pepper) started at Google as a way to
address portability and performance issues with NPAPI, particularly for out of
process plugins. The initial focused efforts eventually expanded to include
capabilities such as generic 2D and 3D graphics and audio.
The first Pepper API designs were created to minimize the changes from legacy
NPAPI, hoping to ease adoption by browser vendors and plugin developers. This
design was implemented and is available to NaCl modules in Chrome 5. The Pepper
APIs were redesigned subsequent to the initial design, deviating more
substantially from legacy NPAPI while, hopefully, also improving the interfaces.
The [revised interfaces](http://code.google.com/p/ppapi/w/list) are sometimes
referred to as “PPAPI” or “Pepper2” and should be available in Chrome 6.

Although traditional NPAPI plugins in Chrome run only out of process, Chrome 5
supports Pepper plugins only in process. Being a somewhat experimental feature,
the only way to load trusted Pepper plugins is through the browser command-line
options. In the future, Pepper plugins will only be supported within Native
Client.

### Native Client (NaCl)

[Native Client ](http://code.google.com/p/nativeclient/)is a sandboxing
technology for safe execution of platform-independent untrusted native code in a
web browser. It allows real-time web applications that are compute-intensive
and/or interactive (e.g. games, media, large data analysis, visualizations) to
leverage the resources of a client's machine and avoid costly network access
while running in a secure environment with restricted access to the host.
[Native Client SDK](http://gonacl.com) is a software development kit for
creating Native Client executables (abbreviated as nexe) from scratch or from
the existing platform-specific web-based native applications. It consists of a
[GNU](http://en.wikipedia.org/wiki/GNU_Project)-based toolchain with customized
versions of [gcc](http://en.wikipedia.org/wiki/GNU_Compiler_Collection),
[binutils](http://en.wikipedia.org/wiki/Binutils) and
[gdb](http://en.wikipedia.org/wiki/Gdb) (32-bit x86 only), precompiled API
libraries and various examples and how-tos. The two usage models include porting
desktop apps and extending web apps with fast native code.
[Naclports](http://code.google.com/p/naclports/) is a collection of ports of
various open-sourced projects (like [zlib](http://en.wikipedia.org/wiki/zlib))
to Native Client for gradual up-streaming. It is still in early stages of
development and is intended to be modeled after
[Macports](http://www.macports.org/).
NaCl started as a downloadable NPAPI plugin for multiple browsers, including
Firefox, Safari, Opera and Chrome and was designed to transparently load and run
other NPAPI plugins compiled as nexes and embedded into the page as a source
file of an [plugin
element](https://developer.mozilla.org/en/Gecko_Plugin_API_Reference/Plug-in_Basics#Using_HTML_to_Display_Plug-ins)
with “application/x-nacl-srpc” type.
NaCl includes a "service runtime" subsystem that provides a reduced system call
interface and resource abstractions to isolate nexes from the host. It provides
a [POSIX](http://en.wikipedia.org/wiki/Posix)-like environment for nexe
execution and is used by nexes to communicate with each other and the browser.
The nexes are run using a loader program, sel_ldr (secure
[ELF](http://en.wikipedia.org/wiki/Executable_and_Linkable_Format) loader),
which is launched as a separate process. The sel_ldr process communicates with
the NaCl plugin via [SRPC](/system/errors/NodeNotFound) over IMC \[citation
needed\].
NaCl was [integrated](/system/errors/NodeNotFound) into Chrome 5 as an
in-process Pepper plugin. The NaCl modules that it runs can utilize the Pepper
API for browser interaction. Part of the integration was to add special support
to Chrome to allow sandboxed plugins to launch the sel_ldr process. NaCl can be
enabled in Chrome 5 and later with the --enable-nacl command-line flag at
browser start-up by developers who want to develop Native Client modules. From
Chrome 14 and onwards Native Client is on by default and can be used in [Chrome
Web Store](https://chrome.google.com/webstore) apps. For development purposes
Native Client is also available to [unpacked
extensions](http://code.google.com/chrome/extensions/getstarted.html). Once we
launch Portable Native Client (PNaCl)
([PDF](http://nativeclient.googlecode.com/svn/data/site/pnacl.pdf)), we plan to
enable Native Client for web pages in general (i.e., not limited to Chrome Web
Store apps).

### Trusted vs Untrusted

In the presence of a sandbox environment, trusted code runs outside of the
sandbox and can perform privileged operations while untrusted code is prohibited
from doing so by the enclosing sandbox, which isolates potentially misbehaving
or malicious software from the rest of the system.
Within the scope of Native Client, whether code is untrusted or trusted depends
on whether it will run inside of a NaCl sandbox (regardless of any outer
sandbox). The code that implements the sandbox abstraction is trusted. The code
that the sandbox hosts is untrusted. Untrusted code is built using NaCl SDK or
any other compiler that outputs binaries that honor alignment and instruction
restrictions and can be validated by NaCl. NaCl will statically verify that
nexes do not attempt privileged operations and therefore do not need to be
trusted.
To summarize, the traditional NPAPI plugins running out of process and outside
of any sandbox are considered trusted. The in process Pepper plugins running
within the Chrome sandbox are trusted with respect to NaCl. And the out of
process plugins (that must run inside Native Client) are untrusted. NaCl's
sel_ldr process is trusted and can do Chrome sandbox system calls on behalf of
the untrusted nexes running within it.

### Diagram

![](/nativeclient/getting-started/getting-started-background-and-basics/nacl_diagram.png)

### Related Documentation

Chrome

<http://www.chromium.org/developers/design-documents/multi-process-architecture>

<http://www.chromium.org/developers/design-documents/inter-process-communication>

<http://www.chromium.org/developers/design-documents/plugin-architecture>

<http://code.google.com/chrome/extensions/npapi.html>

Plugins

<https://developer.mozilla.org/en/Plugins>

<https://developer.mozilla.org/en/Gecko_Plugin_API_Reference/Scripting_plugins>

<https://developer.mozilla.org/en/Gecko_Plugin_API_Reference/Plug-in_Basics>

<https://wiki.mozilla.org/NPAPI>

<https://wiki.mozilla.org/NPAPI:Pepper>

NaCl

<http://code.google.com/p/nativeclient/wiki/Papers>

<http://code.google.com/p/nativeclient/>

==<http://GoNaCl.com>==

<http://code.google.com/p/naclports/>

<http://www.chromium.org/nativeclient/simple-rpc>

[Native Client integration with Chrome](/system/errors/NodeNotFound)

### Articles and Blog Posts

*Important: this section is kept for historic purposes, but is no longer being
updated. For news and announcements see the SDK site at
[GoNaCl.com](http://GoNaCl.com) and the [announcements
list](https://groups.google.com/group/native-client-announce).*

Google

12/08
<http://googlecode.blogspot.com/2008/12/native-client-technology-for-running.html>

12/08
<http://googleonlinesecurity.blogspot.com/2008/12/native-client-technology-for-running.html>

Other

01/09
<http://blog.taragana.com/index.php/archive/google-native-client-a-detailed-discussion/>

06/09 <http://lwn.net/Articles/335974/>

03/10
<http://arstechnica.com/.../google-bakes-flash-into-chrome-hopes-to-improve-plugin-api.ars>

04/10 <http://news.cnet.com/8301-30685_3-20003527-264.html>
