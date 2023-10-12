
if not get_option('backend-x11')
	subdir_done()
endif

config_h.set('BUILD_X11_COMPOSITOR', '1')

srcs_x11 = [
	'x11.c',
	presentation_time_server_protocol_h,
]

dep_x11_xcb = dependency('xcb', version: '>= 1.8', required: false)
if not dep_x11_xcb.found()
	error('x11-backend requires xcb >= 1.8 which was not found. Or, you can use \'-Dbackend-x11=false\'.')
endif

deps_x11 = [
	dep_egl, # optional
	dep_libweston_private,
	dep_libdrm_headers,
	dep_x11_xcb,
	dep_lib_cairo_shared,
	dep_pixman,
]

foreach name : [ 'xcb-shm', 'x11', 'x11-xcb' ]
	d = dependency(name, required: false)
	if not d.found()
		error('x11-backend requires @0@ which was not found. Or, you can use \'-Dbackend-x11=false\'.'.format(name))
	endif
	deps_x11 += d
endforeach

dep_xcb_xkb = dependency('xcb-xkb', version: '>= 1.9', required: false)
if dep_xcb_xkb.found()
	deps_x11 += dep_xcb_xkb
	config_h.set('HAVE_XCB_XKB', '1')
endif

plugin_x11 = shared_library(
	'x11-backend',
	srcs_x11,
	include_directories: common_inc,
	dependencies: deps_x11,
	name_prefix: '',
	install: true,
	install_dir: dir_module_libweston
)
env_modmap += 'x11-backend.so=@0@;'.format(plugin_x11.full_path())

install_headers(backend_x11_h, subdir: dir_include_libweston_install)
