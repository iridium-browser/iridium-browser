if not get_option('backend-fbdev')
	subdir_done()
endif

config_h.set('BUILD_FBDEV_COMPOSITOR', '1')

srcs_fbdev = [
	'fbdev.c',
	presentation_time_server_protocol_h,
]

deps_fbdev = [
	dep_libweston_private,
	dep_session_helper,
	dep_libinput_backend,
	dependency('libudev', version: '>= 136'),
]

plugin_fbdev = shared_library(
	'fbdev-backend',
	srcs_fbdev,
	include_directories: common_inc,
	dependencies: deps_fbdev,
	name_prefix: '',
	install: true,
	install_dir: dir_module_libweston
)
env_modmap += 'fbdev-backend.so=@0@;'.format(plugin_fbdev.full_path())

install_headers(backend_fbdev_h, subdir: dir_include_libweston_install)
