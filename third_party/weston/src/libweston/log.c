/*
 * Copyright © 2012 Martin Minarik
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <wayland-util.h>

#include "shared/timespec-util.h"
#include <libweston/libweston.h>
#include "weston-log-internal.h"

/**
 * \defgroup wlog weston-logging
 */

static int
default_log_handler(const char *fmt, va_list ap);

/** Needs to be set, defaults to default_log_handler
 *
 * \ingroup wlog
 */
static log_func_t log_handler = default_log_handler;

/** Needs to be set, defaults to default_log_handler
 *
 * \ingroup wlog
 */
static log_func_t log_continue_handler = default_log_handler;

/** Sentinel log message handler
 *
 * This function is used as the default handler for log messages. It
 * exists only to issue a noisy reminder to the user that a real handler
 * must be installed prior to issuing logging calls. The process is
 * immediately aborted after the reminder is printed.
 *
 * \param fmt The format string. Ignored.
 * \param ap The variadic argument list. Ignored.
 *
 * \ingroup wlog
 */
static int
default_log_handler(const char *fmt, va_list ap)
{
	fprintf(stderr, "weston_log_set_handler() must be called before using of weston_log().\n");
	abort();
}

/** Install the log handler
 *
 * The given functions will be called to output text as passed to the
 * \a weston_log and \a weston_log_continue functions.
 *
 * \param log The log function. This function will be called when
 *            \a weston_log is called, and should begin a new line,
 *            with user defined line headers, if any.
 * \param cont The continue log function. This function will be called
 *             when \a weston_log_continue is called, and should append
 *             its output to the current line, without any header or
 *             other content in between.
 *
 * \ingroup wlog
 */
WL_EXPORT void
weston_log_set_handler(log_func_t log, log_func_t cont)
{
	log_handler = log;
	log_continue_handler = cont;
}

/** weston_vlog calls log_handler
 * \ingroup wlog
 */
WL_EXPORT int
weston_vlog(const char *fmt, va_list ap)
{
	return log_handler(fmt, ap);
}

/** printf() equivalent in weston compositor.
 *
 * \rststar
 * 	.. note::
 *
 * 		Needs :var:`log_handler` to be set-up!
 * \endrststar
 *
 * \ingroup wlog
 */
WL_EXPORT int
weston_log(const char *fmt, ...)
{
	int l;
	va_list argp;

	va_start(argp, fmt);
	l = weston_vlog(fmt, argp);
	va_end(argp);

	return l;
}

/** weston logger with throttling
 *
 * Throttled logger that will suppress a message after a fixed number of
 * prints, and optionally reset the counter reset_ms miliseconds after
 * the first message in a burst.
 *
 * On the first new message printed with this pacer after the timeout
 * expires, a count of suppressed messages will also be printed.
 *
 * Note that the "initialized" member of struct weston_log_pacer must be
 * set to 0 before first call.
 *
 * \param pacer The pacer instance
 * \param max_burst Number of messages to allow before throttling - must
 * not be zero.
 * \param reset_ms Duration from burst start before the count is reset, or
 * zero to never reset.
 * \param fmt The format string
 *
 * \ingroup wlog
 */
WL_EXPORT void
weston_log_paced(struct weston_log_pacer *pacer,
		 unsigned int max_burst,
		 unsigned int reset_ms,
		 const char *fmt, ...)
{
	va_list argp;
	struct timespec now;
	int64_t since_burst_start;
	int64_t suppressed = 0;

	assert(max_burst != 0);

	/* If CLOCK_MONOTONIC fails we silently give up on ever
	 * reseting the timer. */
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
		now.tv_sec = 0;
		now.tv_nsec = 0;
		pacer->burst_start = now;
	}

	if (!pacer->initialized) {
		pacer->initialized = true;
		pacer->burst_start = now;
		pacer->max_burst = max_burst;
		pacer->reset_ms = reset_ms;
	} else {
		assert(pacer->max_burst == max_burst);
		assert(pacer->reset_ms == reset_ms);
	}
	since_burst_start = timespec_sub_to_msec(&now, &pacer->burst_start);

	if (pacer->reset_ms && since_burst_start > pacer->reset_ms) {
		if (pacer->event_count > pacer->max_burst) {
			suppressed = pacer->event_count -
				     pacer->max_burst;
		}
		pacer->event_count = 0;
	}

	if (pacer->event_count == 0) {
		pacer->burst_start = now;
		since_burst_start = 0;
	}

	pacer->event_count++;
	if (pacer->event_count > pacer->max_burst)
		return;

	va_start(argp, fmt);
	weston_vlog(fmt, argp);
	va_end(argp);

	if (suppressed) {
		weston_log_continue(STAMP_SPACE "Warning: %" PRId64 " similar "
				    "messages previously suppressed\n",
				    suppressed);
	}

	/* If we're not going to throttle next time, return immediately,
	 * otherwise print a little more information */
	if (pacer->event_count != pacer->max_burst)
		return;

	if (pacer->reset_ms) {
		int64_t next_reset = pacer->reset_ms - since_burst_start;

		weston_log_continue(STAMP_SPACE "Warning: the above message "
				    "will be suppresssed for the next %"
				    PRId64 " ms.\n", next_reset);
	} else {
		weston_log_continue(STAMP_SPACE "Warning: the above message "
				    "will not be printed again.\n");
	}
}

/** weston_vlog_continue calls log_continue_handler
 *
 * \ingroup wlog
 */
WL_EXPORT int
weston_vlog_continue(const char *fmt, va_list argp)
{
	return log_continue_handler(fmt, argp);
}

/** weston_log_continue
 *
 * \ingroup wlog
 */
WL_EXPORT int
weston_log_continue(const char *fmt, ...)
{
	int l;
	va_list argp;

	va_start(argp, fmt);
	l = weston_vlog_continue(fmt, argp);
	va_end(argp);

	return l;
}
