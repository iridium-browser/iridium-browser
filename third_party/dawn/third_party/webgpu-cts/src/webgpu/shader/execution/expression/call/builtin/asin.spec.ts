export const description = `
Execution tests for the 'asin' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn asin(e: T ) -> T
Returns the arc sine of e. Component-wise when T is a vector.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { TypeF32 } from '../../../../../util/conversion.js';
import { asinInterval } from '../../../../../util/f32_interval.js';
import { fullF32Range, linearRange } from '../../../../../util/math.js';
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
      return makeUnaryToF32IntervalCase(n, asinInterval);
    };

    const cases = [
      ...linearRange(-1, 1, 100), // asin is defined on [-1, 1]
      ...fullF32Range(),
    ].map(makeCase);
    await run(t, builtin('asin'), [TypeF32], TypeF32, t.params, cases);
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f16 tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();
