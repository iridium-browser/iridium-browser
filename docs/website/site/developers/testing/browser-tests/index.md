---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/testing
  - Testing and infrastructure
page_name: browser-tests
title: Browser Tests
---

## Introduction

Browser tests are the framework used for integration tests of Chrome. As the
name implies, they run inside the browser process.

## Background

In the beginning, Chrome integration tests were in a ui_tests binary which used
automation to control and inspect a running Chrome. This was problematic because
every time we wanted access to an object in the browser process, we had to add
new automation hooks. It also made debugging hard because two processes had to
be debugged (the test process and the browser process). In response to this,
browser_tests was added. This is a binary which has the Chrome code compiled
into it, as well as test code. The test code runs after browser process
initialization and after a window has been created. Each test runs in a new
browser process, to avoid tests impacting each other.

## Test Binaries

These are the current browser test binaries:

*   `browser_tests`: this is the original browser test binary. It's used
            for testing Chrome features (i.e. autofill, extensions).
*   `interactive_ui_tests`: the name is a holdover, but these are
            browser_tests which run on a bot that has an interactive session.
            They are also not sharded (i.e. run many times in parallel). This is
            needed for a small number of tests which need to generate input
            events from the OS (instead of simulating them internally), care
            about focus, etc...
*   `components_browsertests`: for code in //components
*   `content_browsertests`: this is like browser_tests, except that it's
            for testing features that are at the [Content
            module](/developers/content-module) layer. It's based on Content
            Shell instead of Chrome.
*   `extensions_browsertests`: for code in //extensions
*   `performance_browser_tests`: these are perf tests that run on bots
            with hardware GPUs for more realistic performance characteristics.
            These are not sharded.
*   `weblayer_browsertests`: for code in //weblayer

## Example

To write a browser test, instead of using the `TEST_F` GTEST macro, use
`IN_PROC_BROWSER_TEST_F`, which requires `src/content/public/test/browser_test.h`.

The parent class of the test depends on which binary it's in:

*   `browser_tests` and `interactive_ui_tests` use
            [InProcessBrowserTest](http://src.chromium.org/viewvc/chrome/trunk/src/chrome/test/base/in_process_browser_test.h?revision=HEAD&view=markup)
            (src/chrome/test/base/in_process_browser_test.h)
*   `content_browsertests` use
            [ContentBrowserTest](http://src.chromium.org/viewvc/chrome/trunk/src/content/test/content_browser_test.h?view=markup)
            (src/content/test/content_browser_test.h)

Both browser tests in `src/content` and `src/chrome` have access to a collection of
test utilities for browser tests in
[src/content/public/test/browser_test_utils.h](https://cs.chromium.org/chromium/src/content/public/test/browser_test_utils.h).
This includes things like simulating input events, executing JavaScript and
getting the result, modifying cookies etc... This is in addition to test
utilities available to all tests (browser tests and unit tests) that are in
[src/content/public/test/test_utils.h](https://cs.chromium.org/chromium/src/content/public/test/test_utils.h).
This includes things like running nested message loops.

Also include `testing/gtest/include/gtest/gtest.h`, rather than relying on
browser_test.h including it for you.

Browser tests in `src/content` and `src/chrome` have their own utility headers for
interacting with the window, to add helpers for waiting for a new window to be
added, navigating, getting the location of a test file etc.. Tests in `src/chrome`
use
[src/chrome/test/base/ui_test_utils.h](https://cs.chromium.org/chromium/src/chrome/test/base/ui_test_utils.h),
while tests in src/content use
[src/content/public/test/content_browser_test_utils.h](https://cs.chromium.org/chromium/src/content/public/test/content_browser_test_utils.h).

An example test in browser_tests would like:

```c++
IN_PROC_BROWSER_TEST_F(InProcessBrowserTest, Foo) {
  GURL test_url = ui_test_utils::GetTestUrl(test_dir, test_file);
  ui_test_utils::NavigateToURL(browser(), test_url);
  content::WebContents\* web_contents = ...
  content::SimulateMouseClick(web_contents);
  // Inspect C++ objects and modify them now.
}
```

In general, existing tests provide the best examples on how to write a new test.
Look at the `browser_tests` and `interactive_ui_tests` targets in the [chrome/test
build
file](https://chromium.googlesource.com/chromium/src/+/HEAD/chrome/test/BUILD.gn),
or the content_browsertests target in [content/test build
file](https://chromium.googlesource.com/chromium/src/+/HEAD/content/test/BUILD.gn)
to see a list of tests.

## Debugging

Per above, each test runs in a new browser process. To aid in debugging, you can
start the browser test binary with `--single-process-tests` flag (along with
`--gtest_filter=Foo.Bar`). That way you can attach to the process that just got
created, which will contain the test code. In this mode, make sure the filter
only executes one test. Running more than one test will likely result in
undefined behavior because the test fixture does not attempt to reinitialize any
global state.

To prevent the test from timing out while debugging, try playing with switches
like `--ui-test-action-max-timeout=1000000 --test-launcher-timeout=1000000`.
Although the test tries not to time out while a debugger is attached, if you use
`--renderer-startup-dialog` to attach a debugger to the renderer process, the test
can time out in the meantime.

Browser tests do not show pixel output by default, i.e. only a blank white
window is shown. Use `--enable-pixel-output-in-tests` to change this. If you
prefer to hide the window completely, follow [these
instructions](https://chromium.googlesource.com/chromium/src/+/main/docs/linux/debugging.md#to-replicate-window-manager-setup-on-the-bots)
to setup a virtual display using Xvfb and openbox and then set DISPLAY
environment variable to redirect pixel output.

In case you are debugging JavaScript browser tests (e.g. tests defined in
[cr_settings_browsertest.js](https://source.chromium.org/chromium/chromium/src/+/HEAD:chrome/test/data/webui/settings/cr_settings_browsertest.js)),
it can be helpful to add debugger; statements and to pass
`--auto-open-devtools-for-tabs`. This way the browser test will automatically halt
when the `debugger;` statement is hit, allowing you to inspect the execution state
via DevTools.

## Customizing The Test Harness

To allow more customization (such as changing the command line flags of the
browser process, i.e. to enable features that are off by default), the test
fixture can derive from InProcessBrowserTest or ContentBrowserTest and override
some methods. See their shared base class,
[BrowserTestBase](https://chromium.googlesource.com/chromium/src/+/HEAD/content/public/test/browser_test_base.h),
for more information

## Tests Spanning Restarts

A test can span a restart of the browser process. This is useful in testing that
something persisted, as an example. To do this, use the PRE prefix:

```c++
IN_PROC_BROWSER_TEST_F(InProcessBrowserTest, PRE_Foo) {
  // Do something.
}
```

```c++
IN_PROC_BROWSER_TEST_F(InProcessBrowserTest, Foo) {
  // Verify something was done.
}
```

Each test above was run in a separate browser process, but the data directory of
the profile was the same.

## Tests That Don't Run By Default

Something that often comes up is that a team wants to run a browser test based
binary on their own bots. They can create a new binary, but that slows down the
build for all bots. Instead, the tests should be put in browser_tests or
content_browsertests with a MANUAL_ prefix. That way the test is compiled into
the same binary that's used for all tests, but doesn't run by default on bots.
It's only used when `--run-manual` is passed. The team's bots can then run
browser_tests with `--run-manual --gtest_filter=FooTeam\*` .

**Running Tests**

```bash
ninja -C out/Release browser_tests
./out/Release/browser_tests
```

Works, but can take an hour or more, even on a fast machine.

`browser_tests` also has lots of options which you can see via `--help`. Most useful
are:

*   `--test-launcher-bot-mode` used by the bots and is the recommended way to run
browser_tests
*   `--test-launcher-jobs=20` for manually controlling how many jobs are launched
*   `--gtest_filter=Foo.\*` for only running a subset of tests

For example, on linux use the following:

```bash
xvfb-run -s "-screen 0 1024x768x24" ./out/Default/browser_tests
--test-launcher-bot-mode
```

For more info, we have an entire separate page to explain how to run
[browser_tests on specific
platforms](http://www.chromium.org/developers/testing/running-tests#TOC-Running-basic-tests).

## Networking

To avoid flakiness tests are not allowed to access the network. This is enforced
by content::BrowserTestBase::SetUp() through its instantiation of
content::TestHostResolver.

Some tests may need to use multiple origins to test their code paths; one common
way this is done is to redirect all origins to localhost:
host_resolver()-&gt;AddRule("\*", "127.0.0.1")

A small number of tests exercise code paths that make requests to Google domain.
Due to the included [HSTS preload list](https://hstspreload.org/) in Chrome's
networking code, your test will need to use an EmbeddedTestServer that's
configured for https. Additionally, you will need two more switches:

1.  `switches::kIgnoreCertificateErrors`: needed since the test server
            only has a valid certificate for localhost
2.  `switches::kIgnoreGooglePortNumbers`: needed since production code
            only allows known ports (80/443) for Google domains
