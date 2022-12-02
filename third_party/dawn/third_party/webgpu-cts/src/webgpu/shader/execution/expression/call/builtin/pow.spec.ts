export const description = `
Execution tests for the 'pow' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn pow(e1: T ,e2: T ) -> T
Returns e1 raised to the power e2. Component-wise when T is a vector.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { TypeF32 } from '../../../../../util/conversion.js';
import { powInterval } from '../../../../../util/f32_interval.js';
import { fullF32Range } from '../../../../../util/math.js';
import { allInputSources, Case, makeBinaryToF32IntervalCase, run } from '../../expression.js';

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
    const makeCase = (x: number, y: number): Case => {
      return makeBinaryToF32IntervalCase(x, y, powInterval);
    };

    const cases: Array<Case> = [];
    const numeric_range = fullF32Range();
    numeric_range.forEach(x => {
      numeric_range.forEach(y => {
        cases.push(makeCase(x, y));
      });
    });

    await run(t, builtin('pow'), [TypeF32, TypeF32], TypeF32, t.params, cases);
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f16 tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();
