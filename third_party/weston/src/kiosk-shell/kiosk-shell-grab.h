/*
 * Copyright © 2020 Collabora, Ltd.
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

#ifndef WESTON_KIOSK_SHELL_GRAB_H
#define WESTON_KIOSK_SHELL_GRAB_H

#include "kiosk-shell.h"

enum kiosk_shell_grab_result {
	KIOSK_SHELL_GRAB_RESULT_OK,
	KIOSK_SHELL_GRAB_RESULT_IGNORED,
	KIOSK_SHELL_GRAB_RESULT_ERROR,
};

enum kiosk_shell_grab_result
kiosk_shell_grab_start_for_pointer_move(struct kiosk_shell_surface *shsurf,
					struct weston_pointer *pointer);

enum kiosk_shell_grab_result
kiosk_shell_grab_start_for_touch_move(struct kiosk_shell_surface *shsurf,
				      struct weston_touch *touch);

#endif /* WESTON_KIOSK_SHELL_GRAB_H */
