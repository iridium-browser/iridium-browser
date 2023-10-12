# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.
"""Recipe for building GN."""

from recipe_engine.recipe_api import Property

DEPS = [
    'recipe_engine/buildbucket',
    'recipe_engine/cas',
    'recipe_engine/cipd',
    'recipe_engine/context',
    'recipe_engine/file',
    'recipe_engine/json',
    'recipe_engine/path',
    'recipe_engine/platform',
    'recipe_engine/properties',
    'recipe_engine/raw_io',
    'recipe_engine/step',
    'target',
    'macos_sdk',
    'windows_sdk',
]

PROPERTIES = {
    'repository': Property(kind=str, default='https://gn.googlesource.com/gn'),
}

# On select platforms, link the GN executable against jemalloc for a drastic speed boost.
JEMALLOC_GIT_URL = 'https://fuchsia.googlesource.com/third_party/github.com/jemalloc/jemalloc.git'
JEMALLOC_TAG = '5.3.0'

def _get_libcxx_include_path(api):
  # Run the preprocessor with an empty input and print all include paths.
  lines = api.step(
      'xcrun toolchain', [
          'xcrun', '--toolchain', 'clang', 'clang++', '-xc++', '-fsyntax-only',
          '-Wp,-v', '/dev/null'
      ],
      stderr=api.raw_io.output_text(name='toolchain', add_output_log=True),
      step_test_data=lambda: api.raw_io.test_api.stream_output_text(
          str(api.macos_sdk.sdk_dir.join('include', 'c++', 'v1')),
          stream='stderr')).stderr.splitlines()
  # Iterate over all include paths and look for the SDK libc++ one.
  sdk_dir = str(api.macos_sdk.sdk_dir)
  for line in lines:
    line = line.strip()
    if line.startswith(sdk_dir) and 'include/c++/v1' in line:
      return line
  return None  # pragma: no cover


def _get_compilation_environment(api, target, cipd_dir):
  if target.is_linux:
    triple = '--target=%s' % target.triple
    sysroot = '--sysroot=%s' % cipd_dir.join('sysroot')
    env = {
        'CC': cipd_dir.join('bin', 'clang'),
        'CXX': cipd_dir.join('bin', 'clang++'),
        'AR': cipd_dir.join('bin', 'llvm-ar'),
        'CFLAGS': '%s %s' % (triple, sysroot),
        'LDFLAGS': '%s %s -static-libstdc++' % (triple, sysroot),
    }
  elif target.is_mac:
    triple = '--target=%s' % target.triple
    sysroot = '--sysroot=%s' % api.step(
        'xcrun sdk-path', ['xcrun', '--show-sdk-path'],
        stdout=api.raw_io.output_text(name='sdk-path', add_output_log=True),
        step_test_data=lambda: api.raw_io.test_api.stream_output_text(
            '/some/xcode/path')).stdout.strip()
    stdlib = cipd_dir.join('lib', 'libc++.a')
    cxx_include = _get_libcxx_include_path(api)
    env = {
        'CC':
            cipd_dir.join('bin', 'clang'),
        'CXX':
            cipd_dir.join('bin', 'clang++'),
        'AR':
            cipd_dir.join('bin', 'llvm-ar'),
        'CFLAGS':
            '%s %s -nostdinc++ -cxx-isystem %s' %
            (triple, sysroot, cxx_include),
        # TODO(phosek): Use the system libc++ temporarily until we
        # have universal libc++.a for macOS that supports both x86_64
        # and arm64.
        'LDFLAGS':
            '%s %s' % (triple, sysroot),
    }
  else:
    env = {}

  return env


def RunSteps(api, repository):
  src_dir = api.path['start_dir'].join('gn')

  # TODO: Verify that building and linking jemalloc works on OS X and Windows as
  # well.
  with api.step.nest('git'), api.context(infra_steps=True):
    api.step('init', ['git', 'init', src_dir])

    with api.context(cwd=src_dir):
      build_input = api.buildbucket.build_input
      ref = (
          build_input.gitiles_commit.id
          if build_input.gitiles_commit else 'refs/heads/master')
      # Fetch tags so `git describe` works.
      api.step('fetch', ['git', 'fetch', '--tags', repository, ref])
      api.step('checkout', ['git', 'checkout', 'FETCH_HEAD'])
      revision = api.step(
          'rev-parse', ['git', 'rev-parse', 'HEAD'],
          stdout=api.raw_io.output_text()).stdout.strip()
      for change in build_input.gerrit_changes:
        api.step('fetch %s/%s' % (change.change, change.patchset), [
            'git', 'fetch', repository,
            'refs/changes/%s/%s/%s' %
            (str(change.change)[-2:], change.change, change.patchset)
        ])
        api.step('checkout %s/%s' % (change.change, change.patchset),
                 ['git', 'checkout', 'FETCH_HEAD'])

  with api.context(infra_steps=True):
    cipd_dir = api.path['start_dir'].join('cipd')
    pkgs = api.cipd.EnsureFile()
    pkgs.add_package('infra/ninja/${platform}', 'version:1.8.2')
    if api.platform.is_linux or api.platform.is_mac:
      pkgs.add_package('fuchsia/third_party/clang/${platform}', 'integration')
    if api.platform.is_linux:
      pkgs.add_package('fuchsia/third_party/sysroot/linux',
                       'git_revision:c912d089c3d46d8982fdef76a50514cca79b6132',
                       'sysroot')
    api.cipd.ensure(cipd_dir, pkgs)

  def release_targets():
    if api.platform.is_linux:
      return [api.target('linux-amd64'), api.target('linux-arm64')]
    elif api.platform.is_mac:
      return [api.target('mac-amd64'), api.target('mac-arm64')]
    else:
      return [api.target.host]

  # The order is important since release build will get uploaded to CIPD.
  configs = [
      {
          'name': 'debug',
          'args': ['-d'],
          'targets': [api.target.host],
      },
      {
          'name': 'release',
          'args': ['--use-lto', '--use-icf'],
          'targets': release_targets(),
          # TODO: Enable this for OS X and Windows.
          'use_jemalloc': api.platform.is_linux,
      },
  ]

  use_jemalloc = any(c.get('use_jemalloc', False) for c in configs)

  with api.macos_sdk(), api.windows_sdk():
    # Build the jemalloc static library if needed.
    if use_jemalloc:
      # Maps a target.platform string to the location of the corresponding
      # jemalloc static library.
      jemalloc_static_libs = {}

      # Get the list of all target platforms that are listed in `configs`
      # above. Note that this is a list of Target instances, some of them
      # may refer to the same platform string (e.g. linux-amd64).
      #
      # For each platform, a version of jemalloc will be built if necessary,
      # but doing this properly requires having a valid target instance to
      # call _get_compilation_environment. So create a { platform -> Target }
      # map to do that later.
      all_config_platforms = {}
      for c in configs:
        if not c.get('use_jemalloc', False):
          continue
        for t in c['targets']:
          if t.platform not in all_config_platforms:
            all_config_platforms[t.platform] = t

      jemalloc_src_dir = api.path['start_dir'].join('jemalloc')
      with api.step.nest('jemalloc'):
        api.step('init', ['git', 'init', jemalloc_src_dir])
        with api.context(cwd=jemalloc_src_dir, infra_steps=True):
          api.step(
              'fetch',
              ['git', 'fetch', JEMALLOC_GIT_URL, 'refs/tags/' + JEMALLOC_TAG, '--depth=1'])
          api.step('checkout', ['git', 'checkout', 'FETCH_HEAD'])
          api.step('autoconf', ['autoconf'])

        for platform in all_config_platforms:
          # Convert target architecture and os to jemalloc format.
          jemalloc_build_dir = jemalloc_src_dir.join('build-' + platform)

          target = all_config_platforms[platform]
          host_target = api.target.host
          env = _get_compilation_environment(api, target, cipd_dir)

          # Prepare environment for configuring and building jemalloc
          #
          # CFLAGS: -Wno-error is required to avoid warnings that break
          #         compilation with Clang 17.
          #
          # CXXFLAGS: Required to pass the right --target=<target> argument
          #           to the C++ compiler when building jemalloc_cpp.o,
          #           otherwise that file will not be properly cross-compiled.
          #
          # Note that -flto is never passed to CFLAGS/CXXFLAGS/LDFLAGS, since
          # benchmarking shows that this leads to no performance difference
          # for the resulting GN binary.
          #
          env['CFLAGS'] += " -Wno-error"
          env['CXXFLAGS'] = env['CFLAGS']

          api.step('prepare %s build' % platform, ['mkdir', '-p', jemalloc_build_dir])

          with api.step.nest('build jemalloc-' + platform), api.context(
              env=env, cwd=jemalloc_build_dir):
            api.step(
                'configure',
                [
                  '../configure',
                  '--build=' + host_target.triple,
                  '--host=' + target.triple,
                  '--disable-shared',
                  '--enable-static',
                  '--disable-syscall',
                  '--disable-stats',
                ])
            api.step('build', ['make', '-j%d' % api.platform.cpu_count, 'build_lib_static'])
            jemalloc_static_lib = jemalloc_build_dir.join('lib', 'libjemalloc.a')

          jemalloc_static_libs[platform]  = jemalloc_static_lib

    for config in configs:
      with api.step.nest(config['name']):
        for target in config['targets']:
          env = _get_compilation_environment(api, target, cipd_dir)
          with api.step.nest(target.platform), api.context(
              env=env, cwd=src_dir):
            args = config['args']
            if config.get('use_jemalloc', False):
              args = args[:] + [
                  '--link-lib=%s' % jemalloc_static_libs[target.platform]
              ]

            api.step('generate',
                     ['python3', '-u', src_dir.join('build', 'gen.py')] + args)

            # Windows requires the environment modifications when building too.
            api.step('build',
                     [cipd_dir.join('ninja'), '-C',
                      src_dir.join('out')])

            if target.is_host:
              api.step('test', [src_dir.join('out', 'gn_unittests')])

            if config['name'] != 'release':
              continue

            with api.step.nest('upload'):
              gn = 'gn' + ('.exe' if target.is_win else '')

              if build_input.gerrit_changes:
                # Upload to CAS from CQ.
                api.cas.archive('upload binary to CAS', src_dir.join('out'),
                                src_dir.join('out', gn))
                continue

              cipd_pkg_name = 'gn/gn/%s' % target.platform

              pkg_def = api.cipd.PackageDefinition(
                  package_name=cipd_pkg_name,
                  package_root=src_dir.join('out'),
                  install_mode='copy')
              pkg_def.add_file(src_dir.join('out', gn))
              pkg_def.add_version_file('.versions/%s.cipd_version' % gn)

              cipd_pkg_file = api.path['cleanup'].join('gn.cipd')

              api.cipd.build_from_pkg(
                  pkg_def=pkg_def,
                  output_package=cipd_pkg_file,
              )

              if api.buildbucket.builder_id.project == 'infra-internal':
                cipd_pin = api.cipd.search(cipd_pkg_name,
                                           'git_revision:' + revision)
                if cipd_pin:
                  api.step('Package is up-to-date', cmd=None)
                  continue

                api.cipd.register(
                    package_name=cipd_pkg_name,
                    package_path=cipd_pkg_file,
                    refs=['latest'],
                    tags={
                        'git_repository': repository,
                        'git_revision': revision,
                    },
                )


def GenTests(api):
  for platform in ('linux', 'mac', 'win'):
    yield (api.test('ci_' + platform) + api.platform.name(platform) +
           api.buildbucket.ci_build(
               project='gn',
               git_repo='gn.googlesource.com/gn',
           ))

    yield (api.test('cq_' + platform) + api.platform.name(platform) +
           api.buildbucket.try_build(
               project='gn',
               git_repo='gn.googlesource.com/gn',
           ))

  yield (api.test('cipd_exists') + api.buildbucket.ci_build(
      project='infra-internal',
      git_repo='gn.googlesource.com/gn',
      revision='a' * 40,
  ) + api.step_data(
      'git.rev-parse', api.raw_io.stream_output_text('a' * 40)
  ) + api.step_data(
      'release.linux-amd64.upload.cipd search gn/gn/linux-amd64 git_revision:' +
      'a' * 40,
      api.cipd.example_search('gn/gn/linux-amd64',
                              ['git_revision:' + 'a' * 40])))

  yield (api.test('cipd_register') + api.buildbucket.ci_build(
      project='infra-internal',
      git_repo='gn.googlesource.com/gn',
      revision='a' * 40,
  ) + api.step_data(
      'git.rev-parse', api.raw_io.stream_output_text('a' * 40)
  ) + api.step_data(
      'release.linux-amd64.upload.cipd search gn/gn/linux-amd64 git_revision:' +
      'a' * 40, api.cipd.example_search('gn/gn/linux-amd64', [])))
