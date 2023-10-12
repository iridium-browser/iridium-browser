# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
from argparse import ArgumentTypeError

from crossbench.browsers.viewport import Viewport, ViewportMode
from tests import run_helper


class ViewportTestCase(unittest.TestCase):

  def test_is_default(self):
    self.assertTrue(Viewport.DEFAULT.is_default)
    self.assertFalse(Viewport.FULLSCREEN.is_default)
    self.assertFalse(Viewport.MAXIMIZED.is_default)
    self.assertFalse(Viewport.HEADLESS.is_default)
    self.assertFalse(Viewport().is_default)

  def test_has_size(self):
    self.assertTrue(Viewport.DEFAULT.has_size)
    self.assertFalse(Viewport.FULLSCREEN.has_size)
    self.assertFalse(Viewport.MAXIMIZED.has_size)
    self.assertFalse(Viewport.HEADLESS.has_size)
    self.assertTrue(Viewport().has_size)

  def test_is_maximized(self):
    self.assertFalse(Viewport.DEFAULT.is_maximized)
    self.assertFalse(Viewport.FULLSCREEN.is_maximized)
    self.assertTrue(Viewport.MAXIMIZED.is_maximized)
    self.assertFalse(Viewport.HEADLESS.is_maximized)
    self.assertFalse(Viewport().is_maximized)

  def test_is_fullscreen(self):
    self.assertFalse(Viewport.DEFAULT.is_fullscreen)
    self.assertTrue(Viewport.FULLSCREEN.is_fullscreen)
    self.assertFalse(Viewport.MAXIMIZED.is_fullscreen)
    self.assertFalse(Viewport.HEADLESS.is_fullscreen)
    self.assertFalse(Viewport().is_fullscreen)

  def test_is_headless(self):
    self.assertFalse(Viewport.DEFAULT.is_headless)
    self.assertFalse(Viewport.FULLSCREEN.is_headless)
    self.assertFalse(Viewport.MAXIMIZED.is_headless)
    self.assertTrue(Viewport.HEADLESS.is_headless)
    self.assertFalse(Viewport().is_headless)

  def test_mode(self):
    self.assertEqual(Viewport.DEFAULT.mode, ViewportMode.SIZE)
    self.assertEqual(Viewport.FULLSCREEN.mode, ViewportMode.FULLSCREEN)
    self.assertEqual(Viewport.MAXIMIZED.mode, ViewportMode.MAXIMIZED)
    self.assertEqual(Viewport.HEADLESS.mode, ViewportMode.HEADLESS)
    self.assertEqual(Viewport().mode, ViewportMode.SIZE)

  NON_SIZED_DEFAULTS = (Viewport.FULLSCREEN, Viewport.MAXIMIZED,
                        Viewport.HEADLESS)

  def test_position(self):
    for invalid in self.NON_SIZED_DEFAULTS:
      with self.assertRaises(AssertionError):
        _ = invalid.position
    self.assertTupleEqual(Viewport.DEFAULT.position, (10, 50))
    self.assertTupleEqual(Viewport(x=100, y=200).position, (100, 200))

  def test_size(self):
    for invalid in self.NON_SIZED_DEFAULTS:
      with self.assertRaises(AssertionError):
        _ = invalid.size
    self.assertTupleEqual(Viewport.DEFAULT.size, (1500, 1000))
    self.assertTupleEqual(Viewport(100, 200).size, (100, 200))

  def test_width(self):
    for invalid in self.NON_SIZED_DEFAULTS:
      with self.assertRaises(AssertionError):
        _ = invalid.width
    self.assertEqual(Viewport.DEFAULT.width, 1500)
    self.assertEqual(Viewport(100, 200).width, 100)

  def test_height(self):
    for invalid in self.NON_SIZED_DEFAULTS:
      with self.assertRaises(AssertionError):
        _ = invalid.height
    self.assertEqual(Viewport.DEFAULT.height, 1000)
    self.assertEqual(Viewport(100, 200).height, 200)

  def test_x(self):
    for invalid in self.NON_SIZED_DEFAULTS:
      with self.assertRaises(AssertionError):
        _ = invalid.x
    self.assertEqual(Viewport.DEFAULT.x, 10)
    self.assertEqual(Viewport(x=100, y=200).x, 100)

  def test_y(self):
    for invalid in self.NON_SIZED_DEFAULTS:
      with self.assertRaises(AssertionError):
        _ = invalid.y
    self.assertEqual(Viewport.DEFAULT.y, 50)
    self.assertEqual(Viewport(x=100, y=200).y, 200)

  def test_parse_invalid(self):
    INVALID_INPUTS = ("-", "-100x", "100x", "100xXX", "100x-100", "-100x100",
                      "asdf", "100x100,,", "100x100,a", "100x100,100",
                      "100x100,100x", "100x100,-100", "100x100,100x-100",
                      "100x100,-100x100")
    for invalid in INVALID_INPUTS:
      with self.subTest(value=invalid):
        with self.assertRaises((ValueError, ArgumentTypeError)):
          Viewport.parse(invalid)

  def test_parse_placeholders(self):
    for value in ("m", "max", "maximised", "maximized"):
      self.assertTrue(Viewport.parse(value).is_maximized)
    for value in ("f", "full", "fullscreen"):
      self.assertTrue(Viewport.parse(value).is_fullscreen)
    self.assertTrue(Viewport.parse("headless").is_headless)
    self.assertTrue(Viewport.parse("").is_default)

  def test_non_size_wrong_params(self):
    with self.assertRaises(ArgumentTypeError):
      Viewport(width=100, height=0, x=0, y=0, mode=ViewportMode.MAXIMIZED)
    with self.assertRaises(ArgumentTypeError):
      Viewport(width=0, height=100, x=0, y=0, mode=ViewportMode.MAXIMIZED)
    with self.assertRaises(ArgumentTypeError):
      Viewport(width=0, height=0, x=100, y=0, mode=ViewportMode.MAXIMIZED)
    with self.assertRaises(ArgumentTypeError):
      Viewport(width=0, height=0, x=0, y=100, mode=ViewportMode.MAXIMIZED)

  def test_parse(self):
    viewport: Viewport = Viewport.parse("100x200")
    self.assertTupleEqual(viewport.size, (100, 200))
    self.assertTupleEqual(viewport.position, Viewport.DEFAULT.position)
    viewport = Viewport.parse("100x200,22x33")
    self.assertTupleEqual(viewport.size, (100, 200))
    self.assertTupleEqual(viewport.position, (22, 33))


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
