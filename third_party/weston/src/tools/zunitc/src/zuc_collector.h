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

#ifndef ZUC_COLLECTOR_H
#define ZUC_COLLECTOR_H

struct zuc_event_listener;
struct zuc_test;

/**
 * Creates a new instance of an event collector that will attach events
 * to the current test directly or via connection from child to parent.
 *
 * @param pipe_fd pointer to the file descriptor to use for communication if
 * needed. If the value is -1 the events will be attached directly to the
 * current test. Otherwise events will be passed back via IPC over this
 * pipe with the expectation that the payload will be handled in the parent
 * process via zuc_process_message().
 * @return a new collector instance.
 * @see zuc_process_message()
 */
struct zuc_event_listener *
zuc_collector_create(int *pipe_fd);

/**
 * Reads events from the given pipe and processes them.
 *
 * @param test the currently active test to attach events for.
 * @param pipe_fd the file descriptor of the pipe to read from.
 * @return a positive value if a message was received, 0 if the end has
 * been reached and -1 if an error has occurred.
 */
int
zuc_process_message(struct zuc_test *test, int pipe_fd);

#endif /* ZUC_COLLECTOR_H */
