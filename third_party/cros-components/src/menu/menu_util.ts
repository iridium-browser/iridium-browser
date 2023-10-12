/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {MenuItem} from './menu_item';

/** Check if an `Element` is a menu item or not. */
function isMenuItem(element: unknown): element is MenuItem {
  return element instanceof Element && element.tagName === 'CROS-MENU-ITEM';
}

/** Returns the active element, if it's a MenuItem, or null. */
export function shadowPiercingActiveItem(): Element|null {
  let activeElement = document.activeElement;
  while (activeElement && !isMenuItem(activeElement)) {
    activeElement = activeElement.shadowRoot ?
        activeElement.shadowRoot.activeElement :
        null;
  }
  return activeElement;
}
