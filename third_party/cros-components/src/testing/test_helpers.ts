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
 * Interface for helper functions to accept elements that may have a pending
 * Lit render.
 */
export interface MaybeHasUpdateComplete extends HTMLElement {
  updateComplete?: Promise<unknown>;
}

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

/**
 * Use this to wait until the element and all its descendants have finished
 * updating.
 *
 * TODO(b/139310294) replace this when the Polymer API becomes available.
 */
export async function deepUpdateComplete(element: MaybeHasUpdateComplete) {
  // TODO(b/139310294): Replace these await statements with Polymer API to
  // blocks until descendants have loaded (when it becomes available).
  await element.updateComplete;
  // Wait for nested LitElements to finish rendering. Assumes that all nested
  // elements' render() complete in microtasks.
  await completePendingMicrotasks();
}

/**
 * Appends an HTMLElement to the given `parentElement` element and waits until
 * the element and all its descendants have finished rendering.
 *
 * The passed element must have a updateComplete property (e.g., a LitElement).
 * Elements without updateComplete don't need to use this method and may be
 * appended/removed using Element.append/ChildNode.remove.
 *
 * This will not work when descendants defer their update (should be fixed by
 * b/139310294).
 *
 * Each call to appendElementForTest should have a corresponding call to
 * removeElementForTest.
 */
export async function appendElementForTest(
    element: MaybeHasUpdateComplete, parentElement = document.body) {
  parentElement.append(element);
  await deepUpdateComplete(element);
}

/** Removes an HTMLElement from the DOM. Counterpart to appendElementForTest */
export async function removeElementForTest(element: MaybeHasUpdateComplete) {
  // Complete pending tasks before removing from the DOM. The ResizeObserver
  // machinery assumes that "loop limit exceeded" is a useful error if it ever
  // "skips" an observation. But it seems that can also happen simply because
  // the objects being observed are gone, so give it a chance to send out
  // notifications.
  await completePendingMicrotasks();
  element.remove();
}
