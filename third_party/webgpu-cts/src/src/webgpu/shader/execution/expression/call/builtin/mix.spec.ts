export const description = `
Execution tests for the 'mix' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn mix(e1: T, e2: T, e3: T) -> T
Returns the linear blend of e1 and e2 (e.g. e1*(1-e3)+e2*e3). Component-wise when T is a vector.

T is AbstractFloat, f32, or f16
T2 is vecN<T>
@const fn mix(e1: T2, e2: T2, e3: T) -> T2
Returns the component-wise linear blend of e1 and e2, using scalar blending factor e3 for each component.
Same as mix(e1,e2,T2(e3)).

`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { kValue } from '../../../../../util/constants.js';
import { TypeF32 } from '../../../../../util/conversion.js';
import { mixIntervals } from '../../../../../util/f32_interval.js';
import { allInputSources, Case, makeTernaryF32IntervalCase, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

g.test('matching_abstract_float')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`abstract float tests with matching params`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();

g.test('matching_f32')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f32 test with matching third param`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const makeCase = (x: number, y: number, z: number): Case => {
      return makeTernaryF32IntervalCase(x, y, z, ...mixIntervals);
    };

    const values: Array<number> = [
      Number.NEGATIVE_INFINITY,
      kValue.f32.negative.min,
      -10.0,
      -1.0,
      kValue.f32.negative.max,
      kValue.f32.subnormal.negative.min,
      kValue.f32.subnormal.negative.max,
      0.0,
      kValue.f32.subnormal.positive.min,
      kValue.f32.subnormal.positive.max,
      kValue.f32.positive.min,
      1.0,
      10.0,
      kValue.f32.positive.max,
      Number.POSITIVE_INFINITY,
    ];

    const cases: Array<Case> = [];
    values.forEach(x => {
      values.forEach(y => {
        values.forEach(z => {
          cases.push(makeCase(x, y, z));
        });
      });
    });
    run(t, builtin('mix'), [TypeF32, TypeF32, TypeF32], TypeF32, t.params, cases);
  });

g.test('matching_f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f16 tests with matching third param`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();

g.test('nonmatching_abstract_float')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`abstract float tests with vector params and scalar third param`)
  .params(u => u.combine('inputSource', allInputSources).combine('vectorize', [2, 3, 4] as const))
  .unimplemented();

g.test('nonmatching_f32')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f32 tests with vector params and scalar third param`)
  .params(u => u.combine('inputSource', allInputSources).combine('vectorize', [2, 3, 4] as const))
  .unimplemented();

g.test('monmatching_f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f16 tests with vector params and scalar third param`)
  .params(u => u.combine('inputSource', allInputSources).combine('vectorize', [2, 3, 4] as const))
  .unimplemented();
