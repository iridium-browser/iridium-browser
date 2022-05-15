if not get_option('renderer-gl')
	subdir_done()
endif

config_h.set('ENABLE_EGL', '1')

srcs_renderer_gl = [
	'egl-glue.c',
	'gl-renderer.c',
	linux_dmabuf_unstable_v1_protocol_c,
	linux_dmabuf_unstable_v1_server_protocol_h,
]

deps_renderer_gl = [
	dep_libm,
	dep_pixman,
	dep_libweston_private,
	dep_libdrm_headers,
	dep_vertex_clipping
]

foreach name : [ 'egl', 'glesv2' ]
	d = dependency(name, required: false)
	if not d.found()
		error('gl-renderer requires @0@ which was not found. Or, you can use \'-Drenderer-gl=false\'.'.format(name))
	endif
	deps_renderer_gl += d
endforeach

plugin_gl = shared_library(
	'gl-renderer',
	srcs_renderer_gl,
	include_directories: common_inc,
	dependencies: deps_renderer_gl,
	name_prefix: '',
	install: true,
	install_dir: dir_module_libweston
)
env_modmap += 'gl-renderer.so=@0@;'.format(plugin_gl.full_path())
