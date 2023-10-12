# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations
from typing import Final, Tuple

from .jetstream_2 import (JetStream2Probe, JetStream2Story, JetStream2Benchmark,
                          ProbeClsTupleT)


class JetStream21Probe(JetStream2Probe):
  __doc__ = JetStream2Probe.__doc__
  NAME: str = "jetstream_2.1"


class JetStream21Story(JetStream2Story):
  __doc__ = JetStream2Story.__doc__
  NAME: str = "jetstream_2.1"
  URL: str = "https://browserbench.org/JetStream2.1/"
  PROBES: ProbeClsTupleT = (JetStream21Probe,)


class JetStream21Benchmark(JetStream2Benchmark):
  """
  Benchmark runner for JetStream 2.1.
  """

  NAME: str = "jetstream_2.1"
  DEFAULT_STORY_CLS = JetStream21Story

  @classmethod
  def version(cls) -> Tuple[int, ...]:
    return (2, 1)

  @classmethod
  def aliases(cls) -> Tuple[str, ...]:
    return ("js", "jetstream", "js2", "jetstream_2") + super().aliases()