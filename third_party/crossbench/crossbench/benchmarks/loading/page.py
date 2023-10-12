# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import datetime as dt
from typing import TYPE_CHECKING, List, Optional, Sequence, Tuple

from crossbench.stories.story import Story

from .action import Action
from .playback_controller import PlaybackController

if TYPE_CHECKING:
  from crossbench.runner.run import Run
  from crossbench.types import JsonDict


class Page(Story, metaclass=abc.ABCMeta):

  url: Optional[str]

  @classmethod
  def all_story_names(cls) -> Tuple[str, ...]:
    return tuple(page.name for page in PAGE_LIST)

  def __init__(self,
               name: str,
               duration: dt.timedelta = dt.timedelta(seconds=15),
               playback: Optional[PlaybackController] = None):
    self._playback = playback or PlaybackController.once()
    super().__init__(name, duration)

  def set_parent(self, parent: Page) -> None:
    # TODO: support nested playback controllers.
    self._playback = PlaybackController.once()
    del parent


class LivePage(Page):
  url: str

  def __init__(self,
               name: str,
               url: str,
               duration: dt.timedelta = dt.timedelta(seconds=15),
               playback: Optional[PlaybackController] = None) -> None:
    super().__init__(name, duration, playback)
    assert url, "Invalid page url"
    self.url: str = url

  def set_duration(self, duration: dt.timedelta) -> None:
    self._duration = duration

  def details_json(self) -> JsonDict:
    result = super().details_json()
    result["url"] = str(self.url)
    return result

  def run(self, run: Run) -> None:
    for _ in self._playback:
      run.browser.show_url(run.runner, self.url)
      run.runner.wait(self.duration)

  def __str__(self) -> str:
    return f"Page(name={self.name}, url={self.url})"


class CombinedPage(Page):

  def __init__(self,
               pages: Sequence[Page],
               name: str = "combined",
               playback: Optional[PlaybackController] = None):
    assert len(pages), "No sub-pages provided for CombinedPage"
    assert len(pages) > 1, "Combined Page needs more than one page"
    self._pages = pages
    duration = dt.timedelta()
    for page in self._pages:
      page.set_parent(self)
      duration += page.duration
    super().__init__(name, duration, playback)
    self.url = None

  def details_json(self) -> JsonDict:
    result = super().details_json()
    result["pages"] = list(page.details_json() for page in self._pages)
    return result

  def run(self, run: Run) -> None:
    for _ in self._playback:
      for page in self._pages:
        page.run(run)

  def __str__(self) -> str:
    combined_name = ",".join(page.name for page in self._pages)
    return f"CombinedPage({combined_name})"


class InteractivePage(Page):

  def __init__(self,
               actions: List[Action],
               name: str,
               playback: Optional[PlaybackController] = None):
    self._name = name
    assert isinstance(actions, list)
    self._actions = actions
    assert self._actions, "Must have at least 1 valid action"
    duration = self._get_duration()
    super().__init__(name, duration, playback)

  @property
  def actions(self) -> List[Action]:
    return self._actions

  def run(self, run: Run) -> None:
    for _ in self._playback:
      for action in self._actions:
        action.run(run)

  def details_json(self) -> JsonDict:
    result = super().details_json()
    result["actions"] = list(action.to_json() for action in self._actions)
    return result

  def _get_duration(self) -> dt.timedelta:
    duration = dt.timedelta()
    for action in self._actions:
      if action.duration is not None:
        duration += action.duration
    return duration


PAGE_LIST = (
    LivePage("amazon", "https://www.amazon.de/s?k=heizkissen",
             dt.timedelta(seconds=5)),
    LivePage("bing", "https://www.bing.com/images/search?q=not+a+squirrel",
             dt.timedelta(seconds=5)),
    LivePage("caf", "http://www.caf.fr", dt.timedelta(seconds=6)),
    LivePage("cnn", "https://cnn.com/", dt.timedelta(seconds=7)),
    LivePage("ecma262", "https://tc39.es/ecma262/#sec-numbers-and-dates",
             dt.timedelta(seconds=10)),
    LivePage("expedia", "https://www.expedia.com/", dt.timedelta(seconds=7)),
    LivePage("facebook", "https://facebook.com/shakira",
             dt.timedelta(seconds=8)),
    LivePage("maps", "https://goo.gl/maps/TEZde4y4Hc6r2oNN8",
             dt.timedelta(seconds=10)),
    LivePage("microsoft", "https://microsoft.com/", dt.timedelta(seconds=6)),
    LivePage("provincial", "http://www.provincial.com",
             dt.timedelta(seconds=6)),
    LivePage("sueddeutsche", "https://www.sueddeutsche.de/wirtschaft",
             dt.timedelta(seconds=8)),
    LivePage("timesofindia", "https://timesofindia.indiatimes.com/",
             dt.timedelta(seconds=8)),
    LivePage("twitter", "https://twitter.com/wernertwertzog?lang=en",
             dt.timedelta(seconds=6)),
)
PAGES = {page.name: page for page in PAGE_LIST}
PAGE_LIST_SMALL = (PAGES["facebook"], PAGES["maps"], PAGES["timesofindia"],
                   PAGES["cnn"])
