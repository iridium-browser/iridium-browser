---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: autoserv-packaging
title: Autoserv Packaging
---

Autoserv at a glance

[Autoserv](https://github.com/autotest/autotest/wiki/Autoserv) -- as described
on its documentation page -- is a framework for automating machine control. It
can do things like power cycle, install kernels, run arbitrary commands,
transfer files, and run autotest tests. While a “machine” can be virtual, local,
or remote, autoserv is largely used on chromeOS for remotely controlling
chromebooks for automated testing purposes.

Autoserv includes specific support for Autotest. It’s able to install Autotest
on host devices, run control files on them, and then fetch results back to the
server. To install the appropriate files onto a host device, autoserv’s
packaging system is used. Since autotest is it's own package it can change at a
rate independent of the image, and changes to one do not require changes to the
other. Using packages also allows us continue with our suite scheduling logic
while non-essential packages are staged asynchronously.

## Autoserv packaging high-level overview

Autoserv’s packaging system is essentially a set of functions to fetch, upload,
install, and remove packages that are necessary to control a device under test
(DUT). Each package is a collection of files that is tarred and bzipped (into a
\*.tar.bz2 file) to facilitate the transferring of files onto the DUT. When
transferring packages to a DUT, the system computes an MD5 checksum for each
package to determine whether or not any previously-installed package has changed
and needs to be re-installed. A package is only installed if it has changed
according to the checksum (or, of course, if the package is not already on the
DUT).

There are 3 types of packages: those associated with test code (“test”); with
profiler code (“profiler”); and with dependencies (“dep”). These packages are
all generally handled the same by the packaging system, but they’re installed
into different subfolders on the DUT. There is also a special package that is
used to bundle the autotest client code (“client”).

## Implementation details

Much of the packaging system is implemented in
client/common_lib/base_packages.py. It contains the logic for fetching packages
from either HTTP or from a local filesystem (class HttpFetcher and class
LocalFilesystemFetcher, respectively). Class BasePackageManager in that file
also contains the logic for creating package files (tar_package), fetching and
installing (i.e., untarring) package files (fetch_pkg and install_pkg), and
working with checksums.

The autotest client code is installed via the packaging system from the
server/autotest.py file. Autotest deps are installed via setup_dep in
client/bin/job.py. The utils/packager.py file also calls into the packaging
system to create packages.

For test code, packages are generally created at emerge time (this is specific
to chromeOS). They are simply bzipped versions of corresponding test code
located in the source directory at client/site_tests. If a previously-built
package is missing from both a DUT and the server, autoserv will bundle a
temporary package using local source and send it to the DUT. Otherwise, if the
server finds a previously-built package, it will send it to the DUT only if it’s
changed from what’s already there. In this case, local changes to source code
are ignored because the previously-built package is used instead.
autotest_quickmerge works around this by deleting the package files for altered
tests to force autoserv to build and use temporary packages created from local
source.

The interesting package fetching work for test code occurs as part of the
autotest harness, which can be found in client/bin/harness_autoserv.py. This
contains the AutoservFetcher (if you see log messages about fetching from an
autoserv:// repo, this is what's being used). The fetching logic then ends up in
server/autotest.py in _process_line and more interestingly _send_tarball, which
sends the .tar.bz2 package file, or generates one using PackageManager's
tar_package function if it doesn't exist.

Currently, fetching packages via autoserv is only used in non-prod situations,
such as with run_remote_tests.sh. In prod, all of the packages are instead
pulled from devservers via the HttpFetcher using the job_repo_url. This is a
result of [crbug.com/212641](http://crbug.com/212641) and
[crbug.com/216918](http://crbug.com/216918); we were hitting a case where
fetching would hang (it creates a fifo that gets written to, once the package
has been pushed down to the DUT). A few CLs were subsequently pushed so that
fetching from autoserv could be disabled in prod
(<https://gerrit.chromium.org/gerrit/#/c/33804/2> and
<https://gerrit.chromium.org/gerrit/#/c/34665/3>).

At a high level, this is what happens:

1. In the simplest case, a reimage job runs, after which a server jobs runs. The
reimage job stages autotest packages and sets the job_repo_url as part of

the autoupdate 'test'. These packages contain the tests (one bz file for each
test), the dependencies (like chrome_test) and client-autotest.

2. job_repo_url is set in add_cros_version_labels_and_job_repo_url, and points
to the devserver where the required packages are staged.

3. Server job kicks off autotest.py/site_autotest.py, which are the classes
responsible for installing autotest on the client.

4. autotest.py gets the job_repo_url through site_autotest's get_fetch_location,
and creates a PackageManager in _install_using_packaging

5. This package manager is the aforementioned HttpFetcher, which does a wget
from the given url to fetch packages; This happens through

client/common_lib/base_packages install_pkg.

**Deps:**

An example of a dep is pyauto. The actual installation of this dep happens
through bin/job.install_packages on the DUT, though the steps are pretty general
for package installation and are as follows:

1. Installs packages using packages.PackageManager

- Remove the install dir and reinstalls by default.

2. The installation happens through PackageManager.fetch_pkg

The name of the tarball to get from the repo_url is constructed as
pkg_type-name.tar.bz2. In the case of pyauto this is dep-pyauto.tar.bz2

3. Check if the package has already been fetched

If not, check if we need to fetch:

a. Check if a checksum exists on the DUT

b. Fetch checksum from package url if it doesn't, through fetch_pkg_file

c. Parse the contents of the checksum file into a per package dictionary

d. Create an md5 checksum of the package present at
/usr/local/autotest/packages/client-autotest.tar.bz2 on the DUT

e. Update the checksum file on the DUT so we don't pull the same package again
through _save_checksum_dict

f. If the checksums don't match untar contents of the package via untar_pkg

g. This will essentially run a tar command on the DUT, recompute the checksums,
and rewrite them to the DUT.

4. If fetching is needed, invoke base_packages.fetch_pkg_file which will lead to
a wget in the case of pyauto:

wget --connect-timeout=15 --retry-connrefused --wait=5 -nv
http://devserver:port/static/archive/build/autotest/packages/dep-pyauto_dep.tar.bz2
-O /usr/local/autotest/packages/dep-pyauto_dep.tar.bz2

5. Place the package in /usr/local/autotest/packages

6. Update the checksum

7. Install the package through base_packages.install_pkg

8. Check to see if the install dir exists: ls
/usr/local/autotest/packages/install_dir

8. The actual untarring happens in base_packages.untar_pkg, before this we mkdir
install_dir untar_pkg runs tar xjf from_path -C dest_dir

In an end to end run of a pyauto test, the following packages will get installed
through the process described above:

On the server, install client-autotest on the DUT:

1. mkdir /usr/local/autotest/packages

2. Copy autotest client packages to
/usr/local/autotest/packages/client-autotest.tar.bz2

3. Untar the package if the checksum doesn't match, as described above.

On the DUT, install the tests and pyauto dep:

1. Execute a command like: ./bin/autotest -H autoserv --verbose --user=beeps
/usr/local/autotest/control.autoserv

Actually that's a lie, it executes autotestd_monitor which executes autotestd
which leads to that command, but that is irrelevant for this discussion.

2. Installing the test:

a. The control file Invokes job.run_test with the test name, which leads to
common_lib/test.runtest

b. bin/job.install_pkg installs the test using
common_lib.packages.PackageManager leading to a line in the logs like (instead
of the wget):

22:26:48 DEBUG| Successfully fetched test-login_LoginSuccess.tar.bz2 from
http://devserver/static/archive/lumpy-release/R29-

4279.0.0/autotest/packages/test-login_LoginSuccess.tar.bz2

Again this installation happens through a process as described above.

3. Installing deps, cros_ui_test:

a. We then go on to executing the initialize function of the test via
common_lib/test._cherry_pick_call

- Until this point the infrastructure was totally oblivious that the test in
question requires pyauto

b. Since pyauto tests inherit from cros_ui_test, we execute the initialize of
cros_ui_test, which does things like

start an authserver, remove cryptohome valuts, restart cros_ui etc.
PyAutoTest.initialize finally install deps through

cros/pyauto_test._install_deps and then proceeds to import pyauto.

c. PyAutoTest passes in the install_dir (/usr/local/autotest/deps/pyauto_dep)
and the package_dir (/usr/local/packages) which calls into bin/job

## Relevant links

    General autoserv documentation:
    <https://github.com/autotest/autotest/wiki/Autoserv>

*   User-facing documentation about test dependencies:
            <https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/docs/best-practices.md#Adding-test-deps>

*Published May 13, 2013*
