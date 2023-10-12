# Copyright 2023 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Adds external repos necessary for anonymous-tokens.
"""

load("@tink_base//:tink_base_deps.bzl", "tink_base_deps")
load("@tink_cc//:tink_cc_deps.bzl", "tink_cc_deps")
load("@rules_cc//cc:repositories.bzl", "rules_cc_dependencies")

def anonymous_tokens_deps():
    """
    Adds all external repos necessary for anonymous_tokens.
    """
    tink_base_deps()
    tink_cc_deps()
    rules_cc_dependencies()
