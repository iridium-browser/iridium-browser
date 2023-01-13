#!/usr/bin/env python3
# Copyright (c) 2021 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os.path
import subprocess
import unittest

from presubmit_canned_checks_test_mocks import MockFile, MockAffectedFile
from presubmit_canned_checks_test_mocks import MockInputApi, MockOutputApi
from presubmit_canned_checks_test_mocks import MockChange

import presubmit_canned_checks

class InclusiveLanguageCheckTest(unittest.TestCase):
  def testBlockedTerms(self):
    input_api = MockInputApi()
    input_api.change.RepositoryRoot = lambda: ''
    input_api.presubmit_local_path = ''

    input_api.files = [
      MockFile('infra/inclusive_language_presubmit_exempt_dirs.txt', [
               'some/dir 2 1',
               'some/other/dir 2 1',
               ]),
      MockFile('some/ios/file.mm',
               ['TEST(SomeClassTest, SomeInteraction, blacklist) {', # nocheck
                '}']),
      MockFile('some/mac/file.mm',
               ['TEST(SomeClassTest, SomeInteraction, BlackList) {', # nocheck
                '}']),
      MockFile('another/ios_file.mm',
               ['class SomeTest : public testing::Test blocklist {};']),
      MockFile('some/ios/file_egtest.mm',
               ['- (void)testSomething { V(whitelist); }']), # nocheck
      MockFile('some/ios/file_unittest.mm',
               ['TEST_F(SomeTest, Whitelist) { V(allowlist); }']), # nocheck
      MockFile('some/doc/file.md',
               ['# Title',
                'Some markdown text includes master.', # nocheck
               ]),
      MockFile('some/doc/ok_file.md',
               ['# Title',
                 # This link contains a '//' which the matcher thinks is a
                 # C-style comment, and the 'master' term appears after the
                 # '//' in the URL, so it gets ignored as a side-effect.
                '[Ignored](https://git/project.git/+/master/foo)', # nocheck
               ]),
      MockFile('some/doc/branch_name_file.md',
               ['# Title',
                 # Matches appearing before `//` still trigger the check.
                '[src/master](https://git/p.git/+/master/foo)', # nocheck
               ]),
      MockFile('some/java/file/TestJavaDoc.java',
               ['/**',
                ' * This line contains the word master,', # nocheck
                 '* ignored because this is a comment. See {@link',
                ' * https://s/src/+/master:tools/README.md}', # nocheck
                ' */']),
      MockFile('some/java/file/TestJava.java',
               ['class TestJava {',
                '  public String master;', # nocheck
                '}']),
      MockFile('some/html/file.html',
               ['<-- an existing html multiline comment',
                'says "master" here', # nocheck
                'in the comment -->'])
     ]

    errors = presubmit_canned_checks.CheckInclusiveLanguage(input_api,
                                                            MockOutputApi())
    self.assertEqual(1, len(errors))
    self.assertTrue('some/ios/file.mm' in errors[0].message)
    self.assertTrue('another/ios_file.mm' not in errors[0].message)
    self.assertTrue('some/mac/file.mm' in errors[0].message)
    self.assertTrue('some/ios/file_egtest.mm' in errors[0].message)
    self.assertTrue('some/ios/file_unittest.mm' in errors[0].message)
    self.assertTrue('some/doc/file.md' not in errors[0].message)
    self.assertTrue('some/doc/ok_file.md' not in errors[0].message)
    self.assertTrue('some/doc/branch_name_file.md' not in errors[0].message)
    self.assertTrue('some/java/file/TestJavaDoc.java' not in errors[0].message)
    self.assertTrue('some/java/file/TestJava.java' not in errors[0].message)
    self.assertTrue('some/html/file.html' not in errors[0].message)

  def testBlockedTermsWithLegacy(self):
    input_api = MockInputApi()
    input_api.change.RepositoryRoot = lambda: ''
    input_api.presubmit_local_path = ''

    input_api.files = [
      MockFile('infra/inclusive_language_presubmit_exempt_dirs.txt', [
               'some/ios 2 1',
               'some/other/dir 2 1',
               ]),
      MockFile('some/ios/file.mm',
               ['TEST(SomeClassTest, SomeInteraction, blacklist) {', # nocheck
                '}']),
      MockFile('some/ios/subdir/file.mm',
               ['TEST(SomeClassTest, SomeInteraction, blacklist) {', # nocheck
                '}']),
      MockFile('some/mac/file.mm',
               ['TEST(SomeClassTest, SomeInteraction, BlackList) {', # nocheck
                '}']),
      MockFile('another/ios_file.mm',
               ['class SomeTest : public testing::Test blocklist {};']),
      MockFile('some/ios/file_egtest.mm',
               ['- (void)testSomething { V(whitelist); }']), # nocheck
      MockFile('some/ios/file_unittest.mm',
               ['TEST_F(SomeTest, Whitelist) { V(allowlist); }']), # nocheck
    ]

    errors = presubmit_canned_checks.CheckInclusiveLanguage(input_api,
                                                            MockOutputApi())
    self.assertEqual(1, len(errors))
    self.assertTrue('some/ios/file.mm' not in errors[0].message)
    self.assertTrue('some/ios/subdir/file.mm' in errors[0].message)
    self.assertTrue('another/ios_file.mm' not in errors[0].message)
    self.assertTrue('some/mac/file.mm' in errors[0].message)
    self.assertTrue('some/ios/file_egtest.mm' not in errors[0].message)
    self.assertTrue('some/ios/file_unittest.mm' not in errors[0].message)

  def testBlockedTermsWithNocheck(self):
    input_api = MockInputApi()
    input_api.change.RepositoryRoot = lambda: ''
    input_api.presubmit_local_path = ''

    input_api.files = [
      MockFile('infra/inclusive_language_presubmit_exempt_dirs.txt', [
               'some/dir 2 1',
               'some/other/dir 2 1',
               ]),
      MockFile('some/ios/file.mm',
               ['TEST(SomeClassTest, SomeInteraction, ',
                ' blacklist) { // nocheck', # nocheck
                '}']),
      MockFile('some/mac/file.mm',
               ['TEST(SomeClassTest, SomeInteraction, ',
                'BlackList) { // nocheck', # nocheck
                '}']),
      MockFile('another/ios_file.mm',
               ['class SomeTest : public testing::Test blocklist {};']),
      MockFile('some/ios/file_egtest.mm',
               ['- (void)testSomething { ',
                'V(whitelist); } // nocheck']), # nocheck
      MockFile('some/ios/file_unittest.mm',
               ['TEST_F(SomeTest, Whitelist) // nocheck', # nocheck
                ' { V(allowlist); }']),
      MockFile('some/doc/file.md',
               ['Master in markdown <!-- nocheck -->', # nocheck
                '## Subheading is okay']),
      MockFile('some/java/file/TestJava.java',
               ['class TestJava {',
                '  public String master; // nocheck', # nocheck
                '}']),
      MockFile('some/html/file.html',
               ['<-- an existing html multiline comment',
                'says "master" here --><!-- nocheck -->', # nocheck
                '<!-- in the comment -->'])
    ]

    errors = presubmit_canned_checks.CheckInclusiveLanguage(input_api,
                                                            MockOutputApi())
    self.assertEqual(0, len(errors))

  def testTopLevelDirExcempt(self):
    input_api = MockInputApi()
    input_api.change.RepositoryRoot = lambda: ''
    input_api.presubmit_local_path = ''

    input_api.files = [
      MockFile('infra/inclusive_language_presubmit_exempt_dirs.txt', [
               '. 2 1',
               'some/other/dir 2 1',
               ]),
      MockFile('presubmit_canned_checks_test.py',
               ['TEST(SomeClassTest, SomeInteraction, blacklist) {', # nocheck
                '}']),
      MockFile('presubmit_canned_checks.py',
               ['- (void)testSth { V(whitelist); } // nocheck']), # nocheck
    ]

    errors = presubmit_canned_checks.CheckInclusiveLanguage(input_api,
                                                            MockOutputApi())
    self.assertEqual(1, len(errors))
    self.assertTrue('presubmit_canned_checks_test.py' in errors[0].message)
    self.assertTrue('presubmit_canned_checks.py' not in errors[0].message)

  def testChangeIsForSomeOtherRepo(self):
    input_api = MockInputApi()
    input_api.change.RepositoryRoot = lambda: 'v8'
    input_api.presubmit_local_path = ''

    input_api.files = [
      MockFile('some_file', [
               '# this is a blacklist', # nocheck
               ]),
    ]
    errors = presubmit_canned_checks.CheckInclusiveLanguage(input_api,
                                                            MockOutputApi())
    self.assertEqual([], errors)


class DescriptionChecksTest(unittest.TestCase):
  def testCheckDescriptionUsesColonInsteadOfEquals(self):
    input_api = MockInputApi()
    input_api.change.RepositoryRoot = lambda: ''
    input_api.presubmit_local_path = ''

    # Verify error in case of the attempt to use "Bug=".
    input_api.change = MockChange([], 'Broken description\nBug=123')
    errors = presubmit_canned_checks.CheckDescriptionUsesColonInsteadOfEquals(
        input_api, MockOutputApi())
    self.assertEqual(1, len(errors))
    self.assertTrue('Bug=' in errors[0].message)

    # Verify error in case of the attempt to use "Fixed=".
    input_api.change = MockChange([], 'Broken description\nFixed=123')
    errors = presubmit_canned_checks.CheckDescriptionUsesColonInsteadOfEquals(
        input_api, MockOutputApi())
    self.assertEqual(1, len(errors))
    self.assertTrue('Fixed=' in errors[0].message)

    # Verify error in case of the attempt to use the lower case "bug=".
    input_api.change = MockChange([], 'Broken description lowercase\nbug=123')
    errors = presubmit_canned_checks.CheckDescriptionUsesColonInsteadOfEquals(
        input_api, MockOutputApi())
    self.assertEqual(1, len(errors))
    self.assertTrue('Bug=' in errors[0].message)

    # Verify no error in case of "Bug:"
    input_api.change = MockChange([], 'Correct description\nBug: 123')
    errors = presubmit_canned_checks.CheckDescriptionUsesColonInsteadOfEquals(
        input_api, MockOutputApi())
    self.assertEqual(0, len(errors))

    # Verify no error in case of "Fixed:"
    input_api.change = MockChange([], 'Correct description\nFixed: 123')
    errors = presubmit_canned_checks.CheckDescriptionUsesColonInsteadOfEquals(
        input_api, MockOutputApi())
    self.assertEqual(0, len(errors))


if __name__ == '__main__':
  unittest.main()
