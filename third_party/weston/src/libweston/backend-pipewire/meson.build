if not get_option('backend-pipewire')
	subdir_done()
endif
user_hint = 'If you rather not build this, set \'-Dbackend-pipewire=false\'.'

config_h.set('BUILD_PIPEWIRE_COMPOSITOR', '1')

dep_libpipewire = dependency('libpipewire-0.3', required: false)
if not dep_libpipewire.found()
	error('PipeWire backend requires libpipewire 0.3 which was not found. ' + user_hint)
endif

dep_libspa = dependency('libspa-0.2', required: false)
if not dep_libspa.found()
	error('Pipewire plugin requires libspa 0.2 which was not found. ' + user_hint)
endif

deps_pipewire = [
	dep_libweston_private,
	dep_libpipewire,
	dep_libspa,
	dep_libdrm_headers,
]

plugin_pipewire = shared_library(
	'pipewire-backend',
	[ 'pipewire.c' ],
	include_directories: common_inc,
	dependencies: deps_pipewire,
	name_prefix: '',
	install: true,
	install_dir: dir_module_libweston
)
env_modmap += 'pipewire-backend.so=@0@;'.format(plugin_pipewire.full_path())
install_headers(backend_pipewire_h, subdir: dir_include_libweston_install)
