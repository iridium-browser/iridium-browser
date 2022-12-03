export const description = `
Execution tests for the 'normalize' builtin function

T is AbstractFloat, f32, or f16
@const fn normalize(e: vecN<T> ) -> vecN<T>
Returns a unit vector in the same direction as e.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { TypeF32, TypeVec } from '../../../../../util/conversion.js';
import { normalizeInterval } from '../../../../../util/f32_interval.js';
import { kVectorTestValues } from '../../../../../util/math.js';
import { allInputSources, Case, makeVectorToVectorIntervalCase, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

/** @returns a `normalize` Case for a vector of f32s input */
const makeCaseVecF32 = (x: number[]): Case => {
  return makeVectorToVectorIntervalCase(x, normalizeInterval);
};

g.test('abstract_float')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`abstract float tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();

g.test('f32_vec2')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f32 tests using vec2s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases: Case[] = kVectorTestValues[2].map(makeCaseVecF32);

    await run(t, builtin('normalize'), [TypeVec(2, TypeF32)], TypeVec(2, TypeF32), t.params, cases);
  });

g.test('f32_vec3')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f32 tests using vec3s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases: Case[] = kVectorTestValues[3].map(makeCaseVecF32);

    await run(t, builtin('normalize'), [TypeVec(3, TypeF32)], TypeVec(3, TypeF32), t.params, cases);
  });

g.test('f32_vec4')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f32 tests using vec4s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases: Case[] = kVectorTestValues[4].map(makeCaseVecF32);

    await run(t, builtin('normalize'), [TypeVec(4, TypeF32)], TypeVec(4, TypeF32), t.params, cases);
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f16 tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();
