---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: chromium-os-libcros
title: Cros - How Chromium talks to Chromium OS
---

[TOC]

**DEPRECATED**

As of October 2011, the Cros library (libcros.so) is now in the process of
phasing out. The chromium browser talks to the background servers of Chromium OS
via IPC mechanism called D-Bus. The browser has ability to issue D-Bus method
calls and handle D-Bus signals hence no longer needs the Cros library layer.

## Abstract

Cros came about because we needed a way for Chromium UI code to talk to Chromium
OS code. For example, the Chromium networking UI needs to talk to Chromium OS
flimflam to get network status and to make connections to wifi/cellular
networks. Cros is set of APIs that is implemented on the Chromium OS side and
exposed to Chromium via a dynamically linked *libcros.so* file. We designed a
versioning system to make sure that Chromium OS will only boot up if we have the
correct version of *libcros.so*. If either the *libcros.so* version or the
Chromium version is too old, we catch that and disable login.

## Objective

The design behind Cros is based on these requirements:

*   Chromium OS code and Chromium code are in 2 different source trees.
            When we do release builds, we may use different snapshots of these 2
            trees depending on how stable they are. From past experience, we
            would release Chromium OS with a slightly older but stable version
            of Chromium. So Chromium should be able to talk to Chromium OS even
            if the *libcros.so* file in Chromium OS is slightly newer.
*   We should be able to modify Cros implementation without having to
            recompile Chromium.
*   We should be able to add new Cros API methods without having to
            recompile Chromium.
*   When new Cros API methods are needed by Chromium, we should be able
            to tell Chromium to bind to these new methods and use them in
            Chromium.
*   The LKGR build of Chromium OS should always work with the LKGR build
            of Chromium.
*   When Cros in Chromium OS is incompatible with that of Chromium, we
            need fail fast and let the user know why.

## Detailed Description

### Cros Files

<table>
<tr>
<td> <b>File</b></td>
<td><b>Description</b></td>
<td> <b>Used by Chromium OS</b></td>
<td> <b>Used by Chromium</b></td>
</tr>
<tr>
<td> *platform/cros/chromeos_cros_api.h*</td>
<td> Defines version number of Cros: `kCrosAPIMinVersion` and `kCrosAPIVersion`</td>
<td> Yes</td>
<td> Yes (only kCrosAPIVersion)</td>
</tr>
<tr>
<td> *platform/cros/load.cc*</td>
<td> This is the code that Chromium uses to bind methods to *libcros.so*</td>
<td> Yes</td>
</tr>
<tr>
<td> *platform/cros/version_check.cc*</td>
<td> Version checking code to make sure that version of *libcros.so* is compatible with Chromium</td>
<td> Yes</td>
</tr>
<tr>
<td> *platform/cros/chromeos_\*.h*</td>
<td> Defines the Cros API</td>
<td> Yes</td>
<td> Yes</td>
</tr>
<tr>
<td> *platform/cros/chromeos_\*.cc*</td>
<td> Cros Implementation</td>
<td> Yes</td>
</tr>
</table>

### Workflow

This is the workflow of how Cros is used in Chromium OS:

1.  Chromium OS is built and the Cros files listed above are built into
            the *libcros.so* file.
2.  Chromium build uses a DEPS file to fetch a specific version of Cros
            to build against.
3.  Chromium is built against this specific version of Cros API.
4.  When Chromium OS starts up Chromium, we make sure that the version
            of Cros that Chromium is built against is compatible with the
            version of *libcros.so*.
5.  If it's compatible, we binds the method calls from Chromium to the
            implementation in *libcros.so*.
6.  All Cros method calls from Chromium will now execute the
            corresponding method in Chromium OS's *libcros.so*.

### Cros DEPS

In order to have Chromium build against a specific version of Cros, we use this
DEPS file:
<http://src.chromium.org/viewvc/chrome/trunk/src/tools/cros.DEPS/DEPS?view=markup>

```none
vars = {
  "chromium_git": "http://src.chromium.org/git",
}
deps = {
  "src/third_party/cros":
    Var("chromium_git") + "/cros.git@e9730f8a
",
}
```

The 7 characters after the @ is the first 7 characters of the git commit hash
for the last Cros change that we want to build Chromium against.

For this example in this specific case, this is the change:
<http://git.chromium.org/gitweb/?p=cros.git;a=commit;h=e9730f8a8a689ae3e506d1e400c4799190d302d0>

This DEPS file will need to be updated when there are new Cros APIs that are
needed by Chromium.

### Version Checking

The version checking code is in *version_check.cc*, and it gets run by Chromium
when it is started by Chromium OS.

In chromeos-api.h, we define 2 version numbers: `kCrosAPIMinVersion` and
`kCrosAPIVersion`. We use these version numbers to decide if the version of Cros
in Chromium is compatible with the version of Cros in Chromium OS. Since both
Chromium and Chromium OS have their own copy of chromeos-api.h, we have 2 sets
of these 2 version numbers. Chromium OS's `kCrosAPIMinVersion` and
`kCrosAPIVersion` specify the min and max version of the Cros API that the
*libcros.so* supports. In other words, it is only compatible if kCrosAPIVersion
in Chromium is within the range \[`kCrosAPIMinVersion,` `kCrosAPIVersion`\] of
Chromium OS.

This means that the version of Cros in Chromium OS can be newer than the version
of Cros in Chromium. In other words, *libcros.so* can have new methods that are
not used by an older version of Cros in Chromium, but not the other way around.
If Chromium's kCrosAPIVersion is greater than Chromium OS's kCrosAPIVersion,
then Chromium is likely depending on a new Cros API that's not implemented in
*libcros.so*. In this case, the version check fails, and we prevent user from
logging in and display an error like: Incompatible libcros version. Client: 51
Min: 29 Max: 50

The kCrosAPIMinVersion version in *libcros.so* is used to specify the minimum
version of the Cros API that's supported. This number gets increased when we
remove methods from the Cros API. So if Chromium's kCrosAPIVersion is less than
Chromium OS's kCrosAPIMinVersion, then that means Chromium is likely using a
Cros API method that's no longer there. In this case, the version check fails,
and we prevent user from logging in and display an error like: `Incompatible
libcros version. Client: 28 Min: 29 Max: 50`

### Binding *libcros.so*

After version check passes, the code in *load.cc* runs to bind the Chromium
method calls to the implementation in *libcros.so*. If binding fails for any
reason, we prevent the user from logging and display an error like: `Couldn't
load: MethodName`

Common Developer Workflows

### Modifying method implementation

To change a Cros method implementation, you just need to change the code on the
Chromium OS side and you don't need to do anything on the Chromium side.
Chromium will bind with the unchanged Cros API on the new libcros.so and
exercise your new code without any problems.

### Adding methods to Cros API

This is best described by a table showing what you need to do on the Chromium OS
side and on Chromium side:

<table>
<tr>
<td> <b>Chromium OS</b></td>
<td> <b>Chromium</b></td>
</tr>
<tr>
<td> Check in new code:</td>
<td>*chromeos_\*.h* - Add new API to </td>
<td>**chromeos_\*.cc* - Add implementation to* </td>
<td> *- Add code to *load.cc* to declare and initialize new API*</td>
<td> *- Increment kCrosAPIVersion in *chromeos_cros_api.h**</td>
</tr>
<tr>
<td> Get your change's commit hash from:</td>
<td> <a href="http://git.chromium.org/gitweb/?p=chromiumos/platform/cros.git;a=commit;h=HEAD">http://git.chromium.org/gitweb/?p=chromiumos/platform/cros.git;a=commit;h=HEAD</a></td>
</tr>
<tr>
<td> Update DEPS:</td>
<td> - Put your commit hash in cros_deps/DEPS</td>
<td> - Run "gclient sync"</td>
<td> - Confirm third-party/cros includes your change</td>
<td> - Make sure build compiles</td>
<td> - Copy cros_DEPS/DEPS to tools/cros.DEPS/DEPS </td>
<td> - Check in tools/cros.DEPS/DEPS</td>
</tr>
<tr>
<td> Check in new code that utilizes the new API</td>
</tr>
</table>

### Deleting methods from Cros API

Deleting a method from the API is a complicated process that's prone to error.
So make sure you know what you are doing.

It is recommended that you wait at least a week after removing the usage and
binding of the method before you actually remove the implementation. This is
because deleting a method will make things backwards incompatible. So we need to
make sure that all possible versions of Chromium that was use with Chromium OS
will have this new Cros that does not bind with the deprecated method.

<table>
<tr>
<td> <b>Chromium OS</b></td>
<td> <b>Chromium</b></td>
</tr>
<tr>
<td> Remove calls to deprecated API</td>
</tr>
<tr>
<td> Deprecate method:</td>
<td> - Keep the method implementation</td>
<td> - Remove binding of method in *load.cc*</td>
<td> - Increment *kCrosAPIVersion* in *chromeos-api.h*</td>
</tr>
<tr>
<td> Get your change's commit hash from:</td>
<td> <a href="http://git.chromium.org/gitweb/?p=chromiumos/platform/cros.git;a=commit;h=HEAD">http://git.chromium.org/gitweb/?p=chromiumos/platform/cros.git;a=commit;h=HEAD</a> </td>
</tr>
<tr>
<td> Update DEPS:</td>
<td> - Put your commit hash in cros_deps/DEPS</td>
<td> - Run "gclient sync"</td>
<td> - Confirm third-party/cros includes your change</td>
<td> - Make sure build compiles</td>
<td> - Copy cros_DEPS/DEPS to tools/cros.DEPS/DEPS</td>
<td> - Check in tools/cros.DEPS/DEPS</td>
</tr>
<tr>
<td> Delete method:</td>
<td> - Delete implementation</td>
<td> - Set kCrosAPIMinVersion to *kCrosAPIVersion*</td>
<td> - Increment *kCrosAPIVersion* in *chromeos-api.h*</td>
</tr>
<tr>
<td> Get your change's commit hash from:</td>
<td> <a href="http://git.chromium.org/gitweb/?p=chromiumos/platform/cros.git;a=commit;h=HEAD">http://git.chromium.org/gitweb/?p=chromiumos/platform/cros.git;a=commit;h=HEAD</a></td>
</tr>
<tr>
<td> Update DEPS:</td>
<td> - Put your commit hash in cros_deps/DEPS</td>
<td> - Run "gclient sync"</td>
<td> - Confirm third-party/cros includes your change</td>
<td> - Make sure build compiles</td>
<td> - Copy cros_DEPS/DEPS to tools/cros.DEPS/DEPS</td>
<td> - Check in tools/cros.DEPS/DEPS</td>
</tr>
</table>

### Modifying method signatures

It is highly recommend against modifying method signatures. This is because if
you modify a method signature, you would need to check in a backwards
incompatible change. In other words, in order for Chromium to talk to Chromium
OS, it would need this exact version of Cros. So you would have to check in code
to both Chromium OS and Chromium at the same time. And if anyone tries to use
Chromium OS with a slightly older version of Chromium, they will be blocked from
logging in.

Instead of modifying a method signature, it is better to just add a new method
in Cros, change it so that Chromium calls this new method. And after a while,
delete the old deprecated method.

## FAQ

### How do I locally test a Cros and corresponding Chromium change before I check anything in?

After you have "git commit" the Cros code, you can use "git pull" to pull that
code to Chromium:

```none
cd ~/chrome/src/third_party/cros
git pull ~/chromeos/src/platform/cros
```

And if you want to test Chromium with the new *libcros.so*, you need to copy it
to the chromeos directory:

<pre><code>
emerge-x86-generic libcros <i>(in Chromium OS chroot)</i>
mkdir ~/chrome/src/out/Debug/chromeos
cp ~/chromeos/chroot/build/x86-generic/opt/google/chrome/chromeos/* ~/chrome/src/out/Debug/chromeos
</code></pre>

### Why does Cros fail to load with message "Incompatible libcros version. Client:x Min:y Max: z" where x &gt; z?

This happens when the version of Cros in Chromium is newer than the version of
Cros in Chromium OS. To fix this, make sure you fetch the latest code from
*platform/cros* and then emerge libcros.

### Why does Cros fail to load with message "Incompatible libcros version. Client:x Min:y Max: z" where x &lt; y?

This happens when the version of Cros in Chromium is too old for the version of
Cros in Chromium OS. To fix this, make sure you build the latest Chromium.
