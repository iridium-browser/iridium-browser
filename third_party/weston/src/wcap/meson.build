if not get_option('wcap-decode')
	subdir_done()
endif

srcs_wcap = [
	'main.c',
	'wcap-decode.c',
]

wcap_dep_cairo = dependency('cairo', required: false)
if not wcap_dep_cairo.found()
	error('wcap requires cairo which was not found. Or, you can use \'-Dwcap-decode=false\'.')
endif

executable(
	'wcap-decode',
	srcs_wcap,
	include_directories: common_inc,
	dependencies: [ dep_libm, wcap_dep_cairo ],
	install: true
)
