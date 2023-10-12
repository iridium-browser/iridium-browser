if get_option('pipewire')
	user_hint = 'If you rather not build this, set \'-Dpipewire=false\'.'

	if not get_option('backend-drm')
		error('Attempting to build the pipewire plugin without the required DRM backend. ' + user_hint)
	endif

	deps_pipewire = [ dep_libweston_private ]

	dep_libpipewire = dependency('libpipewire-0.3', required: false)
	if not dep_libpipewire.found()
		error('Pipewire plugin requires libpipewire which was not found. ' + user_hint)
	endif
	deps_pipewire += dep_libpipewire

	dep_libspa = dependency('libspa-0.2', required: false)
	if not dep_libspa.found()
		error('Pipewire plugin requires libspa which was not found. ' + user_hint)
	endif
	deps_pipewire += dep_libspa

	plugin_pipewire = shared_library(
		'pipewire-plugin',
		'pipewire-plugin.c',
		include_directories: common_inc,
		dependencies: deps_pipewire,
		name_prefix: '',
		install: true,
		install_dir: dir_module_libweston
	)
	env_modmap += 'pipewire-plugin.so=@0@;'.format(plugin_pipewire.full_path())
endif
