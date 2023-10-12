#!/bin/bash

# folders that are necessary to run Weston tests
mkdir -p /tmp/tests
mkdir -p /tmp/.X11-unix
chmod -R 0700 /tmp

# set environment variables to run Weston tests
export XDG_RUNTIME_DIR=/tmp/tests
export LIBSEAT_BACKEND=seatd
# In our test suite, we use VKMS to run DRM-backend tests. The order in which
# devices are loaded is not predictable, so the DRM node that VKMS takes can
# change across each boot. That's why we have this one-liner shell script to get
# the appropriate node for VKMS.
export WESTON_TEST_SUITE_DRM_DEVICE=$(basename /sys/devices/platform/vkms/drm/card*)
# To run tests in the CI that exercise the zwp_linux_dmabuf_v1 implementation in
# Weston, we use VGEM to allocate buffers.
export WESTON_TEST_SUITE_ALLOC_DEVICE=$(basename /sys/devices/platform/vgem/drm/card*)

# ninja test depends on meson, and meson itself looks for its modules on folder
# $HOME/.local/lib/pythonX.Y/site-packages (the Python version may differ).
# build-deps.sh installs dependencies to /usr/local.
# virtme starts with HOME=/tmp/roothome, but as we installed meson on user root,
# meson can not find its modules. So we change the HOME env var to fix that.
export HOME=/root
export PATH=$HOME/.local/bin:$PATH
export PATH=/usr/local/bin:$PATH

export SEATD_LOGLEVEL=debug

# Terrible hack, per comment in weston-test-runner.c's main(): find Mesa's
# llvmpipe driver module location
export WESTON_CI_LEAK_DL_HANDLE=$(find /usr/local -name swrast_dri.so -print 2>/dev/null || true)

# run the tests and save the exit status
# we give ourselves a very generous timeout multiplier due to ASan overhead
echo 0x1f > /sys/module/drm/parameters/debug
seatd-launch -- meson test --no-rebuild --timeout-multiplier 4 \
	--wrapper $(pwd)/../.gitlab-ci/virtme-scripts/per-test-asan.sh
# note that we need to store the return value from the tests in order to
# determine if the test suite ran successfully or not.
TEST_RES=$?
dmesg &> dmesg.log
echo 0x00 > /sys/module/drm/parameters/debug

# create a file to keep the result of this script:
#   - 0 means the script succeeded
#   - 1 means the tests failed, so the job itself should fail
TESTS_RES_PATH=$(pwd)/tests-res.txt
echo $TEST_RES > $TESTS_RES_PATH

# shutdown virtme
sync
poweroff -f
