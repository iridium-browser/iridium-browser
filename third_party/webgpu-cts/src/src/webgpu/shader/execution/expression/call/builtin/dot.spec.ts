export const description = `
Execution tests for the 'dot' builtin function

T is AbstractInt, AbstractFloat, i32, u32, f32, or f16
@const fn dot(e1: vecN<T>,e2: vecN<T>) -> T
Returns the dot product of e1 and e2.

`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { kValue } from '../../../../../util/constants.js';
import { TypeF32, TypeVec } from '../../../../../util/conversion.js';
import { dotInterval } from '../../../../../util/f32_interval.js';
import { allInputSources, Case, makeVectorPairF32IntervalCase, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

/** Values of interest to test against */
const kFloatTestValues: Array<number> = [
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

/**
 * Set of vectors, indexed by dimension, that contain interesting float values
 *
 * The tests do not do the simple option for coverage of computing the cartesian
 * product of all of the interesting float values N times for vecN tests,
 * because that creates a huge number of tests for vec3 and vec4, leading to
 * time outs.
 * Instead they insert the interesting f32 values into each location of the
 * vector to get a spread of testing over the entire range. This reduces the
 * number of cases being run substantially, but maintains coverage.
 */
const kVectorTestValues = {
  2: kFloatTestValues.flatMap(f => [
    [f, 1.0],
    [1.0, f],
  ]),
  3: kFloatTestValues.flatMap(f => [
    [f, 1.0, 2.0],
    [1.0, f, 2.0],
    [1.0, 2.0, f],
  ]),
  4: kFloatTestValues.flatMap(f => [
    [f, 1.0, 2.0, 3.0],
    [1.0, f, 2.0, 3.0],
    [1.0, 2.0, f, 3.0],
    [1.0, 2.0, 3.0, f],
  ]),
};

g.test('abstract_int')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`abstract int tests`)
  .params(u => u.combine('inputSource', allInputSources))
  .unimplemented();

g.test('i32')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`i32 tests`)
  .params(u => u.combine('inputSource', allInputSources))
  .unimplemented();

g.test('u32')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`u32 tests`)
  .params(u => u.combine('inputSource', allInputSources))
  .unimplemented();

g.test('abstract_float')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`abstract float test`)
  .params(u => u.combine('inputSource', allInputSources))
  .unimplemented();

g.test('f32_vec2')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`f32 tests using vec2s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const makeCase = (x: number[], y: number[]): Case => {
      return makeVectorPairF32IntervalCase(x, y, dotInterval);
    };

    const cases: Case[] = kVectorTestValues[2].flatMap(i => {
      return kVectorTestValues[2].map(j => {
        return makeCase(i, j);
      });
    });

    run(t, builtin('dot'), [TypeVec(2, TypeF32), TypeVec(2, TypeF32)], TypeF32, t.params, cases);
  });

g.test('f32_vec3')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`f32 tests using vec3s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const makeCase = (x: number[], y: number[]): Case => {
      return makeVectorPairF32IntervalCase(x, y, dotInterval);
    };

    const cases: Case[] = kVectorTestValues[3].flatMap(i => {
      return kVectorTestValues[3].map(j => {
        return makeCase(i, j);
      });
    });

    run(t, builtin('dot'), [TypeVec(3, TypeF32), TypeVec(3, TypeF32)], TypeF32, t.params, cases);
  });

g.test('f32_vec4')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`f32 tests using vec3s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const makeCase = (x: number[], y: number[]): Case => {
      return makeVectorPairF32IntervalCase(x, y, dotInterval);
    };

    const cases: Case[] = kVectorTestValues[4].flatMap(i => {
      return kVectorTestValues[4].map(j => {
        return makeCase(i, j);
      });
    });

    run(t, builtin('dot'), [TypeVec(4, TypeF32), TypeVec(4, TypeF32)], TypeF32, t.params, cases);
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`f16 tests`)
  .params(u => u.combine('inputSource', allInputSources))
  .unimplemented();
