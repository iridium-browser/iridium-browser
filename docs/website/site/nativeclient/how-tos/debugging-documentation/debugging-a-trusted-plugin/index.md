---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/how-tos
  - '2: How Tos'
- - /nativeclient/how-tos/debugging-documentation
  - Debugging Documentation
page_name: debugging-a-trusted-plugin
title: Debugging a Trusted Plugin
---

In some cases it can be useful to develop/port your Native Client module using a
two-step process, first as a non-portable trusted Pepper plugin for Windows,
Mac, or Linux, then porting to Native Client. In particular this approach allows
you to use the standard debuggers and tools from your preferred desktop
operating system during trusted plugin development.

Developers should be mindful of the following potential issues when planning
this course of development:

*   Some libraries commonly used with Native Client will not build
            easily on all operating systems.
*   Threading models differ between trusted Pepper and untrusted Pepper
            implementations.
*   Extra effort may be required to get source to compile with multiple
            different compilers, for example GCC vs. MS Visual Studio.
*   Certain operations such as platform-specific library calls and
            system calls may succeed during trusted development but fail in
            Native Client.

With this in mind, these sub-pages provide some tips on working with trusted
Pepper plugins on Windows, Mac and Linux.
