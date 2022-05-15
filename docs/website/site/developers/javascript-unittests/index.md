---
breadcrumbs:
- - /developers
  - For Developers
page_name: javascript-unittests
title: JavaScript Unit tests Cookbook for Chrome Remote Desktop
---

[TOC]

## Overview

This page explains how easy it is write a JavaScript Unit test for chrome remote
desktop using QUnit.

[<img alt="image"
src="/developers/javascript-unittests/QUnit.png">](/developers/javascript-unittests/QUnit.png)

## Key features

1.  Code coverage report with blanket.js
2.  Supports mocks, stubs and spies via sinon.js
3.  Supports DOM testing.
4.  Supports Async testing. (Promises, setTimeouts, etc)
5.  Can be run as a browser test on try-bots and built-bots
6.  Can be run as a standalone page to facilitate fast development
7.  Can be debugged easily with the Developer console.

Currently, the QUnit infrastructure is only used by the chrome remote desktop
team. It can be easily extended to other teams if there are interests

## In a nut shell

The key components are:

1.  **QUnit** is the overall JavaScript unit testing framework. It
            provides a test store, asserts and test reporting.
2.  **SinonJS** provides a mocking/spying/stubbing framework to make
            writing test easier.
3.  **BlanketJS** provides code coverage numbers.
4.  **QUnitBrowserTestRunner** makes it possible to run the unit test in
            browser tests.

## How do I ...

### Create a new test case?

> 1. Create a new file under *src/remoting/unittests*, the filed should be named
> as test_&lt;module_name&gt;.js, here is an example for *test_math.js*

> `// Copyright 2014 The Chromium Authors. All rights reserved.`
> `// Use of this source code is governed by a BSD-style license that can be`
> `// found in the LICENSE file.`
> `(function() {`
> `'use strict';`
> `QUnit.module('Math.floor', {`
> ` beforeEach: function() {`
> ` ...`
> ` },`
> ` afterEach: function() {`
> ` ...`
> ` }`
> `});`
> `QUnit.test('should round down a floating point number', function(assert) {`
> ` assert.equal(Math.floor(1.1), 1);`
> `});`
> `QUnit.test('should work for negative numbers', function(assert) {`
> ` assert.equal(Math.floor(-1.1), 2);`
> `});`
> ` })();`

> 2. Add the file to the **'remoting_webapp_unittest_cases'** variable in
> ***src/remoting/remoting_webapp_files.gypi***

> `# The unit test cases for the webapp`
> `'remoting_webapp_unittest_cases': [`
> ` ...,`
> ` 'webapp/unittests/test_math.js',`
> `],`

> 3. Update gyp

> `gclient runhooks`

> 4. Build the test

> `ninja -C out/Debug -j500 remoting_webapp_unittests`

### Run the test locally?

> #### For the chrome remote desktop unit test

> Go to the source directory

> `./remoting/tools/run_webapp_unittests.py`

> What the script does under the hood is to run the test page in ***Chrome***
> with the --***allow-file-access flag***. ***Blanketjs*** need to load the
> source file locally to instrument it for code coverage.

> If you make changes to your test code, you don't need to rebuild after this.
> Simply edit your test code and hit F5

### Organize my test?

There should be one test file per product file, e.g.

> &lt;product_file_name&gt;_unittest.js

> Group each class into its own module, e.g.

> `QUnit.module('class_name')`

> `Create one test case per behavior. The name of a test should be an English statement in the format of method1() should ... '`
> `If a method has multiple behavior, you can create multiple test for it.`

> `QUnit.module('class_name');`

> ` test('method1() should have behavior X', function (assert) {`

> ` // test body`

> `});`

> `QUnit.test('method1() should have behavior Y', function (`assert) {

> ` // test body`

> `});`

> `QUnit.test('method2() should have behavior Z', function (`assert) {

> ` // test body`

> `});`

### Use asserts?

> QUnit is assertion-based. An assert is claim about the behavior of the piece
> of code under test.

> The most common asserts are:

> `ok(condition)`

> `equal(expected, actual)`

> See <http://api.qunitjs.com/category/assert/> for the API reference.

### Mock out existing components?

> SinonJS allows you to mock existing components. SinonJS provides three key
> functionalities
> **Spying** - **Track calls to functions**
> **Stubs** - Create functions with **pre-programmed behaviors**. When wrapped
> around existing function with a stub, the original function is not called.
> **Mocking** - Create functions with **pre-programmed behaviors** and
> **pre-programmed expectations**
> See <http://sinonjs.org/docs/> for the API reference.

### See the coverage number?

> 1.  Click on ***Enable Coverage*** in the test page
>     [<img alt="image"
      src="/developers/javascript-unittests/EnableCoverage.png">](/developers/javascript-unittests/EnableCoverage.png)
> 2.  The number will be reported inline, with uncovered lines displayed
              with a red highlighted background.
>     [<img alt="image"
      src="/developers/javascript-unittests/Coverage.png">](/developers/javascript-unittests/Coverage.png)

### Debug a test failure?

> Since the QUnit test runs in the browser, you can debug a test failure just
> like any web page using the developer console.

### Run my unit test in the browser test?

> You can also run the test under the browser test infrastructure

> `./out/Debug/browser_tests
> --gtest_filter=QUnitBrowserTestRunner.Remoting_Webapp_Js_Unittest`

> If the test fails, it will print out the test case that fails:

> `Value of: passed`
> `Actual: false`
> Expected: true`
> `1 test failed:`
> ` base.EventSource.removeEventListener.should work even if the listener is removed during |raiseEvent|`
> ` 1. expected spy to be called twice but was called once`
> ` spy(undefined)`
> ` at Object.sinon.assert.fail
> (file:///usr/local/google/home/kelvinp/enlistments/chromium/src/out/Release/remoting/unittests/sinonjs/sinon-qunit.js:36:11)`
> `[ FAILED ] QUnitBrowserTestRunner.Remoting_Webapp_Js_Unittest, where
> TypeParam = and GetParam() = (1340 ms)`

> The easiest way to debug the failure is to run the failed module manually in
> chrome and debug from there.

## Documentation and Design Docs

<table>
<tr>
<td>Design docs</td>
<td> <a href="https://docs.google.com/a/chromium.org/document/d/1SOacnv91zFWEqtietEG6ubh3qDfpds3J3e1Bb1Lfgys/edit?usp=sharing">https://docs.google.com/a/chromium.org/document/d/1SOacnv91zFWEqtietEG6ubh3qDfpds3J3e1Bb1Lfgys/edit?usp=sharing</a></td>
</tr>
<tr>
<td>QUnit documentation </td>
<td> <a href="http://api.qunitjs.com/">http://api.qunitjs.com/</a></td>
</tr>
<tr>
<td>BlanketJS documentation</td>
<td> <a href="http://blanketjs.org/">http://blanketjs.org/</a></td>
</tr>
<tr>
<td>SinonJS documentation</td>
<td> <a href="http://sinonjs.org/docs/">http://sinonjs.org/docs/</a></td>
</tr>
</table>