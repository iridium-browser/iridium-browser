export const description = `
Execution tests for the 'atanh' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn atanh(e: T ) -> T
Returns the hyperbolic arc tangent of e. The result is 0 when abs(e) ≥ 1.
Computes the functional inverse of tanh.
Component-wise when T is a vector.
Note: The result is not mathematically meaningful when abs(e) >= 1.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { kValue } from '../../../../../util/constants.js';
import { TypeF32 } from '../../../../../util/conversion.js';
import { atanhInterval } from '../../../../../util/f32_interval.js';
import { biasedRange, sourceFilteredF32Range } from '../../../../../util/math.js';
import { makeCaseCache } from '../../case_cache.js';
import { allInputSources, Case, makeUnaryToF32IntervalCase, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

export const d = makeCaseCache('atanh', {
  f32_const: () => {
    const makeCase = (n: number): Case => {
      return makeUnaryToF32IntervalCase(n, atanhInterval);
    };

    return [
      ...biasedRange(kValue.f32.negative.less_than_one, -0.9, 20), // discontinuity at x = -1
      ...biasedRange(kValue.f32.positive.less_than_one, 0.9, 20), // discontinuity at x = 1
      ...sourceFilteredF32Range(
        'const',
        kValue.f32.negative.less_than_one,
        kValue.f32.positive.less_than_one
      ),
    ].map(makeCase);
  },
  f32_non_const: () => {
    const makeCase = (n: number): Case => {
      return makeUnaryToF32IntervalCase(n, atanhInterval);
    };

    const cases = [
      ...biasedRange(kValue.f32.negative.less_than_one, -0.9, 20), // discontinuity at x = -1
      ...biasedRange(kValue.f32.positive.less_than_one, 0.9, 20), // discontinuity at x = 1
      ...sourceFilteredF32Range(
        'storage',
        kValue.f32.negative.less_than_one,
        kValue.f32.positive.less_than_one
      ),
    ].map(makeCase);

    // Handle the edge case of -1 and 1 when not doing const-eval
    const edgeCases = [-1, 1].map(makeCase);
    cases.push(...edgeCases);

    return cases;
  },
});

g.test('abstract_float')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`abstract float tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();

g.test('f32')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f32 tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const cases = await d.get(t.params.inputSource === 'const' ? 'f32_const' : 'f32_non_const');
    await run(t, builtin('atanh'), [TypeF32], TypeF32, t.params, cases);
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f16 tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();
