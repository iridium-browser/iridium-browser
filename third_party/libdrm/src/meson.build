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
  version : '2.4.114',
  license : 'MIT',
  meson_version : '>= 0.53',
  default_options : ['buildtype=debugoptimized', 'c_std=c11'],
)

if ['windows', 'darwin'].contains(host_machine.system())
  error('unsupported OS: @0@'.format(host_machine.system()))
endif

pkg = import('pkgconfig')

config = configuration_data()

config.set10('UDEV', get_option('udev'))
with_freedreno_kgsl = get_option('freedreno-kgsl')
with_install_tests = get_option('install-test-programs')
with_tests = get_option('tests')

if ['freebsd', 'dragonfly', 'netbsd'].contains(host_machine.system())
  dep_pthread_stubs = dependency('pthread-stubs', version : '>= 0.4')
else
  dep_pthread_stubs = []
endif
dep_threads = dependency('threads')

cc = meson.get_compiler('c')

android = cc.compiles('''int func() { return __ANDROID__; }''')

symbols_check = find_program('symbols-check.py')
prog_nm = find_program('nm')

# Check for atomics
intel_atomics = false
lib_atomics = false

python3 = import('python').find_installation()
format_mod_static_table = custom_target(
  'format_mod_static_table',
  output : 'generated_static_table_fourcc.h',
  input : 'include/drm/drm_fourcc.h',
  command : [python3, files('gen_table_fourcc.py'), '@INPUT@', '@OUTPUT@'])

dep_atomic_ops = dependency('atomic_ops', required : false)
if cc.links('''
    int atomic_add(int *i) { return __sync_add_and_fetch (i, 1); }
    int atomic_cmpxchg(int *i, int j, int k) { return __sync_val_compare_and_swap (i, j, k); }
    int main() { }
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

dep_pciaccess = dependency('pciaccess', version : '>= 0.10', required : get_option('intel'))

with_intel = false
_intel = get_option('intel')
if not _intel.disabled()
  if _intel.enabled() and not with_atomics
    error('libdrm_intel requires atomics.')
  else
    with_intel = (_intel.enabled() or host_machine.cpu_family().startswith('x86')) and with_atomics and dep_pciaccess.found()
  endif
endif
summary('Intel', with_intel)

with_radeon = false
_radeon = get_option('radeon')
if not _radeon.disabled()
  if _radeon.enabled() and not with_atomics
    error('libdrm_radeon requires atomics.')
  endif
  with_radeon = with_atomics
endif
summary('Radeon', with_radeon)

with_amdgpu = false
_amdgpu = get_option('amdgpu')
if not _amdgpu.disabled()
  if _amdgpu.enabled() and not with_atomics
    error('libdrm_amdgpu requires atomics.')
  endif
  with_amdgpu = with_atomics
endif
summary('AMDGPU', with_amdgpu)

with_nouveau = false
_nouveau = get_option('nouveau')
if not _nouveau.disabled()
  if _nouveau.enabled() and not with_atomics
    error('libdrm_nouveau requires atomics.')
  endif
  with_nouveau = with_atomics
endif
summary('Nouveau', with_nouveau)

with_vmwgfx = false
_vmwgfx = get_option('vmwgfx')
if not _vmwgfx.disabled()
  with_vmwgfx = true
endif
summary('vmwgfx', with_vmwgfx)

with_omap = false
_omap = get_option('omap')
if _omap.enabled()
  if not with_atomics
    error('libdrm_omap requires atomics.')
  endif
  with_omap = true
endif
summary('OMAP', with_omap)

with_freedreno = false
_freedreno = get_option('freedreno')
if not _freedreno.disabled()
  if _freedreno.enabled() and not with_atomics
    error('libdrm_freedreno requires atomics.')
  else
    with_freedreno = (_freedreno.enabled() or ['arm', 'aarch64'].contains(host_machine.cpu_family())) and with_atomics
  endif
endif
summary('Freedreno', with_freedreno)
summary('Freedreon-kgsl', with_freedreno_kgsl)

with_tegra = false
_tegra = get_option('tegra')
if _tegra.enabled()
  if not with_atomics
    error('libdrm_tegra requires atomics.')
  endif
  with_tegra = true
endif
summary('Tegra', with_tegra)

with_etnaviv = false
_etnaviv = get_option('etnaviv')
if not _etnaviv.disabled()
  if _etnaviv.enabled() and not with_atomics
    error('libdrm_etnaviv requires atomics.')
  endif
  with_etnaviv = _etnaviv.enabled() or (
    with_atomics and [
      'loongarch64', 'mips', 'mips64',
      'arm', 'aarch64', 'arc',
    ].contains(host_machine.cpu_family())
  )
endif
summary('Etnaviv', with_etnaviv)

with_exynos = get_option('exynos').enabled()
summary('EXYNOS', with_exynos)

with_vc4 = false
_vc4 = get_option('vc4')
if not _vc4.disabled()
  with_vc4 = _vc4.enabled() or ['arm', 'aarch64'].contains(host_machine.cpu_family())
endif
summary('VC4', with_vc4)

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

# The header is not required on Linux, and is in fact deprecated in glibc 2.30+
if ['linux'].contains(host_machine.system())
  config.set10('HAVE_SYS_SYSCTL_H', false)
else
  # From Niclas Zeising:
  # FreeBSD requires sys/types.h for sys/sysctl.h, so add it as part of
  # the includes when checking for headers.
  config.set10('HAVE_SYS_SYSCTL_H',
    cc.compiles('#include <sys/types.h>\n#include <sys/sysctl.h>', name : 'sys/sysctl.h works'))
endif

foreach header : ['sys/select.h', 'alloca.h']
  config.set10('HAVE_' + header.underscorify().to_upper(), cc.check_header(header))
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

libdrm_c_args = cc.get_supported_arguments([
  '-Wsign-compare', '-Werror=undef', '-Werror=implicit-function-declaration',
  '-Wpointer-arith', '-Wwrite-strings', '-Wstrict-prototypes',
  '-Wmissing-prototypes', '-Wmissing-declarations', '-Wnested-externs',
  '-Wpacked', '-Wswitch-enum', '-Wmissing-format-attribute',
  '-Wstrict-aliasing=2', '-Winit-self', '-Winline', '-Wshadow',
  '-Wdeclaration-after-statement', '-Wold-style-definition',
  '-Wno-unused-parameter', '-Wno-attributes', '-Wno-long-long',
  '-Wno-missing-field-initializers'])

dep_cunit = dependency('cunit', version : '>= 2.1', required : false)
dep_cairo = dependency('cairo', required : get_option('cairo-tests'))
with_cairo_tests = dep_cairo.found()

valgrind_version = []
if with_freedreno
  valgrind_version = '>=3.10.0'
endif
dep_valgrind = dependency('valgrind', required : get_option('valgrind'), version : valgrind_version)
with_valgrind = dep_valgrind.found()

prog_rst2man = find_program('rst2man', 'rst2man.py', required: get_option('man-pages'))
with_man_pages = prog_rst2man.found()

config.set10('HAVE_VISIBILITY', cc.has_function_attribute('visibility:hidden'))

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
add_project_arguments('-include', '@0@'.format(config_file), language : 'c')

inc_root = include_directories('.')
inc_drm = include_directories('include/drm')

libdrm_files = [files(
   'xf86drm.c', 'xf86drmHash.c', 'xf86drmRandom.c', 'xf86drmSL.c',
   'xf86drmMode.c'
  ),
  config_file, format_mod_static_table
]

# Build an unversioned so on android
if android
  libdrm_kw = {}
else
  libdrm_kw = {'version' : '2.4.0'}
endif

libdrm = library(
  'drm',
  libdrm_files,
  c_args : libdrm_c_args,
  dependencies : [dep_valgrind, dep_rt, dep_m],
  include_directories : inc_drm,
  install : true,
  kwargs : libdrm_kw,
  gnu_symbol_visibility : 'hidden',
)

test(
  'core-symbols-check',
  symbols_check,
  args : [
    '--lib', libdrm,
    '--symbols-file', files('core-symbols.txt'),
    '--nm', prog_nm.path(),
  ],
)

ext_libdrm = declare_dependency(
  link_with : libdrm,
  include_directories : [inc_root, inc_drm],
)

if meson.version().version_compare('>= 0.54.0')
  meson.override_dependency('libdrm', ext_libdrm)
endif

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
  libdrm,
  name : 'libdrm',
  subdirs : ['.', 'libdrm'],
  description : 'Userspace interface to kernel DRM services',
)

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
if with_tests
  subdir('tests')
endif
