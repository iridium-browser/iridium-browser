export const description = `
Execution tests for the 'acosh' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn acosh(e: T ) -> T
Returns the hyperbolic arc cosine of e. The result is 0 when e < 1.
Computes the non-negative functional inverse of cosh.
Component-wise when T is a vector.
Note: The result is not mathematically meaningful when e < 1.

`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { TypeF32 } from '../../../../../util/conversion.js';
import { acoshIntervals } from '../../../../../util/f32_interval.js';
import { biasedRange, fullF32Range } from '../../../../../util/math.js';
import { allInputSources, Case, makeUnaryToF32IntervalCase, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

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
    const makeCase = (n: number): Case => {
      return makeUnaryToF32IntervalCase(n, ...acoshIntervals);
    };

    const cases = [
      ...biasedRange(1, 2, 100), // x near 1 can be problematic to implement
      ...fullF32Range(),
    ].map(makeCase);
    await run(t, builtin('acosh'), [TypeF32], TypeF32, t.params, cases);
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f16 tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();
