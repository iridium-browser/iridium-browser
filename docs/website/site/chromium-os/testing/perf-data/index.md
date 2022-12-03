---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: perf-data
title: 'User Guide: Chrome OS Performance Tests and the Chrome Performance Dashboard'
---

[TOC]

## Summary/Quick Start

If you have a performance autotest (either a wrapper around a Telemetry test for
measuring chrome performance, or a more Chromium OS-specific performance
autotest) and you want any of its measured perf values displayed as graphs on
the [performance dashboard](https://chromeperf.appspot.com/), you should
complete the following high-level steps. More details for each of the steps
below are provided later in this document.

1.  Output your perf data in the test. Have your autotest invoke
            self.output_perf_value() for each measured perf value that you want
            displayed on the perf dashboard.
2.  Specify presentation settings. Associate your test with the proper
            sheriff rotation. Also decide if you need to override any of the
            other default presentation settings for your perf graphs on the
            dashboard. To do so, add an entry for your perf test to the
            [perf_dashboard_config.json](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/tko/perf_upload/perf_dashboard_config.json)
            file to specify your sheriff rotation and custom presentation
            settings.
3.  Let the test run a few times. Wait for your test to run a few times
            in the autotest lab so that some measured perf data can be sent to
            the perf dashboard. Autotest automatically uploads measured perf
            data to the perf dashboard as soon as perf test runs complete in the
            lab.
4.  **Notify chrome-perf-dashboard team for regression analysis and
            alerts (Not required)**. If you want to have the perf dashboard
            analyze your data for regressions, you should request to have your
            data monitored. To do this, you can send a monitoring request by
            [clicking
            here](https://code.google.com/p/chromium/issues/entry?summary=Monitoring%20Request&comment=Please%20add%20monitoring%20for%20the%20following%20tests%3A%0A%0ABuildbot%20master%20name%3A%0ATest%20suite%20names%3A%0ARestrict%20to%20these%20specific%20traces%20(if%20any)%3A%0AEmail%20address%20and%2For%20url%20of%20sheriff%20rotation%3A%20%0AReceive%20individual%20email%20alerts%20immediately%20or%20as%20a%20daily%20summary%3F%0AShould%20these%20alerts%20be%20Google-internal%3F%0A&labels=Performance-Dashboard-MonitoringRequest%2CRestrict-View-Google&cc=)
            or send a message to chrome-perf-dashboard-team@google.com.
5.  See your perf graphs on the dashboard. Navigate in your browser to
            the perf dashboard, log in with your @google.com account (perf data
            is internal-only by default), and then locate and view the perf
            graphs for your test on the dashboard.

## The Performance Dashboard

The Chromium OS team will be using the same performance dashboard that Chrome
team uses, which is available here:

<https://chromeperf.appspot.com/>

Reasons for sharing Chrome team's dashboard include:

*   Integration with existing tools for automated detection of
            performance test result anomalies.
*   Shared codebase; no need for Chromium OS team to develop/maintain
            its own dashboard.
*   More consistency in presenting/viewing performance data across
            multiple chrome products, from desktop to Chromium OS to mobile.

The source code for the dashboard is Google internal-only. Performance test data
is organized around several main concepts on the dashboard:

*   **Test**: This refers to a single measured performance metric, and
            it is typically described as either “test_name/metric_name” or
            “test_name/graph_name/metric_name”, the latter of which is used when
            multiple metrics are to be displayed on the same graph.
*   **Bot**: This refers to a platform that is running a test. For
            Chrome OS, it may be something like “cros-lumpy” or “cros-alex”.
            Each bot may have multiple tests associated with it.
*   **Master**: This refers to a logical group of bots that are running
            tests and are monitored by perf sheriffs. The name “Master” comes
            from Chrome Buildbot, which has a concept called a master. Perf
            sheriffs monitor the test results associated with particular
            masters. For Chrome OS tests that do not run in Chrome Buildbot,
            this should be a string describing the top-level category which this
            test should be associated with, e.g. ChromeOSPerf.

The dashboard itself is externally-visible, but supports internal-only data that
can only be viewed if logged into the dashboard with an @google.com account.
Data is either internal or external on a per-bot basis. By default, data
associated with a bot is considered to be internal-only until it is [explicitly
allowed](https://chromeperf.appspot.com/bot_whitelist) to be visible externally
(i.e., without having to login to the dashboard).

To see performance graphs from the front page of the dashboard, click on the
“Performance Graphs” link in the top menu, or else directly navigate to
<https://chromeperf.appspot.com/report>. There, you will see a few dropdowns
that let you specify what graph data you want to see. First, select the test
name from the left-most dropdown. Next, select the bot name from the next
dropdown (bots are sorted alphabetically by name under the master to which they
belong). Finally, select the metric name (or graph name) from the right-most
dropdown to see the selected performance graph.

> *Example*: We'll take a look at the sunspider benchmark results on lumpy.
> First navigate to <https://chromeperf.appspot.com/report>. In the “Select a
> test suite” dropdown, click on “sunspider”. Next, in the “Select bots”
> dropdown, under the “ChromiumPerf” master category, click on “cros-lumpy”. A
> new dropdown will appear that shows the default metric for this benchmark,
> which is “Total”. The page will also display the graph for the “Total” metric.
> If you click on the “Total” dropdown, you can view the other metrics measured
> by this test. Click on another one, say “3d-cube”, to see the graph for that
> metric.

If you don't see your data, you may need to log into the dashboard with your
@google.com account (see the “Sign in” link on the top-right of the page). This
will allow you to also see the internal-only test data.

If you still don't see your data, check to make sure your data is getting
uploaded to the dashboard in the first place. Make sure "results-chart.json"
file in your autotest result directory contains valid values

## Instructions for Getting Data onto the Perf Dashboard

### Output your perf data in the test

In order to get your test's measured perf data piped through to the perf
dashboard, you must have your test invoke self.output_perf_value() for every
perf metric measured by your test that you want displayed on the perf dashboard.

The output_perf_value function is currently defined
[here](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/client/common_lib/test.py).
Here is the function definition:

def output_perf_value(self, description, value, units=None,

higher_is_better=True, graph=None):

"""

Records a measured performance value in an output file.

The output file will subsequently be parsed by the TKO parser to have

the information inserted into the results database.

@param description: A string describing the measured perf value. Must

be maximum length 256, and may only contain letters, numbers,

periods, dashes, and underscores. For example:

"page_load_time", "scrolling-frame-rate".

@param value: A number representing the measured perf value, or a list

of measured values if a test takes multiple measurements.

Measured perf values can be either ints or floats.

@param units: A string describing the units associated with the

measured perf value. Must be maximum length 32, and may only

contain letters, numbers, periods, dashes, and underscores.

For example: "msec", "fps", "score", "runs_per_second".

@param higher_is_better: A boolean indicating whether or not a "higher"

measured perf value is considered to be better. If False, it is

assumed that a "lower" measured value is considered to be

better.

@param graph: A string indicating the name of the graph on which the perf value
will be subsequently displayed on the chrome perf dashboard. This allows
multiple metrics be grouped together on the same graphs. Defaults to None,
indicating that the perf value should be displayed individually on a separate
graph.

"""

Example: Suppose you have a perf test that loads a webpage and measures the
frames-per-second of an animation on the page 5 times over the course of a
minute or so. We want to measure/output the time (in msec) to load the page, as
well as the 5 frames-per-second measurements we've taken from the animation.
Suppose we've measured these values and have them stored in the following
variables:

page_load = 173

fps_vals = \[34.2, 33.1, 38.6, 35.4, 34.7\]

To output these values for display on the perf dashboard, we need to invoke
self.output_perf_value() twice from the test, once for each measured metric:

self.output_perf_value("page_load_time", page_load, "msec",

higher_is_better=False)

self.output_perf_value("animation_quality", fps_vals, "fps")

For the “animation_quality” metric, the perf dashboard will show the average and
standard deviation (error) from among all 5 measured values. Note that the
“higher_is_better” parameter doesn't need to be specified for
“animation_quality”, because it defaults to True and a higher FPS value is
indeed considered to be better.

Autotest
[platform_GesturesRegressionTest](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/client/site_tests/platform_GesturesRegressionTest/platform_GesturesRegressionTest.py)
is an example of a real test that invokes self.output_perf_value().

It's assumed that every perf metric outputted by a test will have a unique
“description”, and you need to invoke self.output_perf_value() once for each
perf metric (unique description) measured by your test.

In some cases, a test may take multiple measurements for a given perf metric.
When doing so, the test should output all of these measurements in a list for
the “value” parameter. The autotest infrastructure will automatically take care
of computing the average and standard deviation of these values, and both of
those values will be uploaded to the perf dashboard (standard deviation values
are used by the perf dashboard to represent “errors in measurement”, and these
errors are depicted in the graphs along with the average values themselves).

### Group multiple performance metrics into one graph

You can group multiple metrics into one graph by passing a graph name via
argument "graph". The value of "graph" defaults to None, indicating each
performance metric to be displayed on a separate graph.

As an example, suppose we have an autotest called “myTest” that outputs 4 perf
metrics, with descriptions “metric1”, “metric2”, “metric3”, “metric4”. And we
want "metric1" and "metric2" to be displayed on a graph called "metricGroupA";
and "metric3" and "metric4" to be displayed on a graph called "metricGroupB". We
need to call self.output_perf_value()in the following way.

> output_perf_value(description="metric1", units=.., higher_is_better=..,

> graph='metricGroupA')

> output_perf_value(description="metric2", units=.., higher_is_better=..,

> graph='metricGroupA')

> output_perf_value(description="metric3", units=.., higher_is_better=..,

> graph='metricGroupB')

> output_perf_value(description="metric4", units=.., higher_is_better=..,

> graph='metricGroupB')

### Specify presentation settings

#### Required

The perf dashboard requires perf data to be sent with a "master name". The term
"master" is used because this originally referred to the Chrome Buildbot master
name, but for Chrome OS tests that aren't run by Chrome Buildbot, "master"
should describe the general category of test, e.g. ChromeOSPerf or ChromeOSWifi.
We generally recommend just use "ChromeOSPerf" for Chrome OS tests.

Once you have determined the appropriate “master” name, add a new entry to the
file
[perf_dashboard_config.json](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/tko/perf_upload/perf_dashboard_config.json)
to specify this. This JSON config file specifies the master name and any
overridden presentation settings. It is formatted as a list of dictionaries,
where each dictionary contains the config values for a particular perf test
(there should be at most one entry in the config file for any given perf test).
The dictionary for a test in the JSON file must contain, at a minimum, an
“autotest_name” and a “master_name”:

*   “**autotest_name**”: The string name of the autotest associated with
            this config file entry. Must match the actual autotest name.
            Required.
*   “**master_name**” : A string describing the top-level category which
            this test should be associated with, for example ChromeOSPerf
            (recommended) or ChromeOSWifi. Required.

#### Optional

By default, without overriding any of the default presentation settings, the
name of the test on the dashboard will be the same as the autotest name. If you
need to change the name of the test as it appears on the dashboard then you need
to add "dashboard_test_name" to your test's entry in the file
perf_dashboard_config.json.

*   “**dashboard_test_name**”: The string name you want to use for this
            test on the dashboard. Optional: only specify if you don't want to
            use the autotest name itself. Typically, you probably won't want to
            override this. However, it's required by the Telemetry performance
            benchmarks, which are given names like “telemetry_Benchmarks.octane”
            in autotest, but which are simply called names like “octane” on the
            perf dashboard as per convention by chrome team.

### Verify your test and changes

Important: Whenever you make any changes to perf_dashboard_config.json, make
sure to also run the unit tests in that directory before checking in your
changes, because those tests run some sanity checks over the JSON file (e.g., to
make sure it's still parseable as proper JSON). You can invoke the unit test
file directly:

&gt; python perf_uploader_unittest.py.

You could also check your autotest result folder. Look for a file called
"results-chart.json". This is an intermediate JSON file which contains data that
will be later uploaded to the dashboard. Manually check whether this file looks
correct.

### Let the test run a few times

Once you've modified your test to invoke self.output_perf_value() where
necessary, you've specified a sheriff rotation name (“master” name) for your
test, and you've overridden the default presentation settings on the perf
dashboard if you choose to do so, you're now ready to start getting that perf
data sent to the perf dashboard.

Check in your code changes and ensure your test starts running in a suite
through the autotest infrastructure. Perf data should get uploaded to the perf
dashboard as soon as each run of your autotest completes.

### Notify chrome-perf-dashboard-team@ for regression analysis and alerts (Not required)

If you want to have the perf dashboard analyze your data for regressions, you
should request to have your data monitored. You can provide an email address to
receive alerts.

To notify chrome-perf-dashboard-team@, please [**click
here**](https://code.google.com/p/chromium/issues/entry?summary=Monitoring%20Request&comment=Please%20add%20monitoring%20for%20the%20following%20tests%3A%0A%0ABuildbot%20master%20name%3A%0ATest%20suite%20names%3A%0ARestrict%20to%20these%20specific%20traces%20(if%20any)%3A%0AEmail%20address%20and%2For%20url%20of%20sheriff%20rotation%3A%20%0AReceive%20individual%20email%20alerts%20immediately%20or%20as%20a%20daily%20summary%3F%0AShould%20these%20alerts%20be%20Google-internal%3F%0A&labels=Performance-Dashboard-MonitoringRequest%2CRestrict-View-Google&cc=)
to file a "Monitoring Request" bug with the label "Performance-Dashboard" with
the following info. Or send an email to chrome-perf-dashboard-team@google.com

> Please add monitoring for the following tests:

> Buildbot master name: &lt;"master_name" you've specified in
> perf_dashboard_config.json, e.g. "ChromeOSPerf"&gt;

> Test suite names: &lt;"dashboard_test_name" if you have one, otherwise just
> put "autotest_name", e.g. "platform_GesturesRegressionTest"&gt;

> Restrict to these specific traces (if any): &lt;Your choice&gt;

> Email address and/or url of sheriff rotation: &lt;Email who will be monitoring
> the perf data.&gt;

> Receive individual email alerts immediately or as a daily summary? &lt;Your
> choice&gt;

> Should these alerts be Google-internal? &lt;Your choice&gt;

### See your perf graphs on the dashboard

Once you're sure your test has run at least a few times in the lab, navigate in
your browser to the [perf dashboard](https://chromeperf.appspot.com/) and look
for your perf graphs. Refer to the earlier section in this document called “The
Performance Dashboard” for an overview of the dashboard itself, and how to look
for your data there.

## Triage issues

1.  Make sure your test has run at least a few times in the lab
2.  Make sure there is an entry for your test in the file
            [perf_dashboard_config.json](https://chromium.googlesource.com/chromiumos/third_party/autotest/+/HEAD/tko/perf_upload/perf_dashboard_config.json)
    1.  If in the control file, the test is run in the following way
        job.run_test('dummy_Pass', tag="bluetooth")
        then in perf_dashboard_config.json, the field "autotest_name" should
        look like "dummy_Pass.bluetooth". **Make sure the tag is included.**
    2.  If perf_dashboard_config.json is modified, the change needs to
                be pushed to our production servers (please contact
                chromeos-lab-infrastructure@ team)
3.  Check the content in "perf_measurement" in the test's result
            directory. Make sure there is data and they look right.
4.  Check the logs in ".parse.log" in the test's result directory for
            error messages related to perf uploading.
