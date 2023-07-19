# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from recipe_engine import recipe_api


PLATFORM_TO_TRIPLE = {
  'fuchsia-amd64': 'x86_64-fuchsia',
  'fuchsia-arm64': 'aarch64-fuchsia',
  'linux-amd64': 'x86_64-linux-gnu',
  'linux-arm64': 'aarch64-linux-gnu',
  'mac-amd64': 'x86_64-apple-darwin',
  'mac-arm64': 'arm64-apple-darwin',
}
PLATFORMS = PLATFORM_TO_TRIPLE.keys()


class Target(object):

  def __init__(self, api, os, arch):
    self.m = api
    self._os = os
    self._arch = arch

  @property
  def is_win(self):
    """Returns True iff the target platform is Windows."""
    return self.os == 'windows'

  @property
  def is_mac(self):
    """Returns True iff the target platform is macOS."""
    return self.os == 'mac'

  @property
  def is_linux(self):
    """Returns True iff the target platform is Linux."""
    return self.os == 'linux'

  @property
  def is_host(self):
    """Returns True iff the target platform is host."""
    return self == self.m.host

  @property
  def os(self):
    """Returns the target os name which will be in:
      * windows
      * mac
      * linux
    """
    return self._os

  @property
  def arch(self):
    """Returns the current CPU architecture."""
    return self._arch

  @property
  def platform(self):
    """Returns the target platform in the <os>-<arch> format."""
    return '%s-%s' % (self.os, self.arch)

  @property
  def triple(self):
    """Returns the target triple."""
    return PLATFORM_TO_TRIPLE[self.platform]

  def __str__(self):
    return self.platform

  def __eq__(self, other):
    if isinstance(other, Target):
      return self._os == other._os and self._arch == other._arch
    return False

  def __ne__(self, other):
    return not self.__eq__(other)


class TargetApi(recipe_api.RecipeApi):

  def __call__(self, platform):
    return Target(self, *platform.split('-', 2))

  @property
  def host(self):
    return Target(self, self.m.platform.name.replace('win', 'windows'), {
        'intel': {
            32: '386',
            64: 'amd64',
        },
        'arm': {
            32: 'armv6',
            64: 'arm64',
        },
    }[self.m.platform.arch][self.m.platform.bits])
