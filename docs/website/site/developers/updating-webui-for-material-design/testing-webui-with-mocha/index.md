---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/updating-webui-for-material-design
  - Updating WebUI for Material Design
page_name: testing-webui-with-mocha
title: Testing WebUI with Mocha
---

As Chromium focuses on building modular WebUI using web components and
well-defined API calls, we aim to phase out and ultimately replace
[WebUIBrowserTest](/Home/domui-testing/webui-browser_tests) with a mocha-based
alternative. [Mocha](http://mochajs.org) is a flexible, lightweight test
framework with strong async support. With mocha, we can write small unit tests
for individual features of components instead of grouping everything into a
monolithic browser test.

Advantages of using mocha over vanilla browser tests include:

*   Allowing multiple test definitions within one browser test,
            amortizing the browser spin-up cost over several tests
*   Pinpointing failure locations, even when several tests fail within
            one browser test or when assertions fail inside in helper functions
*   Associating uncaught exceptions with the correct test
*   Reporting test durations and flagging slow tests
*   Fast test iteration -- changes to tests may not require recompiling

This framework is a work in progress; its first consumers are Polymer projects
in Chromium.

### Adding the BrowserTest

Polymer tests should use `PolymerTest` from `polymer_browser_test_base.js` as
their prototype. The following test will set the command-line args
`--my-switch=my-value` and navigate to `chrome://my-webui/`.

```js
// Polymer BrowserTest fixture.
GEN_INCLUDE(['//chrome/test/data/webui/polymer_browser_test_base.js']);
/**
 * Test fixture for FooBar element.
 * @constructor
 * @extends {PolymerTest}
 */
function FooBarBrowserTest() {}
FooBarBrowserTest.prototype = {
  __proto__: PolymerTest.prototype,
  /**
   * Override browsePreload like this if you need to navigate to a particular
   * WebUI.
   * @override
   */
  browsePreload: 'chrome://my-webui',
  /**
   * Set any command line switches needed by your features.
   */
  commandLineSwitches: [{
    switchName: 'my-switch',
    switchValue: 'my-value'  // omit switchValue for boolean switches
  }],
  /**
   * Override extraLibraries to include extra files when the test runs.
   * @override
   */
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
    // Loads ./some_file.js (relative to source root).
    'some_file.js',
    // Path must be included in browser_tests.isolate.
    '//ui/webui/resources/js/some_file.js',  
  ]);
};
```

Then, create one or more `TEST_F` functions. Each should register some number of
tests with mocha:

```js
TEST_F('FooBarBrowserTest', 'FooBarTest', function() {
  suite('FooBar', function() {
    var fooBar;
    // Import foo_bar.html before running suite.
    suiteSetup(function() {
      // Returns a promise, ensuring mocha waits until the resource loads.
      return PolymerTest.importHtml('chrome://resources/foo_bar.html');
    });
    // Create and initialize a FooBar before each test.
    setup(function() {
      PolymerTest.clearBody();
      // Immediately calls created() and ready().
      fooBar = document.createElement('foo-bar');
      document.body.appendChild(fooBar);  // Immediately calls attached().
    });
    test('is a FooBar', function() {
      assertEquals('FOO-BAR', fooBar.tagName);
      // ... more work and assertions
    });
    // ... more synchronous and asynchronous tests
  });
  // ... more test suites
  // Run all the above tests.
  mocha.run();
});
```

Should you write a large number of tests (woohoo!), it may help to break tests
into separate files. For example, if you have `foo_bar_tests.js`:

```js
cr.define('foo_bar', function() {
  function registerTests() {
    suite('FooBar', function() { /* ... */ });
  }
  return {registerTests: registerTests};
});
```

You can then include these tests in your main `foo_bar_browsertest.js` by adding
the file to `FooBarBrowserTest.prototype.extraLibraries`. Then, in your
`TEST_F`, simply call `foo_bar.registerTests()` before the `mocha.run()` call.

### Including JS files in tests

Files imported via `extraLibraries` are read when the test runs. In isolated
testing, only certain files are copied to the bots. Files in chrome/test/data
are always copied over, but **if your test requires files in other directories,
make sure the file or some parent directory is included in
`chrome/browser_tests.isolate`**.

Explanation: js2gtest allows `#include`-style imports of JavaScript using
`GEN_INCLUDE` and the `extraLibraries` property of the test fixture prototype.
These should be relative to the file defining your test fixture.

Use `GEN_INCLUDE` when the file needs to be included before the rest of the
script is run. This will also result in the code being eval'd during the
js2gtest step.

Use the `extraLibraries` property of your test fixture's prototype when the file
is only needed within your tests. The content of these files, along with any
`GEN_INCLUDE`d files, are added to each GTest body before each test is run.

Because these files are read at runtime, they must be present when running the
browser_tests binary. This is only a concern in
[isolated testing](/developers/testing/isolated-testing/for-swes), and placing
included files in `chrome/test/data` should always work.

### Running the tests

Just
[build and run like any other browser_tests](/developers/testing/browser-tests)
You may filter tests by class or test name (the arguments to TEST_F) via
`--gtest_filter`:

```sh
ninja -C out/Release browser_tests
./out/Release/browser_tests --gtest_filter="FooBarBrowserTest.Foo*"
```

### Tests and suites

With mocha, we can define any number of **suites**; each suite can consist
of any number of **tests**. Top-level tests are in the global suite, but it is
recommended to always create an outer suite for your tests. Each **suite** and
**test** call takes a name and a function.

We can also define a **suiteSetup** to run at the beginning of a suite, a
**setup** to run before each test in a suite, a **teardown** to run after each
test in a suite, and a **suiteTeardown** to run once a suite is completed.

Suites can be nested; each **setup** and **teardown** also applies to all tests
in nested suites.

By default, hooks and tests are synchronous. Any of the functions used in these
calls can take a `done` callback as a parameter, causing mocha to wait until the
`done` parameter is called before moving to the next test or hook. **The `done`
callback must be called with an `Error` on failure, or no arguments on
success.**

```js
suite('Outer suite', function() {
  suiteSetup(function() {
    // Function to run at the beginning of this suite.
  });
  setup(function() {
    // Function to run before each test within this suite.
  });
  test(function() {
    // A test that runs after setup is called.
  });
  test(function(done) {
    // An asynchronous test that runs after setup is called again.
    setTimeout(done, 1000);
  });
  teardown(function() {
    // Function to run after each test in this suite.
  });
  suiteTeardown(function() {
    // Function to run at the very end of this suite.
  });
  suite('Inner suite', function() {
    // Another suite. We can define separate suiteSetup and setup functions
    // here that only run within this suite.
  });
});
```

**Instead of taking a `done` parameter, an asynchronous hook or test can return
a `Promise`.** The test succeeds or fails once the promise is resolved or
rejected.

```js
// Notice no done callback is taken.
suiteSetup(function() {
  return new Promise(function(resolve, reject) {
    window.addEventListener('thingCreated', resolve);
    createThingAsync();
  });
});
test('Something doesn\'t happen', function() {
  return new Promise(function(resolve, reject) {
    // Ensure no event fires.
    window.addEventListener('change', reject);
    setTimeout(resolve, 1000);
    somethingShouldNotChange();
  });
});
```

### More features

For more mocha features, see [the documentation](http://mochajs.org/). The API
is well documented [within the
source](https://github.com/mochajs/mocha/blob/master/lib/mocha.js).
