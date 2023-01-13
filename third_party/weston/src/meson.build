project('weston',
	'c',
	version: '9.0.0',
	default_options: [
		'warning_level=3',
		'c_std=gnu99',
		'b_lundef=true',
	],
	meson_version: '>= 0.47',
	license: 'MIT/Expat',
)

libweston_major = 9

# libweston_revision is manufactured to follow the autotools build's
# library file naming, thanks to libtool
version_weston = meson.project_version()
version_weston_arr = version_weston.split('.')
if libweston_major > version_weston_arr[0].to_int()
	if libweston_major > version_weston_arr[0].to_int() + 1
		error('Bad versions in meson.build: libweston_major is too high')
	endif
	libweston_revision = 0
elif libweston_major == version_weston_arr[0].to_int()
	libweston_revision = version_weston_arr[2].to_int()
else
	error('Bad versions in meson.build: libweston_major is too low')
endif

dir_prefix = get_option('prefix')
dir_bin = join_paths(dir_prefix, get_option('bindir'))
dir_data = join_paths(dir_prefix, get_option('datadir'))
dir_include_libweston = 'libweston-@0@'.format(libweston_major)
dir_include_libweston_install = join_paths(dir_include_libweston, 'libweston')
dir_lib = join_paths(dir_prefix, get_option('libdir'))
dir_libexec = join_paths(dir_prefix, get_option('libexecdir'))
dir_module_weston = join_paths(dir_lib, 'weston')
dir_module_libweston = join_paths(dir_lib, 'libweston-@0@'.format(libweston_major))
dir_data_pc = join_paths(dir_data, 'pkgconfig')
dir_lib_pc = join_paths(dir_lib, 'pkgconfig')
dir_man = join_paths(dir_prefix, get_option('mandir'))
dir_protocol_libweston = join_paths('libweston-@0@'.format(libweston_major), 'protocols')

public_inc = include_directories('include')
common_inc = [ include_directories('.'), public_inc ]

pkgconfig = import('pkgconfig')

git_version_h = vcs_tag(
	input: 'libweston/git-version.h.meson',
	output: 'git-version.h',
	fallback: version_weston
)

config_h = configuration_data()

cc = meson.get_compiler('c')

global_args = []
global_args_maybe = [
	'-Wmissing-prototypes',
	'-Wno-unused-parameter',
	'-Wno-shift-negative-value', # required due to Pixman
	'-Wno-missing-field-initializers',
	'-Wno-pedantic',
	'-fvisibility=hidden',
]
foreach a : global_args_maybe
	if cc.has_argument(a)
		global_args += a
	endif
endforeach
add_global_arguments(global_args, language: 'c')

if cc.has_header_symbol('sys/sysmacros.h', 'major')
	config_h.set('MAJOR_IN_SYSMACROS', 1)
elif cc.has_header_symbol('sys/mkdev.h', 'major')
	config_h.set('MAJOR_IN_MKDEV', 1)
endif

optional_libc_funcs = [
	'mkostemp', 'strchrnul', 'initgroups', 'posix_fallocate', 'memfd_create'
]
foreach func : optional_libc_funcs
	if cc.has_function(func)
		config_h.set('HAVE_' + func.to_upper(), 1)
	endif
endforeach

optional_system_headers = [
	'linux/sync_file.h'
]
foreach hdr : optional_system_headers
	if cc.has_header(hdr)
		config_h.set('HAVE_' + hdr.underscorify().to_upper(), 1)
	endif
endforeach

env_modmap = ''

config_h.set('_GNU_SOURCE', '1')
config_h.set('_ALL_SOURCE', '1')
config_h.set('EGL_NO_X11', '1')
config_h.set('MESA_EGL_NO_X11_HEADERS', '1')

config_h.set_quoted('PACKAGE_STRING', 'weston @0@'.format(version_weston))
config_h.set_quoted('PACKAGE_VERSION', version_weston)
config_h.set_quoted('VERSION', version_weston)
config_h.set_quoted('PACKAGE_URL', 'https://wayland.freedesktop.org')
config_h.set_quoted('PACKAGE_BUGREPORT', 'https://gitlab.freedesktop.org/wayland/weston/issues/')

config_h.set_quoted('BINDIR', dir_bin)
config_h.set_quoted('DATADIR', dir_data)
config_h.set_quoted('LIBEXECDIR', dir_libexec)
config_h.set_quoted('MODULEDIR', dir_module_weston)
config_h.set_quoted('LIBWESTON_MODULEDIR', dir_module_libweston)

config_h.set10('TEST_GL_RENDERER', get_option('test-gl-renderer'))

backend_default = get_option('backend-default')
if backend_default == 'auto'
	foreach b : [ 'headless', 'fbdev', 'x11', 'wayland', 'drm' ]
		if get_option('backend-' + b)
			backend_default = b
		endif
	endforeach
endif
opt_backend_native = backend_default + '-backend.so'
config_h.set_quoted('WESTON_NATIVE_BACKEND', opt_backend_native)
message('The default backend is ' + backend_default)
if not get_option('backend-' + backend_default)
	error('Backend @0@ was chosen as native but is not being built.'.format(backend_default))
endif

dep_xkbcommon = dependency('xkbcommon', version: '>= 0.3.0')
if dep_xkbcommon.version().version_compare('>= 0.5.0')
	config_h.set('HAVE_XKBCOMMON_COMPOSE', '1')
endif

dep_wayland_server = dependency('wayland-server', version: '>= 1.17.0')
dep_wayland_client = dependency('wayland-client', version: '>= 1.17.0')
dep_pixman = dependency('pixman-1', version: '>= 0.25.2')
dep_libinput = dependency('libinput', version: '>= 0.8.0')
dep_libevdev = dependency('libevdev')
dep_libm = cc.find_library('m')
dep_libdl = cc.find_library('dl')
dep_libdrm = dependency('libdrm', version: '>= 2.4.86')
dep_libdrm_headers = dep_libdrm.partial_dependency(compile_args: true)
dep_threads = dependency('threads')

subdir('include')
subdir('protocol')
subdir('shared')
subdir('libweston')
subdir('libweston-desktop')
subdir('xwayland')
subdir('compositor')
subdir('desktop-shell')
subdir('fullscreen-shell')
subdir('ivi-shell')
subdir('kiosk-shell')
subdir('remoting')
subdir('pipewire')
subdir('clients')
subdir('wcap')
subdir('tests')
subdir('data')
subdir('man')

configure_file(output: 'config.h', configuration: config_h)

if get_option('doc')
	subdir('doc/sphinx')
else
	message('Documentation will not be built. Use -Ddoc to build it.')
endif
