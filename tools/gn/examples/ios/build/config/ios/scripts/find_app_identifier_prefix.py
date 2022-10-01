# Copyright 2019 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Finds the team identifier to use for code signing bundle given its
bundle identifier.
"""

import argparse
import fnmatch
import glob
import json
import os
import plistlib
import subprocess
import sys


class ProvisioningProfile(object):

  def __init__(self, mobileprovision_path):
    self._path = mobileprovision_path
    self._data = plistlib.loads(
        subprocess.check_output(
            ['security', 'cms', '-D', '-i', mobileprovision_path]))

  @property
  def application_identifier_pattern(self):
    return self._data.get('Entitlements', {}).get('application-identifier', '')

  @property
  def app_identifier_prefix(self):
    return self._data.get('ApplicationIdentifierPrefix', [''])[0]

  def ValidToSignBundle(self, bundle_identifier):
    """Returns whether the provisioning profile can sign |bundle_identifier|."""
    return fnmatch.fnmatch(
        self.app_identifier_prefix + '.' + bundle_identifier,
        self.application_identifier_pattern)


def GetProvisioningProfilesDir():
  """Returns the location of the locally installed provisioning profiles."""
  return os.path.join(
      os.environ['HOME'], 'Library', 'MobileDevice', 'Provisioning Profiles')


def ListProvisioningProfiles():
  """Returns a list of all installed provisioning profiles."""
  return glob.glob(
      os.path.join(GetProvisioningProfilesDir(), '*.mobileprovision'))


def LoadProvisioningProfile(mobileprovision_path):
  """Loads the Apple Property List embedded in |mobileprovision_path|."""
  return ProvisioningProfile(mobileprovision_path)


def ListValidProvisioningProfiles(bundle_identifier):
  """Returns a list of provisioning profile valid for |bundle_identifier|."""
  result = []
  for mobileprovision_path in ListProvisioningProfiles():
    mobileprovision = LoadProvisioningProfile(mobileprovision_path)
    if mobileprovision.ValidToSignBundle(bundle_identifier):
      result.append(mobileprovision)
  return result


def FindProvisioningProfile(bundle_identifier):
  """Returns the path to the provisioning profile for |bundle_identifier|."""
  return max(
      ListValidProvisioningProfiles(bundle_identifier),
      key=lambda p: len(p.application_identifier_pattern))


def GenerateSubsitutions(bundle_identifier, mobileprovision):
  if mobileprovision:
    app_identifier_prefix = mobileprovision.app_identifier_prefix + '.'
  else:
    app_identifier_prefix = '*.'

  return {
      'CFBundleIdentifier': bundle_identifier,
      'AppIdentifierPrefix': app_identifier_prefix
  }


def ParseArgs(argv):
  """Parses command line arguments."""
  parser = argparse.ArgumentParser(
      description=__doc__,
      formatter_class=argparse.ArgumentDefaultsHelpFormatter)

  parser.add_argument(
      '-b', '--bundle-identifier', required=True,
      help='bundle identifier for the application')
  parser.add_argument(
      '-o', '--output', default='-',
      help='path to the result; - means stdout')

  return parser.parse_args(argv)


def main(argv):
  args = ParseArgs(argv)

  mobileprovision = FindProvisioningProfile(args.bundle_identifier)
  substitutions = GenerateSubsitutions(args.bundle_identifier, mobileprovision)

  if args.output == '-':
    sys.stdout.write(json.dumps(substitutions))
  else:
    with open(args.output, 'w') as output:
      output.write(json.dumps(substitutions))


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
