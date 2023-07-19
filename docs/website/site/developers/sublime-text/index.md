---
breadcrumbs:
- - /developers
  - For Developers
page_name: sublime-text
title: Using Sublime Text as your IDE
---

## The below instructions are outdated.

## New instructions:
<https://chromium.googlesource.com/chromium/src/+/HEAD/docs/sublime_ide.md>

[TOC]

## [<img alt="image" src="/developers/sublime-text/SublimeExample.png">](/developers/sublime-text/SublimeExample.png)

## What is Sublime Text?

It's a fast, powerful and easily extensible code editor. Check out some of the
demos on the site for a quick visual demonstration.

Some of its benefits include:

*   Project support.
*   Theme support.
*   Works on Mac, Windows and Linux.
*   No need to close and re-open during a `gclient sync`.
*   Supports many of the great editing features found in popular IDE's
            like Visual Studio, Eclipse and SlickEdit.
*   Doesn't go to lunch while you're typing.
*   The UI and keyboard shortcuts are pretty standard (e.g. saving a
            file is still Ctrl+S on Windows).
*   It's inexpensive and you can evaluate it (fully functional) for
            free.

## Installing Sublime Text 2

Download and install from here: <http://www.sublimetext.com/>

Help and general documentation is available here:
<http://www.sublimetext.com/docs/2/>

*Assuming you have access to the right repositories, you can also install
Sublime via apt-get on Linux.*

## Preferences

Sublime configuration is done via JSON files. So the UI for configuring the text
editor is simply a text editor. Same goes for project files, key bindings etc.

To modify the default preferences, go to the Preferences menu and select
Settings-Default. Note that if you would rather like to make these settings user
specific, select Settings - User as this applies there as well. The difference
is that the default settings file already contains many settings that you might
want to modify.

Here are some settings that you might want to change (look these variables up in
the settings file and modify their value, you should not have to add them):

```none
    "rulers": [80],
    "tab_size": 2,
    "trim_trailing_white_space_on_save": true,
    "ensure_newline_at_eof_on_save": true,
    "translate_tabs_to_spaces" : true
```

The settings take effect as soon as you save the file.

If you've got a big monitor and are used to viewing more than one source file at
a time, you can use the View-&gt;Layout feature to split the view up into
columns and/or rows and look at multiple files at the same time. There's also
the Shift+F11, distraction free view that allows you to see nothing but code!
?8-D Sublime also supports dragging tabs out into new windows as Chrome
supports, so that might be useful as well.

*One thing to be aware of when editing these JSON files is that Sublime's JSON
parser is slightly stricter than what you might be used to from editing e.g. GYP
files. In particular Sublime does not like it if you end a collection with a
comma. This is legal: {"foo", "bar"} but not this: {"foo", "bar", }. You have
been warned.*

## Project files

Like configuration files, project files are just user editable JSON files.

Here's a very simple project file that was created for WebRTC and should be
saved in the parent folder of the trunk folder (name it webrtc.sublime-project).
It's as bare bones as it gets, so when you open this project file, you'll
probably see all sorts of files that you aren't interested in.

```none
{
	"folders":
	[
		{
			"path": "trunk"
		}
	]
}
```

Here is a slightly more advanced example that has exclusions to reduce clutter.
This one was made for Chrome on a Windows machine and has some Visual Studio
specific excludes. Save this file in the same directory as your .gclient file
and use the .sublime-project extension (e.g. chrome.sublime-project) and then
open it up in Sublime.

```none
{
	"folders":
	[
		{
			"path": "src",
			"name": "src",
			"file_exclude_patterns": [
				"*.vcproj",
				"*.vcxproj",
				"*.sln",
				"*.gitignore",
				"*.gitmodules",
				"*.vcxproj.*"
			],
			"folder_exclude_patterns": [
			  "build",
			  "out",
			  "third_party",
			  ".git",
			  "Debug",
			  "Release"
			]
		}
	]
}
```

## Navigating the project

Here are some basic ways to get you started browsing the source code.

*   **"Goto Anything" or Ctrl+P** is how you can quickly open a file or
            go to a definition of a type such as a class. Just press Ctrl+P and
            start typing.
*   **Open source/header file:** If you're in a header file, press Alt+O
            to open up the corresponding source file and vice versa. For more
            similar features check out the Goto-&gt;Switch File submenu.
*   **"Go to definition"**: Right click a symbol and select "Navigate to
            Definition". A more powerful way to navigate symbols is by using the
            Ctags extension and use the Ctrl+T,Ctrl+T shortcut. See the section
            about source code indexing below.

## Enable source code indexing

For a fast way to look up symbols, we recommend installing the CTags plugin. we
also recommend installing Sublime's Package Control package manager, so let's
start with that.

*   Install the Sublime Package Control package:
            <https://packagecontrol.io/installation>
*   Install Exuberant Ctags and make sure that ctags is in your path:
            <http://ctags.sourceforge.net/>
    *   On linux you should be able to just do: sudo apt-get install
                ctags
*   Install the Ctags plugin: Ctrl+Shift+P and type "Package Control:
            Install Package"

Once installed, you'll get an entry in the context menu when you right click the
top level folder(s) in your project that allow you to build the Ctags database.
If you're working in a Chrome project however, **do not do that at this point**,
since it will index much more than you actually want. Instead, do one of:

1.  Create a batch file (e.g. ctags_builder.bat) that you can run either
            manually or automatically after you do a gclient sync:

    ```none
    ctags --languages=C++ --exclude=third_party --exclude=.git --exclude=build --exclude=out -R -f .tmp_tags & ctags --languages=C++ -a -R -f .tmp_tags third_party\platformsdk_win8 & ctags --languages=C++ -a -R -f .tmp_tags third_party\WebKit & move /Y .tmp_tags .tags
    ```

    This takes a couple of minutes to run, but you can work while it is
    indexing.
2.  Edit the CTags.sublime-settings file for the ctags plugin so that it
            runs ctags with the above parameters. **Note**: the above is a batch
            file - don't simply copy all of it verbatim and paste it into the
            CTags settings file :-)

Once installed, you can quickly look up symbols with Ctrl+t, Ctrl+t etc. More
information here: <https://github.com/SublimeText/CTags>

One more hint - Edit your .gitignore file (under %USERPROFILE% or ~/) so that
git ignores the .tags file. You don't want to commit it. :)

If you don't have a .gitignore in your profile directory, you can tell git about
it with this command:

```none
Windows: git config --global core.excludesfile %USERPROFILE%\.gitignore
Mac, Linux: git config --global core.excludesfile ~/.gitignore
```

## Building with ninja

Assuming that you've got ninja properly configured and that you already have a
project file as described above, here's how to build Chrome using ninja from
within Sublime. For any other target, just replace the target name.

Go to Tools-&gt;Build System-&gt;New build system and save this as a new build
system:

```none
{
	"cmd": ["ninja", "-C", "out\\Debug", "chrome.exe"],
	"working_dir": "${project_path}\\src",
	"file_regex": "^[.\\\\/]*([a-z]?:?[\\w.\\\\/]+)[(:]([0-9]+)[):,]([0-9]+)?[:)]?(.*)$"
}
```

file_regex explained for easier tweaking in future:

<pre><code>
Aims to capture the following error formats while respecting [Sublime's perl-like group matching](http://sublimetext.info/docs/en/reference/build_systems.html):
 1. <i>d:\src\chrome\src\base\threading\sequenced_worker_pool.cc(670): error C2653: 'Foo': is not a class or namespace name</i>
 2. <i>../../base/threading/sequenced_worker_pool.cc:670:26: error: use of undeclared identifier 'Foo'</i>
 <i>3. ../../base/process/memory_win.cc(18,26):  error: use of undeclared identifier 'Foo'</i>
 <i>4. ../..\src/heap/item-parallel-job.h(145,31):  error: expected ';' in 'for' statement specifier</i>
"file_regex": "^[.\\\\/]*([a-z]?:?[\\w.\\-\\\\/]+)[(:]([0-9]+)[):,]([0-9]+)?[:)]?(.*)$"
                (   0   ) (   1  )(      2      ) (3 ) (  4 ) ( 5 ) (  6   )( 7 )(8 )
(0) Cut relative paths (which typically are relative to the out dir and targeting src/ which is already the "working_dir")
(1) Match a drive letter if any
(2) Match the rest of the file
(1)+(2) Capture the "filename group"
(3) File name is followed by open bracket or colon before line number
(4) Capture "line number group"
(5) Line # is either followed by close bracket (no column group) or comma/colon
(6) Capture "column filename group" if any.
(7) If (6) is non-empty there will be a closed bracket or another colon (but can't put it inside brackets as the "column filename group" only wants digits).
(8) Everything else until EOL is the error message.
</code></pre>

*On Linux and Mac, fix the targets up appropriately, fwd slash instead of
backslash, no .exe, etc*

Linux example:

```none
{
 // Pass -j1024 if (and only if!) building with GOMA.
 "cmd": ["ninja", "-C", "out/Debug", "blink", "-j1024"],
 "working_dir": "${project_path}/src",
 // Ninja/GN build errors are build-dir relative, however file_regexp
 // is expected to produce project-relative paths, ignore the leading
 // ../../ to make that happen.
 // ../../(file_path):(line_number):(column):(error_message)
 "file_regex": "^../../([^:\n]*):([0-9]+):?([0-9]+)?:? (.*)$"
}
```

or to avoid making ninja in the path or environment variables:

```none
{
 "cmd": ["/usr/local/google/home/MYUSERNAME/git/depot_tools/ninja", "-j", "150", "-C", ".", "chrome", "content_shell", "blink_tests"],
 "working_dir": "${project_path}/src/out/Release",
 "file_regex": "([^:\n]*):([0-9]+):?([0-9]+)?:? (.*)$",
 "variants":
   [
    {
      "cmd": ["/usr/local/google/home/MYUSERNAME/git/depot_tools/ninja", "-j", "150", "-C", ".", "chrome", "content_shell", "blink_tests"],
      "name": "chrome_debug_blink",
      "working_dir": "${project_path}/src/out/Debug",
      "file_regex": "([^:\n]*):([0-9]+):?([0-9]+)?:? (.*)$"
    }
  ]
}
```

Further [build system
documentation](http://docs.sublimetext.info/en/latest/reference/build_systems.html)
or [older
documentation](http://sublimetext.info/docs/en/reference/build_systems.html) (as
of Nov 2014 older is more complete).

This will make hitting Ctrl-B build chrome.exe (really quickly, thanks to
ninja), F4 will navigate to the next build error, etc. If you're using Goma, you
can play with something like: "cmd": \["ninja", ***"-j", "200",*** "-C",
"out\\\\Debug", "chrome.exe"\],.

You can also add build variants so that you can also have quick access to
building other targets like unit_tests or browser_tests. You build description
file could look like this:

```none
{  "cmd": ["ninja", "-j200", "-C", "out\\Debug", "chrome.exe"],  "working_dir": "${project_path}\\src",  "file_regex": "^[.\\\\/]*([a-z]?:?[\\w.\\\\/]+)[(:]([0-9]+)[):]([0-9]+)?:?(.*)$",  "variants":  [    {      "cmd": ["ninja", "-j200", "-C", "out\\Debug", "unit_tests.exe"],      "name": "unit_tests"    },    {      "cmd": ["ninja", "-j200", "-C", "out\\Debug", "browser_tests.exe"],      "name": "browser_tests"    }  ]}
```

This way, you have separate key binding to start the unit or browser tests
target build as follows:

```none
  { "keys": ["ctrl+shift+u"], "command": "build", "args": {"variant": "unit_tests"} },  { "keys": ["ctrl+shift+b"], "command": "build", "args": {"variant": "browser_tests"} }
```

And keep using "ctrl+b" for a regular, "chrome.exe" build. Enjoy!

## Example plugin

Sublime has a Python console window and supports Python plugins. So if there's
something you feel is missing, you can simply add it.

Here's an example plugin (Tools-&gt;New Plugin) that runs cpplint (assuming
depot_tools is in the path) for the current file and prints the output to
Sublime's console window (Ctrl+\`):

```none
import sublime, sublime_plugin
import subprocess
class RunLintCommand(sublime_plugin.TextCommand):
  def run(self, edit):
      command = ["cpplint.bat", self.view.file_name()]
      process = subprocess.Popen(command, shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
      print process.communicate()[1]﻿
```

Or, in Sublime Text 3:

from __future__ import print_function

import sublime, sublime_plugin

import subprocess

class RunLintCommand(sublime_plugin.TextCommand):

def run(self, edit):

print("AMI: %s" % self.view.file_name())

command = \["/home/fischman/src/depot_tools/cpplint.py", self.view.file_name()\]

process = subprocess.Popen(command, shell=False, stdout=subprocess.PIPE,
stderr=subprocess.PIPE)

output, error = process.communicate()

if error:

`                       print(error)`

Save this file as run_lint.py (Sublime will suggest the right location when you
save the plugin - Packages\\User).

You can run the command via the console like so:

```none
view.run_command('run_lint')
```

Note that here's an interesting thing in how Sublime works. CamelCaps are
converted to lower_case_with_undescore format. Note also that although the
documentation currently has information about "runCommand" member method for the
view object, this too is now subject to that convention.

Taking this a step further, you can create a keybinding for your new plugin.
Here's an example for how you could add a binding to your User key bindings
(Preferences-&gt;Key Bindings - User):

```none
[
    {  
        "keys": ["ctrl+shift+l"], "command": "run_lint"  
    }
]
```

Now, when you hit Ctrl+Shift+L, cpplint will be run for the currently active
view. Here's an example output from the console window:

```none
D:\src\cgit\src\content\browser\browsing_instance.cc:69:  Add #include <string> for string  [build/include_what_you_use] [4]
Done processing D:\src\cgit\src\content\browser\browsing_instance.cc
Total errors found: 1
```

As a side note, if you run into problems with the documentation as I did above,
it's useful to just use Python's ability to dump all properties of an object
with the dir() function:

```none
>>> dir(view)
['__class__', '__delattr__', '__dict__', '__doc__', '__format__', '__getattribute__', '__hash__', '__init__', '__len__', '__module__', '__new__', '__reduce__', '__reduce_ex__', '__repr__', '__setattr__', '__sizeof__', '__str__', '__subclasshook__', '__weakref__', 'add_regions', 'begin_edit', 'buffer_id', 'classify', 'command_history', 'em_width', 'encoding', 'end_edit', 'erase', 'erase_regions', 'erase_status', 'extract_completions', 'extract_scope', ... <snip>
```

### Compile current file using Ninja

As a more complex plug in example, look at the attached python file:
[compile_current_file.py](/developers/sublime-text/compile_current_file.py).
This plugin will compile the current file with Ninja, so will start by making
sure that all this file's project depends on has been built before, and then
build only that file.

First, it confirms that the file is indeed part of the current project (by
making sure it's under the &lt;project_root&gt; folder, which is taken from the
self.view.window().folders() array, the first one seems to always be the project
folder when one is loaded). Then it looks for the file in all the .ninja build
files under the &lt;project_root&gt;\\out\\&lt;target_build&gt;, where
&lt;target_build&gt; must be specified as an argument to the
compile_current_file command. Using the proper target for this file compilation,
it starts Ninja from a background thread and send the results to the output.exec
panel (the same one used by the build system of Sublime Text 2). So you can use
key bindings like these two, to build the current file in either Debug or
Release mode:

```none
  { "keys": ["ctrl+f7"], "command": "compile_current_file", "args": {"target_build": "Debug"} },
  { "keys": ["ctrl+shift+f7"], "command": "compile_current_file", "args": {"target_build": "Release"} },
```

If you are having trouble with this plugin, you can set the python logging level
to DEBUG in the console and see some debug output.

### Format selection (or area around cursor) using clang-format

Copy buildtools/clang_format/scripts/clang-format-sublime.py to
~/.config/sublime-text-3/Packages/User/ (or -2 if still on ST2) and add
something like this to Preferences-&gt;Key Bindings - User:

```none
"keys": ["ctrl+shift+c"], "command": "clang_format",
```

Miscellaneous tips

*   To synchronize the project sidebar with the currently open file,
            right click in the text editor and select "Reveal in Side Bar".
            Alternatively you can install the **SyncedSideBar** sublime package
            (via the Package Manager) to have this happen automatically like in
            Eclipse.
*   If you're used to hitting a key combination to trigger a build (e.g.
            Ctrl+Shift+B in Visual Studio) and would like to continue to do so,
            add this to your Preferences-&gt;Key Bindings - User file:
    *   { "keys": \["ctrl+shift+b"\], "command": "show_panel", "args":
                {"panel": "output.exec"} }
*   Install the **Open-Include** plugin (Ctrl+Shift+P, type:"Install
            Package", type:"Open Include"). Then just put your cursor inside an
            #include path, hit Alt+D and voila, you're there.
    *   If you want to take that a step further, add an entry to the
                right-click context menu by creating a text file named
                "context.sublime-menu" under "%APPDATA%\\Sublime Text
                2\\Packages\\User" with the following content:
        \[ { "command": "open_include", "caption": "Open Include" } \]

## Other useful packages

Assuming you've installed Package Control already
(<https://packagecontrol.io/installation>) you can easily install more packages
via:

1.  Open Command Palette (Ctrl-Shift-P)
2.  Type "Package Control: Install Package" (note: given ST's string
            match is amazing you can just type something like "instp" and it
            will find it :-)).

A few recommended packages:

*   [Case Conversion](https://github.com/jdc0589/CaseConversion)
            (automatically swap casing of selected text -- works marvel with
            multi-select -- go from a kConstantNames to ENUM_NAMES in seconds)
*   CTags (see detailed setup info above).
*   Git
*   Open-Include
*   [Text Pastry](https://github.com/duydao/Text-Pastry) (insert
            incremental number sequences with multi-select, etc.)
*   Wrap Plus (auto-wrap a comment block to 80 columns with Alt-Q)
