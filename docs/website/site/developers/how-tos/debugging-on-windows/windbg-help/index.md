---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/debugging-on-windows
  - Debugging Chromium on Windows
page_name: windbg-help
title: WinDbg help
---

[WinDbg](http://www.microsoft.com/whdc/DevTools/Debugging/default.mspx) is a
great, free tool. It is more powerful than Visual Studio's built-in debugger,
but is harder to use (kind of like gdb on Linux). You can retrieve the [latest
SDK version](http://msdn.microsoft.com/en-us/windows/hardware/hh852365) from
Microsoft's web site. You should end up with two versions of the tool: the
32-bit debugger and the 64-bit debugger. If you already have it installed or if
you are using the packaged Chromium toolchain (which includes windbg) then you
can launch it using tools\\win\\windbg32.bat or tools\\win\\windbg64.bat. These
batch files find and run the appropriate version, wherever it is.

You can also install WinDbg Preview, the new/preview version of WinDbg, from the
Microsoft Store or [this link](https://aka.ms/windbg/download). Discussion of
various issues around getting this version can be found [here](https://github.com/microsoftfeedback/WinDbg-Feedback/issues/19#issuecomment-1512275411).
WinDbg Preview is being more actively developed and has a friendlier
out-of-box experience so it is probably the best choice for new WinDbg users.
The executable name for WinDbg Preview is WinDbgX.exe so substitute that for
windbg in examples below. WinDbgX.exe should automatically be in your path once
you install it.

### Initial setup

Once you're started, you may wish to fix a few things. If you have run WinDbg
before and saved any workspaces, you may wish to start with a clean slate by
deleting the key HKCU\\Software\\Microsoft\\Windbg using your favorite registry
editor.

1.  Set the environment variable **_NT_SYMBOL_PATH**, as per [Symbol
            path for Windows
            debuggers](https://msdn.microsoft.com/library/windows/hardware/ff558829.aspx#controlling_the_symbol_path)
            (e.g., File -&gt; Symbol Search Path), to:
    https://chromium-browser-symsrv.commondatastorage.googleapis.com****SRV\*c:\\code\\symbols\*https://msdl.microsoft.com/download/symbols;SRV\*c:\\code\\symbols\*
2.  Configure WinDbg to use a sensible window layout by navigating
            explorer to "**C:\\Program Files (x86)\\Windows
            Kits\\10\\Debuggers\\x64\\themes**" and double-clicking on
            **standard.reg** (not needed with WinDbg Preview).
3.  Launch **windbg.exe** and:
    1.  In the menu *File,* *Source File Path...*, set the path to
                **srv\***.
        *   If you have a local checkout of the source, you can just
                    point *Source Path* to the root of your code (src). Multiple
                    paths are separated by semicolons.
        *   If you want to download the individual source files to a
                    given directory, add the destination to the path like so:
                    **srv\*c:\\path\\to\\downloaded\\sources;c:\\my\\checkout\\src**
    2.  In the menu *View*, *Source language file extensions*..., add
                **cc=C++** to have automatic source colors.
    3.  Optionally, customize the window layout as desired via the
                *View* menu, and dock the windows as you want them to be. Note
                that the UI allows multiple "Docks" and each Dock can have
                multiple tiled panels in it, and each panel can have multiple
                tabbed windows. You may want to have source files to be tabbed
                on the same panel, and visible at the same time as local
                variables and the stack and command windows. It is useful to
                realize that by default windbg creates a workspace per debugged
                executable or minidump, so each target can have its own
                configuration. The "default" workspace is applied to new
                targets.
    4.  Optionally, run additional customization commands such as:
        1.  **.asm no_code_bytes**
            *   disables display of opcodes
        2.  **.prompt_allow -sym -dis -ea -reg -src**
            *   Disables display of symbol for the current instruction,
                        disassembled instructions, effective address of current
                        instruction, current state of registers and source line
                        for the current instruction
        3.  **.srcfix**
            *   Enables source server. This tells the debugger to use
                        information in the Chrome PDBs to download the correct
                        version of all necessary source files.
    5.  Use *File, Save Workspace* to make this new configuration the
                default for all future execution.
    6.  Exit windbg.
4.  In Windows Explorer, associate **.dmp** extension with windbg.exe.
            You may have to manually add -z to the open command like so:
            **"...\\windbg.exe" -z "%1"** to make this work properly.
            Alternatively, run **windbg.exe -IA**
5.  Register as the default just in time debugger: **windbg.exe -I**

**To set your symbol and source environment variables permanently, you can run
the following commands:**

**setx _NT_SYMBOL_PATH
SRV\*c:\\code\\symbols\*https://msdl.microsoft.com/download/symbols;SRV\*c:\\code\\symbols\*https://chromium-browser-symsrv.commondatastorage.googleapis.com**

**setx _NT_SOURCE_PATH SRV\*c:\\code\\source;c:\\my\\checkout\\src**

### Common commands

*   **dt this-&gt;member_**
    *   Displays the data

*   **x chrome\*!\*function_name**
    *   Finds a symbol.

*   **.open -a \[symbol address or complete symbol name found by using
            x\]**
    *   Opens the source file containing the specified symbol. Pretty
                neat.
*   **k**
    *   Displays the stack.
    *   **kP**: Show all parameters.
    *   **kM**: Show links to each stack frame.
    *   Clicking on the links shifts into the other stack frame,
                allowing you to browse locals, etc.

*   **?? \[data name\]**
    *   Quick evaluation of a C++ symbol (local variable, etc). You
                don't need to specify this-&gt; for member variables but it's
                slower if you don't.

*   **dv \[/V\]**
    *   Displays local variables
*   **dt varname**
    *   Displays a variable.

*   **dd address**
    *   Displays the contents of memory at the given address (as
                doubles... **dc, dw, dq** etc)

*   **dt -r1 type address**
    *   Displays an object of the given type stored at the given
                address, using 1 level of recursion.
*   **uf symbol**
    *   Disassembles a function showing source line number.

*   **!stl**
    *   Displays some stl structures (visualizer)

*   **dt -n &lt;type&gt;**
    *   Displays a type forcing the name to the supplied type (when
                there are problematic characters in the name)

*   **~\*n**
    *   Freezes all threads

*   **~4m**
    *   Thaws thread number 4

*   **Ctrl-Shift-I**
    *   Sets the selected source line to be the next line to be executed

*   **F5, Ctrl-Shift-F5, F9, F10, F11**
    *   Run, restart, toggle breakpoint, step over, step into.

One of the major benefits of WinDbg for debugging Chromium is its ability to
automatically debug child processes. This allows you to skip all the complicated
instructions above. The easiest way to enable this is to check "*Debug child
processes also*" in the "*Open Executable*" dialog box when you start debugging
or start "**windbg.exe -o**". **NOTE that on 64-bit Windows you may need to use
the [64-bit
WinDbg](http://www.microsoft.com/whdc/devtools/debugging/install64bit.mspx) for
this to work.** You can switch dynamically the setting on and off at will with
the **.childdbg 1|0** command, to follow a particular renderer creation. You can
also attach to a running process (**F6**) and even detach without crashing the
process (**.detach**)

### **Common commands when working with a crash**

*   **!address address**
    *   Displays information about the specified address. This can be used as
                !address @eip or !address @rip to get information about the
                instruction that crashed. "Content source: 1 (target)" means the
                memory came from the target machine - from the crash dump.
                "Content source: 2 (mapped)" means the memory came from an image
                file, presumably from the symbol server. !chkimg is only
                meaningful if (target) bytes are available to compare. See
                .dumpdebug for more details. If crashpad is properly configured
                then it will record code bytes from the crash location.
                Note that sometimes running !address will undo .ecxr and you
                will need to run that command again.

*   **!analyze -v**
    *   Displays a basic crash analysis report.

*   **!chkimg**
    *   Checks for memory corruption by comparing code bytes from the symbol
                server with code bytes from the minidump. See !address for how
                to see whether these results are meaningful.

*   **.ecxr**
    *   Switch the context to the exception record.

*   **.exr -1**
    *   Display details about the exception (address dereferenced, for instance)

*   **dds address**
    *   Displays symbols following address (as in a stack or vtable)

*   **.dumpdebug**
    *   Dumps a detailed record of what data (handles, modules, memory ranges)
                was recorded in the crash dump.

*   **k = address address address**
    *   Rebuilds a call stack assuming that address is a valid stack
                frame.

*   **lm vm chr\***
    *   Lists verbose information about all modules with a name that
                starts with chr

*   **ln address**
    *   Lists all symbols that match a given address (dedups a symbol).

*   **.load wow64exts**
    *   On a 64-bit debugger, load the 32-bit extensions so that the
                current architecture can be switched

*   **.effmach x86**
    *   Switches the current architecture to 32-bit.

*   **.effmach x86; k = @ebp @ebp @ebp**
    *   Shows the 32-bit call stack from a 64-bit dump

For more info, see [this example of working with a crash
dump](/developers/how-tos/debugging-on-windows/example-of-working-with-a-dump),
consult the program help (really, it's exhaustive!), see [Common windbg
commands](http://windbg.info/doc/1-common-cmds.html) or use your favorite search
engine.

### **Random handy hints**

To set attach to child processes, and also skip the first breakpoint and the
extra breakpoint on process exit (this gives you a pretty responsive Chrome you
can debug):

**sxn ibp**

**sxn epr**

**.childdbg 1**

**g**

You can also get this effect by using the -g -G -o options when launching
windbg, as in:

**windbg -g -G -o chrome.exe**

To automatically attach to processes you want to run over and over with complex
command lines, just attach WinDbg to your command prompt and then **.childdbg
1** the command prompt - any processes launched from there will automatically be
debugged. H/T pennymac@

To set a breakpoint in the current process you can use this module/function
syntax, among others:

**bp msvcrt!invalid_parameter**

To apply this to future processes that are created (assuming child process
debugging is enabled) you can use this syntax, which says to run the bp command
whenever a new process is created:

**sxe -c "bp msvcrt!invalid_parameter" cpr**

If you want a chance to do this when you first launch the browser process then
you need to launch it without -g (so that the first breakpoint will be hit). You
will probably then want to disable the "Create process" breakpoint and "Initial
breakpoint" with these commands:

**sxn ibp; sxn epr;**

These are equivalent to going to Debug-&gt; Event Filters and setting "Create
process" and "Initial breakpoint" to "Ignore".

Always use **--user-data-dir** when starting Chrome built with
**branding=Chrome** or else you're going to have a bad time.

### Resources

*   [Extensive slide deck](http://windbg.info/doc/2-windbg-a-z.html)
            \[windbg.info\]
