#!/bin/bash

# Copyright 2018 Collabora, Ltd.
# Copyright 2018 General Electric Company
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial
# portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# This is an example script working as Weston's calibration helper.
# Its purpose is to permanently store the calibration matrix for the given
# touchscreen input device into a udev property. Since this script naturally
# runs as the user that runs Weston, it presumably cannot write directly into
# /etc. It is left for the administrator to set up appropriate files and
# permissions.

# To use this script, one needs to edit weston.ini, in section [libinput], add:
#   calibration_helper=/path/to/bin/calibration-helper.bash

# exit immediately if any command fails
set -e

# The arguments Weston gives us:
SYSPATH="$1"
MATRIX="$2 $3 $4 $5 $6 $7"

# Pick something to recognize the right touch device with.
# Usually one would use something like a serial.
SERIAL=$(udevadm info "$SYSPATH" --query=property | \
	awk -- 'BEGIN { FS="=" } { if ($1 == "ID_SERIAL") { print $2; exit } }')

# If cannot find a serial, tell the server to not use the new calibration.
[ -z "$SERIAL" ] && exit 1

# You'd have this write a file instead.
echo "ACTION==\"add|change\",SUBSYSTEM==\"input\",ENV{ID_SERIAL}==\"$SERIAL\",ENV{LIBINPUT_CALIBRATION_MATRIX}=\"$MATRIX\""

# Then you'd tell udev to reload the rules:
#udevadm control --reload
# This lets Weston get the new calibration if you unplug and replug the input
# device. Instead of writing a udev rule directly, you could have a udev rule
# with IMPORT{file}="/path/to/calibration", write
# "LIBINPUT_CALIBRATION_MATRIX=\"$MATRIX\"" into /path/to/calibration instead,
# and skip this reload step.

# Make udev process the new rule by triggering a "change" event:
#udevadm trigger "$SYSPATH"
# If you were to restart Weston without rebooting, this lets it pick up the new
# calibration.
