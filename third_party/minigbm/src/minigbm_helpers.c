/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <amdgpu_drm.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <radeon_drm.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "minigbm_helpers.h"
#include "util.h"

/* These are set in stone. from drm_pciids.h */
static unsigned int radeon_igp_ids[] = {
	0x1304, 0x1305, 0x1306, 0x1307, 0x1309, 0x130A, 0x130B, 0x130C, 0x130D, 0x130E, 0x130F,
	0x1310, 0x1311, 0x1312, 0x1313, 0x1315, 0x1316, 0x1317, 0x1318, 0x131B, 0x131C, 0x131D,
	0x4136, 0x4137, 0x4237, 0x4336, 0x4337, 0x4437, 0x5834, 0x5835, 0x5954, 0x5955, 0x5974,
	0x5975, 0x5a41, 0x5a42, 0x5a61, 0x5a62, 0x7834, 0x7835, 0x791e, 0x791f, 0x793f, 0x7941,
	0x7942, 0x796c, 0x796d, 0x796e, 0x796f, 0x9610, 0x9611, 0x9612, 0x9613, 0x9614, 0x9615,
	0x9616, 0x9640, 0x9641, 0x9642, 0x9643, 0x9644, 0x9645, 0x9647, 0x9648, 0x9649, 0x964a,
	0x964b, 0x964c, 0x964e, 0x964f, 0x9710, 0x9711, 0x9712, 0x9713, 0x9714, 0x9715, 0x9802,
	0x9803, 0x9804, 0x9805, 0x9806, 0x9807, 0x9808, 0x9809, 0x980A, 0x9830, 0x9831, 0x9832,
	0x9833, 0x9834, 0x9835, 0x9836, 0x9837, 0x9838, 0x9839, 0x983a, 0x983b, 0x983c, 0x983d,
	0x983e, 0x983f, 0x9850, 0x9851, 0x9852, 0x9853, 0x9854, 0x9855, 0x9856, 0x9857, 0x9858,
	0x9859, 0x985A, 0x985B, 0x985C, 0x985D, 0x985E, 0x985F, 0x9900, 0x9901, 0x9903, 0x9904,
	0x9905, 0x9906, 0x9907, 0x9908, 0x9909, 0x990A, 0x990B, 0x990C, 0x990D, 0x990E, 0x990F,
	0x9910, 0x9913, 0x9917, 0x9918, 0x9919, 0x9990, 0x9991, 0x9992, 0x9993, 0x9994, 0x9995,
	0x9996, 0x9997, 0x9998, 0x9999, 0x999A, 0x999B, 0x999C, 0x999D, 0x99A0, 0x99A2, 0x99A4
};

static int dri_node_num(const char *dri_node)
{
	long num;
	ssize_t l = strlen(dri_node);

	while (l > 0 && isdigit(dri_node[l - 1]))
		l--;
	num = strtol(dri_node + l, NULL, 10);
	/* Normalize renderDX nodes with cardX nodes. */
	if (num >= 128)
		num -= 128;
	return num;
}

static int fd_node_num(int fd)
{
	char fd_path[64];
	char dri_node[256];
	ssize_t dri_node_size;

	snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd);

	dri_node_size = readlink(fd_path, dri_node, sizeof(dri_node));
	if (dri_node_size < 0)
		return -errno;
	dri_node[dri_node_size] = '\0';
	return dri_node_num(dri_node);
}

#if 0
static int
nouveau_getparam(int fd, uint64_t param, uint64_t *value)
{
   struct drm_nouveau_getparam getparam = { .param = param, .value = 0 };
   int ret = drmCommandWriteRead(fd, DRM_NOUVEAU_GETPARAM, &getparam, sizeof(getparam));
   *value = getparam.value;
   return ret;
}
#endif

/*
 * One would wish we could read .driver_features from DRM driver.
 */
static int detect_device_info(unsigned int detect_flags, int fd, struct gbm_device_info *info)
{
	drmVersionPtr version;
	drmModeResPtr resources;

	info->dev_type_flags = 0;

	version = drmGetVersion(fd);

	if (!version)
		return -EINVAL;

	resources = drmModeGetResources(fd);
	if (resources) {
		info->connectors = (unsigned int)(resources->count_connectors);
		if (resources->count_connectors)
			info->dev_type_flags |= GBM_DEV_TYPE_FLAG_DISPLAY;
		if (detect_flags & GBM_DETECT_FLAG_CONNECTED) {
			int c;
			for (c = 0; c < resources->count_connectors; c++) {
				drmModeConnectorPtr conn =
				    drmModeGetConnector(fd, resources->connectors[c]);
				if (!conn)
					continue;
				if (conn->connection == DRM_MODE_CONNECTED)
					info->connected++;
				if (conn->connector_type == DRM_MODE_CONNECTOR_eDP ||
				    conn->connector_type == DRM_MODE_CONNECTOR_LVDS ||
				    conn->connector_type == DRM_MODE_CONNECTOR_DSI ||
				    conn->connector_type == DRM_MODE_CONNECTOR_DPI)
					info->dev_type_flags |= GBM_DEV_TYPE_FLAG_INTERNAL_LCD;
				drmModeFreeConnector(conn);
			}
		}
		drmModeFreeResources(resources);
	}

	if (strncmp("i915", version->name, version->name_len) == 0) {
		/*
		 * Detect Intel dGPU here when special getparam ioctl is added.
		 */
		info->dev_type_flags |= GBM_DEV_TYPE_FLAG_DISPLAY | GBM_DEV_TYPE_FLAG_3D;
	} else if (strncmp("amdgpu", version->name, version->name_len) == 0) {
		struct drm_amdgpu_info request = { 0 };
		struct drm_amdgpu_info_device dev_info = { 0 };
		int ret;

		info->dev_type_flags = GBM_DEV_TYPE_FLAG_DISPLAY | GBM_DEV_TYPE_FLAG_3D;
		request.return_pointer = (uintptr_t)&dev_info;
		request.return_size = sizeof(dev_info);
		request.query = AMDGPU_INFO_DEV_INFO;

		ret =
		    drmCommandWrite(fd, DRM_AMDGPU_INFO, &request, sizeof(struct drm_amdgpu_info));

		if (ret != 0)
			goto done;
		if (!(dev_info.ids_flags & AMDGPU_IDS_FLAGS_FUSION))
			info->dev_type_flags |= GBM_DEV_TYPE_FLAG_DISCRETE;

	} else if (strncmp("radeon", version->name, version->name_len) == 0) {
		struct drm_radeon_info radinfo = { 0, 0, 0 };
		int ret;
		uint32_t value = 0;
		size_t d;

		info->dev_type_flags |=
		    GBM_DEV_TYPE_FLAG_DISPLAY | GBM_DEV_TYPE_FLAG_3D | GBM_DEV_TYPE_FLAG_DISCRETE;
		radinfo.request = RADEON_INFO_DEVICE_ID;
		radinfo.value = (uintptr_t)&value;
		ret = drmCommandWriteRead(fd, DRM_RADEON_INFO, &radinfo,
					  sizeof(struct drm_radeon_info));
		if (ret != 0)
			goto done;

		for (d = 0; d < (sizeof(radeon_igp_ids) / sizeof(radeon_igp_ids[0])); d++) {
			if (value == radeon_igp_ids[d]) {
				info->dev_type_flags &= ~GBM_DEV_TYPE_FLAG_DISCRETE;
				break;
			}
		}

	} else if (strncmp("nvidia", version->name, version->name_len) == 0) {
		info->dev_type_flags |=
		    GBM_DEV_TYPE_FLAG_DISPLAY | GBM_DEV_TYPE_FLAG_3D | GBM_DEV_TYPE_FLAG_DISCRETE;
	} else if (strncmp("nouveau", version->name, version->name_len) == 0) {
		info->dev_type_flags |=
		    GBM_DEV_TYPE_FLAG_DISPLAY | GBM_DEV_TYPE_FLAG_3D | GBM_DEV_TYPE_FLAG_DISCRETE;
	} else if (strncmp("msm", version->name, version->name_len) == 0) {
		info->dev_type_flags |=
		    GBM_DEV_TYPE_FLAG_DISPLAY | GBM_DEV_TYPE_FLAG_3D | GBM_DEV_TYPE_FLAG_ARMSOC;
	} else if (strncmp("armada", version->name, version->name_len) == 0) {
		info->dev_type_flags |= GBM_DEV_TYPE_FLAG_DISPLAY | GBM_DEV_TYPE_FLAG_ARMSOC;
	} else if (strncmp("exynos", version->name, version->name_len) == 0) {
		info->dev_type_flags |= GBM_DEV_TYPE_FLAG_DISPLAY | GBM_DEV_TYPE_FLAG_ARMSOC;
	} else if (strncmp("mediatek", version->name, version->name_len) == 0) {
		info->dev_type_flags |= GBM_DEV_TYPE_FLAG_DISPLAY | GBM_DEV_TYPE_FLAG_ARMSOC;
	} else if (strncmp("rockchip", version->name, version->name_len) == 0) {
		info->dev_type_flags |= GBM_DEV_TYPE_FLAG_DISPLAY | GBM_DEV_TYPE_FLAG_ARMSOC;
	} else if (strncmp("omapdrm", version->name, version->name_len) == 0) {
		info->dev_type_flags |= GBM_DEV_TYPE_FLAG_DISPLAY | GBM_DEV_TYPE_FLAG_ARMSOC;
	} else if (strncmp("vc4", version->name, version->name_len) == 0) {
		info->dev_type_flags |=
		    GBM_DEV_TYPE_FLAG_DISPLAY | GBM_DEV_TYPE_FLAG_3D | GBM_DEV_TYPE_FLAG_ARMSOC;
	} else if (strncmp("etnaviv", version->name, version->name_len) == 0) {
		info->dev_type_flags |= GBM_DEV_TYPE_FLAG_3D | GBM_DEV_TYPE_FLAG_ARMSOC;
	} else if (strncmp("lima", version->name, version->name_len) == 0) {
		info->dev_type_flags |= GBM_DEV_TYPE_FLAG_3D | GBM_DEV_TYPE_FLAG_ARMSOC;
	} else if (strncmp("panfrost", version->name, version->name_len) == 0) {
		info->dev_type_flags |= GBM_DEV_TYPE_FLAG_3D | GBM_DEV_TYPE_FLAG_ARMSOC;
	} else if (strncmp("pvr", version->name, version->name_len) == 0) {
		info->dev_type_flags |= GBM_DEV_TYPE_FLAG_3D | GBM_DEV_TYPE_FLAG_ARMSOC;
	} else if (strncmp("v3d", version->name, version->name_len) == 0) {
		info->dev_type_flags |= GBM_DEV_TYPE_FLAG_3D | GBM_DEV_TYPE_FLAG_ARMSOC;
	} else if (strncmp("vgem", version->name, version->name_len) == 0) {
		info->dev_type_flags |= GBM_DEV_TYPE_FLAG_BLOCKED;
	} else if (strncmp("evdi", version->name, version->name_len) == 0) {
		info->dev_type_flags |=
		    GBM_DEV_TYPE_FLAG_DISPLAY | GBM_DEV_TYPE_FLAG_USB | GBM_DEV_TYPE_FLAG_BLOCKED;
	} else if (strncmp("udl", version->name, version->name_len) == 0) {
		info->dev_type_flags |=
		    GBM_DEV_TYPE_FLAG_DISPLAY | GBM_DEV_TYPE_FLAG_USB | GBM_DEV_TYPE_FLAG_BLOCKED;
	}

done:
	drmFreeVersion(version);
	return 0;
}

PUBLIC int gbm_get_default_device_fd(void)
{
	DIR *dir;
	int ret, fd, dfd = -1;
	char *rendernode_name;
	struct dirent *dir_ent;
	struct gbm_device_info info;

	dir = opendir("/dev/dri");
	if (!dir)
		return -errno;

	fd = -1;
	while ((dir_ent = readdir(dir))) {
		if (dir_ent->d_type != DT_CHR)
			continue;

		if (strncmp(dir_ent->d_name, "renderD", 7))
			continue;

		ret = asprintf(&rendernode_name, "/dev/dri/%s", dir_ent->d_name);
		if (ret < 0)
			continue;

		fd = open(rendernode_name, O_RDWR | O_CLOEXEC | O_NOCTTY | O_NONBLOCK);
		free(rendernode_name);

		if (fd < 0)
			continue;

		memset(&info, 0, sizeof(info));
		if (detect_device_info(0, fd, &info) < 0) {
			close(fd);
			fd = -1;
			continue;
		}

		if (info.dev_type_flags & GBM_DEV_TYPE_FLAG_BLOCKED) {
			close(fd);
			fd = -1;
			continue;
		}

		if (!(info.dev_type_flags & GBM_DEV_TYPE_FLAG_DISPLAY)) {
			close(fd);
			fd = -1;
			continue;
		}

		if (info.dev_type_flags & GBM_DEV_TYPE_FLAG_DISCRETE) {
			if (dfd < 0)
				dfd = fd;
			else
				close(fd);
			fd = -1;
		} else {
			if (dfd >= 0) {
				close(dfd);
				dfd = -1;
			}
			break;
		}
	}

	closedir(dir);

	if (dfd >= 0)
		return dfd;

	return fd;
}

PUBLIC int gbm_detect_device_info(unsigned int detect_flags, int fd, struct gbm_device_info *info)
{
	if (!info)
		return -EINVAL;
	memset(info, 0, sizeof(*info));
	info->dri_node_num = fd_node_num(fd);
	return detect_device_info(detect_flags, fd, info);
}

PUBLIC int gbm_detect_device_info_path(unsigned int detect_flags, const char *dev_node,
				       struct gbm_device_info *info)
{
	char rendernode_name[64];
	int fd;
	int ret;

	if (!info)
		return -EINVAL;
	memset(info, 0, sizeof(*info));
	info->dri_node_num = dri_node_num(dev_node);

	snprintf(rendernode_name, sizeof(rendernode_name), "/dev/dri/renderD%d",
		 info->dri_node_num + 128);
	fd = open(rendernode_name, O_RDWR | O_CLOEXEC | O_NOCTTY | O_NONBLOCK);
	if (fd < 0)
		return -errno;
	ret = detect_device_info(detect_flags, fd, info);
	close(fd);
	return ret;
}
