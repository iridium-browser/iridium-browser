---
breadcrumbs:
- - /blink
  - Blink (Rendering Engine)
page_name: getting-started-with-blink-debugging
title: Getting Started with Blink Debugging
---

[TOC]

### Introduction

While many of the tools and tips on this page can be used for it, this page
focuses on debugging Blink outside the context of the web tests. For more web
test specific instructions, see [this
page](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/testing/web_tests.md).
For more general Chromium debugging info, see the respective pages for debugging
on [Windows](/developers/how-tos/debugging-on-windows),
[Mac](/developers/how-tos/debugging-on-os-x), and
[Linux](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/linux_debugging.md).

### Linux

#### Getting Started

There are two main ways to get into blink: via debugging the chromium binary
itself or `content_shell`. For most purposes of exclusive Blink debugging, the
latter is the recommended option because it drastically reduces size and
complexity. This means building `content_shell`, which should be as simple as
making it the build target for your build method of choice. This should stick a
`content_shell` binary in your `out/Default` directory.

`content_shell` itself takes as an argument the HTML file you wish to run Blink
on. Furthermore, one of the simplest types of debugging you might want to do is
to see the basic page structure after a page load (this internal structure in
Blink is called the Layout Tree, not to be confused with the DOM Tree or the
Line Box Tree). You can do this with a simple command line option of
`--run-web-tests`. Thus, one of your simplest debugging tools, seeing the page
structure after a page load, might look something like:

```none
content_shell --run-web-tests test.html
```

#### Starting the Debugger

Debugging on Linux is generally done with GDB, so we will assume that's what you
are using here. Not surprisingly, you will almost always want to compile Blink
in debug mode to get all the symbols and tools you will need.

You will also want to use the `gdbinit` script to help gdb find source code,
pretty-print common types, and other such niceties that will avoid you staring
at assembly code. See
[`docs/gdbinit.md`](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/gdbinit.md)
for more details.

**Important: Previous revisions of this document suggested running with the
`--single-process `when debugging. That flag is no longer supported; it may
work, it may fail outright, and it may appear to work yet manifest subtle
differences in behavior that will cause you to waste many hours in understanding
them.**

You may need to modify your system's ptrace security policy before you can debug
the renderer
([details](http://askubuntu.com/questions/41629/after-upgrade-gdb-wont-attach-to-process));
run this command:

```none
echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
```

Both `content_shell` and `chromium` spawn renderer sub-processes; to debug
blink, you need to attach a debugger to one of those sub-processes. The simplest
way to do this is with the `--no-sandbox` and `--renderer-startup-dialog`
parameters:

```none
out/Default/content_shell --no-sandbox --renderer-startup-dialog test.html
```

When you do this, `content_shell` will print a message like this:

```none
[10506:10506:0904/174115:2537132352130:ERROR:child_process.cc(131)] Renderer (10506) paused waiting for debugger to attach. Send SIGUSR1 to unpause.
```

This tells you that the pid of the renderer process is 10506. You can now attach
to it:

```none
gdb -p 10506
```

When gdb loads up, set whatever breakpoints you want in the blink code and
'continue'; for example:

```none
(gdb) b blink::LayoutView::layout
(gdb) c
```

Then, send SIGUSR1 to the renderer process to tell it to proceed:

```none
(gdb) signal SIGUSR1
```

If you are running web test, `third_party/blink/tools/debug_web_tests` does
almost everything except for SIGUSR1 automatically.

If you see 'Could not find DWO CU' errors, you may need to have a symlink to the
build directory from the current working directory.

```none
% cd third_party/blink
% ln -s ../../out/Default/obj .
% ./tools/debug_web_tests --target=Default
```

### General Useful Debugging Tools

#### Debugging functions

There are some key functions built into objects once you've reach a breakpoint
inside Blink. For the examples here, we'll assume you're using GDB on Linux.
These can be incredibly useful for showing the trees midway during execution to
try and identify points when things change. You can use the GDB command print to
display them. Here are some of Blink's debugging functions:

<table>
<tr>
<td> Function</td>
<td>Objects it's available on </td>
<td>Description</td>
</tr>
<tr>
<td> ShowTreeForThis()</td>
<td>`Node`s and LayoutObjects</td>
<td>Outputs the DOM tree, marking this with a \*</td>
</tr>
<tr>
<td> ShowLayoutTreeForThis()</td>
<td>LayoutObjects</td>
<td>Outputs the Layout tree, marking this with a \*</td>
</tr>
<tr>
<td> ShowLineTreeForThis()</td>
<td>LayoutObjects and InlineBoxes</td>
<td>Outputs the Inline Box tree for the associated block flow, marking all matching inline boxes associated with this with a \*</td>
</tr>
<tr>
<td> ShowDebugData()</td>
<td>DisplayItemLists</td>
<td>Outputs the list of display items and associated debug data</td>
</tr>
</table>

Assuming a local variable `child` in scope that's a `LayoutObject`, the
following will print the Layout Tree:

```none
(gdb) print child->showLayerTreeForThis()
```

`#### Blink GDB python library`

`When using a GDB build that supports python, there's a library of useful Blink
functions and pretty printers that can make working with some of Blink's types
easier and more convenient, such as pretty printers for LayoutUnit and
LayoutSize classes. You can find it at third_party/blink/tools/gdb/blink.py; see
[LinuxDebugging](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/linux/debugging.md)
for instructions.`

### Printing back trace

#### Use Chromium's StackTrace

```none
#include "base/debug/stack_trace.h"
...
base::debug::StackTrace().Print();
// or
LOG(ERROR) << base::debug::StackTrace();
```

and run Chrome with `--no-sandbox` command line option.

### Debugging Printing related issues

It is difficult to understand whether an issue lies in the Print Preview logic
or the Rendering logic. This
[document](https://docs.google.com/document/d/1aK27hiUPEm75OD4Dw2yQ9CmNDkLjL7_ZzglLdHW6UzQ/edit?usp=sharing)
uses some of the tools available to us to help solve this issue.
