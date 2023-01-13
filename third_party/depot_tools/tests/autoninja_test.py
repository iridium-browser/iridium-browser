#!/usr/bin/env python3
# Copyright (c) 2022 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import multiprocessing
import os
import os.path
import sys
import unittest
import unittest.mock

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, ROOT_DIR)

import autoninja


class AutoninjaTest(unittest.TestCase):
  def test_autoninja(self):
    autoninja.main([])

  def test_autoninja_goma(self):
    with unittest.mock.patch(
        'os.path.exists',
        return_value=True) as mock_exists, unittest.mock.patch(
            'autoninja.open', unittest.mock.mock_open(
                read_data='use_goma=true')) as mock_open, unittest.mock.patch(
                    'subprocess.call', return_value=0):
      args = autoninja.main([]).split()
      mock_exists.assert_called()
      mock_open.assert_called_once()

    self.assertIn('-j', args)
    parallel_j = int(args[args.index('-j') + 1])
    self.assertGreater(parallel_j, multiprocessing.cpu_count())


if __name__ == '__main__':
  unittest.main()
