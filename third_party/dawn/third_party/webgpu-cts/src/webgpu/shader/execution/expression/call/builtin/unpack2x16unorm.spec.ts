export const description = `
Decomposes a 32-bit value into two 16-bit chunks, then reinterprets each chunk
as an unsigned normalized floating point value.
Component i of the result is v ÷ 65535, where v is the interpretation of bits
16×i through 16×i+15 of e as an unsigned integer.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { TypeF32, TypeU32, TypeVec } from '../../../../../util/conversion.js';
import { unpack2x16unormInterval } from '../../../../../util/f32_interval.js';
import { fullU32Range } from '../../../../../util/math.js';
import { makeCaseCache } from '../../case_cache.js';
import { allInputSources, Case, makeU32ToVectorIntervalCase, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

export const d = makeCaseCache('unpack2x16unorm', {
  u32: () => {
    const makeCase = (n: number): Case => {
      return makeU32ToVectorIntervalCase(n, unpack2x16unormInterval);
    };

    return fullU32Range().map(makeCase);
  },
});

g.test('unpack')
  .specURL('https://www.w3.org/TR/WGSL/#unpack-builtin-functions')
  .desc(
    `
@const fn unpack2x16unorm(e: u32) -> vec2<f32>
`
  )
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases = await d.get('u32');
    await run(t, builtin('unpack2x16unorm'), [TypeU32], TypeVec(2, TypeF32), t.params, cases);
  });
