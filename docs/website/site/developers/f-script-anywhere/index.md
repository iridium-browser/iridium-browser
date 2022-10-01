---
breadcrumbs:
- - /developers
  - For Developers
page_name: f-script-anywhere
title: F-Script Anywhere
---

F-Script Anywhere turns any Mac app into a smalltalk-y environment: You're able
to click on any view to learn its type, and you can send messages to every
object in the program at run time. It's very useful to get around in Chrome: If
you're wondering which piece of code is responsible for the findbar, you'd just
click the findbar and get the name of the class that implements it. Put that in
<http://cs.chromium.org>, and you're done. This page explains how to install and
use F-Script Anywhere.

### Installing F-Script Anywhere

1.  Download
            <http://www.fscript.org/download/F-ScriptInjectionService.zip> and
            follow the instructions in the included readme.txt.
2.  Make sure you also downloaded FScript.Framework (as described in the
            aforementioned readme.txt!)
3.  If you run OSX 10.8 or later, you need to fix up the automator
            workflow. Edit the injection service with Automator, and make sure
            the "New Text File" action generates plain text. Also, ensure that
            the following AppleScript refers to /tmp/gdbtemp***.txt*** - "New
            Text File" now enforces an extension.

See <http://pmougin.wordpress.com/2010/01/05/the-revenge-of-f-script-anywhere/>
for more information.

### Using F-Script Anywhere

Click "Chrome-&gt;Services-&gt;Inject F-Script into application". **Note**: This
won't work for programs that are running in gdb (e.g. it won't work in Chromium
if you started it through Xcode).

[<img alt="image"
src="/developers/f-script-anywhere/Screen%20shot%202011-04-01%20at%2011.52.02%20AM.png">](/developers/f-script-anywhere/Screen%20shot%202011-04-01%20at%2011.52.02%20AM.png)

This will take about 15 seconds in Release builds and several minutes in Debug
builds.

Alternatively, you could do it right from gdb (the Service is an Automator
script that runs gdb for you, and there are several Automator scripts floating
around, some of them don't work):

&gt;gdb --args ./out/Release/Chromium.app/Contents/MacOS/Chromium (or wherever
it is)

(gdb) r

.....

&lt;Ctrl-C&gt;

(gdb) p (char)\[\[NSBundle
bundleWithPath:@"/Library/Frameworks/FScript.framework"\] load\]

(gdb) p (void)\[FScriptMenuItem insertInMainMenu\]

(gdb) c

When it's done, an "F-Script" menu entry will appear in the top level menu:

[<img alt="image"
src="/developers/f-script-anywhere/Screen%20shot%202011-04-01%20at%2011.57.03%20AM.png">](/developers/f-script-anywhere/Screen%20shot%202011-04-01%20at%2011.57.03%20AM.png)

Click "Open Object Browser". In the window that opens, click "Select View". Say
you're wondering which class implements the Omnibox, so click the Omnibox next.
F-Script Anywhere will tell you that the Omnibox is an AutocompleteTextField.

[<img alt="image"
src="/developers/f-script-anywhere/Screen%20shot%202011-04-01%20at%204.27.30%20PM.png">](/developers/f-script-anywhere/Screen%20shot%202011-04-01%20at%204.27.30%20PM.png)

You can even send methods to this object: Type "sethid" into the search box in
the upper right corner, and then click "setHidden:" on the right:

[<img alt="image"
src="/developers/f-script-anywhere/Screen%20shot%202011-04-01%20at%2011.59.12%20AM.png">](/developers/f-script-anywhere/Screen%20shot%202011-04-01%20at%2011.59.12%20AM.png)

Type "YES" into the popup that opens, and the Omnibox will disappear:

[<img alt="image"
src="/developers/f-script-anywhere/Screen%20shot%202011-04-01%20at%2011.59.21%20AM.png">](/developers/f-script-anywhere/Screen%20shot%202011-04-01%20at%2011.59.21%20AM.png)

You can also navigate to a view's superview this way, and do many other fun
things. Read the F-Script documentation for more. F-Script Anywhere works with
every OS X app, not just with Chrome.
