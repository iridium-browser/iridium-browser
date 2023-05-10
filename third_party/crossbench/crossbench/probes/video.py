# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
import os
import pathlib
import shutil
import subprocess
import tempfile
from typing import TYPE_CHECKING, Dict, List, Tuple

import crossbench
from crossbench import helper
from crossbench.probes import base
from crossbench.probes.results import ProbeResult

#TODO: fix imports
cb = crossbench

if TYPE_CHECKING:
  from crossbench.env import HostEnvironment
  from crossbench.runner import BrowsersRunGroup, RepetitionsRunGroup, Run
  from crossbench.stories import Story


class VideoProbe(base.Probe):
  """
  General-purpose Probe that collects screen-recordings.

  It also produces a timestrip png and creates merged versions of these files
  for visually comparing various browsers / variants / cb.stories
  """
  NAME = "video"
  VIDEO_QUALITY = ["-vcodec", "libx264", "-crf", "20"]
  IMAGE_FORMAT = "png"
  TIMESTRIP_FILE_SUFFIX = f".timestrip.{IMAGE_FORMAT}"

  def __init__(self, *args, **kwargs):
    super().__init__(*args, **kwargs)
    self._duration = None

  @property
  def results_file_name(self) -> str:
    return f"{self.name}.mp4"

  def pre_check(self, env: HostEnvironment) -> None:
    super().pre_check(env)
    if env.runner.repetitions > 10:
      env.handle_warning(
          f"Probe={self.NAME} might not be able to merge so many "
          f"repetitions={env.runner.repetitions}.")
    env.check_installed(
        binaries=("ffmpeg",), message="Missing binaries for video probe: {}")
    # Check that ffmpeg can be executed
    env.check_sh_success("ffmpeg", "-version")
    env.check_installed(
        binaries=("montage",),
        message="Missing 'montage' binary, please install imagemagick.")
    # Check that montage can be executed
    env.check_sh_success("montage", "--version")

  class Scope(base.Probe.Scope):
    IMAGE_FORMAT = "png"
    FFMPEG_TIMELINE_TEXT = (
        "drawtext="
        "fontfile=/Library/Fonts/Arial.ttf:"
        "text='%{eif\\:t\\:d}.%{eif\\:t*100-floor(t)*100\\:d}s':"
        "fontsize=h/16:"
        "y=h-line_h-5:x=5:"
        "box=1:boxborderw=15:boxcolor=white")

    def __init__(self, *args, **kwargs):
      super().__init__(*args, **kwargs)
      self._record_process = None
      self._recorder_log_file = None

    def start(self, run: Run) -> None:
      browser = run.browser
      cmd = self._record_cmd(browser.x, browser.y, browser.width,
                             browser.height)
      self._recorder_log_file = self.results_file.with_suffix(
          ".recorder.log").open(
              "w", encoding="utf-8")
      self._record_process = self.browser_platform.popen(
          *cmd,
          stdin=subprocess.PIPE,
          stderr=subprocess.STDOUT,
          stdout=self._recorder_log_file)
      assert self._record_process is not None, "Could not start screen recorder"
      # TODO: Add common start-story-delay on runner for these cases.
      self.runner_platform.sleep(1)

    def _record_cmd(self, x: int, y: int, width: int,
                    height: int) -> Tuple[str, ...]:
      if self.browser_platform.is_linux:
        env_display = os.environ.get("DISPLAY", ":0.0")
        return ("ffmpeg", "-hide_banner", "-video_size", f"{width}x{height}",
                "-f", "x11grab", "-framerate", "60", "-i",
                f"{env_display}+{x},{y}", self.results_file)
      if self.browser_platform.is_macos:
        return ("/usr/sbin/screencapture", "-v", f"-R{x},{y},{width},{height}",
                self.results_file)
      raise Exception("Invalid platform")

    def stop(self, run: Run) -> None:
      if self.browser_platform.is_macos:
        assert not self._record_process.poll(), (
            "screencapture stopped early. "
            "Please ensure that the parent application has screen recording "
            "permissions")
        # The mac screencapture stops on the first (arbitrary) input.
        self._record_process.communicate(input=b"stop")
      else:
        self._record_process.terminate()

    def tear_down(self, run: Run) -> ProbeResult:
      self._recorder_log_file.close()
      if self._record_process.poll() is not None:
        self._record_process.wait(timeout=5)
      assert self.results_file.exists(), (
          f"No screen recording video found at: {self.results_file}")
      with tempfile.TemporaryDirectory() as tmp_dir:
        timestrip_file = self._create_time_strip(pathlib.Path(tmp_dir))
      return ProbeResult(file=(self.results_file, timestrip_file))

    def _create_time_strip(self, tmpdir: pathlib.Path) -> pathlib.Path:
      logging.info("TIMESTRIP")
      progress_dir = tmpdir / "progress"
      progress_dir.mkdir(parents=True, exist_ok=True)
      timeline_dir = tmpdir / "timeline"
      timeline_dir.mkdir(exist_ok=True)
      # Try detect scene changes / steps
      self.runner_platform.sh(
          "ffmpeg", "-hide_banner", "-i", self.results_file, \
            "-filter_complex", "scale=1000:-2,"
          "select='gt(scene\\,0.011)'," + self.FFMPEG_TIMELINE_TEXT, \
          "-fps_mode", "vfr", \
            f"{progress_dir}/%02d.{self.IMAGE_FORMAT}")
      # Extract at regular intervals of 100ms, assuming 60fps input
      every_nth_frame = 60 / 20
      # TODO
      safe_duration = 10
      safe_duration = 2
      self.runner_platform.sh(
          "ffmpeg", "-hide_banner", "-i", self.results_file, "-filter_complex",
          f"trim=duration={safe_duration},"
          "scale=1000:-2,"
          f"select=not(mod(n\\,{every_nth_frame}))," +
          self.FFMPEG_TIMELINE_TEXT, "-vsync", "vfr",
          f"{timeline_dir}/%02d.{self.IMAGE_FORMAT}")

      timeline_strip_file = self.results_file.with_suffix(
          self.probe.TIMESTRIP_FILE_SUFFIX)
      self.runner.platform.sh("montage",
                              f"{timeline_dir}/*.{self.IMAGE_FORMAT}", "-tile",
                              "x1", "-gravity", "NorthWest", "-geometry",
                              "x100", timeline_strip_file)
      return timeline_strip_file

  def merge_repetitions(self, group: RepetitionsRunGroup) -> ProbeResult:
    result_file = group.get_probe_results_file(self)
    timeline_strip_file = result_file.with_suffix(self.TIMESTRIP_FILE_SUFFIX)
    runs = tuple(group.runs)
    if len(runs) == 1:
      # In the simple case just copy the files
      run_result_file, run_timeline_strip_file = runs[0].results[self].file_list
      # TODO migrate to platform
      shutil.copy(run_result_file, result_file)
      shutil.copy(run_timeline_strip_file, timeline_strip_file)
      return ProbeResult(file=(result_file, timeline_strip_file))
    logging.info("TIMESTRIP merge page iterations")
    timeline_strips = (run.results[self].file_list[1] for run in runs)
    self.runner_platform.sh("montage", *timeline_strips, "-tile", "1x",
                            "-gravity", "NorthWest", "-geometry", "x100",
                            timeline_strip_file)

    logging.info("VIDEO merge page iterations")
    browser = group.browser
    video_file_inputs = []
    for run in runs:
      video_file_inputs += ["-i", run.results[self].file_list[0]]
    draw_text = ("fontfile='/Library/Fonts/Arial.ttf':"
                 f"text='{browser.app_name} {browser.label}':"
                 "fontsize=h/15:"
                 "y=h-line_h-10:x=10:"
                 "box=1:boxborderw=20:boxcolor=white")
    self.runner_platform.sh(
        "ffmpeg", "-hide_banner", *video_file_inputs, "-filter_complex",
        f"hstack=inputs={len(runs)},"
        f"drawtext={draw_text},"
        "scale=3000:-2", *self.VIDEO_QUALITY, result_file)
    return ProbeResult(file=(result_file, timeline_strip_file))

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    """Merge story videos from multiple browser/configurations"""
    groups = list(group.repetitions_groups)
    if len(groups) <= 1:
      return ProbeResult()
    grouped: Dict[Story, List[RepetitionsRunGroup]] = helper.group_by(
        groups, key=lambda repetitions_group: repetitions_group.story)

    result_dir = group.get_probe_results_file(self)
    result_dir = result_dir / result_dir.stem
    result_dir.mkdir(parents=True)
    return ProbeResult(
        file=(self._merge_stories_for_browser(result_dir, story,
                                              repetitions_groups)
              for story, repetitions_groups in grouped.items()))

  def _merge_stories_for_browser(self, result_dir: pathlib.Path, story: Story,
                                 repetitions_groups: List[RepetitionsRunGroup]
                                ) -> pathlib.Path:
    story = repetitions_groups[0].story
    result_file = result_dir / f"{story.name}_combined.mp4"

    if len(repetitions_groups) == 1:
      # In the simple case just copy files
      input_file = repetitions_groups[0].results[self].file_list[0]
      # TODO migrate to platform
      shutil.copy(input_file, result_file)
      return result_file

    input_files: List[str] = []
    for repetitions_group in repetitions_groups:
      result_files = repetitions_group.results[self].file_list
      input_files += ["-i", str(result_files[0])]
    self.runner_platform.sh("ffmpeg", "-hide_banner", *input_files,
                            "-filter_complex",
                            f"vstack=inputs={len(repetitions_groups)}",
                            *self.VIDEO_QUALITY, result_file)
    return result_file
