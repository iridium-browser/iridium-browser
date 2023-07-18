---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/autotest-keyvals
  - Autotest Keyvals
page_name: perf-keyvals
title: Perf Keyvals
---

# Note perf keyvals described in this page has been deprecated. If your test output perf values, please refer to <http://www.chromium.org/chromium-os/testing/perf-data>

# Currently, autotest uses a special kind of keyval called a “perf keyval” to store performance test data. A perf keyval -- like any keyval in autotest -- is simply a key/value pair, where the key is a string that describes the value. The string key can contain only letters, numbers, periods, dashes, and underscores, as defined in write_keyval() in chromeos/src/third_party/autotest/files/client/common_lib/base_utils.py.

Perf keyvals are outputted by autotests via calls to write_perf_keyval(DICT),
where DICT is a dictionary of perf key/value pairs. There is no standardized
format for how a chromeOS performance key string is specified. Unofficially,
perf keys tend to follow the format “UNITS_DESCRIPTION”, as in
“fps_WebGLAquarium”, but this format is not enforced by calls to
write_perf_keyval().

## Life of a perf keyval

Summary: a test outputs perf keyvals using write_perf_keyval(), which are then
pushed to output text files. The autotest infrastructure then pulls these
keyvals from the output files and dumps them into the results database when
parsing server job results.

### From a test to output file(s)

    dictionary of perf keyvals written by a test using
    “self.write_perf_keyval()”. That function is defined in:
    chromium-os/src/third_party/autotest/files/client/common_lib/test.py, in
    class base_test

    That in turn passes the keyvals along to “write_iteration_keyval()”, defined
    in the same class

    write_iteration_keyval() accepts a dictionary of “attr” type keyvals, and
    “perf” type keyvals. It then calls utils.write_keyval() with each
    dictionary, passing along the “attr” and “perf” tags for each dictionary
    appropriately. It then appends a newline to the “keyval” file in the current
    results directory.

    write_keyval() is defined in:
    chromium-os/src/third_party/autotest/files/client/common_lib/base_utils.py.
    This function verifies that the keys in the keyval dictionary match an
    expected regex; then it writes the keyvals in sorted order to the keyval
    file in the specified directory (in “append” mode).

    write_keyval() also accepts a tap_report object; if that is present and its
    “do_tap_report” attribute is true, then the keyvals are also passed to the
    tap_report’s “record_keyval()” function.

    A tap_report object is associated with the job that a given test instance is
    a part of. Hence, it’s referenced by write_perf_keyval() in class base_test
    as “self.job._tap”, which is initialized in file
    “chromium-os/src/third_party/autotest/files/client/common_lib/base_job.py”
    as a new TAPReport object. class TAPReport is defined in the same file.

    TAPReport is autotest’s support for generating TAP ([Test Anything
    Protocol](http://en.wikipedia.org/wiki/Test_Anything_Protocol)) report
    files.

### From output file(s) into the results database

    Autotest’s TKO parser is what puts data into the results database

    parse_test() in chromium-os/src/third_party/autotest/files/tko/models.py
    reads in the keyval file for a test and constructs a new models.test object
    from it.

    parse_test() is invoked by the state_iterator() function in
    chromium-os/src/third_party/autotest/files/tko/parsers/version_0.py (there’s
    also a version_1.py in there). The files in this directory implement a state
    machine that parses/processes the results of an autotest job.

    The state_iterator is driven by process_lines() and end() in
    chromium-os/src/third_party/autotest/files/tko/parsers/base.py.

    process_lines() is driven by _parse_status() in
    chromium-os/src/third_party/autotest/files/server/server_job.py

    When _parse_status() calls process_lines(), it gets back information about
    new tests associated with the job. It then calls __insert_test() with each
    of these tests, which then inserts the test results into the results
    database via a call to self.results_db.insert_test().

    _parse_status() is invoked by the hook in class server_job_record_hook that
    implements the job.record hook for server job.

    The record_hook is invoked in the record_entry() function in class
    status_logger in file
    chromium-os/src/third_party/autotest/files/client/common_lib/base_job.py.
    That function is invoked whenever autotest needs to record a
    status_log_entry into the appropriate status log files.

## Storage of perf keyvals

    “keyval” files: Perf keyvals are initially written into an output file named
    keyval that is contained within the results output folder of an autotest
    run. Each perf keyval is printed on a separate line in this file. An example
    is:
    FPS_FlashGaming{perf}=17.349133
    The format of each line is PERF_KEY_STRING{perf}=PERF_VALUE. The “{perf}” is
    just a tag indicating that the keyval is a performance keyval. Autotests can
    also run for multiple iterations. When that happens, results for all
    iterations are printed inside the same (single) keyval file, with a blank
    line separating the perf keyvals from each iteration.

    autotest results database: perf keyvals are also extracted from the keyval
    output files by autotest’s TKO parser, and are then dumped into the autotest
    results database. The perf keyvals are stored in a table named
    “tko_iteration_result”, with fields “attribute” (the perf key) and “value”
    (the perf value). Each entry in that table is associated with a unique test
    run index, in field “test_idx”. There is a view in the autotest results
    database called “tko_perf_view_2” that simplifies querying for the perf
    keyvals associated with a test run.

## Consumers of perf keyvals

    Consumers of perf keyval files

        Autotest TKO parser (to get the keyvals into the results database)

        team members who are manually inspecting the results of performance test
        runs

    Consumers of perf keyvals in the autotest results database

        Performance dashboard: automatically extracts new perf keyvals daily at
        3am
