# Copyright 2019 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Collects information about the SDK and return them as JSON file."""

import argparse
import json
import os
import re
import subprocess
import sys

# Patterns used to extract the Xcode version and build version.
XCODE_VERSION_PATTERN = re.compile(r'Xcode (\d+)\.(\d+)')
XCODE_BUILD_PATTERN = re.compile(r'Build version (.*)')


def GetCommandOutput(command):
  """Returns the output of `command` as a string."""
  return subprocess.check_output(command, encoding='utf-8')


def GetAppleCpuName(target_cpu):
  """Returns the name of the |target_cpu| using Apple's convention."""
  return {
      'x64': 'x86_64',
      'arm': 'armv7',
      'x86': 'i386'
  }.get(target_cpu, target_cpu)


def GetPlatform(target_environment):
  """Returns the platform for |target_environment|."""
  return {
      'simulator': 'iphonesimulator',
      'device': 'iphoneos'
  }[target_environment]


def GetPlaformDisplayName(target_environment):
  """Returns the platform display name for |target_environment|."""
  return {
      'simulator': 'iPhoneSimulator',
      'device': 'iPhoneOS'
  }[target_environment]


def ExtractOSVersion():
  """Extract the version of macOS of the current machine."""
  return GetCommandOutput(['sw_vers', '-buildVersion']).strip()


def ExtractXcodeInfo():
  """Extract Xcode version and build version."""
  version, build = None, None
  for line in GetCommandOutput(['xcodebuild', '-version']).splitlines():
    match = XCODE_VERSION_PATTERN.search(line)
    if match:
      major, minor = match.group(1), match.group(2)
      version = major.rjust(2, '0') + minor.ljust(2, '0')
      continue

    match = XCODE_BUILD_PATTERN.search(line)
    if match:
      build = match.group(1)
      continue

  assert version is not None and build is not None
  return version, build


def ExtractSDKInfo(info, sdk):
  """Extract information about the SDK."""
  return GetCommandOutput(['xcrun', '--sdk', sdk, '--show-sdk-' + info]).strip()


def GetDeveloperDir():
  """Returns the developer dir."""
  return GetCommandOutput(['xcode-select', '-print-path']).strip()


def GetSDKInfoForCpu(target_cpu, environment, sdk_version, deployment_target):
  """Returns a dictionary with information about the SDK."""
  platform = GetPlatform(environment)
  sdk_version = sdk_version or ExtractSDKInfo('version', platform)
  deployment_target = deployment_target or sdk_version

  target = target_cpu + '-apple-ios' + deployment_target
  if environment == 'simulator':
    target = target + '-simulator'

  xcode_version, xcode_build = ExtractXcodeInfo()
  effective_sdk = platform + sdk_version

  sdk_info = {}
  sdk_info['compiler'] = 'com.apple.compilers.llvm.clang.1_0'
  sdk_info['is_simulator'] = environment == 'simulator'
  sdk_info['macos_build'] = ExtractOSVersion()
  sdk_info['platform'] = platform
  sdk_info['platform_name'] = GetPlaformDisplayName(environment)
  sdk_info['sdk'] = effective_sdk
  sdk_info['sdk_build'] = ExtractSDKInfo('build-version', effective_sdk)
  sdk_info['sdk_path'] = ExtractSDKInfo('path', effective_sdk)
  sdk_info['toolchain_path'] = os.path.join(
      GetDeveloperDir(), 'Toolchains/XcodeDefault.xctoolchain')
  sdk_info['sdk_version'] = sdk_version
  sdk_info['target'] = target
  sdk_info['xcode_build'] = xcode_build
  sdk_info['xcode_version'] = xcode_version

  return sdk_info


def ParseArgs(argv):
  """Parses command line arguments."""
  parser = argparse.ArgumentParser(
      description=__doc__,
      formatter_class=argparse.ArgumentDefaultsHelpFormatter)

  parser.add_argument(
      '-t', '--target-cpu', default='x64',
      choices=('x86', 'x64', 'arm', 'arm64'),
      help='target cpu')
  parser.add_argument(
      '-e',
      '--target-environment',
      default='simulator',
      choices=('simulator', 'device'),
      help='target environment')
  parser.add_argument(
      '-s', '--sdk-version',
      help='version of the sdk')
  parser.add_argument(
      '-d', '--deployment-target',
      help='iOS deployment target')
  parser.add_argument(
      '-o', '--output', default='-',
      help='path of the output file to create; - means stdout')

  return parser.parse_args(argv)


def main(argv):
  args = ParseArgs(argv)

  sdk_info = GetSDKInfoForCpu(
      GetAppleCpuName(args.target_cpu), args.target_environment,
      args.sdk_version, args.deployment_target)

  if args.output == '-':
    sys.stdout.write(json.dumps(sdk_info))
  else:
    with open(args.output, 'w') as output:
      output.write(json.dumps(sdk_info))


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
