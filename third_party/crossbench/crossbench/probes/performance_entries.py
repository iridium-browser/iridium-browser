# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from __future__ import annotations

from typing import TYPE_CHECKING, Any, Dict

from crossbench.probes import helper as probes_helper
from crossbench.probes.json import JsonResultProbe

if TYPE_CHECKING:
  from crossbench.probes.results import ProbeResult
  from crossbench.browsers.base import Browser
  from crossbench.runner import (Actions, BrowsersRunGroup, StoriesRunGroup)


class PerformanceEntriesProbe(JsonResultProbe):
  """
  Extract all JavaScript PerformanceEntry [1] from a website.
  Website owners can define more entries via `performance.mark()`.

  [1] https://developer.mozilla.org/en-US/docs/Web/API/PerformanceEntry
  """
  NAME = "performance.entries"

  def is_compatible(self, browser: Browser) -> bool:
    return hasattr(browser, "js")

  def to_json(self, actions: Actions) -> Dict[str, Any]:
    return actions.js("""
      let data = { __proto__: null, paint: {}, mark: {}};
      for (let entryType of Object.keys(data)) {
        for (let entry of performance.getEntriesByType(entryType)) {
           const typeData = data[entryType];
           let values = typeData[entry.name];
           if (values === undefined) {
             values = typeData[entry.name] = {startTime:[], duration:[]};
           }
           for (let metricName of Object.keys(values)) {
            values[metricName].push(entry[metricName]);
          }
        }
      }
      return data;
      """)

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    merged = probes_helper.ValuesMerger.merge_json_list(
        story_group.results[self].json
        for story_group in group.repetitions_groups)
    return self.write_group_result(group, merged)

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    return self.merge_browsers_json_list(group)
