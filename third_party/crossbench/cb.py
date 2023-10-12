#!/usr/bin/env vpython3
# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

from crossbench.cli import CrossBenchCLI

if __name__ == "__main__":
  argv = sys.argv
  cli = CrossBenchCLI()
  cli.run(argv[1:])
