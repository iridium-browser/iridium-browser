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
page_name: autotest-results-logs
title: Autotest Results Logs
---

This is a listing and brief explanation of the various results logs available
through autotest. It is in no way comprehensive, and is only meant as an
introduction to the topic.

**High Level Overview:**

1. Test runs and dumps its results into /usr/local/autotest/results

2. Server job pulls this entire directory back via
log_collector.collect_client_job_results. This does an rsync from the server
results dir to the client results dir.

3. [GSoffloader](http://www.chromium.org/chromium-os/testing/gs-offloader)
uploads these results to google storage.

The rest of this document describes how step 1 occurs, to learn more about the
server job or gsoffloader please consult relevant documents.

**How are these logs created?**

Sysinfo logs are created by executing a command in a subprocess and redirecting
it’s stdout to the appropriate file through base_sysinfo.py run(), examples of
such commands are 'ls -l /boot' and '/opt/google/chrome/chrome --version', or
just executing something like shutil.copyfile through a hook. There are 4 types
of hooks:

*   boot_loggables - default commands to log per boot, /proc/mounts,
            /proc/cmdline, uname
*   test_loggables - default commands to log per test, dmesg, df
*   before_iteration_loggables - default commands to log before
            iteration
*   after_iteration_loggables - default commands to log after iteration

**How does job.run_test trigger one of the loggables?**

job.run_test creates a _sysinfo_logger through server/test.py

Before invoking common_lib/test.py runtest, it sets up sysinfo logging by:

1.  installing autotest into a tmp sysinfo directory like
            /tmp/sysinfo/autoserv-asdf
2.  executing the sysinfo_before_test_script, which leads to a
            job.sysinfo.log_before_each_test

**How do I collect a random directory as a result?**

You should add it as a loggable hook. Say you would like to collect some
directory every test iteration, you will first need:

*   A class that understands how to do this:
    *   logdir: plain old copy from source to destination, create
                destination if it doesn't exist.
    *   diffable_logdir: copies the diff of the contents of the dir.
    *   purgeable_logdir: copies the contents and deletes source.
    *   command: runs a command
*   A sysinfo hook: The job will call self.sysinfo.log_after_each_test,
            which will execute the 'run' arguments of all the classes in the
            list, for each hook.
    *   iteration loggables are called on every invocation of run_job,
                for a given test job instance
    *   boot/test loggables are called on every instantiation of the job
                object, with the latter only happening for client jobs that
                represent tests (i.e there are different types of jobs, like
                site_job, base_client_job, base_job etc but most often you only
                care about the client jobs).

Once you've picked a class and hook:

    Instantiate this class and add it to a list of classes to call every
    iteration/test/boo

    1.  Import site_sysinfo in your test: from
                autotest_lib.client.bin.site_sysinfo
    2.  within your initialize method add:
                self.test_loggables.add(site_sysinfo.logdir('/tmp/mytempdir_to_copy'))

    After your test is done, check the
    &lt;results_folder&gt;/&lt;test_name&gt;/sysinfo/tmp/mytempdir_to_copy
    directory, where the results folder is the one you specified through -r when
    invoking run_remote_tests.

What follows is an inspection of directories we pull back from the DUT:

<table>
<tr>

<td>Autotest</td>

<td>generate_logs</td>

</tr>
<tr>

<td>.</td>
<td> |-crashinfo.172.22.193.233</td>
<td> |-debug</td>
<td> |-power_Resume</td>
<td> |---debug</td>
<td> |---profiling</td>
<td> |---results</td>
<td> |---sysinfo</td>
<td> |-----iteration.1</td>
<td> |-------var</td>
<td> |---------spool</td>
<td> |-----------crash</td>
<td> |-----var</td>
<td> |-------log_diff</td>
<td> |---------chrome</td>
<td> |-----------Crash Reports</td>
<td> |---------gct</td>
<td> |---------metrics</td>
<td> |---------power_manager</td>
<td> |---------recover_duts</td>
<td> |---------ui</td>
<td> |---------update_engine</td>
<td> |---------window_manager</td>
<td> |---------xorg</td>
<td> |-------spool</td>
<td> |---------crash</td>
<td> |-sysinfo</td>

<td>.</td>
<td> |-crashdumps</td>
<td> |---Crash Reports</td>
<td> |-network_profiles</td>
<td> |---var</td>
<td> |-----cache</td>
<td> |-------shill</td>
<td> |-policy_data</td>
<td> |---whitelist</td>
<td> |-system_level_logs</td>
<td> |---chrome</td>
<td> |-----Crash Reports</td>
<td> |---gct</td>
<td> |---metrics</td>
<td> |---power_manager</td>
<td> |---recover_duts</td>
<td> |---ui</td>
<td> |---update_engine</td>
<td> |---window_manager</td>
<td> |---xorg</td>
<td> |-user_level_logs</td>

</tr>
</table>

The collection of these logs happens through site_sysinfo, base_sysinfo.
client/bin base_sysinfo has a set of default commands to run per
test/boot/iteration and sets them in base_sysinfo __init__.

1.

var/

--log_diff/

----logs from cc files grouped by function.eg: power_manager directory contains
log messages from files input.cc, powerd.cc etc. The redirection of their output
occurs through LOG macros (eg chromium/src/base/logging.cc: ~307), and autotest
pulls the new contents of ‘/var/log’ generated during the test in its
log_after_\* hooks using site_sysinfo.diffable_logdir (which rsyncs).

--spool/

----crash

\[server/site_crashcollect.py\]

On every crash we get a minidmp on the DUT, the server pulls this file back and
attempts to symbolicate it with symbols in /build/ and the devserver. how does
the server know theres a crash, where does it go looking for a crash dump file
and what generates a dump file.

Normal minidumps are uploaded to a crash server which attempts to produce a
stack trace txt file. Autotest tries to run stacktrace with the appropriate
symbols on a crash server.

2.

\[base_sysinfo in bin/base_sysinfo.py\]

boot_loggables-cmdline, installed_packages, proc_mounts, uname, meminfo,
slabinfo, version, pci, cpuinfo, modules, interrupts, partitions, lspci -vn,
gcc--version, ld--version, mount, hostname, uptime.

test_loggables-df, dmesg, installed_packages, schedstat, meminfo, slabinfo,
interrupts

If you have come this far, you may also be interested in reading the autotest
client tests
[codelab](/chromium-os/testing/test-code-labs/autotest-client-tests).
