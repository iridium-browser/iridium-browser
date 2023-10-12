/*
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

#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "rdp.h"

static int cached_tm_mday = -1;

void rdp_debug_print(struct weston_log_scope *log_scope, bool cont, char *fmt, ...)
{
	char timestr[128];
	int len_va;
	char *str;

	if (!log_scope || !weston_log_scope_is_enabled(log_scope))
		return;

	va_list ap;
	va_start(ap, fmt);

	if (cont) {
		weston_log_scope_vprintf(log_scope, fmt, ap);
		goto end;
	}

	weston_log_timestamp(timestr, sizeof(timestr), &cached_tm_mday);
	len_va = vasprintf(&str, fmt, ap);
	if (len_va >= 0) {
		weston_log_scope_printf(log_scope, "%s %s",
					timestr, str);
		free(str);
	} else {
		const char *oom = "Out of memory";

		weston_log_scope_printf(log_scope, "%s %s",
					timestr, oom);
	}
end:
	va_end(ap);
}

void
assert_compositor_thread(struct rdp_backend *b)
{
	assert(b->compositor_tid == gettid());
}

void
assert_not_compositor_thread(struct rdp_backend *b)
{
	assert(b->compositor_tid != gettid());
}

bool
rdp_event_loop_add_fd(struct wl_event_loop *loop,
		      int fd, uint32_t mask,
		      wl_event_loop_fd_func_t func,
		      void *data, struct wl_event_source **event_source)
{
	*event_source = wl_event_loop_add_fd(loop, fd, 0, func, data);
	if (!*event_source) {
		weston_log("%s: wl_event_loop_add_fd failed.\n", __func__);
		return false;
	}

	wl_event_source_fd_update(*event_source, mask);
	return true;
}

void
rdp_dispatch_task_to_display_loop(RdpPeerContext *peerCtx,
				  rdp_loop_task_func_t func,
				  struct rdp_loop_task *task)
{
	/* this function is ONLY used to queue the task from FreeRDP thread,
	 * and the task to be processed at wayland display loop thread. */
	assert_not_compositor_thread(peerCtx->rdpBackend);

	task->peerCtx = peerCtx;
	task->func = func;

	pthread_mutex_lock(&peerCtx->loop_task_list_mutex);
	/* this inserts at head */
	wl_list_insert(&peerCtx->loop_task_list, &task->link);
	pthread_mutex_unlock(&peerCtx->loop_task_list_mutex);

	eventfd_write(peerCtx->loop_task_event_source_fd, 1);
}

static int
rdp_dispatch_task(int fd, uint32_t mask, void *arg)
{
	RdpPeerContext *peerCtx = (RdpPeerContext *)arg;
	struct rdp_loop_task *task, *tmp;
	eventfd_t dummy;

	/* this must be called back at wayland display loop thread */
	assert_compositor_thread(peerCtx->rdpBackend);

	eventfd_read(peerCtx->loop_task_event_source_fd, &dummy);

	pthread_mutex_lock(&peerCtx->loop_task_list_mutex);
	/* dequeue the first task which is at last, so use reverse. */
	assert(!wl_list_empty(&peerCtx->loop_task_list));
	wl_list_for_each_reverse_safe(task, tmp, &peerCtx->loop_task_list, link) {
		wl_list_remove(&task->link);
		break;
	}
	pthread_mutex_unlock(&peerCtx->loop_task_list_mutex);

	/* Dispatch and task will be freed by caller. */
	task->func(false, task);

	return 0;
}

bool
rdp_initialize_dispatch_task_event_source(RdpPeerContext *peerCtx)
{
	struct rdp_backend *b = peerCtx->rdpBackend;
	struct wl_event_loop *loop;
	bool ret;

	if (pthread_mutex_init(&peerCtx->loop_task_list_mutex, NULL) == -1) {
		weston_log("%s: pthread_mutex_init failed. %s\n", __func__, strerror(errno));
		goto error_mutex;
	}

	assert(peerCtx->loop_task_event_source_fd == -1);
	peerCtx->loop_task_event_source_fd = eventfd(0, EFD_SEMAPHORE | EFD_CLOEXEC);
	if (peerCtx->loop_task_event_source_fd == -1) {
		weston_log("%s: eventfd(EFD_SEMAPHORE) failed. %s\n", __func__, strerror(errno));
		goto error_event_source_fd;
	}

	assert(wl_list_empty(&peerCtx->loop_task_list));

	loop = wl_display_get_event_loop(b->compositor->wl_display);
	assert(peerCtx->loop_task_event_source == NULL);

	ret = rdp_event_loop_add_fd(loop,
				    peerCtx->loop_task_event_source_fd,
				    WL_EVENT_READABLE, rdp_dispatch_task,
				    peerCtx,
				    &peerCtx->loop_task_event_source);
	if (!ret)
		goto error_event_loop_add_fd;

	return true;

error_event_loop_add_fd:
	close(peerCtx->loop_task_event_source_fd);
	peerCtx->loop_task_event_source_fd = -1;

error_event_source_fd:
	pthread_mutex_destroy(&peerCtx->loop_task_list_mutex);

error_mutex:
	return false;
}

void
rdp_destroy_dispatch_task_event_source(RdpPeerContext *peerCtx)
{
	struct rdp_loop_task *task, *tmp;

	/* This function must be called all virtual channel thread at FreeRDP is terminated,
	 * that ensures no more incoming tasks. */

	if (peerCtx->loop_task_event_source) {
		wl_event_source_remove(peerCtx->loop_task_event_source);
		peerCtx->loop_task_event_source = NULL;
	}

	wl_list_for_each_reverse_safe(task, tmp, &peerCtx->loop_task_list, link) {
		wl_list_remove(&task->link);
		/* inform caller task is not really scheduled prior to context destruction,
		 * inform them to clean them up. */
		task->func(true /* freeOnly */, task);
	}
	assert(wl_list_empty(&peerCtx->loop_task_list));

	if (peerCtx->loop_task_event_source_fd != -1) {
		close(peerCtx->loop_task_event_source_fd);
		peerCtx->loop_task_event_source_fd = -1;
	}

	pthread_mutex_destroy(&peerCtx->loop_task_list_mutex);
}

/* This is a little tricky - it makes sure there's always at least
 * one spare byte in the array in case the caller needs to add a
 * null terminator to it. We can't just null terminate the array
 * here, because some callers won't want that - and some won't
 * like having an odd number of bytes.
 */
int
rdp_wl_array_read_fd(struct wl_array *array, int fd)
{
	int len, size;
	char *data;

	/* Make sure we have at least 1024 bytes of space left */
	if (array->alloc - array->size < 1024) {
		if (!wl_array_add(array, 1024)) {
			errno = ENOMEM;
			return -1;
		}
		array->size -= 1024;
	}
	data = (char *)array->data + array->size;
	/* Leave one char in case the caller needs space for a
	 * null terminator */
	size = array->alloc - array->size - 1;
	do {
		len = read(fd, data, size);
	} while (len == -1 && errno == EINTR);

	if (len == -1)
		return -1;

	array->size += len;

	return len;
}
