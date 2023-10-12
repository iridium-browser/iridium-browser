plugin_test_shell_desktop = shared_library(
	'weston-test-desktop-shell',
	'weston-test-desktop-shell.c',
	include_directories: common_inc,
	dependencies: [ dep_libweston_public, dep_libexec_weston ],
	name_prefix: '',
	install: false
)
env_modmap += 'weston-test-desktop-shell.so=@0@;'.format(plugin_test_shell_desktop.full_path())

lib_test_runner = static_library(
	'test-runner',
	'weston-test-runner.c',
	dependencies: [
		dep_libweston_private_h_deps,
		dep_wayland_client,
                dep_libdl,
	],
	include_directories: common_inc,
	install: false,
)
dep_test_runner = declare_dependency(
	dependencies: dep_wayland_client,
	link_with: lib_test_runner
)

lib_test_client = static_library(
	'test-client',
	[
		'weston-test-client-helper.c',
		'weston-test-fixture-compositor.c',
		weston_test_client_protocol_h,
		weston_test_protocol_c,
		weston_output_capture_client_protocol_h,
		weston_output_capture_protocol_c,
		viewporter_client_protocol_h,
		viewporter_protocol_c,
		'color_util.h',
		'color_util.c',
	],
	include_directories: common_inc,
	dependencies: [
		dep_libshared,
		dep_wayland_client,
		dep_libexec_weston,
		dep_libweston_private,
		dep_libdrm_headers,
		dep_pixman,
		dependency('cairo'),
	],
	install: false,
)
dep_test_client = declare_dependency(
	link_with: lib_test_client,
	sources: [
		viewporter_client_protocol_h,
	],
	dependencies: [
		dep_wayland_client,
		dep_test_runner,
		dep_pixman,
		dep_libdrm_headers,
		dependency('libudev', version: '>= 136'),
	]
)

lib_lcms_util = static_library(
	'lib_lcms_util',
	[ 'lcms_util.c' ],
	include_directories: common_inc,
	dependencies: [
		dep_lcms2, dep_libm
	],
	build_by_default: false,
	install: false,
)
dep_lcms_util = declare_dependency(
	link_with: lib_lcms_util,
	dependencies: [ dep_lcms2 ]
)

exe_plugin_test = shared_library(
	'test-plugin',
	'weston-test.c',
	weston_test_server_protocol_h,
	weston_test_protocol_c,
	include_directories: common_inc,
	dependencies: [
		dep_libexec_weston,
		dep_libweston_private,
		dep_threads
	],
	name_prefix: '',
	install: false,
)

deps_zuc = [ dep_libshared ]
config_h.set10('ENABLE_JUNIT_XML', get_option('test-junit-xml'))
if get_option('test-junit-xml')
	d = dependency('libxml-2.0', version: '>= 2.6', required: false)
	if not d.found()
		error('JUnit XML support requires libxml-2.0 >= 2.6 which was not found. Or, you can use \'-Dtest-junit-xml=false\'.')
	endif
	deps_zuc += d
endif

lib_zuc = static_library(
	'zunitc',
	'../tools/zunitc/inc/zunitc/zunitc.h',
	'../tools/zunitc/inc/zunitc/zunitc_impl.h',
	'../tools/zunitc/src/zuc_base_logger.c',
	'../tools/zunitc/src/zuc_base_logger.h',
	'../tools/zunitc/src/zuc_context.h',
	'../tools/zunitc/src/zuc_event.h',
	'../tools/zunitc/src/zuc_event_listener.h',
	'../tools/zunitc/src/zuc_junit_reporter.c',
	'../tools/zunitc/src/zuc_junit_reporter.h',
	'../tools/zunitc/src/zuc_types.h',
	'../tools/zunitc/src/zunitc_impl.c',
	include_directories: [ common_inc, include_directories('../tools/zunitc/inc') ],
	dependencies: deps_zuc,
)
dep_zuc = declare_dependency(
	link_with: lib_zuc,
	dependencies: dep_libshared,
	include_directories: include_directories('../tools/zunitc/inc')
)

lib_zucmain = static_library(
	'zunitcmain',
	'../tools/zunitc/src/main.c',
	include_directories: [ common_inc, include_directories('../tools/zunitc/inc') ],
	dependencies: [ dep_libshared, dep_zuc ],
)
dep_zucmain = declare_dependency(
	link_with: lib_zucmain,
	dependencies: dep_zuc
)

tests = [
	{
		'name': 'alpha-blending',
		'dep_objs': dep_libm,
	},
	{	'name': 'bad-buffer', },
	{	'name': 'buffer-transforms', },
	{
		'name': 'color-metadata-errors',
		'dep_objs': dep_libexec_weston,
	},
	{	'name': 'color-manager', },
        {       'name': 'custom-env', },
	{	'name': 'devices', },
	{
		'name': 'drm-formats',
		'dep_objs': dep_libdrm_headers,
	},
	{	'name': 'drm-smoke', 'run_exclusive': true },
	{	'name': 'drm-writeback-screenshot', 'run_exclusive': true },
	{	'name': 'event', },
	{	'name': 'internal-screenshot', },
	{
		'name': 'keyboard',
		'sources': [
			'keyboard-test.c',
			'input-timestamps-helper.c',
			input_timestamps_unstable_v1_client_protocol_h,
			input_timestamps_unstable_v1_protocol_c,
		],
	},
	{
		'name': 'linux-explicit-synchronization',
		'sources': [
			'linux-explicit-synchronization-test.c',
			linux_explicit_synchronization_unstable_v1_client_protocol_h,
			linux_explicit_synchronization_unstable_v1_protocol_c,
		],
	},
	{
		'name': 'matrix',
		'dep_objs': [ dep_libm ]
	},
	{
		'name': 'matrix-transform',
		'dep_objs': dep_libm,
	},
	{
		'name': 'output-capture-protocol',
		'sources': [
			'output-capture-protocol-test.c',
			weston_output_capture_protocol_c,
			weston_output_capture_client_protocol_h,
		],
		'dep_objs': [ dep_libdrm_headers ],
	},
	{	'name': 'output-damage', },
	{	'name': 'output-decorations', },
	{	'name': 'output-transforms', },
	{	'name': 'plugin-registry', },
	{
		'name': 'pointer',
		'sources': [
			'pointer-test.c',
			'input-timestamps-helper.c',
			input_timestamps_unstable_v1_client_protocol_h,
			input_timestamps_unstable_v1_protocol_c,
		],
	},
	{	'name': 'pointer-shot', },
	{
		'name': 'presentation',
		'sources': [
			'presentation-test.c',
			presentation_time_client_protocol_h,
			presentation_time_protocol_c,
		],
	},
	{
		'name': 'roles',
		'sources': [
			'roles-test.c',
			xdg_shell_client_protocol_h,
			xdg_shell_protocol_c,
		],
	},
	{
		'name': 'single-pixel-buffer',
		'sources': [
			'single-pixel-buffer-test.c',
			single_pixel_buffer_v1_client_protocol_h,
			single_pixel_buffer_v1_protocol_c,
		]
	},
	{	'name': 'string', },
	{	'name': 'subsurface', },
	{	'name': 'subsurface-shot', },
	{	'name': 'surface', },
	{	'name': 'surface-global', },
	{
		'name': 'text',
		'sources': [
			'text-test.c',
			text_input_unstable_v1_client_protocol_h,
			text_input_unstable_v1_protocol_c,
		],
	},
	{
		'name': 'touch',
		'sources': [
			'touch-test.c',
			'input-timestamps-helper.c',
			input_timestamps_unstable_v1_client_protocol_h,
			input_timestamps_unstable_v1_protocol_c,
		],
	},
	{	'name': 'viewporter', },
	{	'name': 'viewporter-shot', },
	{
		'name': 'yuv-buffer',
		'dep_objs': [
			dep_libdrm_headers,
			dep_libm,
		],
	},
	{	'name': 'safe-signal', },
	{	'name': 'safe-signal-output-removal',
		'sources': [
		  'safe-signal-output-removal-test.c',
		],
		'dep_objs': [ dep_libweston_public ]
	},
]

if get_option('renderer-gl')
	tests += {
		'name': 'vertex-clip',
		'link_with': plugin_gl,
	}

endif

if get_option('color-management-lcms')
	if not dep_lcms2.found()
		error('color-management-lcms tests require lcms2 which was not found. Or, you can use \'-Dcolor-management-lcms=false\'.')
	endif
	tests += [
		{
			'name': 'color-icc-output',
			'dep_objs': [ dep_libm, dep_lcms_util ]
		},
		{	'name': 'color-metadata-parsing' },
		{
			'name': 'lcms-util',
			'dep_objs': [ dep_lcms_util ]
		},
	]
endif


tests_standalone = [
	['config-parser', [], [ dep_zucmain ]],
	['timespec', [], [ dep_zucmain ]],
	['zuc',
		[
			'../tools/zunitc/test/fixtures_test.c',
			'../tools/zunitc/test/zunitc_test.c'
		],
		[ dep_zucmain ]
	],
]

if get_option('xwayland')
	xcb_dep = dependency('xcb', required: false)
	xcb_cursor_dep = dependency('xcb-cursor', required: false)

	if not xcb_dep.found() or not xcb_cursor_dep.found()
		error('xcb and xcb-cursor required for running xwayland tests')
	endif

	libxwayland_test_client = static_library(
		'test-xwayland-client',
		[ 'xcb-client-helper.c', weston_test_client_protocol_h ],
		include_directories: common_inc,
		dependencies: [
			dep_pixman, dep_xcb_xwayland,
			xcb_dep, xcb_cursor_dep,
			dep_wayland_client,
		],
		install: false,
	)

	dep_libxwayland_test = declare_dependency(
		dependencies: [ xcb_dep, xcb_cursor_dep ],
		link_with: libxwayland_test_client,
	)

	tests += [ {
		'name': 'xwayland',
		'dep_objs': dep_libxwayland_test,
	} ]
endif

# Manual test plugin, not used in the automatic suite
surface_screenshot_test = shared_library(
	'test-surface-screenshot',
	'surface-screenshot-test.c',
	include_directories: common_inc,
	dependencies: [ dep_libweston_private, dep_libshared ],
	name_prefix: '',
	install: false,
)

if get_option('shell-ivi')
	ivi_layout_test_plugin = shared_library(
		'test-ivi-layout',
		[
			'ivi-layout-test-plugin.c',
			weston_test_server_protocol_h,
			weston_test_protocol_c,
		],
		include_directories: common_inc,
		dependencies: [ dep_libweston_private, dep_libexec_weston ],
		name_prefix: '',
		install: false,
	)
	env_modmap += 'test-ivi-layout.so=' + ivi_layout_test_plugin.full_path() + ';'

	tests += [
		{
			'name': 'ivi-layout-client',
			'sources': [
				'ivi-layout-test-client.c',
				ivi_application_client_protocol_h,
				ivi_application_protocol_c,
			],
			'test_deps': [ ivi_layout_test_plugin ],
		},
		{	'name': 'ivi-layout-internal', },
		{
			'name': 'ivi-shell-app',
			'sources': [
				'ivi-shell-app-test.c',
				ivi_application_client_protocol_h,
				ivi_application_protocol_c,
			],
		},
	]
endif

test_config_h = configuration_data()
test_config_h.set_quoted('WESTON_TEST_REFERENCE_PATH', meson.current_source_dir() + '/reference')
test_config_h.set_quoted('WESTON_MODULE_MAP', env_modmap)
test_config_h.set_quoted('WESTON_DATA_DIR', join_paths(meson.current_source_dir(), '..', 'data'))
test_config_h.set_quoted('TESTSUITE_PLUGIN_PATH', exe_plugin_test.full_path())
test_config_h.set10('WESTON_TEST_SKIP_IS_FAILURE', get_option('test-skip-is-failure'))
configure_file(output: 'test-config.h', configuration: test_config_h)

foreach t : tests
	t_name = 'test-' + t.get('name')
	t_sources = t.get('sources', [t.get('name') + '-test.c'])
	t_sources += weston_test_client_protocol_h

	t_deps = [ dep_test_client, dep_libweston_private_h ]
	t_deps += t.get('dep_objs', [])

	run_exclusive = t.get('run_exclusive', false)

	t_exe = executable(
		t_name,
		t_sources,
		c_args: [
			'-DTHIS_TEST_NAME="' + t_name + '"',
		],
		build_by_default: true,
		include_directories: common_inc,
		dependencies: t_deps,
		link_with: t.get('link_with', []),
		install: false,
	)

	test(
		t.get('name'),
		t_exe,
		depends: t.get('test_deps', []),
		timeout: 120,
		protocol: 'tap',
		is_parallel: not run_exclusive
	)
endforeach

# FIXME: the multiple loops is lame. rethink this.
foreach t : tests_standalone
	if t[0] != 'zuc'
		srcs_t = [
			'@0@-test.c'.format(t.get(0)),
			weston_test_client_protocol_h,
		]
	else
		srcs_t = []
	endif

	if t.length() > 1
		srcs_t += t.get(1)
	endif

	if t.length() > 2
		deps_t = t[2]
	else
		deps_t = [ dep_test_client ]
	endif

	exe_t = executable(
		'test-@0@'.format(t.get(0)),
		srcs_t,
		build_by_default: true,
		include_directories: common_inc,
		dependencies: deps_t,
		install: false,
	)

	# matrix-test is a manual test
	if t[0] != 'matrix'
		test(t.get(0), exe_t)
	endif
endforeach

if get_option('backend-drm')
	executable(
		'setbacklight',
		'setbacklight.c',
		dependencies: [
			dep_backlight,
			dep_libdrm,
			dependency('libudev')
		],
		include_directories: common_inc,
		install: false
	)
endif
