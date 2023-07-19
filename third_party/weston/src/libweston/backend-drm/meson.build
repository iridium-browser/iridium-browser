if not get_option('backend-drm')
	subdir_done()
endif

lib_backlight = static_library(
	'backlight',
	'libbacklight.c',
	dependencies: [
		dep_libdrm_headers,
		dependency('libudev')
	],
	include_directories: common_inc,
	install: false
)
dep_backlight = declare_dependency(
	link_with: lib_backlight,
	include_directories: include_directories('.')
)

config_h.set('BUILD_DRM_COMPOSITOR', '1')

srcs_drm = [
	'drm.c',
	'fb.c',
	'modes.c',
	'kms.c',
	'state-helpers.c',
	'state-propose.c',
	linux_dmabuf_unstable_v1_protocol_c,
	linux_dmabuf_unstable_v1_server_protocol_h,
	presentation_time_server_protocol_h,
]

deps_drm = [
	dep_libdl,
	dep_libweston_private,
	dep_session_helper,
	dep_libdrm,
	dep_libinput_backend,
	dependency('libudev', version: '>= 136'),
	dep_backlight
]

if get_option('renderer-gl')
	dep_gbm = dependency('gbm', required: false)
	if not dep_gbm.found()
		error('drm-backend with GL renderer requires gbm which was not found. Or, you can use \'-Drenderer-gl=false\'.')
	endif
	if dep_gbm.version().version_compare('>= 17.1')
		config_h.set('HAVE_GBM_MODIFIERS', '1')
	endif
	if dep_gbm.version().version_compare('>= 17.2')
		config_h.set('HAVE_GBM_FD_IMPORT', '1')
	endif
	deps_drm += dep_gbm
	srcs_drm += 'drm-gbm.c'
	config_h.set('BUILD_DRM_GBM', '1')
endif

if get_option('backend-drm-screencast-vaapi')
	foreach name : [ 'libva', 'libva-drm' ]
		d = dependency(name, version: '>= 0.34.0', required: false)
		if not d.found()
			error('VA-API recorder requires @0@ >= 0.34.0 which was not found. Or, you can use \'-Dbackend-drm-screencast-vaapi=false\'.'.format(name))
		endif
		deps_drm += d
	endforeach

	srcs_drm += 'vaapi-recorder.c'
	deps_drm += dependency('threads')
	config_h.set('BUILD_VAAPI_RECORDER', '1')
endif

if get_option('remoting') or get_option('pipewire')
	if not get_option('renderer-gl')
		error('DRM virtual requires renderer-gl.')
	endif
	srcs_drm += 'drm-virtual.c'
	config_h.set('BUILD_DRM_VIRTUAL', '1')
endif

plugin_drm = shared_library(
	'drm-backend',
	srcs_drm,
	include_directories: common_inc,
	dependencies: deps_drm,
	name_prefix: '',
	install: true,
	install_dir: dir_module_libweston
)
env_modmap += 'drm-backend.so=@0@;'.format(plugin_drm.full_path())

install_headers(backend_drm_h, subdir: dir_include_libweston_install)
