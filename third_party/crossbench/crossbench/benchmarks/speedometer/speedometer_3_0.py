# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Final, Tuple

from .speedometer import (ProbeClsTupleT, SpeedometerBenchmark,
                          SpeedometerProbe, SpeedometerStory)


class Speedometer30Probe(SpeedometerProbe):
  """
  Speedometer3-specific probe (compatible with v3.0).
  Extracts all speedometer times and scores.
  """
  NAME: str = "speedometer_3.0"


class Speedometer30Story(SpeedometerStory):
  __doc__ = SpeedometerStory.__doc__
  NAME: str = "speedometer_3.0"
  PROBES: ProbeClsTupleT = (Speedometer30Probe,)
  # TODO: Update once public version is available
  URL: str = "https://sp3-alpha-testing.netlify.app/"
  URL_LOCAL: str = "http://127.0.0.1:7000"
  SUBSTORIES = (
      "TodoMVC-JavaScript-ES5",
      "TodoMVC-JavaScript-ES6",
      "TodoMVC-JavaScript-ES6-Webpack",
      "TodoMVC-WebComponents",
      "TodoMVC-React",
      "TodoMVC-React-Complex-DOM",
      "TodoMVC-React-Redux",
      "TodoMVC-Backbone",
      "TodoMVC-Angular",
      "TodoMVC-Vue",
      "TodoMVC-jQuery",
      "TodoMVC-Preact",
      "TodoMVC-Svelte",
      "TodoMVC-Lit",
      "NewsSite-Next",
      "NewsSite-Nuxt",
      "Editor-CodeMirror",
      "Editor-TipTap",
      "Charts-observable-plot",
      "Charts-chartjs",
      "React-Stockcharts-SVG",
      "Perf-Dashboard",
  )

class Speedometer30Benchmark(SpeedometerBenchmark):
  """
  Benchmark runner for Speedometer 3.0
  """
  NAME: str = "speedometer_3.0"
  DEFAULT_STORY_CLS = Speedometer30Story

  @classmethod
  def version(cls) -> Tuple[int, ...]:
    return (3, 0)

  @classmethod
  def aliases(cls) -> Tuple[str, ...]:
    return ("sp3", "speedometer_3") + super().aliases()
