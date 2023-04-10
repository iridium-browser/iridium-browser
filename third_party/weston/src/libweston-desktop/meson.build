srcs_libdesktop = [
	'libweston-desktop.c',
	'client.c',
	'seat.c',
	'surface.c',
	'xwayland.c',
	'wl-shell.c',
	'xdg-shell.c',
	'xdg-shell-v6.c',
	xdg_shell_unstable_v6_server_protocol_h,
	xdg_shell_unstable_v6_protocol_c,
	xdg_shell_server_protocol_h,
	xdg_shell_protocol_c,
]
lib_desktop = shared_library(
	'weston-desktop-@0@'.format(libweston_major),
	srcs_libdesktop,
	include_directories: common_inc,
	install: true,
	version: '0.0.@0@'.format(libweston_revision),
	dependencies: dep_libweston_public
)
dep_lib_desktop = declare_dependency(
	link_with: lib_desktop,
	dependencies: dep_libweston_public
)

pkgconfig.generate(
	lib_desktop,
	filebase: 'libweston-desktop-@0@'.format(libweston_major),
	name: 'libweston-desktop',
	version: version_weston,
	description: 'Desktop shells abstraction library for libweston compositors',
	requires_private: [ lib_weston, dep_wayland_server ],
	subdirs: dir_include_libweston
)
