/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {Sidenav} from './sidenav';
import {SidenavItem} from './sidenav_item';

/** Check if an `Element` is a sidenav or not. */
export function isSidenav(element: Element): element is Sidenav {
  return element.tagName === 'CROS-SIDENAV';
}

/** Check if an `Element` is a sidenav item or not. */
export function isSidenavItem(element: Element): element is SidenavItem {
  return element.tagName === 'CROS-SIDENAV-ITEM';
}

/** True if the page is read right-to-left. */
export function isRTL() {
  return document.documentElement.dir === 'rtl';
}
