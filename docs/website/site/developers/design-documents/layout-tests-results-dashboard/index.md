---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
page_name: layout-tests-results-dashboard
title: Tests Results Dashboard
---

WARNING: THIS DOCUMENT IS FAIRLY STALE.

This is a mini-design doc for the test dashboard at
<http://test-results.appspot.com/dashboards/flakiness_dashboard.html>. This
dashboard helps identify flaky tests and compare test expectations across
platforms, including upstream webkit.org's expectations. It currently supports
webkit tests and gtest tests.

The dashboard itself consists of flakiness_dashboard.html and
dashboard/dashboard_base.js. In order to minimize maintenance cost, there is no
server-side logic. Instead the dashboard grabs JSONP files from each builder.
Each builder creates and archives two JSON files that it updates with each run's
results. There is one results.json file per builder that holds the results for
**all** runs and there is an expectations.json file that translates
src/webkit/tools/layout_tests/text_expectations.txt into JSON that the dashboard
can parse. The expectations.json files will be the same on all the bots, so the
dashboard can grab it from any of them.

The JSON files are created by run_webkit_tests.py. It stores them at the top
level of the layout test results directory (same place as results.html). On each
run, it reads in the results.json file from that location, adds the results from
the current run and writes it out again. If it does not exist locally (e.g. if
the layout test results directory was deleted), then it grabs it off the archive
location on the bot. Once both JSON files have been written, they are both
copied to the archive location for the layout test results so that they can be
accessed via http from the dashboard.

Once all the JSON files have loaded, the dashboard does a bunch of processing on
them and generates the HTML for the page, which is mostly then innerHTMLed. The
results.json file also has a bunch of data that is counts of failing tests that
is used in the [test progress
dashboard](http://src.chromium.org/viewvc/chrome/trunk/src/webkit/tools/layout_tests/dashboards/aggregate_results.html).
For the purposes of the flakiness dashboard, they can just be excluded from the
JSON. If they are included, then the layout test progress dashboard would work
as well.

The dashboard stores all the relevant state in the URL's hash and uses
hashchange events to detect changes. Thus it only works in browsers that support
the hashchange event.

**Changes needed in order to add upstream webkit.org**

1.  output results.json and expectations.json per builder. See the two
            notes below marked "For upstream webkit.org".
2.  Add useUpstreamWebkit. It should mimic what useWebKitCanary does
            (primarily it just adds a new builders array).
3.  getPlatformAndBuildType: Needs to be modified to understand any new
            mapping from builderName to platform/buildType
4.  Consider excluding bugs, modifiers, expectations, missing and extra
            columns from the dashboard since upstream webkit doesn't have a
            test_expectations.txt file.

See the webkit bug for progress on making the dashboard work for the webkit
bots: <https://bugs.webkit.org/show_bug.cgi?id=32954>.

#### JSON file size optimizations

If we store information about every test on every run, the JSON files quickly
become many megabytes large. In order to keep the size manageable we do the
following:

*   The number of runs stored is hard-coded at 750.
*   Any test that has passed for all runs and takes less than 1 second
            to run for all those runs is dropped from the JSON.
*   The data for failures/speed of each test result is run-length
            encoded.
*   Strip all unnecessary whitespace.

#### **Incremental updates of JSON files**

Currently the JSON files are generated and updated locally, then they are
uploaded to the archive location (test-results.appspot.com). In order to improve
performance and make it so that the bots don't rely on local state, we will
create a local JSON file with just the incremental updates. That JSON file will
then be uploaded to the appengine server, which will **merge** it with the
stored JSON file. In that case, the logic of truncating to 750 runs will be in
the appengine server.

The JSON format for an incremental update is exactly the same as that of the
stored JSON file. The only catch in doing the merge is that the runs being
merged in may not be uploaded in order (e.g. if we have multiple builders
processing separate runs). So the merge function must take care to look at the
buildNumbers array and merge the values in correctly.

### JSON file formats

#### The order of all arrays is from newest run to oldest run. In other words, the results from each run are prepended to the existing results.json file's contents.

#### results.json

ADD_RESULTS({

"version": NUMBER,

"BUILDER_NAME": {

"wontfixCounts": FAILURE_COUNTS,

"deferredCounts": FAILURE_COUNTS,

"fixableCounts": FAILURE_COUNTS,

"fixableCount": ARRAY_OF_NUMBERS,

"allFixableCount": ARRAY_OF_NUMBERS,

"buildNumbers": ARRAY_OF_NUMBERS,

"webkitRevision": ARRAY_OF_NUMBERS,

"chromeRevision": ARRAY_OF_NUMBERS,

"secondsSinceEpoch": ARRAY_OF_NUMBERS,

"tests": {

"TEST": {

"results": RUN_LENGTH_ENCODED_RESULTS,

"times": RUN_LENGTH_ENCODED_RESULTS

},

...

}

}

});

BUILDER_NAME: Name of the builder the tests were run on (e.g. Webkit Mac10.5).
TEST: Relative path to the test, e.g. "LayoutTests/fast/foo/bar.html".
TODO(ojan): Exclude the LayoutTests prefix now that all layout tests live in
that directory.

FAILURE_COUNTS: Array of objects mapping single-char failure type to number of
failures of that type. The mapping from single-char failure type to the
human-readable string used in expectations.json is held in EXPECTATIONS_MAP in
dashaboard_base.js.

ARRAY_OF_NUMBERS: An array of numbers.

RUN_LENGTH_ENCODED_RESULTS: An array of RUN_LENGTH_VALUEs.

RUN_LENGTH_VALUE: An array with exactly two items. The first item is the number
of runs that this value represents. The second item is the value of the result
for that run. The values used for the results are in dashboard_base.js as
EXPECTATIONS_MAP. If a test is listed in results.json, but was not run during
this run for some reason (e.g. it's skipped), then the 'N' value is used for
that run to indicate we have no data for that test for that run. There's a copy
below for reference:

var EXPECTATIONS_MAP = { 'T': 'TIMEOUT', 'C': 'CRASH', 'P': 'PASS', 'F': 'TEXT',
'I': 'IMAGE', 'Z': 'IMAGE+TEXT', // We used to glob a bunch of expectations into
"O" as OTHER. Expectations // are more precise now though and it just means
MISSING. 'O': 'MISSING', 'N': 'NO DATA', 'X': 'SKIP' };

version: The version of the JSON format that we're using. This is so we can make
backwards-incompatible changes and know to upgrade the JSON data appropriately.
Currently just hard-coded to 1.

wontfixCounts: Number of tests marked WONTFIX broken down by type of failure
that actually occurred.

deferredCounts: Number of tests marked DEFER broken down by type of failure that
actually occurred.

fixableCounts: Number of tests in test_expectations.txt not marked DEFER/WONTFIX
broken down by type of failure that actually occurred.

fixableCount: Number of tests not marked DEFER/WONTFIX that failed.

allFixableCount: Number of tests we not marked DEFER/WONTFIX (includes expected
passes and failures)

buildNumbers: The build number for this run on the bot.

webkitRevision: The revision of webkit being used.

chromeRevision: The revision of chorme being used.

secondsSinceEpoch: Number of seconds since the epoch (time.time() in python)

TODO(ojan): Track V8/Skia revision numbers as well so we can better track
regressions from V8/Skia.

**For upstream webkit.org** results we can exclude chromeRevision and all the
count/Counts values. The latter are not meaningful if failing tests are skipped.
If that ever changes, then we can change upstream to include these numbers and
we can track number of failures over time.

Sample results.json:

ADD_RESULTS({

"Webkit Mac10.5": {

"wontfixCounts": \[

{

"C": 0,

"F": 105,

"I": 2,

"O": 0,

"T": 1,

"X": 33,

"Z": 11

}

\],

"fixableCounts": \[

{

"C": 2,

"F": 136,

"I": 725,

"O": 1,

"T": 15,

"X": 8665,

"Z": 25

}

\],

"tests": {

"LayoutTests/fast/frames/viewsource-attribute.html": {

"results": \[

\[

751,

"I"

\]

\],

"times": \[

\[

741,

0

\],

\[

10,

2

\]

\]

}

},

"secondsSinceEpoch": \[

1259810345

\],

"fixableCount": \[

904

\],

"allFixableCount": \[

9598

\],

"buildNumbers": \[

"7279"

\],

"webkitRevision": \[

"51615"

\],

"deferredCounts": \[

{

"C": 0,

"F": 56,

"I": 0,

"O": 0,

"T": 0,

"X": 7,

"Z": 0

}

\],

"chromeRevision": \[

"33659"

\]

},

"version": 1

}

expectations.json

ADD_EXPECTATIONS({

"TEST": \[ MODIFIERS_AND_EXPECTATIONS, MODIFIERS_AND_EXPECTATIONS, ... \],

"TEST": \[ MODIFIERS_AND_EXPECTATIONS \]

});

TEST: Same as above in results.json.

MODIFERS_AND_EXPECTATIONS: object of the following form { "modifiers":
MODIFIERS, "expectations": EXPECTATIONS }

MODIFIERS: Space separated string of modifiers. This includes bug numbers
(BUG1234), platforms (LINUX, MAC, etc), build types (debug, release), SKIP,
DEFER, WONTFIX and SLOW.

EXPECTATIONS: The expected results for this test. TIMEOUT, TEXT, IMAGE, PASS,
etc.

**For upstream webkit.org**, we should probably make an expectations file that
parses all the SKIPPED lists and creates the appropriate expectations file.
Something like:

ADD_EXPECTATIONS({

"foo/bar/baz.html", \[ {"modifiers": "SKIP LINUX", "expectations": ""},
{"modifiers": "SKIP WIN", "expectations": ""} \],

...

});
