export const description = `
Execution tests for the 'refract' builtin function

T is vecN<I>
I is AbstractFloat, f32, or f16
@const fn refract(e1: T ,e2: T ,e3: I ) -> T
For the incident vector e1 and surface normal e2, and the ratio of indices of
refraction e3, let k = 1.0 -e3*e3* (1.0 - dot(e2,e1) * dot(e2,e1)).
If k < 0.0, returns the refraction vector 0.0, otherwise return the refraction
vector e3*e1- (e3* dot(e2,e1) + sqrt(k)) *e2.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { f32, TypeF32, TypeVec, Vector } from '../../../../../util/conversion.js';
import { refractInterval } from '../../../../../util/f32_interval.js';
import {
  kVectorSparseTestValues,
  quantizeToF32,
  sparseF32Range,
} from '../../../../../util/math.js';
import { allInputSources, Case, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

/** @returns a `reflect` Case for a pair of vectors of f32s and a f32 input */
const makeCaseF32 = (i: number[], s: number[], r: number): Case => {
  i = i.map(quantizeToF32);
  s = s.map(quantizeToF32);
  r = quantizeToF32(r);

  const i_f32 = i.map(f32);
  const s_f32 = s.map(f32);
  const r_f32 = f32(r);

  return {
    input: [new Vector(i_f32), new Vector(s_f32), r_f32],
    expected: refractInterval(i, s, r),
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
    const cases: Case[] = kVectorSparseTestValues[2].flatMap(i => {
      return kVectorSparseTestValues[2].flatMap(j => {
        return sparseF32Range(t.params.inputSource === 'const').map(k => {
          return makeCaseF32(i, j, k);
        });
      });
    });

    await run(
      t,
      builtin('refract'),
      [TypeVec(2, TypeF32), TypeVec(2, TypeF32), TypeF32],
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
    const cases: Case[] = kVectorSparseTestValues[3].flatMap(i => {
      return kVectorSparseTestValues[3].flatMap(j => {
        return sparseF32Range(t.params.inputSource === 'const').map(k => {
          return makeCaseF32(i, j, k);
        });
      });
    });

    await run(
      t,
      builtin('refract'),
      [TypeVec(3, TypeF32), TypeVec(3, TypeF32), TypeF32],
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
    const cases: Case[] = kVectorSparseTestValues[4].flatMap(i => {
      return kVectorSparseTestValues[4].flatMap(j => {
        return sparseF32Range(t.params.inputSource === 'const').map(k => {
          return makeCaseF32(i, j, k);
        });
      });
    });

    await run(
      t,
      builtin('refract'),
      [TypeVec(4, TypeF32), TypeVec(4, TypeF32), TypeF32],
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
