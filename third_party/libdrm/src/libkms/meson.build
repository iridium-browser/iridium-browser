# Copyright © 2017-2018 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

libkms_include = [inc_root, inc_drm]
files_libkms = files(
  'linux.c',
  'dumb.c',
  'api.c',
)
if with_vmwgfx
  files_libkms += files('vmwgfx.c')
endif
if with_intel
  files_libkms += files('intel.c')
endif
if with_nouveau
  files_libkms += files('nouveau.c')
endif
if with_radeon
  files_libkms += files('radeon.c')
endif
if with_exynos
  files_libkms += files('exynos.c')
  libkms_include += include_directories('../exynos')
endif

libkms = shared_library(
  'kms',
  [files_libkms, config_file],
  c_args : libdrm_c_args,
  include_directories : libkms_include,
  link_with : libdrm,
  version : '1.0.0',
  install : true,
)

ext_libkms = declare_dependency(
  link_with : [libdrm, libkms],
  include_directories : [libkms_include],
)

install_headers('libkms.h', subdir : 'libkms')

pkg.generate(
  name : 'libkms',
  libraries : libkms,
  subdirs : ['libkms'],
  version : '1.0.0',
  requires_private : 'libdrm',
  description : 'Library that abstracts away the different mm interfaces for kernel drivers',
)

test(
  'kms-symbol-check',
  find_program('kms-symbol-check'),
  env : env_test,
  args : libkms,
)
