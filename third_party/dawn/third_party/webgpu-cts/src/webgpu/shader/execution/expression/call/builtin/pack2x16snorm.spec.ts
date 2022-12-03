export const description = `
Converts two normalized floating point values to 16-bit signed integers, and then combines them into one u32 value.
Component e[i] of the input is converted to a 16-bit twos complement integer value
⌊ 0.5 + 32767 × min(1, max(-1, e[i])) ⌋ which is then placed in
bits 16 × i through 16 × i + 15 of the result.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

g.test('pack')
  .specURL('https://www.w3.org/TR/WGSL/#pack-builtin-functions')
  .desc(
    `
@const fn pack2x16snorm(e: vec2<f32>) -> u32
`
  )
  .unimplemented();
