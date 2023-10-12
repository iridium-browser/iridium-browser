#!/usr/bin/env python3
# Copyright (c) 2023 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import io
import os
import os.path
import sys
import tempfile
import unittest
import unittest.mock

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, ROOT_DIR)

import reclient_metrics


class ReclientMetricsTest(unittest.TestCase):
  def test_is_googler(self):
    with unittest.mock.patch('subprocess.run') as run_mock:
      run_mock.return_value.returncode = 0
      run_mock.return_value.stdout = 'Logged in as abc@google.com.'
      self.assertTrue(reclient_metrics.is_googler())

    with unittest.mock.patch('subprocess.run') as run_mock:
      run_mock.return_value.returncode = 1
      self.assertFalse(reclient_metrics.is_googler())

    with unittest.mock.patch('subprocess.run') as run_mock:
      run_mock.return_value.returncode = 0
      run_mock.return_value.stdout = ''
      self.assertFalse(reclient_metrics.is_googler())

    with unittest.mock.patch('subprocess.run') as run_mock:
      run_mock.return_value.returncode = 0
      run_mock.return_value.stdout = 'Logged in as foo@example.com.'
      self.assertFalse(reclient_metrics.is_googler())

    with unittest.mock.patch('subprocess.run') as run_mock:
      self.assertTrue(reclient_metrics.is_googler({
          'is-googler': True,
      }))
      self.assertFalse(reclient_metrics.is_googler({
          'is-googler': False,
      }))
      run_mock.assert_not_called()

  def test_load_and_save_config(self):
    with tempfile.TemporaryDirectory() as tmpdir:
      reclient_metrics.CONFIG = os.path.join(tmpdir, 'reclient_metrics.cfg')
      with unittest.mock.patch('subprocess.run') as run_mock:
        run_mock.return_value.returncode = 0
        run_mock.return_value.stdout = 'Logged in as abc@google.com.'
        cfg1 = reclient_metrics.load_config()
        self.assertDictEqual(
            cfg1, {
                'is-googler': True,
                'countdown': 10,
                'version': reclient_metrics.VERSION,
            })
        reclient_metrics.save_config(cfg1)
        cfg2 = reclient_metrics.load_config()
        self.assertDictEqual(
            cfg2, {
                'is-googler': True,
                'countdown': 9,
                'version': reclient_metrics.VERSION,
            })
        run_mock.assert_called_once()

  def test_check_status(self):
    with tempfile.TemporaryDirectory() as tmpdir:
      reclient_metrics.CONFIG = os.path.join(tmpdir, 'reclient_metrics.cfg')
      with unittest.mock.patch('subprocess.run') as run_mock:
        run_mock.return_value.returncode = 0
        run_mock.return_value.stdout = 'Logged in as abc@google.com.'
        with unittest.mock.patch('sys.stdout',
                                 new=io.StringIO()) as stdout_mock:
          self.assertFalse(reclient_metrics.check_status("outdir"))
          self.assertIn("Your reclient metrics will", stdout_mock.getvalue())
          self.assertIn(
              os.path.join("outdir", ".reproxy_tmp", "logs", "rbe_metrics.txt"),
              stdout_mock.getvalue())
        run_mock.assert_called_once()

    with tempfile.TemporaryDirectory() as tmpdir:
      reclient_metrics.CONFIG = os.path.join(tmpdir, 'reclient_metrics.cfg')
      with unittest.mock.patch('subprocess.run') as run_mock:
        run_mock.return_value.returncode = 0
        run_mock.return_value.stdout = 'Logged in as abc@google.com.'
        for i in range(10):
          with unittest.mock.patch('sys.stdout',
                                   new=io.StringIO()) as stdout_mock:
            self.assertFalse(reclient_metrics.check_status("outdir"))
            self.assertIn("Your reclient metrics will", stdout_mock.getvalue())
            self.assertIn(
                os.path.join("outdir", ".reproxy_tmp", "logs",
                             "rbe_metrics.txt"), stdout_mock.getvalue())
            self.assertIn("you run autoninja another %d time(s)" % (10 - i),
                          stdout_mock.getvalue())
        with unittest.mock.patch('sys.stdout',
                                 new=io.StringIO()) as stdout_mock:
          self.assertTrue(reclient_metrics.check_status("outdir"))
          self.assertNotIn("Your reclient metrics will", stdout_mock.getvalue())
          self.assertNotIn(
              os.path.join("outdir", ".reproxy_tmp", "logs", "rbe_metrics.txt"),
              stdout_mock.getvalue())
        run_mock.assert_called_once()

    with tempfile.TemporaryDirectory() as tmpdir:
      reclient_metrics.CONFIG = os.path.join(tmpdir, 'reclient_metrics.cfg')
      with unittest.mock.patch('subprocess.run') as run_mock:
        run_mock.return_value.returncode = 0
        run_mock.return_value.stdout = 'Logged in as abc@example.com.'
        with unittest.mock.patch('sys.stdout',
                                 new=io.StringIO()) as stdout_mock:
          self.assertFalse(reclient_metrics.check_status("outdir"))
          self.assertNotIn("Your reclient metrics will", stdout_mock.getvalue())
          self.assertNotIn(
              os.path.join("outdir", ".reproxy_tmp", "logs", "rbe_metrics.txt"),
              stdout_mock.getvalue())
        run_mock.assert_called_once()

    with tempfile.TemporaryDirectory() as tmpdir:
      reclient_metrics.CONFIG = os.path.join(tmpdir, 'reclient_metrics.cfg')
      with unittest.mock.patch('subprocess.run') as run_mock:
        run_mock.return_value.returncode = 1
        run_mock.return_value.stdout = ''
        with unittest.mock.patch('sys.stdout',
                                 new=io.StringIO()) as stdout_mock:
          self.assertFalse(reclient_metrics.check_status("outdir"))
          self.assertNotIn("Your reclient metrics will", stdout_mock.getvalue())
          self.assertNotIn(
              os.path.join("outdir", ".reproxy_tmp", "logs", "rbe_metrics.txt"),
              stdout_mock.getvalue())
        run_mock.assert_called_once()

    with tempfile.TemporaryDirectory() as tmpdir:
      reclient_metrics.CONFIG = os.path.join(tmpdir, 'reclient_metrics.cfg')
      with unittest.mock.patch('subprocess.run') as run_mock:
        run_mock.return_value.returncode = 0
        run_mock.return_value.stdout = 'Logged in as abc@google.com.'
        reclient_metrics.main(["reclient_metrics.py", "opt-in"])
        with unittest.mock.patch('sys.stdout',
                                 new=io.StringIO()) as stdout_mock:
          self.assertTrue(reclient_metrics.check_status("outdir"))
          self.assertNotIn("Your reclient metrics will", stdout_mock.getvalue())
          self.assertNotIn(
              os.path.join("outdir", ".reproxy_tmp", "logs", "rbe_metrics.txt"),
              stdout_mock.getvalue())
        run_mock.assert_called_once()

    with tempfile.TemporaryDirectory() as tmpdir:
      reclient_metrics.CONFIG = os.path.join(tmpdir, 'reclient_metrics.cfg')
      with unittest.mock.patch('subprocess.run') as run_mock:
        run_mock.return_value.returncode = 0
        run_mock.return_value.stdout = 'Logged in as abc@google.com.'
        for i in range(3):
          with unittest.mock.patch('sys.stdout',
                                   new=io.StringIO()) as stdout_mock:
            self.assertFalse(reclient_metrics.check_status("outdir"))
            self.assertIn("Your reclient metrics will", stdout_mock.getvalue())
            self.assertIn(
                os.path.join("outdir", ".reproxy_tmp", "logs",
                             "rbe_metrics.txt"), stdout_mock.getvalue())
            self.assertIn("you run autoninja another %d time(s)" % (10 - i),
                          stdout_mock.getvalue())
        reclient_metrics.main(["reclient_metrics.py", "opt-in"])
        with unittest.mock.patch('sys.stdout',
                                 new=io.StringIO()) as stdout_mock:
          self.assertTrue(reclient_metrics.check_status("outdir"))
          self.assertNotIn("Your reclient metrics will", stdout_mock.getvalue())
          self.assertNotIn(
              os.path.join("outdir", ".reproxy_tmp", "logs", "rbe_metrics.txt"),
              stdout_mock.getvalue())
        run_mock.assert_called_once()

    with tempfile.TemporaryDirectory() as tmpdir:
      reclient_metrics.CONFIG = os.path.join(tmpdir, 'reclient_metrics.cfg')
      with unittest.mock.patch('subprocess.run') as run_mock:
        run_mock.return_value.returncode = 0
        run_mock.return_value.stdout = 'Logged in as abc@example.com.'
        with unittest.mock.patch('sys.stdout',
                                 new=io.StringIO()) as stdout_mock:
          self.assertFalse(reclient_metrics.check_status("outdir"))
          self.assertNotIn("Your reclient metrics will", stdout_mock.getvalue())
          self.assertNotIn(
              os.path.join("outdir", ".reproxy_tmp", "logs", "rbe_metrics.txt"),
              stdout_mock.getvalue())
        reclient_metrics.main(["reclient_metrics.py", "opt-in"])
        with unittest.mock.patch('sys.stdout',
                                 new=io.StringIO()) as stdout_mock:
          self.assertFalse(reclient_metrics.check_status("outdir"))
          self.assertNotIn("Your reclient metrics will", stdout_mock.getvalue())
          self.assertNotIn(
              os.path.join("outdir", ".reproxy_tmp", "logs", "rbe_metrics.txt"),
              stdout_mock.getvalue())
        run_mock.assert_called_once()

    with tempfile.TemporaryDirectory() as tmpdir:
      reclient_metrics.CONFIG = os.path.join(tmpdir, 'reclient_metrics.cfg')
      with unittest.mock.patch('subprocess.run') as run_mock:
        run_mock.return_value.returncode = 0
        run_mock.return_value.stdout = 'Logged in as abc@google.com.'
        for i in range(3):
          with unittest.mock.patch('sys.stdout',
                                   new=io.StringIO()) as stdout_mock:
            self.assertFalse(reclient_metrics.check_status("outdir"))
            self.assertIn("Your reclient metrics will", stdout_mock.getvalue())
            self.assertIn(
                os.path.join("outdir", ".reproxy_tmp", "logs",
                             "rbe_metrics.txt"), stdout_mock.getvalue())
            self.assertIn("you run autoninja another %d time(s)" % (10 - i),
                          stdout_mock.getvalue())
        reclient_metrics.main(["reclient_metrics.py", "opt-out"])
        with unittest.mock.patch('sys.stdout',
                                 new=io.StringIO()) as stdout_mock:
          self.assertFalse(reclient_metrics.check_status("outdir"))
          self.assertNotIn("Your reclient metrics will", stdout_mock.getvalue())
          self.assertNotIn(
              os.path.join("outdir", ".reproxy_tmp", "logs", "rbe_metrics.txt"),
              stdout_mock.getvalue())
        run_mock.assert_called_once()


if __name__ == '__main__':
  unittest.main()
