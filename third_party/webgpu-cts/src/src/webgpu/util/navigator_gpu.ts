/// <reference types="@webgpu/types" />

import { assert } from '../../common/util/util.js';

let impl: GPU | undefined = undefined;

/**
 * Finds and returns the `navigator.gpu` object (or equivalent, for non-browser implementations).
 * Throws an exception if not found.
 */
export function getGPU(): GPU {
  if (impl) {
    return impl;
  }

  assert(
    typeof navigator !== 'undefined' && navigator.gpu !== undefined,
    'No WebGPU implementation found'
  );

  impl = navigator.gpu;
  return impl;
}
