#!/usr/bin/env python3
# Copyright 2022 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import subprocess
import sys

cmd = "moc-qt" + sys.argv[3]
subprocess.check_call([cmd, sys.argv[1], "-o", sys.argv[2]])
