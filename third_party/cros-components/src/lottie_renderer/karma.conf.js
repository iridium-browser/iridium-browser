/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Configure karma to serve assets and test data from the source tree.
 *
 * @param {?} config The existing config that we're extending.
 */
module.exports = function(config) {
  config.files.push({
    pattern: 'third_party/javascript/cros_components/lottie_renderer/assets/**',
    watched: false,
    included: false,
  });
  config.files.push({
    pattern: 'third_party/javascript/cros_components/lottie_renderer/js/**',
    watched: false,
    included: false,
  });
  config.proxies = {
    '/assets/':
        '/base/third_party/javascript/cros_components/lottie_renderer/assets/',
    '/js/': '/base/third_party/javascript/cros_components/lottie_renderer/js/',
  };
};
