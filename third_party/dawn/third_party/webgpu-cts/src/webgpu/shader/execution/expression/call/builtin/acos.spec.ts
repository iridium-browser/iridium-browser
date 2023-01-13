export const description = `
Execution tests for the 'acos' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn acos(e: T ) -> T
Returns the arc cosine of e. Component-wise when T is a vector.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { TypeF32 } from '../../../../../util/conversion.js';
import { acosInterval } from '../../../../../util/f32_interval.js';
import { sourceFilteredF32Range, linearRange } from '../../../../../util/math.js';
import { makeCaseCache } from '../../case_cache.js';
import { allInputSources, Case, makeUnaryToF32IntervalCase, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

export const d = makeCaseCache('acos', {
  f32_const: () => {
    const makeCase = (n: number): Case => {
      return makeUnaryToF32IntervalCase(n, acosInterval);
    };

    return [
      ...linearRange(-1, 1, 100), // acos is defined on [-1, 1]
      ...sourceFilteredF32Range('const', -1, 1),
    ].map(makeCase);
  },
  f32_non_const: () => {
    const makeCase = (n: number): Case => {
      return makeUnaryToF32IntervalCase(n, acosInterval);
    };

    return [
      ...linearRange(-1, 1, 100), // acos is defined on [-1, 1]
      ...sourceFilteredF32Range('storage', -1, 1),
    ].map(makeCase);
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
    await run(t, builtin('acos'), [TypeF32], TypeF32, t.params, cases);
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f16 tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();
