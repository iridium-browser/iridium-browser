# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations
from typing import Final, Tuple

from .jetstream_2 import (JetStream2Probe, JetStream2Story, JetStream2Benchmark,
                          ProbeClsTupleT)


class JetStream20Probe(JetStream2Probe):
  __doc__ = JetStream2Probe.__doc__
  NAME: str = "jetstream_2.0"


class JetStream20Story(JetStream2Story):
  __doc__ = JetStream2Story.__doc__
  NAME: str = "jetstream_2.0"
  URL: str = "https://browserbench.org/JetStream2.0/"
  PROBES: ProbeClsTupleT = (JetStream20Probe,)


class JetStream20Benchmark(JetStream2Benchmark):
  """
  Benchmark runner for JetStream 2.0.
  """

  NAME: str = "jetstream_2.0"
  DEFAULT_STORY_CLS = JetStream20Story

  @classmethod
  def version(cls) -> Tuple[int, ...]:
    return (2, 0)
