/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import 'jasmine';

import {Deflaker} from 'google3/testing/js/deflaker/deflaker';
import {Scuba} from 'google3/testing/karma/karma_scuba_framework';
import {cleanState} from 'google3/testing/web/jasmine/state/clean_state';

/**
 * Creates a container element for elements to be screenshot in and append it
 * to the dom. Returns a clean state instance with a reference to this container
 * and a diff function which automatically checks this container element against
 * a given golden.
 */
export function setupScreenshotTests(scuba: Scuba) {
  const state = cleanState(() => {
    const container = document.createElement('div');
    container.style.display = 'inline-block';
    return {container};
  });


  /**
   * Assert that the contents of the `state.container` match the golden
   * identified by `screenshotName`.
   */
  async function diff(screenshotName: string) {
    await Deflaker.waitForStabilityPromise();

    state.container.classList.add('scuba-screenshot-element');
    expect(await scuba.diffElement(screenshotName, '.scuba-screenshot-element'))
        .toHavePassed();
    state.container.classList.remove('scuba-screenshot-element');
  }

  beforeEach(() => {
    document.body.appendChild(state.container);
  });
  afterEach(() => {
    document.body.removeChild(state.container);
  });

  return {state, diff};
}
