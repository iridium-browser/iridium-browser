/*
 * Copyright 2012 Intel Corporation
 * Copyright 2015,2019,2021 Collabora, Ltd.
 * Copyright 2016 NVIDIA Corporation
 * Copyright 2019 Harish Krupo
 * Copyright 2019 Intel Corporation
 * Copyright 2021 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libweston/libweston.h>
#include <libweston/weston-log.h>
#include <GLES2/gl2.h>

#include <string.h>

#include "gl-renderer.h"
#include "gl-renderer-internal.h"
#include "shared/helpers.h"
#include "shared/timespec-util.h"

/* static const char vertex_shader[]; vertex.glsl */
#include "vertex-shader.h"

/* static const char fragment_shader[]; fragment.glsl */
#include "fragment-shader.h"

struct gl_shader {
	struct gl_shader_requirements key;
	GLuint program;
	GLuint vertex_shader, fragment_shader;
	GLint proj_uniform;
	GLint tex_uniforms[3];
	GLint view_alpha_uniform;
	GLint color_uniform;
	GLint color_pre_curve_lut_2d_uniform;
	GLint color_pre_curve_lut_scale_offset_uniform;
	union {
		struct {
			GLint tex_uniform;
			GLint scale_offset_uniform;
		} lut3d;
		GLint matrix_uniform;
	} color_mapping;
	GLint color_post_curve_lut_2d_uniform;
	GLint color_post_curve_lut_scale_offset_uniform;
	struct wl_list link; /* gl_renderer::shader_list */
	struct timespec last_used;
};

static const char *
gl_shader_texture_variant_to_string(enum gl_shader_texture_variant v)
{
	switch (v) {
#define CASERET(x) case x: return #x;
	CASERET(SHADER_VARIANT_NONE)
	CASERET(SHADER_VARIANT_RGBX)
	CASERET(SHADER_VARIANT_RGBA)
	CASERET(SHADER_VARIANT_Y_U_V)
	CASERET(SHADER_VARIANT_Y_UV)
	CASERET(SHADER_VARIANT_Y_XUXV)
	CASERET(SHADER_VARIANT_XYUV)
	CASERET(SHADER_VARIANT_SOLID)
	CASERET(SHADER_VARIANT_EXTERNAL)
#undef CASERET
	}

	return "!?!?"; /* never reached */
}

static const char *
gl_shader_color_curve_to_string(enum gl_shader_color_curve kind)
{
	switch (kind) {
#define CASERET(x) case x: return #x;
	CASERET(SHADER_COLOR_CURVE_IDENTITY)
	CASERET(SHADER_COLOR_CURVE_LUT_3x1D)
#undef CASERET
	}

	return "!?!?"; /* never reached */
}

static const char *
gl_shader_color_mapping_to_string(enum gl_shader_color_mapping kind)
{
	switch (kind) {
#define CASERET(x) case x: return #x;
	CASERET(SHADER_COLOR_MAPPING_IDENTITY)
	CASERET(SHADER_COLOR_MAPPING_3DLUT)
	CASERET(SHADER_COLOR_MAPPING_MATRIX)
#undef CASERET
	}

	return "!?!?"; /* never reached */
}

static void
dump_program_with_line_numbers(int count, const char **sources)
{
	FILE *fp;
	char *dumpstr;
	size_t dumpstrsz;
	const char *cur;
	const char *delim;
	int line = 1;
	int i;
	bool new_line = true;

	fp = open_memstream(&dumpstr, &dumpstrsz);
	if (!fp)
		return;

	for (i = 0; i < count; i++) {
		cur = sources[i];
		while ((delim = strchr(cur, '\n'))) {
			if (new_line)
				fprintf(fp, "%6d: ", line++);
			fprintf(fp, "%.*s\n", (int)(delim - cur), cur);
			new_line = true;
			cur = delim + 1;
		}
		if (new_line)
			fprintf(fp, "%6d: ", line++);
		new_line = false;
		fprintf(fp, "%s", cur);
	}

	if (fclose(fp) == 0)
		weston_log_continue("%s\n", dumpstr);

	free(dumpstr);
}

static GLuint
compile_shader(GLenum type, int count, const char **sources)
{
	GLuint s;
	char msg[512];
	GLint status;

	s = glCreateShader(type);
	glShaderSource(s, count, sources, NULL);
	glCompileShader(s);
	glGetShaderiv(s, GL_COMPILE_STATUS, &status);
	if (!status) {
		glGetShaderInfoLog(s, sizeof msg, NULL, msg);
		weston_log("shader info: %s\n", msg);
		weston_log("shader source:\n");
		dump_program_with_line_numbers(count, sources);
		return GL_NONE;
	}

	return s;
}

static char *
create_shader_description_string(const struct gl_shader_requirements *req)
{
	int size;
	char *str;

	size = asprintf(&str, "%s %s %s %s %cinput_is_premult %cgreen",
			gl_shader_texture_variant_to_string(req->variant),
			gl_shader_color_curve_to_string(req->color_pre_curve),
			gl_shader_color_mapping_to_string(req->color_mapping),
			gl_shader_color_curve_to_string(req->color_post_curve),
			req->input_is_premult ? '+' : '-',
			req->green_tint ? '+' : '-');
	if (size < 0)
		return NULL;
	return str;
}

static char *
create_shader_config_string(const struct gl_shader_requirements *req)
{
	int size;
	char *str;

	size = asprintf(&str,
			"#define DEF_GREEN_TINT %s\n"
			"#define DEF_INPUT_IS_PREMULT %s\n"
			"#define DEF_COLOR_PRE_CURVE %s\n"
			"#define DEF_COLOR_MAPPING %s\n"
			"#define DEF_COLOR_POST_CURVE %s\n"
			"#define DEF_VARIANT %s\n",
			req->green_tint ? "true" : "false",
			req->input_is_premult ? "true" : "false",
			gl_shader_color_curve_to_string(req->color_pre_curve),
			gl_shader_color_mapping_to_string(req->color_mapping),
			gl_shader_color_curve_to_string(req->color_post_curve),
			gl_shader_texture_variant_to_string(req->variant));
	if (size < 0)
		return NULL;
	return str;
}

static struct gl_shader *
gl_shader_create(struct gl_renderer *gr,
		 const struct gl_shader_requirements *requirements)
{
	bool verbose = weston_log_scope_is_enabled(gr->shader_scope);
	struct gl_shader *shader = NULL;
	char msg[512];
	GLint status;
	const char *sources[3];
	char *conf = NULL;

	shader = zalloc(sizeof *shader);
	if (!shader) {
		weston_log("could not create shader\n");
		goto error_vertex;
	}

	wl_list_init(&shader->link);
	shader->key = *requirements;

	if (verbose) {
		char *desc;

		desc = create_shader_description_string(requirements);
		weston_log_scope_printf(gr->shader_scope,
					"Compiling shader program for: %s\n",
					desc);
		free(desc);
	}

	sources[0] = vertex_shader;
	shader->vertex_shader = compile_shader(GL_VERTEX_SHADER, 1, sources);
	if (shader->vertex_shader == GL_NONE)
		goto error_vertex;

	conf = create_shader_config_string(&shader->key);
	if (!conf)
		goto error_fragment;

	sources[0] = "#version 100\n";
	sources[1] = conf;
	sources[2] = fragment_shader;
	shader->fragment_shader = compile_shader(GL_FRAGMENT_SHADER,
						 3, sources);
	if (shader->fragment_shader == GL_NONE)
		goto error_fragment;

	shader->program = glCreateProgram();
	glAttachShader(shader->program, shader->vertex_shader);
	glAttachShader(shader->program, shader->fragment_shader);
	glBindAttribLocation(shader->program, 0, "position");
	glBindAttribLocation(shader->program, 1, "texcoord");

	glLinkProgram(shader->program);
	glGetProgramiv(shader->program, GL_LINK_STATUS, &status);
	if (!status) {
		glGetProgramInfoLog(shader->program, sizeof msg, NULL, msg);
		weston_log("link info: %s\n", msg);
		goto error_link;
	}

	glDeleteShader(shader->vertex_shader);
	glDeleteShader(shader->fragment_shader);

	shader->proj_uniform = glGetUniformLocation(shader->program, "proj");
	shader->tex_uniforms[0] = glGetUniformLocation(shader->program, "tex");
	shader->tex_uniforms[1] = glGetUniformLocation(shader->program, "tex1");
	shader->tex_uniforms[2] = glGetUniformLocation(shader->program, "tex2");
	shader->view_alpha_uniform = glGetUniformLocation(shader->program, "view_alpha");
	if (requirements->variant == SHADER_VARIANT_SOLID) {
		shader->color_uniform = glGetUniformLocation(shader->program,
							     "unicolor");
		assert(shader->color_uniform != -1);
	} else {
		shader->color_uniform = -1;
	}
	shader->color_pre_curve_lut_2d_uniform =
		glGetUniformLocation(shader->program, "color_pre_curve_lut_2d");
	shader->color_pre_curve_lut_scale_offset_uniform =
		glGetUniformLocation(shader->program, "color_pre_curve_lut_scale_offset");

	shader->color_post_curve_lut_2d_uniform =
		glGetUniformLocation(shader->program, "color_post_curve_lut_2d");
	shader->color_post_curve_lut_scale_offset_uniform =
		glGetUniformLocation(shader->program, "color_post_curve_lut_scale_offset");

	switch(requirements->color_mapping) {
	case SHADER_COLOR_MAPPING_3DLUT:
		shader->color_mapping.lut3d.tex_uniform =
			glGetUniformLocation(shader->program,
					     "color_mapping_lut_3d");
		shader->color_mapping.lut3d.scale_offset_uniform =
			glGetUniformLocation(shader->program,
					     "color_mapping_lut_scale_offset");
		break;
	case SHADER_COLOR_MAPPING_MATRIX:
		shader->color_mapping.matrix_uniform =
			glGetUniformLocation(shader->program,
					     "color_mapping_matrix");
		break;
	case SHADER_COLOR_MAPPING_IDENTITY:
		break;
	}
	free(conf);

	wl_list_insert(&gr->shader_list, &shader->link);

	return shader;

error_link:
	glDeleteProgram(shader->program);
	glDeleteShader(shader->fragment_shader);

error_fragment:
	glDeleteShader(shader->vertex_shader);

error_vertex:
	free(conf);
	free(shader);
	return NULL;
}

void
gl_shader_destroy(struct gl_renderer *gr, struct gl_shader *shader)
{
	char *desc;

	if (weston_log_scope_is_enabled(gr->shader_scope)) {
		desc = create_shader_description_string(&shader->key);
		weston_log_scope_printf(gr->shader_scope,
					"Deleting shader program for: %s\n",
					desc);
		free(desc);
	}

	glDeleteProgram(shader->program);
	wl_list_remove(&shader->link);
	free(shader);
}

void
gl_renderer_shader_list_destroy(struct gl_renderer *gr)
{
	struct gl_shader *shader, *next_shader;

	wl_list_for_each_safe(shader, next_shader, &gr->shader_list, link)
		gl_shader_destroy(gr, shader);
}

static int
gl_shader_requirements_cmp(const struct gl_shader_requirements *a,
			   const struct gl_shader_requirements *b)
{
	return memcmp(a, b, sizeof(*a));
}

static void
gl_shader_scope_new_subscription(struct weston_log_subscription *subs,
				 void *data)
{
	static const char bar[] = "-----------------------------------------------------------------------------";
	struct gl_renderer *gr = data;
	struct gl_shader *shader;
	struct timespec now;
	int msecs;
	int count = 0;
	char *desc;

	weston_compositor_read_presentation_clock(gr->compositor, &now);

	weston_log_subscription_printf(subs,
				       "Vertex shader body:\n"
				       "%s\n%s\n"
				       "Fragment shader body:\n"
				       "%s\n%s\n%s\n",
				       bar, vertex_shader,
				       bar, fragment_shader, bar);

	weston_log_subscription_printf(subs,
		"Cached GLSL programs:\n    id: (used secs ago) description +/-flags\n");
	wl_list_for_each(shader, &gr->shader_list, link) {
		count++;
		msecs = timespec_sub_to_msec(&now, &shader->last_used);
		desc = create_shader_description_string(&shader->key);
		weston_log_subscription_printf(subs,
					       "%6u: (%.1f) %s\n",
					       shader->program,
					       msecs / 1000.0, desc);
	}
	weston_log_subscription_printf(subs, "Total: %d programs.\n", count);
}

struct weston_log_scope *
gl_shader_scope_create(struct gl_renderer *gr)
{

	return weston_compositor_add_log_scope(gr->compositor,
		"gl-shader-generator",
		"GL renderer shader compilation and cache.\n",
		gl_shader_scope_new_subscription,
		NULL,
		gr);
}

struct gl_shader *
gl_renderer_create_fallback_shader(struct gl_renderer *gr)
{
	static const struct gl_shader_requirements fallback_requirements = {
		.variant = SHADER_VARIANT_SOLID,
		.input_is_premult = true,
		.color_pre_curve = SHADER_COLOR_CURVE_IDENTITY,
		.color_mapping = SHADER_COLOR_MAPPING_IDENTITY,
		.color_post_curve = SHADER_COLOR_CURVE_IDENTITY,
	};
	struct gl_shader *shader;

	shader = gl_shader_create(gr, &fallback_requirements);
	if (!shader)
		return NULL;

	/*
	 * This shader must be exempt from any automatic garbage collection.
	 * It is destroyed explicitly.
	 */
	wl_list_remove(&shader->link);
	wl_list_init(&shader->link);

	return shader;
}

static struct gl_shader *
gl_renderer_get_program(struct gl_renderer *gr,
			const struct gl_shader_requirements *requirements)
{
	struct gl_shader_requirements reqs = *requirements;
	struct gl_shader *shader;

	assert(reqs.pad_bits_ == 0);

	if (gr->fragment_shader_debug)
		reqs.green_tint = true;

	if (gr->current_shader &&
	    gl_shader_requirements_cmp(&reqs, &gr->current_shader->key) == 0)
		return gr->current_shader;

	wl_list_for_each(shader, &gr->shader_list, link) {
		if (gl_shader_requirements_cmp(&reqs, &shader->key) == 0)
			return shader;
	}

	shader = gl_shader_create(gr, &reqs);
	if (shader)
		return shader;

	return NULL;
}

void
gl_renderer_garbage_collect_programs(struct gl_renderer *gr)
{
	struct gl_shader *shader, *tmp;
	unsigned count = 0;

	wl_list_for_each_safe(shader, tmp, &gr->shader_list, link) {
		/* Keep the 10 most recently used always. */
		if (count++ < 10)
			continue;

		/* Keep everything used in the past 1 minute. */
		if (timespec_sub_to_msec(&gr->compositor->last_repaint_start,
					 &shader->last_used) < 60000)
			continue;

		/* The rest throw away. */
		gl_shader_destroy(gr, shader);
	}
}

bool
gl_shader_texture_variant_can_be_premult(enum gl_shader_texture_variant v)
{
	switch (v) {
	case SHADER_VARIANT_SOLID:
	case SHADER_VARIANT_RGBA:
	case SHADER_VARIANT_EXTERNAL:
		return true;
	case SHADER_VARIANT_NONE:
	case SHADER_VARIANT_RGBX:
	case SHADER_VARIANT_Y_U_V:
	case SHADER_VARIANT_Y_UV:
	case SHADER_VARIANT_Y_XUXV:
	case SHADER_VARIANT_XYUV:
		return false;
	}
	return true;
}

GLenum
gl_shader_texture_variant_get_target(enum gl_shader_texture_variant v)
{
	if (v == SHADER_VARIANT_EXTERNAL)
		return GL_TEXTURE_EXTERNAL_OES;
	else
		return GL_TEXTURE_2D;
}

static void
gl_shader_load_config(struct gl_shader *shader,
		      const struct gl_shader_config *sconf)
{
	GLint in_filter = sconf->input_tex_filter;
	GLenum in_tgt;
	int i;

	glUniformMatrix4fv(shader->proj_uniform,
			   1, GL_FALSE, sconf->projection.d);

	if (shader->color_uniform != -1)
		glUniform4fv(shader->color_uniform, 1, sconf->unicolor);

	glUniform1f(shader->view_alpha_uniform, sconf->view_alpha);

	in_tgt = gl_shader_texture_variant_get_target(sconf->req.variant);
	for (i = 0; i < GL_SHADER_INPUT_TEX_MAX; i++) {
		if (sconf->input_tex[i] == 0)
			continue;

		assert(shader->tex_uniforms[i] != -1);
		glUniform1i(shader->tex_uniforms[i], i);
		glActiveTexture(GL_TEXTURE0 + i);

		glBindTexture(in_tgt, sconf->input_tex[i]);
		glTexParameteri(in_tgt, GL_TEXTURE_MIN_FILTER, in_filter);
		glTexParameteri(in_tgt, GL_TEXTURE_MAG_FILTER, in_filter);
	}

	/* Fixed texture unit for color_pre_curve LUT if it is available */
	i = GL_SHADER_INPUT_TEX_MAX;
	switch (sconf->req.color_pre_curve) {
	case SHADER_COLOR_CURVE_IDENTITY:
		assert(sconf->color_pre_curve_lut_tex == 0);
		break;
	case SHADER_COLOR_CURVE_LUT_3x1D:
		assert(sconf->color_pre_curve_lut_tex != 0);
		assert(shader->color_pre_curve_lut_2d_uniform != -1);
		assert(shader->color_pre_curve_lut_scale_offset_uniform != -1);
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, sconf->color_pre_curve_lut_tex);
		glUniform1i(shader->color_pre_curve_lut_2d_uniform, i);
		i++;
		glUniform2fv(shader->color_pre_curve_lut_scale_offset_uniform,
			     1, sconf->color_pre_curve_lut_scale_offset);
		break;
	}

	switch (sconf->req.color_mapping) {
	case SHADER_COLOR_MAPPING_IDENTITY:
		break;
	case SHADER_COLOR_MAPPING_3DLUT:
		assert(shader->color_mapping.lut3d.tex_uniform != -1);
		assert(sconf->color_mapping.lut3d.tex != 0);
		assert(shader->color_mapping.lut3d.scale_offset_uniform != -1);
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_3D, sconf->color_mapping.lut3d.tex);
		glUniform1i(shader->color_mapping.lut3d.tex_uniform, i);
		i++;
		glUniform2fv(shader->color_mapping.lut3d.scale_offset_uniform,
			     1, sconf->color_mapping.lut3d.scale_offset);
		break;
	case SHADER_COLOR_MAPPING_MATRIX:
		assert(shader->color_mapping.matrix_uniform != -1);
		glUniformMatrix3fv(shader->color_mapping.matrix_uniform,
				   1, GL_FALSE,
				   sconf->color_mapping.matrix);
		break;
	}

	switch (sconf->req.color_post_curve) {
	case SHADER_COLOR_CURVE_IDENTITY:
		assert(sconf->color_post_curve_lut_tex == 0);
		break;
	case SHADER_COLOR_CURVE_LUT_3x1D:
		assert(sconf->color_post_curve_lut_tex != 0);
		assert(shader->color_post_curve_lut_2d_uniform != -1);
		assert(shader->color_post_curve_lut_scale_offset_uniform != -1);
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, sconf->color_post_curve_lut_tex);
		glUniform1i(shader->color_post_curve_lut_2d_uniform, i);
		i++;
		glUniform2fv(shader->color_post_curve_lut_scale_offset_uniform,
			     1, sconf->color_post_curve_lut_scale_offset);
		break;
	}
}

bool
gl_renderer_use_program(struct gl_renderer *gr,
			const struct gl_shader_config *sconf)
{
	static const GLfloat fallback_shader_color[4] = { 0.2, 0.1, 0.0, 1.0 };
	struct gl_shader *shader;

	shader = gl_renderer_get_program(gr, &sconf->req);
	if (!shader) {
		weston_log("Error: failed to generate shader program.\n");
		gr->current_shader = NULL;

		/*
		 * We only have one fallback shader, so it cannot do correct
		 * color on color managed outputs. Hence, what is painted
		 * with this one will have undefined look. Therefore the
		 * fallback is important to not be too bright as that might
		 * be shocking on a monitor in HDR mode.
		 */

		shader = gr->fallback_shader;
		glUseProgram(shader->program);
		glUniform4fv(shader->color_uniform, 1, fallback_shader_color);
		glUniform1f(shader->view_alpha_uniform, 1.0f);
		return false;
	}

	if (shader != gr->fallback_shader) {
		/* Update list order for most recently used. */
		wl_list_remove(&shader->link);
		wl_list_insert(&gr->shader_list, &shader->link);
	}
	shader->last_used = gr->compositor->last_repaint_start;

	if (gr->current_shader != shader) {
		glUseProgram(shader->program);
		gr->current_shader = shader;
	}

	gl_shader_load_config(shader, sconf);

	return true;
}
