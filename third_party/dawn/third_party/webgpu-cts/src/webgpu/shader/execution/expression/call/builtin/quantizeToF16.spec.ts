export const description = `
Execution tests for the 'quantizeToF16' builtin function

T is f32 or vecN<f32>
@const fn quantizeToF16(e: T ) -> T
Quantizes a 32-bit floating point value e as if e were converted to a IEEE 754
binary16 value, and then converted back to a IEEE 754 binary32 value.
Component-wise when T is a vector.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { kValue } from '../../../../../util/constants.js';
import { TypeF32 } from '../../../../../util/conversion.js';
import { quantizeToF16Interval } from '../../../../../util/f32_interval.js';
import { fullF32Range } from '../../../../../util/math.js';
import { allInputSources, Case, makeUnaryToF32IntervalCase, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

g.test('f32')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f32 tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const makeCase = (x: number): Case => {
      return makeUnaryToF32IntervalCase(x, quantizeToF16Interval);
    };

    const cases = [
      kValue.f16.negative.min,
      kValue.f16.negative.max,
      kValue.f16.subnormal.negative.min,
      kValue.f16.subnormal.negative.max,
      kValue.f16.subnormal.positive.min,
      kValue.f16.subnormal.positive.max,
      kValue.f16.positive.min,
      kValue.f16.positive.max,
      ...fullF32Range(),
    ].map(makeCase);

    await run(t, builtin('quantizeToF16'), [TypeF32], TypeF32, t.params, cases);
  });
