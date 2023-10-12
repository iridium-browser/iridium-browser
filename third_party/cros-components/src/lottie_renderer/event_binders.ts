/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @fileoverview The event binding code used when lottie-renderer is running in
 * chromium. NOTE this is in a sub folder so automatic build tools don't keep
 * trying to add cr_worker into ts_library rules. This file must never actually
 * get compiled in google3 because it's not correct code in google3. It only
 * works when in web ui in chromium.
 */

import {COLOR_PROVIDER_CHANGED, ColorChangeUpdater} from 'chrome://resources/cr_components/color_change_listener/colors_css_updater.js';

/**
 * Registers `listener` to be called everytime the current color scheme
 * changes.
 */
export function addColorSchemeChangeListener(listener: () => void) {
  ColorChangeUpdater.forDocument().eventTarget.addEventListener(
      COLOR_PROVIDER_CHANGED, listener);
}

/**
 * Unregisters a `listener` previously registered with
 * `addColorSchemeChangeListener`.
 */
export function removeColorSchemeChangeListener(listener: () => void) {
  ColorChangeUpdater.forDocument().eventTarget.removeEventListener(
      COLOR_PROVIDER_CHANGED, listener);
}
