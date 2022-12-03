---
breadcrumbs:
- - /Home
  - Chromium
page_name: domui-testing
title: DOMUI Testing
---

This document contains a mini design doc for one way to approach testing of
DOMUI pages.

PLEASE See [WebUI browser_tests](/Home/domui-testing/webui-browser_tests) for
details on current implementation.

Objective

Improve quality of DOMUI and find regressions early.

*   Allow unit testing of JS used for DOM UI.
*   Allow unit testing of HTML widgets used for DOM UI.
*   Allow testing end to end DOMUI

Problem

More and more of the UI in Chrome and ChromeOS is built using DOMUI. DOMUI is
the name of Chrome UI built using HTML, JS and CSS with the ability to
comunicate to with the browser process using chrome.send calls. Example of UIs
implemented using DOMUI consists of the New Tab Page, History, Downloads,
Extensions, about:versions and many others. ChromeOS is also in the process of
using DOMUI for menus.

Testing these UIs is [possible
today](http://www.google.com/codesearch/p?hl=en#OAMlx_jo-ck/src/chrome/browser/dom_ui/new_tab_ui_uitest.cc&q=NewTabUITest%20NTPHasLoginName&exact_package=chromium&sa=N&cd=1&ct=rc&l=55)
using
[UITest](http://www.google.com/codesearch/p?hl=en#OAMlx_jo-ck/src/chrome/test/ui/ui_test.h&q=%22class%20UITest%22&exact_package=chromium&sa=N&cd=1&ct=rc&l=458)
with DOM automation enabled. However, writing these test require generating
strings of JavaScript code that is (using the automation provider)
asynchronously evaluated in the browser tab and the result of that is
asynchronously returned.

There are 3 problems with this approach:

1.  Setting up the browser tab requires some C++ and a lot of waiting
            for the document to be ready.
2.  A string representing the JavaScript needs to be generated.
3.  The evaluation is async making test harder to write and more flaky.

Requirements

Make it easier to write tests by

1.  Allowing writing the test in JavaScript. After all, we need to use
            JavaScript to interact with the UI so it is only natural to do the
            test with it. Also dynamic languages are very good for writing unit
            tests.
2.  Provide a new test class that is specialized for writing tests for
            DOMUI. It would wrap up all of the boiler plate code and it
            should...
3.  Allow any DOM UI to be tested by simply providing the URL of the
            page and...
4.  Provide the URL or path to the test file and...
5.  allow other utility js files to be injected

It might also be interesting to allow a blank page to be used and just inject
all the js and css we care about. This could be useful for testing widgets in
isolation.

Solution

Create a new test class similar to ExtensionBrowserTest but optimized for DOMUI.
This class would have a RunDOMUITest (similar to RunExtensionTest) which would:

1.  Open a tab and navigate to a URL.
2.  Inject one or more javascript files.
3.  Run tests provided by the JavaScript files (ExtensionBrowerTest uses
            a global array of functions to run called test)

This is better than what we have now for a few reasons:

*   We do not have to open a new browser for every single test. We can
            run multiple tests in the same web page.
*   We can write the test in JS, reducing the work needed to write
            tests.
*   We do not have to generate js code and evaluate it using the
            automation provider for every single test (these are async)

Example Use-cases

The following is based on the same model as the ExtensionApiTest.

.cc file

IN_PROC_BROWSER_TEST_F(DOMUITest, NewTabPage) {

ASSERT_TRUE(RunDOMUITest("chrome://newtabpage", path_to_js_test_file);

}

.js file

var assertEq = chrome.test.assertEq;

var tests = \[

function testLogin() {

var loginSpan = $(‘login-username’);

assertEq(loginSpan.textContent, ‘test@gmail.com’);

}

\];

Tradeoffs

This solution requires 2 (or more files) for every test. One for the C++ test
function that bootstraps the javascript tests.

Alternatives

We could probably do incremental improvements over UITest...

The most similar thing today is Extension API tests. These are test end to end
of the extension APIs and these use extension pages and

Another option is to make all DOMUI extensions and just use
