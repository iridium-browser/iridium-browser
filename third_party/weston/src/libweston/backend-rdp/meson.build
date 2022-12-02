if not get_option('backend-rdp')
	subdir_done()
endif

config_h.set('BUILD_RDP_COMPOSITOR', '1')

dep_frdp = dependency('freerdp2', version: '>= 2.0.0', required: false)
if not dep_frdp.found()
	error('RDP-backend requires freerdp2 which was not found. Or, you can use \'-Dbackend-rdp=false\'.')
endif

dep_wpr = dependency('winpr2', version: '>= 2.0.0', required: false)
if not dep_wpr.found()
	error('RDP-backend requires winpr2 which was not found. Or, you can use \'-Dbackend-rdp=false\'.')
endif

if cc.has_header('freerdp/version.h', dependencies: dep_frdp)
	config_h.set('HAVE_FREERDP_VERSION_H', '1')
endif

if cc.has_member(
	'SURFACE_BITS_COMMAND', 'bmp',
	dependencies : dep_frdp,
	prefix : '#include <freerdp/update.h>'
)
	config_h.set('HAVE_SURFACE_BITS_BMP', '1')
endif

if cc.has_type(
	'enum SURFCMD_CMDTYPE',
	dependencies : dep_frdp,
	prefix : '#include <freerdp/update.h>'
)
	config_h.set('HAVE_SURFCMD_CMDTYPE', '1')
endif

if cc.has_function(
	'nsc_context_set_parameters',
	dependencies : dep_frdp,
	prefix: '#include <freerdp/codec/nsc.h>'
)
	config_h.set('HAVE_NSC_CONTEXT_SET_PARAMETERS', '1')
endif

deps_rdp = [
	dep_libweston_private,
	dep_frdp,
	dep_wpr,
]
plugin_rdp = shared_library(
	'rdp-backend',
	'rdp.c',
	include_directories: common_inc,
	dependencies: deps_rdp,
	name_prefix: '',
	install: true,
	install_dir: dir_module_libweston
)
env_modmap += 'rdp-backend.so=@0@;'.format(plugin_rdp.full_path())
install_headers(backend_rdp_h, subdir: dir_include_libweston_install)
