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
page_name: debugging-nacl-apps-in-chrome-os
title: Debugging NaCl apps in Chrome OS
---

It is not possible to run debugger on Chrome OS without using dev mode. Luckily,
debugger uses network connection to communicate with the NaCl debug stub. This
makes remote debugging possible with debugger running on a remote machine and
NaCl application running on Chrome OS machine.

## Prepare Chrome OS for NaCl debugging

On chrome://flags page there are two flags related to NaCl debugging. We need to
enable "Native Client GDB-based debugging" flag and switch "Restrict Native
Client GDB-based debugging by pattern" to "Debug everything except secure shell"
or "Debug only if URL manifest ends with debug.nmf" value. Changes on
chrome://flags page require browser restart to take effect. Log out, close and
open the lid to restart the browser. If they are set properly, secure shell
extension should work, but NaCl application we are trying to debug should hang
waiting for debugger to connect.

## Tunneling debugger connection

Once you launch your NaCl application now, the NaCl debug stub opens 4014 port
on Chrome OS machine. This port is not accessible from outside, so we need to
tunnel it through ssh connection to remote machine. There are two ways to do
this.

The first way is to open new secure shell tab or window and add "-R
port-number:localhost:4014" to "SSH Arguments" field before connecting to remote
computer. Then you can use secure shell to launch NaCl debugger on that machine
where you should use "target remote :port-number" command to connect to debug
stub running on Chrome OS.

The second way is to use an existing secure shell tab or window. Press enter, ~,
shift+c. This combination opens ssh-prompt where you can enter "-R
port-number:localhost:4014" command. Pressing enter returns control to normal
shell where you can launch NaCl debugger and use "target remote :port-number"
command to connect to debug stub running on Chrome OS.

In both cases the port number can be any free port number on the remote machine
above 1024 including 4014.
