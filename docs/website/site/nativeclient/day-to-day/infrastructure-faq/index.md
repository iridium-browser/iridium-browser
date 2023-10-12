---
breadcrumbs:
- - /nativeclient
  - Native Client
- - /nativeclient/day-to-day
  - '6: Day-to-Day Engineering'
page_name: infrastructure-faq
title: Infrastructure FAQ
---

[TOC]

### **Why do some DEPS / .gclient URLs use http://, https://, svn:// ?**

We would like .gclient files to function correctly for users both inside and
outside Google.

In order to facilitate this, whenever a DEPS file lists repositories, read-only
publicly accessible URLs are preferred.

These typically being with http://

For example:

http://src.chromium.org/chrome/trunk/base - *read-only URL for base from
chromium.*

http://nativeclient.googlecode.com/svn/trunk/src/native_client - read-only nacl
main repo

Sometimes svn:// and https:// URLs are used.

There are two main reasons:

*   To specify checking out a writable svn client ==inside a .gclient
            file==
*   To pull in an internal only writable repository inside a DEPS file
            (since it's internal you can assume it can always be checked out
            writable).

Google Code (nativeclient, naclport, etc) only supports http:// and https://.

Chromium's primary svn mirror (src.chromium.org) does http:// only.

Chromium's svn servers (svn.chromium.org, chrome-svn) support only svn://.

### **How do I make changes outside src/native_client?**

If the repository is internal only, make changes as usual, use gcl in each
repository.

You can use ==svn info== to get information about the repository you're
currently in.

For public repositories, you may need to make sure you've got a writable client.

(gclient can use svn switch to switch from read-only to writable in most cases
preserving patches).

To make a particular repository checkout writable, add a section like the
following to you .gclient file:

"custom_deps" : {

"native_client/third_party/foo":
"https://nativeclient.googlecode.com/svn/trunk/deps/third_party/foo",

},

This overrides the URL specified for foo in native_client/DEPS

### **How can I make gclient not check everything out?**

Modify your .gclient file to add a custom_deps section, listing None for the
paths to exclude:

"custom_deps" : {

"src/data/esctf": None,

"src/data/mozilla_js_tests": None,

"src/data/page_cycler": None,

"src/data/tab_switching": None,

"src/data/saved_caches": None,

"src/data/selenium_core": None,

"src/tools/grit/grit/test/data": None,

}

**Note:** the above instruction is not useful because most of the checkout time
is consumed by downloading all the necessary toolchains and unpacking them. To
manually disable one of the toolchains from downloading, first download all of
them in the initial checkout.

```none
shell$ ls native_client/toolchain
linux_arm-trusted
linux_x86
linux_x86_newlib
pnacl_linux_x86_64
pnacl_linux_x86_64_glibc
pnacl_linux_x86_64_newlib
pnacl_translator
```

To disable updating one of them, for example, pnacl_linux_x86_64, change the
test in file:

```none
shell$ echo manual > native_client/toolchain/pnacl_linux_x86_64/SOURCE_URL
```

All subsequent gclient syncs would (hopefully) assume that this toolchain is
up-to-date.

### **I keep getting patch failure on the trybots, what do I do?**

Patches typically fail on the trybots because of version mismatch between what
is in the patch and the revision that the trybot wants to patch against.

If your client is really old, do a gclient update before submitting try jobs to
bring it in sync.

The trybot uses http://nativeclient-status.appspot.com/lkgr (last known good
revision), to decide what to patch against.

This is intended to allow you to submit try jobs, even when the tree is red.

If you ever need to apply a patch which does not work against the default on the
try bot, you can add various flags to gcl try:

*   -r REV# -- patch against a specific revision
*   -r HEAD -- patch against tip of trunk
*   -c -- clobber the build before applying the patch (of limited use
            for nacl, but useful for chromium)

### **I get strange error messages when compiling on Windows, what do I do?**

Be sure to install all the service packs listed for chromium:

http://www.chromium.org/developers/how-tos/build-instructions-windows

### **The toolchain in native_client appears to be out of date. How do I safely update it?**

If you make changes to the untrusted or shared code, you need to update the
toolchain revision(s) after your change is committed and the toolchain build is
green. (If the toolchain build is red, you will need to check in a fix right
away or simply rollback your change).
For example, we updated the ppapi revision in DEPS, which required updates to
the code under src/shared. This needs a toolchain revision update. So we have:
PPAPI DEPS revision:
<http://src.chromium.org/viewvc/native_client?view=rev&revision=3418>
Follow-up fix revision:
<http://src.chromium.org/viewvc/native_client?view=rev&revision=3421>
There are two families of toolchains that generally need to be updated in
nactive_client/DEPS: `arm_toolchain_version` and `x86_toolchain_version`. It is
recommended to update them both to the same revision at a time.
To perform this straightforward toolchain revision update:

*   go to <http://build.chromium.org/p/client.nacl.toolchain/console>
            (linked under "Toolchain" header on
            <http://build.chromium.org/p/client.nacl/console>):
*   find the latest green row and let `$REV` to be the corresponding SVN
            revision to that row
*   check for toolchain tarballs availability:

    ```none
    shell> cd native_client/
    shell> python build/find_toolchain_revisions.py -s $REV
    ```

*   if all tarballs are available, you will get the output:

    ```none
    Searching for available toolchains...
    checking r$REV: x86: OK, arm: OK
    ```

*   The script will go and check the older revisions, interrupt it as
            soon as you do not need more results
*   This revision `$REV` can now be used in `native_client/DEPS` for
            both toolchain versions.
*   If some tarballs are not available, the output would be something
            like:

    ```none
    checking r$REV: x86: missing win_x86, arm: missing pnacl_linux_x86_64_newlib
    ```

*   When found a revision with all toolchains available, do not forget
            to check that all builds in the "Toolchain" builder console are
            green for that row. Sometimes they can be red for reasons irrelevant
            to toolchain correctness, you may try those if you are brave.

To perform toolchain revision update for only one family (i.e.
`arm_toolchain_version` or `x86_toolchain_version`) you will need only the 'OK'
for that family in toolchain tarballs availability and buildber green status
only of a subset of the rows (subject to frequent change):

*   arm_toolchain_version
    *   linux-pnacl-x86_64
    *   linux-armtools-x86_32
    *   linux-pnacl-x86_32
    *   mac-pnacl-x86_32
*   x86_toolchain_version
    *   win7-toolchain_x86
    *   mac-toolchain_x86
    *   lucid64-toolchain_x86
    *   win7-glibc
    *   mac-glibc
    *   lucid64-glibc

TODO: describe the update procedure for NaCl SDK (requires updating toolchain
DEPS in the Chrome tree, pushing the new manifest).
