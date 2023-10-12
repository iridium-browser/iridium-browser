# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations
from typing import Final, Tuple

from .speedometer import SpeedometerStory

class Speedometer2Story(SpeedometerStory):
  __doc__ = SpeedometerStory.__doc__
  SUBSTORIES: Tuple[str, ...] = (
      "VanillaJS-TodoMVC",
      "Vanilla-ES2015-TodoMVC",
      "Vanilla-ES2015-Babel-Webpack-TodoMVC",
      "React-TodoMVC",
      "React-Redux-TodoMVC",
      "EmberJS-TodoMVC",
      "EmberJS-Debug-TodoMVC",
      "BackboneJS-TodoMVC",
      "AngularJS-TodoMVC",
      "Angular2-TypeScript-TodoMVC",
      "VueJS-TodoMVC",
      "jQuery-TodoMVC",
      "Preact-TodoMVC",
      "Inferno-TodoMVC",
      "Elm-TodoMVC",
      "Flight-TodoMVC",
  )
