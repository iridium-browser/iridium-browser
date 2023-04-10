/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import 'jasmine';

import {Deflaker} from 'google3/testing/js/deflaker/deflaker';
import {init as initDeflaker} from 'google3/testing/js/deflaker/init';
import {getDefaults} from 'google3/testing/js/deflaker/settings';
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

/**
 * Waits until a new task can start executing. This means that (at least) the
 * pending microtasks have been flushed.
 */
 export async function completePendingMicrotasks() {
  // The spec guarantees Promises to execute before timeouts.
  // https://html.spec.whatwg.org/multipage/webappapis.html#event-loop-processing-model
  return new Promise(resolve => void setTimeout(resolve, 0));
}

/** Converts an event occurrence to a promise. */
export function htmlEventToPromise<K extends keyof HTMLElementEventMap>(
    eventType: K,
    target: Element|EventTarget|Window): Promise<HTMLElementEventMap[K]> {
  return new Promise((resolve, reject) => {
    target.addEventListener(eventType, function f(e: Event) {
      target.removeEventListener(eventType, f);
      resolve(e as HTMLElementEventMap[K]);
    });
  });
}

let deflakerInited = false;

/** Nonce initialisation of the scuba deflaker. */
export function nonceInitDeflaker() {
  if (!deflakerInited) {
    const deflakerSettings = getDefaults();
    initDeflaker(deflakerSettings);
    deflakerInited = true;
  }
}
