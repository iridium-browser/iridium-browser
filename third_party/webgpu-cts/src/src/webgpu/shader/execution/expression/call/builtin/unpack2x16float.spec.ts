export const description = `
Decomposes a 32-bit value into two 16-bit chunks, and reinterpets each chunk as
a floating point value.
Component i of the result is the f32 representation of v, where v is the
interpretation of bits 16×i through 16×i+15 of e as an IEEE-754 binary16 value.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { TypeF32, TypeU32, TypeVec } from '../../../../../util/conversion.js';
import { unpack2x16floatInterval } from '../../../../../util/f32_interval.js';
import { fullU32Range } from '../../../../../util/math.js';
import { allInputSources, Case, makeU32ToVectorIntervalCase, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

g.test('unpack')
  .specURL('https://www.w3.org/TR/WGSL/#unpack-builtin-functions')
  .desc(
    `
@const fn unpack2x16float(e: u32) -> vec2<f32>
`
  )
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const makeCase = (n: number): Case => {
      return makeU32ToVectorIntervalCase(n, unpack2x16floatInterval);
    };

    const cases: Array<Case> = fullU32Range().map(makeCase);

    await run(t, builtin('unpack2x16float'), [TypeU32], TypeVec(2, TypeF32), t.params, cases);
  });
