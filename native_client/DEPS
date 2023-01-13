# DEPS files look like -*- Python -*-

vars = {
  # These revisions are slices of the chromium repository.
  # Because they come from separate sub-slices, their hashes do not match at
  # equivalent revisions. When updating them, use the roll-dep script
  # to move them to equivalent revisions. Additionally, because not all
  # directories contain commits at each revision, you will need to select
  # revisions at latest revision up to a high watermark from each slice.
  # Document the high watermark here:
  # chrome_rev: 1039183
  "build_rev": "80d1bcf5591e724463be35b794ffe3261a5b9006", # from cr commit position 1039183
  "buildtools_revision": "dbde006685de56293b98deda3130a5bcf73c354b", # from cr commit position 1038197
  "clang_rev": "0fc72d33cae3fefb8ced28631f9a8cf7e54fa873", # from cr commit position 1038345

  # build_overrides/ is a separate, NaCl-specific repo *forked* from
  # chromium/src/build_overrides/. It may need to be updated if
  # //build_overrides/build.gni changes in Chromium.
  "build_overrides_rev": "3a801f70d2dad1d2b4c935103cc44b6f16144f8d",

  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling lss
  # and whatever else without interference from each other.
  "lss_revision": "3f6478ac95edf86cd3da300c2c0d34a438f5dbeb",

  "breakpad_rev": "54fa71efbe50fb2b58096d871575b59e12edba6d",

  # GN CIPD package version.
  "gn_version": "git_revision:b79031308cc878488202beb99883ec1f2efd9a6d",

  # Separately pinned repositories, update with roll-dep individually.
  "gtest_rev": "2d3543f81d6d4583332f8b60768ade18e0f96220",
  "gyp_rev": "e7079f0e0e14108ab0dba58728ff219637458563",
  "mingw_rev": "3cc8b140b883a9fe4986d12cfd46c16a093d3527", # from svn revision 7064
  "lcov_rev": "b37daf5968200da8ff520ce65c4e5bce4047dd15", # from svn revision 149720
  "gnu_binutils_rev": "f4003433b61b25666565690caf3d7a7a1a4ec436", # from svn revision 8151
  "nsis_rev": "21b6ad22daa7bfc04b9f1c1805a34622e2607a93", # from svn revision 7071
  "ragel_rev": "da42bb33f1b67c2d70b38ec1d2edf5263271b635", # from svn revision 9010
  "third_party_rev": "d6b76289219c3183f0be9b5fc0be9daedda20e04",
  "validator_snapshots_rev": "ef053694ef9b0d98d9bed0b9bb649963084bfc81",

  # Dummy variables for compatibility with //build.
  "build_with_chromium": False,
  "cros_boards": Str(""),
  "cros_boards_with_qemu_images": Str(""),

  "chromium_git": "https://chromium.googlesource.com",
}

gclient_gn_args_file = 'build/config/gclient_args.gni'
gclient_gn_args = [
  'build_with_chromium',
  'checkout_android',  # this is provided by gclient
  'cros_boards',
  'cros_boards_with_qemu_images',
]


deps = {
  "breakpad":
    Var("chromium_git") + "/breakpad/breakpad.git@" +
    Var("breakpad_rev"),
  "buildtools":
    Var("chromium_git") + "/chromium/src/buildtools.git@" +
     Var("buildtools_revision"),
  "build":
    Var("chromium_git") + "/chromium/src/build.git@" +
    Var("build_rev"),
  "build_overrides":
    Var("chromium_git") + "/native_client/src/build_overrides.git@" +
    Var("build_overrides_rev"),
  "testing/gtest":
    (Var("chromium_git") + "/external/github.com/google/googletest.git@" +
     Var("gtest_rev")),
  "third_party":
    Var("chromium_git") + "/native_client/src/third_party.git@" +
    Var("third_party_rev"),
  "validator_snapshots":
    (Var("chromium_git") + "/native_client/src/validator_snapshots.git@" +
     Var("validator_snapshots_rev")),
  "third_party/lcov":
    Var("chromium_git") + "/chromium/src/third_party/lcov.git@" +
    Var("lcov_rev"),
  "third_party/lss":
    Var("chromium_git") + "/linux-syscall-support.git@" +
    Var("lss_revision"),
  'third_party/ninja': {
    'packages': [
      {
        # https://chrome-infra-packages.appspot.com/p/infra/3pp/tools/ninja
        'package': 'infra/3pp/tools/ninja/${{platform}}',
        'version': 'version:2@1.8.2.chromium.3',
      }
    ],
    'dep_type': 'cipd',
  },
  "tools/clang":
    Var("chromium_git") + "/chromium/src/tools/clang.git@" + Var("clang_rev"),
  "tools/gyp":
    Var("chromium_git") + "/external/gyp.git@" + Var("gyp_rev"),

  'buildtools/linux64': {
    'packages': [
      {
        'package': 'gn/gn/linux-amd64',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "linux"',
  },
  'buildtools/mac': {
    'packages': [
      {
        'package': 'gn/gn/mac-amd64',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "mac"',
  },
  'buildtools/win': {
    'packages': [
      {
        'package': 'gn/gn/windows-amd64',
        'version': Var('gn_version'),
      }
    ],
    'dep_type': 'cipd',
    'condition': 'host_os == "win"',
  },
}

deps_os = {
  "win": {
    # GNU binutils assembler for x86-32.
    "third_party/gnu_binutils":
      Var("chromium_git") +
      "/native_client/deps/third_party/gnu_binutils.git@" +
      Var("gnu_binutils_rev"),
    # GNU binutils assembler for x86-64.
    "third_party/mingw-w64/mingw/bin":
      Var("chromium_git") +
      "/native_client/deps/third_party/mingw-w64/mingw/bin.git@" +
      Var("mingw_rev"),
    "third_party/NSIS":
      Var("chromium_git") + "/native_client/deps/third_party/NSIS.git@" +
      Var("nsis_rev"),
  },
  "unix": {
    # Ragel for validator_ragel
    "third_party/ragel":
      Var("chromium_git") + "/native_client/deps/third_party/ragel.git@" +
      Var("ragel_rev"),
  },
}

hooks = [
  ###
  ### From here until the similar marker below, these clauses are copied
  ### almost verbatim from chromium/src/DEPS.  They are modified to drop
  ### the src/ prefix on file names but otherwise they should stay identical.
  ###

  {
    'name': 'sysroot_arm',
    'pattern': '.',
    'condition': 'checkout_linux',
    'action': ['python3', 'build/linux/sysroot_scripts/install-sysroot.py',
               '--arch=arm'],
  },
  {
    'name': 'sysroot_arm64',
    'pattern': '.',
    'condition': 'checkout_linux',
    'action': ['python3', 'build/linux/sysroot_scripts/install-sysroot.py',
               '--arch=arm64'],
  },
  {
    # Downloads the current stable linux sysroot to build/linux/ if needed.
    # This sysroot updates at about the same rate that the chrome build deps
    # change. This script is a no-op except for linux users who are doing
    # official chrome builds or cross compiling.
    'name': 'sysroot_x64',
    'pattern': '.',
    'condition': 'checkout_linux',
    'action': ['python3', 'build/linux/sysroot_scripts/install-sysroot.py',
               '--arch=x64'],
  },
  {
    'name': 'sysroot_x86',
    'pattern': '.',
    'condition': 'checkout_linux',
    'action': ['python3', 'build/linux/sysroot_scripts/install-sysroot.py',
               '--arch=x86'],
  },
  {
    # Update the Windows toolchain if necessary.
    'name': 'win_toolchain',
    'pattern': '.',
    'condition': 'checkout_win',
    'action': ['python3', 'build/vs_toolchain.py', 'update'],
  },
  {
    # Update the Mac toolchain if necessary.
    'name': 'mac_toolchain',
    'pattern': '.',
    'condition': 'checkout_mac',
    'action': ['python3', 'build/mac_toolchain.py'],
  },
  {
    # Pull clang if needed or requested via GYP_DEFINES.
    # Note: On Win, this should run after win_toolchain, as it may use it.
    'name': 'clang',
    'pattern': '.',
    'action': ['python3', 'tools/clang/scripts/update.py'],
  },
  {
    # Update LASTCHANGE.
    'name': 'lastchange',
    'pattern': '.',
    'action': ['python3', 'build/util/lastchange.py',
               '-o', 'build/util/LASTCHANGE'],
  },
  # Pull clang-format binaries using checked-in hashes.
  {
    'name': 'clang_format_win',
    'pattern': '.',
    'condition': 'host_os == "win"',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'buildtools/win/clang-format.exe.sha1',
    ],
  },
  {
    'name': 'clang_format_mac',
    'pattern': '.',
    'condition': 'host_os == "mac"',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'buildtools/mac/clang-format.x64.sha1',
    ],
  },
  {
    'name': 'clang_format_linux',
    'pattern': '.',
    'condition': 'host_os == "linux"',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'buildtools/linux64/clang-format.sha1',
    ],
  },

  ###
  ### End of clauses copied (almost verbatim) from chromium/src/DEPS.
  ###

  # Pull NaCl Toolchain binaries.
  {
    "pattern": ".",
    "action": ["python3",
               "native_client/build/package_version/package_version.py",
               "sync", "--extract",
    ],
  },
  # Cleanup any stale package_version files.
  {
    "pattern": ".",
    "action": ["python3",
               "native_client/build/package_version/package_version.py",
               "cleanup",
    ],
  },
]

include_rules = [
  "+native_client/src/include",
  "+gtest",
]
