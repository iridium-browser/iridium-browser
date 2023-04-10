---
breadcrumbs:
- - /Home
  - Chromium
- - /Home/domui-testing
  - DOMUI Testing
page_name: webui-browser_tests
title: WebUI browser_tests
---

***Note: This page is out of date. If you are using modern practices, you are
using a BrowserProxy for your JS/C++ interactions. The
[BrowserProxy](https://docs.google.com/document/d/1c20VYdwpUPyBRQeAS0CMr6ahwWnb0s26gByomOwqDjk/edit)
document covers some testing practices for that case.***

[TOC]

~~### Problem~~

~~See Also [domui-testing](/Home/domui-testing).~~

~~WebUI contains Javascript, which runs in the renderer and a C++ handler, which
runs in the UI thread of the browser process. While this is a necessary part of
the design, testing across these boundaries of both language and process/thread
is cumbersome at best.~~

~~### Objective~~

~~Make it possible to test the Javascript portion of WebUI in Javascript:~~

*   ~~Write WebUI tests in Javascript.~~
*   ~~Run in browser_tests so that the page is loaded in a real chrome
            browser.~~
*   ~~Allow WebUI handlers to be mocked in Javascript.~~

~~### Solution~~

~~The solution comes in the following parts:~~

*   ~~Support libraries: `chrome/test/data/webui/test_api.js`,
            `chrome/third_party/mock4js/mock4js.js`.~~
*   ~~gyp rules for js2webui generator feeding results into
            browser_tests.~~
*   ~~WebUIBrowserTest C++ class in
            `chrome/browser/ui/webui/web_ui_browsertest*` .~~
*   ~~Test source files next to the implementation or in
            chrome/test/data/webui/ - see
            chrome/browser/ui/webui/options/options_browsertest.js and
            chrome/test/data/webui/print_preview.js for reference.~~

~~### How to write a test~~

~~The best reference examples are chrome/test/data/webui/print_preview.js and
chrome/browser/ui/webui/options/options_browsertest.js~~

*   ~~#### \[maybe\] create a new test file~~

    *   ~~Create new file `chrome/test/data/webui/`*`mytest`*`.js`~~
    *   ~~Add `chrome/test/data/webui/`*`mytest`*`.js` to the sources
                for browser_tests in `chrome/chrome_tests.gypi`~~

*   ~~#### Write a test fixture, defining the page to browse to:~~

~~```none
/**
 * TestFixture for OptionsPage WebUI testing.
 * @extends {testing.Test}
 * @constructor
 **/
function OptionsWebUITest() {}
OptionsWebUITest.prototype = {
  __proto__: testing.Test.prototype,
  /**
   * Browse to the options page & call our preLoad().
   **/
  browsePreload: 'chrome://settings-frame',
  // ...
};
```~~

*   ~~#### Mock the Javascript handler:~~

~~```none
OptionsWebUITest.prototype = {
...
  /**
   * Register a mock handler to ensure expectations are met and options pages
   * behave correctly.
   **/
  preLoad: function() {
      this.makeAndRegisterMockHandler(
            ['defaultZoomFactorAction',
             'fetchPrefs',
             'observePrefs',
             'setBooleanPref',
             'setIntegerPref',
             'setDoublePref',
             'setStringPref',
             'setObjectPref',
             'clearPref',
             'coreOptionsUserMetricsAction',
            ]);
      // Register stubs for methods expected to be called before/during tests.
      // Specific expectations can be made in the tests themselves.
      this.mockHandler.stubs().fetchPrefs(ANYTHING);
      this.mockHandler.stubs().observePrefs(ANYTHING);
      this.mockHandler.stubs().coreOptionsUserMetricsAction(ANYTHING);
  },
...
};
```~~

*   ~~#### Mock stubs which call a function:~~

~~```none
    mockHandler.stubs().getDefaultPrinter().
        will(callFunction(function() {
          setDefaultPrinter('FooDevice');
        }));
```~~

*   ~~#### Define a test using mock expectations:~~

~~```none
TEST_F('OptionsWebUITest', 'testSetBooleanPrefTriggers', function() {
  var showHomeButton = $('toolbarShowHomeButton');
  var trueListValue = [
    'browser.show_home_button',
    true,
    'Options_Homepage_HomeButton',
  ];
  // Note: this expectation is checked in testing::Test::TearDown.
  this.mockHandler.expects(once()).setBooleanPref(trueListValue);
  // Cause the handler to be called.
  showHomeButton.click();
  showHomeButton.blur();
});
```~~

*   ~~#### Conditionally run a test using generated c++ ifdefs:~~

~~#### See [Handling a failing test](/developers/tree-sheriffs/handling-a-failing-test) for more details on style and how/when to disable a test.~~

~~```none
// Not meant to run on ChromeOS at this time.
// Not finishing in windows. http://crbug.com/81723
GEN('#if defined(OS_CHROMEOS) || defined(OS_MACOSX) || defined(OS_WIN) \\');
GEN('    || defined(TOUCH_UI)');
GEN('#define MAYBE_testRefreshStaysOnCurrentPage \\');
GEN('    DISABLED_testRefreshStaysOnCurrentPage');
GEN('#else');
GEN('#define MAYBE_testRefreshStaysOnCurrentPage ' +
    'testRefreshStaysOnCurrentPage');
GEN('#endif');
TEST_F('OptionsWebUITest', 'MAYBE_testRefreshStaysOnCurrentPage', function() {
  var item = $('advancedPageNav');
  item.onclick();
  window.location.reload();
  var pageInstance = AdvancedOptions.getInstance();
  var topPage = OptionsPage.getTopmostVisiblePage();
  var expectedTitle = pageInstance.title;
  var actualTitle = document.title;
  expectEquals("chrome://settings/advanced", document.location.href);
  expectEquals(expectedTitle, actualTitle);
  expectEquals(pageInstance, topPage);
});
```~~

~~### Maintaining~~

*   ~~Adding more goodies to generator - edit
            `chrome/test/ui/webui/javascript2webui.js` and tests in
            `chrome/test/data/webui/`.~~
*   ~~Disabling a test - find tests in `chrome/test/data/webui/` & mark
            `FLAKY_`, `DISABLED_` or use the `MAYBE_` trick shown above to
            conditionally decide.~~

~~### Considerations/FAQs~~

*   ~~*Isn't mocking in javascript not testing the WebUI message
            passing?* True, but that should be tested as a unit test and then
            trusted. Mocking in JS is easier, less flaky (always recursive - no
            synchronization challenges) and much better than not having any
            tests at all.~~
*   ~~*If you already have the page in question why don't you just run
            all tests without starting a new IN_PROCESS_BROWSER_TEST?* You are
            more than welcome to group tons of expect\* calls into a single
            test; all errors will be reported after the entire test runs. Having
            tests be separate IN_PROCESS_BROWSER_TEST calls ensures the state is
            exactly the same at the start of each tests with no pollution from
            previous tests.~~

~~### Caveats~~

*   ~~\[[crbug.com/88104](http://crbug.com/88104)\] The generator relies
            on d8 to be built for the host. Currently the v8.gyp rules aren't
            correct for Arm as they have conditionals on the 'target_host' and
            don't heed the 'toolset". A gyp condition only runs the js2webui
            rule on non-arm platforms.~~
*   ~~Use of `MAYBE_` to ifdef will have a different run name from GTEST
            than from `test_api.js` - this is because `MAYBE_xyz` will be
            defined as either `xyz` or `DISABLED_xyz` in C++, but not changed in
            javascript.~~

~~### Best practices~~

*   ~~As described in the [gtest
            docs](http://code.google.com/p/googletest/), prefer expect\* over
            assert\*, as it will not halt the test, but will register the
            failure and allow other checks in that particular testcase to run.~~
*   ~~Since the call is included in the failure error message, the
            optional `message` parameter should only include information not
            available:~~

~~```none
// NO
TEST_F('FooTest', 'TestFoo', function() {
  expectEquals(foo, bar, 'foo != bar');
  expectEquals(foo, bar, foo + '!=' + bar);
  var i = ...;
  expectEquals(5, array[i], 'array[i] != 5');
});
// YES
TEST_F('FooTest', 'TestFoo', function() {
  expectEquals(foo, bar);
  expectEquals(foo, bar);
  var i = ...;
  expectEquals(5, array[i], 'i=' + i);
});
```~~
