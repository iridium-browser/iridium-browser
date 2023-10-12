/*
 * Copyright © 2016 Benoit Gschwind
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

#ifndef WESTON_COMPOSITOR_RDP_H
#define WESTON_COMPOSITOR_RDP_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <libweston/libweston.h>
#include <libweston/plugin-registry.h>

#define WESTON_RDP_OUTPUT_API_NAME "weston_rdp_output_api_v2"
#define RDP_DEFAULT_FREQ 60

struct weston_rdp_monitor {
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
	uint32_t desktop_scale;
};

struct weston_rdp_output_api {
	/** Get config from RDP client when connected
	 */
	void (*head_get_monitor)(struct weston_head *head,
				 struct weston_rdp_monitor *monitor);

	/** Set mode for an output */
	void (*output_set_mode)(struct weston_output *base,
				struct weston_mode *mode);
};

static inline const struct weston_rdp_output_api *
weston_rdp_output_get_api(struct weston_compositor *compositor)
{
	const void *api;
	api = weston_plugin_api_get(compositor, WESTON_RDP_OUTPUT_API_NAME,
				    sizeof(struct weston_rdp_output_api));

	return (const struct weston_rdp_output_api *)api;
}

#define WESTON_RDP_BACKEND_CONFIG_VERSION 3

typedef void *(*rdp_audio_in_setup)(struct weston_compositor *c, void *vcm);
typedef void (*rdp_audio_in_teardown)(void *audio_private);
typedef void *(*rdp_audio_out_setup)(struct weston_compositor *c, void *vcm);
typedef void (*rdp_audio_out_teardown)(void *audio_private);

struct weston_rdp_backend_config {
	struct weston_backend_config base;
	enum weston_renderer_type renderer;
	char *bind_address;
	int port;
	char *rdp_key;
	char *server_cert;
	char *server_key;
	int env_socket;
	int no_clients_resize;
	int force_no_compression;
	bool remotefx_codec;
	int external_listener_fd;
	int refresh_rate;
	rdp_audio_in_setup audio_in_setup;
	rdp_audio_in_teardown audio_in_teardown;
	rdp_audio_out_setup audio_out_setup;
	rdp_audio_out_teardown audio_out_teardown;
};

#ifdef  __cplusplus
}
#endif

#endif /* WESTON_COMPOSITOR_RDP_H */
