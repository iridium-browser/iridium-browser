/*
 * Copyright © 2012 Collabora, Ltd.
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

#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include "test-runner.h"
#include "wayland-util.h"
#include "wayland-private.h"

#include "test-compositor.h"

extern int fd_leak_check_enabled;

TEST(empty)
{
}

TEST(exit_success)
{
	exit(EXIT_SUCCESS);
}

FAIL_TEST(exit_failure)
{
	exit(EXIT_FAILURE);
}

FAIL_TEST(fail_abort)
{
	test_disable_coredumps();
	abort();
}

FAIL_TEST(fail_wl_abort)
{
	test_disable_coredumps();
	wl_abort("Abort the program\n");
}

FAIL_TEST(fail_kill)
{
	kill(getpid(), SIGTERM);
}

FAIL_TEST(fail_segv)
{
	char * volatile *null = 0;

	test_disable_coredumps();
	*null = "Goodbye, world";
}

FAIL_TEST(sanity_assert)
{
	test_disable_coredumps();
	/* must fail */
	assert(0);
}

FAIL_TEST(sanity_fd_leak)
{
	int fd[2];

	assert(fd_leak_check_enabled);

	/* leak 2 file descriptors */
	if (pipe(fd) < 0)
		exit(EXIT_SUCCESS); /* failed to fail */

	test_disable_coredumps();
}

FAIL_TEST(sanity_fd_leak_exec)
{
	int fd[2];
	int nr_fds = count_open_fds();

	/* leak 2 file descriptors */
	if (pipe(fd) < 0)
		exit(EXIT_SUCCESS); /* failed to fail */

	test_disable_coredumps();
	exec_fd_leak_check(nr_fds);
}

TEST(sanity_fd_exec)
{
	int fd[2];
	int nr_fds = count_open_fds();

	/* create 2 file descriptors, that should pass over exec */
	assert(pipe(fd) >= 0);

	exec_fd_leak_check(nr_fds + 2);
}

static void
sanity_fd_no_leak(void)
{
	int fd[2];

	assert(fd_leak_check_enabled);

	/* leak 2 file descriptors */
	if (pipe(fd) < 0)
		exit(EXIT_SUCCESS); /* failed to fail */

	close(fd[0]);
	close(fd[1]);
}

static void
sanity_client_no_leak(void)
{
	struct wl_display *display = wl_display_connect(NULL);
	assert(display);

	wl_display_disconnect(display);
}

TEST(tc_client_no_fd_leaks)
{
	struct display *d = display_create();

	/* Client which does not consume the WAYLAND_DISPLAY socket. */
	client_create_noarg(d, sanity_fd_no_leak);
	display_run(d);

	/* Client which does consume the WAYLAND_DISPLAY socket. */
	client_create_noarg(d, sanity_client_no_leak);
	display_run(d);

	display_destroy(d);
}

FAIL_TEST(tc_client_fd_leaks)
{
	struct display *d = display_create();

	client_create_noarg(d, sanity_fd_leak);
	display_run(d);

	test_disable_coredumps();
	display_destroy(d);
}

FAIL_TEST(tc_client_fd_leaks_exec)
{
	struct display *d = display_create();

	client_create_noarg(d, sanity_fd_leak_exec);
	display_run(d);

	test_disable_coredumps();
	display_destroy(d);
}

FAIL_TEST(timeout_tst)
{
	test_set_timeout(1);
	test_disable_coredumps();
	/* test should reach timeout */
	test_sleep(2);
}

TEST(timeout2_tst)
{
	/* the test should end before reaching timeout,
	 * thus it should pass */
	test_set_timeout(1);
	/* 200 000 microsec = 0.2 sec */
	test_usleep(200000);
}

FAIL_TEST(timeout_reset_tst)
{
	test_set_timeout(5);
	test_set_timeout(10);
	test_set_timeout(1);

	test_disable_coredumps();
	/* test should fail on timeout */
	test_sleep(2);
}

TEST(timeout_turnoff)
{
	test_set_timeout(1);
	test_set_timeout(0);

	test_usleep(2);
}

/* test timeouts with test-compositor */
FAIL_TEST(tc_timeout_tst)
{
	struct display *d = display_create();
	client_create_noarg(d, timeout_tst);
	display_run(d);
	test_disable_coredumps();
	display_destroy(d);
}

FAIL_TEST(tc_timeout2_tst)
{
	struct display *d = display_create();
	client_create_noarg(d, timeout_reset_tst);
	display_run(d);
	test_disable_coredumps();
	display_destroy(d);
}

TEST(tc_timeout3_tst)
{
	struct display *d = display_create();

	client_create_noarg(d, timeout2_tst);
	display_run(d);

	client_create_noarg(d, timeout_turnoff);
	display_run(d);

	display_destroy(d);
}
