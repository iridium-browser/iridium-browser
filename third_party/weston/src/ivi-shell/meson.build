if get_option('shell-ivi')
	srcs_shell_ivi = [
		'ivi-shell.c',
		'ivi-layout.c',
		'ivi-layout-transition.c',
		ivi_application_server_protocol_h,
		ivi_application_protocol_c,
		input_method_unstable_v1_server_protocol_h,
		input_method_unstable_v1_protocol_c,
	]
	plugin_shell_ivi = shared_library(
		'ivi-shell',
		srcs_shell_ivi,
		include_directories: common_inc,
		dependencies: [
			dep_libm,
			dep_libexec_weston,
			dep_lib_desktop,
			dep_libweston_public
		],
		name_prefix: '',
		install: true,
		install_dir: dir_module_weston,
		install_rpath: '$ORIGIN'
	)
	env_modmap += 'ivi-shell.so=@0@;'.format(plugin_shell_ivi.full_path())

	install_headers('ivi-layout-export.h', subdir: 'weston')

	srcs_ivi_hmi = [
		'hmi-controller.c',
		ivi_hmi_controller_server_protocol_h,
		ivi_hmi_controller_protocol_c,
	]
	plugin_ivi_hmi = shared_library(
		'hmi-controller',
		srcs_ivi_hmi,
		include_directories: common_inc,
		dependencies: [
			dep_libexec_weston,
			dep_libweston_public,
			dep_libshared
		],
		name_prefix: '',
		install: true,
		install_dir: dir_module_weston,
		install_rpath: '$ORIGIN'
	)
	env_modmap += 'hmi-controller.so=@0@;'.format(plugin_ivi_hmi.full_path())

	ivi_test_config = configuration_data()
	ivi_test_config.set('bindir', dir_bin)
	ivi_test_config.set('libexecdir', dir_libexec)
	ivi_test_config.set('plugin_prefix', meson.current_build_dir())
	ivi_test_config.set('westondatadir', join_paths(dir_data, 'weston'))
	ivi_test_ini = configure_file(
		input: '../ivi-shell/weston.ini.in',
		output: 'weston-ivi-test.ini',
		configuration: ivi_test_config
	)
endif
