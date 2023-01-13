export const description = `
Execution tests for the 'faceForward' builtin function

T is vecN<AbstractFloat>, vecN<f32>, or vecN<f16>
@const fn faceForward(e1: T ,e2: T ,e3: T ) -> T
Returns e1 if dot(e2,e3) is negative, and -e1 otherwise.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { anyOf } from '../../../../../util/compare.js';
import { f32, TypeF32, TypeVec, Vector } from '../../../../../util/conversion.js';
import { faceForwardIntervals } from '../../../../../util/f32_interval.js';
import { kVectorSparseTestValues, quantizeToF32 } from '../../../../../util/math.js';
import { allInputSources, Case, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

/**
 * @returns a `faceForward` Case for a triplet of vectors of f32s input
 *
 * Needs to be a custom implementation, since faceFowardIntervals returns an
 * array of vector of intervals, which are to be treated as discrete
 * possibilities.
 */
const makeCase = (x: number[], y: number[], z: number[]): Case => {
  x = x.map(quantizeToF32);
  y = y.map(quantizeToF32);
  z = z.map(quantizeToF32);

  const x_f32 = x.map(f32);
  const y_f32 = y.map(f32);
  const z_f32 = z.map(f32);

  const intervals = faceForwardIntervals(x, y, z);

  return {
    input: [new Vector(x_f32), new Vector(y_f32), new Vector(z_f32)],
    expected: anyOf(...intervals),
  };
};

g.test('abstract_float')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`abstract float tests`)
  .params(u => u.combine('inputSource', allInputSources).combine('vectorize', [2, 3, 4] as const))
  .unimplemented();

g.test('f32_vec2')
  .specURL('https://www.w3.org/TR/WGSL/#numeric-builtin-functions')
  .desc(`f32 tests using vec2s`)
  .params(u => u.combine('inputSource', allInputSources))
  .fn(async t => {
    const cases: Case[] = kVectorSparseTestValues[2].flatMap(i =>
      kVectorSparseTestValues[2].flatMap(j =>
        kVectorSparseTestValues[2].map(k => makeCase(i, j, k))
      )
    );

    await run(
      t,
      builtin('faceForward'),
      [TypeVec(2, TypeF32), TypeVec(2, TypeF32), TypeVec(2, TypeF32)],
      TypeVec(2, TypeF32),
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
      kVectorSparseTestValues[3].flatMap(j =>
        kVectorSparseTestValues[3].map(k => makeCase(i, j, k))
      )
    );

    await run(
      t,
      builtin('faceForward'),
      [TypeVec(3, TypeF32), TypeVec(3, TypeF32), TypeVec(3, TypeF32)],
      TypeVec(3, TypeF32),
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
      kVectorSparseTestValues[4].flatMap(j =>
        kVectorSparseTestValues[4].map(k => makeCase(i, j, k))
      )
    );

    await run(
      t,
      builtin('faceForward'),
      [TypeVec(4, TypeF32), TypeVec(4, TypeF32), TypeVec(4, TypeF32)],
      TypeVec(4, TypeF32),
      t.params,
      cases
    );
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f16 tests`)
  .params(u => u.combine('inputSource', allInputSources).combine('vectorize', [2, 3, 4] as const))
  .unimplemented();
