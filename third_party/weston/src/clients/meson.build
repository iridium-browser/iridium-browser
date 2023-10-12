if get_option('resize-pool')
	config_h.set('USE_RESIZE_POOL', '1')
endif

srcs_toytoolkit = [
	'window.c',
	xdg_shell_client_protocol_h,
	xdg_shell_protocol_c,
	text_cursor_position_client_protocol_h,
	text_cursor_position_protocol_c,
	relative_pointer_unstable_v1_client_protocol_h,
	relative_pointer_unstable_v1_protocol_c,
	pointer_constraints_unstable_v1_client_protocol_h,
	pointer_constraints_unstable_v1_protocol_c,
	tablet_unstable_v2_client_protocol_h,
	tablet_unstable_v2_protocol_c,
	ivi_application_client_protocol_h,
	ivi_application_protocol_c,
	viewporter_client_protocol_h,
	viewporter_protocol_c,
]
deps_toytoolkit = [
	dep_wayland_client,
	dep_lib_cairo_shared,
	dep_matrix_c,
	dep_xkbcommon,
	dependency('wayland-cursor'),
	cc.find_library('util'),
]
lib_toytoolkit = static_library(
	'toytoolkit',
	srcs_toytoolkit,
	include_directories: common_inc,
	dependencies: deps_toytoolkit,
	install: false,
)
dep_toytoolkit = declare_dependency(
	link_with: lib_toytoolkit,
	dependencies: deps_toytoolkit,
)
dep_gbm = dependency('gbm', required: false)
if dep_gbm.found() and dep_gbm.version().version_compare('>= 21.3')
	config_h.set('HAVE_GBM_BO_CREATE_WITH_MODIFIERS2', '1')
endif

simple_clients_enabled = get_option('simple-clients')
simple_build_all = simple_clients_enabled.contains('all')

simple_clients = [
	{
		'name': 'damage',
		'sources': [
			'simple-damage.c',
			viewporter_client_protocol_h,
			viewporter_protocol_c,
			xdg_shell_client_protocol_h,
			xdg_shell_protocol_c,
			fullscreen_shell_unstable_v1_client_protocol_h,
			fullscreen_shell_unstable_v1_protocol_c,
		],
		'dep_objs': [ dep_wayland_client, dep_libshared ]
	},
	{
		'name': 'dmabuf-feedback',
		'sources': [
			'simple-dmabuf-feedback.c',
			'../libweston/pixel-formats.c',
			linux_dmabuf_unstable_v1_client_protocol_h,
			linux_dmabuf_unstable_v1_protocol_c,
			presentation_time_client_protocol_h,
			presentation_time_protocol_c,
			xdg_shell_client_protocol_h,
			xdg_shell_protocol_c,
		],
		'dep_objs': [
			dep_wayland_client,
			dep_libshared,
			dep_pixman,
			dep_libdrm,
			dependency('libudev', version: '>= 136'),
			# gbm_bo_get_fd_for_plane() from 21.1.0
			dependency('gbm', version: '>= 21.1.1',
				required: simple_build_all or simple_clients_enabled.contains('dmabuf-feedback'),
				not_found_message: 'dmabuf-feedback requires gbm which was not found. If you rather not build this, drop "dmabuf-feedback" from simple-clients option.',
				disabler: true)
		],
		'deps': [ 'egl', 'glesv2', 'gbm' ],
		'options': [ 'renderer-gl' ]
	},
	{
		'name': 'dmabuf-egl',
		'sources': [
			'simple-dmabuf-egl.c',
			linux_dmabuf_unstable_v1_client_protocol_h,
			linux_dmabuf_unstable_v1_protocol_c,
			linux_explicit_synchronization_unstable_v1_client_protocol_h,
			linux_explicit_synchronization_unstable_v1_protocol_c,
			xdg_shell_client_protocol_h,
			xdg_shell_protocol_c,
			weston_direct_display_client_protocol_h,
			weston_direct_display_protocol_c,
			fullscreen_shell_unstable_v1_client_protocol_h,
			fullscreen_shell_unstable_v1_protocol_c,
		],
		'dep_objs': [
			dep_wayland_client,
			dep_libdrm,
			dep_libm,
			dep_matrix_c,
		],
		'deps': [ 'egl', 'glesv2', 'gbm' ],
		'options': [ 'renderer-gl' ]
	},
	{
		'name': 'dmabuf-v4l',
		'sources': [
			'simple-dmabuf-v4l.c',
			linux_dmabuf_unstable_v1_client_protocol_h,
			linux_dmabuf_unstable_v1_protocol_c,
			xdg_shell_client_protocol_h,
			xdg_shell_protocol_c,
			weston_direct_display_client_protocol_h,
			weston_direct_display_protocol_c,
			fullscreen_shell_unstable_v1_client_protocol_h,
			fullscreen_shell_unstable_v1_protocol_c,
		],
		'dep_objs': [ dep_wayland_client, dep_libdrm_headers ]
	},
	{
		'name': 'egl',
		'sources': [
			'simple-egl.c',
			fractional_scale_v1_client_protocol_h,
			fractional_scale_v1_protocol_c,
			tearing_control_v1_client_protocol_h,
			tearing_control_v1_protocol_c,
			viewporter_client_protocol_h,
			viewporter_protocol_c,
			xdg_shell_client_protocol_h,
			xdg_shell_protocol_c,
			ivi_application_client_protocol_h,
			ivi_application_protocol_c,
		],
		'dep_objs': [
			dep_libm,
			dep_libshared,
			dep_matrix_c,
			dep_wayland_client,
		],
		'deps': [ 'egl', 'wayland-egl', 'glesv2', 'wayland-cursor' ],
		'options': [ 'renderer-gl' ]
	},
	# weston-simple-im is handled specially separately due to install_dir and odd window.h usage
	{
		'name': 'shm',
		'sources': [
			'simple-shm.c',
			xdg_shell_client_protocol_h,
			xdg_shell_protocol_c,
			fullscreen_shell_unstable_v1_client_protocol_h,
			fullscreen_shell_unstable_v1_protocol_c,
			ivi_application_client_protocol_h,
			ivi_application_protocol_c,
		],
		'dep_objs': [ dep_wayland_client, dep_libshared ]
	},
	{
		'name': 'touch',
		'sources': [
			'simple-touch.c',
			xdg_shell_client_protocol_h,
			xdg_shell_protocol_c,
		],
		'dep_objs': [ dep_wayland_client, dep_libshared ]
	},
]

foreach t : simple_clients
	if simple_build_all or simple_clients_enabled.contains(t.get('name'))
		t_name = 'weston-simple-' + t.get('name')
		t_deps = t.get('dep_objs', [])
		foreach depname : t.get('deps', [])
			dep = dependency(depname, required: false)
			if not dep.found()
				error('@0@ requires @1@ which was not found. If you rather not build this, drop "@2@" from simple-clients option.'.format(t_name, depname, t.get('name')))
			endif
			t_deps += dep
		endforeach

		foreach optname : t.get('options', [])
			if not get_option(optname)
				error('@0@ requires option @1@ which is not enabled. If you rather not build this, drop "@2@" from simple-clients option.'.format(t_name, optname, t.get('name')))
			endif
		endforeach

		executable(
			t_name, t.get('sources'),
			include_directories: common_inc,
			dependencies: t_deps,
			install: true
		)
	endif
endforeach

if simple_build_all or simple_clients_enabled.contains('im')
	executable(
		'weston-simple-im', [
			'simple-im.c',
			input_method_unstable_v1_client_protocol_h,
			input_method_unstable_v1_protocol_c,
		],
		include_directories: common_inc,
		dependencies: [
			dep_libshared,
			dep_wayland_client,
			dep_xkbcommon,
			dependency('wayland-cursor'),
			dependency('cairo')
		],
		install: true,
		install_dir: dir_libexec
	)
endif

tools_enabled = get_option('tools')
tools_list = [
	{
		'name': 'calibrator',
		'sources': [ 'calibrator.c' ],
		'deps': [ dep_toytoolkit, dep_matrix_c ],
	},
	{
		'name': 'debug',
		'sources': [
			'weston-debug.c',
			weston_debug_client_protocol_h,
			weston_debug_protocol_c,
		],
		'deps': [ dep_wayland_client ]
	},
	{
		'name': 'terminal',
		'sources': [ 'terminal.c' ],
		'deps': [ dep_toytoolkit ],
	},
	{
		'name': 'touch-calibrator',
		'sources': [
			'touch-calibrator.c',
			weston_touch_calibration_client_protocol_h,
			weston_touch_calibration_protocol_c,
		],
		'deps': [ dep_toytoolkit, dep_matrix_c ],
	},
]

foreach t : tools_list
	if tools_enabled.contains(t.get('name'))
		executable(
			'weston-@0@'.format(t.get('name')),
			t.get('sources'),
			include_directories: common_inc,
			dependencies: t.get('deps', []),
			install: true
		)
	endif
endforeach

demo_clients = [
	{ 'basename': 'clickdot' },
	{
		'basename': 'cliptest',
		'dep_objs': [ dep_vertex_clipping, dep_matrix_c ]
	},
	{ 'basename': 'confine' },
	{
		'basename': 'content_protection',
		'add_sources': [
			weston_content_protection_client_protocol_h,
			weston_content_protection_protocol_c,
		]
	},

	{ 'basename': 'dnd' },
	{
		'basename': 'editor',
		'add_sources': [
			text_input_unstable_v1_client_protocol_h,
			text_input_unstable_v1_protocol_c,
		],
		'deps': [ 'pangocairo', 'gobject-2.0' ]
	},
	{ 'basename': 'eventdemo' },
	{ 'basename': 'flower' },
	{
		'basename': 'fullscreen',
		'add_sources': [
			fullscreen_shell_unstable_v1_client_protocol_h,
			fullscreen_shell_unstable_v1_protocol_c,
		]
	},
	{ 'basename': 'image' },
	{
		'basename': 'multi-resource',
		'add_sources': [
			xdg_shell_client_protocol_h,
			xdg_shell_protocol_c,
		]
	},
	{
		'basename': 'presentation-shm',
		'add_sources': [
			presentation_time_client_protocol_h,
			presentation_time_protocol_c,
			xdg_shell_client_protocol_h,
			xdg_shell_protocol_c,
		]
	},
	{ 'basename': 'resizor' },
	{
		'basename': 'scaler',
		'add_sources': [
			viewporter_client_protocol_h,
			viewporter_protocol_c,
		]
	},
	{ 'basename': 'smoke' },
	{ 'basename': 'stacking' },
	{
		'basename': 'subsurfaces',
		'deps': [ 'egl', 'glesv2', 'wayland-egl' ]
	},
	{
		'basename': 'tablet',
		'add_sources': [
			tablet_unstable_v2_client_protocol_h,
			tablet_unstable_v2_protocol_c,
		],
	},
	{ 'basename': 'transformed' },
]

if get_option('demo-clients')
	foreach t : demo_clients
		t_name = 'weston-' + t.get('basename')
		t_srcs = [ t.get('basename') + '.c' ] + t.get('add_sources', [])
		t_deps = [ dep_toytoolkit, t.get('dep_objs', []) ]
		foreach depname : t.get('deps', [])
			dep = dependency(depname, required: false)
			if not dep.found()
				error('@0@ requires \'@1@\' which was not found. If you rather not build this, set \'-Ddemo-clients=false\'.'.format(t_name, depname))
			endif
			t_deps += dep
		endforeach

		executable(
			t_name, t_srcs,
			include_directories: common_inc,
			dependencies: t_deps,
			install: true
		)
	endforeach
endif

if get_option('shell-desktop')
	exe_keyboard = executable(
		'weston-keyboard',
		'keyboard.c',
		text_input_unstable_v1_client_protocol_h,
		text_input_unstable_v1_protocol_c,
		input_method_unstable_v1_client_protocol_h,
		input_method_unstable_v1_protocol_c,
		include_directories: common_inc,
		dependencies: dep_toytoolkit,
		install_dir: get_option('libexecdir'),
		install: true
	)
	env_modmap += 'weston-keyboard=@0@;'.format(exe_keyboard.full_path())

	exe_shell_desktop = executable(
		'weston-desktop-shell',
		'desktop-shell.c',
		weston_desktop_shell_client_protocol_h,
		weston_desktop_shell_protocol_c,
		tablet_unstable_v2_client_protocol_h,
		tablet_unstable_v2_protocol_c,
		include_directories: common_inc,
		dependencies: dep_toytoolkit,
		install_dir: get_option('libexecdir'),
		install: true
	)
	env_modmap += 'weston-desktop-shell=@0@;'.format(exe_shell_desktop.full_path())
endif

if get_option('shell-desktop') or get_option('shell-fullscreen') or get_option('shell-kiosk') or get_option('shell-ivi')
	exe_shooter = executable(
		'weston-screenshooter',
		'screenshot.c',
		weston_output_capture_client_protocol_h,
		weston_output_capture_protocol_c,
		include_directories: common_inc,
		dependencies: [
			dep_toytoolkit,
			dep_libweston_private, # for pixel-formats.h
			dep_pixman,
		],
		install_dir: get_option('bindir'),
		install: true
	)
	env_modmap += 'weston-screenshooter=@0@;'.format(exe_shooter.full_path())
endif

if get_option('shell-ivi')
	exe_shell_ivi_ui = executable(
		'weston-ivi-shell-user-interface',
		'ivi-shell-user-interface.c',
		ivi_hmi_controller_client_protocol_h,
		ivi_hmi_controller_protocol_c,
		ivi_application_client_protocol_h,
		ivi_application_protocol_c,
		include_directories: common_inc,
		dependencies: dep_toytoolkit,
		install: true,
		install_dir: get_option('libexecdir')
	)
	env_modmap += 'weston-ivi-shell-user-interface=@0@;'.format(exe_shell_ivi_ui.full_path())
endif
