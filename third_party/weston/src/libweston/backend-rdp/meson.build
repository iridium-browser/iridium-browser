if not get_option('backend-rdp')
	subdir_done()
endif

config_h.set('BUILD_RDP_COMPOSITOR', '1')

dep_frdp = dependency('freerdp2', version: '>= 2.3.0', required: false)
if not dep_frdp.found()
	error('RDP-backend requires freerdp >= 2.3.0 which was not found. Or, you can use \'-Dbackend-rdp=false\'.')
endif

dep_frdp_server = dependency('freerdp-server2', version: '>= 2.3.0', required: false)
if not dep_frdp_server.found()
	error('RDP-backend requires freerdp-server2 >= 2.3.0 which was not found. Or, you can use \'-Dbackend-rdp=false\'.')
endif

dep_wpr = dependency('winpr2', version: '>= 2.3.0', required: false)
if not dep_wpr.found()
	error('RDP-backend requires winpr >= 2.3.0 which was not found. Or, you can use \'-Dbackend-rdp=false\'.')
endif

deps_rdp = [
	dep_libweston_private,
	dep_frdp,
	dep_frdp_server,
	dep_wpr,
]
srcs_rdp = [
        'rdp.c',
        'rdpclip.c',
	'rdpdisp.c',
        'rdputil.c',
]

plugin_rdp = shared_library(
	'rdp-backend',
	srcs_rdp,
	include_directories: common_inc,
	dependencies: deps_rdp,
	name_prefix: '',
	install: true,
	install_dir: dir_module_libweston
)
env_modmap += 'rdp-backend.so=@0@;'.format(plugin_rdp.full_path())
install_headers(backend_rdp_h, subdir: dir_include_libweston_install)
