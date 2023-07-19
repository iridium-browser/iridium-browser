
/**
 * @license
 * copyright 2022 google llc
 * spdx-license-identifier: apache-2.0
 */

import 'jasmine';

import {waitForEvent} from 'google3/third_party/javascript/cros_components/async_helpers/async_helpers';

import {CrosLottieEvent, LottieRenderer} from './lottie-renderer';

interface LottieComponentParams {
  assetUrl: string;
  autoplay: boolean;
  loop: boolean;
  dynamic: boolean;
}

const ASSET_URL = '/assets/oobe_pin.json';
/**
 * Render an lottie renderer component, and return a DOM reference to it.
 * This is done via a function in each individual test body rather than as
 * part of a clean state, in order to ensure the initialization flow works as
 * expected.
 */
export function createLottieRenderer(
    params: Partial<LottieComponentParams> = {}) {
  const renderer = new LottieRenderer();
  renderer.assetUrl = ASSET_URL;
  renderer.autoplay = params.autoplay || false;
  renderer.loop = params.loop || false;

  // The component itself defaults to the GM2 palette, but for tests we can set
  // this to true.
  renderer.dynamic = params.dynamic || true;
  return renderer;
}

/** Create a jasmine Spy for a given event, and attach it as a listener. */
export function createAndAttachSpyListener(
    event: CrosLottieEvent, renderer: LottieRenderer) {
  const listener = jasmine.createSpy(event);
  renderer.addEventListener(event, listener);
  return listener;
}

/** Return an en expectation waiting on a CrosLottieEvent promise. */
export function eventToPromiseExpectation(
    event: CrosLottieEvent, renderer: LottieRenderer) {
  return expectAsync(waitForEvent(renderer, event));
}

/**
 * Mocks out the computed style CSS declaration and binds it as the return value
 * for getComputedStyle. Clients can then set the mock to return a known hex
 * color which will be used in the color mapping logic of the LottieRenderer.
 */
export function mockComputedStyle() {
  const mockComputedStyle = jasmine.createSpyObj('computedStyle', [
    'getPropertyValue',
  ]);
  spyOn(window, 'getComputedStyle').and.returnValue(mockComputedStyle);
  return mockComputedStyle;
}
