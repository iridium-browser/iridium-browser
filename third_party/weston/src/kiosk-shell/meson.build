if get_option('shell-kiosk')
	srcs_shell_kiosk = [
		'kiosk-shell.c',
		'kiosk-shell-grab.c',
		input_method_unstable_v1_server_protocol_h,
		input_method_unstable_v1_protocol_c,
	]
	deps_shell_kiosk = [
		dep_libm,
		dep_libexec_weston,
		dep_libshared,
		dep_libweston_public,
	]
	plugin_shell_kiosk = shared_library(
		'kiosk-shell',
		srcs_shell_kiosk,
		include_directories: common_inc,
		dependencies: deps_shell_kiosk,
		name_prefix: '',
		install: true,
		install_dir: dir_module_weston,
		install_rpath: '$ORIGIN'
	)
	env_modmap += 'kiosk-shell.so=@0@;'.format(plugin_shell_kiosk.full_path())
endif
