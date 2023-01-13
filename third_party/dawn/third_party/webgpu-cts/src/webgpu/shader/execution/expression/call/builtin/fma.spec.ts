export const description = `
Execution tests for the 'fma' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn fma(e1: T ,e2: T ,e3: T ) -> T
Returns e1 * e2 + e3. Component-wise when T is a vector.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { TypeF32 } from '../../../../../util/conversion.js';
import { fmaInterval } from '../../../../../util/f32_interval.js';
import { sparseF32Range } from '../../../../../util/math.js';
import { allInputSources, Case, makeTernaryToF32IntervalCase, run } from '../../expression.js';

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
    // Using sparseF32Range since this will generate N^3 test cases
    const values = sparseF32Range(t.params.inputSource === 'const');
    const cases: Array<Case> = [];
    values.forEach(x => {
      values.forEach(y => {
        values.forEach(z => {
          cases.push(makeTernaryToF32IntervalCase(x, y, z, fmaInterval));
        });
      });
    });

    await run(t, builtin('fma'), [TypeF32, TypeF32, TypeF32], TypeF32, t.params, cases);
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f16 tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();
