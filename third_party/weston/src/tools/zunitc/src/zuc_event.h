/*
 * Copyright © 2015 Samsung Electronics Co., Ltd
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

#ifndef ZUC_EVENT_H
#define ZUC_EVENT_H

#include <stdint.h>

#include "zunitc/zunitc_impl.h"

/**
 *
 */
enum zuc_event_type
{
	ZUC_EVENT_IMMEDIATE,
	ZUC_EVENT_DEFERRED
};

/**
 * Status enum for posted events.
 */
enum zuc_fail_state
{
	ZUC_CHECK_OK, /**< no problem. */
	ZUC_CHECK_SKIP, /**< runtime skip directive encountered. */
	ZUC_CHECK_FAIL, /**< non-fatal check fails. */
	ZUC_CHECK_FATAL, /**< fatal assertion/check fails. */
	ZUC_CHECK_ERROR /**< internal level problem. */
};

/**
 * Record of an event that occurs during testing such as assert macro
 * failures.
 */
struct zuc_event
{
	char *file;
	int32_t line;
	enum zuc_fail_state state;
	enum zuc_check_op op;
	enum zuc_check_valtype valtype;
	intptr_t val1;
	intptr_t val2;
	char *expr1;
	char *expr2;

	struct zuc_event *next;
};

/**
 * Attaches an event to the specified test.
 *
 * @param test the test to attach to.
 * @param event the event to attach.
 * @param event_type of event (immediate or deferred) to attach.
 * @param transferred true if the event has been processed elsewhere and
 * is being transferred for storage, false otherwise.
 */
void
zuc_attach_event(struct zuc_test *test, struct zuc_event *event,
		 enum zuc_event_type event_type, bool transferred);

#endif /* ZUC_EVENT_H */
