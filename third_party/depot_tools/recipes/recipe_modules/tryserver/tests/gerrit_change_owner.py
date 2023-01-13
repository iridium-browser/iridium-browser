# Copyright 2020 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from recipe_engine import post_process

PYTHON_VERSION_COMPATIBILITY = 'PY2+3'

DEPS = [
    'recipe_engine/buildbucket',
    'gerrit',
    'tryserver',
]


def RunSteps(api):
  api.tryserver.gerrit_change_owner


def GenTests(api):
  yield api.test(
      'basic',
      api.buildbucket.try_build()
  )
