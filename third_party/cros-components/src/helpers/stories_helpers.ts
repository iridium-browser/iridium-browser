/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {boolInput, Knob} from 'google3/javascript/lit/stories';

const LEFT_TO_RIGHT = 'ltr';
const RIGHT_TO_LEFT = 'rtl';

/** Returns document direction */
export function getDirection(): 'ltr'|'rtl' {
  return document.dir === RIGHT_TO_LEFT ? RIGHT_TO_LEFT : LEFT_TO_RIGHT;
}

/** Returns true if the document is in 'rtl' (right to left) mode. */
export function isRtl(): boolean {
  return getDirection() === RIGHT_TO_LEFT;
}

/**
 * Helper function for storybook demos to set the document direction. Provide
 * this function to the RTL knob's `wiring` property.
 *
 * Usage:
 * ```
 * new Knob('rtl', {
 *    ui: boolInput(),
 *    defaultValue: false,
 *    wiring: changeDocumentDirection,
 * });
 * ```
 */
export function changeDocumentDirection(
    knob: Knob<boolean>, value: boolean, container: HTMLElement) {
  document.documentElement.dir = value ? RIGHT_TO_LEFT : LEFT_TO_RIGHT;
}

/** Returns Knob for use in stories to switch document RTL direction */
export function rtlKnob(): Knob<boolean> {
  return new Knob(
      RIGHT_TO_LEFT,
      {defaultValue: false, ui: boolInput(), wiring: changeDocumentDirection});
}
