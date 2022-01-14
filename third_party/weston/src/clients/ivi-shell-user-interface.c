/*
 * Copyright (C) 2013 DENSO CORPORATION
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <linux/input.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <getopt.h>
#include <errno.h>
#include <wayland-cursor.h>
#include <wayland-client-protocol.h>
#include "shared/cairo-util.h"
#include <libweston/config-parser.h>
#include "shared/helpers.h"
#include "shared/os-compatibility.h"
#include "shared/xalloc.h"
#include <libweston/zalloc.h>
#include "shared/file-util.h"
#include "ivi-application-client-protocol.h"
#include "ivi-hmi-controller-client-protocol.h"

/**
 * A reference implementation how to use ivi-hmi-controller interface to
 * interact with hmi-controller. This is launched from hmi-controller by using
 * hmi_client_start and create a pthread.
 *
 * The basic flow is as followed,
 * 1/ read configuration from weston.ini.
 * 2/ draw png file to surface according to configuration of weston.ini
 * 3/ set up UI by using ivi-hmi-controller protocol
 * 4/ Enter event loop
 * 5/ If a surface receives touch/pointer event, followings are invoked
 *	according to type of event and surface
 * 5-1/ If a surface to launch ivi_application receive touch up, it execs
 *	  ivi-application configured in weston.ini.
 * 5-2/ If a surface to switch layout mode receive touch up, it sends a request,
 *	  ivi_hmi_controller_switch_mode, to hmi-controller.
 * 5-3/ If a surface to show workspace having launchers, it sends a request,
 *	  ivi_hmi_controller_home, to hmi-controller.
 * 5-4/ If touch down events happens in workspace,
 *	  ivi_hmi_controller_workspace_control is sent to slide workspace.
 *	  When control finished, event: ivi_hmi_controller_workspace_end_control
 *	  is received.
 */

/*****************************************************************************
 *  structure, globals
 ****************************************************************************/
enum cursor_type {
	CURSOR_BOTTOM_LEFT,
	CURSOR_BOTTOM_RIGHT,
	CURSOR_BOTTOM,
	CURSOR_DRAGGING,
	CURSOR_LEFT_PTR,
	CURSOR_LEFT,
	CURSOR_RIGHT,
	CURSOR_TOP_LEFT,
	CURSOR_TOP_RIGHT,
	CURSOR_TOP,
	CURSOR_IBEAM,
	CURSOR_HAND1,
	CURSOR_WATCH,

	CURSOR_BLANK
};
struct wlContextCommon {
	struct wl_display		*wlDisplay;
	struct wl_registry		*wlRegistry;
	struct wl_compositor		*wlCompositor;
	struct wl_shm			*wlShm;
	uint32_t			formats;
	struct wl_seat			*wlSeat;
	struct wl_pointer		*wlPointer;
	struct wl_touch			*wlTouch;
	struct ivi_application		*iviApplication;
	struct ivi_hmi_controller	*hmiCtrl;
	struct hmi_homescreen_setting	*hmi_setting;
	struct wl_list			list_wlContextStruct;
	struct wl_surface		*enterSurface;
	int32_t				is_home_on;
	struct wl_cursor_theme		*cursor_theme;
	struct wl_cursor		**cursors;
	struct wl_surface		*pointer_surface;
	enum   cursor_type		current_cursor;
	uint32_t			enter_serial;
};

struct wlContextStruct {
	struct wlContextCommon	*cmm;
	struct wl_surface	*wlSurface;
	struct wl_buffer	*wlBuffer;
	cairo_surface_t		*ctx_image;
	void			*data;
	uint32_t		id_surface;
	struct wl_list		link;
};

struct
hmi_homescreen_srf {
	uint32_t	id;
	char		*filePath;
	uint32_t	color;
};

struct
hmi_homescreen_workspace {
	struct wl_array	launcher_id_array;
	struct wl_list	link;
};

struct
hmi_homescreen_launcher {
	uint32_t	icon_surface_id;
	uint32_t	workspace_id;
	char		*icon;
	char		*path;
	struct wl_list	link;
};

struct
hmi_homescreen_setting {
	struct hmi_homescreen_srf background;
	struct hmi_homescreen_srf panel;
	struct hmi_homescreen_srf tiling;
	struct hmi_homescreen_srf sidebyside;
	struct hmi_homescreen_srf fullscreen;
	struct hmi_homescreen_srf random;
	struct hmi_homescreen_srf home;
	struct hmi_homescreen_srf workspace_background;

	struct wl_list workspace_list;
	struct wl_list launcher_list;

	char		*cursor_theme;
	int32_t		cursor_size;
	uint32_t	transition_duration;
	uint32_t	surface_id_offset;
	int32_t		screen_num;
};

/*****************************************************************************
 *  Event Handler
 ****************************************************************************/

static void
shm_format(void *data, struct wl_shm *pWlShm, uint32_t format)
{
	struct wlContextCommon *pCtx = data;

	pCtx->formats |= (1 << format);
}

static struct wl_shm_listener shm_listenter = {
	shm_format
};

static int32_t
getIdOfWlSurface(struct wlContextCommon *pCtx, struct wl_surface *wlSurface)
{
	struct wlContextStruct *pWlCtxSt = NULL;

	if (NULL == pCtx || NULL == wlSurface )
		return 0;

	wl_list_for_each(pWlCtxSt, &pCtx->list_wlContextStruct, link) {
		if (pWlCtxSt->wlSurface == wlSurface)
			return pWlCtxSt->id_surface;
	}

	return -1;
}

static void
set_pointer_image(struct wlContextCommon *pCtx, uint32_t index)
{
	struct wl_cursor *cursor = NULL;
	struct wl_cursor_image *image = NULL;
	struct wl_buffer *buffer = NULL;

	if (!pCtx->wlPointer || !pCtx->cursors)
		return;

	if (CURSOR_BLANK == pCtx->current_cursor) {
		wl_pointer_set_cursor(pCtx->wlPointer, pCtx->enter_serial,
							  NULL, 0, 0);
		return;
	}

	cursor = pCtx->cursors[pCtx->current_cursor];
	if (!cursor)
		return;

	if (cursor->image_count <= index) {
		fprintf(stderr, "cursor index out of range\n");
		return;
	}

	image = cursor->images[index];
	buffer = wl_cursor_image_get_buffer(image);

	if (!buffer)
		return;

	wl_pointer_set_cursor(pCtx->wlPointer, pCtx->enter_serial,
			      pCtx->pointer_surface,
			      image->hotspot_x, image->hotspot_y);

	wl_surface_attach(pCtx->pointer_surface, buffer, 0, 0);

	wl_surface_damage(pCtx->pointer_surface, 0, 0,
					  image->width, image->height);

	wl_surface_commit(pCtx->pointer_surface);
}

static void
PointerHandleEnter(void *data, struct wl_pointer *wlPointer, uint32_t serial,
		   struct wl_surface *wlSurface, wl_fixed_t sx, wl_fixed_t sy)
{
	struct wlContextCommon *pCtx = data;

	pCtx->enter_serial = serial;
	pCtx->enterSurface = wlSurface;
	set_pointer_image(pCtx, 0);
#ifdef _DEBUG
	printf("ENTER PointerHandleEnter: x(%d), y(%d)\n", sx, sy);
#endif
}

static void
PointerHandleLeave(void *data, struct wl_pointer *wlPointer, uint32_t serial,
		   struct wl_surface *wlSurface)
{
	struct wlContextCommon *pCtx = data;

	pCtx->enterSurface = NULL;

#ifdef _DEBUG
	printf("ENTER PointerHandleLeave: serial(%d)\n", serial);
#endif
}

static void
PointerHandleMotion(void *data, struct wl_pointer *wlPointer, uint32_t time,
		    wl_fixed_t sx, wl_fixed_t sy)
{
#ifdef _DEBUG
	printf("ENTER PointerHandleMotion: x(%d), y(%d)\n", sx, sy);
#endif
}

/**
 * if a surface assigned as launcher receives touch-off event, invoking
 * ivi-application which configured in weston.ini with path to binary.
 */
extern char **environ; /*defied by libc */

static pid_t
execute_process(char *path, char *argv[])
{
	pid_t pid = fork();
	if (pid < 0)
		fprintf(stderr, "Failed to fork\n");

	if (pid)
		return pid;

	if (-1 == execve(path, argv, environ)) {
		fprintf(stderr, "Failed to execve %s\n", path);
		exit(1);
	}

	return pid;
}

static int32_t
launcher_button(uint32_t surfaceId, struct wl_list *launcher_list)
{
	struct hmi_homescreen_launcher *launcher = NULL;

	wl_list_for_each(launcher, launcher_list, link) {
		char *argv[] = { NULL };

		if (surfaceId != launcher->icon_surface_id)
			continue;

		execute_process(launcher->path, argv);

		return 1;
	}

	return 0;
}

/**
 * is-method to identify a surface set as launcher in workspace or workspace
 * itself. This is-method is used to decide whether request;
 * ivi_hmi_controller_workspace_control is sent or not.
 */
static int32_t
isWorkspaceSurface(uint32_t id, struct hmi_homescreen_setting *hmi_setting)
{
	struct hmi_homescreen_launcher *launcher = NULL;

	if (id == hmi_setting->workspace_background.id)
		return 1;

	wl_list_for_each(launcher, &hmi_setting->launcher_list, link) {
		if (id == launcher->icon_surface_id)
			return 1;
	}

	return 0;
}

/**
 * Decide which request is sent to hmi-controller
 */
static void
touch_up(struct ivi_hmi_controller *hmi_ctrl, uint32_t id_surface,
	 int32_t *is_home_on, struct hmi_homescreen_setting *hmi_setting)
{
	if (launcher_button(id_surface, &hmi_setting->launcher_list)) {
		*is_home_on = 0;
		ivi_hmi_controller_home(hmi_ctrl, IVI_HMI_CONTROLLER_HOME_OFF);
	} else if (id_surface == hmi_setting->tiling.id) {
		ivi_hmi_controller_switch_mode(hmi_ctrl,
				IVI_HMI_CONTROLLER_LAYOUT_MODE_TILING);
	} else if (id_surface == hmi_setting->sidebyside.id) {
		ivi_hmi_controller_switch_mode(hmi_ctrl,
				IVI_HMI_CONTROLLER_LAYOUT_MODE_SIDE_BY_SIDE);
	} else if (id_surface == hmi_setting->fullscreen.id) {
		ivi_hmi_controller_switch_mode(hmi_ctrl,
				IVI_HMI_CONTROLLER_LAYOUT_MODE_FULL_SCREEN);
	} else if (id_surface == hmi_setting->random.id) {
		ivi_hmi_controller_switch_mode(hmi_ctrl,
				IVI_HMI_CONTROLLER_LAYOUT_MODE_RANDOM);
	} else if (id_surface == hmi_setting->home.id) {
		*is_home_on = !(*is_home_on);
		if (*is_home_on) {
			ivi_hmi_controller_home(hmi_ctrl,
						IVI_HMI_CONTROLLER_HOME_ON);
		} else {
			ivi_hmi_controller_home(hmi_ctrl,
						IVI_HMI_CONTROLLER_HOME_OFF);
		}
	}
}

/**
 * Even handler of Pointer event. IVI system is usually manipulated by touch
 * screen. However, some systems also have pointer device.
 * Release is the same behavior as touch off
 * Pressed is the same behavior as touch on
 */
static void
PointerHandleButton(void *data, struct wl_pointer *wlPointer, uint32_t serial,
		    uint32_t time, uint32_t button, uint32_t state)
{
	struct wlContextCommon *pCtx = data;
	struct ivi_hmi_controller *hmi_ctrl = pCtx->hmiCtrl;
	const uint32_t id_surface = getIdOfWlSurface(pCtx, pCtx->enterSurface);

	if (BTN_RIGHT == button)
		return;

	switch (state) {
	case WL_POINTER_BUTTON_STATE_RELEASED:
		touch_up(hmi_ctrl, id_surface, &pCtx->is_home_on,
			 pCtx->hmi_setting);
		break;

	case WL_POINTER_BUTTON_STATE_PRESSED:

		if (isWorkspaceSurface(id_surface, pCtx->hmi_setting)) {
			ivi_hmi_controller_workspace_control(hmi_ctrl,
							     pCtx->wlSeat,
							     serial);
		}

		break;
	}
#ifdef _DEBUG
	printf("ENTER PointerHandleButton: button(%d), state(%d)\n",
	       button, state);
#endif
}

static void
PointerHandleAxis(void *data, struct wl_pointer *wlPointer, uint32_t time,
		  uint32_t axis, wl_fixed_t value)
{
#ifdef _DEBUG
	printf("ENTER PointerHandleAxis: axis(%d), value(%d)\n", axis, value);
#endif
}

static struct wl_pointer_listener pointer_listener = {
	PointerHandleEnter,
	PointerHandleLeave,
	PointerHandleMotion,
	PointerHandleButton,
	PointerHandleAxis
};

/**
 * Even handler of touch event
 */
static void
TouchHandleDown(void *data, struct wl_touch *wlTouch, uint32_t serial,
		uint32_t time, struct wl_surface *surface, int32_t id,
		wl_fixed_t x_w, wl_fixed_t y_w)
{
	struct wlContextCommon *pCtx = data;
	struct ivi_hmi_controller *hmi_ctrl = pCtx->hmiCtrl;
	uint32_t id_surface = 0;

	if (0 == id)
		pCtx->enterSurface = surface;

	id_surface = getIdOfWlSurface(pCtx, pCtx->enterSurface);

	/**
	 * When touch down happens on surfaces of workspace, ask
	 * hmi-controller to start control workspace to select page of
	 * workspace. After sending seat to hmi-controller by
	 * ivi_hmi_controller_workspace_control,
	 * hmi-controller-homescreen doesn't receive any event till
	 * hmi-controller sends back it.
	 */
	if (isWorkspaceSurface(id_surface, pCtx->hmi_setting)) {
		ivi_hmi_controller_workspace_control(hmi_ctrl, pCtx->wlSeat,
						     serial);
	}
}

static void
TouchHandleUp(void *data, struct wl_touch *wlTouch, uint32_t serial,
	      uint32_t time, int32_t id)
{
	struct wlContextCommon *pCtx = data;
	struct ivi_hmi_controller *hmi_ctrl = pCtx->hmiCtrl;

	const uint32_t id_surface = getIdOfWlSurface(pCtx, pCtx->enterSurface);

	/**
	 * triggering event according to touch-up happening on which surface.
	 */
	if (id == 0){
		touch_up(hmi_ctrl, id_surface, &pCtx->is_home_on,
			 pCtx->hmi_setting);
	}
}

static void
TouchHandleMotion(void *data, struct wl_touch *wlTouch, uint32_t time,
		  int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
}

static void
TouchHandleFrame(void *data, struct wl_touch *wlTouch)
{
}

static void
TouchHandleCancel(void *data, struct wl_touch *wlTouch)
{
}

static struct wl_touch_listener touch_listener = {
	TouchHandleDown,
	TouchHandleUp,
	TouchHandleMotion,
	TouchHandleFrame,
	TouchHandleCancel,
};

/**
 * Handler of capabilities
 */
static void
seat_handle_capabilities(void *data, struct wl_seat *seat, uint32_t caps)
{
	struct wlContextCommon *p_wlCtx = (struct wlContextCommon*)data;
	struct wl_seat *wlSeat = p_wlCtx->wlSeat;
	struct wl_pointer *wlPointer = p_wlCtx->wlPointer;
	struct wl_touch *wlTouch = p_wlCtx->wlTouch;

	if (p_wlCtx->hmi_setting->cursor_theme) {
		if ((caps & WL_SEAT_CAPABILITY_POINTER) && !wlPointer){
			wlPointer = wl_seat_get_pointer(wlSeat);
			wl_pointer_add_listener(wlPointer,
						&pointer_listener, data);
		} else
		if (!(caps & WL_SEAT_CAPABILITY_POINTER) && wlPointer){
			wl_pointer_destroy(wlPointer);
			wlPointer = NULL;
		}
		p_wlCtx->wlPointer = wlPointer;
	}

	if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !wlTouch){
		wlTouch = wl_seat_get_touch(wlSeat);
		wl_touch_add_listener(wlTouch, &touch_listener, data);
	} else
	if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && wlTouch){
		wl_touch_destroy(wlTouch);
		wlTouch = NULL;
	}
	p_wlCtx->wlTouch = wlTouch;
}

static struct wl_seat_listener seat_Listener = {
	seat_handle_capabilities,
};

/**
 * Registration of event
 * This event is received when hmi-controller server finished controlling
 * workspace.
 */
static void
ivi_hmi_controller_workspace_end_control(void *data,
					 struct ivi_hmi_controller *hmi_ctrl,
					 int32_t is_controlled)
{
	struct wlContextCommon *pCtx = data;
	const uint32_t id_surface = getIdOfWlSurface(pCtx, pCtx->enterSurface);

	if (is_controlled)
		return;

	/**
	 * During being controlled by hmi-controller, any input event is not
	 * notified. So when control ends with touch up, it invokes launcher
	 * if up event happens on a launcher surface.
	 *
	 */
	if (launcher_button(id_surface, &pCtx->hmi_setting->launcher_list)) {
		pCtx->is_home_on = 0;
		ivi_hmi_controller_home(hmi_ctrl, IVI_HMI_CONTROLLER_HOME_OFF);
	}
}

static const struct ivi_hmi_controller_listener hmi_controller_listener = {
	ivi_hmi_controller_workspace_end_control
};

/**
 * Registration of interfaces
 */
static void
registry_handle_global(void *data, struct wl_registry *registry, uint32_t name,
		       const char *interface, uint32_t version)
{
	struct wlContextCommon *p_wlCtx = (struct wlContextCommon*)data;

	if (!strcmp(interface, "wl_compositor")) {
		p_wlCtx->wlCompositor =
			wl_registry_bind(registry, name,
					 &wl_compositor_interface, 1);
	} else if (!strcmp(interface, "wl_shm")) {
		p_wlCtx->wlShm =
			wl_registry_bind(registry, name, &wl_shm_interface, 1);
		wl_shm_add_listener(p_wlCtx->wlShm, &shm_listenter, p_wlCtx);
	} else if (!strcmp(interface, "wl_seat")) {
		/* XXX: should be handling multiple wl_seats */
		if (p_wlCtx->wlSeat)
			return;

		p_wlCtx->wlSeat =
			wl_registry_bind(registry, name, &wl_seat_interface, 1);
		wl_seat_add_listener(p_wlCtx->wlSeat, &seat_Listener, data);
	} else if (!strcmp(interface, "ivi_application")) {
		p_wlCtx->iviApplication =
			wl_registry_bind(registry, name,
					 &ivi_application_interface, 1);
	} else if (!strcmp(interface, "ivi_hmi_controller")) {
		p_wlCtx->hmiCtrl =
			wl_registry_bind(registry, name,
					 &ivi_hmi_controller_interface, 1);

		ivi_hmi_controller_add_listener(p_wlCtx->hmiCtrl,
				&hmi_controller_listener, p_wlCtx);
	} else if (!strcmp(interface, "wl_output")) {
		p_wlCtx->hmi_setting->screen_num++;
	}
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
			      uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};

static void
frame_listener_func(void *data, struct wl_callback *callback, uint32_t time)
{
	if (callback)
		wl_callback_destroy(callback);
}

static const struct wl_callback_listener frame_listener = {
	frame_listener_func
};

/*
 * The following correspondences between file names and cursors was copied
 * from: https://bugs.kde.org/attachment.cgi?id=67313
 */
static const char *bottom_left_corners[] = {
	"bottom_left_corner",
	"sw-resize",
	"size_bdiag"
};

static const char *bottom_right_corners[] = {
	"bottom_right_corner",
	"se-resize",
	"size_fdiag"
};

static const char *bottom_sides[] = {
	"bottom_side",
	"s-resize",
	"size_ver"
};

static const char *grabbings[] = {
	"grabbing",
	"closedhand",
	"208530c400c041818281048008011002"
};

static const char *left_ptrs[] = {
	"left_ptr",
	"default",
	"top_left_arrow",
	"left-arrow"
};

static const char *left_sides[] = {
	"left_side",
	"w-resize",
	"size_hor"
};

static const char *right_sides[] = {
	"right_side",
	"e-resize",
	"size_hor"
};

static const char *top_left_corners[] = {
	"top_left_corner",
	"nw-resize",
	"size_fdiag"
};

static const char *top_right_corners[] = {
	"top_right_corner",
	"ne-resize",
	"size_bdiag"
};

static const char *top_sides[] = {
	"top_side",
	"n-resize",
	"size_ver"
};

static const char *xterms[] = {
	"xterm",
	"ibeam",
	"text"
};

static const char *hand1s[] = {
	"hand1",
	"pointer",
	"pointing_hand",
	"e29285e634086352946a0e7090d73106"
};

static const char *watches[] = {
	"watch",
	"wait",
	"0426c94ea35c87780ff01dc239897213"
};

struct cursor_alternatives {
	const char **names;
	size_t count;
};

static const struct cursor_alternatives cursors[] = {
	{ bottom_left_corners, ARRAY_LENGTH(bottom_left_corners) },
	{ bottom_right_corners, ARRAY_LENGTH(bottom_right_corners) },
	{ bottom_sides, ARRAY_LENGTH(bottom_sides) },
	{ grabbings, ARRAY_LENGTH(grabbings) },
	{ left_ptrs, ARRAY_LENGTH(left_ptrs) },
	{ left_sides, ARRAY_LENGTH(left_sides) },
	{ right_sides, ARRAY_LENGTH(right_sides) },
	{ top_left_corners, ARRAY_LENGTH(top_left_corners) },
	{ top_right_corners, ARRAY_LENGTH(top_right_corners) },
	{ top_sides, ARRAY_LENGTH(top_sides) },
	{ xterms, ARRAY_LENGTH(xterms) },
	{ hand1s, ARRAY_LENGTH(hand1s) },
	{ watches, ARRAY_LENGTH(watches) },
};

static void
create_cursors(struct wlContextCommon *cmm)
{
	uint32_t i = 0;
	uint32_t j = 0;
	struct wl_cursor *cursor = NULL;
	char *cursor_theme = cmm->hmi_setting->cursor_theme;
	int32_t cursor_size = cmm->hmi_setting->cursor_size;

	cmm->cursor_theme = wl_cursor_theme_load(cursor_theme, cursor_size,
						 cmm->wlShm);

	cmm->cursors =
		xzalloc(ARRAY_LENGTH(cursors) * sizeof(cmm->cursors[0]));

	for (i = 0; i < ARRAY_LENGTH(cursors); i++) {
		cursor = NULL;

		for (j = 0; !cursor && j < cursors[i].count; ++j) {
			cursor = wl_cursor_theme_get_cursor(
				cmm->cursor_theme, cursors[i].names[j]);
		}

		if (!cursor) {
			fprintf(stderr, "could not load cursor '%s'\n",
					cursors[i].names[0]);
		}

		cmm->cursors[i] = cursor;
	}
}

static void
destroy_cursors(struct wlContextCommon *cmm)
{
	if (cmm->cursor_theme)
		wl_cursor_theme_destroy(cmm->cursor_theme);

	free(cmm->cursors);
}

/**
 * Internal method to prepare parts of UI
 */
static void
createShmBuffer(struct wlContextStruct *p_wlCtx)
{
	struct wl_shm_pool *pool;

	int fd = -1;
	int size = 0;
	int width = 0;
	int height = 0;
	int stride = 0;

	width  = cairo_image_surface_get_width(p_wlCtx->ctx_image);
	height = cairo_image_surface_get_height(p_wlCtx->ctx_image);
	stride = cairo_image_surface_get_stride(p_wlCtx->ctx_image);

	size = stride * height;

	fd = os_create_anonymous_file(size);
	if (fd < 0) {
		fprintf(stderr, "creating a buffer file for %d B failed: %s\n",
			size, strerror(errno));
		return ;
	}

	p_wlCtx->data =
		mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (MAP_FAILED == p_wlCtx->data) {
		fprintf(stderr, "mmap failed: %s\n", strerror(errno));
		close(fd);
		return;
	}

	pool = wl_shm_create_pool(p_wlCtx->cmm->wlShm, fd, size);
	p_wlCtx->wlBuffer = wl_shm_pool_create_buffer(pool, 0,
						      width,
						      height,
						      stride,
						      WL_SHM_FORMAT_ARGB8888);

	if (NULL == p_wlCtx->wlBuffer) {
		fprintf(stderr, "wl_shm_create_buffer failed: %s\n",
			strerror(errno));
		close(fd);
		return;
	}

	wl_shm_pool_destroy(pool);
	close(fd);
}

static void
destroyWLContextCommon(struct wlContextCommon *p_wlCtx)
{
	destroy_cursors(p_wlCtx);

	if (p_wlCtx->pointer_surface)
		wl_surface_destroy(p_wlCtx->pointer_surface);

	if (p_wlCtx->wlCompositor)
		wl_compositor_destroy(p_wlCtx->wlCompositor);
}

static void
destroyWLContextStruct(struct wlContextStruct *p_wlCtx)
{
	if (p_wlCtx->wlSurface)
		wl_surface_destroy(p_wlCtx->wlSurface);

	if (p_wlCtx->ctx_image) {
		cairo_surface_destroy(p_wlCtx->ctx_image);
		p_wlCtx->ctx_image = NULL;
	}
}

static int
createSurface(struct wlContextStruct *p_wlCtx)
{
	p_wlCtx->wlSurface =
		wl_compositor_create_surface(p_wlCtx->cmm->wlCompositor);
	if (NULL == p_wlCtx->wlSurface) {
		printf("Error: wl_compositor_create_surface failed.\n");
		destroyWLContextCommon(p_wlCtx->cmm);
		abort();
	}

	return 0;
}

static void
drawImage(struct wlContextStruct *p_wlCtx)
{
	struct wl_callback *callback;

	int width = 0;
	int height = 0;
	int stride = 0;
	void *data = NULL;

	width = cairo_image_surface_get_width(p_wlCtx->ctx_image);
	height = cairo_image_surface_get_height(p_wlCtx->ctx_image);
	stride = cairo_image_surface_get_stride(p_wlCtx->ctx_image);
	data = cairo_image_surface_get_data(p_wlCtx->ctx_image);

	memcpy(p_wlCtx->data, data, stride * height);

	wl_surface_attach(p_wlCtx->wlSurface, p_wlCtx->wlBuffer, 0, 0);
	wl_surface_damage(p_wlCtx->wlSurface, 0, 0, width, height);

	callback = wl_surface_frame(p_wlCtx->wlSurface);
	wl_callback_add_listener(callback, &frame_listener, NULL);

	wl_surface_commit(p_wlCtx->wlSurface);
}

static void
create_ivisurface(struct wlContextStruct *p_wlCtx,
				  uint32_t id_surface,
				  cairo_surface_t *surface)
{
	struct ivi_surface *ivisurf = NULL;

	p_wlCtx->ctx_image = surface;

	p_wlCtx->id_surface = id_surface;
	wl_list_init(&p_wlCtx->link);
	wl_list_insert(&p_wlCtx->cmm->list_wlContextStruct, &p_wlCtx->link);

	createSurface(p_wlCtx);
	createShmBuffer(p_wlCtx);

	ivisurf = ivi_application_surface_create(p_wlCtx->cmm->iviApplication,
						 id_surface,
						 p_wlCtx->wlSurface);
	if (ivisurf == NULL) {
		fprintf(stderr, "Failed to create ivi_client_surface\n");
		return;
	}

	drawImage(p_wlCtx);
}

static void
create_ivisurfaceFromFile(struct wlContextStruct *p_wlCtx,
			  uint32_t id_surface,
			  const char *imageFile)
{
	cairo_surface_t *surface = load_cairo_surface(imageFile);

	if (NULL == surface) {
		fprintf(stderr, "Failed to load_cairo_surface %s\n", imageFile);
		return;
	}

	create_ivisurface(p_wlCtx, id_surface, surface);
}

static void
set_hex_color(cairo_t *cr, uint32_t color)
{
	cairo_set_source_rgba(cr,
		((color >> 16) & 0xff) / 255.0,
		((color >>  8) & 0xff) / 255.0,
		((color >>  0) & 0xff) / 255.0,
		((color >> 24) & 0xff) / 255.0);
}

static void
create_ivisurfaceFromColor(struct wlContextStruct *p_wlCtx,
			   uint32_t id_surface,
			   uint32_t width, uint32_t height,
			   uint32_t color)
{
	cairo_surface_t *surface = NULL;
	cairo_t *cr = NULL;

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					     width, height);

	cr = cairo_create(surface);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, 0, 0, width, height);
	set_hex_color(cr, color);
	cairo_fill(cr);
	cairo_destroy(cr);

	create_ivisurface(p_wlCtx, id_surface, surface);
}

static void
UI_ready(struct ivi_hmi_controller *controller)
{
	ivi_hmi_controller_UI_ready(controller);
}

/**
 * Internal method to set up UI by using ivi-hmi-controller
 */
static void
create_background(struct wlContextStruct *p_wlCtx, const uint32_t id_surface,
		  const char *imageFile)
{
	create_ivisurfaceFromFile(p_wlCtx, id_surface, imageFile);
}

static void
create_panel(struct wlContextStruct *p_wlCtx, const uint32_t id_surface,
	     const char *imageFile)
{
	create_ivisurfaceFromFile(p_wlCtx, id_surface, imageFile);
}

static void
create_button(struct wlContextStruct *p_wlCtx, const uint32_t id_surface,
	      const char *imageFile, uint32_t number)
{
	create_ivisurfaceFromFile(p_wlCtx, id_surface, imageFile);
}

static void
create_home_button(struct wlContextStruct *p_wlCtx, const uint32_t id_surface,
		   const char *imageFile)
{
	create_ivisurfaceFromFile(p_wlCtx, id_surface, imageFile);
}

static void
create_workspace_background(struct wlContextStruct *p_wlCtx,
			    struct hmi_homescreen_srf *srf)
{
	create_ivisurfaceFromColor(p_wlCtx, srf->id, 1, 1, srf->color);
}

static void
create_launchers(struct wlContextCommon *cmm, struct wl_list *launcher_list)
{
	struct hmi_homescreen_launcher **launchers;
	struct hmi_homescreen_launcher *launcher = NULL;

	int launcher_count = wl_list_length(launcher_list);
	int ii = 0;
	int start = 0;

	if (0 == launcher_count)
		return;

	launchers = xzalloc(launcher_count * sizeof(*launchers));

	wl_list_for_each(launcher, launcher_list, link) {
		launchers[ii] = launcher;
		ii++;
	}

	for (ii = 0; ii < launcher_count; ii++) {
		int jj = 0;

		if (ii != launcher_count - 1 &&
		    launchers[ii]->workspace_id ==
		    launchers[ii + 1]->workspace_id)
			continue;

		for (jj = start; jj <= ii; jj++) {
			struct wlContextStruct *p_wlCtx;

			p_wlCtx = xzalloc(sizeof(*p_wlCtx));
			p_wlCtx->cmm = cmm;
			create_ivisurfaceFromFile(p_wlCtx,
						  launchers[jj]->icon_surface_id,
						  launchers[jj]->icon);
		}

		start = ii + 1;
	}

	free(launchers);
}

/**
 * Internal method to read out weston.ini to get configuration
 */
static struct hmi_homescreen_setting *
hmi_homescreen_setting_create(void)
{
	const char *config_file;
	struct weston_config *config = NULL;
	struct weston_config_section *shellSection = NULL;
	struct hmi_homescreen_setting *setting = xzalloc(sizeof(*setting));
	struct weston_config_section *section = NULL;
	const char *name = NULL;
	uint32_t workspace_layer_id;
	uint32_t icon_surface_id = 0;
	char *filename;

	wl_list_init(&setting->workspace_list);
	wl_list_init(&setting->launcher_list);

	config_file = weston_config_get_name_from_env();
	config = weston_config_parse(config_file);

	shellSection =
		weston_config_get_section(config, "ivi-shell", NULL, NULL);

	weston_config_section_get_string(
		shellSection, "cursor-theme", &setting->cursor_theme, NULL);

	weston_config_section_get_int(
		shellSection, "cursor-size", &setting->cursor_size, 32);

	weston_config_section_get_uint(
		shellSection, "workspace-layer-id", &workspace_layer_id, 3000);

	filename = file_name_with_datadir("background.png");
	weston_config_section_get_string(
		shellSection, "background-image", &setting->background.filePath,
		filename);
	free(filename);

	weston_config_section_get_uint(
		shellSection, "background-id", &setting->background.id, 1001);

	filename = file_name_with_datadir("panel.png");
	weston_config_section_get_string(
		shellSection, "panel-image", &setting->panel.filePath,
		filename);
	free(filename);

	weston_config_section_get_uint(
		shellSection, "panel-id", &setting->panel.id, 1002);

	filename = file_name_with_datadir("tiling.png");
	weston_config_section_get_string(
		shellSection, "tiling-image", &setting->tiling.filePath,
		filename);
	free(filename);

	weston_config_section_get_uint(
		shellSection, "tiling-id", &setting->tiling.id, 1003);

	filename = file_name_with_datadir("sidebyside.png");
	weston_config_section_get_string(
		shellSection, "sidebyside-image", &setting->sidebyside.filePath,
		filename);
	free(filename);

	weston_config_section_get_uint(
		shellSection, "sidebyside-id", &setting->sidebyside.id, 1004);

	filename = file_name_with_datadir("fullscreen.png");
	weston_config_section_get_string(
		shellSection, "fullscreen-image", &setting->fullscreen.filePath,
		filename);
	free(filename);

	weston_config_section_get_uint(
		shellSection, "fullscreen-id", &setting->fullscreen.id, 1005);

	filename = file_name_with_datadir("random.png");
	weston_config_section_get_string(
		shellSection, "random-image", &setting->random.filePath,
		filename);
	free(filename);

	weston_config_section_get_uint(
		shellSection, "random-id", &setting->random.id, 1006);

	filename = file_name_with_datadir("home.png");
	weston_config_section_get_string(
		shellSection, "home-image", &setting->home.filePath,
		filename);
	free(filename);

	weston_config_section_get_uint(
		shellSection, "home-id", &setting->home.id, 1007);

	weston_config_section_get_color(
		shellSection, "workspace-background-color",
		&setting->workspace_background.color, 0x99000000);

	weston_config_section_get_uint(
		shellSection, "workspace-background-id",
		&setting->workspace_background.id, 2001);

	weston_config_section_get_uint(
		shellSection, "surface-id-offset", &setting->surface_id_offset, 10);

	icon_surface_id = workspace_layer_id + 1;

	while (weston_config_next_section(config, &section, &name)) {
		struct hmi_homescreen_launcher *launcher;

		if (strcmp(name, "ivi-launcher") != 0)
			continue;

		launcher = xzalloc(sizeof(*launcher));
		wl_list_init(&launcher->link);

		weston_config_section_get_string(section, "icon",
						 &launcher->icon, NULL);
		weston_config_section_get_string(section, "path",
						 &launcher->path, NULL);
		weston_config_section_get_uint(section, "workspace-id",
					       &launcher->workspace_id, 0);
		weston_config_section_get_uint(section, "icon-id",
					       &launcher->icon_surface_id,
					       icon_surface_id);
		icon_surface_id++;

		wl_list_insert(setting->launcher_list.prev, &launcher->link);
	}

	weston_config_destroy(config);
	return setting;
}

/**
 * Main thread
 *
 * The basic flow are as followed,
 * 1/ read configuration from weston.ini by hmi_homescreen_setting_create
 * 2/ draw png file to surface according to configuration of weston.ini and
 *	set up UI by using ivi-hmi-controller protocol by each create_* method
 */
int main(int argc, char **argv)
{
	struct wlContextCommon wlCtxCommon;
	struct wlContextStruct *wlCtx_BackGround;
	struct wlContextStruct *wlCtx_Panel;
	struct wlContextStruct wlCtx_Button_1;
	struct wlContextStruct wlCtx_Button_2;
	struct wlContextStruct wlCtx_Button_3;
	struct wlContextStruct wlCtx_Button_4;
	struct wlContextStruct wlCtx_HomeButton;
	struct wlContextStruct wlCtx_WorkSpaceBackGround;
	struct wl_list launcher_wlCtxList;
	int ret = 0;
	struct hmi_homescreen_setting *hmi_setting;
	struct wlContextStruct *pWlCtxSt = NULL;
	int i = 0;

	hmi_setting = hmi_homescreen_setting_create();

	memset(&wlCtxCommon, 0x00, sizeof(wlCtxCommon));
	memset(&wlCtx_Button_1,   0x00, sizeof(wlCtx_Button_1));
	memset(&wlCtx_Button_2,   0x00, sizeof(wlCtx_Button_2));
	memset(&wlCtx_Button_3,   0x00, sizeof(wlCtx_Button_3));
	memset(&wlCtx_Button_4,   0x00, sizeof(wlCtx_Button_4));
	memset(&wlCtx_HomeButton, 0x00, sizeof(wlCtx_HomeButton));
	memset(&wlCtx_WorkSpaceBackGround, 0x00,
	       sizeof(wlCtx_WorkSpaceBackGround));
	wl_list_init(&launcher_wlCtxList);
	wl_list_init(&wlCtxCommon.list_wlContextStruct);

	wlCtxCommon.hmi_setting = hmi_setting;

	wlCtxCommon.wlDisplay = wl_display_connect(NULL);
	if (NULL == wlCtxCommon.wlDisplay) {
		printf("Error: wl_display_connect failed.\n");
		return -1;
	}

	/* get wl_registry */
	wlCtxCommon.formats = 0;
	wlCtxCommon.wlRegistry = wl_display_get_registry(wlCtxCommon.wlDisplay);
	wl_registry_add_listener(wlCtxCommon.wlRegistry,
				 &registry_listener, &wlCtxCommon);
	wl_display_roundtrip(wlCtxCommon.wlDisplay);

	if (wlCtxCommon.wlShm == NULL) {
		fprintf(stderr, "No wl_shm global\n");
		exit(1);
	}

	wl_display_roundtrip(wlCtxCommon.wlDisplay);

	if (!(wlCtxCommon.formats & (1 << WL_SHM_FORMAT_XRGB8888))) {
		fprintf(stderr, "WL_SHM_FORMAT_XRGB32 not available\n");
		exit(1);
	}

	wlCtx_BackGround = xzalloc(hmi_setting->screen_num * sizeof(struct wlContextStruct));
	wlCtx_Panel= xzalloc(hmi_setting->screen_num * sizeof(struct wlContextStruct));

	if (wlCtxCommon.hmi_setting->cursor_theme) {
		create_cursors(&wlCtxCommon);

		wlCtxCommon.pointer_surface =
			wl_compositor_create_surface(wlCtxCommon.wlCompositor);

		wlCtxCommon.current_cursor = CURSOR_LEFT_PTR;
	}

	wlCtx_Button_1.cmm   = &wlCtxCommon;
	wlCtx_Button_2.cmm   = &wlCtxCommon;
	wlCtx_Button_3.cmm   = &wlCtxCommon;
	wlCtx_Button_4.cmm   = &wlCtxCommon;
	wlCtx_HomeButton.cmm = &wlCtxCommon;
	wlCtx_WorkSpaceBackGround.cmm = &wlCtxCommon;

	/* create desktop widgets */
	for (i = 0; i < hmi_setting->screen_num; i++) {
		wlCtx_BackGround[i].cmm = &wlCtxCommon;
		create_background(&wlCtx_BackGround[i],
				  hmi_setting->background.id +
					(i * hmi_setting->surface_id_offset),
				  hmi_setting->background.filePath);

		wlCtx_Panel[i].cmm = &wlCtxCommon;
		create_panel(&wlCtx_Panel[i],
			     hmi_setting->panel.id + (i * hmi_setting->surface_id_offset),
			     hmi_setting->panel.filePath);
	}

	create_button(&wlCtx_Button_1, hmi_setting->tiling.id,
		      hmi_setting->tiling.filePath, 0);

	create_button(&wlCtx_Button_2, hmi_setting->sidebyside.id,
		      hmi_setting->sidebyside.filePath, 1);

	create_button(&wlCtx_Button_3, hmi_setting->fullscreen.id,
		      hmi_setting->fullscreen.filePath, 2);

	create_button(&wlCtx_Button_4, hmi_setting->random.id,
		      hmi_setting->random.filePath, 3);

	create_workspace_background(&wlCtx_WorkSpaceBackGround,
				    &hmi_setting->workspace_background);

	create_launchers(&wlCtxCommon, &hmi_setting->launcher_list);

	create_home_button(&wlCtx_HomeButton, hmi_setting->home.id,
			   hmi_setting->home.filePath);

	UI_ready(wlCtxCommon.hmiCtrl);

	while (ret != -1)
		ret = wl_display_dispatch(wlCtxCommon.wlDisplay);

	wl_list_for_each(pWlCtxSt, &wlCtxCommon.list_wlContextStruct, link) {
		destroyWLContextStruct(pWlCtxSt);
	}

	free(wlCtx_BackGround);
	free(wlCtx_Panel);

	destroyWLContextCommon(&wlCtxCommon);

	return 0;
}
