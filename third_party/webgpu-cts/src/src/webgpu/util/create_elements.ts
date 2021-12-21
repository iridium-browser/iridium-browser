import { Fixture } from '../../common/framework/fixture.js';
import { unreachable } from '../../common/util/util.js';

export const allCanvasTypes = ['onscreen', 'offscreen'] as const;
export type canvasTypes = 'onscreen' | 'offscreen';

/** Create HTMLCanvas/OffscreenCanvas. */
export function createCanvas(
  test: Fixture,
  canvasType: 'onscreen' | 'offscreen',
  width: number,
  height: number
): HTMLCanvasElement | OffscreenCanvas {
  let canvas: HTMLCanvasElement | OffscreenCanvas;
  if (canvasType === 'onscreen') {
    if (typeof document !== 'undefined') {
      canvas = createOnscreenCanvas(test, width, height);
    } else {
      test.skip('Cannot create HTMLCanvasElement');
    }
  } else if (canvasType === 'offscreen') {
    canvas = createOffscreenCanvas(test, width, height);
  } else {
    unreachable();
  }

  return canvas;
}

/** Create HTMLCanvasElement. */
export function createOnscreenCanvas(
  test: Fixture,
  width: number,
  height: number
): HTMLCanvasElement {
  let canvas: HTMLCanvasElement;
  if (typeof document !== 'undefined') {
    canvas = document.createElement('canvas');
    canvas.width = width;
    canvas.height = height;
  } else {
    test.skip('Cannot create HTMLCanvasElement');
  }
  return canvas;
}

/** Create OffscreenCanvas. */
export function createOffscreenCanvas(
  test: Fixture,
  width: number,
  height: number
): OffscreenCanvas {
  if (typeof OffscreenCanvas === 'undefined') {
    test.skip('OffscreenCanvas is not supported');
  }

  return new OffscreenCanvas(width, height);
}
