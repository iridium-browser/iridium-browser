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
  'gn_version': 'git_revision:ab9104586734cb45aa77c70ca5042edbcc9f6aa5',
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
      '@' + 'c2e4795660817c2776dbabd778b92ed58c074032',
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
      '@' + '8942a9ba43d8bb196230c321d46d6a137957a719',
    'condition': 'not build_with_chromium',
  },

  'third_party/zlib/src': {
    'url': Var('github') +
      '/madler/zlib.git' +
      '@' + '21767c654d31d2dccdde4330529775c6c5fd5389', # version 1.2.12
    'condition': 'not build_with_chromium',
  },

  'third_party/jsoncpp/src': {
    'url': Var('chromium_git') +
      '/external/github.com/open-source-parsers/jsoncpp.git' +
      '@' + '5defb4ed1a4293b8e2bf641e16b156fb9de498cc', # version 1.9.5
    'condition': 'not build_with_chromium',
  },

  'third_party/googletest/src': {
    'url': Var('chromium_git') +
      '/external/github.com/google/googletest.git' +
      '@' + 'af29db7ec28d6df1c7f0f745186884091e602e07',
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
      '@' + 'b454d36994bc5f1b6794a7973858f8aa7ead654b',
    'condition': 'not build_with_chromium',
  },

  'third_party/tinycbor/src':
    Var('chromium_git') + '/external/github.com/intel/tinycbor.git' +
    '@' +  'd393c16f3eb30d0c47e6f9d92db62272f0ec4dc7',  # Version 0.6.0

  # Abseil recommends living at head.  Chromium takes an Abseil snapshot
  # irregularly, every 1-2 months. It's OK for us to come out slightly ahead
  # of Chrome's copy here.
  'third_party/abseil/src': {
    'url': Var('chromium_git') +
      '/external/github.com/abseil/abseil-cpp.git' + '@' +
      '273292d1cfc0a94a65082ee350509af1d113344d',  # lts_2022_06_23 
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
    '@' + '9c5ac792b14461c78c804d6fb0bc60667bfdb09c',
    'condition': 'not build_with_chromium',
  },

  'third_party/valijson/src': {
    'url': Var('github') + '/tristanpenman/valijson.git' +
      '@' + '2dfc7499a31b84edef71189f4247919268ebc74e', # Version 0.6
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
