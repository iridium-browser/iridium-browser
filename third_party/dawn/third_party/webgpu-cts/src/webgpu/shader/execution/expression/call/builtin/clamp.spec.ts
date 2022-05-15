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
import { anyOf, correctlyRoundedMatch } from '../../../../../util/compare.js';
import { kBit } from '../../../../../util/constants.js';
import {
  f32,
  f32Bits,
  i32,
  i32Bits,
  Scalar,
  TypeF32,
  TypeI32,
  TypeU32,
  u32,
  u32Bits,
} from '../../../../../util/conversion.js';
import { isSubnormalScalar } from '../../../../../util/math.js';
import { Case, Config, run } from '../../expression.js';

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

/**
 * Calculates clamp as the median of three numbers
 *
 * Operates on indices of an ascending sorted array, instead of the actual
 * values to avoid rounding issues.
 *
 * @returns the index of the clamped value
 */
function calculateMedianClamp(ei: number, fi: number, gi: number): number {
  return [ei, fi, gi].sort((a, b) => {
    return a - b;
  })[1];
}

/** @returns a set of clamp test cases from an ascending list of integer values */
function generateIntegerTestCases(test_values: Array<Scalar>): Array<Case> {
  const cases = new Array<Case>();
  test_values.forEach((e, ei) => {
    test_values.forEach((f, fi) => {
      test_values.forEach((g, gi) => {
        const expected_idx = calculateMinMaxClamp(ei, fi, gi);
        const precise_expected = test_values[expected_idx];
        const expected = isSubnormalScalar(precise_expected)
          ? anyOf(precise_expected, f32(0.0))
          : precise_expected;
        cases.push({ input: [e, f, g], expected });
      });
    });
  });
  return cases;
}

/** @returns a set of clamp test cases from an ascending list of floating point values */
function generateFloatTestCases(test_values: Array<Scalar>): Array<Case> {
  const cases = new Array<Case>();
  test_values.forEach((e, ei) => {
    test_values.forEach((f, fi) => {
      test_values.forEach((g, gi) => {
        // Spec allows backends for floats to either return the min-max formula or median of 3 numbers
        const expected_values: Array<Scalar> = [];
        {
          const expected_idx = calculateMinMaxClamp(ei, fi, gi);
          const precise_expected = test_values[expected_idx];
          if (!expected_values.includes(precise_expected)) {
            expected_values.push(precise_expected);
          }
        }
        {
          const expected_idx = calculateMedianClamp(ei, fi, gi);
          const precise_expected = test_values[expected_idx];
          if (!expected_values.includes(precise_expected)) {
            expected_values.push(precise_expected);
          }
        }
        const contains_subnormals =
          expected_values.filter(x => {
            return isSubnormalScalar(x);
          }).length > 0;
        if (contains_subnormals) {
          expected_values.push(f32(0.0));
        }
        cases.push({ input: [e, f, g], expected: anyOf(...expected_values) });
      });
    });
  });
  return cases;
}

g.test('abstract_int')
  .specURL('https://www.w3.org/TR/WGSL/#integer-builtin-functions')
  .desc(`abstract int tests`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();

g.test('u32')
  .specURL('https://www.w3.org/TR/WGSL/#integer-builtin-functions')
  .desc(`u32 tests`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
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
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
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
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();

g.test('f32')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f32 tests`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const cfg: Config = t.params;
    cfg.cmpFloats = correctlyRoundedMatch();

    // This array must be strictly increasing, since that ordering determines
    // the expected values.
    const test_values: Array<Scalar> = [
      f32Bits(kBit.f32.infinity.negative),
      f32Bits(kBit.f32.negative.min),
      f32(-10.0),
      f32(-1.0),
      f32Bits(kBit.f32.negative.max),
      f32Bits(kBit.f32.subnormal.negative.min),
      f32Bits(kBit.f32.subnormal.negative.max),
      f32(0.0),
      f32Bits(kBit.f32.subnormal.positive.min),
      f32Bits(kBit.f32.subnormal.positive.max),
      f32Bits(kBit.f32.positive.min),
      f32(1.0),
      f32(10.0),
      f32Bits(kBit.f32.positive.max),
      f32Bits(kBit.f32.infinity.positive),
    ];

    run(
      t,
      builtin('clamp'),
      [TypeF32, TypeF32, TypeF32],
      TypeF32,
      cfg,
      generateFloatTestCases(test_values)
    );
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f16 tests`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();
