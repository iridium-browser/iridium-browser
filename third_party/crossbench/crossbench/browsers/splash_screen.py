# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import html
import pathlib
import urllib.parse
from argparse import ArgumentTypeError
from typing import TYPE_CHECKING, Dict

if TYPE_CHECKING:
  from crossbench.runner.run import Run


class SplashScreen:
  NONE: SplashScreen
  MINIMAL: SplashScreen
  DETAILED: SplashScreen
  DEFAULT: SplashScreen

  @classmethod
  def parse(cls, value: str) -> SplashScreen:
    if not value or value == "default":
      return cls.DEFAULT
    if value in ("none", "skip"):
      return cls.NONE
    if value == "minimal":
      return cls.MINIMAL
    if value == "detailed":
      return cls.DETAILED
    if value.startswith("http:") or value.startswith("https:"):
      return URLSplashScreen(value)
    maybe_path = pathlib.Path(value)
    if maybe_path.exists():
      return URLSplashScreen(maybe_path.absolute().as_uri())
    raise ArgumentTypeError(f"Unknown splashscreen: {value}")

  def run(self, run: Run) -> None:
    pass


class BaseURLSplashScreen(SplashScreen, metaclass=abc.ABCMeta):

  def __init__(self, timeout: float = 2) -> None:
    super().__init__()
    self._timeout = timeout

  def run(self, run: Run) -> None:
    with run.actions("SplashScreen") as action:
      action.show_url(self.get_url(run))
      action.wait(self._timeout)

  @abc.abstractmethod
  def get_url(self, run: Run) -> str:
    pass


class DetailedSplashScreen(BaseURLSplashScreen):

  def get_url(self, run: Run) -> str:
    browser = run.browser
    title = html.escape(browser.app_name.title())
    version = html.escape(browser.version)
    page = ("<html><head>"
            f"<title>Run Details</title>"
            "<style>"
            """
            html { font-family: sans-serif; }
            dl {
              display: grid;
              grid-template-columns: max-content auto;
            }
            dt { grid-column-start: 1; }
            dd { grid-column-start: 2;  font-family: monospace; }
        """
            "</style>"
            "</head><body>"
            f"<h1>{title} {version}</h1>")
    page += self._render_browser_details(run)
    page += self._render_run_details(run)
    page += "</body></html>"
    data_url = f"data:text/html;charset=utf-8,{urllib.parse.quote(page)}"
    return data_url

  def _render_properties(self, title: str, properties: Dict[str, str]) -> str:
    section = f"<h2>{html.escape(title)}</h2><dl>"
    for property_name, value in properties.items():
      section += f"<dt>{html.escape(property_name)}</dt>"
      section += f"<dd>{html.escape(str(value))}</dd>"
    section += "</dl>"
    return section

  def _render_browser_details(self, run: Run) -> str:
    browser = run.browser
    properties = {
        "User Agent": browser.user_agent(run.runner),
        **browser.details_json()
    }
    return self._render_properties("Browser Details", properties)

  def _render_run_details(self, run: Run) -> str:
    return self._render_properties("Run Details", run.details_json())


class MinimalSplashScreen(DetailedSplashScreen):

  def _render_browser_details(self, run: Run) -> str:
    properties = {"User Agent": run.browser.user_agent(run.runner)}
    return self._render_properties("Browser Details", properties)


class URLSplashScreen(BaseURLSplashScreen):

  def __init__(self, url: str, timeout: float = 2) -> None:
    super().__init__(timeout)
    self._url = url

  def get_url(self, run: Run) -> str:
    return self._url

  @property
  def url(self) -> str:
    return self._url


SplashScreen.NONE = SplashScreen()
SplashScreen.MINIMAL = MinimalSplashScreen()
SplashScreen.DETAILED = DetailedSplashScreen()
SplashScreen.DEFAULT = SplashScreen.DETAILED
