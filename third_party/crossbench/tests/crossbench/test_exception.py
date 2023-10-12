# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import unittest
from unittest import mock

from crossbench.exception import (ArgumentTypeMultiException, Entry,
                                  ExceptionAnnotator, MultiException, annotate,
                                  annotate_argparsing)
from tests import run_helper


class ExceptionHandlerTestCase(unittest.TestCase):

  def test_annotate(self):
    with self.assertRaises(MultiException) as cm:
      with annotate("BBB"):
        with annotate("AAA"):
          raise ValueError("an exception")
    exception: MultiException = cm.exception
    annotator: ExceptionAnnotator = exception.annotator
    self.assertTrue(len(annotator.exceptions), 1)
    entry: Entry = annotator.exceptions[0]
    self.assertTupleEqual(entry.info_stack, ("BBB", "AAA"))
    self.assertIsInstance(entry.exception, ValueError)

  def test_annotate_argparse(self):
    with self.assertRaises(ArgumentTypeMultiException) as cm:
      with annotate_argparsing("BBB"):
        with annotate("AAA"):
          with annotate("000"):
            raise ValueError("an exception")
    exception: MultiException = cm.exception
    self.assertIsInstance(exception, argparse.ArgumentTypeError)
    annotator: ExceptionAnnotator = exception.annotator
    self.assertTrue(len(annotator.exceptions), 1)
    entry: Entry = annotator.exceptions[0]
    self.assertTupleEqual(entry.info_stack, ("BBB", "AAA", "000"))
    self.assertIsInstance(entry.exception, ValueError)

  def test_empty(self):
    annotator = ExceptionAnnotator()
    self.assertTrue(annotator.is_success)
    self.assertListEqual(annotator.exceptions, [])
    self.assertListEqual(annotator.to_json(), [])
    with mock.patch("logging.error") as logging_mock:
      annotator.log()
    # No exceptions => no error output
    logging_mock.assert_not_called()

  def test_handle_exception(self):
    annotator = ExceptionAnnotator()
    exception = ValueError("custom message")
    try:
      raise exception
    except ValueError as e:
      annotator.append(e)
    self.assertFalse(annotator.is_success)
    serialized = annotator.to_json()
    self.assertEqual(len(serialized), 1)
    self.assertEqual(serialized[0]["title"], str(exception))
    with mock.patch("logging.debug") as logging_mock:
      annotator.log()
    logging_mock.assert_has_calls([mock.call(exception)])

  def test_handle_rethrow(self):
    annotator = ExceptionAnnotator(throw=True)
    exception = ValueError("custom message")
    with self.assertRaises(ValueError) as cm:
      try:
        raise exception
      except ValueError as e:
        annotator.append(e)
    self.assertEqual(cm.exception, exception)
    self.assertFalse(annotator.is_success)
    serialized = annotator.to_json()
    self.assertEqual(len(serialized), 1)
    self.assertEqual(serialized[0]["title"], str(exception))

  def test_info_stack(self):
    annotator = ExceptionAnnotator(throw=True)
    exception = ValueError("custom message")
    with self.assertRaises(ValueError) as cm, annotator.info(
        "info 1", "info 2"):
      self.assertTupleEqual(annotator.info_stack, ("info 1", "info 2"))
      try:
        raise exception
      except ValueError as e:
        annotator.append(e)
    self.assertEqual(cm.exception, exception)
    self.assertFalse(annotator.is_success)
    self.assertEqual(len(annotator.exceptions), 1)
    entry = annotator.exceptions[0]
    self.assertTupleEqual(entry.info_stack, ("info 1", "info 2"))
    serialized = annotator.to_json()
    self.assertEqual(len(serialized), 1)
    self.assertEqual(serialized[0]["title"], str(exception))
    self.assertEqual(serialized[0]["info_stack"], ("info 1", "info 2"))

  def test_info_stack_logging(self):
    annotator = ExceptionAnnotator()
    try:
      with annotator.info("info 1", "info 2"):
        raise ValueError("custom message")
    except ValueError as e:
      annotator.append(e)
    with self.assertLogs(level="ERROR") as cm:
      annotator.log()
    output = "\n".join(cm.output)
    self.assertIn("info 1", output)
    self.assertIn("info 2", output)
    self.assertIn("custom message", output)

  def test_handle_keyboard_interrupt(self):
    annotator = ExceptionAnnotator()
    keyboard_interrupt = KeyboardInterrupt()
    with mock.patch("sys.exit", side_effect=ValueError) as exit_mock:
      with self.assertRaises(ValueError) as cm:
        try:
          raise keyboard_interrupt
        except KeyboardInterrupt as e:
          annotator.append(e)
      self.assertNotEqual(cm.exception, keyboard_interrupt)
    exit_mock.assert_called_once_with(0)

  def test_extend(self):
    annotator_1 = ExceptionAnnotator()
    try:
      raise ValueError("error_1")
    except ValueError as e:
      annotator_1.append(e)
    annotator_2 = ExceptionAnnotator()
    try:
      raise ValueError("error_2")
    except ValueError as e:
      annotator_2.append(e)
    annotator_3 = ExceptionAnnotator()
    annotator_4 = ExceptionAnnotator()
    self.assertFalse(annotator_1.is_success)
    self.assertFalse(annotator_2.is_success)
    self.assertTrue(annotator_3.is_success)
    self.assertTrue(annotator_4.is_success)

    self.assertEqual(len(annotator_1.exceptions), 1)
    self.assertEqual(len(annotator_2.exceptions), 1)
    annotator_2.extend(annotator_1)
    self.assertEqual(len(annotator_2.exceptions), 2)
    self.assertFalse(annotator_1.is_success)
    self.assertFalse(annotator_2.is_success)

    self.assertEqual(len(annotator_1.exceptions), 1)
    self.assertEqual(len(annotator_3.exceptions), 0)
    self.assertEqual(len(annotator_4.exceptions), 0)
    annotator_3.extend(annotator_1)
    annotator_3.extend(annotator_4)
    self.assertEqual(len(annotator_3.exceptions), 1)
    self.assertFalse(annotator_3.is_success)
    self.assertTrue(annotator_4.is_success)

  def test_extend_nested(self):
    annotator_1 = ExceptionAnnotator()
    annotator_2 = ExceptionAnnotator()
    exception_1 = ValueError("error_1")
    exception_2 = ValueError("error_2")
    with annotator_1.capture("info 1", "info 2", exceptions=(ValueError,)):
      raise exception_1
    self.assertEqual(len(annotator_1.exceptions), 1)
    self.assertEqual(len(annotator_2.exceptions), 0)
    with annotator_1.info("info 1", "info 2"):
      with annotator_2.capture("info 3", "info 4", exceptions=(ValueError,)):
        raise exception_2
      annotator_1.extend(annotator_2, is_nested=True)
    self.assertEqual(len(annotator_1.exceptions), 2)
    self.assertEqual(len(annotator_2.exceptions), 1)
    self.assertTupleEqual(annotator_1.exceptions[0].info_stack,
                          ("info 1", "info 2"))
    self.assertTupleEqual(annotator_1.exceptions[1].info_stack,
                          ("info 1", "info 2", "info 3", "info 4"))
    self.assertTupleEqual(annotator_2.exceptions[0].info_stack,
                          ("info 3", "info 4"))


if __name__ == "__main__":
  run_helper.run_pytest(__file__)
