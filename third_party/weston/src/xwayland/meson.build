if not get_option('xwayland')
	subdir_done()
endif

srcs_xwayland = [
	'launcher.c',
	'window-manager.c',
	'selection.c',
	'dnd.c',
	'hash.c',
]

dep_names_xwayland = [
	'xcb',
	'xcb-composite',
	'xcb-shape',
	'xcb-xfixes',
	'xcursor',
	'cairo-xcb',
]

deps_xwayland = [ dep_libweston_public ]

foreach name : dep_names_xwayland
	d = dependency(name, required: false)
	if not d.found()
		error('xwayland requires @0@ which was not found. Or, you can use \'-Dxwayland=false\'.'.format(name))
	endif
	deps_xwayland += d
endforeach

plugin_xwayland = shared_library(
	'xwayland',
	srcs_xwayland,
	link_with: lib_cairo_shared,
	include_directories: common_inc,
	dependencies: deps_xwayland,
	name_prefix: '',
	install: true,
	install_dir: dir_module_libweston
)
env_modmap += 'xwayland.so=@0@;'.format(plugin_xwayland.full_path())

install_headers(xwayland_api_h, subdir: dir_include_libweston_install)
