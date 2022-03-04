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


libdrm_radeon = shared_library(
  'drm_radeon',
  [
    files(
      'radeon_bo_gem.c', 'radeon_cs_gem.c', 'radeon_cs_space.c', 'radeon_bo.c',
      'radeon_cs.c', 'radeon_surface.c',
    ),
    config_file,
  ],
  c_args : libdrm_c_args,
  include_directories : [inc_root, inc_drm],
  link_with : libdrm,
  dependencies : [dep_pthread_stubs, dep_atomic_ops],
  version : '1.0.1',
  install : true,
)

ext_libdrm_radeon = declare_dependency(
  link_with : [libdrm, libdrm_radeon],
  include_directories : [inc_drm, include_directories('.')],
)

install_headers(
	'radeon_bo.h', 'radeon_cs.h', 'radeon_surface.h', 'radeon_bo_gem.h',
	'radeon_cs_gem.h', 'radeon_bo_int.h', 'radeon_cs_int.h', 'r600_pci_ids.h',
  subdir : 'libdrm'
)

pkg.generate(
  name : 'libdrm_radeon',
  libraries : libdrm_radeon,
  subdirs : ['.', 'libdrm'],
  version : meson.project_version(),
  requires_private : 'libdrm',
  description : 'Userspace interface to kernel DRM services for radeon',
)

test(
  'radeon-symbol-check',
  find_program('radeon-symbol-check'),
  env : env_test,
  args : libdrm_radeon,
)
