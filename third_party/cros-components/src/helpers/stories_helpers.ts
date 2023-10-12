/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {boolInput, Knob, KnobUi} from 'google3/javascript/lit/stories';
import {html} from 'lit';

const LEFT_TO_RIGHT = 'ltr';
const RIGHT_TO_LEFT = 'rtl';

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
  container.dir = value ? RIGHT_TO_LEFT : LEFT_TO_RIGHT;
}

/** Returns Knob for use in stories to switch document RTL direction */
export function rtlKnob(): Knob<boolean, 'rtl'> {
  return new Knob(
      RIGHT_TO_LEFT,
      {defaultValue: false, ui: boolInput(), wiring: changeDocumentDirection});
}

/**
 * A knobUI that allows the user to upload a local file. The file is converted
 * to a base64 data url for use by the current story.
 */
export function fileInput(): KnobUi<string> {
  return {
    render(knob: Knob<string>, onChange: (val: string) => void) {
      const valueChanged = (e: Event) => {
        const file = (e.target as HTMLInputElement).files![0];
        const reader = new FileReader();
        reader.addEventListener('load', () => {
          if (reader.result === null || typeof reader.result !== 'string') {
            return;
          }
          onChange(reader.result);
        });
        reader.readAsDataURL(file);
      };

      return html`
        <div>
          <mwc-formfield .label="${knob.name}">
            <input @change=${valueChanged} type="file" accept=".svg"/>
          </mwc-formfield>
        </div>
      `;
    },
    deserialize(serialized: string) {
      return serialized;
    },
    serialize(url: string): string {
      return url;
    },
  };
}
