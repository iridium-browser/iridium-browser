import { Fixture } from '../../common/framework/fixture.js';
import { unreachable } from '../../common/util/util.js';

export const kAllCanvasTypes = ['onscreen', 'offscreen'] as const;
export type CanvasType = typeof kAllCanvasTypes[number];

/** Valid contextId for HTMLCanvasElement/OffscreenCanvas,
 *  spec: https://html.spec.whatwg.org/multipage/canvas.html#dom-canvas-getcontext
 */
export const kValidCanvasContextIds = [
  '2d',
  'bitmaprenderer',
  'webgl',
  'webgl2',
  'webgpu',
] as const;
export type CanvasContext = typeof kValidCanvasContextIds[number];

/** Helper(s) to determine if context is copyable. */
export function canCopyFromCanvasContext(contextName: CanvasContext) {
  switch (contextName) {
    case '2d':
    case 'webgl':
    case 'webgl2':
    case 'webgpu':
      return true;
    default:
      return false;
  }
}

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
    if (typeof OffscreenCanvas !== 'undefined') {
      canvas = createOffscreenCanvas(test, width, height);
    } else {
      test.skip('Cannot create an OffscreenCanvas');
    }
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
