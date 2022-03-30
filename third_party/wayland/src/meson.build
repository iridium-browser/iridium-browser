project(
	'wayland', 'c', 'cpp',
	version: '1.19.0',
	license: 'MIT',
	meson_version: '>= 0.52.1',
	default_options: [
		'warning_level=2',
		'buildtype=debugoptimized'
	]
)

config_h = configuration_data()
config_h.set_quoted('PACKAGE', meson.project_name())
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())

compiler_flags = [
	'-Wno-unused-parameter',
	'-Wstrict-prototypes',
	'-Wmissing-prototypes',
	'-fvisibility=hidden',
]

cc = meson.get_compiler('c')
add_project_arguments(
	cc.get_supported_arguments(compiler_flags),
	language: 'c'
)

foreach h: [ 'sys/prctl.h' ]
	config_h.set('HAVE_' + h.underscorify().to_upper(), cc.has_header(h))
endforeach

have_funcs = [
	'accept4',
	'mkostemp',
	'posix_fallocate',
	'prctl',
	'memfd_create',
	'strndup',
]
foreach f: have_funcs
	config_h.set('HAVE_' + f.underscorify().to_upper(), cc.has_function(f))
endforeach

if get_option('libraries')
	ffi_dep = dependency('libffi')

	decls = [
		{ 'header': 'sys/signalfd.h', 'symbol': 'SFD_CLOEXEC' },
		{ 'header': 'sys/timerfd.h', 'symbol': 'TFD_CLOEXEC' },
		{ 'header': 'time.h', 'symbol': 'CLOCK_MONOTONIC' },
	]

	foreach d: decls
		if not cc.has_header_symbol(d['header'], d['symbol'])
			error('@0@ is needed to compile Wayland libraries'.format(d['symbol']))
		endif
	endforeach

	rt_dep = []
	if not cc.has_function('clock_gettime', prefix: '#include <time.h>')
		rt_dep = cc.find_library('rt')
		if not cc.has_function('clock_gettime', prefix: '#include <time.h>', dependencies: rt_dep)
			error('clock_gettime not found')
		endif
	endif
endif

scanner_deps = [ dependency('expat') ]

if get_option('dtd_validation')
	scanner_deps += dependency('libxml-2.0')
	config_h.set('HAVE_LIBXML', 1)
endif

configure_file(
	output: 'config.h',
	configuration: config_h,
)

pkgconfig = import('pkgconfig')

wayland_protocol_xml = files('protocol/wayland.xml')

root_inc = include_directories('.')
protocol_inc = include_directories('protocol')
src_inc = include_directories('src')

subdir('src')

if get_option('libraries')
	subdir('cursor')
	subdir('egl')
	subdir('tests')
	if get_option('documentation')
		subdir('doc')
	endif
endif

if get_option('scanner')
	install_data([
		'wayland-scanner.mk',
		'protocol/wayland.xml',
		'protocol/wayland.dtd',
	])

	install_data(
		[ 'wayland-scanner.m4' ],
		install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'aclocal'),
	)
endif
