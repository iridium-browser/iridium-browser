/*
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef _MINIGBM_HELPERS_H_
#define _MINIGBM_HELPERS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define GBM_DEV_TYPE_FLAG_DISCRETE (1u << 0)	 /* Discrete GPU. Separate chip, dedicated VRAM. */
#define GBM_DEV_TYPE_FLAG_DISPLAY (1u << 1)	 /* Device capable of display. */
#define GBM_DEV_TYPE_FLAG_3D (1u << 2)		 /* Device capable or 3D rendering. */
#define GBM_DEV_TYPE_FLAG_ARMSOC (1u << 3)	 /* Device on ARM SOC. */
#define GBM_DEV_TYPE_FLAG_USB (1u << 4)		 /* USB device, udl, evdi. */
#define GBM_DEV_TYPE_FLAG_BLOCKED (1u << 5)	 /* Unsuitable device e.g. vgem, udl, evdi. */
#define GBM_DEV_TYPE_FLAG_INTERNAL_LCD (1u << 6) /* Device is driving internal LCD. */

struct gbm_device_info {
	uint32_t dev_type_flags;
	int dri_node_num; /* DRI node number (0..63), for easy matching of devices. */
	unsigned int connectors;
	unsigned int connected;
};

#define GBM_DETECT_FLAG_CONNECTED (1u << 0) /* Check if any connectors are connected. SLOW! */

int gbm_detect_device_info(unsigned int detect_flags, int fd, struct gbm_device_info *info);
int gbm_detect_device_info_path(unsigned int detect_flags, const char *dev_node,
				struct gbm_device_info *info);

/*
 * Select "default" device to use for graphics memory allocator.
 */
int gbm_get_default_device_fd(void);

#ifdef __cplusplus
}
#endif

#endif
