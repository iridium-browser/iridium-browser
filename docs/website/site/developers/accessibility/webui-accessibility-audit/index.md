---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/accessibility
  - Accessibility for Chromium Developers
page_name: webui-accessibility-audit
title: WebUI accessibility audit
---

[TOC]

### What is it?

Any test written using the [WebUI browser test
framework](/Home/domui-testing/webui-browser_tests) will now run an
accessibility audit at the conclusion of each test case.

### How do I find and fix errors?

Each error gives you querySelector strings pointing to the elements that failed
the audit, and an error code indicating the exact test that failed.

For example, suppose your test fails with the following error:

```none
*** Begin accessibility audit results ***
An accessibility audit found 1 error on this page.
For more information, please see http://chromium.org/developers/accessibility/webui-accessibility-audit
Error: controlsWithoutLabel (AX_TEXT_01) failed on the following elements (1 - 2 of 2):
#searchbox > INPUT
#searchbox > SELECT
*** End accessibility audit results ***
```

The error code here is AX_TEXT_01. You can find the definition of the error
codes on the [Audit
Rules](https://code.google.com/p/accessibility-developer-tools/wiki/AuditRules)
page. That should tell you exactly how to fix your error.

Two elements failed this test, with the querySelector strings #searchbox &gt;
INPUT and #searchbox &gt; SELECT. In other words, there's an element with id
`searchbox`, and it has two direct descendant elements - one an INPUT element,
one a SELECT element. You can use the developer tools to interactively evaluate,
e.g., `document.querySelector('#searchbox > INPUT')` to find one of these
elements.

### Can I run the audit interactively?

Yes! This audit is based on the public Accessibility Developer Tools Chrome
extension.

There are different versions of the extension depending on what you want to run
it on:

*   [Accessibility Developer Tools
            Web](https://chrome.google.com/webstore/detail/accessibility-developer-t/fpkknkljclfencbdbgkenhalefipecmb)
            - for auditing any web page other than an app or built-in page
*   A[ccessibility Developer Tools
            Apps](https://chrome.google.com/webstore/a/google.com/detail/accessibility-developer-t/lfcjaoacndhilkpdhgnfjnienfoibnaa)
            - for auditing packaged apps (link is Google-only)
*   [Accessibility Developer Tools
            WebUI](https://chrome.google.com/webstore/a/google.com/detail/accessibility-developer-t/eacmnlimniaidhecpinhhfjjilfdaccm)
            - for Chromium WebUI like chrome://settings/ (link is Google-only)
*   [Build it from
            source](https://github.com/GoogleChrome/accessibility-developer-tools)

Note that normally, content scripts don't run on Chrome's WebUI pages with a
chrome:// url, for security. However, you can bypass this by running Chrome with
the `--extensions-on-chrome-urls` flag (or toggling the setting in
chrome://flags) and then using an extension that adds "chrome://\*/\*" to its
list of permissions. The "Accessibility Developer Tools WebUI" version linked
above already has this flag set.

Install the extension, navigate to the url you want to test, open developer
tools, click on the Audit tab, and run the accessibility audit.

### I think the audit is wrong. How do I disable it?

You can disable the accessibility check for a particular **test** **case** by
calling `disableAccessibilityChecks()` during the test method.

For example,

```none
TEST_F('SomeWebUITest', 'basicTest', function() {
   this.disableAccessibilityChecks();
    // rest of test code
});
```

You can disable the accessibility check for a **test** **fixture** by setting
the `runA11yChecks` parameter on the fixture object.

For example,

```none
SomeWebUITest.prototype = {
        _proto__: testing.Test.prototype,
        runAccessibilityChecks: false,
    // rest of fixture
}; 
```

You can then enable the accessibility check on a **per-test** basis using the
`enableAccessibilityChecks()`:

```none
TEST_F('SomeWebUITest', 'aDifferentTest', function()
    enableAccessibilityChecks();
    //rest of test code
})
```

### How is the audit automatically triggered for WebUI tests?

The code to trigger it is in
[chrome/test/data/webui/test_api.js](https://code.google.com/p/chromium/codesearch#chromium/src/chrome/test/data/webui/test_api.js).
Before `tearDown()`, it runs all the audit rules from the
[accessibility-developer-tools](http://code.google.com/p/accessibility-developer-tools/source/browse/#git%2Fsrc%2Faudits)
library on the WebUI page under test. Each rule consists of a selector function
which picks out the relevant elements on the page, and a test which is run on
each matching element. If the test fails, the element is added to a list of
failing elements. For each failing audit rule, up to five elements will be
output at the end of the test, in the form of a query selector string which can
be used to find the element on the page.

### What do the test results mean?

The [accessibility-developer-tools
wiki](http://code.google.com/p/accessibility-developer-tools/wiki/AuditRules)
has more information on the specific audit rules.
