export const description = `
Execution tests for the 'distance' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn distance(e1: T ,e2: T ) -> f32
Returns the distance between e1 and e2 (e.g. length(e1-e2)).

`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { TypeF32, TypeVec } from '../../../../../util/conversion.js';
import { distanceInterval } from '../../../../../util/f32_interval.js';
import { fullF32Range, kVectorSparseTestValues } from '../../../../../util/math.js';
import {
  allInputSources,
  Case,
  makeBinaryToF32IntervalCase,
  makeVectorPairToF32IntervalCase,
  run,
} from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

/** @returns a `distance` Case for a pair of vectors of f32s input */
const makeCaseVecF32 = (x: number[], y: number[]): Case => {
  return makeVectorPairToF32IntervalCase(x, y, distanceInterval);
};

g.test('abstract_float')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
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
    const makeCase = (x: number, y: number): Case => {
      return makeBinaryToF32IntervalCase(x, y, distanceInterval);
    };
    const cases: Case[] = fullF32Range().flatMap(i => fullF32Range().map(j => makeCase(i, j)));

    await run(t, builtin('distance'), [TypeF32, TypeF32], TypeF32, t.params, cases);
  });

g.test('f32_vec2')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f32 tests using vec2s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases: Case[] = kVectorSparseTestValues[2].flatMap(i =>
      kVectorSparseTestValues[2].map(j => makeCaseVecF32(i, j))
    );

    await run(
      t,
      builtin('distance'),
      [TypeVec(2, TypeF32), TypeVec(2, TypeF32)],
      TypeF32,
      t.params,
      cases
    );
  });

g.test('f32_vec3')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f32 tests using vec3s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases: Case[] = kVectorSparseTestValues[3].flatMap(i =>
      kVectorSparseTestValues[3].map(j => makeCaseVecF32(i, j))
    );

    await run(
      t,
      builtin('distance'),
      [TypeVec(3, TypeF32), TypeVec(3, TypeF32)],
      TypeF32,
      t.params,
      cases
    );
  });

g.test('f32_vec4')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f32 tests using vec4s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases: Case[] = kVectorSparseTestValues[4].flatMap(i =>
      kVectorSparseTestValues[4].map(j => makeCaseVecF32(i, j))
    );

    await run(
      t,
      builtin('distance'),
      [TypeVec(4, TypeF32), TypeVec(4, TypeF32)],
      TypeF32,
      t.params,
      cases
    );
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f16 tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();
