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
page_name: getting-started-with-debug-stub
title: Getting started with debug stub
---

### Introduction

New versions of chrome include NaCl with debug stub support. If
--enable-nacl-debug switch is passed to chrome or -g switch is passed to
sel_ldr, all NaCl applications start with the debug stub enabled. The debug stub
stops untrusted code at the first instruction inside the IRT and listens on a
TCP port of localhost network interface (4014 currently) for incoming connection
from nacl-gdb. When nacl-gdb connects to the debug stub, they talk with each
other using RSP protocol. Since remote debugging does not need to use
OS-specific debugging functions, we need only one version of nacl-gdb for both
32 and 64-bit debugging. Moreover, if one forwards port to a remote machine, it
is possible to use nacl-gdb for one OS to debug NaCl application on a different
OS. All the differences between OSes are abstracted away by debug stub.

## Get the debugger

The debugger is included in the latest version of NaCl SDK. It is located in
nacl_sdk/pepper_canary/toolchain/linux_x86_glibc/bin/x86_64-nacl-gdb on Linux,
nacl_sdk/pepper_canary/toolchain/win_x86_glibc/bin/x86_64-nacl-gdb.exe on
Windows, etc. Debuggers in \*_x86_glibc and \*_x86_newlib directories are
exactly the same. You can use either one on them. Moreover, unlike the rest of
SDK, you can copy debugger to a different folder.

Regarding the pepper and chrome versions, the latest debugger should work with
the previous versions of chrome/sel_ldr but new functionality may be missing.

## Debugging NaCl applications in Chrome

NaCl applications can be launched using two ways in Chrome. You can load
unpacked extension on [chrome://chrome/extensions/](javascript:void(0);) page
(switch on developers mode) and open it. Alternative way is to launch a web
server and either launch chrome with --enable-nacl switch or enable NaCl
applications outside of Chrome Web Store or
[chrome://flags/](javascript:void(0);) page (you need to relaunch chrome after
that). In order to switch on debug stub support, you need to pass
--enable-nacl-debug and --no-sandbox switches to chrome. Create a shell script
that launches chrome with these flags. You can also add
--user-data-dir=some-path to use a separate profile.

### Debugging newlib applications

In order to debug NaCl application, you need to launch Chrome and open NaCl
application there first. Then run x86_64-nacl-gdb. Enter following commands
there. If you have spaces in paths, use quotes and double all slashes (or use
backslashes). For pepper29+ debugger you should use quotes without doubling
slashes.

```none
(gdb) file c:\nacl_sdk\pepper_canary\examples\hello_world\newlib\Debug\hello_world_x86_64.nexe
(gdb) target remote :4014
```

Point file command to 64-bit nexe on 64-bit platforms and to 32-bit nexe on
32-bit platforms. Then you can use all normal gdb commands.

### Debugging glibc applications

Debugging glibc applications is harder. You need to use following commands.

```none
(gdb) nacl-manifest c:\nacl_sdk\pepper_canary\examples\hello_world\glibc\Debug\hello_world.nmf
(gdb) target remote :4014
```

If you launched NaCl as unpacked chrome extension or from local http server, you
already have the NaCl manifest file which you should use here. If you launched
NaCl from an external http server, you need to download \*.nmf, \*.nexe and
\*.so files of the NaCl application and place them in the same directory
structure that is used on the server (\*.nmf file uses relative paths to
reference main executable and libraries). Then point nacl-manifest command to
the downloaded \*.nmf file.

### Debugging with IRT symbols

You can additionally load IRT symbols. This helps to understand what application
is doing when it is stopped inside NaCl syscall.

```none
nacl-irt c:\Users\username\AppData\Local\Google\Chrome SxS\Application\23.0.x.x\nacl_irt_x86_64.nexe
```

### Automatic debugger launching

Chrome can be configured to launch debugger automatically. Use additional
command-line option --nacl-gdb="path-to-nacl-gdb" on Windows or
--nacl-gdb="command line to launch nacl-gdb in the new shell" on Linux. You can
use --nacl-gdb-script="path-to-gdb-script" to execute your gdb script at start
up. Chrome will autodetect its IRT location and execute nacl-irt command.
Additionally, if NaCl application is in a chrome extension, nacl-manifest
command is executed automatically. Example chrome command lines are shown below.

```none
chrome.exe --enable-nacl-debug --no-sandbox "--nacl-gdb=c:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\bin\x86_64-nacl-gdb" "--nacl-gdb-script=c:\Users\User Name\Documents\script.gdb"
```

```none
./chrome --enable-nacl-debug --no-sandbox "--nacl-gdb=xterm /home/user_name/nacl_sdk/pepper_canary/toolchain/linux_x86_glibc/bin/x86_64-nacl-gdb" --nacl-gdb-script=/home/user_name/script.gdb
```

## Debugging NaCl applications in sel_ldr

Debugging command line NaCl applications is enabled by passing -g switch to
sel_ldr. Newlib debugging is the same, glibc debugging requires creating an
artificial manifest. You need to reference runnable-ld.so, main executable and
all \*.so libraries using relative paths from manifest file. If you want to load
IRT symbols, use nacl-irt command with the same IRT that is passed to sel_ldr
using -B switch.
