#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import print_function
from __future__ import with_statement

import os.path
import platform
import re
import shutil
import subprocess
import sys
import time

from buildbot_lib import (
    BuildContext, BuildStatus, Command, EnsureDirectoryExists, GNArch,
    ParseStandardCommandLine, RemoveDirectory, RemovePath,
    RemoveGypBuildDirectories, RemoveSconsBuildDirectories, RunBuild, SCons,
    SetupLinuxEnvironment, SetupWindowsEnvironment,
    Step, StepLink, StepText, TryToCleanContents,
    RunningOnBuildbot)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_DIR = os.path.dirname(SCRIPT_DIR)
ROOT_DIR = os.path.dirname(NACL_DIR)
NINJA_PATH = os.path.join(ROOT_DIR, 'third_party', 'ninja', 'ninja')


def SetupContextVars(context):
  # The branch is set to native_client on the main bots, on the trybots it's
  # set to ''.  Otherwise, we should assume a particular branch is being used.
  context['branch'] = os.environ.get('BUILDBOT_BRANCH', 'native_client')
  context['off_trunk'] = context['branch'] not in ['native_client', '']


def ValidatorTest(context, architecture, validator, warn_only=False):
  cmd = [
      sys.executable,
      'tests/abi_corpus/validator_regression_test.py',
      '--keep-going',
      '--validator', validator,
      '--arch', architecture
  ]
  if warn_only:
    cmd.append('--warn-only')
  Command(context, cmd=cmd)


def SummarizeCoverage(context):
  Command(context, [
      sys.executable,
      'tools/coverage_summary.py',
      context['platform'] + '-' + context['default_scons_platform'],
  ])


def ArchiveCoverage(context):
  gsutil = '/b/build/third_party/gsutil/gsutil'
  gsd_url = 'http://gsdview.appspot.com/nativeclient-coverage2/revs'
  variant_name = ('coverage-' + context['platform'] + '-' +
                  context['default_scons_platform'])
  coverage_path = variant_name + '/html/index.html'
  revision = os.environ.get('BUILDBOT_REVISION', 'None')
  link_url = gsd_url + '/' + revision + '/' + coverage_path
  gsd_base = 'gs://nativeclient-coverage2/revs'
  gs_path = gsd_base + '/' + revision + '/' + variant_name
  cov_dir = 'scons-out/' + variant_name + '/coverage'
  # Copy lcov file.
  Command(context, [
      sys.executable, gsutil,
      'cp', '-a', 'public-read',
      cov_dir + '/coverage.lcov',
      gs_path + '/coverage.lcov',
  ])
  # Copy html.
  Command(context, [
      sys.executable, gsutil,
      'cp', '-R', '-a', 'public-read',
      'html', gs_path,
  ], cwd=cov_dir)
  print('@@@STEP_LINK@view@%s@@@' % link_url)


def DoGNBuild(status, context, force_arch=None):
  if context['no_gn']:
    return False

  # Linux builds (or cross-builds) for every target.  Mac builds for
  # x86-32 and x86-64, and can build untrusted code for others.
  if context.Windows() and context['arch'] != '64' and force_arch == None:
    # The GN scripts for MSVC barf for a target_cpu other than x86 or x64
    # even if we only try to build the untrusted code.  Windows does build
    # for both x86-32 and x86-64 targets, but the GN Windows MSVC toolchain
    # scripts only support x86-64 hosts--and the Windows build of Clang
    # only has x86-64 binaries--while NaCl's x86-32 testing bots have to be
    # actual x86-32 hosts.
    return False

  if force_arch is not None:
    arch = force_arch
  else:
    arch = context['arch']

  if context.Linux():
    # The Linux build uses a sysroot.  'gclient runhooks' installs this
    # for the default architecture, but this might be a cross-build that
    # gclient didn't know was going to be done.  The script completes
    # quickly when it's redundant with a previous run.
    with Step('update_sysroot', status):
      sysroot_arch = {'arm': 'arm',
                      '32': 'i386',
                      '64': 'amd64',
                      'mips32': 'mips'}[arch]
      Command(context, cmd=[sys.executable,
                            '../build/linux/sysroot_scripts/install-sysroot.py',
                            '--arch=' + sysroot_arch])

  out_suffix = '_' + arch
  gn_out = '../out' + out_suffix

  def BoolFlag(cond):
    return 'true' if cond else 'false'

  gn_newlib = BoolFlag(not context['use_glibc'])
  gn_glibc = BoolFlag(context['use_glibc'])
  gn_arch_name = GNArch(arch)

  gn_gen_args = [
      # The Chromium GN definitions might default enable_nacl to false
      # in some circumstances, but various BUILD.gn files involved in
      # the standalone NaCl build assume enable_nacl==true.
      'enable_nacl=true',
      'target_cpu="%s"' % gn_arch_name,
      'is_debug=' + context['gn_is_debug'],
      'use_gcc_glibc=' + gn_glibc,
      'use_clang_newlib=' + gn_newlib,
  ]

  # The ASan runtime requires libstdc++, and the version on the bots is older
  # than the version in the sysroot, so ASan-built sel_ldr from the sysroot
  # won't run. For now we disable the sysroot (this matches the SCons build)
  # but the term fix would be to just use GN's libcxx build.
  if context['asan']:
    gn_gen_args.append('use_sysroot=false')

  # If this is a 32-bit build but the kernel reports as 64-bit,
  # then gn will set host_cpu=x64 when we want host_cpu=x86.
  if context.Linux() and arch == '32':
    gn_gen_args.append('host_cpu="x86"')

  # Mac can build the untrusted code for machines Mac doesn't
  # support, but the GN files will get confused in a couple of ways.
  # Just don't do the build, as there are no current Chrome configurations
  # for non-CrOS ARM.
  if context.Mac() and arch not in ('32', '64'):
    return False

  if context.Mac():
    gn_gen_args += ['use_system_xcode=false']

  gn_out_trusted = gn_out
  gn_out_irt = os.path.join(gn_out, 'irt_' + gn_arch_name)

  gn_cmd = [
      'gn.bat' if context.Windows() else 'gn',
      '--dotfile=../native_client/.gn', '--root=..',
      # Note: quotes are not needed around this space-separated
      # list of args.  The shell would remove them before passing
      # them to a program, and Python bypasses the shell.  Adding
      # quotes will cause an error because GN will see unexpected
      # double quotes.
      '--args=%s' % ' '.join(gn_gen_args),
      'gen', gn_out,
  ]

  gn_ninja_cmd = [NINJA_PATH + '.exe' if context.Windows() else NINJA_PATH,
                  '-C', gn_out, '-v']
  if gn_arch_name not in ('x86', 'x64') and not context.Linux():
    # On non-Linux non-x86, we can only build the untrusted code.
    gn_ninja_cmd.append('untrusted')

  with Step('gn_compile' + out_suffix, status):
    Command(context, cmd=gn_cmd)
    Command(context, cmd=gn_ninja_cmd)

  return (gn_out_trusted, gn_out_irt)

def RunSconsTests(status, context, using_gn, step_suffix = ''):
  if not context['use_glibc'] and not context['no_scons']:
    # Bypassing the IRT with glibc is not a supported case,
    # and in fact does not work at all with the new glibc.
    with Step('small_tests' + step_suffix, status, halt_on_fail=False):
      SCons(context, args=['small_tests'])
    with Step('medium_tests' + step_suffix, status, halt_on_fail=False):
      SCons(context, args=['medium_tests'])
    with Step('large_tests' + step_suffix, status, halt_on_fail=False):
      SCons(context, args=['large_tests'])

  with Step('compile IRT tests' + step_suffix, status):
    args = []
    if context.Windows():
      # Windows can't build trusted code. Using force_tls_edit prevents
      # that and allows tls_edit.exe to be used to build the tests.
      # See https://bugs.chromium.org/p/nativeclient/issues/detail?id=4408
      tls_edit = os.path.join(using_gn[0], 'tls_edit.exe')
      args=['force_tls_edit=' + tls_edit]
    SCons(context, parallel=True, mode=['nacl,nacl_irt_test'], args=args)

  if not context['no_scons']:
    with Step('small_tests under IRT' + step_suffix, status,
              halt_on_fail=False):
      SCons(context, mode=context['default_scons_mode'] + ['nacl_irt_test'],
            args=['small_tests_irt'])
    with Step('medium_tests under IRT' + step_suffix, status,
              halt_on_fail=False):
      SCons(context, mode=context['default_scons_mode'] + ['nacl_irt_test'],
            args=['medium_tests_irt'])
    with Step('large_tests under IRT' + step_suffix, status,
              halt_on_fail=False):
      SCons(context, mode=context['default_scons_mode'] + ['nacl_irt_test'],
            args=['large_tests_irt'])

def DoGNTest(status, context, using_gn, gn_perf_prefix, gn_step_suffix):
  if not using_gn:
    return

  gn_out_trusted, gn_out_irt = using_gn

  # Non-Linux can build non-x86 untrusted code, but can't build or run
  # the trusted code and so cannot test.
  if context['arch'] not in ('32', '64') and not context.Linux():
    return

  gn_sel_ldr = os.path.join(gn_out_trusted, 'sel_ldr')
  if context.Windows():
    gn_sel_ldr += '.exe'
  gn_extra = [
      'force_sel_ldr=' + gn_sel_ldr,
      'force_irt=' + os.path.join(gn_out_irt, 'irt_core.nexe'),
      'perf_prefix=' + gn_perf_prefix,
      ]
  if context.Linux():
    gn_extra.append('force_bootstrap=' +
                    os.path.join(gn_out_trusted, 'nacl_helper_bootstrap'))
  elif context.Windows():
    gn_extra.append('force_tls_edit=' +
                    os.path.join(gn_out_trusted, 'tls_edit.exe'))
  def RunGNTests(step_suffix, extra_scons_modes, suite_suffix):
    for suite in ['small_tests', 'medium_tests', 'large_tests']:
      with Step(suite + step_suffix + gn_step_suffix, status,
                halt_on_fail=False):
        SCons(context,
              mode=context['default_scons_mode'] + extra_scons_modes,
              args=[suite + suite_suffix] + gn_extra)
  if not context['use_glibc']:
    RunGNTests('', [], '')
  RunGNTests(' under IRT', ['nacl_irt_test'], '_irt')


def BuildScript(status, context):
  inside_toolchain = context['inside_toolchain']

  # Clean out build directories.
  with Step('clobber', status):
    RemoveSconsBuildDirectories()
    RemoveGypBuildDirectories()

  with Step('cleanup_temp', status):
    # Picking out drive letter on which the build is happening so we can use
    # it for the temp directory.
    if context.Windows():
      build_drive = os.path.splitdrive(os.path.abspath(__file__))[0]
      tmp_dir = os.path.join(build_drive, os.path.sep + 'temp')
      context.SetEnv('TEMP', tmp_dir)
      context.SetEnv('TMP', tmp_dir)
    else:
      tmp_dir = '/tmp'
    print('Making sure %s exists...' % tmp_dir)
    EnsureDirectoryExists(tmp_dir)
    print('Cleaning up the contents of %s...' % tmp_dir)
    # Only delete files and directories like:
    #   */nacl_tmp/*
    # TODO(bradnelson): Drop this after a bit.
    # Also drop files and directories like these to cleanup current state:
    #   */nacl_tmp*
    #   */nacl*
    #   83C4.tmp
    #   .org.chromium.Chromium.EQrEzl
    #   tmp_platform*
    #   tmp_mmap*
    #   tmp_pwrite*
    #   tmp_syscalls*
    #   workdir*
    #   nacl_chrome_download_*
    #   browserprofile_*
    #   tmp*
    regex_body = (
        r'(tmp_nacl[\\/].+|'
        r'tmp_nacl.+|'
        r'nacl.+|'
        r'[0-9a-fA-F]+\.tmp|'
        r'\.org\.chrom\w+\.Chrom\w+\.[^\\/]+|'
        r'tmp_platform[^\\/]+|'
        r'tmp_mmap[^\\/]+|'
        r'tmp_pwrite[^\\/]+|'
        r'tmp_syscalls[^\\/]+|'
        r'workdir[^\\/]+|'
        r'nacl_chrome_download_[^\\/]+|'
        r'browserprofile_[^\\/]+|'
        r'tmp[^\\/]+'
        r')$'
    )
    file_name_re = re.compile(
        r'([\\/]' + regex_body + r')|(\A' + regex_body + r')')
    file_name_filter = lambda fn: file_name_re.search(fn) is not None

    # Clean nacl_tmp/* separately, so we get a list of leaks.
    nacl_tmp = os.path.join(tmp_dir, 'nacl_tmp')
    if os.path.exists(nacl_tmp):
      for bot in os.listdir(nacl_tmp):
        bot_path = os.path.join(nacl_tmp, bot)
        print('Cleaning prior build temp dir: %s' % bot_path)
        sys.stdout.flush()
        if os.path.isdir(bot_path):
          for d in os.listdir(bot_path):
            path = os.path.join(bot_path, d)
            print('Removing leftover: %s' % path)
            sys.stdout.flush()
            RemovePath(path)
          os.rmdir(bot_path)
        else:
          print('Removing rogue file: %s' % bot_path)
          RemovePath(bot_path)
      os.rmdir(nacl_tmp)
    # Clean /tmp so we get a list of what's accumulating.
    TryToCleanContents(tmp_dir, file_name_filter)

    # Recreate TEMP, as it may have been clobbered.
    if 'TEMP' in os.environ and not os.path.exists(os.environ['TEMP']):
      os.makedirs(os.environ['TEMP'])

    # Mac has an additional temporary directory; clean it up.
    # TODO(bradnelson): Fix Mac Chromium so that these temp files are created
    #     with open() + unlink() so that they will not get left behind.
    if context.Mac():
      subprocess.call(
          "find /var/folders -name '.org.chromium.*' -exec rm -rfv '{}' ';'",
          shell=True)
      subprocess.call(
          "find /var/folders -name '.com.google.Chrome*' -exec rm -rfv '{}' ';'",
          shell=True)

  # Always update Clang.  On Linux and Mac, it's the default for the GN build.
  # It's also used for the Linux Breakpad build. On Windows, we do a second
  # Clang GN build.
  with Step('update_clang', status):
    Command(context, cmd=[sys.executable, '../tools/clang/scripts/update.py'])

  # Make sure our GN build is working.
  using_gn = DoGNBuild(status, context)

  if context.Windows():
    if context['arch'] == '64':
      # On Windows, do a second GN build for 32-bit.  The 32-bit
      # bots can't do GN builds at all, because the toolchains GN uses don't
      # support 32-bit hosts.  The 32-bit binaries built here cannot be
      # tested on a 64-bit host, but compile time issues can be caught.
      DoGNBuild(status, context, '32')
    elif using_gn == False:
      # The GN build we want is not supported (e.g. ARM), then force-substitute
      # a host arch that will run on the buildbot, so we can at last use
      # tls_edit.exe to test the Scons build
      using_gn = DoGNBuild(status, context, '64')

  # Just build both bitages of validator and test for --validator mode.
  if context['validator']:
    with Step('build ragel_validator-32', status):
      SCons(context, platform='x86-32', parallel=True, args=['ncval_new'])
    with Step('build ragel_validator-64', status):
      SCons(context, platform='x86-64', parallel=True, args=['ncval_new'])

    # Check validator trie proofs on both 32 + 64 bits.
    with Step('check validator proofs', status):
      SCons(context, platform='x86-64', parallel=False, args=['dfachecktries'])

    with Step('predownload validator corpus', status):
      Command(context,
          cmd=[sys.executable,
               'tests/abi_corpus/validator_regression_test.py',
               '--download-only'])

    with Step('validator_regression_test ragel x86-32', status,
        halt_on_fail=False):
      ValidatorTest(
          context, 'x86-32',
          'scons-out/opt-linux-x86-32/staging/ncval_new')
    with Step('validator_regression_test ragel x86-64', status,
        halt_on_fail=False):
      ValidatorTest(
          context, 'x86-64',
          'scons-out/opt-linux-x86-64/staging/ncval_new')

    return

  # Run checkdeps script to vet #includes.
  with Step('checkdeps', status):
    Command(context, cmd=[sys.executable, 'tools/checkdeps/checkdeps.py'])

  # On a subset of Linux builds, build Breakpad tools for testing.
  if context['use_breakpad_tools']:
    with Step('breakpad configure', status):
      Command(context, cmd=['mkdir', '-p', 'breakpad-out'])

      # Breakpad requires C++11, so use clang and the sysroot rather than
      # hoping that the host toolchain will provide support.
      configure_args = []
      if context.Linux():
        cc = 'CC=../../third_party/llvm-build/Release+Asserts/bin/clang'
        cxx = 'CXX=../../third_party/llvm-build/Release+Asserts/bin/clang++'
        flags = ' -Wno-non-c-typedef-for-linkage'
        if context['arch'] == '32':
          flags += ' -m32'
          sysroot_arch = 'i386'
        else:
          flags += ' -m64'
          sysroot_arch = 'amd64'
        flags += (' --sysroot=../../build/linux/debian_bullseye_%s-sysroot' %
                  sysroot_arch)
        configure_args += [cc + flags, cxx + flags]
        configure_args += ['CXXFLAGS=-I../..',  # For third_party/lss
                           'LDFLAGS=-fuse-ld=lld -static-libstdc++']

      try:
        Command(
            context,
            cwd='breakpad-out',
            cmd=['bash', '../../breakpad/configure'] + configure_args)
      except:
        f = open(os.path.join('breakpad-out', 'config.log')).read()
        print(f)
        raise

    with Step('breakpad make', status):
      Command(context, cmd=['make', '-j%d' % context['max_jobs'],
                            # This avoids a broken dependency on
                            # src/third_party/lss files within the breakpad
                            # source directory.  We are not putting lss
                            # there, but using the -I switch above to
                            # find the lss in ../third_party instead.
                            'includelss_HEADERS=',
                            ],
              cwd='breakpad-out')

  # The main compile step.
  if not context['no_scons']:
    with Step('scons_compile', status):
      SCons(context, parallel=True, args=[])

  if context['coverage']:
    with Step('collect_coverage', status, halt_on_fail=True):
      SCons(context, args=['coverage'])
    with Step('summarize_coverage', status, halt_on_fail=False):
      SummarizeCoverage(context)
    slave_type = os.environ.get('BUILDBOT_SLAVE_TYPE')
    if slave_type != 'Trybot' and slave_type is not None:
      with Step('archive_coverage', status, halt_on_fail=True):
        ArchiveCoverage(context)
    return

  # Android bots don't run tests for now.
  if context['android']:
    return

  RunSconsTests(status, context, using_gn)

  # Tests for saigo.
  context['saigo'] = True
  if (context['arch'] in ('32', '64', 'arm')
      and not context['use_glibc']
      and not context['no_scons']):
    with Step('scons_compile for Saigo', status):
      SCons(context, parallel=True, args=[])
    RunSconsTests(status, context, using_gn, step_suffix=' for Saigo')
  context['saigo'] = False

  if context.Windows() and context['arch'] != '64':
    # Currently only the only Windows bots are 64-bit, and they can't run the
    # 32-bit sandbox.
    return
  ### BEGIN GN tests ###
  DoGNTest(status, context, using_gn, 'gn_', ' (GN)')

  context['saigo'] = True
  if context['arch'] in ('32', '64', 'arm') and not context['use_glibc']:
    DoGNTest(status, context, using_gn, 'gn_', ' for Saigo (GN)')
  context['saigo'] = False
  ### END GN tests ###


def Main():
  # TODO(ncbray) make buildbot scripts composable to support toolchain use case.
  context = BuildContext()
  status = BuildStatus(context)
  ParseStandardCommandLine(context)
  SetupContextVars(context)
  if context.Windows():
    if not context['no_scons']:
      SetupWindowsEnvironment(context)
  elif context.Linux():
    if not context['android']:
      SetupLinuxEnvironment(context)
  elif context.Mac():
    # No setup to do for Mac.
    pass
  else:
    raise Exception("Unsupported platform.")
  RunBuild(BuildScript, status)


def TimedMain():
  start_time = time.time()
  try:
    Main()
  finally:
    time_taken = time.time() - start_time
    print('RESULT BuildbotTime: total= %.3f minutes' % (time_taken / 60))


if __name__ == '__main__':
  TimedMain()
