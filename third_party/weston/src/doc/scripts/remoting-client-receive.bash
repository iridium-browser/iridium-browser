#!/bin/bash

# Copyright © 2018 Renesas Electronics Corp.
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
#
# Authors: IGEL Co., Ltd.

# By using this script, client can receive remoted output via gstreamer.
# Usage:
#	remoting-client-receive.bash <PORT NUMBER>

gst-launch-1.0 rtpbin name=rtpbin \
	       udpsrc caps="application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=JPEG,payload=26" port=$1 ! \
	       rtpbin.recv_rtp_sink_0 \
	       rtpbin. ! rtpjpegdepay ! jpegdec ! autovideosink \
	       udpsrc port=$(($1 + 1)) ! rtpbin.recv_rtcp_sink_0 \
	       rtpbin.send_rtcp_src_0 ! \
	       udpsink port=$(($1 + 2)) sync=false async=false
