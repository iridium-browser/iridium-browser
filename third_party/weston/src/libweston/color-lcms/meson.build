if not get_option('color-management-lcms')
	subdir_done()
endif

if not dep_lcms2.found()
	error('color-lcms plugin requires lcms2 which was not found. Or, you can use \'-Dcolor-management-lcms=false\'.')
endif

srcs_color_lcms = [
	'color-lcms.c',
	'color-profile.c',
	'color-transform.c',
]

deps_color_lcms = [
	dep_libm,
	dep_libweston_private,
	dep_lcms2,
]

plugin_color_lcms = shared_library(
	'color-lcms',
	srcs_color_lcms,
	include_directories: common_inc,
	dependencies: deps_color_lcms,
	name_prefix: '',
	install: true,
	install_dir: dir_module_libweston
)
env_modmap += 'color-lcms.so=@0@;'.format(plugin_color_lcms.full_path())
