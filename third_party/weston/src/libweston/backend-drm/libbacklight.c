/*
 * libbacklight - userspace interface to Linux backlight control
 *
 * Copyright © 2012 Intel Corporation
 * Copyright 2010 Red Hat <mjg@redhat.com>
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Matthew Garrett <mjg@redhat.com>
 *    Tiago Vignatti <vignatti@freedesktop.org>
 */

#include "config.h"

#include "libbacklight.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/types.h>
#include <dirent.h>
#include <drm.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>

#include "shared/string-helpers.h"

static long backlight_get(struct backlight *backlight, char *node)
{
	char buffer[100];
	char *path;
	int fd, value;
	long ret;

	if (asprintf(&path, "%s/%s", backlight->path, node) < 0)
		return -ENOMEM;
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		ret = -1;
		goto out;
	}

	ret = read(fd, &buffer, sizeof(buffer));
	if (ret < 1) {
		ret = -1;
		goto out;
	}

	if (!safe_strtoint(buffer, &value)) {
		ret = -1;
		goto out;
	}

	ret = value;

out:
	if (fd >= 0)
		close(fd);
	free(path);
	return ret;
}

long backlight_get_brightness(struct backlight *backlight)
{
	return backlight_get(backlight, "brightness");
}

long backlight_get_max_brightness(struct backlight *backlight)
{
	return backlight_get(backlight, "max_brightness");
}

long backlight_get_actual_brightness(struct backlight *backlight)
{
	return backlight_get(backlight, "actual_brightness");
}

long backlight_set_brightness(struct backlight *backlight, long brightness)
{
	char *path;
	char *buffer = NULL;
	int fd;
	long ret;

	if (asprintf(&path, "%s/%s", backlight->path, "brightness") < 0)
		return -ENOMEM;

	fd = open(path, O_RDWR);
	if (fd < 0) {
		ret = -1;
		goto out;
	}

	ret = read(fd, &buffer, sizeof(buffer));
	if (ret < 1) {
		ret = -1;
		goto out;
	}

	if (asprintf(&buffer, "%ld", brightness) < 0) {
		ret = -1;
		goto out;
	}

	ret = write(fd, buffer, strlen(buffer));
	if (ret < 0) {
		ret = -1;
		goto out;
	}

	ret = backlight_get_brightness(backlight);
	backlight->brightness = ret;
out:
	free(buffer);
	free(path);
	if (fd >= 0)
		close(fd);
	return ret;
}

void backlight_destroy(struct backlight *backlight)
{
	if (!backlight)
		return;

	if (backlight->path)
		free(backlight->path);

	free(backlight);
}

struct backlight *backlight_init(struct udev_device *drm_device,
				 uint32_t connector_type)
{
	const char *syspath = NULL;
	char *pci_name = NULL;
	char *chosen_path = NULL;
	char *path = NULL;
	DIR *backlights = NULL;
	struct dirent *entry;
	enum backlight_type type = 0;
	char buffer[100];
	struct backlight *backlight = NULL;
	int ret;

	if (!drm_device)
		return NULL;

	syspath = udev_device_get_syspath(drm_device);
	if (!syspath)
		return NULL;

	if (asprintf(&path, "%s/%s", syspath, "device") < 0)
		return NULL;

	ret = readlink(path, buffer, sizeof(buffer) - 1);
	free(path);
	if (ret < 0)
		return NULL;

	buffer[ret] = '\0';
	pci_name = basename(buffer);

	if (connector_type <= 0)
		return NULL;

	backlights = opendir("/sys/class/backlight");
	if (!backlights)
		return NULL;

	/* Find the "best" backlight for the device. Firmware
	   interfaces are preferred over platform interfaces are
	   preferred over raw interfaces. For raw interfaces we'll
	   check if the device ID in the form of pci match, while
	   for firmware interfaces we require the pci ID to
	   match. It's assumed that platform interfaces always match,
	   since we can't actually associate them with IDs.

	   A further awkwardness is that, while it's theoretically
	   possible for an ACPI interface to include support for
	   changing the backlight of external devices, it's unlikely
	   to ever be done. It's effectively impossible for a platform
	   interface to do so. So if we get asked about anything that
	   isn't LVDS or eDP, we pretty much have to require that the
	   control be supplied via a raw interface */

	while ((entry = readdir(backlights))) {
		char *backlight_path;
		char *parent;
		enum backlight_type entry_type;
		int fd;

		if (entry->d_name[0] == '.')
			continue;

		if (asprintf(&backlight_path, "%s/%s", "/sys/class/backlight",
			     entry->d_name) < 0)
			goto err;

		if (asprintf(&path, "%s/%s", backlight_path, "type") < 0) {
			free(backlight_path);
			goto err;
		}

		fd = open(path, O_RDONLY);

		if (fd < 0)
			goto out;

		ret = read (fd, &buffer, sizeof(buffer));
		close (fd);

		if (ret < 1)
			goto out;

		buffer[ret] = '\0';

		if (!strncmp(buffer, "raw\n", sizeof(buffer)))
			entry_type = BACKLIGHT_RAW;
		else if (!strncmp(buffer, "platform\n", sizeof(buffer)))
			entry_type = BACKLIGHT_PLATFORM;
		else if (!strncmp(buffer, "firmware\n", sizeof(buffer)))
			entry_type = BACKLIGHT_FIRMWARE;
		else
			goto out;

		if (connector_type != DRM_MODE_CONNECTOR_LVDS &&
		    connector_type != DRM_MODE_CONNECTOR_eDP) {
			/* External displays are assumed to require
			   gpu control at the moment */
			if (entry_type != BACKLIGHT_RAW)
				goto out;
		}

		free (path);

		if (asprintf(&path, "%s/%s", backlight_path, "device") < 0)
			goto err;

		ret = readlink(path, buffer, sizeof(buffer) - 1);

		if (ret < 0)
			goto out;

		buffer[ret] = '\0';

		parent = basename(buffer);

		/* Perform matching for raw and firmware backlights -
		   platform backlights have to be assumed to match */
		if (entry_type == BACKLIGHT_RAW ||
		    entry_type == BACKLIGHT_FIRMWARE) {
			if (!(pci_name && !strcmp(pci_name, parent)))
				goto out;
		}

		if (entry_type < type)
			goto out;

		type = entry_type;

		if (chosen_path)
			free(chosen_path);
		chosen_path = strdup(backlight_path);

	out:
		free(backlight_path);
		free(path);
	}

	if (!chosen_path)
		goto err;

	backlight = malloc(sizeof(struct backlight));

	if (!backlight)
		goto err;

	backlight->path = chosen_path;
	backlight->type = type;

	backlight->max_brightness = backlight_get_max_brightness(backlight);
	if (backlight->max_brightness < 0)
		goto err;

	backlight->brightness = backlight_get_actual_brightness(backlight);
	if (backlight->brightness < 0)
		goto err;

	closedir(backlights);
	return backlight;
err:
	closedir(backlights);
	free (chosen_path);
	free (backlight);
	return NULL;
}
