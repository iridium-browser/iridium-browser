export const description = `
Converts four normalized floating point values to 8-bit unsigned integers, and then combines them into one u32 value.
Component e[i] of the input is converted to an 8-bit unsigned integer value
⌊ 0.5 + 255 × min(1, max(0, e[i])) ⌋ which is then placed in
bits 8 × i through 8 × i + 7 of the result.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

g.test('pack')
  .specURL('https://www.w3.org/TR/WGSL/#pack-builtin-functions')
  .desc(
    `
@const fn pack4x8unorm(e: vec4<f32>) -> u32
`
  )
  .unimplemented();
