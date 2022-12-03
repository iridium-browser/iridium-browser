---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/depottools
  - Using depot_tools
page_name: presubmit-scripts
title: Presubmit Scripts
---

[TOC]

### Overview

`git cl` and `git-cl` will check for and run presubmit scripts before you upload
and/or commit your changes. Presubmit scripts are a way of applying automatic
verification to your changes before they are reviewed and/or before they are
committed.

Presubmit scripts can perform automated checks on the files in your change and
the description of your change, and either fail your attempt to upload or
commit, show a warning that you must acknowledge before uploading/committing, or
simply show an informational message as part of the output of `git cl`.

Examples of things presubmit scripts may be useful for include:

*   Ensuring your change is linked to a bug via a BUG= line.
*   Ensuring you have run
            [cpplint.py](http://src.chromium.org/viewvc/chrome/trunk/tools/depot_tools/cpplint.py?view=markup)
            or automatically running cpplint for you.
*   Enforcing coding conventions.
*   Preventing you from breaking dependency rules e.g. by including a
            header that code in your directory is not supposed to depend upon.
*   Warning you if you haven't built your client and/or run unit tests
            within the last X hours.

Generally you want the same tests for upload and committing, but there are
exceptions.

Firstly, the commit bot can autofix various problems, so these problems only
need to be checked on commit (to check that they have been fixed), not on upload
(another good reason to use the commit queue!). Secondly, you can skip slow
tests on upload and only run them on commit, if running these every time slows
development too much, but this can result in problems only being caught at the
last minute, and thus requiring last-minute changes after you think they're good
to go.

### Bypassing tests

To skip the scripts on upload, use the `--bypass-hooks` flag, as in:

```none
git cl upload --bypass-hooks
```

To skip the scripts on commit, use --bypass-hooks and [directly
commit](/developers/contributing-code/direct-commit) your change.

You should only do these if necessary, as the presubmit scripts are there for a
reason, but they're not perfect.

If you have trouble with a presubmit script, it's preferable to *fix it,* rather
than simply bypassing it. See [depot_tools: sending
patches](http://www.chromium.org/developers/how-tos/depottools#TOC-Sending-patches)
for how to contribute.

If you're not sure which presubmit check is generating results you can request
double-verbose mode with `git cl presubmit -v -v`. This will trigger noisy
diagnostics, which append a call stack to each presubmit result.

### Design

When you run `git cl upload` or `git cl commit`, `git cl` will look for all
files named `PRESUBMIT.py` in folders enclosing the files in your change, up to
the repository root.

For each such file, it will load the file into the Python interpreter and call
the appropriate Check functions, depending on what presubmit version you have
selected (details below) and whether you are committing or uploading.

The same applies to `git-cl upload`, `git-cl dcommit` and `git-cl push`.

Please note that presubmit scripts are a best-effort kind of thing; they do not
prevent users from submitting without running the scripts, since one can always
dcommit, and in fact there is a --bypass-hooks (formerly `--no_presubmit`) flag
to git cl that skips presubmit checks. Further, since they use the local copy of
the `PRESUBMIT.py` files, users must sync their repos before the latest
presubmit checks will run when they upload or submit.

More subtly, presubmit scripts do not guarantee invariants: even if presubmit
scripts pass prior to submission to CQ, once all changes land, the scripts may
fail! This is because 2 changes may individually pass the tests, and the patches
both apply cleanly together, but the combined change does not pass tests. Since
presubmit/precommit scripts run at upload or at start of CQ steps, if two such
changes are in the CQ at the same time, they can both pass, both be enqueued,
and both land, at which point the tests start failing. A common example is
change 1 adding a new test, and change 2 changing existing tests. After they
both land, there is a new test in the old style (from change 1), which is out of
sync with the new tests (from change 2).

Another very common way that presubmit failures can land is when a PRESUBMIT or
associated settings are changed. Because presubmits generally only check the
files in the change and because presubmits and their settings affect many other
files, a disconnect can easily occur. For instance, the `.eslintrc.js` files
control what warnings are enabled in .js and .ts files. However, the presubmits
only check .js and .ts files when they are modified, so a change that enables a
warning in `.eslintrc.js` can pass presubmits even if hundreds of files will
trigger the warning. Ideally, developers will run `git cl presubmit --files "*.js;*.ts"`
to check these, but this is not enforced. In general, modifying presubmits is
the most common way to introduce latent presubmit failures. Occasionally running
`git cl presubmit --all` and looking for errors is the only reliable way to find
these.

### Writing tests

To create a new test, either create a new PRESUBMIT.py script or edit an
existing one, adding a new function for your test.

To check your changes, first commit locally (else `git cl` will complain about
the dirty tree), then:

To test the upload checks:

```none
git cl presubmit --upload
```

To test the submit checks:

```none
git cl presubmit
```

#### **Version 1**

In the original presubmit system the functions must match these method
signatures. You do not need to define both functions if you're only interested
in one type of event, and if you want to run the same tests in both events, just
have them both call a single underlying function:

```none
def CheckChangeOnUpload(input_api, output_api):
    CommonChecks(input_api, output_api)
def CheckChangeOnCommit(input_api, output_api):
    CommonChecks(input_api, output_api)
```

The `input_api` parameter is an object through which you can get information
about the change. Using the `output_api` you can create result objects.

Both `CheckChangeOnXXX` functions must return a list or tuple of result objects,
or an empty list or tuple if there is nothing to report. The types of result
objects you may use are `output_api.PresubmitError` (a critical error),
`output_api.PresubmitPromptWarning` (a warning the user must acknowledge before
the command will continue) and `output_api.PresubmitNotifyResult` (a message
that should be shown). Each takes a message parameter, and optional "items" and
"long_text" parameters.

#### **Version 2**

Presubmit version 2 reduces some of the overhead of managing which checks are
executed on upload and/or submit. To enable presubmit version 2, in the global
scope of your presubmit script, define:

```none
PRESUBMIT_VERSION = '2.0.0'
```

In presubmit version 2, you are not required to use `CheckChangeOnUpload` and
`CheckChangeOnCommit` as your entry points. Any function that begins with the
prefix `Check` will be executed, receiving `input_api` and `output_api` as its
parameters. `CheckXXX` functions that end in `Upload` will only be executed on
upload and `CheckXXX` functions that end in `Commit` will only be executed on
commit. `CheckXXX` functions that do not end in either suffix will be executed
at both upload and commit. The format of return values of `CheckChangeOnXXX`
functions is the same as for `CheckChangeOnUpload/CheckChangeOnCommit`. This
makes existing presubmit scripts backwards compatible with presubmit version 2
provided there are no functions in the global scope of the script that begin
with `Check`. Sample functions might look like this:

```none
def CheckSomeInvariant(input_api, output_api)
    DoSomething()
def CheckSomeInvariantUpload(input_api, output_api)
    DoSomething()
def CheckSomeInvariantCommit(input_api, output_api)
    DoSomething()
```

#### InputApi

The `input_api` parameter is an object of type `presubmit.InputApi`; see [`class
InputApi`](https://source.chromium.org/chromium/chromium/tools/depot_tools/+/main:presubmit_support.py?q=file:presubmit_support.py%20%22class%20InputApi%22)
in
[`presubmit_support.py`](https://source.chromium.org/chromium/chromium/tools/depot_tools/+/main:presubmit_support.py)
for implementation.

This object can be used to transform from local to repository paths and vice
versa, and to get information on the files in the change that are contained in
the same directory as your `PRESUBMIT.py` file or subdirectories thereof.

The `input_api.change` object represents the change itself. Using this object
you can retrieve the description of the change, any key-value pairs in the
description (e.g. BUG=123), and details on all of the files in the change (not
just the ones contained by the directory your `PRESUBMIT.py` file resides in).

An `input_api.canned_checks` object contains a set of ready-made checks that you
can easily add to your presubmit script.

The `os/os.path` module is available as `input_api.os_path`, so you do not need
to import this yourself.

Files in the API are represented by an `AffectedFile` object through which you
can query the `LocalPath()`, `ServerPath()`, and the `Action()` being performed
('A', 'M' or 'D' for Add, Modify or Delete).

The `input_api.is_committing` attribute indicates whether the CL is being
committed or just uploaded. This is particularly useful if you wish the same
test to be run for both upload and committing, but with different behavior. A
common pattern is to prompt a warning on upload, but an error on committing,
which allows a CL to be uploaded and reviewed even if the test fails, but not
committed (without dcommit). This can be done as follows:

```none
if input_api.is_committing:
  message_type = output_api.PresubmitError
else:
  message_type = output_api.PresubmitPromptWarning
```

### Caveats

It is possible to run arbitrary Python code in the presubmit scripts. To avoid
side effects and hard-to-debug errors, it is safest to run tests in
subprocesses. The InputApi object provides various facilities to assist with
this; see example below.

Please note that you should not use any functions or attributes on the API
objects that begin with an underscore (_) as these are private functions that
may change, whereas all public functions will be retained through future
versions of the API.

### Details and Example

The most detailed documentation for the presubmit API is in its [implementation
code](https://cs.chromium.org/chromium/tools/depot_tools/presubmit_support.py).

The [canned
checks](https://source.chromium.org/chromium/chromium/tools/depot_tools/+/main:presubmit_canned_checks.py)
are good examples of what you can do with the presubmit API.

An example simple file might be as follows:

```none
# Optional but recommended
PRESUBMIT_VERSION = '2.0.0'

# Mandatory: run under Python 3
USE_PYTHON3 = True

def CheckChange(input_api, output_api):
    results = []
    results += input_api.canned_checks.CheckDoNotSubmit(input_api, output_api)
    results += input_api.canned_checks.CheckChangeHasNoTabs(input_api, output_api)
    results += input_api.canned_checks.CheckLongLines(input_api, output_api)
    # Require a BUG= line and a HOW_TO_TEST= line.
    if not input_api.change.BUG or not input_api.change.HOW_TO_TEST:
        results += [output_api.PresubmitError(
            'Must provide a BUG= line and a HOW_TO_TEST line.')]
    return results
```

However many of the canned checks, such as `ChecksCommon` and `CheckLongLines`,
are called from the root-level `PRESUBMIT.py` (either directly or through
`PanProjectChecks`) and therefore needn't be called from other Chromium
presubmit scripts.

A simple example of a custom command is:

```none
def CheckMyTest(input_api, output_api):
  test_path = input_api.os_path.join(input_api.PresubmitLocalPath(), 'my_test.py')
  cmd_name = 'my_test'
  cmd = [input_api.python3_executable, test_path]
  test_cmd = input_api.Command(
    name=cmd_name,
    cmd=cmd,
    kwargs={},
    message=output_api.PresubmitPromptWarning)
  if input_api.verbose:
    print('Running ' + cmd_name)
  return input_api.RunTests([test_cmd])
```

Alternatively, you can set `cmd = [test_path]` and then pass `python3=True` to
run the script under Python 3. `input_api.python_executable` (runs Python 2) is
deprecated and should not be used in new presubmits.

You can look at existing scripts for examples ([search:
PRESUBMIT.py](https://code.google.com/p/chromium/codesearch#search/&q=file:PRESUBMIT.py&sq=package:chromium&type=cs)).
[Chromium's
PRESUBMIT.py](http://src.chromium.org/viewvc/chrome/trunk/src/PRESUBMIT.py?view=markup)
and [Chromium-os
PRESUBMIT.py](http://src.chromium.org/cgi-bin/gitweb.cgi?p=chromium.git;a=blob;f=PRESUBMIT.py;hb=HEAD)
are detailed examples, while the [Blink bindings
PRESUBMIT.py](https://code.google.com/p/chromium/codesearch#chromium/src/third_party/WebKit/Source/bindings/PRESUBMIT.py)
is a very simple example.
