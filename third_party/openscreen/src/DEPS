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

  'checkout_chromium_quic_boringssl': False,

  # Needed to download additional clang binaries for processing coverage data
  # (from binaries with GN arg `use_coverage=true`).
  #
  # TODO(issuetracker.google.com/155195126): Change this to False and update
  # buildbot to call tools/download-clang-update-script.py instead.
  'checkout_clang_coverage_tools': True,

  # GN CIPD package version.
  'gn_version': 'git_revision:39a87c0b36310bdf06b692c098f199a0d97fc810',
  'clang_format_revision':    '99803d74e35962f63a775f29477882afd4d57d94',
}

deps = {
  # NOTE: This commit hash here references a repository/branch that is a mirror
  # of the commits to the buildtools directory in the Chromium repository. This
  # should be regularly updated with the tip of the MIRRORED master branch,
  # found here:
  # https://chromium.googlesource.com/chromium/src/buildtools/+/refs/heads/main.
  'buildtools': {
    'url': Var('chromium_git') + '/chromium/src/buildtools' +
      '@' + 'fba2905150c974240f14aa5334c3e5c93f873032',
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
      '@' + 'cacf7f1d4e3d44d871b605da3b647f07d718623f', # version 1.2.11
    'condition': 'not build_with_chromium',
  },

  'third_party/jsoncpp/src': {
    'url': Var('chromium_git') +
      '/external/github.com/open-source-parsers/jsoncpp.git' +
      '@' + '9059f5cad030ba11d37818847443a53918c327b1', # version 1.9.4
    'condition': 'not build_with_chromium',
  },

  'third_party/googletest/src': {
    'url': Var('chromium_git') +
      '/external/github.com/google/googletest.git' +
      '@' + '36d8eb532022d3b543bf55aa8ffa01b6e9f03490',
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

  'third_party/chromium_quic/src': {
    'url': Var('chromium_git') + '/openscreen/quic.git' +
      '@' + '79eec3fc28f5c4e1d06c6146825e31def6e3b793',
    'condition': 'not build_with_chromium',
  },

  # To roll forward, use quiche_revision from chromium/src/DEPS.
  'third_party/quiche/src': {
    'url': Var('quiche_git') + '/quiche.git' +
      '@' + '51f584db29001036c20db3f72f09b00b875ae625',
    'condition': 'not build_with_chromium',
  },

  'third_party/tinycbor/src':
    Var('chromium_git') + '/external/github.com/intel/tinycbor.git' +
    '@' + 'fc42a049853b802e45f49588f8148fc29d7b4d9c',

  # Abseil recommends living at head.  Chromium takes an Abseil snapshot
  # irregularly, every 1-2 months. It's OK for us to come out slightly ahead
  # of Chrome's copy here.
  'third_party/abseil/src': {
    'url': Var('chromium_git') +
      '/external/github.com/abseil/abseil-cpp.git' + '@' +
      'e19260fd7dbef881492fd73891e0be5bd4a09b95',
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
    '@' + '85754265d3847383704d19232e1ab873547cda9d',
    'condition': 'not build_with_chromium',
  },

  'third_party/valijson/src': {
    'url': Var('github') + '/tristanpenman/valijson.git' +
      '@' + 'cf648930313655b19dc07ebae2f9c3fc37966a33', # Tip-of-tree
    'condition': 'not build_with_chromium'
  }
}

hooks = [
  {
    'name': 'clang_update_script',
    'pattern': '.',
    'condition': 'not build_with_chromium',
    'action': [ 'python', 'tools/download-clang-update-script.py',
                '--output', 'tools/clang/scripts/update.py' ],
    # NOTE: This file appears in .gitignore, as it is not a part of the
    # openscreen repo.
  },
 {
    'name': 'yajsv_update_script',
    'pattern': '.',
    'condition': 'not build_with_chromium',
    'action': [ 'python', 'tools/download-yajsv.py' ],
  },
  {
    'name': 'update_clang',
    'pattern': '.',
    'condition': 'not build_with_chromium',
    'action': [ 'python', 'tools/clang/scripts/update.py' ],
  },
  {
    'name': 'clang_coverage_tools',
    'pattern': '.',
    'condition': 'not build_with_chromium and checkout_clang_coverage_tools',
    'action': ['python', 'tools/clang/scripts/update.py',
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
    'name': 'clang_format_mac',
    'pattern': '.',
    'action': [ 'download_from_google_storage.py', '--no_resume', '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'buildtools/mac/clang-format.sha1' ],
    'condition': 'host_os == "mac" and not build_with_chromium',
  },
]

recursedeps = [
  'third_party/chromium_quic/src',
  'cast',
  'buildtools',
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

skip_child_includes = [
  'third_party/chromium_quic',
]
