if get_option('pipewire')
	user_hint = 'If you rather not build this, set "pipewire=false".'

	if not get_option('backend-drm')
		error('Attempting to build the pipewire plugin without the required DRM backend. ' + user_hint)
	endif

	depnames = [
		'libpipewire-0.2', 'libspa-0.1'
	]
	deps_pipewire = [ dep_libweston_private ]
	foreach depname : depnames
		dep = dependency(depname, required: false)
		if not dep.found()
			error('Pipewire plugin requires @0@ which was not found. '.format(depname) + user_hint)
		endif
		deps_pipewire += dep
	endforeach

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
	install_headers('pipewire-plugin.h', subdir: dir_include_libweston_install)
endif
