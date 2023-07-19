/* Sync file Linux kernel UAPI */

#ifndef WESTON_LINUX_SYNC_FILE_UAPI_H
#define WESTON_LINUX_SYNC_FILE_UAPI_H

#include <linux/ioctl.h>
#include <linux/types.h>

struct sync_fence_info {
	char obj_name[32];
	char driver_name[32];
	__s32 status;
	__u32 flags;
	__u64 timestamp_ns;
};

struct sync_file_info {
	char name[32];
	__s32 status;
	__u32 flags;
	__u32 num_fences;
	__u32 pad;

	__u64 sync_fence_info;
};

#define SYNC_IOC_MAGIC '>'
#define SYNC_IOC_FILE_INFO _IOWR(SYNC_IOC_MAGIC, 4, struct sync_file_info)

#endif /* WESTON_LINUX_SYNC_FILE_UAPI_H */
