#!/usr/bin/env python3
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates build.ninja that will build GN."""

import argparse
import os
import platform
import re
import shlex
import subprocess
import sys

# IMPORTANT: This script is also executed as python2 on
# GN's CI builders.

try:  # py3
  from shlex import quote as shell_quote
except ImportError:  # py2
  from pipes import quote as shell_quote

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(SCRIPT_DIR)

class Platform(object):
  """Represents a host/target platform."""
  def __init__(self, platform):
    self._platform = platform
    if self._platform is not None:
      return
    self._platform = sys.platform
    if self._platform.startswith('linux'):
      self._platform = 'linux'
    elif self._platform.startswith('darwin'):
      self._platform = 'darwin'
    elif self._platform.startswith('mingw'):
      self._platform = 'mingw'
    elif self._platform.startswith('msys'):
      self._platform = 'msys'
    elif self._platform.startswith('win'):
      self._platform = 'msvc'
    elif self._platform.startswith('aix'):
      self._platform = 'aix'
    elif self._platform.startswith('fuchsia'):
      self._platform = 'fuchsia'
    elif self._platform.startswith('freebsd'):
      self._platform = 'freebsd'
    elif self._platform.startswith('netbsd'):
      self._platform = 'netbsd'
    elif self._platform.startswith('openbsd'):
      self._platform = 'openbsd'
    elif self._platform.startswith('haiku'):
      self._platform = 'haiku'
    elif self._platform.startswith('sunos'):
      self._platform = 'solaris'
    elif self._platform.startswith('zos'):
      self._platform = 'zos'
    elif self._platform.startswith('serenity'):
      self._platform = 'serenity'

  @staticmethod
  def known_platforms():
    return ['linux', 'darwin', 'mingw', 'msys', 'msvc', 'aix', 'fuchsia', 'freebsd', 'netbsd', 'openbsd', 'haiku', 'solaris', 'zos', 'serenity']

  def platform(self):
    return self._platform

  def is_linux(self):
    return self._platform == 'linux'

  def is_mingw(self):
    return self._platform == 'mingw'

  def is_msys(self):
    return self._platform == 'msys'

  def is_msvc(self):
    return self._platform == 'msvc'

  def is_windows(self):
    return self.is_mingw() or self.is_msvc()

  def is_darwin(self):
    return self._platform == 'darwin'

  def is_aix(self):
    return self._platform == 'aix'

  def is_haiku(self):
    return self._platform == 'haiku'

  def is_solaris(self):
    return self._platform == 'solaris'

  def is_posix(self):
    return self._platform in ['linux', 'freebsd', 'darwin', 'aix', 'openbsd', 'haiku', 'solaris', 'msys', 'netbsd', 'serenity']

  def is_zos(self):
    return self._platform == 'zos'

  def is_serenity(self):
    return self_.platform == 'serenity'

class ArgumentsList:
  """Helper class to accumulate ArgumentParser argument definitions
  and be able to regenerate a corresponding command-line to be
  written in the generated Ninja file for the 'regen' rule.
  """
  def __init__(self):
    self._arguments = []

  def add(self, *args, **kwargs):
    """Add an argument definition, use as argparse.ArgumentParser.add_argument()."""
    self._arguments.append((args, kwargs))

  def add_to_parser(self, parser):
    """Add all known arguments to parser."""
    for args, kwargs in self._arguments:
      parser.add_argument(*args, **kwargs)

  def gen_command_line_args(self, parser_arguments):
    """Generate a gen.py argument list to be embedded in a Ninja file."""
    result = []
    for args, kwargs in self._arguments:
      if len(args) == 2:
        long_option = args[1]
      else:
        long_option = args[0]
      dest = kwargs.get('dest', None)
      if dest is None:
        assert long_option.startswith('--')
        dest = long_option[2:].replace('-', '_')

      if getattr(parser_arguments, dest, None) is None:
        # This was not set on the command-line so skip it.
        continue

      action = kwargs.get('action', None)
      if action == 'store_true':
        if getattr(parser_arguments, dest):
          result.append(long_option)
      elif action == 'store' or action is None:
        result.append('%s=%s' % (long_option, getattr(parser_arguments, dest)))
      elif action == 'append':
        for item in getattr(parser_arguments, dest):
          result.append('%s=%s' % (long_option, item))
      else:
        assert action is None, "Unsupported action " + action
    return ' '.join(shell_quote(item) for item in result)


def main(argv):
  parser = argparse.ArgumentParser(description=sys.modules[__name__].__doc__)
  args_list = ArgumentsList()

  args_list.add('-d', '--debug', action='store_true',
                    help='Do a debug build. Defaults to release build.')
  args_list.add('--platform',
                    help='target platform (' +
                         '/'.join(Platform.known_platforms()) + ')',
                    choices=Platform.known_platforms())
  args_list.add('--host',
                    help='host platform (' +
                         '/'.join(Platform.known_platforms()) + ')',
                    choices=Platform.known_platforms())
  args_list.add('--use-lto', action='store_true',
                    help='Enable the use of LTO')
  args_list.add('--use-icf', action='store_true',
                    help='Enable the use of Identical Code Folding')
  args_list.add('--use-asan', action='store_true',
                    help='Enable the use of AddressSanitizer')
  args_list.add('--use-ubsan', action='store_true',
                    help='Enable the use of UndefinedBehaviorSanitizer')
  args_list.add('--no-last-commit-position', action='store_true',
                    help='Do not generate last_commit_position.h.')
  args_list.add('--out-path',
                    help='The path to generate the build files in.')
  args_list.add('--no-strip', action='store_true',
                    help='Don\'t strip release build. Useful for profiling.')
  args_list.add('--no-static-libstdc++', action='store_true',
                    default=False, dest='no_static_libstdcpp',
                    help='Don\'t link libstdc++ statically')
  args_list.add('--link-lib',
                    action='append',
                    metavar='LINK_LIB',
                    default=[],
                    dest='link_libs',
                    help=('Add a library to the final executable link. ' +
                          'LINK_LIB must be the path to a static or shared ' +
                          'library, or \'-l<name>\' on POSIX systems. Can be ' +
                          'used multiple times. Useful to link custom malloc ' +
                          'or cpu profiling libraries.'))
  args_list.add('--allow-warnings', action='store_true', default=False,
                    help=('Allow compiler warnings, don\'t treat them as '
                          'errors.'))
  if sys.platform == 'zos':
    args_list.add('--zoslib-dir',
                      action='store',
                      default='../third_party/zoslib',
                      dest='zoslib_dir',
                      help=('Specify the path of ZOSLIB directory, to link ' +
                            'with <ZOSLIB_DIR>/install/lib/libzoslib.a, and ' +
                            'add -I<ZOSLIB_DIR>/install/include to the compile ' +
                            'commands. See README.md for details.'))

  args_list.add_to_parser(parser)
  options = parser.parse_args(argv)

  platform = Platform(options.platform)
  if options.host:
    host = Platform(options.host)
  else:
    host = platform

  out_dir = options.out_path or os.path.join(REPO_ROOT, 'out')
  if not os.path.isdir(out_dir):
    os.makedirs(out_dir)
  if not options.no_last_commit_position:
    GenerateLastCommitPosition(host,
                               os.path.join(out_dir, 'last_commit_position.h'))
  WriteGNNinja(os.path.join(out_dir, 'build.ninja'), platform, host, options, args_list)
  return 0


def GenerateLastCommitPosition(host, header):
  ROOT_TAG = 'initial-commit'
  describe_output = subprocess.check_output(
      ['git', 'describe', 'HEAD', '--abbrev=12', '--match', ROOT_TAG],
      shell=host.is_windows(), cwd=REPO_ROOT)
  mo = re.match(ROOT_TAG + '-(\d+)-g([0-9a-f]+)', describe_output.decode())
  if not mo:
    raise ValueError(
        'Unexpected output from git describe when generating version header')

  contents = '''// Generated by build/gen.py.

#ifndef OUT_LAST_COMMIT_POSITION_H_
#define OUT_LAST_COMMIT_POSITION_H_

#define LAST_COMMIT_POSITION_NUM %s
#define LAST_COMMIT_POSITION "%s (%s)"

#endif  // OUT_LAST_COMMIT_POSITION_H_
''' % (mo.group(1), mo.group(1), mo.group(2))

  # Only write/touch this file if the commit position has changed.
  old_contents = ''
  if os.path.isfile(header):
    with open(header, 'r') as f:
      old_contents = f.read()

  if old_contents != contents:
    with open(header, 'w') as f:
      f.write(contents)


def WriteGenericNinja(path, static_libraries, executables,
                      cxx, ar, ld, platform, host, options,
                      args_list, cflags=[], ldflags=[],
                      libflags=[], include_dirs=[], solibs=[]):
  args = args_list.gen_command_line_args(options)
  if args:
    args = " " + args

  ninja_header_lines = [
    'cxx = ' + cxx,
    'ar = ' + ar,
    'ld = ' + ld,
    '',
    'rule regen',
    '  command = %s ../build/gen.py%s' % (sys.executable, args),
    '  description = Regenerating ninja files',
    '',
    'build build.ninja: regen',
    '  generator = 1',
    '  depfile = build.ninja.d',
    '',
  ]


  template_filename = os.path.join(SCRIPT_DIR, {
      'msvc': 'build_win.ninja.template',
      'mingw': 'build_linux.ninja.template',
      'msys': 'build_linux.ninja.template',
      'darwin': 'build_mac.ninja.template',
      'linux': 'build_linux.ninja.template',
      'freebsd': 'build_linux.ninja.template',
      'aix': 'build_aix.ninja.template',
      'openbsd': 'build_openbsd.ninja.template',
      'haiku': 'build_haiku.ninja.template',
      'solaris': 'build_linux.ninja.template',
      'netbsd': 'build_linux.ninja.template',
      'zos': 'build_zos.ninja.template',
      'serenity': 'build_linux.ninja.template',
  }[platform.platform()])

  with open(template_filename) as f:
    ninja_template = f.read()

  if platform.is_windows():
    executable_ext = '.exe'
    library_ext = '.lib'
    object_ext = '.obj'
  else:
    executable_ext = ''
    library_ext = '.a'
    object_ext = '.o'

  def escape_path_ninja(path):
    return path.replace('$ ', '$$ ').replace(' ', '$ ').replace(':', '$:')

  def src_to_obj(path):
    return escape_path_ninja('%s' % os.path.splitext(path)[0] + object_ext)

  def library_to_a(library):
    return '%s%s' % (library, library_ext)

  ninja_lines = []
  def build_source(src_file, settings):
    ninja_lines.extend([
        'build %s: cxx %s' % (src_to_obj(src_file),
                              escape_path_ninja(
                                  os.path.relpath(
                                      os.path.join(REPO_ROOT, src_file),
                                      os.path.dirname(path)))),
        '  includes = %s' % ' '.join(
            ['-I' + escape_path_ninja(dirname) for dirname in include_dirs]),
        '  cflags = %s' % ' '.join(cflags),
    ])

  for library, settings in static_libraries.items():
    for src_file in settings['sources']:
      build_source(src_file, settings)

    ninja_lines.append('build %s: alink_thin %s' % (
        library_to_a(library),
        ' '.join([src_to_obj(src_file) for src_file in settings['sources']])))
    ninja_lines.append('  libflags = %s' % ' '.join(libflags))


  for executable, settings in executables.items():
    for src_file in settings['sources']:
      build_source(src_file, settings)

    ninja_lines.extend([
      'build %s%s: link %s | %s' % (
          executable, executable_ext,
          ' '.join([src_to_obj(src_file) for src_file in settings['sources']]),
          ' '.join([library_to_a(library) for library in settings['libs']])),
      '  ldflags = %s' % ' '.join(ldflags),
      '  solibs = %s' % ' '.join(solibs),
      '  libs = %s' % ' '.join(
          [library_to_a(library) for library in settings['libs']]),
    ])

  ninja_lines.append('')  # Make sure the file ends with a newline.

  with open(path, 'w') as f:
    f.write('\n'.join(ninja_header_lines))
    f.write(ninja_template)
    f.write('\n'.join(ninja_lines))

  with open(path + '.d', 'w') as f:
    f.write('build.ninja: ' +
            os.path.relpath(os.path.join(SCRIPT_DIR, 'gen.py'),
                            os.path.dirname(path)) + ' ' +
            os.path.relpath(template_filename, os.path.dirname(path)) + '\n')


def WriteGNNinja(path, platform, host, options, args_list):
  if platform.is_msvc():
    cxx = os.environ.get('CXX', 'cl.exe')
    ld = os.environ.get('LD', 'link.exe')
    ar = os.environ.get('AR', 'lib.exe')
  elif platform.is_aix():
    cxx = os.environ.get('CXX', 'g++')
    ld = os.environ.get('LD', 'g++')
    ar = os.environ.get('AR', 'ar -X64')
  elif platform.is_msys() or platform.is_mingw():
    cxx = os.environ.get('CXX', 'g++')
    ld = os.environ.get('LD', 'g++')
    ar = os.environ.get('AR', 'ar')
  else:
    cxx = os.environ.get('CXX', 'clang++')
    ld = cxx
    ar = os.environ.get('AR', 'ar')

  cflags = os.environ.get('CFLAGS', '').split()
  cflags += os.environ.get('CXXFLAGS', '').split()
  ldflags = os.environ.get('LDFLAGS', '').split()
  libflags = os.environ.get('LIBFLAGS', '').split()
  include_dirs = [
      os.path.relpath(os.path.join(REPO_ROOT, 'src'), os.path.dirname(path)),
      '.',
  ]
  if platform.is_zos():
    include_dirs += [ options.zoslib_dir + '/install/include' ]

  libs = []

  if not platform.is_msvc():
    if options.debug:
      cflags.extend(['-O0', '-g'])
    else:
      cflags.append('-DNDEBUG')
      cflags.append('-O3')
      if options.no_strip:
        cflags.append('-g')
      ldflags.append('-O3')
      # Use -fdata-sections and -ffunction-sections to place each function
      # or data item into its own section so --gc-sections can eliminate any
      # unused functions and data items.
      cflags.extend(['-fdata-sections', '-ffunction-sections'])
      ldflags.extend(['-fdata-sections', '-ffunction-sections'])
      if platform.is_darwin():
        ldflags.append('-Wl,-dead_strip')
      elif not platform.is_aix() and not platform.is_solaris() and not platform.is_zos():
        # Garbage collection is done by default on aix, and option is unsupported on z/OS.
        ldflags.append('-Wl,--gc-sections')

      # Omit all symbol information from the output file.
      if options.no_strip is None:
        if platform.is_darwin():
          ldflags.append('-Wl,-S')
        elif platform.is_aix():
          ldflags.append('-Wl,-s')
        elif platform.is_solaris():
          ldflags.append('-Wl,--strip-all')
        elif not platform.is_zos():
          # /bin/ld on z/OS doesn't have an equivalent option.
          ldflags.append('-Wl,-strip-all')

      # Enable identical code-folding.
      if options.use_icf and not platform.is_darwin():
        ldflags.append('-Wl,--icf=all')

      if options.use_lto:
        cflags.extend(['-flto', '-fwhole-program-vtables'])
        ldflags.extend(['-flto', '-fwhole-program-vtables'])

    if options.use_asan:
      cflags.append('-fsanitize=address')
      ldflags.append('-fsanitize=address')

    if options.use_ubsan:
      cflags.append('-fsanitize=undefined')
      ldflags.append('-fsanitize=undefined')

    if not options.allow_warnings:
      cflags.append('-Werror')

    cflags.extend([
        '-D_FILE_OFFSET_BITS=64',
        '-D__STDC_CONSTANT_MACROS', '-D__STDC_FORMAT_MACROS',
        '-pthread',
        '-pipe',
        '-fno-exceptions',
        '-fno-rtti',
        '-fdiagnostics-color',
        '-Wall',
        '-Wextra',
        '-Wno-unused-parameter',

        '-Wextra-semi',
        '-Wundef',

        '-std=c++17'
    ])

    # flags not supported by gcc/g++.
    if cxx == 'clang++':
      cflags.extend(['-Wrange-loop-analysis', '-Wextra-semi-stmt'])

    if platform.is_linux() or platform.is_mingw() or platform.is_msys():
      ldflags.append('-Wl,--as-needed')

      if not options.no_static_libstdcpp:
        ldflags.append('-static-libstdc++')

      if platform.is_mingw() or platform.is_msys():
        cflags.remove('-std=c++17')
        cflags.extend([
          '-Wno-deprecated-copy',
          '-Wno-implicit-fallthrough',
          '-Wno-redundant-move',
          '-Wno-unused-variable',
          '-Wno-format',             # Use of %llx, which is supported by _UCRT, false positive
          '-Wno-strict-aliasing',    # Dereferencing punned pointer
          '-Wno-cast-function-type', # Casting FARPROC to RegDeleteKeyExPtr
          '-std=gnu++17',
        ])
      else:
        # This is needed by libc++.
        libs.append('-ldl')
    elif platform.is_darwin():
      min_mac_version_flag = '-mmacosx-version-min=10.9'
      cflags.append(min_mac_version_flag)
      ldflags.append(min_mac_version_flag)
    elif platform.is_aix():
      cflags.append('-maix64')
      ldflags.append('-maix64')
    elif platform.is_haiku():
      cflags.append('-fPIC')
      cflags.extend(['-D_BSD_SOURCE'])
    elif platform.is_zos():
      cflags.append('-fzos-le-char-mode=ascii')
      cflags.append('-Wno-unused-function')
      cflags.append('-D_OPEN_SYS_FILE_EXT')
      cflags.append('-DPATH_MAX=1024')

    if platform.is_posix() and not platform.is_haiku():
      ldflags.append('-pthread')

    if platform.is_mingw() or platform.is_msys():
      cflags.extend(['-DUNICODE',
                     '-DNOMINMAX',
                     '-DWIN32_LEAN_AND_MEAN',
                     '-DWINVER=0x0A00',
                     '-D_CRT_SECURE_NO_DEPRECATE',
                     '-D_SCL_SECURE_NO_DEPRECATE',
                     '-D_UNICODE',
                     '-D_WIN32_WINNT=0x0A00',
                     '-D_HAS_EXCEPTIONS=0'
      ])
  elif platform.is_msvc():
    if not options.debug:
      cflags.extend(['/O2', '/DNDEBUG', '/Zc:inline'])
      ldflags.extend(['/OPT:REF'])

      if options.use_icf:
        libflags.extend(['/OPT:ICF'])
      if options.use_lto:
        cflags.extend(['/GL'])
        libflags.extend(['/LTCG'])
        ldflags.extend(['/LTCG'])

    if not options.allow_warnings:
      cflags.append('/WX')

    cflags.extend([
        '/DNOMINMAX',
        '/DUNICODE',
        '/DWIN32_LEAN_AND_MEAN',
        '/DWINVER=0x0A00',
        '/D_CRT_SECURE_NO_DEPRECATE',
        '/D_SCL_SECURE_NO_DEPRECATE',
        '/D_UNICODE',
        '/D_WIN32_WINNT=0x0A00',
        '/FS',
        '/W4',
        '/Zi',
        '/wd4099',
        '/wd4100',
        '/wd4127',
        '/wd4244',
        '/wd4267',
        '/wd4505',
        '/wd4838',
        '/wd4996',
        '/std:c++17',
        '/GR-',
        '/D_HAS_EXCEPTIONS=0',
    ])

    ldflags.extend(['/DEBUG', '/MACHINE:x64'])

  static_libraries = {
      'base': {'sources': [
        'src/base/command_line.cc',
        'src/base/environment.cc',
        'src/base/files/file.cc',
        'src/base/files/file_enumerator.cc',
        'src/base/files/file_path.cc',
        'src/base/files/file_path_constants.cc',
        'src/base/files/file_util.cc',
        'src/base/files/scoped_file.cc',
        'src/base/files/scoped_temp_dir.cc',
        'src/base/json/json_parser.cc',
        'src/base/json/json_reader.cc',
        'src/base/json/json_writer.cc',
        'src/base/json/string_escape.cc',
        'src/base/logging.cc',
        'src/base/md5.cc',
        'src/base/memory/ref_counted.cc',
        'src/base/memory/weak_ptr.cc',
        'src/base/sha1.cc',
        'src/base/strings/string_number_conversions.cc',
        'src/base/strings/string_split.cc',
        'src/base/strings/string_util.cc',
        'src/base/strings/string_util_constants.cc',
        'src/base/strings/stringprintf.cc',
        'src/base/strings/utf_string_conversion_utils.cc',
        'src/base/strings/utf_string_conversions.cc',
        'src/base/timer/elapsed_timer.cc',
        'src/base/value_iterators.cc',
        'src/base/values.cc',
      ]},
      'gn_lib': {'sources': [
        'src/gn/action_target_generator.cc',
        'src/gn/action_values.cc',
        'src/gn/analyzer.cc',
        'src/gn/args.cc',
        'src/gn/binary_target_generator.cc',
        'src/gn/build_settings.cc',
        'src/gn/builder.cc',
        'src/gn/builder_record.cc',
        'src/gn/bundle_data.cc',
        'src/gn/bundle_data_target_generator.cc',
        'src/gn/bundle_file_rule.cc',
        'src/gn/builtin_tool.cc',
        'src/gn/c_include_iterator.cc',
        'src/gn/c_substitution_type.cc',
        'src/gn/c_tool.cc',
        'src/gn/command_analyze.cc',
        'src/gn/command_args.cc',
        'src/gn/command_check.cc',
        'src/gn/command_clean.cc',
        'src/gn/command_clean_stale.cc',
        'src/gn/command_desc.cc',
        'src/gn/command_format.cc',
        'src/gn/command_gen.cc',
        'src/gn/command_help.cc',
        'src/gn/command_ls.cc',
        'src/gn/command_meta.cc',
        'src/gn/command_outputs.cc',
        'src/gn/command_path.cc',
        'src/gn/command_refs.cc',
        'src/gn/commands.cc',
        'src/gn/compile_commands_writer.cc',
        'src/gn/rust_project_writer.cc',
        'src/gn/config.cc',
        'src/gn/config_values.cc',
        'src/gn/config_values_extractors.cc',
        'src/gn/config_values_generator.cc',
        'src/gn/copy_target_generator.cc',
        'src/gn/create_bundle_target_generator.cc',
        'src/gn/deps_iterator.cc',
        'src/gn/desc_builder.cc',
        'src/gn/eclipse_writer.cc',
        'src/gn/err.cc',
        'src/gn/escape.cc',
        'src/gn/exec_process.cc',
        'src/gn/filesystem_utils.cc',
        'src/gn/file_writer.cc',
        'src/gn/frameworks_utils.cc',
        'src/gn/function_exec_script.cc',
        'src/gn/function_filter.cc',
        'src/gn/function_foreach.cc',
        'src/gn/function_forward_variables_from.cc',
        'src/gn/function_get_label_info.cc',
        'src/gn/function_get_path_info.cc',
        'src/gn/function_get_target_outputs.cc',
        'src/gn/function_process_file_template.cc',
        'src/gn/function_read_file.cc',
        'src/gn/function_rebase_path.cc',
        'src/gn/function_set_default_toolchain.cc',
        'src/gn/function_set_defaults.cc',
        'src/gn/function_template.cc',
        'src/gn/function_toolchain.cc',
        'src/gn/function_write_file.cc',
        'src/gn/functions.cc',
        'src/gn/functions_target.cc',
        'src/gn/general_tool.cc',
        'src/gn/generated_file_target_generator.cc',
        'src/gn/group_target_generator.cc',
        'src/gn/header_checker.cc',
        'src/gn/import_manager.cc',
        'src/gn/input_conversion.cc',
        'src/gn/input_file.cc',
        'src/gn/input_file_manager.cc',
        'src/gn/item.cc',
        'src/gn/json_project_writer.cc',
        'src/gn/label.cc',
        'src/gn/label_pattern.cc',
        'src/gn/lib_file.cc',
        'src/gn/loader.cc',
        'src/gn/location.cc',
        'src/gn/metadata.cc',
        'src/gn/metadata_walk.cc',
        'src/gn/ninja_action_target_writer.cc',
        'src/gn/ninja_binary_target_writer.cc',
        'src/gn/ninja_build_writer.cc',
        'src/gn/ninja_bundle_data_target_writer.cc',
        'src/gn/ninja_c_binary_target_writer.cc',
        'src/gn/ninja_copy_target_writer.cc',
        'src/gn/ninja_create_bundle_target_writer.cc',
        'src/gn/ninja_generated_file_target_writer.cc',
        'src/gn/ninja_group_target_writer.cc',
        'src/gn/ninja_rust_binary_target_writer.cc',
        'src/gn/ninja_target_command_util.cc',
        'src/gn/ninja_target_writer.cc',
        'src/gn/ninja_toolchain_writer.cc',
        'src/gn/ninja_tools.cc',
        'src/gn/ninja_utils.cc',
        'src/gn/ninja_writer.cc',
        'src/gn/operators.cc',
        'src/gn/output_conversion.cc',
        'src/gn/output_file.cc',
        'src/gn/parse_node_value_adapter.cc',
        'src/gn/parse_tree.cc',
        'src/gn/parser.cc',
        'src/gn/path_output.cc',
        'src/gn/pattern.cc',
        'src/gn/pool.cc',
        'src/gn/qt_creator_writer.cc',
        'src/gn/resolved_target_data.cc',
        'src/gn/runtime_deps.cc',
        'src/gn/rust_substitution_type.cc',
        'src/gn/rust_tool.cc',
        'src/gn/rust_values.cc',
        'src/gn/rust_values_generator.cc',
        'src/gn/rust_variables.cc',
        'src/gn/scheduler.cc',
        'src/gn/scope.cc',
        'src/gn/scope_per_file_provider.cc',
        'src/gn/settings.cc',
        'src/gn/setup.cc',
        'src/gn/source_dir.cc',
        'src/gn/source_file.cc',
        'src/gn/standard_out.cc',
        'src/gn/string_atom.cc',
        'src/gn/string_output_buffer.cc',
        'src/gn/string_utils.cc',
        'src/gn/substitution_list.cc',
        'src/gn/substitution_pattern.cc',
        'src/gn/substitution_type.cc',
        'src/gn/substitution_writer.cc',
        'src/gn/swift_values.cc',
        'src/gn/swift_values_generator.cc',
        'src/gn/swift_variables.cc',
        'src/gn/switches.cc',
        'src/gn/target.cc',
        'src/gn/target_generator.cc',
        'src/gn/template.cc',
        'src/gn/token.cc',
        'src/gn/tokenizer.cc',
        'src/gn/tool.cc',
        'src/gn/toolchain.cc',
        'src/gn/trace.cc',
        'src/gn/value.cc',
        'src/gn/value_extractors.cc',
        'src/gn/variables.cc',
        'src/gn/version.cc',
        'src/gn/visibility.cc',
        'src/gn/visual_studio_utils.cc',
        'src/gn/visual_studio_writer.cc',
        'src/gn/xcode_object.cc',
        'src/gn/xcode_writer.cc',
        'src/gn/xml_element_writer.cc',
        'src/util/atomic_write.cc',
        'src/util/exe_path.cc',
        'src/util/msg_loop.cc',
        'src/util/semaphore.cc',
        'src/util/sys_info.cc',
        'src/util/ticks.cc',
        'src/util/worker_pool.cc',
      ]},
  }

  executables = {
      'gn': {'sources': [ 'src/gn/gn_main.cc' ], 'libs': []},

      'gn_unittests': { 'sources': [
        'src/gn/action_target_generator_unittest.cc',
        'src/gn/analyzer_unittest.cc',
        'src/gn/args_unittest.cc',
        'src/gn/builder_unittest.cc',
        'src/gn/builder_record_map_unittest.cc',
        'src/gn/c_include_iterator_unittest.cc',
        'src/gn/command_format_unittest.cc',
        'src/gn/commands_unittest.cc',
        'src/gn/compile_commands_writer_unittest.cc',
        'src/gn/config_unittest.cc',
        'src/gn/config_values_extractors_unittest.cc',
        'src/gn/escape_unittest.cc',
        'src/gn/exec_process_unittest.cc',
        'src/gn/filesystem_utils_unittest.cc',
        'src/gn/file_writer_unittest.cc',
        'src/gn/frameworks_utils_unittest.cc',
        'src/gn/function_filter_unittest.cc',
        'src/gn/function_foreach_unittest.cc',
        'src/gn/function_forward_variables_from_unittest.cc',
        'src/gn/function_get_label_info_unittest.cc',
        'src/gn/function_get_path_info_unittest.cc',
        'src/gn/function_get_target_outputs_unittest.cc',
        'src/gn/function_process_file_template_unittest.cc',
        'src/gn/function_rebase_path_unittest.cc',
        'src/gn/function_template_unittest.cc',
        'src/gn/function_toolchain_unittest.cc',
        'src/gn/function_write_file_unittest.cc',
        'src/gn/functions_target_rust_unittest.cc',
        'src/gn/functions_target_unittest.cc',
        'src/gn/functions_unittest.cc',
        'src/gn/hash_table_base_unittest.cc',
        'src/gn/header_checker_unittest.cc',
        'src/gn/input_conversion_unittest.cc',
        'src/gn/json_project_writer_unittest.cc',
        'src/gn/rust_project_writer_unittest.cc',
        'src/gn/rust_project_writer_helpers_unittest.cc',
        'src/gn/label_pattern_unittest.cc',
        'src/gn/label_unittest.cc',
        'src/gn/loader_unittest.cc',
        'src/gn/metadata_unittest.cc',
        'src/gn/metadata_walk_unittest.cc',
        'src/gn/ninja_action_target_writer_unittest.cc',
        'src/gn/ninja_binary_target_writer_unittest.cc',
        'src/gn/ninja_build_writer_unittest.cc',
        'src/gn/ninja_bundle_data_target_writer_unittest.cc',
        'src/gn/ninja_c_binary_target_writer_unittest.cc',
        'src/gn/ninja_copy_target_writer_unittest.cc',
        'src/gn/ninja_create_bundle_target_writer_unittest.cc',
        'src/gn/ninja_generated_file_target_writer_unittest.cc',
        'src/gn/ninja_group_target_writer_unittest.cc',
        'src/gn/ninja_rust_binary_target_writer_unittest.cc',
        'src/gn/ninja_target_command_util_unittest.cc',
        'src/gn/ninja_target_writer_unittest.cc',
        'src/gn/ninja_toolchain_writer_unittest.cc',
        'src/gn/operators_unittest.cc',
        'src/gn/output_conversion_unittest.cc',
        'src/gn/parse_tree_unittest.cc',
        'src/gn/parser_unittest.cc',
        'src/gn/path_output_unittest.cc',
        'src/gn/pattern_unittest.cc',
        'src/gn/pointer_set_unittest.cc',
        'src/gn/resolved_target_data_unittest.cc',
        'src/gn/resolved_target_deps_unittest.cc',
        'src/gn/runtime_deps_unittest.cc',
        'src/gn/scope_per_file_provider_unittest.cc',
        'src/gn/scope_unittest.cc',
        'src/gn/setup_unittest.cc',
        'src/gn/source_dir_unittest.cc',
        'src/gn/source_file_unittest.cc',
        'src/gn/string_atom_unittest.cc',
        'src/gn/string_output_buffer_unittest.cc',
        'src/gn/string_utils_unittest.cc',
        'src/gn/substitution_pattern_unittest.cc',
        'src/gn/substitution_writer_unittest.cc',
        'src/gn/target_public_pair_unittest.cc',
        'src/gn/target_unittest.cc',
        'src/gn/template_unittest.cc',
        'src/gn/test_with_scheduler.cc',
        'src/gn/test_with_scope.cc',
        'src/gn/tokenizer_unittest.cc',
        'src/gn/unique_vector_unittest.cc',
        'src/gn/value_unittest.cc',
        'src/gn/vector_utils_unittest.cc',
        'src/gn/version_unittest.cc',
        'src/gn/visibility_unittest.cc',
        'src/gn/visual_studio_utils_unittest.cc',
        'src/gn/visual_studio_writer_unittest.cc',
        'src/gn/xcode_object_unittest.cc',
        'src/gn/xml_element_writer_unittest.cc',
        'src/util/atomic_write_unittest.cc',
        'src/util/test/gn_test.cc',
      ], 'libs': []},
  }

  if platform.is_posix() or platform.is_zos():
    static_libraries['base']['sources'].extend([
        'src/base/files/file_enumerator_posix.cc',
        'src/base/files/file_posix.cc',
        'src/base/files/file_util_posix.cc',
        'src/base/posix/file_descriptor_shuffle.cc',
        'src/base/posix/safe_strerror.cc',
    ])

  if platform.is_zos():
    libs.extend([ options.zoslib_dir + '/install/lib/libzoslib.a' ])

  if platform.is_windows():
    static_libraries['base']['sources'].extend([
        'src/base/files/file_enumerator_win.cc',
        'src/base/files/file_util_win.cc',
        'src/base/files/file_win.cc',
        'src/base/win/registry.cc',
        'src/base/win/scoped_handle.cc',
        'src/base/win/scoped_process_information.cc',
    ])

    if platform.is_msvc():
      libs.extend([
          'advapi32.lib',
          'dbghelp.lib',
          'kernel32.lib',
          'ole32.lib',
          'shell32.lib',
          'user32.lib',
          'userenv.lib',
          'version.lib',
          'winmm.lib',
          'ws2_32.lib',
          'Shlwapi.lib',
      ])
    else:
      libs.extend([
          '-ladvapi32',
          '-ldbghelp',
          '-lkernel32',
          '-lole32',
          '-lshell32',
          '-luser32',
          '-luserenv',
          '-lversion',
          '-lwinmm',
          '-lws2_32',
          '-lshlwapi',
      ])


  libs.extend(options.link_libs)

  # we just build static libraries that GN needs
  executables['gn']['libs'].extend(static_libraries.keys())
  executables['gn_unittests']['libs'].extend(static_libraries.keys())

  WriteGenericNinja(path, static_libraries, executables, cxx, ar, ld,
                    platform, host, options, args_list,
                    cflags, ldflags, libflags, include_dirs, libs)

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
