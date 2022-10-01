---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/how-tos
  - '2: How Tos'
page_name: debugging-documentation
title: Debugging Documentation
---

**The Basics**

The NaCl team is hard at work developing an integrated debugging solution(1).
But before this feature is available, developers must resort to alternative
debugging techniques. These techniques are described here (with reservation!)
First, Native Client applications can be debugged the old school way: with
printf() and logging. Second, to a limited degree, Native Client can be debugged
with the host's GDB, but using normal GDB will have problems resolving addresses
-- some manual tweaking needs to be done to map NaCl addresses into the global
address space GDB is using. Third, there are some experimental patches to
customize GDB which enable various degrees of debugging untrusted code.

First, we recommend debugging as much as possible compiled as a normal trusted
plugin/executable outside of Native Client, which of course will allow full use
of debuggers such as GDB, MSVC, and WinDebug. Native Client most resembles
Linux, so using GCC and POSIX will be the closest match to the NaCl runtime
environment.

For untrusted Native Client applications, we again recommend debugging on a
Linux based host, and launching a debug build of Chrome from a terminal window.
A Native Client application can use printf() which will output messages to the
terminal window from which Chrome was launched.

Native Client's Service Runtime can output debug messages to the terminal
window. The level of detail in these logging messages can be controlled via the
environment variable NACLVERBOSITY

For example:

developer@host:/home/developer/chrome/src/out/Debug$ export NACLVERBOSITY=3

developer@host:/home/developer/chrome/src/out/Debug$ ./chrome –enable-nacl

Additional messages from the plug-in can be separately enabled via
NACL_SRPC_DEBUG environment variable.

To capture log output to file “outfile” use:

developer@host:/home/developer/chrome/src/out/Debug$ ./chrome –enable-nacl
&&gt;outfile

To get logs in Windows, you must instead run chrome with the environment
variable NACLLOG=&lt;absolute path to log file / path relative to
chrome.exe&gt;. This requires running Chrome with the --no-sandbox flag.

**Getting the output from the Native Client Program on Windows:**

On Linux and OSX, the Native Cilent application inherits standard output and
standard error from Chrome, so though cumbersome, printf-style debugging is
feasible. On Windows, like any non-console application, standard output and
standard error outputs are thrown into the bit bucket. To get the Native Client
application's output on Windows, set the NACL_EXE_STDOUT and NACL_EXE_STDERR
environment variables to be the absolute path to files, and then start Chrome
with the --no-sandbox flag. Output written to standard output and standard error
will then be written to the specified files.

**Getting the Process ID (PID) of the Native Client Program from Chrome:**

Chrome -&gt; Developer-&gt;Task Manager, right-click process table header,
toggle on "Process ID". The PID can be used to attach GDB to running Native
Client processes. Or click on a new tab and type "about:memory" into the URL
address bar.
