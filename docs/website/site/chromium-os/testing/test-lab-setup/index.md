---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: test-lab-setup
title: Test Lab Setup
---

This page will walk you through duplicating the majority of the test lab set up
that Chrome OS runs internally to validate builds on real hardware.

**You will need:**

*   A linux machine for running the multiple servers required.
    *   Note you can run them all on one machine or multiple in order to
                scale and spread load.
*   A Chromiumos checkout
    *   <http://www.chromium.org/test lab setup (i.e. Autotest Server
                and Devserver)chromium-os/quick-start-guide>
*   A Chrome OS Device.

## Setting up SSH Access to your test device.

In order to run the automated tests against your device you need to ensure it is
running a Test Image and you have password-less SSH Access.

Please follow:
<https://chromium.googlesource.com/chromiumos/docs/+/HEAD/developer_guide.md#Set-up-SSH-connection-between-chroot-and-DUT>

## Setting up an Autotest Server and Web Frontend

Autotest is the software that is used to run the tests on devices and also
collect the results.

Please follow to install and configure the tools:

<https://sites.google.com/a/ch
romium.org/dev/chromium-os/testing/autotest-developer-faq/setup-autotest-server>

After this is complete you should have an Autotest Frontend (AFE) running on
your server, please go to <http://localhost> to verify.

## Using the Autotest Server

Now that you have set up the Autotest Server, you need to import your tests, add
test hosts and launch the scheduler. Please refer to
<https://www.chromium.org/chromium-os/testing/autotest-developer-faq/autotest-server-usage>
for more information.

**Note: At this point you can run tests through the Web Interface. However you
lack the ability to automatically have your devices be re-imaged by the test
servers. The following steps will setup a Devserver to stage test images from
Google Storage so that you can do automated updates. This process is currently
for Internal Google Use and our Partners. If you are a Partner, please contact
your Google Representative about requesting access to our images.**

## Devserver Prerequisites

Our images are stored in Google Storage and first you need to setup your machine
to give you access to these builds.

**You will need:**

*   Python modules: CherryPy and lockfile
    *   **apt-get install python-cherrypy3 python-lockfile**, or
                easy_install -U cherrypy3 lockfile
*   [gsutil
            installed](https://chromium.googlesource.com/chromiumos/docs/+/HEAD/gsutil.md#installing-gsutil)
*   A .boto file in your home directory
    *   Run \`gsutil config\`

If you are a Chrome OS Googler you can verify everything is setup by running
\`gsutil ls gs://chromeos-image-archive\` if you are an external partner test
with the bucket you have been supplied.

## Devserver Setup

\*After you have installed the above dependencies:

```none
cd [Your Chromium OS Checkout]/src/platform/dev
./devserver.py -t --production --archive_dir /some/path/images
```

Explanation of options above:

*   -t - This tells the devserver to serve test images. If you do not
            want to serve test images leave this option off. For a lab set up
            test images are required
*   --production - This increases the server thread pool for cherrypy.
            Again if you are not expecting any sort of load on this server this
            is unnecessary
*   --archive_dir - The directory where all images are stored that are
            staged by the devserver.

After running the above command you will see some information printed out by
cherrypy and you should be able to browse to*http://server_hostname:8080*. There
you will see a short index of the different operations that can be carried out
similar to what is summarized in the "Services Provided by Devserver" paragraph
above.

## Running Test Suites

Now that everything is configured please refer to the Test Suite Documentation
on how to the "lab" you just set up. You will likely want to pay attention to
the sections involving test_that and run_suite.py

<https://www.chromium.org/chromium-os/testing/test-suites>
