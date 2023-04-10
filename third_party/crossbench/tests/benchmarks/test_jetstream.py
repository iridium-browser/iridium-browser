# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

import pytest

from crossbench.benchmarks import jetstream
from tests.benchmarks import jetstream_helper


class JetStream20TestCase(jetstream_helper.JetStream2BaseTestCase):

  @property
  def benchmark_cls(self):
    return jetstream.JetStream20Benchmark

  @property
  def story_cls(self):
    return jetstream.JetStream20Story

  @property
  def probe_cls(self):
    return jetstream.JetStream20Probe

  @property
  def name(self):
    return "jetstream_2.0"


class JetStream21TestCase(jetstream_helper.JetStream2BaseTestCase):

  @property
  def benchmark_cls(self):
    return jetstream.JetStream21Benchmark

  @property
  def story_cls(self):
    return jetstream.JetStream21Story

  @property
  def probe_cls(self):
    return jetstream.JetStream21Probe

  @property
  def name(self):
    return "jetstream_2.1"


if __name__ == "__main__":
  sys.exit(pytest.main([__file__]))
