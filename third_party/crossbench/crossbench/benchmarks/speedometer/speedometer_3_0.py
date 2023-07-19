# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations
from typing import Final

from .speedometer import (SpeedometerProbe, SpeedometerStory,
                          SpeedometerBenchmark, ProbeClsTupleT)


class Speedometer30Probe(SpeedometerProbe):
  """
  Speedometer3-specific probe (compatible with v3.0).
  Extracts all speedometer times and scores.
  """
  NAME: Final[str] = "speedometer_3.0"


class Speedometer30Story(SpeedometerStory):
  __doc__ = SpeedometerStory.__doc__
  URL_LOCAL: str = "http://127.0.0.1:7000"
  NAME: Final[str] = "speedometer_3.0"
  PROBES: Final[ProbeClsTupleT] = (Speedometer30Probe,)
  # TODO: Update once public version is available
  URL: Final[str] = "https://browserbench.org/Speedometer3.0/"
  SUBSTORIES = (
      "TodoMVC-JavaScript-ES5",
      "TodoMVC-JavaScript-ES6",
      "TodoMVC-JavaScript-ES6-Webpack",
      "TodoMVC-React",
      "TodoMVC-React-Redux",
      "TodoMVC-Ember",
      "TodoMVC-Backbone",
      "TodoMVC-Angular",
      "TodoMVC-Vue",
      "TodoMVC-jQuery",
      "TodoMVC-Preact",
      "Editor-CodeMirror",
      "Editor-TipTap",
      "React-Stockcharts",
      "React-Stockcharts-SVG",
  )


class Speedometer30Benchmark(SpeedometerBenchmark):
  """
  Benchmark runner for Speedometer 3.0
  """
  NAME: Final[str] = "speedometer_3.0"
  DEFAULT_STORY_CLS = Speedometer30Story
