---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/writing-and-developing-tests
  - Writing and Developing Tests
page_name: test-deps-faq
title: Test Deps FAQ
---

[TOC]

### Where do I store large files that need to be publicly accessible for tests?

Google Storage is used as the general storage facility for large files. Please
avoid putting large files in git as it increases everyone's check out size. A
good general rule of thumb is:

If it is a file larger than 5 megs that will be changing over time, put it in
Google Storage.

A similar approach is taken as
[localmirror](https://sites.google.com/a/google.com/chromeos/resources/engineering/releng/localmirror)
except we use the Google Storage bucket ***gs://chromeos-test-public***

To upload your files please follow ["How to get your files my files to local
mirror"](https://sites.google.com/a/google.com/chromeos/resources/engineering/releng/localmirror#TOC-How-do-I-get-my-files-in-localmirro)
but using the bucket ***gs://chromeos-test-public***

<http://sandbox.google.com/storage/?arg=chromeos-test-public>

### How do I use deps?

The deps feature is used to install packages that are required by tests.
Sometimes these packages must be compiled from source and installed on the test
machine. Rather than include this installation procedure in the test itself, we
can create a 'dep' which does this for us. Then the test can specify that it
needs this dep to work. Autotest will make sure that the dependency is built and
copied to the target.
First take a look at an example: hardware_SsdDetection. You will notice that it
does this in the setup method:

```none
self.job.setup_dep(['hdparm'])
```

This ends up running the python file `client/deps/hdparm.py`. This will look
after building the hdparm utility and making sure it is available for copying to
the target. The mechanics of this are explained in the next topic, but imagine
using `./configure` and installing with the `client/deps/hdparm` directory as
the install root.
Within the run_once() method in the test you will see this:

```none
        dep = 'hdparm'
        dep_dir = os.path.join(self.autodir, 'deps', dep)
        self.job.install_pkg(dep, 'dep', dep_dir)
```

This is the code which actually installs the hdparm utility on the target. It
will be made available in the `/usr/local/autotest/deps/hdparm` directory in
this case. When it comes time to run it, you will see this code:

```none
        path = self.autodir + '/deps/hdparm/sbin/'
        hdparm = utils.run(path + 'hdparm -I %s' % device)
```

This is pretty simple - since the hdparm binary was installed to
`client/deps/hdparm/sbin/hdparm` we can run it there on the target.

### How can I create my own dep?

If you have your own tool and want to create a dep for it so that various tests
can use it, this section is for you.
First create a subdirectory within deps. Let's call your dep 'harry'. So you
will create `client/deps/harry` and put `harry.py` inside there.
In harry.py you will want something like the following:

```none
#!/usr/bin/python
# Copyright 2018 The ChromiumOS Authors.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os
from autotest_lib.client.bin import utils
version = 1
def setup(tarball, topdir):
    srcdir = os.path.join(topdir, 'src')
    utils.extract_tarball_to_dir(tarball, srcdir)
    os.chdir(srcdir)
    utils.system('patch -p1 < ../fix-wibble.patch')
    utils.configure()
    utils.make()
    utils.make('install')
# We got the src from http://harry.net/harry-0.24.tar.bz2
pwd = os.getcwd()
tarball = os.path.join(pwd, 'harry-0.24.tar.bz2')
utils.update_version(pwd + '/src', False, version, setup, tarball, pwd)
```

The URL mentioned in the file should be the location where you found your
tarball. Download this and put it in the deps directory as well. You will be
checking this into the repository as a binary file.
The `utils.update_version()` call ensures that the correct version of harry is
installed. It will call your setup() method to build and install it. If you
specify False for the preserve_srcdir parameter then it will always erase the
src directory, and call your setup() method. But if you specify True for
preserve_srcdir, then update_version() will check the installed version of the
src directory, and if it hasn't changed it will not bother installing it again.
You will also see that pwd is set to the current directory. This is
`client/deps/harry` in this case because autotest changes this directory before
importing your python module. We unpack into a src subdirectory within that to
keep things clean.
When your setup() method is called it should unpack the tarball, configure and
build the source, then install the resulting binaries under the same
`client/deps/harry` directory. You can see the steps in the example above - it
mirrors a standard GNU UNIX build process. If you need to pass parameters to
configure or make you can do that also. Any parameters after 'setup' in the
update_version() call are passed to your setup method.
After calling utils.update_version() from within harry.py we will have binaries
installed (say in `client/deps/harry/sbin/harry`) as well as the src directory
still there (`client/deps/harry/src`).
Finally for reasons that we won't go into you should create the file common.py
in your directory, like this:

```none
# Copyright 2018 The ChromiumOS Authors.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os, sys
dirname = os.path.dirname(sys.modules[__name__].__file__)
client_dir = os.path.abspath(os.path.join(dirname, "../../"))
sys.path.insert(0, client_dir)
import setup_modules
sys.path.pop(0)
setup_modules.setup(base_path=client_dir,
                    root_module_name="autotest_lib.client")
```

and you need a file called 'control' too:

```none
# Copyright 2018 The ChromiumOS Authors.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
job.setup_dep(['fio'])
```

### Troubleshooting your dep

If you have created a new dep but run_remote_tests.sh doesn't work and you get
an error like:

```none
PackageInstallError: Installation of harry(type:dep) failed : dep-harry.tar.bz2 could not be fetched from any of the repos ['autoserv://']
```

then this section is for you. It explains a little more of the detail of how the
dependencies are transfered from server (your workstation) to the client
(target).
When you call self.job.install_pkg(dep, 'dep', self.dep_dir), it causes the
autotest client to install the dependency. In this case the client will end up
using the autoserv fetcher. Within client/bin/harness_autoserve.py you will see
the class AutoservFetcher. It has a method fetch_pkg_file() which calls
harness_autoserv.fetch_package() to fetch the package. This issues a
AUTOTEST_FETCH_PACKAGE command over the logging connection to autoserv,
requesting the package. Yes the logging connection is re-used to support a kind
of ftp server for the client!
The server end of autoserv sits in server/autotest.py. Each line of log output
coming from the client is examined in client_logger._process_line(). When it
sees a line starting with AUTOTEST_FETCH_PACKAGE it calls _send_tarball() which
tries to find a matching directory to package up and send. The pkg_name is in a
standard format: `pkgtype-name.tar.bz2` and in this case pkgtype is 'dep'. It
will package up the client/deps/&lt;name&gt; directory into a tarball and send
it to the client. When this is working you will see something like this message
when you run the test:

```none
12:08:06 INFO | Bundling /home/sjg/trunk/src/third_party/autotest/files/client/deps/harry into dep-harry.tar.bz2
```

When it is not working, make sure the directory exists on the server side in the
right place.

### What if my dep is built by another ebuild?

The above method is fine for external packages, but it is not uncommon to want
to use a byproduct of another ebuild within a test. A good example of this is
the Chromium functional tests, which require PyAuto, a test automation framework
built by Chromium. In the
`src/third_party/chromiumos-overlay/chromeos-base/chromeos-chrome` ebuild you
will see:

```none
    if use build_tests; then
        install_chrome_test_resources "${WORKDIR}/test_src"
        # NOTE: Since chrome is built inside distfiles, we have to get
        # rid of the previous instance first.
        rm -rf "${WORKDIR}/${P}/${AUTOTEST_DEPS}/chrome_test/test_src"
        mv "${WORKDIR}/test_src" "${WORKDIR}/${P}/${AUTOTEST_DEPS}/chrome_test/"
        # HACK: It would make more sense to call autotest_src_prepare in
        # src_prepare, but we need to call install_chrome_test_resources first.
        autotest_src_prepare
```

You can see that this is creating a deps directory within the build root of
chrome, called chrome_test. From the previous section we know that a
chrome_test.py file must be installed in this deps directory for this to work.
This actually comes from the chromium tree
(`chrome/src/chrome/test/chromeos/autotest/files/client/deps/chrome_test/chrome_test.py`
since you asked). This file is very simple:

```none
#!/usr/bin/python
# Copyright 2018 The ChromiumOS Authors.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import common, commands, logging, os
from autotest_lib.client.bin import utils
version = 1
def setup(top_dir):
    return
pwd = os.getcwd()
utils.update_version(pwd + '/src', False, version, setup, None)
```

In this case the directory already contains pre-built binaries, since they were
built and copied in by the ebuild above. The setup() method will always be
called, but has nothing to do.
