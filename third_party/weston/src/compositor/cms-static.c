/*
 * Copyright © 2013 Richard Hughes
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

#include <libweston/libweston.h>
#include "cms-helper.h"
#include "shared/helpers.h"
#include "weston.h"

struct cms_static {
	struct weston_compositor	*ec;
	struct wl_listener		 destroy_listener;
	struct wl_listener		 output_created_listener;
};

static void
cms_output_created(struct cms_static *cms, struct weston_output *o)
{
	struct weston_color_profile *p;
	struct weston_config_section *s;
	char *profile;

	weston_log("cms-static: output %i [%s] created\n", o->id, o->name);

	if (o->name == NULL)
		return;
	s = weston_config_get_section(wet_get_config(cms->ec),
				      "output", "name", o->name);
	if (s == NULL)
		return;
	if (weston_config_section_get_string(s, "icc_profile", &profile, NULL) < 0)
		return;
	p = weston_cms_load_profile(profile);
	if (p == NULL && strlen(profile) > 0) {
		weston_log("cms-static: failed to load %s\n", profile);
	} else {
		weston_log("cms-static: loading %s for %s\n",
			   (p != NULL) ? profile : "identity LUT",
			   o->name);
		weston_cms_set_color_profile(o, p);
	}
}

static void
cms_notifier_output_created(struct wl_listener *listener, void *data)
{
	struct weston_output *o = (struct weston_output *) data;
	struct cms_static *cms =
		container_of(listener, struct cms_static, output_created_listener);
	cms_output_created(cms, o);
}

static void
cms_module_destroy(struct cms_static *cms)
{
	free(cms);
}

static void
cms_notifier_destroy(struct wl_listener *listener, void *data)
{
	struct cms_static *cms = container_of(listener, struct cms_static, destroy_listener);
	cms_module_destroy(cms);
}


WL_EXPORT int
wet_module_init(struct weston_compositor *ec,
		int *argc, char *argv[])
{
	struct cms_static *cms;
	struct weston_output *output;

	weston_log("cms-static: initialized\n");

	/* create local state object */
	cms = zalloc(sizeof *cms);
	if (cms == NULL)
		return -1;

	cms->ec = ec;

	if (!weston_compositor_add_destroy_listener_once(ec,
							 &cms->destroy_listener,
							 cms_notifier_destroy)) {
		free(cms);
		return 0;
	}

	cms->output_created_listener.notify = cms_notifier_output_created;
	wl_signal_add(&ec->output_created_signal, &cms->output_created_listener);

	/* discover outputs */
	wl_list_for_each(output, &ec->output_list, link)
		cms_output_created(cms, output);

	return 0;
}
