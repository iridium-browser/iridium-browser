/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {MdFilledTextField} from '@material/web/textfield/filled-text-field';
import {cast} from 'google3/javascript/common/asserts/asserts';
import {isInstanceOf} from 'google3/javascript/common/asserts/guards';
import {TextFieldHarness} from 'google3/third_party/javascript/material/web/textfield/harness';

import {Textfield} from './textfield';

/** Get the internal `md-textfield` element. */
export function getMdTextfield(textfield: Textfield): MdFilledTextField {
  return cast(
      textfield.renderRoot.querySelector('md-filled-text-field'),
      isInstanceOf(MdFilledTextField));
}

/** Get a harness for the `cros-textfield` element. */
export function getHarnessFor(textfield: Textfield) {
  return new TextFieldHarness(getMdTextfield(textfield));
}

/**
 * Change events are only fired on blur, so for accuracy here we simulate the
 * full sequence of events on the textfield.
 */
export async function simulateInputChange(
    harness: TextFieldHarness, input: string) {
  await harness.focusWithKeyboard();
  await harness.inputValue(input);
  await harness.blur();
}
