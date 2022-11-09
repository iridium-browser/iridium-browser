icondir = get_option('icon_directory')
if icondir == ''
	icondir = join_paths(get_option('prefix'), get_option('datadir'), 'icons')
endif

wayland_cursor = library(
	'wayland-cursor',
	sources: [
		'wayland-cursor.c',
		'os-compatibility.c',
		'xcursor.c',
	],
	version: '0.0.0',
	dependencies: [ wayland_client_dep ],
	c_args: [ '-DICONDIR="@0@"'.format(icondir) ],
	install: true,
)

install_headers('wayland-cursor.h')

pkgconfig.generate(
	name: 'Wayland Cursor',
	description: 'Wayland cursor helper library',
	version: meson.project_version(),
	libraries: wayland_cursor,
	filebase: 'wayland-cursor',
)
