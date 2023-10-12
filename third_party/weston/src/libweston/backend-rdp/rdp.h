/*
 * Copyright © 2013 Hardening <rdp.effort@gmail.com>
 * Copyright © 2020 Microsoft
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

#ifndef RDP_H
#define RDP_H

#include <freerdp/version.h>

#include <freerdp/freerdp.h>
#include <freerdp/listener.h>
#include <freerdp/update.h>
#include <freerdp/input.h>
#include <freerdp/codec/color.h>
#include <freerdp/codec/rfx.h>
#include <freerdp/codec/nsc.h>
#include <freerdp/locale/keyboard.h>
#include <freerdp/channels/wtsvc.h>
#include <freerdp/server/cliprdr.h>

#include <libweston/libweston.h>
#include <libweston/backend-rdp.h>
#include <libweston/weston-log.h>

#include "backend.h"

#include "shared/helpers.h"
#include "shared/string-helpers.h"

#define MAX_FREERDP_FDS 32
#define RDP_MAX_MONITOR 16
#define DEFAULT_AXIS_STEP_DISTANCE 10
#define DEFAULT_PIXEL_FORMAT PIXEL_FORMAT_BGRA32

/* https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getkeyboardtype
 * defines a keyboard type that isn't currently defined in FreeRDP, but is
 * available for RDP connections */
#ifndef KBD_TYPE_KOREAN
#define KBD_TYPE_KOREAN 8
#endif

/* WinPR's GetVirtualKeyCodeFromVirtualScanCode() can't handle hangul/hanja keys */
/* 0x1f1 and 0x1f2 keys are only exists on Korean 103 keyboard (Type 8:SubType 6) */

/* From Linux's keyboard driver at drivers/input/keyboard/atkbd.c */
#define ATKBD_RET_HANJA 0xf1
#define ATKBD_RET_HANGEUL 0xf2

struct rdp_backend {
	struct weston_backend base;
	struct weston_compositor *compositor;

	freerdp_listener *listener;
	struct wl_event_source *listener_events[MAX_FREERDP_FDS];
	struct weston_log_scope *debug;
	struct weston_log_scope *verbose;

	struct weston_log_scope *clipboard_debug;
	struct weston_log_scope *clipboard_verbose;

	struct wl_list peers;

	char *server_cert;
	char *server_key;
	char *rdp_key;
	int tls_enabled;
	int no_clients_resize;
	int force_no_compression;
	bool remotefx_codec;
	int external_listener_fd;
	int rdp_monitor_refresh_rate;
	pid_t compositor_tid;

        rdp_audio_in_setup audio_in_setup;
        rdp_audio_in_teardown audio_in_teardown;
        rdp_audio_out_setup audio_out_setup;
        rdp_audio_out_teardown audio_out_teardown;

	uint32_t head_index;
};

enum peer_item_flags {
	RDP_PEER_ACTIVATED      = (1 << 0),
	RDP_PEER_OUTPUT_ENABLED = (1 << 1),
};

struct rdp_peers_item {
	int flags;
	freerdp_peer *peer;
	struct weston_seat *seat;

	struct wl_list link;
};

struct rdp_head {
	struct weston_head base;
	uint32_t index;
	bool matched;
	rdpMonitor config;
};

struct rdp_output {
	struct weston_output base;
	struct rdp_backend *backend;
	struct wl_event_source *finish_frame_timer;
	struct weston_renderbuffer *renderbuffer;
};

struct rdp_peer_context {
	rdpContext _p;

	struct rdp_backend *rdpBackend;
	struct wl_event_source *events[MAX_FREERDP_FDS + 1]; /* +1 for WTSVirtualChannelManagerGetFileDescriptor */
	RFX_CONTEXT *rfx_context;
	wStream *encode_stream;
	RFX_RECT *rfx_rects;
	NSC_CONTEXT *nsc_context;

	struct rdp_peers_item item;

	bool button_state[5];

	int verticalAccumWheelRotationPrecise;
	int verticalAccumWheelRotationDiscrete;
	int horizontalAccumWheelRotationPrecise;
	int horizontalAccumWheelRotationDiscrete;

	HANDLE vcm;

	/* list of outstanding event_source sent from FreeRDP thread to display loop.*/
	int loop_task_event_source_fd;
	struct wl_event_source *loop_task_event_source;
	pthread_mutex_t loop_task_list_mutex;
	struct wl_list loop_task_list; /* struct rdp_loop_task::link */

	/* Clipboard support */
	CliprdrServerContext *clipboard_server_context;

	void *audio_in_private;
	void *audio_out_private;

	struct rdp_clipboard_data_source *clipboard_client_data_source;
	struct rdp_clipboard_data_source *clipboard_inflight_client_data_source;

	struct wl_listener clipboard_selection_listener;

	/* Multiple monitor support (monitor topology) */
	int32_t desktop_top, desktop_left;
	int32_t desktop_width, desktop_height;
};

typedef struct rdp_peer_context RdpPeerContext;

typedef void (*rdp_loop_task_func_t)(bool freeOnly, void *data);

struct rdp_loop_task {
	struct wl_list link;
	RdpPeerContext *peerCtx;
	rdp_loop_task_func_t func;
};

#define rdp_debug_verbose(b, ...) \
	rdp_debug_print(b->verbose, false, __VA_ARGS__)
#define rdp_debug_verbose_continue(b, ...) \
	rdp_debug_print(b->verbose, true,  __VA_ARGS__)
#define rdp_debug(b, ...) \
	rdp_debug_print(b->debug, false, __VA_ARGS__)
#define rdp_debug_continue(b, ...) \
	rdp_debug_print(b->debug, true,  __VA_ARGS__)

#define rdp_debug_clipboard_verbose(b, ...) \
	rdp_debug_print(b->clipboard_verbose, false, __VA_ARGS__)
#define rdp_debug_clipboard_verbose_continue(b, ...) \
	rdp_debug_print(b->clipboard_verbose, true,  __VA_ARGS__)
#define rdp_debug_clipboard(b, ...) \
	rdp_debug_print(b->clipboard_debug, false, __VA_ARGS__)
#define rdp_debug_clipboard_continue(b, ...) \
	rdp_debug_print(b->clipboard_debug, true,  __VA_ARGS__)

/* rdpdisp.c */
bool
handle_adjust_monitor_layout(freerdp_peer *client,
			     int monitor_count, rdpMonitor *monitors);

struct weston_output *
to_weston_coordinate(RdpPeerContext *peerContext,
		     int32_t *x, int32_t *y);

/* rdputil.c */
void
rdp_debug_print(struct weston_log_scope *log_scope, bool cont, char *fmt, ...);

int
rdp_wl_array_read_fd(struct wl_array *array, int fd);

void
convert_rdp_keyboard_to_xkb_rule_names(UINT32 KeyboardType, UINT32 KeyboardSubType, UINT32 KeyboardLayout, struct xkb_rule_names *xkbRuleNames);

void
assert_compositor_thread(struct rdp_backend *b);

void
assert_not_compositor_thread(struct rdp_backend *b);

bool
rdp_event_loop_add_fd(struct wl_event_loop *loop,
		      int fd, uint32_t mask,
		      wl_event_loop_fd_func_t func,
		      void *data,
		      struct wl_event_source **event_source);

void
rdp_dispatch_task_to_display_loop(RdpPeerContext *peerCtx,
				  rdp_loop_task_func_t func,
				  struct rdp_loop_task *task);

bool
rdp_initialize_dispatch_task_event_source(RdpPeerContext *peerCtx);

void
rdp_destroy_dispatch_task_event_source(RdpPeerContext *peerCtx);

/* rdpclip.c */
int
rdp_clipboard_init(freerdp_peer *client);

void
rdp_clipboard_destroy(RdpPeerContext *peerCtx);

/* rdp.c */
void
rdp_head_create(struct rdp_backend *backend, rdpMonitor *config);

void
rdp_destroy(struct weston_backend *backend);

void
rdp_head_destroy(struct weston_head *base);

static inline struct rdp_head *
to_rdp_head(struct weston_head *base)
{
	if (base->backend->destroy != rdp_destroy)
		return NULL;
	return container_of(base, struct rdp_head, base);
}

void
rdp_output_destroy(struct weston_output *base);

static inline struct rdp_output *
to_rdp_output(struct weston_output *base)
{
	if (base->destroy != rdp_output_destroy)
		return NULL;
	return container_of(base, struct rdp_output, base);
}

#endif
