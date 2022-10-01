export const description = `
Execution tests for the 'clamp' builtin function

S is AbstractInt, i32, or u32
T is S or vecN<S>
@const fn clamp(e: T , low: T, high: T) -> T
Returns min(max(e,low),high). Component-wise when T is a vector.

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const clamp(e: T , low: T , high: T) -> T
Returns either min(max(e,low),high), or the median of the three values e, low, high.
Component-wise when T is a vector.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { kBit, kValue } from '../../../../../util/constants.js';
import {
  i32,
  i32Bits,
  Scalar,
  TypeF32,
  TypeI32,
  TypeU32,
  u32,
  u32Bits,
} from '../../../../../util/conversion.js';
import { clampIntervals } from '../../../../../util/f32_interval.js';
import { allInputSources, Case, makeTernaryF32IntervalCase, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

/**
 * Calculates clamp using the min-max formula.
 * clamp(e, f, g) = min(max(e, f), g)
 *
 * Operates on indices of an ascending sorted array, instead of the actual
 * values to avoid rounding issues.
 *
 * @returns the index of the clamped value
 */
function calculateMinMaxClamp(ei: number, fi: number, gi: number): number {
  return Math.min(Math.max(ei, fi), gi);
}

/** @returns a set of clamp test cases from an ascending list of integer values */
function generateIntegerTestCases(test_values: Array<Scalar>): Array<Case> {
  const cases = new Array<Case>();
  test_values.forEach((e, ei) => {
    test_values.forEach((f, fi) => {
      test_values.forEach((g, gi) => {
        const expected_idx = calculateMinMaxClamp(ei, fi, gi);
        const expected = test_values[expected_idx];
        cases.push({ input: [e, f, g], expected });
      });
    });
  });
  return cases;
}

g.test('abstract_int')
  .specURL('https://www.w3.org/TR/WGSL/#integer-builtin-functions')
  .desc(`abstract int tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();

g.test('u32')
  .specURL('https://www.w3.org/TR/WGSL/#integer-builtin-functions')
  .desc(`u32 tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    // This array must be strictly increasing, since that ordering determines
    // the expected values.
    const test_values: Array<Scalar> = [
      u32Bits(kBit.u32.min),
      u32(1),
      u32(2),
      u32(0x70000000),
      u32(0x80000000),
      u32Bits(kBit.u32.max),
    ];

    run(
      t,
      builtin('clamp'),
      [TypeU32, TypeU32, TypeU32],
      TypeU32,
      t.params,
      generateIntegerTestCases(test_values)
    );
  });

g.test('i32')
  .specURL('https://www.w3.org/TR/WGSL/#integer-builtin-functions')
  .desc(`i32 tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    // This array must be strictly increasing, since that ordering determines
    // the expected values.
    const test_values: Array<Scalar> = [
      i32Bits(kBit.i32.negative.min),
      i32(-2),
      i32(-1),
      i32(0),
      i32(1),
      i32(2),
      i32Bits(0x70000000),
      i32Bits(kBit.i32.positive.max),
    ];

    run(
      t,
      builtin('clamp'),
      [TypeI32, TypeI32, TypeI32],
      TypeI32,
      t.params,
      generateIntegerTestCases(test_values)
    );
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
    const makeCase = (x: number, y: number, z: number): Case => {
      return makeTernaryF32IntervalCase(x, y, z, ...clampIntervals);
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

    const cases: Array<Case> = new Array<Case>();
    values.forEach(x => {
      values.forEach(y => {
        values.forEach(z => {
          cases.push(makeCase(x, y, z));
        });
      });
    });

    run(t, builtin('clamp'), [TypeF32, TypeF32, TypeF32], TypeF32, t.params, cases);
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f16 tests`)
  .params(u =>
    u.combine('inputSource', allInputSources).combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();
