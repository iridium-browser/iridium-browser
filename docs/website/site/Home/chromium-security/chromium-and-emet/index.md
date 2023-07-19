---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/chromium-security
  - Chromium Security
page_name: chromium-and-emet
title: Chromium and EMET
---

[EMET](http://technet.microsoft.com/en-us/security/jj653751) is a security tool
provided by Microsoft to improve exploit-resistance of software running on
Windows. Enterprises and users can deploy EMET on systems and configure which
applications are protected by it. [EMET then injects code into the selected
processes that adds protection against common exploit
techniques](http://support.microsoft.com/kb/2909257), typically causing the
process to terminate if behavior is detected that appears to indicate an attack.

**Google Chrome 35+** is built for Windows using VS 2013 and is not compatible
with EMET's **ROP chain detection**, and **Google Chrome 53+** built with PGO
optimizations is not compatible with EMET's **EAF+** mitigation.

# Specific Compatibility issues

Some applications cannot run with EMET protections because they have internal
behavior that, to EMET, resembles an exploit in progress. This does not mean the
applications are dangerous or are doing anything harmful to the user. In this
case EMET causes the application to unexpectedly terminate even though no actual
attack conditions are present.

Two specific incompatibilities have been found with EMET:

## ROP chain detection and prevention (Chrome 35+)

After [Chromium revision
254340](http://src.chromium.org/viewvc/chrome?view=revision&revision=254340),
Visual Studio 2013 is the only supported build chain for Chromium on Windows.
Unfortunately, we have observed compatibility problems with Microsoft’s Enhanced
Mitigation Experience Toolkit (EMET) and Chromium compiled on Windows using
Visual Studio 2013.

One specific issue we have encountered with Chromium compiled using VS 2013
relates to [tail-call optimizations](http://en.wikipedia.org/wiki/Tail_call) in
wrapper functions for Windows APIs. By using jmp to enter the Windows API call
from the wrapper, the Visual Studio compiler avoids an additional call/ret pair,
and the API would return directly into the wrapper function’s caller rather than
the wrapper function itself. However, EMET protects various ‘critical’ Windows
APIs against an exploit technique known as Return-Oriented Programming (ROP),
and one of these protections is incompatible with tail-call optimization. EMET’s
code checks that the return address from the API call is immediately preceded by
a call to that API, since in ROP exploits this will typically not be the case
but in normal function calls it will. The tail-call optimization violates EMET’s
assumption and causes a false positive result for exploit detection.

## EAF (Export Address Table Filtering) and EAF+ (Chrome 53+ 64-bit)

The [Chromium
sandbox](/developers/design-documents/sandbox#TOC-The-broker-process) uses an
interception layer to allow the sandboxed process to still access a limited set
of Windows APIs. These interceptions are achieved by the interception manager
whose job it is to patch the windows API calls that should be forwarded via IPC
to the browser process. Chrome 53 added Profile Guided Optimization
([PGO](https://blogs.msdn.microsoft.com/vcblog/2013/04/04/build-faster-and-high-performing-native-applications-using-pgo/))
to our build process and this seems to have an incompatibility with EMET's EAF+.

EMET subsequently incorrectly falsely detects our optimizations as an exploit
attempting to patch system DLLs and as a result will not allow any sandboxed
processes to start. The bug tracking this is [643775](https://crbug.com/643775).
A current workaround is to remove chrome_child.dll from the EAF+ DLL allowlist.

# Fix and Workarounds

We are in contact with Microsoft to investigate and address these problems.
Microsoft currently is already recommending that the EMET caller mitigation not
be enabled for Chrome. Google recommend that EAF+ protection be disabled for
Chrome, and we are working with Microsoft to update their guidance.

## Users

In the meantime, users experiencing this problem with Chrome or Chromium-based
browsers can resolve the issue by either:

    Removing the browser from the list of applications monitored by EMET, and/or

    Disabling the caller mitigation setting and EAF+ specifically for
    chrome_child.dll.

The Chrome security team does not generally recommend the use of EMET with
Chromium because it has negative performance impact and adds no security benefit
in most situations. The most effective anti-exploit techniques that EMET
provides are already built into Chromium or superseded by stronger mitigations.

<table>
<tr>

<td>EMET Protection</td>

<td>Benefit to Chromium</td>

</tr>
<tr>

<td>Forcing Data Execution Prevention</td>

<td>None, Chromium builds with this already enabled.</td>

</tr>
<tr>

<td>Forcing Address Space Layout Randomization</td>

<td>None, Chromium builds with this already enabled.</td>

</tr>
<tr>

<td>Structured Exception Handling Overwrite Protection</td>

<td>None, Chromium enables OS SEHOP on all versions of Windows where supported. EMET can add SEHOP on Windows Vista, however as of Chrome 50 Windows Vista is no longer supported.</td>

</tr>
<tr>

<td>Heap Spray Page Reservation</td>

<td>This can protect Chromium from some exploits, but can also be trivially bypassed by exploit writers who are aware of EMET protections.</td>

</tr>
<tr>

<td>Export Address Table (EAF and EAF+) access filtering</td>

<td>This can protect Chromium from some exploits, but can also be bypassed by exploit writers who are aware of EMET protections.</td>

</tr>
<tr>

<td>ROP chain detection and prevention</td>

<td>This can protect Chromium from some exploits, but can also be bypassed by exploit writers who are aware of EMET protections.</td>

</tr>
</table>
