/*
 * Copyright © 2021-2023 Philipp Zabel <p.zabel@pengutronix.de>
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

#ifndef WESTON_COMPOSITOR_PIPEWIRE_H
#define WESTON_COMPOSITOR_PIPEWIRE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <libweston/libweston.h>
#include <libweston/plugin-registry.h>

#define WESTON_PIPEWIRE_OUTPUT_API_NAME "weston_pipewire_output_api_v2"

struct pipewire_config {
	int32_t width;
	int32_t height;
	uint32_t framerate;
};

struct weston_pipewire_output_api {
	/** Create a new PipeWire head.
	 *
	 * \param backend    The backend.
	 * \param name       Desired name for the new head
	 * \param config     The pipewire_config of the new head.
	 *
	 * Returns 0 on success, -1 on failure.
	 */
	void (*head_create)(struct weston_backend *backend,
			    const char *name,
			    const struct pipewire_config *config);

	/** Set the size of a PipeWire output to the specified width and height.
	 *
	 * If the width or height are set to -1, the size of the underlying
	 * PipeWire head will be used.
	 *
	 * \param output     The weston output for which the size shall be set
	 * \param width	     Desired width of the output
	 * \param height     Desired height of the output
	 *
	 * Returns 0 on success, -1 on failure.
	 */
	int (*output_set_size)(struct weston_output *output,
			       int width, int height);

	/** The pixel format to be used by the output.
	 *
	 * \param output     The weston output for which the pixel format is set
	 * \param gbm_format String representation of the pixel format
	 *
	 * Valid values for the gbm_format are:
	 * - NULL - The format set at backend creation time will be used;
	 * - "xrgb8888";
	 * - "rgb565";
	 */
	void (*set_gbm_format)(struct weston_output *output,
			       const char *gbm_format);
};

static inline const struct weston_pipewire_output_api *
weston_pipewire_output_get_api(struct weston_compositor *compositor)
{
	const void *api;
	api = weston_plugin_api_get(compositor, WESTON_PIPEWIRE_OUTPUT_API_NAME,
				    sizeof(struct weston_pipewire_output_api));

	return (const struct weston_pipewire_output_api *)api;
}

#define WESTON_PIPEWIRE_BACKEND_CONFIG_VERSION 1

struct weston_pipewire_backend_config {
	struct weston_backend_config base;
	enum weston_renderer_type renderer;
	char *gbm_format;
	int32_t num_outputs;
};

#ifdef  __cplusplus
}
#endif

#endif /* WESTON_COMPOSITOR_PIPEWIRE_H */
