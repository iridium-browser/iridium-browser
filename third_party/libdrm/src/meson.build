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

project(
  'libdrm',
  ['c'],
  version : '2.4.100',
  license : 'MIT',
  meson_version : '>= 0.43',
  default_options : ['buildtype=debugoptimized', 'c_std=gnu99'],
)

pkg = import('pkgconfig')

config = configuration_data()

config.set10('UDEV', get_option('udev'))
with_freedreno_kgsl = get_option('freedreno-kgsl')
with_install_tests = get_option('install-test-programs')

if ['freebsd', 'dragonfly', 'netbsd'].contains(host_machine.system())
  dep_pthread_stubs = dependency('pthread-stubs', version : '>= 0.4')
else
  dep_pthread_stubs = []
endif
dep_threads = dependency('threads')

cc = meson.get_compiler('c')

# Check for atomics
intel_atomics = false
lib_atomics = false

dep_atomic_ops = dependency('atomic_ops', required : false)
if cc.compiles('''
    int atomic_add(int *i) { return __sync_add_and_fetch (i, 1); }
    int atomic_cmpxchg(int *i, int j, int k) { return __sync_val_compare_and_swap (i, j, k); }
    ''',
    name : 'Intel Atomics')
  intel_atomics = true
  with_atomics = true
  dep_atomic_ops = []
elif dep_atomic_ops.found()
  lib_atomics = true
  with_atomics = true
elif cc.has_function('atomic_cas_uint')
  with_atomics = true
else
  with_atomics = false
endif

config.set10('HAVE_LIBDRM_ATOMIC_PRIMITIVES', intel_atomics)
config.set10('HAVE_LIB_ATOMIC_OPS', lib_atomics)

with_intel = false
_intel = get_option('intel')
if _intel != 'false'
  if _intel == 'true' and not with_atomics
    error('libdrm_intel requires atomics.')
  else
    with_intel = _intel == 'true' or host_machine.cpu_family().startswith('x86')
  endif
endif

with_radeon = false
_radeon = get_option('radeon')
if _radeon != 'false'
  if _radeon == 'true' and not with_atomics
    error('libdrm_radeon requires atomics.')
  endif
  with_radeon = true
endif

with_amdgpu = false
_amdgpu = get_option('amdgpu')
if _amdgpu != 'false'
  if _amdgpu == 'true' and not with_atomics
    error('libdrm_amdgpu requires atomics.')
  endif
  with_amdgpu = true
endif

with_nouveau = false
_nouveau = get_option('nouveau')
if _nouveau != 'false'
  if _nouveau == 'true' and not with_atomics
    error('libdrm_nouveau requires atomics.')
  endif
  with_nouveau = true
endif

with_vmwgfx = false
_vmwgfx = get_option('vmwgfx')
if _vmwgfx != 'false'
  with_vmwgfx = true
endif

with_omap = false
_omap = get_option('omap')
if _omap == 'true'
  if not with_atomics
    error('libdrm_omap requires atomics.')
  endif
  with_omap = true
endif

with_freedreno = false
_freedreno = get_option('freedreno')
if _freedreno != 'false'
  if _freedreno == 'true' and not with_atomics
    error('libdrm_freedreno requires atomics.')
  else
    with_freedreno = _freedreno == 'true' or ['arm', 'aarch64'].contains(host_machine.cpu_family())
  endif
endif

with_tegra = false
_tegra = get_option('tegra')
if _tegra == 'true'
  if not with_atomics
    error('libdrm_tegra requires atomics.')
  endif
  with_tegra = true
endif

with_etnaviv = false
_etnaviv = get_option('etnaviv')
if _etnaviv == 'true'
  if not with_atomics
    error('libdrm_etnaviv requires atomics.')
  endif
  with_etnaviv = true
endif

with_exynos = get_option('exynos') == 'true'

with_vc4 = false
_vc4 = get_option('vc4')
if _vc4 != 'false'
  with_vc4 = _vc4 == 'true' or ['arm', 'aarch64'].contains(host_machine.cpu_family())
endif

# XXX: Apparently only freebsd and dragonfly bsd actually need this (and
# gnu/kfreebsd), not openbsd and netbsd
with_libkms = false
_libkms = get_option('libkms')
if _libkms != 'false'
  with_libkms = _libkms == 'true' or ['linux', 'freebsd', 'dragonfly'].contains(host_machine.system())
endif

# Among others FreeBSD does not have a separate dl library.
if not cc.has_function('dlsym')
  dep_dl = cc.find_library('dl', required : with_nouveau)
else
  dep_dl = []
endif
# clock_gettime might require -rt, or it might not. find out
if not cc.has_function('clock_gettime', prefix : '#define _GNU_SOURCE\n#include <time.h>')
  # XXX: untested
  dep_rt = cc.find_library('rt')
else
  dep_rt = []
endif
dep_m = cc.find_library('m', required : false)
# From Niclas Zeising:
# FreeBSD requires sys/types.h for sys/sysctl.h, add it as part of the
# includes when checking for headers.
foreach header : ['sys/sysctl.h', 'sys/select.h', 'alloca.h']
  config.set('HAVE_' + header.underscorify().to_upper(),
    cc.compiles('#include <sys/types.h>\n#include <@0@>'.format(header), name : '@0@ works'.format(header)))
endforeach
if (cc.has_header_symbol('sys/sysmacros.h', 'major') and
  cc.has_header_symbol('sys/sysmacros.h', 'minor') and
  cc.has_header_symbol('sys/sysmacros.h', 'makedev'))
  config.set10('MAJOR_IN_SYSMACROS', true)
endif
if (cc.has_header_symbol('sys/mkdev.h', 'major') and
  cc.has_header_symbol('sys/mkdev.h', 'minor') and
  cc.has_header_symbol('sys/mkdev.h', 'makedev'))
  config.set10('MAJOR_IN_MKDEV', true)
endif
config.set10('HAVE_OPEN_MEMSTREAM', cc.has_function('open_memstream'))

warn_c_args = []
foreach a : ['-Wall', '-Wextra', '-Wsign-compare', '-Werror=undef',
             '-Werror=implicit-function-declaration', '-Wpointer-arith',
             '-Wwrite-strings', '-Wstrict-prototypes', '-Wmissing-prototypes',
             '-Wmissing-declarations', '-Wnested-externs', '-Wpacked',
             '-Wswitch-enum', '-Wmissing-format-attribute',
             '-Wstrict-aliasing=2', '-Winit-self', '-Winline', '-Wshadow',
             '-Wdeclaration-after-statement', '-Wold-style-definition']
  if cc.has_argument(a)
    warn_c_args += a
  endif
endforeach
# GCC will never error for -Wno-*, so check for -W* then add -Wno-* to the list
# of options
foreach a : ['unused-parameter', 'attributes', 'long-long',
             'missing-field-initializers']
  if cc.has_argument('-W@0@'.format(a))
    warn_c_args += '-Wno-@0@'.format(a)
  endif
endforeach

# all c args:
libdrm_c_args = warn_c_args + ['-fvisibility=hidden']


dep_pciaccess = dependency('pciaccess', version : '>= 0.10', required : with_intel)
dep_cunit = dependency('cunit', version : '>= 2.1', required : false)
_cairo_tests = get_option('cairo-tests')
if _cairo_tests != 'false'
  dep_cairo = dependency('cairo', required : _cairo_tests == 'true')
  with_cairo_tests = dep_cairo.found()
else
  dep_cairo = []
  with_cairo_tests = false
endif
_valgrind = get_option('valgrind')
if _valgrind != 'false'
  dep_valgrind = dependency('valgrind', required : _valgrind == 'true')
  with_valgrind = dep_valgrind.found()
else
  dep_valgrind = []
  with_valgrind = false
endif

with_man_pages = get_option('man-pages')
prog_xslt = find_program('xsltproc', required : with_man_pages == 'true')
prog_sed = find_program('sed', required : with_man_pages == 'true')
manpage_style = 'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl'
if prog_xslt.found()
  if run_command(prog_xslt, '--nonet', manpage_style).returncode() != 0
    if with_man_pages == 'true'
      error('Manpage style sheet cannot be found')
    endif
    with_man_pages = 'false'
  endif
endif
with_man_pages = with_man_pages != 'false' and prog_xslt.found() and prog_sed.found()

config.set10('HAVE_VISIBILITY',
  cc.compiles('''int foo_hidden(void) __attribute__((visibility(("hidden"))));''',
              name : 'compiler supports __attribute__(("hidden"))'))

foreach t : [
             [with_exynos, 'EXYNOS'],
             [with_freedreno_kgsl, 'FREEDRENO_KGSL'],
             [with_intel, 'INTEL'],
             [with_nouveau, 'NOUVEAU'],
             [with_radeon, 'RADEON'],
             [with_vc4, 'VC4'],
             [with_vmwgfx, 'VMWGFX'],
             [with_cairo_tests, 'CAIRO'],
             [with_valgrind, 'VALGRIND'],
            ]
  config.set10('HAVE_@0@'.format(t[1]), t[0])
endforeach
if with_freedreno_kgsl and not with_freedreno
  error('cannot enable freedreno-kgsl without freedreno support')
endif
config.set10('_GNU_SOURCE', true)
config_file = configure_file(
  configuration : config,
  output : 'config.h',
)
add_project_arguments('-include', 'config.h', language : 'c')

inc_root = include_directories('.')
inc_drm = include_directories('include/drm')

libdrm = shared_library(
  'drm',
  [files(
     'xf86drm.c', 'xf86drmHash.c', 'xf86drmRandom.c', 'xf86drmSL.c',
     'xf86drmMode.c'
   ),
   config_file,
  ],
  c_args : libdrm_c_args,
  dependencies : [dep_valgrind, dep_rt, dep_m],
  include_directories : inc_drm,
  version : '2.4.0',
  install : true,
)

ext_libdrm = declare_dependency(
  link_with : libdrm,
  include_directories : [inc_root, inc_drm],
)

install_headers('libsync.h', 'xf86drm.h', 'xf86drmMode.h')
install_headers(
  'include/drm/drm.h', 'include/drm/drm_fourcc.h', 'include/drm/drm_mode.h',
  'include/drm/drm_sarea.h', 'include/drm/i915_drm.h',
  'include/drm/mach64_drm.h', 'include/drm/mga_drm.h',
  'include/drm/msm_drm.h', 'include/drm/nouveau_drm.h',
  'include/drm/qxl_drm.h', 'include/drm/r128_drm.h',
  'include/drm/radeon_drm.h', 'include/drm/amdgpu_drm.h',
  'include/drm/savage_drm.h', 'include/drm/sis_drm.h',
  'include/drm/tegra_drm.h', 'include/drm/vc4_drm.h',
  'include/drm/via_drm.h', 'include/drm/virtgpu_drm.h',
  subdir : 'libdrm',
)
if with_vmwgfx
  install_headers('include/drm/vmwgfx_drm.h', subdir : 'libdrm')
endif

pkg.generate(
  name : 'libdrm',
  libraries : libdrm,
  subdirs : ['.', 'libdrm'],
  version : meson.project_version(),
  description : 'Userspace interface to kernel DRM services',
)

env_test = environment()
env_test.set('NM', find_program('nm').path())

if with_libkms
  subdir('libkms')
endif
if with_intel
  subdir('intel')
endif
if with_nouveau
  subdir('nouveau')
endif
if with_radeon
  subdir('radeon')
endif
if with_amdgpu
  subdir('amdgpu')
endif
if with_omap
  subdir('omap')
endif
if with_exynos
  subdir('exynos')
endif
if with_freedreno
  subdir('freedreno')
endif
if with_tegra
  subdir('tegra')
endif
if with_vc4
  subdir('vc4')
endif
if with_etnaviv
  subdir('etnaviv')
endif
if with_man_pages
  subdir('man')
endif
subdir('data')
subdir('tests')

message('')
message('@0@ will be compiled with:'.format(meson.project_name()))
message('')
message('  libkms         @0@'.format(with_libkms))
message('  Intel API      @0@'.format(with_intel))
message('  vmwgfx API     @0@'.format(with_vmwgfx))
message('  Radeon API     @0@'.format(with_radeon))
message('  AMDGPU API     @0@'.format(with_amdgpu))
message('  Nouveau API    @0@'.format(with_nouveau))
message('  OMAP API       @0@'.format(with_omap))
message('  EXYNOS API     @0@'.format(with_exynos))
message('  Freedreno API  @0@ (kgsl: @1@)'.format(with_freedreno, with_freedreno_kgsl))
message('  Tegra API      @0@'.format(with_tegra))
message('  VC4 API        @0@'.format(with_vc4))
message('  Etnaviv API    @0@'.format(with_etnaviv))
message('')
