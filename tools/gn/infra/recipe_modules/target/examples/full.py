# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

DEPS = [
    'target',
    'recipe_engine/platform',
    'recipe_engine/properties',
    'recipe_engine/step',
]


def RunSteps(api):
  target = api.target('fuchsia-arm64')
  assert not target.is_win
  assert not target.is_linux
  assert not target.is_mac
  assert api.target.host.is_host
  assert target != api.target.host
  assert target != 'foo'
  step_result = api.step('platform things', cmd=None)
  step_result.presentation.logs['name'] = [target.os]
  step_result.presentation.logs['arch'] = [target.arch]
  step_result.presentation.logs['platform'] = [target.platform]
  step_result.presentation.logs['triple'] = [target.triple]
  step_result.presentation.logs['string'] = [str(target)]


def GenTests(api):
  for platform in ('linux', 'mac', 'win'):
    yield api.test(platform) + api.platform.name(platform)
