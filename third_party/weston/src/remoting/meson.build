if get_option('remoting')
	user_hint = 'If you rather not build this, set \'-Dremoting=false\'.'

	if not get_option('backend-drm') or not get_option('renderer-gl')
		error('Attempting to build the remoting plugin without the required DRM backend and GL renderer. ' + user_hint)
	endif

	depnames = [
		'gstreamer-1.0', 'gstreamer-allocators-1.0',
		'gstreamer-app-1.0', 'gstreamer-video-1.0',
		'gobject-2.0', 'glib-2.0'
	]
	deps_remoting = [ dep_libweston_private, dep_libdrm_headers ]
	foreach depname : depnames
		dep = dependency(depname, required: false)
		if not dep.found()
			error('Remoting plugin requires @0@ which was not found. '.format(depname) + user_hint)
		endif
		deps_remoting += dep
	endforeach

	plugin_remoting = shared_library(
		'remoting-plugin',
		'remoting-plugin.c',
		include_directories: common_inc,
		dependencies: deps_remoting,
		name_prefix: '',
		install: true,
		install_dir: dir_module_libweston
	)
	env_modmap += 'remoting-plugin.so=@0@;'.format(plugin_remoting.full_path())
	install_headers('remoting-plugin.h', subdir: dir_include_libweston_install)
endif
