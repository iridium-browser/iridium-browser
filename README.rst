Iridium Browser
===============

Iridium is an open modification of the Chromium code base, with privacy being
enhanced in several key areas. Automatic transmission of partial queries,
keywords, metrics to central services is inhibited and only occurs with
consent.

Some more information and binary downloads can be found at
https://iridiumbrowser.de/ .


Development
===========

The repository is at git://git.iridiumbrowser.de/iridium-browser and
https://github.com/iridium-browser/iridium-browser/ .

To build from source, it is advised is to re-use the mechanisms of one's Linux
distribution, that is, their build environment and description for chromium,
and replacing the source tarball by the one from
https://dl.iridiumbrowser.de/source/ .

Be aware there are different types of source code collections:

* The "chromium-git state" where one needs to run gclient to download
  additional sourc code.

* The "tarball state" where we imported the source code from
  https://commondatastorage.googleapis.com/chromium-browser-official/ into Git.
  This has all source present.

The current Git branch you are looking at and reading this README.rst from
is in the **tarball state**.


Reporting bugs and issues
=========================

Use the Iridium Browser `issue tracker`_ on GitHub to report your findings.

.. _issue tracker: https://github.com/iridium-browser/iridium-browser/issues


Build steps for chromium-git state
==================================

The following is a collection of steps, saved here primarily as a note for
ourselves. When starting off with the chromium-git state, the following
preparatory steps may be needed:

#) Download `depot_tools` and add it to your ``$PATH`` environment variable.

.. _depot_tools: https://chromium.googlesource.com/chromium/tools/depot_tools.git

#) Issue command: ``gclient sync`` (this utility comes from depot_tools). For
   gclient, you need to have python 2(!) and python2-virtualenv. The sync
   command will not necessary complete successfully due to webrtc.

#) Download WebRTC if gclient did not do it for you. Google populates the Chromium
   ``DEPS`` file with a commit hash for WebRTC, and if there is no branch head
   which is exactly equal to the hash, ``gclient sync`` barfs about it. Then,
   you have to fetch the data manually:

   .. code-block:: sh

      pushd third_party/webrtc/
      git ls-remote origin | grep 0b2302e5e0418b6716fbc0b3927874fd3a842caf abcdef... refs/branch-heads/m78
      git fetch origin refs/branch-heads/m78
      git reset --hard FETCH_HEAD
      popd

#) Undo the PATH change from 1, since depot_tools ships an untrusted copy of ninja.

From here on, steps apply to both source builds of any kind, that is,
chromium-git or tarball state.


Steps for tarball state
=======================

5) The gn files in Iridium are edited to respect the ``$CC`` etc. environment variables.
   These env vars _must_ always be set, so issue

   .. code-block:: sh

      export CC=gcc CXX=g++ AR=ar NM=nm

   or

   .. code-block:: sh

      export CC=clang CXX=clang++ AR=llvm-ar NM=llvm-nm

   (can pick any preferred toolchain, though).

#) Link up nodejs:

   .. code-block:: sh

      mkdir -p third_party/node/linux/node-linux-x64/bin
      ln -s /usr/bin/node third_party/node/linux/node-linux-x64/bin/

#) Generate Makefilery:

   .. code-block:: sh

      export CFLAGS="-O2 -Wall $(pkg-config wayland-client xkbcommon --cflags)"
      export CXXFLAGS="$CFLAGS"
      gn gen '--args= custom_toolchain="//build/toolchain/linux/unbundle:default" host_toolchain="//build/toolchain/linux/unbundle:default" use_custom_libcxx=false host_cpu="x64" host_os="linux" is_debug=false dcheck_always_on=false enable_nacl=false use_swiftshader_with_subzero=true is_component_ffmpeg=true use_cups=true use_aura=true symbol_level=1 blink_symbol_level=0 use_kerberos=true enable_vr=false optimize_webui=false enable_reading_list=false use_pulseaudio=true link_pulseaudio=true is_component_build=false use_sysroot=false fatal_linker_warnings=false use_allocator_shim=true use_partition_alloc=true disable_fieldtrial_testing_config=true use_unofficial_version_number=false use_vaapi=true use_sysroot=false treat_warnings_as_errors=false enable_widevine=false use_dbus=true media_use_openh264=false rtc_use_h264=false use_v8_context_snapshot=true v8_use_external_startup_data=true enable_rust=false gtk_version=4 moc_qt5_path="/usr/lib64/qt5/bin" use_qt6=true moc_qt6_path="/usr/libexec/qt6" use_system_harfbuzz=true use_system_freetype=true enable_hangout_services_extension=true enable_vulkan=true rtc_use_pipewire=true rtc_link_pipewire=true is_clang=true clang_base_path="/usr" clang_use_chrome_plugins=false use_thin_lto=true use_lld=true icu_use_data_file=false proprietary_codecs=true ffmpeg_branding="Chrome"' out

   Note that gn embodies the environment variables' values (CC, CFLAGS, etc.)
   into Makefiles, so upon change of any of those variables, gn needs to be
   re-run.

#) There are a number of dependencies, and they may vary across operating
   systems. The gn command fails if there are unmet dependencies, and it will
   tell you which. Install and repeat the gn command as needed. Consult your
   distribution's package manager. On openSUSE, it is possible to use ``zypper
   si -d chromium`` to do that in a single shot. On Debian, there is something
   like ``apt-get build-dep``.

#) Execute this to build:

   .. code-block:: sh

      export PATH="$PATH:$PWD"
      ln -s /usr/libexec/qt6/moc moc-qt6
      LD_LIBRARY_PATH=$PWD/out nice -n20 ninja -C out chrome chromedriver

   Because chromium is too ignorant to look for moc in the right place and
   name, or query some qt utility for the desired info, the path to it needs to
   be manually specified.

#) The just-built executable can be run thus

   .. code-block:: sh

      cd out; LD_LIBRARY_PATH=$PWD ./chromium


Installation procedure
======================

There is no "install" target. Thanks, Google.
Every distro has to roll their own install procedure. For example,

 * https://github.com/bmwiedemann/openSUSE/blob/master/packages/c/chromium/chromium.spec#L869
 * https://github.com/archlinux/svntogit-packages/blob/packages/chromium/trunk/PKGBUILD#L219

This is why it was mentioned in one of the previous sections that you will have
to re-use/leverage/adapt the chromium build recipe that your distro had.
