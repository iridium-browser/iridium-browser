/*
 * Copyright © 2017 Pekka Paalanen <pq@iki.fi>
 * Copyright © 2018 Zodiac Inflight Innovations
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

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <wayland-client.h>

#include "shared/helpers.h"
#include <libweston/zalloc.h>
#include "weston-debug-client-protocol.h"

struct debug_app {
	struct {
		bool help;
		bool list;
		bool bind_all;
		char *output;
		char *outfd;
	} opt;

	int out_fd;
	struct wl_display *dpy;
	struct wl_registry *registry;
	struct weston_debug_v1 *debug_iface;
	struct wl_list stream_list;
};

struct debug_stream {
	struct wl_list link;
	bool should_bind;
	char *name;
	char *desc;
	struct weston_debug_stream_v1 *obj;
};

/**
 * Called either through stream_find in response to an advertisement
 * event (see comment on stream_find) when we have all the information,
 * or directly from option parsing to make a placeholder entry when the
 * stream was explicitly named on the command line to bind to.
 */
static struct debug_stream *
stream_alloc(struct debug_app *app, const char *name, const char *desc)
{
	struct debug_stream *stream;

	stream = zalloc(sizeof *stream);
	if (!stream)
		return NULL;

	stream->name = strdup(name);
	if (!stream->name) {
		free(stream);
		return NULL;
	}

	if (desc) {
		stream->desc = strdup(desc);
		if (!stream->desc) {
			free(stream->name);
			free(stream);
			return NULL;
		}
	}

	stream->should_bind = app->opt.bind_all;
	wl_list_insert(app->stream_list.prev, &stream->link);

	return stream;
}

/**
 * Called in response to a stream advertisement event. If our stream was
 * manually specified on the command line, then it will already have a
 * dummy entry in stream_list: we fill in its description and return.
 * If there's no entry in the list, we make a new one and return that.
 */
static struct debug_stream *
stream_find(struct debug_app *app, const char *name, const char *desc)
{
	struct debug_stream *stream;

	wl_list_for_each(stream, &app->stream_list, link) {
		if (strcmp(stream->name, name) == 0) {
			assert(stream->desc == NULL);
			if (desc)
				stream->desc = strdup(desc);
			return stream;
		}
	}

	return stream_alloc(app, name, desc);
}

static void
stream_destroy(struct debug_stream *stream)
{
	if (stream->obj)
		weston_debug_stream_v1_destroy(stream->obj);

	wl_list_remove(&stream->link);
	free(stream->name);
	free(stream);
}

static void
destroy_streams(struct debug_app *app)
{
	struct debug_stream *stream;
	struct debug_stream *tmp;

	wl_list_for_each_safe(stream, tmp, &app->stream_list, link)
		stream_destroy(stream);
}

static void
debug_advertise(void *data, struct weston_debug_v1 *debug, const char *name,
		const char *desc)
{
	struct debug_app *app = data;
	(void) stream_find(app, name, desc);
}

static const struct weston_debug_v1_listener debug_listener = {
	debug_advertise,
};

static void
global_handler(void *data, struct wl_registry *registry, uint32_t id,
	       const char *interface, uint32_t version)
{
	struct debug_app *app = data;
	uint32_t myver;

	assert(app->registry == registry);

	if (!strcmp(interface, weston_debug_v1_interface.name)) {
		if (app->debug_iface)
			return;

		myver = MIN(1, version);
		app->debug_iface =
			wl_registry_bind(registry, id,
					 &weston_debug_v1_interface, myver);
		weston_debug_v1_add_listener(app->debug_iface, &debug_listener,
					     app);
	}
}

static void
global_remove_handler(void *data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	global_handler,
	global_remove_handler
};

static void
handle_stream_complete(void *data, struct weston_debug_stream_v1 *obj)
{
	struct debug_stream *stream = data;

	assert(stream->obj == obj);

	stream_destroy(stream);
}

static void
handle_stream_failure(void *data, struct weston_debug_stream_v1 *obj,
		      const char *msg)
{
	struct debug_stream *stream = data;

	assert(stream->obj == obj);

	fprintf(stderr, "Debug stream '%s' aborted: %s\n", stream->name, msg);

	stream_destroy(stream);
}

static const struct weston_debug_stream_v1_listener stream_listener = {
	handle_stream_complete,
	handle_stream_failure
};

static void
start_streams(struct debug_app *app)
{
	struct debug_stream *stream;

	wl_list_for_each(stream, &app->stream_list, link) {
		if (!stream->should_bind)
			continue;

		stream->obj = weston_debug_v1_subscribe(app->debug_iface,
							stream->name,
							app->out_fd);
		weston_debug_stream_v1_add_listener(stream->obj,
						    &stream_listener, stream);
	}
}

static void
list_streams(struct debug_app *app)
{
	struct debug_stream *stream;

	fprintf(stderr, "Available debug streams:\n");

	wl_list_for_each(stream, &app->stream_list, link) {
		if (stream->should_bind && stream->desc) {
			fprintf(stderr, "    %s [will bind]\n", stream->name);
			fprintf(stderr, "        %s\n", stream->desc);
		} else if (stream->should_bind) {
			fprintf(stderr, "    %s [wanted but not found]\n",
				stream->name);
		} else {
			fprintf(stderr, "    %s [will not bind]\n",
				stream->name);
			fprintf(stderr, "        %s\n", stream->desc);
		}
	}
}

static int
setup_out_fd(const char *output, const char *outfd)
{
	int fd = -1;
	int flags;

	assert(!(output && outfd));

	if (output) {
		if (strcmp(output, "-") == 0) {
			fd = STDOUT_FILENO;
		} else {
			fd = open(output,
				  O_WRONLY | O_APPEND | O_CREAT, 0644);
			if (fd < 0) {
				fprintf(stderr,
					"Error: opening file '%s' failed: %s\n",
					output, strerror(errno));
			}
			return fd;
		}
	} else if (outfd) {
		fd = atoi(outfd);
	} else {
		fd = STDOUT_FILENO;
	}

	flags = fcntl(fd, F_GETFL);
	if (flags == -1) {
		fprintf(stderr,
			"Error: cannot use file descriptor %d: %s\n", fd,
			strerror(errno));
		return -1;
	}

	if ((flags & O_ACCMODE) != O_WRONLY &&
	    (flags & O_ACCMODE) != O_RDWR) {
		fprintf(stderr,
			"Error: file descriptor %d is not writable.\n", fd);
		return -1;
	}

	return fd;
}

static void
print_help(void)
{
	fprintf(stderr,
		"Usage: weston-debug [options] [names]\n"
		"Where options may be:\n"
		"  -h, --help\n"
		"     This help text, and exit with success.\n"
		"  -l, --list\n"
		"     Print a list of available debug streams to stderr.\n"
		"  -a, --all-streams\n"
		"     Bind to all available streams.\n"
		"  -o FILE, --output FILE\n"
		"     Direct output to file named FILE. Use - for stdout.\n"
		"     Stdout is the default. Mutually exclusive with -f.\n"
		"  -f FD, --outfd FD\n"
		"     Direct output to the file descriptor FD.\n"
		"     Stdout (1) is the default. Mutually exclusive with -o.\n"
		"Names are whatever debug stream names the compositor supports.\n"
		);
}

static int
parse_cmdline(struct debug_app *app, int argc, char **argv)
{
	static const struct option opts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "list", no_argument, NULL, 'l' },
		{ "all-streams", no_argument, NULL, 'a' },
		{ "output", required_argument, NULL, 'o' },
		{ "outfd", required_argument, NULL, 'f' },
		{ 0 }
	};
	static const char optstr[] = "hlao:f:";
	int c;
	bool failed = false;

	while (1) {
		c = getopt_long(argc, argv, optstr, opts, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			app->opt.help = true;
			break;
		case 'l':
			app->opt.list = true;
			break;
		case 'a':
			app->opt.bind_all = true;
			break;
		case 'o':
			free(app->opt.output);
			app->opt.output = strdup(optarg);
			break;
		case 'f':
			free(app->opt.outfd);
			app->opt.outfd = strdup(optarg);
			break;
		case '?':
			failed = true;
			break;
		default:
			fprintf(stderr, "huh? getopt => %c (%d)\n", c, c);
			failed = true;
		}
	}

	if (failed)
		return -1;

	while (optind < argc) {
		struct debug_stream *stream =
			stream_alloc(app, argv[optind++], NULL);
		stream->should_bind = true;
	}

	return 0;
}

int
main(int argc, char **argv)
{
	struct debug_app app = {};
	int ret = 0;

	wl_list_init(&app.stream_list);
	app.out_fd = -1;

	if (parse_cmdline(&app, argc, argv) < 0) {
		ret = 1;
		goto out_parse;
	}

	if (app.opt.help) {
		print_help();
		goto out_parse;
	}

	if (!app.opt.list && !app.opt.bind_all &&
	    wl_list_empty(&app.stream_list)) {
		fprintf(stderr, "Error: no options given.\n\n");
		ret = 1;
		print_help();
		goto out_parse;
	}

	if (app.opt.bind_all && !wl_list_empty(&app.stream_list)) {
		fprintf(stderr, "Error: --all and specific stream names cannot be used simultaneously.\n");
		ret = 1;
		goto out_parse;
	}

	if (app.opt.output && app.opt.outfd) {
		fprintf(stderr, "Error: options --output and --outfd cannot be used simultaneously.\n");
		ret = 1;
		goto out_parse;
	}

	app.out_fd = setup_out_fd(app.opt.output, app.opt.outfd);
	if (app.out_fd < 0) {
		ret = 1;
		goto out_parse;
	}

	app.dpy = wl_display_connect(NULL);
	if (!app.dpy) {
		fprintf(stderr, "Error: Could not connect to Wayland display: %s\n",
			strerror(errno));
		ret = 1;
		goto out_parse;
	}

	app.registry = wl_display_get_registry(app.dpy);
	wl_registry_add_listener(app.registry, &registry_listener, &app);
	wl_display_roundtrip(app.dpy);

	if (!app.debug_iface) {
		ret = 1;
		fprintf(stderr,
			"The Wayland server does not support %s interface.\n",
			weston_debug_v1_interface.name);
		goto out_conn;
	}

	wl_display_roundtrip(app.dpy); /* for weston_debug_v1::advertise */

	if (app.opt.list)
		list_streams(&app);

	start_streams(&app);

	weston_debug_v1_destroy(app.debug_iface);

	while (1) {
		struct debug_stream *stream;
		bool empty = true;

		wl_list_for_each(stream, &app.stream_list, link) {
			if (stream->obj) {
				empty = false;
				break;
			}
		}

		if (empty)
			break;

		if (wl_display_dispatch(app.dpy) < 0) {
			ret = 1;
			break;
		}
	}

out_conn:
	destroy_streams(&app);

	/* Wait for server to close all files */
	wl_display_roundtrip(app.dpy);

	wl_registry_destroy(app.registry);
	wl_display_disconnect(app.dpy);

out_parse:
	if (app.out_fd != -1)
		close(app.out_fd);

	destroy_streams(&app);
	free(app.opt.output);
	free(app.opt.outfd);

	return ret;
}
