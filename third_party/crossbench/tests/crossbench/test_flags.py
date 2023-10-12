# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from crossbench.flags import ChromeFeatures, ChromeFlags, Flags, JSFlags

from tests import run_helper


class TestFlags(unittest.TestCase):

  CLASS = Flags

  def test_construct(self):
    flags = self.CLASS()
    self.assertEqual(len(flags), 0)
    self.assertNotIn("foo", flags)

  def test_construct_dict(self):
    flags = self.CLASS({"--foo": "v1", "--bar": "v2"})
    self.assertIn("--foo", flags)
    self.assertIn("--bar", flags)
    self.assertEqual(flags["--foo"], "v1")
    self.assertEqual(flags["--bar"], "v2")

  def test_construct_list(self):
    flags = self.CLASS(("--foo", "--bar"))
    self.assertIn("--foo", flags)
    self.assertIn("--bar", flags)
    self.assertIsNone(flags["--foo"])
    self.assertIsNone(flags["--bar"])
    with self.assertRaises(AssertionError):
      self.CLASS(("--foo=v1", "--bar=v2"))
    flags = self.CLASS((("--foo", "v3"), "--bar"))
    self.assertEqual(flags["--foo"], "v3")
    self.assertIsNone(flags["--bar"])

  def test_construct_flags(self):
    original_flags = self.CLASS({"--foo": "v1", "--bar": "v2"})
    flags = self.CLASS(original_flags)
    self.assertIn("--foo", flags)
    self.assertIn("--bar", flags)
    self.assertEqual(flags["--foo"], "v1")
    self.assertEqual(flags["--bar"], "v2")

  def test_set(self):
    flags = self.CLASS()
    flags["--foo"] = "v1"
    with self.assertRaises(AssertionError):
      flags["--foo"] = "v2"
    # setting the same value is ok
    flags["--foo"] = "v1"
    self.assertEqual(flags["--foo"], "v1")
    flags.set("--bar")
    self.assertIn("--foo", flags)
    self.assertIn("--bar", flags)
    self.assertIsNone(flags["--bar"])
    with self.assertRaises(AssertionError):
      flags.set("--bar", "v3")
    flags.set("--bar", "v4", override=True)
    self.assertEqual(flags["--foo"], "v1")
    self.assertEqual(flags["--bar"], "v4")

  def test_get_list(self):
    flags = self.CLASS({"--foo": "v1", "--bar": None})
    self.assertEqual(list(flags.get_list()), ["--foo=v1", "--bar"])

  def test_copy(self):
    flags = self.CLASS({"--foo": "v1", "--bar": None})
    copy = flags.copy()
    self.assertEqual(list(flags.get_list()), list(copy.get_list()))

  def test_update(self):
    flags = self.CLASS({"--foo": "v1", "--bar": None})
    with self.assertRaises(AssertionError):
      flags.update({"--bar": "v2"})
    self.assertEqual(flags["--foo"], "v1")
    self.assertIsNone(flags["--bar"])
    flags.update({"--bar": "v2"}, override=True)
    self.assertEqual(flags["--foo"], "v1")
    self.assertEqual(flags["--bar"], "v2")

  def test_str_basic(self):
    flags = self.CLASS({"--foo": None})
    self.assertEqual(str(flags), "--foo")
    flags = self.CLASS({"--foo": "bar"})
    self.assertEqual(str(flags), "--foo=bar")

  def test_str_multiple(self):
    flags = self.CLASS({
        "--flag1": "value1",
        "--flag2": None,
        "--flag3": "value3"
    })
    self.assertEqual(str(flags), "--flag1=value1 --flag2 --flag3=value3")


class TestChromeFlags(TestFlags):

  CLASS = ChromeFlags

  def test_js_flags(self):
    flags = self.CLASS({
        "--foo": None,
        "--bar": "v1",
    })
    self.assertIsNone(flags["--foo"])
    self.assertEqual(flags["--bar"], "v1")
    self.assertNotIn("--js-flags", flags)
    with self.assertRaises(ValueError):
      flags["--js-flags"] = None
    self.assertNotIn("--js-flags", flags)
    with self.assertRaises(AssertionError):
      flags["--js-flags"] = "--js-foo, --no-js-foo"
    flags["--js-flags"] = "--js-foo=v3, --no-js-bar"
    with self.assertRaises(AssertionError):
      flags["--js-flags"] = "--js-foo=v4, --no-js-bar"
    js_flags = flags.js_flags
    self.assertEqual(js_flags["--js-foo"], "v3")
    self.assertIsNone(js_flags["--no-js-bar"])

  def test_js_flags_initial_data(self):
    flags = self.CLASS({
        "--js-flags": "--foo=v1,--no-bar",
    })
    js_flags = flags.js_flags
    self.assertEqual(js_flags["--foo"], "v1")
    self.assertIsNone(js_flags["--no-bar"])

  def test_features(self):
    flags = self.CLASS()
    features = flags.features
    self.assertTrue(features.is_empty)
    flags["--enable-features"] = "F1,F2"
    with self.assertRaises(ValueError):
      flags["--disable-features"] = "F1,F2"
    with self.assertRaises(ValueError):
      flags["--disable-features"] = "F2,F1"
    flags["--disable-features"] = "F3,F4"
    self.assertEqual(features.enabled, {"F1": None, "F2": None})
    self.assertEqual(features.disabled, set(("F3", "F4")))

  def test_features_invalid_none(self):
    flags = self.CLASS()
    features = flags.features
    self.assertTrue(features.is_empty)
    with self.assertRaises(ValueError):
      flags["--disable-features"] = None
    self.assertTrue(features.is_empty)
    with self.assertRaises(ValueError):
      flags["--enable-features"] = None
    self.assertTrue(features.is_empty)

  def test_get_list(self):
    flags = self.CLASS()
    flags["--js-flags"] = "--js-foo=v3, --no-js-bar"
    flags["--enable-features"] = "F1,F2"
    flags["--disable-features"] = "F3,F4"
    flags_list = list(flags.get_list())
    self.assertListEqual(flags_list, [
        "--js-flags=--js-foo=v3,--no-js-bar", "--enable-features=F1,F2",
        "--disable-features=F3,F4"
    ])

  def test_initial_data_empty(self):
    flags = self.CLASS()
    flags_copy = self.CLASS(flags)
    self.assertListEqual(list(flags.get_list()), list(flags_copy.get_list()))
    flags_copy = self.CLASS()
    flags_copy.update(flags)
    self.assertListEqual(list(flags.get_list()), list(flags_copy.get_list()))

  def test_initial_data_simple(self):
    flags = self.CLASS()
    flags["--no-sandbox"] = None
    flags_copy = self.CLASS(flags)
    self.assertListEqual(list(flags.get_list()), list(flags_copy.get_list()))
    flags_copy = self.CLASS()
    flags_copy.update(flags)
    self.assertListEqual(list(flags.get_list()), list(flags_copy.get_list()))

  def test_initial_data_js_flags(self):
    flags = self.CLASS()
    flags["--js-flags"] = "--js-foo=v3, --no-js-bar"
    flags_copy = self.CLASS(flags)
    self.assertListEqual(list(flags.get_list()), list(flags_copy.get_list()))
    flags_copy = self.CLASS()
    flags_copy.update(flags)
    self.assertListEqual(list(flags.get_list()), list(flags_copy.get_list()))

  def test_initial_data_features(self):
    flags = self.CLASS()
    flags["--enable-features"] = "F1,F2"
    flags["--disable-features"] = "F3,F4"
    flags_copy = self.CLASS(flags)
    self.assertListEqual(list(flags.get_list()), list(flags_copy.get_list()))
    flags_copy = self.CLASS()
    flags_copy.update(flags)
    self.assertListEqual(list(flags.get_list()), list(flags_copy.get_list()))

  def test_initial_data_all(self):
    flags = self.CLASS()
    flags["--no-sandbox"] = None
    flags["--js-flags"] = "--js-foo=v3, --no-js-bar"
    flags["--enable-features"] = "F1,F2"
    flags["--disable-features"] = "F3,F4"
    flags_copy = self.CLASS(flags)
    self.assertListEqual(list(flags.get_list()), list(flags_copy.get_list()))
    flags_copy = self.CLASS()
    flags_copy.update(flags)
    self.assertListEqual(list(flags.get_list()), list(flags_copy.get_list()))


class TestJSFlags(TestFlags):

  CLASS = JSFlags

  def test_conflicting_flags(self):
    with self.assertRaises(AssertionError):
      flags = self.CLASS(("--foo", "--no-foo"))
    with self.assertRaises(AssertionError):
      flags = self.CLASS(("--foo", "--nofoo"))
    flags = self.CLASS(("--foo", "--no-bar"))
    self.assertIsNone(flags["--foo"])
    self.assertIsNone(flags["--no-bar"])
    self.assertIn("--foo", flags)
    self.assertNotIn("--no-foo", flags)
    self.assertNotIn("--bar", flags)
    self.assertIn("--no-bar", flags)

  def test_conflicting_override(self):
    flags = self.CLASS(("--foo", "--no-bar"))
    with self.assertRaises(AssertionError):
      flags.set("--no-foo")
    with self.assertRaises(AssertionError):
      flags.set("--nofoo")
    flags.set("--nobar")
    with self.assertRaises(AssertionError):
      flags.set("--bar")
    with self.assertRaises(AssertionError):
      flags.set("--foo", "v2")
    self.assertIsNone(flags["--foo"])
    self.assertIsNone(flags["--no-bar"])
    flags.set("--no-foo", override=True)
    self.assertNotIn("--foo", flags)
    self.assertIn("--no-foo", flags)
    self.assertNotIn("--bar", flags)
    self.assertIn("--no-bar", flags)

  def test_str_multiple(self):
    flags = self.CLASS({
        "--flag1": "value1",
        "--flag2": None,
        "--flag3": "value3"
    })
    self.assertEqual(str(flags), "--flag1=value1,--flag2,--flag3=value3")

  def test_initial_data_empty(self):
    flags = self.CLASS()
    flags_copy = self.CLASS(flags)
    self.assertEqual(str(flags), str(flags_copy))
    flags_copy = self.CLASS()
    flags_copy.update(flags)
    self.assertEqual(str(flags), str(flags_copy))

  def test_initial_data(self):
    flags = self.CLASS({
        "--flag1": "value1",
        "--flag2": None,
        "--flag3": "value3"
    })
    flags_copy = self.CLASS(flags)
    self.assertEqual(str(flags), str(flags_copy))
    flags_copy = self.CLASS()
    flags_copy.update(flags)
    self.assertEqual(str(flags), str(flags_copy))


class ChromeFeaturesTestCase(unittest.TestCase):

  def test_empty(self):
    features = ChromeFeatures()
    self.assertEqual(str(features), "")
    features_list = list(features.get_list())
    self.assertEqual(len(features_list), 0)
    self.assertDictEqual(features.enabled, {})
    self.assertSetEqual(features.disabled, set())

  def test_enable_complex_features(self):
    features = ChromeFeatures()
    features.enable("feature1")
    features.enable("feature2:k1")
    features.enable("feature3:k1/v1/k2/v2")
    features.enable("feature4<Trial1:k1/v1/k2/v2")
    features.enable("feature5<Trial1.Group1:k1/v1/k2/v2")
    features_list = list(features.get_list())
    self.assertEqual(len(features_list), 1)

  def test_disable_complex_features(self):
    features = ChromeFeatures()
    features.disable("feature1")
    features.disable("feature2:k1")
    features.disable("feature3:k1/v1/k2/v2")
    features.disable("feature4<Trial1:k1/v1/k2/v2")
    features.disable("feature5<Trial1.Group1:k1/v1/k2/v2")
    features_list = list(features.get_list())
    self.assertEqual(len(features_list), 1)
    features_str = str(features)
    self.assertIn("feature1", features_str)
    self.assertIn("feature2", features_str)
    self.assertIn("feature3", features_str)
    self.assertIn("feature4", features_str)

  def test_enable_simple(self):
    features = ChromeFeatures()
    features.enable("feature1")
    features.enable("feature2")
    features_list = list(features.get_list())
    self.assertEqual(len(features_list), 1)
    features_str = str(features)
    self.assertEqual(features_str, "--enable-features=feature1,feature2")

  def test_disable_simple(self):
    features = ChromeFeatures()
    features.disable("feature1")
    features.disable("feature2")
    features_list = list(features.get_list())
    self.assertEqual(len(features_list), 1)
    features_str = str(features)
    self.assertEqual(features_str, "--disable-features=feature1,feature2")

  def test_enable_disable(self):
    features = ChromeFeatures()
    features.enable("feature1")
    features.disable("feature2")
    features_list = list(features.get_list())
    self.assertEqual(len(features_list), 2)
    features_str = str(features)
    self.assertEqual(features_str,
                     "--enable-features=feature1 --disable-features=feature2")
    self.assertDictEqual(features.enabled, {"feature1": None})
    self.assertSetEqual(features.disabled, {"feature2"})

  def test_enable_disable_complex(self):
    features = ChromeFeatures()
    features.enable("feature0")
    features.enable("feature1:k1/v1")
    features.enable("feature2<Trial.Group:k2/v2")
    features.disable("feature3:k3/v3")
    self.assertDictEqual(features.enabled, {
        "feature0": None,
        "feature1": ":k1/v1",
        "feature2": "<Trial.Group:k2/v2"
    })
    self.assertSetEqual(features.disabled, {"feature3"})

  def test_conflicting_values_enabled(self):
    features = ChromeFeatures()
    features.enable("feature1")
    features.enable("feature1")
    with self.assertRaises(ValueError):
      features.disable("feature1")
    with self.assertRaises(ValueError):
      features.enable("feature1:k1/v1")
    features_str = str(features)
    self.assertEqual(features_str, "--enable-features=feature1")

  def test_conflicting_values_disabled(self):
    features = ChromeFeatures()
    features.disable("feature1")
    features.disable("feature1")
    with self.assertRaises(ValueError):
      features.enable("feature1")
    features.disable("feature1:k1/v1")
    features_str = str(features)
    self.assertEqual(features_str, "--disable-features=feature1")


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
