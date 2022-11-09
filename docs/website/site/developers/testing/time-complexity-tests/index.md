---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: time-complexity-tests
title: Time Complexity Tests
---

[TOC]

**HISTORICAL (documentation)**

**Removed in [crrev.com/1149253002](http://crrev.com/1149253002)
([crbug.com/488986](http://crbug.com/488986)).**

Time complexity tests verify the order of growth of time complexity *T*(*n*) for
various operations, generally verifying that this is *O*(1) or *O*(*n*), rather
than *O*(*n*2), say.

These are a form of performance test (see [Adding Performance
Tests](/developers/testing/adding-performance-tests)), but since they have a
binary answer (satisfies the bound or doesn't), and we are concerned with the
overall growth, not the absolute speed, they are instead technically in the
Blink [Layout Tests](/developers/testing/webkit-layout-tests), and produce
PASS/FAIL.

These are **potentially flaky,** and layout test failures **break the CQ,** so
*please take extra care in testing and committing these.*

Tests consist of:

*   Description of what you are testing, and relevant bug or bugs.
*   Function whose runtime you are testing.
*   Test parameters for what range you assert the growth rate holds, and
            with what tolerance.

### Test format

These tests are written in JavaScript, and stored in the `LayoutTests/perf`
directory in the Blink tree (as of 2013-April). The library that these tests use
is
`[LayoutTests/resources/magnitude-perf.js](http://src.chromium.org/viewvc/blink/trunk/LayoutTests/resources/magnitude-perf.js)`.

Tests take this form:

```none
<!DOCTYPE html>
<script src="../resources/magnitude-perf.js"></script>
<script>
if (window.testRunner)
    testRunner.dumpAsText();
function setupFoo(magnitude)
{
    // ...
}
function testFoo(magnitude)
{
    // ...
}
Magnitude.description('Verifies that fooing is linear in the number of bars. See http://crbug.com/42');
Magnitude.initialExponent = 0;
Magnitude.numPoints = 10;
Magnitude.trim = 1;
Magnitude.tolerance = 0.10;
Magnitude.run(setupFoo, testFoo, Magnitude.LINEAR);
</script>
```

You can have several tests in the same file (they should be related); simply
have several `run` commands, each of which can have different parameters.
[`class-list-remove.html`](http://src.chromium.org/viewvc/blink/trunk/LayoutTests/perf/class-list-remove.html)
is a good example with two tests in one file.

### Test parameters

You can set some test parameters, specifying what range of magnitudes to test
and how strict the tests should be. This is optional, but generally necessary.
This is because the tests default to quite strict tolerances, and start testing
from *n* = 1, which is usually too low, since constant terms often dominate for
low *n*.

The design reason for requiring parameters is to make the data of "over what
range and to what tolerance the growth rate holds" an intrinsic part of the
test: by specifying these parameters, someone reading the test file can see at a
glance how big input needs to be to be "close enough to infinity", and how noisy
the data is.

You can also specify more trials, or a different cutoff for overall success, but
this is generally not necessary.

#### Calibration procedure

To set the parameters:

*   figure out size – over what range is the growth rate valid: from 20
            to 210? 28 to 212?
*   figure out noisiness and tolerance – how many outliers are trimmed
            before computing the statistic (`trim`), and what percent deviation
            from a perfect fit is allowed (`tolerance`)?
*   test locally and on [try bots](/developers/testing/try-server-usage)
*   revise parameters
*   test again
*   You're shooting for parameters that are lax enough to never fail
            with current code and bots, but that will fail if code regresses

When committing, safest is to:

*   Initially list the test in TestExpectations as flaky (even after
            using your best guesses for parameters; can use bug number for which
            you are writing the test in below):

```none
crbug.com/123 [ Release ] perf/foo.html [ Pass Failure ]
```

*   ...then keep an eye on it in the [flakiness
            dashboard](/developers/testing/flakiness-dashboard) (specifically,
            filter by
            [^perf/](http://test-results.appspot.com/dashboards/flakiness_dashboard.html#tests=%5Eperf%2F),
            or more narrowly your specific test) for a few days after
            committing.
*   If there are flaky failures, post a follow-up patch updating
            parameters.
*   Once reliably passes, post a final patch removing the line from
            TestExpectations, turning on the test for real.

It is ok, and often a good idea, to add a bit of a margin of error (or extra
test runs) if you're concerned about false alarms, but be careful to not make
the test so lax that it always passes. The test should fail if something changes
for the worse; that's what it's there for.

#### Common parameters

You will primarily vary the parameters `initialExponent, numPoints, trim,` and
`tolerance.` For example:

```none
Magnitude.initialExponent = 0;
Magnitude.numPoints = 10;
Magnitude.trim = 1;
Magnitude.tolerance = 0.10;
```

This means:

*   start with **initial** *n* = 20;
*   run for 10 **points** (magnitudes), so up to *n* = 20 + (10 − 1) =
            29;
*   **trim** off the 1 most extreme value (we use [trimmed
            estimators](http://en.wikipedia.org/wiki/Trimmed_estimator), meaning
            "discard some outliers", to deal with noise);
*   succeed if data is within 10% **tolerance** of perfect fit.

#### Full list

The parameters are as follows:

*Range of magnitudes*

*   `Magnitude.initialExponent` – start at *n* = 2*k* (initialExponent)
*   `Magnitude.numPoints` – run for 2*k*, 2*k* +1, 2*k* + 2, ... 2*k* +
            (numPoints − 1)

*You will usually need to modify these, as for low magnitudes low order terms
(especially constants) often dominate, obscuring the high order terms (linear,
quadratic, exponential) that we're interested in, but we don't want to test for
220 to 230, say, both because this is slow, and has noise from memory management
once you get this big. Usually around 28 to 215 is a good range, but it varies.*

*Overall success*

*   `Magnitude.numTrials` – run test (for whole range of magnitudes)
            numTrial times
*   `Magnitude.successThreshold` – percentage of trials that must
            succeed for overall test to succeed

Usually leaving these at defaults (3 trials, 50% success threshold) is fine, and
instead modify statistical parameters (trim and tolerance).

You generally want an odd number of trials, so you can use majority rule:

> "Never go to sea with two chronometers; take one or three."

> > (quoted in Fredrick Brooks, *The Mythical Man-Month*, p. 64),

and 3 trials is usually enough.

50% (majority rule) optimizes the overall trade-off between [sensitivity and
specificity](http://en.wikipedia.org/wiki/Sensitivity_and_specificity) (true
positive rate and true negative rate). If you have the same rate for both, the
complimentary rates (false negative rate and false positive rate) are attenuated
linearly (as powers, i.e., on the log scale) in number of trials needed for
success/failure. By "linear attenuation" we mean like "decreasing decibels of
noise": if false negative rate is *α* and false positive rate is *β,* then
requiring *j* successes for overall success or *k* = (*n* − *j*) + 1 failures
for overall failure makes the false negative rate approximately (ignoring
constant factor) *αj* and false positive rate approximately *βk*, which in log
terms is *j* log *α* and *k* log *β*. For *n* = 3 and requiring 2 successes for
overall success (hence 2 failures for overall failure), this doubles the
attenuation of the false negative and false positive rate, which is usually
enough.

In rare circumstances you may want to change these:

*   Increase numTrials to increase power if either high false negative
            rate or high false positive rate;
*   decrease successThreshold if very noisy test (lacks specificity), so
            2 successes out of 5 trials enough, say;
*   increase successThreshold if test not sensitive enough but very
            reliable, so 4 out of 5 trials needed, say.

It's ok to use an even number of trials if you change the success threshold,
since you no longer want the same trade-off between sensitivity and specificity.

*Run time*

*   `Magnitude.millisecondsPerRun` – how long each run (at a specific
            magnitude *n*) runs

At each magnitude, the test is run until this time is used up, then averaged.
You can increase this if you have a slow test, or at running at high magnitudes
where you're not getting enough iterations to get a good average. Consider if
you can instead decrease the maximum magnitude.

*Statistical parameters*

*   `Magnitude.trim` – number of extreme values to trim ([trimmed
            estimator](http://en.wikipedia.org/wiki/Trimmed_estimator))
*   `Magnitude.tolerance` – succeed if data is within this percentage of
            model (perfect fit); formally, if maximum deviation is less than
            this threshold.

Given the times for all magnitudes, the test statistic (see below) is computed
by first trimming the most extreme values (basically outliers in the times,
actually derived values like ratio between successive times), then computed and
checked against the tolerance. These default to quite strict values – no trim
and 5% tolerance – which are quite safe, but usually too strict. You will thus
need to relax them and specify how far from model the actual data is. Common
reasonable values include:

```none
trim = 1; tolerance = 0.10;
trim = 2; tolerance = 0.20;
```

Beware of trimming too much, as you're effectively reducing `numPoints`!

Larger tolerances are ok when testing for *O*(1) – even values of 100% or more
are ok if you're only interested in checking that it's not linear or worse
(rather than not log) – but if you start getting to 50% (at which point you're
not rejecting *O*(log *n*)), you have very noisy data.

### Interpreting output

For linear and polynomial tests – *O*(*n*) and *O*(*n*2) – the key numbers to
look at are the log-ratios (base 2), which are effectively the exponent of the
order of growth at that step. Thus for linear growth this should be almost 1,
while for quadratic or greater growth it should be 2 or more.

For example, given the following data:

```none
log-ratios (base 2):
0.44961898695576596,1.4623608881897614,2.432344314952156,2.5037162001090283,1.9487150784480256,1.2812301235421082
Maximum Absolute Deviation: 1.5037162001090283
```

...rounding yields:

```none
log-ratios (base 2):
0.45,1.46,2.43,2.50,1.95,1.28
Maximum Absolute Deviation: 1.50
```

...so we see that at first it grows slowly (.45), then super-linearly (1.46),
then superquadratically (2.43, 2.50, 1.95 is close), then slows down but still
superlinear (1.28), and overall it deviates substantially from linear (2.50
differs from ideal of 1.00 by 1.50, which is the Maximum Absolute Deviation).
This is an extreme case (normally there'll be a trend), but shows that this
growth isn't linear over this range of inputs.

### Testing procedure

The code at
`[magnitude-perf.js](http://src.chromium.org/viewvc/blink/trunk/LayoutTests/resources/magnitude-perf.js)`
is of course the ultimate reference.

You can get very detailed diagnostics and intermediate calculations by running
in the TestRunner (?), or setting an invalid expectation (e.g., expecting
CONSTANT for data you expect to be LINEAR).

In outline, testing proceeds as follows.

We time the run time of the tested function at exponentially spaced magnitudes,
e.g., 20, 21, 22, 23, ....

We then verify if these times look like the time complexity we're expecting
(constant, linear, or polynomial (quadratic or greater)).

Tests are [robust](https://en.wikipedia.org/wiki/Robust_statistics),
[non-parametric](https://en.wikipedia.org/wiki/Non-parametric_statistics)
statistical tests, since timing is noisy (so need to be robust), and noise can
take various forms (so non-parametric, since no particular model of noise).

In more detail:

*   Perform the test (across all magnitudes) numTrials number of times.
*   During each trial, for each magnitude, compute the run time.
*   To compute the run time, run the test function repeatedly, until
            millisecondsPerRun is up, then divide total run time by number of
            iterations completed.
*   Check that we haven't hit a delay at the end of the test run (if
            overrun allotted time by more than a full run time), and if so,
            retry a few times to reduce noise (if possible)
*   Given the data, perform a statistical test (see below), returning
            either success or failure.
*   After all trials are done, check if ratio of successes meets
            successThreshold, and return overall PASS or FAIL.

#### Statistical tests

Tests are all of the form: "use
[trimmed](http://en.wikipedia.org/wiki/Trimmed_estimator) [maximum absolute
deviation](http://en.wikipedia.org/wiki/Maximum_absolute_deviation) (from model)
as [test statistic](http://en.wikipedia.org/wiki/Test_statistic)", meaning
"compare this value against tolerance, and succeed if error doesn't exceed
tolerance". In detail:

*   Compute absolute deviations from model (e.g., for constant, compare
            each time against the median; ideally these would all be equal) –
            this is the first non-parametric part
*   Trim: discard `trim` number of extreme values – this is the robust
            part
*   Compare max of remaining to `tolerance` – this is the second
            non-parametric part – and pass if and only if within tolerance.

For constant, this compares each value against the median.

(If deviation from median is only 20% or 30% as you vary *n* from 1 to 1,024,
any linear or higher term is very small.)

For linear and polynomial (quadratic or more), at each step, check what doubling
the input does to the time – for linear it should double it, while for
polynomial it should quadruple it or more. Taking log (base 2), this says the
log of ratios of successive times (equivalently, difference of successive logs)
should be 1 (linear) or more than 2 (quadratic or more).

This is effectively looking at slope of successive steps on the [log-log
scale](http://en.wikipedia.org/wiki/Log-log_plot), as the slope of a monomial on
the log-log scale is its exponent.

These are simple tests, not sophisticated ones (we're not doing robust
non-parametric linear regressions on the log-log scale, say), but they are thus
transparent, and should be sufficient. More sophisticated tests that are still
easily implemented include doing Ordinary Least Squares (OLS) on the log-log
scale, and doing this iteratively (discarding point with greatest residual and
repeating).

### Caveats

When writing tests, watch out for:

*   **too strict** parameters – yields false positives, setting off
            false alarms and breaking the commit queue
*   **too lax** parameters – yields false negatives, missing regressions
*   **slow tests** – these tests are inevitably somewhat slow, because
            they're asymptotic behavior (hence large values of input); avoid
            *unnecessary* slowness (can you test up to a smaller maximum value?)
*   **slow setup** – beyond the test itself, the setup can cause
            overhead. E.g., when testing for *O*(*n*) growth, it's easy to do
            *O*(*n*) work *n* times during setup, which yields a test with
            *O*(*n*2) startup time!
*   **large magnitudes** – at large magnitudes (often around 215 or
            220), memory management starts to add measurable noise, specifically
            V8 garbage collection. This is often small relative to linear growth
            though, but can be significant when checking for constant growth.
*   **noisy tests** – some functions are simply noisy; if noise is
            constant or small relative to time of test, signal-to-noise
            increases and noise is small at large magnitudes. However, when
            checking for *O*(1) performance, noise isn't offset by growing
            signal and may even increase with increasing magnitude.
