---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: android-tests
title: Android Tests
---

## Concepts

*   **Native Unit Tests** - Normal Chromium unit tests based on the
            gtest framework.
*   **Java Unit Tests** - Tests for pure Java code, run on the host
            using the Junit framework. Can use
            [Robolectric](http://robolectric.org/) to emulate Android, and
            [Mockito](http://mockito.org/) to mock or stub classes that are not
            part of the code under test.
    *   These are relatively new, and only exist in a few places so far.
*   **Instrumentation Tests** - Java tests written using the [Android
            Instrumentation Test
            Framework](http://developer.android.com/tools/testing/testing_android.html).
*   **Unit Instrumentation Tests** - Instrumentation tests that only
            tests Java code and not native code. Do not require JNI bindings and
            do not require starting up ContentShell (or another application).
            These should extend InstrumentationTestCase directly.
*   **Integration Instrumentation Tests** - Instrumentation tests that
            tests the entire stack from Java into native code, require JNI
            bindings to be set up and thus require ContentShell (or another
            application) to be started. These typically extend more specialized
            test base classes such as ContentShellTestBase.

## Running Tests

View this guide for running Android tests:
<https://chromium.googlesource.com/chromium/src/+/HEAD/docs/testing/android_test_instructions.md>

## Location of Test Code

As a general rule, tests should live next to the production code and test
utilities should live in a test folder.
In general, this should look like:

*   foo/android/java/src/bar - production code.
*   foo/android/junit/src/bar - Java unit tests
*   foo/android/javatests/src/bar - unit instrumentation tests.
*   foo/test/android/javatests/src/bar/test/util - reusable test
            utilities used to test the foo production code.
*   native unit tests should live next to the source (same as general
            Chromium unit test)

Test utilities that are pure Java and do not depend on content or other Chromium
code should live in:

*   base/test/android/javatests/src/org/chromium/base/test/util

Test utilities in the content layer that are re-used by chrome should live in:

*   content/public/test/android/javatests/src/org/chromium/content/test/util

The location of integration instrumentation tests depends on what they are
testing:

*   If the test is directly testing code in
            content/public/android/java/src, it should live in
            content/public/android/javatests/src (although it might require the
            test base class from content/shell/android/javatests/src).
*   If the test is directly testing code in chrome/android/java/src, it
            should live in chrome/android/javatests/src (although it might
            require the test base class from
            chrome/android/testshell/javatests/src).
*   If the test is a general integration test (i.e. testing general
            functionality that might not be tied directly to specific code), it
            should live in the appropriate javatests folder:
    *   content/shell/android/javatests/src - Content Shell
    *   chrome/android/testshell/javatests/src - Chrome Test Shell
    *   android_webview/javatests/src - [Android
                WebView](/developers/testing/android-tests/android-webview-tests)

Please note that this implies that outside of Content Shell, Chrome Test Shell
and Android WebView, you should:

*   write Java unit tests (if possible) or unit instrumentation tests to
            test your Java code.
*   write native tests for everything else.

## Guidelines for Writing Tests

### TestBase Classes and Test Utilities

*   TestBase classes
    *   should only contain state that all subclasses will need.
    *   should be as small and clean as possible.
    *   should not include utility methods, instead use utility classes.
*   Unit instrumentation test classes should extend
            InstrumentationTestCase and not any other TestBase classes.
*   Utility test classes
    *   are the preferred method of sharing functionality between test
                packages.
    *   should only depend on production code or other test utilities.
    *   should live in foo/test/android/javatests/src/bar/test/util.

### Annotations

Each instrumentation test should have a test size and feature annotation.

#### Test Size Annotations

*   Always annotate a test with one of the test size annotations:
    *   @SmallTest
    *   @MediumTest
    *   @LargeTest
    *   android.test.suitebuilder.annotation.LargeTest/MediumTest/SmallTest
*   Know the size of your test.
    *   In general:
        *   unit Instrumentation tests are small.
        *   integration Instrumentation tests are medium or large.
*   The timeouts for each test size is:
    *   @SmallTest - 1 minute
    *   @MediumTest - 3 minutes
    *   @LargeTest - 5 minutes

#### Feature Annotation

*   The feature annotation is used to specify which feature(s) the test
            is exercising.
*   The list of features can be found in
            [crbug](http://chromegw.corp.google.com/viewvc/chrome-internal/trunk/tools/issue_tracker/labels.txt?view=markup).
*   To add a feature annotation to your test, add the @Feature
            annotation and specify the feature(s) that the test is testing.
    *   For example: @Feature({"Mobile-WebView"})
*   To run the tests for a specific feature, use:
    *   foo/android/junit/src/bar python build/android/test_runner.py
                instrumentation --test-apk ContentShellTest --A
                Feature=Android-WebView,Main

### Flaky and Crashing Tests

If a test is flaky or randomly crashes, the test should **not** run on the build
bots.
The guidelines for handling flaky and crashing tests are as follows:

*   To mark a test disabled, do the following
    *   create a bug for the test/area owner to fix the test.
    *   don't mark a test disabled without creating a bug!!
    *   add the @DisabledTest annotation to the test.
    *   add the relevant package import in the file header, import
                org.chromium.base.test.util.DisabledTest;
    *   add the bug number in a comment to the test.
    *   please follow this example:
        @DisabledTest(message = "crbug.com/XXXXXX").
    *   if the test has any other annotations, do NOT remove these!
    *   if a test fails only on a subset of devices, consider disabling
                it conditionally, like:
        @DisableIf.Build(sdk_is_greater_than = 23, message = "crbug.com/XXXXXX")
