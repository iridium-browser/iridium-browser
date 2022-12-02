---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/accessibility
  - Accessibility for Chromium Developers
- - /developers/accessibility/testing
  - Accessibility Testing
page_name: axe-core
title: Axe-Core Testing
---

TL;DR

Writing an accessibility (a11y) test with axe-core will create 1 test for each
rule and test created. This will prevent timeouts.

**A11y tests DO NOT run in DEBUG builds** (also to prevent timeouts), this
shouldn't be an issue since we're only testing web UI with axe-core.

Simple Test

// Copyright 2019 The Chromium Authors.

// Use of this source code is governed by a BSD-style license that can be

// found in the LICENSE file.

// Polymer BrowserTest fixture and aXe-core accessibility audit.

GEN_INCLUDE(\[

'//chrome/test/data/webui/a11y/accessibility_test.js',

'//chrome/test/data/webui/polymer_browser_test_base.js',

\]);

TestFixture = class extends PolymerTest {

/\*\* @override \*/

get browsePreload() {

return 'chrome://settings/'; // URL of page to test.

}

};

AccessibilityTest.define('TestFixture', {

// Must be unique within the test fixture and cannot have spaces.

name: 'TestSuiteName',

// Optional. Configuration for axe-core. Can be used to disable a test.

axeOptions: {},

// Optional. Filter on failures. Use this for individual false positives.

violationFilter: {},

// Optional. Any setup required for all tests. This will run before each one.

setup: function() {

},

tests: {

'Test case description': function() {

// Set up an individual test case. You can open up a dialog or click

// buttons to get the test to the state that should be tested for a11y.

// This will run once per axe-core a11y rule.

// Can return a promise for async setup.

}

},

});

Examples

*   [MD
            Settings](https://cs.chromium.org/chromium/src/chrome/test/data/webui/settings/a11y/)
*   [MD
            Extensions](https://cs.chromium.org/chromium/src/chrome/test/data/webui/extensions/a11y/)

How it Works

Accessibility audits can take a while to run, so we broke each audit out into
its own test. This would be tedious to do by hand so the
AccessibilityTest.define function will create them based on some params. This
calls TEST_F for each test that's being created and uses a test fixture like all
web UI tests. In the past, a11y audit was tacked onto the end of all web UI
tests but we want to avoid timeouts since the number of tests in axe-core is
much larger than ADT, so a11y tests now have to be created manually-ish.

Each test inside the tests data member will be run as its own Mocha test.

Example:

AccessibilityTest.define('TestFixture', {

name: 'TestSuiteName',

tests: {

'Test1': function() {

// No special setup

},

'Test2': function() {

// Show the dialog before testing for a11y.

let dialog = document.querySelector('#dialog');

dialog.showModal();

},

},

});

That code snippet will create the following tests:

> TestFixture.TestSuiteName_accesskeys

> TestFixture.TestSuiteName_area_alt

> TestFixture.TestSuiteName_aria_allowed_attr

> TestFixture.TestSuiteName_aria_hidden_body

> TestFixture.TestSuiteName_aria_required_attr

> TestFixture.TestSuiteName_aria_required_children

> TestFixture.TestSuiteName_aria_required_parent

> TestFixture.TestSuiteName_aria_roles

> TestFixture.TestSuiteName_aria_valid_attr

> TestFixture.TestSuiteName_aria_valid_attr_value

> TestFixture.TestSuiteName_audio_caption

> TestFixture.TestSuiteName_blink

> TestFixture.TestSuiteName_button_name

> TestFixture.TestSuiteName_bypass

> TestFixture.TestSuiteName_checkboxgroup

> TestFixture.TestSuiteName_definition_list

> TestFixture.TestSuiteName_dlitem

> TestFixture.TestSuiteName_document_title

> TestFixture.TestSuiteName_duplicate_id

> TestFixture.TestSuiteName_empty_heading

> TestFixture.TestSuiteName_frame_title

> TestFixture.TestSuiteName_frame_title_unique

> TestFixture.TestSuiteName_heading_order

> TestFixture.TestSuiteName_hidden_content

> TestFixture.TestSuiteName_href_no_hash

> TestFixture.TestSuiteName_html_has_lang

> TestFixture.TestSuiteName_html_lang_valid

> TestFixture.TestSuiteName_image_alt

> TestFixture.TestSuiteName_image_redundant_alt

> TestFixture.TestSuiteName_input_image_alt

> TestFixture.TestSuiteName_label

> TestFixture.TestSuiteName_label_title_only

> TestFixture.TestSuiteName_layout_table

> TestFixture.TestSuiteName_link_in_text_block

> TestFixture.TestSuiteName_link_name

> TestFixture.TestSuiteName_list

> TestFixture.TestSuiteName_listitem

> TestFixture.TestSuiteName_marquee

> TestFixture.TestSuiteName_meta_refresh

> TestFixture.TestSuiteName_meta_viewport

> TestFixture.TestSuiteName_meta_viewport_large

> TestFixture.TestSuiteName_object_alt

> TestFixture.TestSuiteName_p_as_heading

> TestFixture.TestSuiteName_radiogroup

> TestFixture.TestSuiteName_region

> TestFixture.TestSuiteName_scope_attr_valid

> TestFixture.TestSuiteName_server_side_image_map

> TestFixture.TestSuiteName_tabindex

> TestFixture.TestSuiteName_table_duplicate_name

> TestFixture.TestSuiteName_table_fake_caption

> TestFixture.TestSuiteName_td_has_header

> TestFixture.TestSuiteName_td_headers_attr

> TestFixture.TestSuiteName_th_has_data_cells

> TestFixture.TestSuiteName_valid_lang

> TestFixture.TestSuiteName_video_caption

> TestFixture.TestSuiteName_video_description

Each of these tests will have a mocha body that will run both Test1 and Test2,
then run the a11y audit on the state of the page after each test setup.

[Googler only link to design doc](http://go/a11y-chrome-settings-design). (Doc
has a lot of overlap with this page, but goes more into design considerations)

Debugging

Axe-Core has an extension that can be used to debug by running the same library
that we run the tests with.

[Official
Extension](https://chrome.google.com/webstore/detail/axe/lhdoppojpmngadmnindnejefpokejbdd)

[Googler Extension](http://go/axe-core-chrome-extension)
([source](http://go/axe-core-chrome-extension-source))

You must pass in the --extensions-on-chrome-urls flag in order to run these
extensions on chrome:// urls. This is a security risk, so only run extensions
you trust when doing this.

Running Locally

The axe-core a11y audit is part of browser_tests. Instructions for building are
the same as that target.

Note: axe-core tests are NOT part of DEBUG builds to prevent timeouts on test
bots.
