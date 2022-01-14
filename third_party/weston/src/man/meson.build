man_conf = configuration_data()
man_conf.set('weston_native_backend', opt_backend_native)
man_conf.set('weston_modules_dir', dir_module_weston)
man_conf.set('libweston_modules_dir', dir_module_libweston)
man_conf.set('weston_shell_client', get_option('desktop-shell-client-default'))
man_conf.set('weston_libexecdir', dir_libexec)
man_conf.set('weston_bindir', dir_bin)
man_conf.set('xserver_path', get_option('xwayland-path'))
man_conf.set('version', version_weston)

configure_file(
	input: 'weston.man',
	output: 'weston.1',
	install_dir: join_paths(dir_man, 'man1'),
	configuration: man_conf
)

configure_file(
	input: 'weston-bindings.man',
	output: 'weston-bindings.7',
	install_dir: join_paths(dir_man, 'man7'),
	configuration: man_conf
)

configure_file(
	input: 'weston-debug.man',
	output: 'weston-debug.1',
	install_dir: join_paths(dir_man, 'man1'),
	configuration: man_conf
)

configure_file(
	input: 'weston.ini.man',
	output: 'weston.ini.5',
	install_dir: join_paths(dir_man, 'man5'),
	configuration: man_conf
)

if get_option('backend-drm')
	configure_file(
		input: 'weston-drm.man',
		output: 'weston-drm.7',
		install_dir: join_paths(dir_man, 'man7'),
		configuration: man_conf
	)
endif

if get_option('backend-rdp')
	configure_file(
		input: 'weston-rdp.man',
		output: 'weston-rdp.7',
		install_dir: join_paths(dir_man, 'man7'),
		configuration: man_conf
	)
endif
