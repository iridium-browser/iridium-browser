if get_option('shell-desktop')
	config_h.set_quoted('WESTON_SHELL_CLIENT', get_option('desktop-shell-client-default'))

	srcs_shell_desktop = [
		'shell.c',
		'input-panel.c',
		weston_desktop_shell_server_protocol_h,
		weston_desktop_shell_protocol_c,
		input_method_unstable_v1_server_protocol_h,
		input_method_unstable_v1_protocol_c,
	]
	deps_shell_desktop = [
		dep_libm,
		dep_libexec_weston,
		dep_libshared,
		dep_libweston_public,
	]
	plugin_shell_desktop = shared_library(
		'desktop-shell',
		srcs_shell_desktop,
		include_directories: common_inc,
		dependencies: deps_shell_desktop,
		name_prefix: '',
		install: true,
		install_dir: dir_module_weston,
		install_rpath: '$ORIGIN'
	)
	env_modmap += 'desktop-shell.so=@0@;'.format(plugin_shell_desktop.full_path())
endif
