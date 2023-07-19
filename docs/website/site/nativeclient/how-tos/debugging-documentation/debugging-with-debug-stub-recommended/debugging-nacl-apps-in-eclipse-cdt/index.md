---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/how-tos
  - '2: How Tos'
- - /nativeclient/how-tos/debugging-documentation
  - Debugging Documentation
- - /nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended
  - Debugging with debug stub (recommended)
page_name: debugging-nacl-apps-in-eclipse-cdt
title: Debugging NaCl apps in Eclipse CDT
---

Eclipse CDT plugin is designed for development with GNU tools. Its flexibility
allows to configure it for developing NaCl applications without using any
additional plugins. The cost of this flexibility is that a lot of advanced
features are not available. The following instructions are written for Windows
but they can be used on any OS if you replace Windows paths to paths on your OS.

## Creating NaCl project

Create NaCl Project directory inside Eclipse workspace
(C:\\Users\\username\\workspace by default). Copy all files from
nacl_sdk\\pepper_22\\examples\\hello_world except hello_world.c to this
directory. Create src subdirectory and copy hello_world.c file there. Placing
source files in the src directory makes cleaner project structure. Now go to
File-&gt;New-&gt;Makefile Project with Existing Code menu in Eclipse. Choose
project name, enter project folder location and select Cross GCC toolchain.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20new%20project.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20new%20project.png)

Your new project will look like this.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20project%20view.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20project%20view.png)

## Building NaCl project

Open Makefile in the editor. Comment out lines
ALL_TARGETS+=win/Debug/hello_world.nmf and
ALL_TARGETS+=win/Release/hello_world.nmf and replace hello_world.c with
src/hello_world.c (Edit-&gt;Find/Replace... in the menu or Ctrl+F with default
key bindings). Then right click on the project and choose Preferences in the
popup menu. Go to C/C++ Build-&gt;Build Variables page and add NACL_SDK_ROOT
variable. Point the variable to the pepper version you want to build with. If
you downloaded NaCl SDK to c:\\nacl_sdk directory and want to use 22 pepper, set
it to c:\\nacl_sdk\\pepper_22.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20new%20build%20variable.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20new%20build%20variable.png)

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20build%20variables.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20build%20variables.png)

Now we can use this variable to set build command on C/C++ Build page to
${NACL_SDK_ROOT}/tools/make NACL_SDK_ROOT=${NACL_SDK_ROOT}. On Linux and Mac use
make NACL_SDK_ROOT=${NACL_SDK_ROOT}.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20build.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20build.png)

Enable parallel build on Behaviour tab.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20build%20behaviour%20tab.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20build%20behaviour%20tab.png)

Press OK in properties window. You can now build the project by right clicking
on it and selecting Build Project in the popup menu or by pressing hammer icon
[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20hammer%20icon.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20hammer%20icon.png)
in the toolbar. Make build log will be shown in console view.

## Set up eclipse indexer

Eclipse indexer doesn't use external compiler and so it should be configured
separately. Open project properties and go to C/C++ General-&gt;Paths and
Symbols page. Open Source Location tab, add src folder using Add Folder...
button and remove root of the project using Delete button.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20source%20location.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20source%20location.png)

Then go to Output Location path, add glibc and newlib folders there and remove
root of the project.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20output%20location.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20output%20location.png)

Now let us setup include paths. Go to Includes tab and press Add... button.
Enter ${NACL_SDK_ROOT}/include in the directory field and select Add to all
configurations and Add to all languages checkboxes.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20add%20directory%20path%201.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20add%20directory%20path%201.png)

Then press Add... button again and add
${NACL_SDK_ROOT}/toolchain/win_x86_glibc/x86_64-nacl/include directory (replace
win_x86_glibc with linux_x86_glibc or mac_x86_glibc if you use Linux or Mac OS X
accordingly).

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20add%20directory%20path%202.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20add%20directory%20path%202.png)

Press OK button in properties window and open hello_world.c. There should not be
any compile errors now.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20with%20indexer%20enabled.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20with%20indexer%20enabled.png)

## Debug NaCl newlib application

Launching and debugging applications in Eclipse is done via running and
debugging configurations. Usually they are created automatically. Unfortunately,
NaCl debugging is done using remote debugging protocol. Setting up remote
connection requires parameters that can't be determined automatically. So we
have to create debugging configuration by hand. Go to Run-&gt;Debug
Configurations... menu or press on debug icon arrow [<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20debug%20icon.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20debug%20icon.png)
in toolbar and choose Debug Configurations... in the popup menu. Select C/C++
Remote Application in Debug Configurations windows and press new button [<img
alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20new%20debug%20configuration%20icon.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20new%20debug%20configuration%20icon.png)
above filter text field or choose New in the popup menu.

Change configuration name to NaCl Project newlib and fill C/C++ Application
field with ${workspace_loc}\\NaCl
Project\\newlib\\Debug\\hello_world_x86_64.nexe. We use ${workspace_loc} instead
of ${project_loc} since the later depends on currently selected project.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20debug%20configuration%20main.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20debug%20configuration%20main.png)

Now press Select other link at the bottom of the window. If you don't see the
link, skip this step. Select Use configuration specific settings checkbox and
choose GDB (DSF) Manual Remote Debugging Launcher in the list.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20select%20preferred%20launcher.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20select%20preferred%20launcher.png)

Press OK and go to the debugger tab. Change Stop on startup at field to
Instance_DidCreate or deselect the Stop on startup checkbox. On Main tab in
Debugger Options set GDB debugger field to
c:\\nacl_sdk\\pepper_canary\\toolchain\\win_x86_glibc\\bin\\x86_64-nacl-gdb.exe.
Ensure that Non-stop mode checkbox is not selected since nacl-gdb doesn't
support this mode.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20debug%20configuration%20Debugger%20Main.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20debug%20configuration%20Debugger%20Main.png)

Set port number to 4014 in Gdbserver Settings tab.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20debug%20configuration%20Debugger%20Gdbserver%20Settings.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20debug%20configuration%20Debugger%20Gdbserver%20Settings.png)

Save debug configuration by clicking Apply button.

Now you need to launch chrome with --no-sandbox and --enable-nacl-debug flags
and run your NaCl application. NaCl debug stub will stop application at first
instruction. Then you need to open Debug Configuration window, select our debug
configuration and press Debug button. Next time, you can launch it by pressing
debug icon arrow that shows 10 last debug configurations.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20last%20debug%20configurations.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20last%20debug%20configurations.png)

When program will stop on the start breakpoint or breakpoint you set in project,
Eclipse will switch to debug perspective.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20hit%20breakpoint.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20hit%20breakpoint.png)

## Debugging NaCl glibc application

Create gdb_init.txt file and add line nacl-manifest
"c:\\\\Users\\\\username\\\\workspace\\\\NaCl
Project\\\\glibc\\\\Debug\\\\hello_world.nmf". If you use debugger from
pepper_29+, you don't need to double slashes.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20gdb_init%20file.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20gdb_init%20file.png)

Go to Run-&gt;Debug configuration... menu and duplicate newlib debug
configuration using Duplicate item in popup menu. Change new configuration name
to NaCl Project glibc and C/C++ Application field to ${workspace_loc}\\NaCl
Project\\glibc\\Debug\\lib64\\runnable-ld.so.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20debug%20configuration%20glibc%20main.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20debug%20configuration%20glibc%20main.png)

Then go to Debugger tab and change GDB command file on Main subtab to
c:\\Users\\username\\workspace\\NaCl Project\\gdb_init.txt. Ensure that Non-stop
mode checkbox is not selected.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20debug%20configuration%20glibc%20Debugger%20Main.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20debug%20configuration%20glibc%20Debugger%20Main.png)

Press Apply button and close the window. Now you need to run your NaCl
application in chrome with --enable-nacl-debug and --no-sandbox flags, select
this debug configuration in Debug Configurations window and press Debug button.
You may see some error messages like

```none
No source file named hello_world.c.
warning: Could not load shared library symbols for runnable-ld.so.
Do you need "set solib-search-path" or "set sysroot"?
```

You should ignore them.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20glibc%20hit%20breakpoint.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20glibc%20hit%20breakpoint.png)

## Troubleshooting

If something goes wrong, you can look on gdb input and output. Press on the
arrow of console icon in Console view to show "NaCl Project glibc \[C/C++ Remote
Application\] gdb traces" console.

[<img alt="image"
src="/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20consoles.png">](/nativeclient/how-tos/debugging-documentation/debugging-with-debug-stub-recommended/debugging-nacl-apps-in-eclipse-cdt/eclipse%20consoles.png)
