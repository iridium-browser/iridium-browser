---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/test-code-labs
  - Test Code Labs
- - /chromium-os/testing/test-code-labs/autotest-client-tests
  - Autotest Client Tests
page_name: basic-ebuild-troubleshooting
title: Basic Ebuild Troubleshooting
---

### This is a brief explanation of how one might trouble shoot local emerge/ebuild failures. It is in no way comprehensive, and is only meant as an introduction to the topic:

#### Emerge errors during cros build-image

When following the [Chromium OS Developer Guide](/chromium-os/developer-guide)
emerge errors can be encountered for missing ebuild. A error that can be
encountered when running `cros build-image` is:

> emerge: there are no ebuilds to satisfy XYZ

Where XYZ is a library or other missing package.

This can be resolved by running:

> emerge-$BOARD XYZ

Then recalling `cros build-image`:

```bash
cros build-image --board=${BOARD} --no-enable-rootfs-verification dev
```

<table>
<tr>

<td>References:</td>

<td> \[1\] <a
href="http://www.chromium.org/chromium-os/testing/autotest-user-doc">Autotest
ebuild documentation</a></td>

<td> \[2\]<a
href="http://www.chromium.org/chromium-os/developer-guide/chromite-shell-quick-start">
Chromite documentation</a></td>

<td>From within the chroot, try running the a new test through run_remote_tests,
as described in the codelab on autotest client tests, without actually adding it
to an ebuild. This might give you an error about not being able to find
kernel_HdParmBasic’s control file.</td>

<td>To resolve this, from within chroot:</td>

<td>1. Find the package with autotest tests:</td>

<td>emerge --search autotest | grep tests</td>

<td>2. Dry emerge it and look for your test in the output:</td>

<td>emerge-lumpy -pv chromeos-base/autotest-tests</td>

<td> ==Two things to note:==</td>

<td> a. The first line will say something like:
“chromeos-base/autotest-tests-0.0.1-r3531“</td>

<td> This means it’s emerging the r3531 ebuild. </td>

<td> Since we would like it to use our local bits, we need to cros_workon the
package.</td>

<td> b. You will not find kernel_HdParmBasic in the output.</td>

<td>3. cros_workon the appropriate package:</td>

<td> cros_workon --board=&lt;board name&gt; start &lt;package name&gt;</td>

<td> eg: cros_workon --board=lumpy start autotest-tests</td>

<td> emerge the package: emerge-&lt;board name&gt; &lt;package name&gt;</td>

<td> eg: emerge-lumpy -pv chromeos-base/autotest-tests</td>

<td> ==Two things to note:==</td>

<td> a. The first line will say something like:</td>

<td>“chromeos-base/autotest-tests-9999”</td>

<td> This is good, it means it’s pulling local bits.</td>

<td> b. You still will not find kernel_HdParmBasic in the output.</td>

<td> This is because your new test hasn’t been added to the ebuild yet.</td>

<td>4. Edit the ebuild to include your test:</td>

<td> Find the ebuild file: find -iname “autotest-tests-9999.ebuild” from
src/third_party/. </td>

<td> a. It will most probably point you to:</td>

<td>
third_party/chromiumos-overlay/chromeos-base/autotest-tests/autotest-tests-9999.ebuild.
</td>

<td> b. open this file and add “+test_kernel_HdParmBasic” in the IUSE_TESTS
section.</td>

<td>5. Emerge the new folder:</td>

<td> a. Make sure you’re cros_working on the right packages:</td>

<td> cros_workon --board=lumpy list</td>

<td> should show autotest_tests</td>

<td> b. Make sure the emerge will touch kernel_HdParmBasic:</td>

<td> emerge-lumpy -pv chromeos-base/autotest-tests | grep
kernel_HdParmBasic</td>

<td> c. Actual emerge:</td>

<td> emerge-lumpy chromeos-base/autotest-tests</td>

<td>6. Check the staging area for your new test:</td>

<td> eg: ls
/build/lumpy/usr/local/autotest/client/site_tests/kernel_HdParmBasic</td>

<td>7. Re-run the run_remote command and look for the results directory:</td>

<td> you should see a line like:</td>

<td> &gt;&gt;&gt; Details stored under /tmp/run_remote_tests.KTQ4</td>
<td> Alternatively, you can specify your own results directory using
‘--results_dir_root’. </td>

<td>Some common ebuilds to cros_workon and emerge: </td>

*   <td>autotest-all (Meta ebuild for all packages providing tests),
            </td>
*   <td>autotest-factory (Autotest Factory tests), </td>
*   <td>autotest-chrome (Autotest tests that require chrome_test or
            pyauto deps), </td>
*   <td>autotest-tests (Pure Autotest tests), </td>
*   <td>autotest (Autotest scripts and tools).</td>

<td>If you have come this far, you may also be interested in reading the
autotest client tests <a
href="/chromium-os/testing/test-code-labs/autotest-client-tests">codelab</a>.</td>

<td><b>Access Violation Errors when Emerging</b></td>

<td>When you're attempting to include a file from another project, for example
#include &lt;shill/net/rtnl_handler.h&gt; Your build may fail with access
violations, e.g. ... arc-networkd-9999: \[0/3\] CXX
obj/arc/network/arc-networkd.manager.o \* ACCESS DENIED: open_rd:
/mnt/host/source/src/platform2/shill/net/rtnl_listener.h arc-networkd-9999: \*
ACCESS DENIED: open_rd: /mnt/host/source/src/platform2/shill/net/shill_export.h
arc-networkd-9999: \* ACCESS DENIED: open_rd:
/mnt/host/source/src/platform2/shill/net/rtnl_listener.h arc-networkd-9999: \*
ACCESS DENIED: open_rd: /mnt/host/source/src/platform2/shill/net/shill_export.h
arc-networkd-9999: arc-networkd-9999: \[1/3\] CXX
obj/arc/network/arc-networkd.manager.o arc-networkd-9999: FAILED:
obj/arc/network/arc-networkd.manager.o The proper way to handle this is to make
sure that the ebuild for the package you are including a file from (in this
case, shill), is installing the necessary headers to /usr/include. This should
look like: src_install() {    # ...   insinto /usr/include/\[package_name\]
doins header_one.h      doins header_two.h      # ... }</td>

<td>Notably, it is <b>incorrect</b> to add the package name you are depending on
to your package's ebuild under CROS_WORKON_SUBTREE.</td>

</tr>
</table>
