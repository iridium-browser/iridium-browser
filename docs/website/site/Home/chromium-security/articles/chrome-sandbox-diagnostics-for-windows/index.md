---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
- - /Home/chromium-security/articles
  - Articles
page_name: chrome-sandbox-diagnostics-for-windows
title: chrome://sandbox Diagnostics for Windows
---

## You are adequately sandboxed

February 2020

The latest Chrome stable release for Windows, [Chrome
80](https://chromereleases.googleblog.com/2020/02/stable-channel-update-for-desktop.html)
(early February 2020) provides detailed debugging information on how Chromium’s
processes are sandboxed. You can access this by typing chrome://sandbox into the
url-bar (Omnibox). The output is mainly of interest to Chromium developers but,
along with chrome://conflicts, could be helpful when troubleshooting
incompatibility between misbehaving software and the sandbox. This post will
outline how the sandbox works on Windows and the information that is displayed.

Chrome, and all web browsers, have a difficult job. Users visit sites that store
sensitive information, serve complex data formats and run JavaScript code. Users
visit more than one site in each browsing session, and each site might include
components from a range of third parties. Chromium has to protect itself from
malicious sites, and protect each site’s data from being accessed by other
sites. Chromium developers mainly achieve this by writing secure code, testing,
and fuzzing. However, Chrome has a JavaScript engine and parsers for complex
data formats written in C/C++ for performance, which may have bugs that a
malicious attacker can exploit to run arbitrary code. This, in turn, could allow
an attacker to do anything Chrome can, such as accessing a user’s files, or
calling Windows APIs, which may themselves have exploitable bugs. To provide a
secondary layer of defense Chrome restricts what it can do by containing sites
and services in sandboxed processes.

Chrome uses a [multi-process
architecture](https://www.google.com/googlebooks/chrome/small_04.html) for
security and stability. This way, a bug or a crash in the process running one
site will not bring down other sites, and data in one process can be made
inaccessible to other processes. Chrome calls the processes running sites
renderers and its main coordinating process the browser. The browser is
supported by network, audio and gpu processes, and uses short-lived data-decoder
and utility processes to parse and verify complex data formats. The goal of the
[sandbox](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/design/sandbox.md)
is to allow these processes the level of access to the operating system they
need to do their jobs, and nothing more. Below, we’ll use the output of
chrome://sandbox to show some of these restrictions.

## chrome://sandbox

First you’ll see a table of active processes (excluding the browser process, as
that is not sandboxed) and some summary information:-

<table>
<tr>
<td> Process </td>
<td> Type </td>
<td> Name</td>
<td> Sandbox</td>
<td> Integrity</td>
<td> Mitigations</td>
</tr>
<tr>
<td> 42152</td>
<td> GPU </td>
<td> GPU </td>
<td> Limited </td>
<td> S-1-16-4096 Low</td>
<td> 01111001010110000000000000010000</td>
</tr>
<tr>
<td> 28388</td>
<td> Utility</td>
<td> Network Service </td>
<td> Not Sandboxed </td>
</tr>
<tr>
<td> 43376</td>
<td> Utility</td>
<td> V8 Proxy Resolver </td>
<td> Lockdown </td>
<td> S-1-16-0 Untrusted</td>
<td> 01111001010110000000000000010000</td>
</tr>
<tr>
<td> 4880</td>
<td> Utility</td>
<td> Audio Service</td>
<td> Restricted Non Admin</td>
<td> S-1-16-4096 Low</td>
<td> 01111011010110000000000000010000</td>
</tr>
<tr>
<td> 13892</td>
<td> Native Client module</td>
<td> https://earth.google.com/static/9.3.100.2/earthnacl_pexe.nmf</td>
<td> Lockdown</td>
<td> S-1-16-0 Untrusted</td>
<td> 01111001010110000000000000010000</td>
</tr>
<tr>
<td> 25980</td>
<td> Renderer	</td>
<td> Lockdown</td>
<td> S-1-16-0 Untrusted</td>
<td> 01111001110110000000000000010000</td>
</tr>
<tr>
<td> 35952</td>
<td> Renderer	</td>
<td> Lockdown</td>
<td> S-1-16-0 Untrusted</td>
<td> 01111001110110000000000000010000</td>
</tr>
</table>

Followed by a raw dump of every process’s detailed sandbox configuration in
JSON, which we’ll look at later.

Depending on how many tabs you have open, and which plugins or extensions you’re
running, you’ll see something similar. First you’ll see the various utility and
plugin host processes, followed by a long list of renderer processes which host
sites. Utility processes have different sandbox configurations, while every
renderer process has the same sandbox configuration. For information on which
renderer process hosts which site, see the Task Manager (Shift+Esc).

[Sandbox
Type](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/design/sandbox.md)
ranges from ‘No Sandbox’ (the most permissive), via ‘Limited’ and ‘Restricted’
to ‘Lockdown’ (the least permissive). The main browser process runs without a
sandbox, and some supporting processes might do so, especially if the services
they contain have only recently been moved out of the browser process. Our goal
is to gradually tighten these sandboxes. Renderers are locked down so that they
have only very limited access to the operating system, file system and other
objects.

Sandboxing is mainly achieved by dropping privileges and applying operating
system mitigations. Privileges are controlled by [restricting the
token](https://docs.microsoft.com/en-us/windows/win32/secauthz/restricted-tokens)
and [Integrity
Level](https://docs.microsoft.com/en-us/windows/win32/secauthz/mandatory-integrity-control)
of a sandboxed process. Most things a process can interact with on Windows, like
desktops, files, and pipes are [securable
objects](https://docs.microsoft.com/en-us/windows/win32/secauthz/securable-objects).
Windows checks a process’s token against access control lists before allowing a
process to access or modify an object. Chrome’s browser process starts running
with the same token as the user that launched it, which, for instance, can
access all the user’s files. Renderers shed every privilege possible and limit
their token to only the S-1-16-0 Untrusted integrity level significantly
reducing the number of objects they can interact with. Services such as the GPU
process, which interacts with the Windows graphics system, may need more access
so run at higher integrity levels.

The final column of the table shows the operating system mitigations which are
applied to the process. This will vary based on the version and architecture of
Windows that you are using. On the latest iteration of Windows 10 renderer
processes will have:-

01111001110110000000000000010000

This is a hex representation of the [Windows process mitigation
options](https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocessmitigationpolicy).
You can decode this [using the attached
decoder](https://docs.google.com/a/chromium.org/viewer?a=v&pid=sites&srcid=Y2hyb21pdW0ub3JnfGRldnxneDo3MDg0MDMzODNjODgzMDMy),
which will show that the following process mitigations are enabled:
HEAP_TERMINATE, BOTTOM_UP_ASLR, STRICT_HANDLE_CHECKS,
WIN32K_SYSTEM_CALL_DISABLE, EXTENSION_POINT_DISABLE,
BLOCK_NON_MICROSOFT_BINARIES, FONT_DISABLE, IMAGE_LOAD_NO_REMOTE,
IMAGE_LOAD_NO_LOW_LABEL and RESTRICT_INDIRECT_BRANCH_PREDICTION.

Other processes are similar (or not sandboxed). The audio process also has
PROHIBIT_DYNAMIC_CODE enabled but, along with the data decoder and other utility
processes, is missing WIN32K_SYSTEM_CALL_DISABLE. Dynamic code is required in
the renderer as this is used by v8 to compile JavaScript to native code. If you
have a PDF open you will see that its process does not require dynamic code.
While PDFs can include JavaScript it is run in interpreted mode.

On Windows 7 in 32-bit mode the story is quite different. The mitigations are
0000000000000005 which corresponds to only DEP_ENABLE and SEHOP_ENABLE. These
are not even indicated on 64-bit Windows 10 as they are always enabled there.
This reduced set of mitigations means that a renderer compromise on Windows 7 is
much more serious than on Windows 10. More of the operating system can be
reached from a compromised process, allowing a malicious site more avenues to
access data outside of their own site’s process. This is why we encourage our
users to run the latest version of Windows so that they can benefit from the
latest security technologies.

Below the summary table you’ll see JSON showing more detailed output for the
same processes:

"NtCreateFile": \[

"!(p\[1\] & 1) && !(prefix(p\[0\], '\\\\??\\\\')) -&gt; askBroker",

"!(p\[1\] & 1) && scan(p\[0\], '~') -&gt; askBroker",

"!(p\[2\] & 5fedff56) && p\[3\] == 1 && exact_i(p\[0\],
'\\\\??\\\\C:\\\\Windows\\\\Fonts') -&gt; askBroker",

"!(p\[2\] & 5fedff56) && p\[3\] == 1 && prefix_i(p\[0\],
'\\\\??\\\\C:\\\\Windows\\\\Fonts\\\\') -&gt; askBroker",

"prefix_i(p\[0\], '\\\\??\\\\pipe\\\\chrome.') -&gt; askBroker"

\],

The desiredMitigations field is defined by Chromium and shows which mitigations
and options we would set if they were supported by the operating system. These
map closely to the platform mitigations and can be decoded using the same tool.
There are also a set of policy rules which are used when system calls are
intercepted in a child process. The WIN32K_SYSTEM_CALL_DISABLE mitigation, and
the reduced token of the child process, prevents renderers from calling various
functions or from opening files or pipes they may need to do their job. The
sandbox intercepts these functions as the child process starts and uses an IPC
mechanism to forward calls that match a policy rule to the browser. These rules
are evaluated again in the browser and, if allowed, the call is made and the
results passed back into the renderer for it to use. The rule above allows
renderer processes to read system fonts and connect to pipes matching
\\??\\pipe\\chrome.\*. Any other attempts to open files are blocked. The first
two rules are special and apply only in the renderer but are skipped in the
browser process. This is because short names must be normalised before being
tested, and this must occur in the browser process.

New sandbox rules and policies are introduced to the Canary, Beta and Dev
channels of Chrome before being enabled on the Stable channel so the output of
chrome://sandbox might be different depending on which channel you are running.
Rarely, other software may be incompatible with the sandbox, and you may see
advice to run with the ‘--no-sandbox’ flag or to use Windows 7 compatibility
mode. We do not encourage this as it will significantly reduce your security,
and may prevent Chromium from working properly. Instead we encourage you to
search the [Chrome community support
forum](https://support.google.com/chrome/community?hl=en) and to report problems
to your suppliers. We encourage software vendors and enterprises to test their
applications and sites with Chrome’s Beta and Dev channels to uncover
incompatibilities before they might affect their products or business. The
output of chrome://sandbox, in combination with chrome://conflicts, might help
when diagnosing incompatibilities or failures and might be useful when added to
any bug reports. The output of these special pages contains system information
which may include sensitive data such as usernames so we advise you to be
careful if sharing it.
