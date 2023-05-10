# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from crossbench.browsers.base import (BROWSERS_CACHE, Browser,
                                      convert_flags_to_label)
from crossbench.browsers.chrome import Chrome, ChromeWebDriver
from crossbench.browsers.chromium import Chromium, ChromiumWebDriver
from crossbench.browsers.edge import Edge, EdgeWebDriver
from crossbench.browsers.firefox import Firefox, FirefoxWebDriver
from crossbench.browsers.safari import Safari, SafariWebDriver
from crossbench.browsers.webdriver import RemoteWebDriver
