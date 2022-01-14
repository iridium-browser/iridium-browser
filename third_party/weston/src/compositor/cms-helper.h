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

#ifndef _WESTON_CMS_H_
#define _WESTON_CMS_H_

#include "config.h"

#include <libweston/libweston.h>

/* General overview on how to be a CMS plugin:
 *
 * First, some nomenclature:
 *
 *  CMF:       Color management framework, i.e. "Use foo.icc for device $bar"
 *  CMM:       Color management module that converts pixel colors, which is
 *             usually lcms2 on any modern OS.
 *  CMS:       Color management system that encompasses both a CMF and CMM.
 *  ICC:       International Color Consortium, the people that define the
 *             binary encoding of a .icc file.
 *  VCGT:      Video Card Gamma Tag. An Apple extension to the ICC specification
 *             that allows the calibration state to be stored in the ICC profile
 *  Output:    Physical port with a display attached, e.g. LVDS1
 *
 * As a CMF is probably something you don't want or need on an embedded install
 * these functions will not be called if the icc_profile key is set for a
 * specific [output] section in weston.ini
 *
 * Most desktop environments want the CMF to decide what profile to use in
 * different situations, so that displays can be profiled and also so that
 * the ICC profiles can be changed at runtime depending on the task or ambient
 * environment.
 *
 * The CMF can be selected using the 'modules' key in the [core] section.
 */

struct weston_color_profile {
	char	*filename;
	void	*lcms_handle;
};

void
weston_cms_set_color_profile(struct weston_output *o,
			     struct weston_color_profile *p);
struct weston_color_profile *
weston_cms_create_profile(const char *filename,
			  void *lcms_profile);
struct weston_color_profile *
weston_cms_load_profile(const char *filename);
void
weston_cms_destroy_profile(struct weston_color_profile *p);

#endif
