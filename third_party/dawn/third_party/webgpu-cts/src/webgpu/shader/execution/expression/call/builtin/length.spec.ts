export const description = `
Execution tests for the 'length' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn length(e: T ) -> f32
Returns the length of e (e.g. abs(e) if T is a scalar, or sqrt(e[0]^2 + e[1]^2 + ...) if T is a vector).
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { TypeF32, TypeVec } from '../../../../../util/conversion.js';
import { lengthInterval } from '../../../../../util/f32_interval.js';
import { fullF32Range, vectorTestValues } from '../../../../../util/math.js';
import { makeCaseCache } from '../../case_cache.js';
import {
  allInputSources,
  Case,
  makeUnaryToF32IntervalCase,
  makeVectorToF32IntervalCase,
  run,
} from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

export const d = makeCaseCache('length', {
  f32: () => {
    const makeCase = (x: number): Case => {
      return makeUnaryToF32IntervalCase(x, lengthInterval);
    };
    return fullF32Range().map(makeCase);
  },
  f32_vec2: () => {
    return vectorTestValues(2, false).map(makeCaseVecF32);
  },
  f32_vec3: () => {
    return vectorTestValues(3, false).map(makeCaseVecF32);
  },
  f32_vec4: () => {
    return vectorTestValues(4, false).map(makeCaseVecF32);
  },
});

/** @returns a `length` Case for a vector of f32s input */
const makeCaseVecF32 = (x: number[]): Case => {
  return makeVectorToF32IntervalCase(x, lengthInterval);
};

g.test('abstract_float')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`abstract float tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();

g.test('f32')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f32 tests`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases = await d.get('f32');
    await run(t, builtin('length'), [TypeF32], TypeF32, t.params, cases);
  });

g.test('f32_vec2')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f32 tests using vec2s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases = await d.get('f32_vec2');
    await run(t, builtin('length'), [TypeVec(2, TypeF32)], TypeF32, t.params, cases);
  });

g.test('f32_vec3')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f32 tests using vec3s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases = await d.get('f32_vec3');
    await run(t, builtin('length'), [TypeVec(3, TypeF32)], TypeF32, t.params, cases);
  });

g.test('f32_vec4')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f32 tests using vec4s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases = await d.get('f32_vec4');
    await run(t, builtin('length'), [TypeVec(4, TypeF32)], TypeF32, t.params, cases);
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f16 tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();
