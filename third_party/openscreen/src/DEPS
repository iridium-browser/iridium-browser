# This file is used to manage the dependencies of the Open Screen repo. It is
# used by gclient to determine what version of each dependency to check out.
#
# For more information, please refer to the official documentation:
#   https://sites.google.com/a/chromium.org/dev/developers/how-tos/get-the-code
#
# When adding a new dependency, please update the top-level .gitignore file
# to list the dependency's destination directory.

use_relative_paths = True

vars = {
  'boringssl_git': 'https://boringssl.googlesource.com',
  'chromium_git': 'https://chromium.googlesource.com',
  'quiche_git': 'https://quiche.googlesource.com',

  # NOTE: we should only reference GitHub directly for dependencies toggled
  # with the "not build_with_chromium" condition.
  'github': 'https://github.com',

  # NOTE: Strangely enough, this will be overridden by any _parent_ DEPS, so
  # in Chromium it will correctly be True.
  'build_with_chromium': False,

  # Needed to download additional clang binaries for processing coverage data
  # (from binaries with GN arg `use_coverage=true`).
  #
  # TODO(issuetracker.google.com/155195126): Change this to False and update
  # buildbot to call tools/download-clang-update-script.py instead.
  'checkout_clang_coverage_tools': True,

  # GN CIPD package version.
  'gn_version': 'git_revision:5e19d2fb166fbd4f6f32147fbb2f497091a54ad8',
  'clang_format_revision':    'e435ad79c17b1888b34df88d6a30a094936e3836',
}

deps = {
  # NOTE: This commit hash here references a repository/branch that is a mirror
  # of the commits to the buildtools directory in the Chromium repository. This
  # should be regularly updated with the tip of the MIRRORED master branch,
  # found here:
  # https://chromium.googlesource.com/chromium/src/buildtools/+/refs/heads/main
  'buildtools': {
    'url': Var('chromium_git') + '/chromium/src/buildtools' +
      '@' + '3c7e3f1b8b1e4c0b6ec693430379cea682de78d6',
    'condition': 'not build_with_chromium',
  },
  'buildtools/clang_format/script': {
    'url': Var('chromium_git') +
      '/external/github.com/llvm/llvm-project/clang/tools/clang-format.git' +
      '@' + Var('clang_format_revision'),
    'condition': 'not build_with_chromium',
  },
  'buildtools/linux64': {
    'packages': [
      {
        'package': 'gn/gn/linux-amd64',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "linux" and not build_with_chromium',
  },
  'buildtools/mac': {
    'packages': [
      {
        'package': 'gn/gn/mac-${{arch}}',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "mac" and not build_with_chromium',
  },

  'third_party/ninja': {
    'packages': [
      # https://chrome-infra-packages.appspot.com/p/infra/3pp/tools/ninja
      {
        'package': 'infra/3pp/tools/ninja/${{platform}}',
        'version': 'version:2@1.8.2.chromium.3',
      }
    ],
    'dep_type': 'cipd',
    'condition': 'not build_with_chromium',
  },

  'third_party/protobuf/src': {
    'url': Var('chromium_git') +
      '/external/github.com/protocolbuffers/protobuf.git' +
      '@' + '909a0f36a10075c4b4bc70fdee2c7e32dd612a72', # version 3.17.3
    'condition': 'not build_with_chromium',
  },

  'third_party/libprotobuf-mutator/src': {
    'url': Var('chromium_git') +
      '/external/github.com/google/libprotobuf-mutator.git' +
      '@' + 'a304ec48dcf15d942607032151f7e9ee504b5dcf',
    'condition': 'not build_with_chromium',
  },

  'third_party/zlib/src': {
    'url': Var('github') +
      '/madler/zlib.git' +
      '@' + '04f42ceca40f73e2978b50e93806c2a18c1281fc', # version 1.2.13
    'condition': 'not build_with_chromium',
  },

  'third_party/jsoncpp/src': {
    'url': Var('chromium_git') +
      '/external/github.com/open-source-parsers/jsoncpp.git' +
      '@' + '5defb4ed1a4293b8e2bf641e16b156fb9de498cc', # version 1.9.5
    'condition': 'not build_with_chromium',
  },

  # googletest now recommends "living at head," which is a bit of a crapshoot
  # because regressions land upstream frequently.  This is a known good revision.
  'third_party/googletest/src': {
    'url': Var('chromium_git') +
      '/external/github.com/google/googletest.git' +
      '@' + 'b495f72f1f096135cf9cf8c7879b5b89250de50a',  # 2023-01-25
    'condition': 'not build_with_chromium',
  },

  # Note about updating BoringSSL: after changing this hash, run the update
  # script in BoringSSL's util folder for generating build files from the
  # <openscreen src-dir>/third_party/boringssl directory:
  # python ./src/util/generate_build_files.py gn
  'third_party/boringssl/src': {
    'url' : Var('boringssl_git') + '/boringssl.git' +
      '@' + 'f6bd54efbcafcf4625ce99b5f702dc4850b0ca50',
    'condition': 'not build_with_chromium',
  },

  # To roll forward, use quiche_revision from chromium/src/DEPS.
  'third_party/quiche/src': {
    'url': Var('quiche_git') + '/quiche.git' +
      '@' + '7b58beaec1f65b8e074ddb81796e34f3d6d83cf3',
    'condition': 'not build_with_chromium',
  },

  'third_party/tinycbor/src':
    Var('chromium_git') + '/external/github.com/intel/tinycbor.git' +
    '@' +  'd393c16f3eb30d0c47e6f9d92db62272f0ec4dc7',  # Version 0.6.0

  # Abseil recommends living at head; we take a revision from one of the LTS
  # tags.  Chromium has forked abseil for reasons and it seems to be rolled
  # frequently, but LTS should generally be safe for interop with Chromium code.
  'third_party/abseil/src': {
    'url': Var('chromium_git') +
      '/external/github.com/abseil/abseil-cpp.git' + '@' +
      '78be63686ba732b25052be15f8d6dee891c05749',  # lts_2023_01_25
    'condition': 'not build_with_chromium',
  },

  'third_party/libfuzzer/src': {
    'url': Var('chromium_git') +
      '/chromium/llvm-project/compiler-rt/lib/fuzzer.git' +
      '@' + 'debe7d2d1982e540fbd6bd78604bf001753f9e74',
    'condition': 'not build_with_chromium',
  },

  'third_party/modp_b64': {
    'url': Var('chromium_git') + '/chromium/src/third_party/modp_b64'
    '@' + '3643752c065d984647f0ded68a9a01926fb3b9cd',  # 2022-11-28
    'condition': 'not build_with_chromium',
  },

  'third_party/valijson/src': {
    'url': Var('github') + '/tristanpenman/valijson.git' +
      '@' + '78ac8a737df56b5334354efe104ea8f99e2a2f00', # Version 1.0
    'condition': 'not build_with_chromium'
  }
}

hooks = [
  {
    'name': 'clang_update_script',
    'pattern': '.',
    'condition': 'not build_with_chromium',
    'action': [ 'tools/download-clang-update-script.py',
                '--output', 'tools/clang/scripts/update.py' ],
    # NOTE: This file appears in .gitignore, as it is not a part of the
    # openscreen repo.
  },
  {
    'name': 'update_clang',
    'pattern': '.',
    'condition': 'not build_with_chromium',
    'action': [ 'python3', 'tools/clang/scripts/update.py' ],
  },
  {
    'name': 'clang_coverage_tools',
    'pattern': '.',
    'condition': 'not build_with_chromium and checkout_clang_coverage_tools',
    'action': ['python3', 'tools/clang/scripts/update.py',
               '--package=coverage_tools'],
  },
  {
    'name': 'clang_format_linux64',
    'pattern': '.',
    'action': [ 'download_from_google_storage.py', '--no_resume', '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'buildtools/linux64/clang-format.sha1' ],
    'condition': 'host_os == "linux" and not build_with_chromium',
  },
  {
    'name': 'clang_format_mac_x64',
    'pattern': '.',
    'action': [ 'download_from_google_storage.py', '--no_resume', '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'buildtools/mac/clang-format.x64.sha1' ],
    'condition': 'host_os == "mac" and host_cpu == "x64" and not build_with_chromium',
  },
  {
    'name': 'clang_format_mac_arm64',
    'pattern': '.',
    'action': [ 'download_from_google_storage.py', '--no_resume', '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'buildtools/mac/clang-format.arm64.sha1' ],
    'condition': 'host_os == "mac" and host_cpu == "arm64" and not build_with_chromium',
  },
]

recursedeps = [
  'cast',
]

include_rules = [
  '+util',
  '+platform/api',
  '+platform/base',
  '+platform/test',
  '+testing/util',
  '+third_party',

  # Inter-module dependencies must be through public APIs.
  '-discovery',
  '+discovery/common',
  '+discovery/dnssd/public',
  '+discovery/mdns/public',
  '+discovery/public',

  # Don't include abseil from the root so the path can change via include_dirs
  # rules when in Chromium.
  '-third_party/abseil',

  # Abseil allowed headers.
  '+absl/algorithm/container.h',
  '+absl/base/thread_annotations.h',
  '+absl/hash/hash.h',
  '+absl/hash/hash_testing.h',
  '+absl/strings/ascii.h',
  '+absl/strings/match.h',
  '+absl/strings/numbers.h',
  '+absl/strings/str_cat.h',
  '+absl/strings/str_join.h',
  '+absl/strings/str_replace.h',
  '+absl/strings/str_split.h',
  '+absl/strings/string_view.h',
  '+absl/strings/substitute.h',
  '+absl/types/optional.h',
  '+absl/types/span.h',
  '+absl/types/variant.h',

  # Similar to abseil, don't include boringssl using root path.  Instead,
  # explicitly allow 'openssl' where needed.
  '-third_party/boringssl',

  # Test framework includes.
  "-third_party/googletest",
  "+gtest",
  "+gmock",
]
