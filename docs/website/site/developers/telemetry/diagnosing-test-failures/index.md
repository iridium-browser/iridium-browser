---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/telemetry
  - 'Telemetry: Introduction'
page_name: diagnosing-test-failures
title: 'Telemetry: Diagnosing Test Failures'
---

If you're seeing a test failure on the bots and are unable to diagnose the issue
from the test log, here are some steps to help debug the issue.

## ==Reproducing the Issue==

### Determining how Telemetry was invoked

The command line used to invoke Telemetry can be seen when looking at the log
for the failing build step, the output is very verbose but if you search for
\`run_benchmark\` you should be able to find it. Look for a command that
resembles the following:

```none
/tools/perf/run_benchmark -v --output-format=chartjson --upload-results blink_perf.shadow_dom --browser=reference --output-trace-tag=_ref
```

### Running a Windows VM

Instructions on setting up a Windows VM on a Linux host [can be found
here](https://docs.google.com/a/google.com/document/d/1fAQRjM9oJqVrxzNpS1GH_62smLgphvpeWChCKo-wW2Y/edit?usp=sharing)
(for Googlers only). For how to run it locally on windows, check the
instructions [here](/developers/telemetry/run_locally).

### Diagnosing on the trybots

Reproducing a failure locally is the most desirable option, both in terms of
speed and ease of debugging. If the failure only occurs on a specific OS you
don't have access to or if the failure only reproduces on a specific bot you may
need to access that bot directly. Information on accessing a trybot remotely
[can be found here](https://goto.google.com/chrome-trybot-remote-access)
(Internal-only).

You can find the name of the trybot the test is failing on by looking at the
"BuildSlave" section of the test run (build58-a1 in the below screenshot):

[<img alt="image"
src="/developers/telemetry/diagnosing-test-failures/Screen%20Shot%202014-12-10%20at%209.42.17%20AM.png"
height=109
width=200>](/developers/telemetry/diagnosing-test-failures/Screen%20Shot%202014-12-10%20at%209.42.17%20AM.png)

Another option is to use the [performance
trybots](/developers/telemetry/performance-try-bots) to try a patch with extra
diagnostics.

## Tips on Diagnosis

*   Telemetry prints local variables on test failure and will attempt to
            print a stack trace for the browser process if it crashes, this
            should be visible in the builder output.
*   If the Telemetry is wedged, you can send it a SIGUSR1 ($kill
            -SIGUSR1 &lt;telemetry pid&gt; on a POSIXy system), this has the
            effect of printing the current stack trace (search for
            InstallStackDumpOnSigusr1() to see the code behind this.)
*   Consider adding logging.info()messages to the code to print
            diagnostic information, as can be seen above - the bots run
            Telemetry with the '-v' option so these will be visible in the build
            output. These can be sent to the [performance
            trybots](/developers/telemetry/performance-try-bots) or committed
            and reverted afterwards but consider leaving in messages that might
            help diagnose similar issues in the future. If left in, beware of
            spamming the console.
*   As the benchmark runs, devtools can be used to examine the state of
            the running page.

### Useful Telemetry command line flags

<table>
<tr>
<td>Name</td>
<td> Effect</td>
</tr>
<tr>
<td> --browser={list,&lt;version&gt;}</td>
<td> Change the version of the browser used, list will print all the browsers that Telemetry can see and a browser name will run that browser e.g. --browser=release</td>
</tr>
<tr>
<td> --repeat=&lt;N&gt;</td>
<td> Repeats the test N times. Note that flaky tests might fail after repetition e.g. --repeat=5</td>
</tr>
<tr>
<td> --story-filter=&lt;regex&gt;</td>
<td> Only run pages from the pageset that match the given regex, this is useful to make test runs faster if a test only fails on a specific page e.g. --story-filter=flickr</td>
</tr>
<tr>
<td> --story-filter-exclude=&lt;regex&gt;</td>
<td> Inverse of the above.</td>
</tr>
<tr>
<td> -v, -vv</td>
<td> Change log level</td>
</tr>
<tr>
<td> --show-stdout</td>
<td> Show browser stdout</td>
</tr>
<tr>
<td> --extra-browser-args=</td>
<td> Pass extra arguments when invoking the browser, you can find a useful list of Chrome's command-line arguments <a href="http://peter.sh/experiments/chromium-command-line-switches/">here</a>.</td>
<td>E.g.: '--extra-browser-args=--enable-logging=stderr --v=2'</td>
</tr>
</table>
