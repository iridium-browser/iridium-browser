import { memcpy, TypedArrayBufferView } from '../../common/util/util.js';

import { align } from './math.js';

/**
 * Creates a buffer with the contents of some TypedArray.
 */
export function makeBufferWithContents(
  device: GPUDevice,
  dataArray: TypedArrayBufferView,
  usage: GPUBufferUsageFlags,
  opts: { padToMultipleOf4?: boolean } = {}
): GPUBuffer {
  const buffer = device.createBuffer({
    mappedAtCreation: true,
    size: align(dataArray.byteLength, opts.padToMultipleOf4 ? 4 : 1),
    usage,
  });
  memcpy({ src: dataArray }, { dst: buffer.getMappedRange() });
  buffer.unmap();
  return buffer;
}
