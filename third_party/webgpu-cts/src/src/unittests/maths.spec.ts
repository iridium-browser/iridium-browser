export const description = `
Util math unit tests.
`;

import { makeTestGroup } from '../common/framework/test_group.js';
import { kBit, kValue } from '../webgpu/util/constants.js';
import { f32, f32Bits, float32ToUint32, Scalar } from '../webgpu/util/conversion.js';
import {
  biasedRange,
  correctlyRounded,
  diffULP,
  lerp,
  linearRange,
  nextAfter,
} from '../webgpu/util/math.js';

import { UnitTest } from './unit_test.js';

export const g = makeTestGroup(UnitTest);

/** Converts a 32-bit hex values to a 32-bit float value */
function hexToF32(hex: number): number {
  return new Float32Array(new Uint32Array([hex]).buffer)[0];
}

/** Converts two 32-bit hex values to a 64-bit float value */
function hexToFloat64(h32: number, l32: number): number {
  const u32Arr = new Uint32Array(2);
  u32Arr[0] = l32;
  u32Arr[1] = h32;
  const f64Arr = new Float64Array(u32Arr.buffer);
  return f64Arr[0];
}

/**
 * @returns true if arrays are equal, doing element-wise comparison as needed, and considering NaNs to be equal.
 *
 * Depends on the correctness of diffULP, which is tested in this file.
 **/
function compareArrayOfNumbers(got: Array<number>, expect: Array<number>): boolean {
  return (
    got.length === expect.length &&
    got.every((value, index) => {
      const expected = expect[index];
      return 1 >= diffULP(value, expected) || Number.isNaN(value && Number.isNaN(expected));
    })
  );
}

interface DiffULPCase {
  a: number;
  b: number;
  ulp: number;
}

g.test('test,math,diffULP')
  .paramsSimple<DiffULPCase>([
    { a: 0, b: 0, ulp: 0 },
    { a: 1, b: 2, ulp: 2 ** 23 }, // Single exponent step
    { a: 2, b: 1, ulp: 2 ** 23 }, // Single exponent step
    { a: 2, b: 4, ulp: 2 ** 23 }, // Single exponent step
    { a: 4, b: 2, ulp: 2 ** 23 }, // Single exponent step
    { a: -1, b: -2, ulp: 2 ** 23 }, // Single exponent step
    { a: -2, b: -1, ulp: 2 ** 23 }, // Single exponent step
    { a: -2, b: -4, ulp: 2 ** 23 }, // Single exponent step
    { a: -4, b: -2, ulp: 2 ** 23 }, // Single exponent step
    { a: 1, b: 4, ulp: 2 ** 24 }, // Double exponent step
    { a: 4, b: 1, ulp: 2 ** 24 }, // Double exponent step
    { a: -1, b: -4, ulp: 2 ** 24 }, // Double exponent step
    { a: -4, b: -1, ulp: 2 ** 24 }, // Double exponent step
    { a: hexToF32(0x00800000), b: hexToF32(0x00800001), ulp: 1 }, // Single mantissa step
    { a: hexToF32(0x00800001), b: hexToF32(0x00800000), ulp: 1 }, // Single mantissa step
    { a: hexToF32(0x03800000), b: hexToF32(0x03800001), ulp: 1 }, // Single mantissa step
    { a: hexToF32(0x03800001), b: hexToF32(0x03800000), ulp: 1 }, // Single mantissa step
    { a: -hexToF32(0x00800000), b: -hexToF32(0x00800001), ulp: 1 }, // Single mantissa step
    { a: -hexToF32(0x00800001), b: -hexToF32(0x00800000), ulp: 1 }, // Single mantissa step
    { a: -hexToF32(0x03800000), b: -hexToF32(0x03800001), ulp: 1 }, // Single mantissa step
    { a: -hexToF32(0x03800001), b: -hexToF32(0x03800000), ulp: 1 }, // Single mantissa step
    { a: hexToF32(0x00800000), b: hexToF32(0x00800002), ulp: 2 }, // Double mantissa step
    { a: hexToF32(0x00800002), b: hexToF32(0x00800000), ulp: 2 }, // Double mantissa step
    { a: hexToF32(0x03800000), b: hexToF32(0x03800002), ulp: 2 }, // Double mantissa step
    { a: hexToF32(0x03800002), b: hexToF32(0x03800000), ulp: 2 }, // Double mantissa step
    { a: -hexToF32(0x00800000), b: -hexToF32(0x00800002), ulp: 2 }, // Double mantissa step
    { a: -hexToF32(0x00800002), b: -hexToF32(0x00800000), ulp: 2 }, // Double mantissa step
    { a: -hexToF32(0x03800000), b: -hexToF32(0x03800002), ulp: 2 }, // Double mantissa step
    { a: -hexToF32(0x03800002), b: -hexToF32(0x03800000), ulp: 2 }, // Double mantissa step
    { a: hexToF32(0x00800000), b: 0, ulp: 1 }, // Normals near 0
    { a: 0, b: hexToF32(0x00800000), ulp: 1 }, // Normals near 0
    { a: -hexToF32(0x00800000), b: 0, ulp: 1 }, // Normals near 0
    { a: 0, b: -hexToF32(0x00800000), ulp: 1 }, // Normals near 0
    { a: hexToF32(0x00800000), b: -hexToF32(0x00800000), ulp: 2 }, // Normals around 0
    { a: -hexToF32(0x00800000), b: hexToF32(0x00800000), ulp: 2 }, // Normals around 0
    { a: hexToF32(0x00000001), b: 0, ulp: 0 }, // Subnormals near 0
    { a: 0, b: hexToF32(0x00000001), ulp: 0 }, // Subnormals near 0
    { a: -hexToF32(0x00000001), b: 0, ulp: 0 }, // Subnormals near 0
    { a: 0, b: -hexToF32(0x00000001), ulp: 0 }, // Subnormals near 0
    { a: hexToF32(0x00000001), b: -hexToF32(0x00000001), ulp: 0 }, // Subnormals near 0
    { a: -hexToF32(0x00000001), b: hexToF32(0x00000001), ulp: 0 }, // Subnormals near 0
    { a: hexToF32(0x00000001), b: hexToF32(0x00800000), ulp: 1 }, // Normal/Subnormal boundary
    { a: hexToF32(0x00800000), b: hexToF32(0x00000001), ulp: 1 }, // Normal/Subnormal boundary
    { a: -hexToF32(0x00000001), b: -hexToF32(0x00800000), ulp: 1 }, // Normal/Subnormal boundary
    { a: -hexToF32(0x00800000), b: -hexToF32(0x00000001), ulp: 1 }, // Normal/Subnormal boundary
    { a: hexToF32(0x00800001), b: hexToF32(0x00000000), ulp: 2 }, // Just-above-Normal/Subnormal boundary
    { a: hexToF32(0x00800001), b: hexToF32(0x00000001), ulp: 2 }, // Just-above-Normal/Subnormal boundary
    { a: hexToF32(0x00800005), b: hexToF32(0x00000001), ulp: 6 }, // Just-above-Normal/Subnormal boundary
    { a: hexToF32(0x00800005), b: hexToF32(0x00000111), ulp: 6 }, // Just-above-Normal/Subnormal boundary
  ])
  .fn(t => {
    const a = t.params.a;
    const b = t.params.b;
    const got = diffULP(a, b);
    const expect = t.params.ulp;
    t.expect(got === expect, `diffULP(${a}, ${b}) returned ${got}. Expected ${expect}`);
  });

interface nextAfterCase {
  val: number;
  dir: boolean;
  result: Scalar;
}

g.test('test,math,nextAfterFlushToZero')
  .paramsSubcasesOnly<nextAfterCase>(
    // prettier-ignore
    [
    // Edge Cases
    { val: Number.NaN, dir: true, result: f32Bits(0x7fffffff) },
    { val: Number.NaN, dir: false, result: f32Bits(0x7fffffff) },
    { val: Number.POSITIVE_INFINITY, dir: true, result: f32Bits(kBit.f32.infinity.positive) },
    { val: Number.POSITIVE_INFINITY, dir: false, result: f32Bits(kBit.f32.infinity.positive) },
    { val: Number.NEGATIVE_INFINITY, dir: true, result: f32Bits(kBit.f32.infinity.negative) },
    { val: Number.NEGATIVE_INFINITY, dir: false, result: f32Bits(kBit.f32.infinity.negative) },

    // Zeroes
    { val: +0, dir: true, result: f32Bits(kBit.f32.positive.min) },
    { val: +0, dir: false, result: f32Bits(kBit.f32.negative.max) },
    { val: -0, dir: true, result: f32Bits(kBit.f32.positive.min) },
    { val: -0, dir: false, result: f32Bits(kBit.f32.negative.max) },

    // Subnormals
    { val: hexToF32(kBit.f32.subnormal.positive.min), dir: true, result: f32Bits(kBit.f32.positive.min) },
    { val: hexToF32(kBit.f32.subnormal.positive.min), dir: false, result: f32Bits(kBit.f32.negative.max) },
    { val: hexToF32(kBit.f32.subnormal.positive.max), dir: true, result: f32Bits(kBit.f32.positive.min) },
    { val: hexToF32(kBit.f32.subnormal.positive.max), dir: false, result: f32Bits(kBit.f32.negative.max) },
    { val: hexToF32(kBit.f32.subnormal.negative.min), dir: true, result: f32Bits(kBit.f32.positive.min) },
    { val: hexToF32(kBit.f32.subnormal.negative.min), dir: false, result: f32Bits(kBit.f32.negative.max) },
    { val: hexToF32(kBit.f32.subnormal.negative.max), dir: true, result: f32Bits(kBit.f32.positive.min) },
    { val: hexToF32(kBit.f32.subnormal.negative.max), dir: false, result: f32Bits(kBit.f32.negative.max) },

    // Normals
    { val: hexToF32(kBit.f32.positive.max), dir: true, result: f32Bits(kBit.f32.infinity.positive) },
    { val: hexToF32(kBit.f32.positive.max), dir: false, result: f32Bits(0x7f7ffffe) },
    { val: hexToF32(kBit.f32.positive.min), dir: true, result: f32Bits(0x00800001) },
    { val: hexToF32(kBit.f32.positive.min), dir: false, result: f32(0) },
    { val: hexToF32(kBit.f32.negative.max), dir: true, result: f32(0) },
    { val: hexToF32(kBit.f32.negative.max), dir: false, result: f32Bits(0x80800001) },
    { val: hexToF32(kBit.f32.negative.min), dir: true, result: f32Bits(0xff7ffffe) },
    { val: hexToF32(kBit.f32.negative.min), dir: false, result: f32Bits(kBit.f32.infinity.negative) },
    { val: hexToF32(0x03800000), dir: true, result: f32Bits(0x03800001) },
    { val: hexToF32(0x03800000), dir: false, result: f32Bits(0x037fffff) },
    { val: hexToF32(0x83800000), dir: true, result: f32Bits(0x837fffff) },
    { val: hexToF32(0x83800000), dir: false, result: f32Bits(0x83800001) },
  ]
  )
  .fn(t => {
    const val = t.params.val;
    const dir = t.params.dir;
    const expect = t.params.result;
    const expect_type = typeof expect;
    const got = nextAfter(val, dir, true);
    const got_type = typeof got;
    t.expect(
      got.value === expect.value || (Number.isNaN(got.value) && Number.isNaN(expect.value)),
      `nextAfter(${val}, ${dir}, true) returned ${got} (${got_type}). Expected ${expect} (${expect_type})`
    );
  });

g.test('test,math,nextAfterNoFlush')
  .paramsSubcasesOnly<nextAfterCase>(
    // prettier-ignore
    [
    // Edge Cases
    { val: Number.NaN, dir: true, result: f32Bits(0x7fffffff) },
    { val: Number.NaN, dir: false, result: f32Bits(0x7fffffff) },
    { val: Number.POSITIVE_INFINITY, dir: true, result: f32Bits(kBit.f32.infinity.positive) },
    { val: Number.POSITIVE_INFINITY, dir: false, result: f32Bits(kBit.f32.infinity.positive) },
    { val: Number.NEGATIVE_INFINITY, dir: true, result: f32Bits(kBit.f32.infinity.negative) },
    { val: Number.NEGATIVE_INFINITY, dir: false, result: f32Bits(kBit.f32.infinity.negative) },

    // Zeroes
    { val: +0, dir: true, result: f32Bits(kBit.f32.subnormal.positive.min) },
    { val: +0, dir: false, result: f32Bits(kBit.f32.subnormal.negative.max) },
    { val: -0, dir: true, result: f32Bits(kBit.f32.subnormal.positive.min) },
    { val: -0, dir: false, result: f32Bits(kBit.f32.subnormal.negative.max) },

    // Subnormals
    { val: hexToF32(kBit.f32.subnormal.positive.min), dir: true, result: f32Bits(0x00000002) },
    { val: hexToF32(kBit.f32.subnormal.positive.min), dir: false, result: f32(0) },
    { val: hexToF32(kBit.f32.subnormal.positive.max), dir: true, result: f32Bits(kBit.f32.positive.min) },
    { val: hexToF32(kBit.f32.subnormal.positive.max), dir: false, result: f32Bits(0x007ffffe) },
    { val: hexToF32(kBit.f32.subnormal.negative.min), dir: true, result: f32Bits(0x807ffffe) },
    { val: hexToF32(kBit.f32.subnormal.negative.min), dir: false, result: f32Bits(kBit.f32.negative.max) },
    { val: hexToF32(kBit.f32.subnormal.negative.max), dir: true, result: f32(0) },
    { val: hexToF32(kBit.f32.subnormal.negative.max), dir: false, result: f32Bits(0x80000002) },

    // Normals
    { val: hexToF32(kBit.f32.positive.max), dir: true, result: f32Bits(kBit.f32.infinity.positive) },
    { val: hexToF32(kBit.f32.positive.max), dir: false, result: f32Bits(0x7f7ffffe) },
    { val: hexToF32(kBit.f32.positive.min), dir: true, result: f32Bits(0x00800001) },
    { val: hexToF32(kBit.f32.positive.min), dir: false, result: f32Bits(kBit.f32.subnormal.positive.max) },
    { val: hexToF32(kBit.f32.negative.max), dir: true, result: f32Bits(kBit.f32.subnormal.negative.min) },
    { val: hexToF32(kBit.f32.negative.max), dir: false, result: f32Bits(0x80800001) },
    { val: hexToF32(kBit.f32.negative.min), dir: true, result: f32Bits(0xff7ffffe) },
    { val: hexToF32(kBit.f32.negative.min), dir: false, result: f32Bits(kBit.f32.infinity.negative) },
    { val: hexToF32(0x03800000), dir: true, result: f32Bits(0x03800001) },
    { val: hexToF32(0x03800000), dir: false, result: f32Bits(0x037fffff) },
    { val: hexToF32(0x83800000), dir: true, result: f32Bits(0x837fffff) },
    { val: hexToF32(0x83800000), dir: false, result: f32Bits(0x83800001) },
  ]
  )
  .fn(t => {
    const val = t.params.val;
    const dir = t.params.dir;
    const expect = t.params.result;
    const expect_type = typeof expect;
    const got = nextAfter(val, dir, false);
    const got_type = typeof got;
    t.expect(
      got.value === expect.value || (Number.isNaN(got.value) && Number.isNaN(expect.value)),
      `nextAfter(${val}, ${dir}, false) returned ${got} (${got_type}). Expected ${expect} (${expect_type})`
    );
  });

interface correctlyRoundedCase {
  test_val: Scalar;
  target: number;
  is_correct: boolean;
}

g.test('test,math,correctlyRounded')
  .paramsSubcasesOnly<correctlyRoundedCase>(
    // prettier-ignore
    [
    // NaN Cases
    { test_val: f32Bits(kBit.f32.nan.positive.s), target: NaN, is_correct: true },
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: NaN, is_correct: true },
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: NaN, is_correct: true },
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: NaN, is_correct: true },
    { test_val: f32Bits(0x7fffffff), target: NaN, is_correct: true },
    { test_val: f32Bits(0xffffffff), target: NaN, is_correct: true },
    { test_val: f32Bits(kBit.f32.infinity.positive), target: NaN, is_correct: false },
    { test_val: f32Bits(kBit.f32.infinity.negative), target: NaN, is_correct: false },
    { test_val: f32Bits(kBit.f32.positive.zero), target: NaN, is_correct: false },
    { test_val: f32Bits(kBit.f32.negative.zero), target: NaN, is_correct: false },

    // Infinities
    { test_val: f32Bits(kBit.f32.nan.positive.s), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0x7fffffff), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0xffffffff), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.infinity.positive), target: Number.POSITIVE_INFINITY, is_correct: true },
    { test_val: f32Bits(kBit.f32.infinity.negative), target: Number.POSITIVE_INFINITY, is_correct: false },

    { test_val: f32Bits(kBit.f32.nan.positive.s), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0x7fffffff), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0xffffffff), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.infinity.positive), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.infinity.negative), target: Number.NEGATIVE_INFINITY, is_correct: true },

    // Zeros
    { test_val: f32Bits(kBit.f32.positive.zero), target: 0, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: 0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: 0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: 0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: 0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: 0, is_correct: true },

    { test_val: f32Bits(kBit.f32.positive.zero), target: -0, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: -0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: -0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: -0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: -0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: -0, is_correct: true },

    // 32-bit subnormals
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },

    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },

    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },

    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },

    // 64-bit subnormals
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },

    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },

    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },

    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },

    // 32-bit normals
    { test_val: f32Bits(kBit.f32.positive.max), target: hexToF32(kBit.f32.positive.max), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.min), target: hexToF32(kBit.f32.positive.min), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.max), target: hexToF32(kBit.f32.negative.max), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.min), target: hexToF32(kBit.f32.negative.min), is_correct: true },
    { test_val: f32Bits(0x03800000), target: hexToF32(0x03800000), is_correct: true },
    { test_val: f32Bits(0x03800000), target: hexToF32(0x03800002), is_correct: false },
    { test_val: f32Bits(0x03800001), target: hexToF32(0x03800001), is_correct: true },
    { test_val: f32Bits(0x03800001), target: hexToF32(0x03800010), is_correct: false },
    { test_val: f32Bits(0x83800000), target: hexToF32(0x83800000), is_correct: true },
    { test_val: f32Bits(0x83800000), target: hexToF32(0x83800002), is_correct: false },
    { test_val: f32Bits(0x83800001), target: hexToF32(0x83800001), is_correct: true },
    { test_val: f32Bits(0x83800001), target: hexToF32(0x83800010), is_correct: false },

    // 64-bit normals
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00000, 0x00000001), is_correct: true },
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00010, 0x00000001), is_correct: false },
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00000, 0x00000001), is_correct: true },
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00020, 0x00000001), is_correct: false },
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00000, 0x00000002), is_correct: true },
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00030, 0x00000002), is_correct: false },
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00000, 0x00000002), is_correct: true },
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00040, 0x00000002), is_correct: false },
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00000, 0x00000001), is_correct: true },
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00050, 0x00000001), is_correct: false },
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00000, 0x00000001), is_correct: true },
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00060, 0x00000001), is_correct: false },
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00000, 0x00000002), is_correct: true },
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00070, 0x00000002), is_correct: false },
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00000, 0x00000002), is_correct: true },
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00080, 0x00000002), is_correct: false },
  ]
  )
  .fn(t => {
    const test_val = t.params.test_val;
    const target = t.params.target;
    const is_correct = t.params.is_correct;

    const got = correctlyRounded(test_val, target);
    t.expect(
      got === is_correct,
      `correctlyRounded(${test_val}, ${target}) returned ${got}. Expected ${is_correct}`
    );
  });

g.test('test,math,correctlyRoundedNoFlushOnly')
  .paramsSubcasesOnly<correctlyRoundedCase>(
    // prettier-ignore
    [
    // NaN Cases
    { test_val: f32Bits(kBit.f32.nan.positive.s), target: NaN, is_correct: true },
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: NaN, is_correct: true },
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: NaN, is_correct: true },
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: NaN, is_correct: true },
    { test_val: f32Bits(0x7fffffff), target: NaN, is_correct: true },
    { test_val: f32Bits(0xffffffff), target: NaN, is_correct: true },
    { test_val: f32Bits(kBit.f32.infinity.positive), target: NaN, is_correct: false },
    { test_val: f32Bits(kBit.f32.infinity.negative), target: NaN, is_correct: false },
    { test_val: f32Bits(kBit.f32.positive.zero), target: NaN, is_correct: false },
    { test_val: f32Bits(kBit.f32.negative.zero), target: NaN, is_correct: false },

    // Infinities
    { test_val: f32Bits(kBit.f32.nan.positive.s), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0x7fffffff), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0xffffffff), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.infinity.positive), target: Number.POSITIVE_INFINITY, is_correct: true },
    { test_val: f32Bits(kBit.f32.infinity.negative), target: Number.POSITIVE_INFINITY, is_correct: false },

    { test_val: f32Bits(kBit.f32.nan.positive.s), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0x7fffffff), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0xffffffff), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.infinity.positive), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.infinity.negative), target: Number.NEGATIVE_INFINITY, is_correct: true },

    // Zeros
    { test_val: f32Bits(kBit.f32.positive.zero), target: 0, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: 0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: 0, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: 0, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: 0, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: 0, is_correct: false },

    { test_val: f32Bits(kBit.f32.positive.zero), target: -0, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: -0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: -0, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: -0, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: -0, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: -0, is_correct: false },

    // 32-bit subnormals
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: false },

    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: false },

    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: false },

    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },

    // 64-bit subnormals
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },

    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },

    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },

    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },

    // 32-bit normals
    { test_val: f32Bits(kBit.f32.positive.max), target: hexToF32(kBit.f32.positive.max), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.min), target: hexToF32(kBit.f32.positive.min), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.max), target: hexToF32(kBit.f32.negative.max), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.min), target: hexToF32(kBit.f32.negative.min), is_correct: true },
    { test_val: f32Bits(0x03800000), target: hexToF32(0x03800000), is_correct: true },
    { test_val: f32Bits(0x03800000), target: hexToF32(0x03800002), is_correct: false },
    { test_val: f32Bits(0x03800001), target: hexToF32(0x03800001), is_correct: true },
    { test_val: f32Bits(0x03800001), target: hexToF32(0x03800010), is_correct: false },
    { test_val: f32Bits(0x83800000), target: hexToF32(0x83800000), is_correct: true },
    { test_val: f32Bits(0x83800000), target: hexToF32(0x83800002), is_correct: false },
    { test_val: f32Bits(0x83800001), target: hexToF32(0x83800001), is_correct: true },
    { test_val: f32Bits(0x83800001), target: hexToF32(0x83800010), is_correct: false },

    // 64-bit normals
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00000, 0x00000001), is_correct: true },
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00010, 0x00000001), is_correct: false },
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00000, 0x00000001), is_correct: true },
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00020, 0x00000001), is_correct: false },
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00000, 0x00000002), is_correct: true },
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00030, 0x00000002), is_correct: false },
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00000, 0x00000002), is_correct: true },
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00040, 0x00000002), is_correct: false },
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00000, 0x00000001), is_correct: true },
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00050, 0x00000001), is_correct: false },
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00000, 0x00000001), is_correct: true },
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00060, 0x00000001), is_correct: false },
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00000, 0x00000002), is_correct: true },
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00070, 0x00000002), is_correct: false },
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00000, 0x00000002), is_correct: true },
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00080, 0x00000002), is_correct: false },
  ]
  )
  .fn(t => {
    const test_val = t.params.test_val;
    const target = t.params.target;
    const is_correct = t.params.is_correct;

    const got = correctlyRounded(test_val, target, false, true);
    t.expect(
      got === is_correct,
      `correctlyRounded(${test_val}, ${target}) returned ${got}. Expected ${is_correct}`
    );
  });

g.test('test,math,correctlyRoundedFlushToZeroOnly')
  .paramsSubcasesOnly<correctlyRoundedCase>(
    // prettier-ignore
    [
    // NaN Cases
    { test_val: f32Bits(kBit.f32.nan.positive.s), target: NaN, is_correct: true },
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: NaN, is_correct: true },
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: NaN, is_correct: true },
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: NaN, is_correct: true },
    { test_val: f32Bits(0x7fffffff), target: NaN, is_correct: true },
    { test_val: f32Bits(0xffffffff), target: NaN, is_correct: true },
    { test_val: f32Bits(kBit.f32.infinity.positive), target: NaN, is_correct: false },
    { test_val: f32Bits(kBit.f32.infinity.negative), target: NaN, is_correct: false },
    { test_val: f32Bits(kBit.f32.positive.zero), target: NaN, is_correct: false },
    { test_val: f32Bits(kBit.f32.negative.zero), target: NaN, is_correct: false },

    // Infinities
    { test_val: f32Bits(kBit.f32.nan.positive.s), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0x7fffffff), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0xffffffff), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.infinity.positive), target: Number.POSITIVE_INFINITY, is_correct: true },
    { test_val: f32Bits(kBit.f32.infinity.negative), target: Number.POSITIVE_INFINITY, is_correct: false },

    { test_val: f32Bits(kBit.f32.nan.positive.s), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0x7fffffff), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0xffffffff), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.infinity.positive), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(kBit.f32.infinity.negative), target: Number.NEGATIVE_INFINITY, is_correct: true },

    // Zeros
    { test_val: f32Bits(kBit.f32.positive.zero), target: 0, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: 0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: 0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: 0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: 0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: 0, is_correct: true },

    { test_val: f32Bits(kBit.f32.positive.zero), target: -0, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: -0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: -0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: -0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: -0, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: -0, is_correct: true },

    // 32-bit subnormals
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },

    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },

    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },

    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },

    // 64-bit subnormals
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },

    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },

    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },

    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },

    // 32-bit normals
    { test_val: f32Bits(kBit.f32.positive.max), target: hexToF32(kBit.f32.positive.max), is_correct: true },
    { test_val: f32Bits(kBit.f32.positive.min), target: hexToF32(kBit.f32.positive.min), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.max), target: hexToF32(kBit.f32.negative.max), is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.min), target: hexToF32(kBit.f32.negative.min), is_correct: true },
    { test_val: f32Bits(0x03800000), target: hexToF32(0x03800000), is_correct: true },
    { test_val: f32Bits(0x03800000), target: hexToF32(0x03800002), is_correct: false },
    { test_val: f32Bits(0x03800001), target: hexToF32(0x03800001), is_correct: true },
    { test_val: f32Bits(0x03800001), target: hexToF32(0x03800010), is_correct: false },
    { test_val: f32Bits(0x83800000), target: hexToF32(0x83800000), is_correct: true },
    { test_val: f32Bits(0x83800000), target: hexToF32(0x83800002), is_correct: false },
    { test_val: f32Bits(0x83800001), target: hexToF32(0x83800001), is_correct: true },
    { test_val: f32Bits(0x83800001), target: hexToF32(0x83800010), is_correct: false },

    // 64-bit normals
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00000, 0x00000001), is_correct: true },
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00010, 0x00000001), is_correct: false },
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00000, 0x00000001), is_correct: true },
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00020, 0x00000001), is_correct: false },
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00000, 0x00000002), is_correct: true },
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00030, 0x00000002), is_correct: false },
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00000, 0x00000002), is_correct: true },
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00040, 0x00000002), is_correct: false },
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00000, 0x00000001), is_correct: true },
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00050, 0x00000001), is_correct: false },
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00000, 0x00000001), is_correct: true },
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00060, 0x00000001), is_correct: false },
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00000, 0x00000002), is_correct: true },
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00070, 0x00000002), is_correct: false },
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00000, 0x00000002), is_correct: true },
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00080, 0x00000002), is_correct: false },
  ]
  )
  .fn(t => {
    const test_val = t.params.test_val;
    const target = t.params.target;
    const is_correct = t.params.is_correct;

    const got = correctlyRounded(test_val, target, true, false);
    t.expect(
      got === is_correct,
      `correctlyRounded(${test_val}, ${target}) returned ${got}. Expected ${is_correct}`
    );
  });

interface lerpCase {
  a: number;
  b: number;
  t: number;
  result: number;
}

g.test('test,math,lerp')
  .paramsSimple<lerpCase>([
    // Infinite cases
    { a: 0.0, b: Number.POSITIVE_INFINITY, t: 0.5, result: Number.NaN },
    { a: Number.POSITIVE_INFINITY, b: 0.0, t: 0.5, result: Number.NaN },
    { a: Number.NEGATIVE_INFINITY, b: 1.0, t: 0.5, result: Number.NaN },
    { a: 1.0, b: Number.NEGATIVE_INFINITY, t: 0.5, result: Number.NaN },
    { a: Number.NEGATIVE_INFINITY, b: Number.POSITIVE_INFINITY, t: 0.5, result: Number.NaN },
    { a: Number.POSITIVE_INFINITY, b: Number.NEGATIVE_INFINITY, t: 0.5, result: Number.NaN },
    { a: 0.0, b: 1.0, t: Number.NEGATIVE_INFINITY, result: Number.NaN },
    { a: 1.0, b: 0.0, t: Number.NEGATIVE_INFINITY, result: Number.NaN },
    { a: 0.0, b: 1.0, t: Number.POSITIVE_INFINITY, result: Number.NaN },
    { a: 1.0, b: 0.0, t: Number.POSITIVE_INFINITY, result: Number.NaN },

    // [0.0, 1.0] cases
    { a: 0.0, b: 1.0, t: -1.0, result: -1.0 },
    { a: 0.0, b: 1.0, t: 0.0, result: 0.0 },
    { a: 0.0, b: 1.0, t: 0.1, result: 0.1 },
    { a: 0.0, b: 1.0, t: 0.01, result: 0.01 },
    { a: 0.0, b: 1.0, t: 0.001, result: 0.001 },
    { a: 0.0, b: 1.0, t: 0.25, result: 0.25 },
    { a: 0.0, b: 1.0, t: 0.5, result: 0.5 },
    { a: 0.0, b: 1.0, t: 0.9, result: 0.9 },
    { a: 0.0, b: 1.0, t: 0.99, result: 0.99 },
    { a: 0.0, b: 1.0, t: 0.999, result: 0.999 },
    { a: 0.0, b: 1.0, t: 1.0, result: 1.0 },
    { a: 0.0, b: 1.0, t: 2.0, result: 2.0 },

    // [1.0, 0.0] cases
    { a: 1.0, b: 0.0, t: -1.0, result: 2.0 },
    { a: 1.0, b: 0.0, t: 0.0, result: 1.0 },
    { a: 1.0, b: 0.0, t: 0.1, result: 0.9 },
    { a: 1.0, b: 0.0, t: 0.01, result: 0.99 },
    { a: 1.0, b: 0.0, t: 0.001, result: 0.999 },
    { a: 1.0, b: 0.0, t: 0.25, result: 0.75 },
    { a: 1.0, b: 0.0, t: 0.5, result: 0.5 },
    { a: 1.0, b: 0.0, t: 0.9, result: 0.1 },
    { a: 1.0, b: 0.0, t: 0.99, result: 0.01 },
    { a: 1.0, b: 0.0, t: 0.999, result: 0.001 },
    { a: 1.0, b: 0.0, t: 1.0, result: 0.0 },
    { a: 1.0, b: 0.0, t: 2.0, result: -1.0 },

    // [0.0, 10.0] cases
    { a: 0.0, b: 10.0, t: -1.0, result: -10.0 },
    { a: 0.0, b: 10.0, t: 0.0, result: 0.0 },
    { a: 0.0, b: 10.0, t: 0.1, result: 1.0 },
    { a: 0.0, b: 10.0, t: 0.01, result: 0.1 },
    { a: 0.0, b: 10.0, t: 0.001, result: 0.01 },
    { a: 0.0, b: 10.0, t: 0.25, result: 2.5 },
    { a: 0.0, b: 10.0, t: 0.5, result: 5.0 },
    { a: 0.0, b: 10.0, t: 0.9, result: 9.0 },
    { a: 0.0, b: 10.0, t: 0.99, result: 9.9 },
    { a: 0.0, b: 10.0, t: 0.999, result: 9.99 },
    { a: 0.0, b: 10.0, t: 1.0, result: 10.0 },
    { a: 0.0, b: 10.0, t: 2.0, result: 20.0 },

    // [10.0, 0.0] cases
    { a: 10.0, b: 0.0, t: -1.0, result: 20.0 },
    { a: 10.0, b: 0.0, t: 0.0, result: 10.0 },
    { a: 10.0, b: 0.0, t: 0.1, result: 9 },
    { a: 10.0, b: 0.0, t: 0.01, result: 9.9 },
    { a: 10.0, b: 0.0, t: 0.001, result: 9.99 },
    { a: 10.0, b: 0.0, t: 0.25, result: 7.5 },
    { a: 10.0, b: 0.0, t: 0.5, result: 5.0 },
    { a: 10.0, b: 0.0, t: 0.9, result: 1.0 },
    { a: 10.0, b: 0.0, t: 0.99, result: 0.1 },
    { a: 10.0, b: 0.0, t: 0.999, result: 0.01 },
    { a: 10.0, b: 0.0, t: 1.0, result: 0.0 },
    { a: 10.0, b: 0.0, t: 2.0, result: -10.0 },

    // [2.0, 10.0] cases
    { a: 2.0, b: 10.0, t: -1.0, result: -6.0 },
    { a: 2.0, b: 10.0, t: 0.0, result: 2.0 },
    { a: 2.0, b: 10.0, t: 0.1, result: 2.8 },
    { a: 2.0, b: 10.0, t: 0.01, result: 2.08 },
    { a: 2.0, b: 10.0, t: 0.001, result: 2.008 },
    { a: 2.0, b: 10.0, t: 0.25, result: 4.0 },
    { a: 2.0, b: 10.0, t: 0.5, result: 6.0 },
    { a: 2.0, b: 10.0, t: 0.9, result: 9.2 },
    { a: 2.0, b: 10.0, t: 0.99, result: 9.92 },
    { a: 2.0, b: 10.0, t: 0.999, result: 9.992 },
    { a: 2.0, b: 10.0, t: 1.0, result: 10.0 },
    { a: 2.0, b: 10.0, t: 2.0, result: 18.0 },

    // [10.0, 2.0] cases
    { a: 10.0, b: 2.0, t: -1.0, result: 18.0 },
    { a: 10.0, b: 2.0, t: 0.0, result: 10.0 },
    { a: 10.0, b: 2.0, t: 0.1, result: 9.2 },
    { a: 10.0, b: 2.0, t: 0.01, result: 9.92 },
    { a: 10.0, b: 2.0, t: 0.001, result: 9.992 },
    { a: 10.0, b: 2.0, t: 0.25, result: 8.0 },
    { a: 10.0, b: 2.0, t: 0.5, result: 6.0 },
    { a: 10.0, b: 2.0, t: 0.9, result: 2.8 },
    { a: 10.0, b: 2.0, t: 0.99, result: 2.08 },
    { a: 10.0, b: 2.0, t: 0.999, result: 2.008 },
    { a: 10.0, b: 2.0, t: 1.0, result: 2.0 },
    { a: 10.0, b: 2.0, t: 2.0, result: -6.0 },

    // [-1.0, 1.0] cases
    { a: -1.0, b: 1.0, t: -2.0, result: -5.0 },
    { a: -1.0, b: 1.0, t: 0.0, result: -1.0 },
    { a: -1.0, b: 1.0, t: 0.1, result: -0.8 },
    { a: -1.0, b: 1.0, t: 0.01, result: -0.98 },
    { a: -1.0, b: 1.0, t: 0.001, result: -0.998 },
    { a: -1.0, b: 1.0, t: 0.25, result: -0.5 },
    { a: -1.0, b: 1.0, t: 0.5, result: 0.0 },
    { a: -1.0, b: 1.0, t: 0.9, result: 0.8 },
    { a: -1.0, b: 1.0, t: 0.99, result: 0.98 },
    { a: -1.0, b: 1.0, t: 0.999, result: 0.998 },
    { a: -1.0, b: 1.0, t: 1.0, result: 1.0 },
    { a: -1.0, b: 1.0, t: 2.0, result: 3.0 },

    // [1.0, -1.0] cases
    { a: 1.0, b: -1.0, t: -2.0, result: 5.0 },
    { a: 1.0, b: -1.0, t: 0.0, result: 1.0 },
    { a: 1.0, b: -1.0, t: 0.1, result: 0.8 },
    { a: 1.0, b: -1.0, t: 0.01, result: 0.98 },
    { a: 1.0, b: -1.0, t: 0.001, result: 0.998 },
    { a: 1.0, b: -1.0, t: 0.25, result: 0.5 },
    { a: 1.0, b: -1.0, t: 0.5, result: 0.0 },
    { a: 1.0, b: -1.0, t: 0.9, result: -0.8 },
    { a: 1.0, b: -1.0, t: 0.99, result: -0.98 },
    { a: 1.0, b: -1.0, t: 0.999, result: -0.998 },
    { a: 1.0, b: -1.0, t: 1.0, result: -1.0 },
    { a: 1.0, b: -1.0, t: 2.0, result: -3.0 },

    // [-1.0, 0.0] cases
    { a: -1.0, b: 0.0, t: -1.0, result: -2.0 },
    { a: -1.0, b: 0.0, t: 0.0, result: -1.0 },
    { a: -1.0, b: 0.0, t: 0.1, result: -0.9 },
    { a: -1.0, b: 0.0, t: 0.01, result: -0.99 },
    { a: -1.0, b: 0.0, t: 0.001, result: -0.999 },
    { a: -1.0, b: 0.0, t: 0.25, result: -0.75 },
    { a: -1.0, b: 0.0, t: 0.5, result: -0.5 },
    { a: -1.0, b: 0.0, t: 0.9, result: -0.1 },
    { a: -1.0, b: 0.0, t: 0.99, result: -0.01 },
    { a: -1.0, b: 0.0, t: 0.999, result: -0.001 },
    { a: -1.0, b: 0.0, t: 1.0, result: 0.0 },
    { a: -1.0, b: 0.0, t: 2.0, result: 1.0 },

    // [0.0, -1.0] cases
    { a: 0.0, b: -1.0, t: -1.0, result: 1.0 },
    { a: 0.0, b: -1.0, t: 0.0, result: 0.0 },
    { a: 0.0, b: -1.0, t: 0.1, result: -0.1 },
    { a: 0.0, b: -1.0, t: 0.01, result: -0.01 },
    { a: 0.0, b: -1.0, t: 0.001, result: -0.001 },
    { a: 0.0, b: -1.0, t: 0.25, result: -0.25 },
    { a: 0.0, b: -1.0, t: 0.5, result: -0.5 },
    { a: 0.0, b: -1.0, t: 0.9, result: -0.9 },
    { a: 0.0, b: -1.0, t: 0.99, result: -0.99 },
    { a: 0.0, b: -1.0, t: 0.999, result: -0.999 },
    { a: 0.0, b: -1.0, t: 1.0, result: -1.0 },
    { a: 0.0, b: -1.0, t: 2.0, result: -2.0 },
  ])
  .fn(test => {
    const a = test.params.a;
    const b = test.params.b;
    const t = test.params.t;
    const got = lerp(a, b, t);
    const expect = test.params.result;

    test.expect(
      1 >= diffULP(got, expect) || (Number.isNaN(got) && Number.isNaN(expect)),
      `lerp(${a}, ${b}, ${t}) returned ${got}. Expected ${expect}`
    );
  });

interface rangeCase {
  a: number;
  b: number;
  num_steps: number;
  result: Array<number>;
}

g.test('test,math,linearRange')
  .paramsSimple<rangeCase>(
    // prettier-ignore
    [
    { a: 0.0, b: Number.POSITIVE_INFINITY, num_steps: 10, result: new Array<number>(10).fill(Number.NaN) },
    { a: Number.POSITIVE_INFINITY, b: 0.0, num_steps: 10, result: new Array<number>(10).fill(Number.NaN) },
    { a: Number.NEGATIVE_INFINITY, b: 1.0, num_steps: 10, result: new Array<number>(10).fill(Number.NaN) },
    { a: 1.0, b: Number.NEGATIVE_INFINITY, num_steps: 10, result: new Array<number>(10).fill(Number.NaN) },
    { a: Number.NEGATIVE_INFINITY, b: Number.POSITIVE_INFINITY, num_steps: 10, result: new Array<number>(10).fill(Number.NaN) },
    { a: Number.POSITIVE_INFINITY, b: Number.NEGATIVE_INFINITY, num_steps: 10, result: new Array<number>(10).fill(Number.NaN) },
    { a: 0.0, b: 0.0, num_steps: 10, result: new Array<number>(10).fill(0.0) },
    { a: 10.0, b: 10.0, num_steps: 10, result: new Array<number>(10).fill(10.0) },
    { a: 0.0, b: 10.0, num_steps: 1, result: [0.0] },
    { a: 10.0, b: 0.0, num_steps: 1, result: [10] },
    { a: 0.0, b: 10.0, num_steps: 11, result: [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0] },
    { a: 10.0, b: 0.0, num_steps: 11, result: [10.0, 9.0, 8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0, 0.0] },
    { a: 0.0, b: 1000.0, num_steps: 11, result: [0.0, 100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 1000.0] },
    { a: 1000.0, b: 0.0, num_steps: 11, result: [1000.0, 900.0, 800.0, 700.0, 600.0, 500.0, 400.0, 300.0, 200.0, 100.0, 0.0] },
    { a: 1.0, b: 5.0, num_steps: 5, result: [1.0, 2.0, 3.0, 4.0, 5.0] },
    { a: 5.0, b: 1.0, num_steps: 5, result: [5.0, 4.0, 3.0, 2.0, 1.0] },
    { a: 0.0, b: 1.0, num_steps: 11, result: [0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0] },
    { a: 1.0, b: 0.0, num_steps: 11, result: [1.0, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1, 0.0] },
    { a: 0.0, b: 1.0, num_steps: 5, result: [0.0, 0.25, 0.5, 0.75, 1.0] },
    { a: 1.0, b: 0.0, num_steps: 5, result: [1.0, 0.75, 0.5, 0.25, 0.0] },
    { a: -1.0, b: 1.0, num_steps: 11, result: [-1.0, -0.8, -0.6, -0.4, -0.2, 0.0, 0.2, 0.4, 0.6, 0.8, 1.0] },
    { a: 1.0, b: -1.0, num_steps: 11, result: [1.0, 0.8, 0.6, 0.4, 0.2, 0.0, -0.2, -0.4, -0.6, -0.8, -1.0] },
    { a: -1.0, b: 0, num_steps: 11, result: [-1.0, -0.9, -0.8, -0.7, -0.6, -0.5, -0.4, -0.3, -0.2, -0.1, 0.0] },
    { a: 0.0, b: -1.0, num_steps: 11, result: [0.0, -0.1, -0.2, -0.3, -0.4, -0.5, -0.6, -0.7, -0.8, -0.9, -1.0] },
  ]
  )
  .fn(test => {
    const a = test.params.a;
    const b = test.params.b;
    const num_steps = test.params.num_steps;
    const got = linearRange(a, b, num_steps);
    const expect = test.params.result;

    test.expect(
      compareArrayOfNumbers(got, expect),
      `linearRange(${a}, ${b}, ${num_steps}) returned ${got}. Expected ${expect}`
    );
  });

g.test('test,math,biasedRange')
  .paramsSimple<rangeCase>(
    // prettier-ignore
    [
    { a: 0.0, b: Number.POSITIVE_INFINITY, num_steps: 10, result: new Array<number>(10).fill(Number.NaN) },
    { a: Number.POSITIVE_INFINITY, b: 0.0, num_steps: 10, result: new Array<number>(10).fill(Number.NaN) },
    { a: Number.NEGATIVE_INFINITY, b: 1.0, num_steps: 10, result: new Array<number>(10).fill(Number.NaN) },
    { a: 1.0, b: Number.NEGATIVE_INFINITY, num_steps: 10, result: new Array<number>(10).fill(Number.NaN) },
    { a: Number.NEGATIVE_INFINITY, b: Number.POSITIVE_INFINITY, num_steps: 10, result: new Array<number>(10).fill(Number.NaN) },
    { a: Number.POSITIVE_INFINITY, b: Number.NEGATIVE_INFINITY, num_steps: 10, result: new Array<number>(10).fill(Number.NaN) },
    { a: 0.0, b: 0.0, num_steps: 10, result: new Array<number>(10).fill(0.0) },
    { a: 10.0, b: 10.0, num_steps: 10, result: new Array<number>(10).fill(10.0) },
    { a: 0.0, b: 10.0, num_steps: 1, result: [0.0] },
    { a: 10.0, b: 0.0, num_steps: 1, result: [10.0] },
    { a: 0.0, b: 10.0, num_steps: 11, result: [0.0, 0.1, 0.4, 0.9, 1.6, 2.5, 3.6, 4.9, 6.4, 8.1, 10.0] },
    { a: 10.0, b: 0.0, num_steps: 11, result: [10.0, 9.9, 9.6, 9.1, 8.4, 7.5, 6.4, 5.1, 3.6, 1.9, 0.0] },
    { a: 0.0, b: 1000.0, num_steps: 11, result: [0.0, 10.0, 40.0, 90.0, 160.0, 250.0, 360.0, 490.0, 640.0, 810.0, 1000.0] },
    { a: 1000.0, b: 0.0, num_steps: 11, result: [1000.0, 990.0, 960.0, 910.0, 840.0, 750.0, 640.0, 510.0, 360.0, 190.0, 0.0] },
    { a: 1.0, b: 5.0, num_steps: 5, result: [1.0, 1.25, 2.0, 3.25, 5.0] },
    { a: 5.0, b: 1.0, num_steps: 5, result: [5.0, 4.75, 4.0, 2.75, 1.0] },
    { a: 0.0, b: 1.0, num_steps: 11, result: [0.0, 0.01, 0.04, 0.09, 0.16, 0.25, 0.36, 0.49, 0.64, 0.81, 1.0] },
    { a: 1.0, b: 0.0, num_steps: 11, result: [1.0, 0.99, 0.96, 0.91, 0.84, 0.75, 0.64, 0.51, 0.36, 0.19, 0.0] },
    { a: 0.0, b: 1.0, num_steps: 5, result: [0.0, 0.0625, 0.25, 0.5625, 1.0] },
    { a: 1.0, b: 0.0, num_steps: 5, result: [1.0, 0.9375, 0.75, 0.4375, 0.0] },
    { a: -1.0, b: 1.0, num_steps: 11, result: [-1.0, -0.98, -0.92, -0.82, -0.68, -0.5, -0.28 ,-0.02, 0.28, 0.62, 1.0] },
    { a: 1.0, b: -1.0, num_steps: 11, result: [1.0, 0.98, 0.92, 0.82, 0.68, 0.5, 0.28 ,0.02, -0.28, -0.62, -1.0] },
    { a: -1.0, b: 0, num_steps: 11, result: [-1.0 , -0.99, -0.96, -0.91, -0.84, -0.75, -0.64, -0.51, -0.36, -0.19, 0.0] },
    { a: 0.0, b: -1.0, num_steps: 11, result: [0.0, -0.01, -0.04, -0.09, -0.16, -0.25, -0.36, -0.49, -0.64, -0.81, -1.0] },
  ]
  )
  .fn(test => {
    const a = test.params.a;
    const b = test.params.b;
    const num_steps = test.params.num_steps;
    const got = biasedRange(a, b, num_steps);
    const expect = test.params.result;

    test.expect(
      compareArrayOfNumbers(got, expect),
      `biasedRange(${a}, ${b}, ${num_steps}) returned ${got}. Expected ${expect}`
    );
  });

interface limitsCase {
  bits: number;
  value: number;
}

// Test to confirm kBit and kValue constants are equivalent for f32
g.test('test,math,f32LimitsEquivalency')
  .paramsSimple<limitsCase>([
    { bits: kBit.f32.positive.max, value: kValue.f32.positive.max },
    { bits: kBit.f32.positive.min, value: kValue.f32.positive.min },
    { bits: kBit.f32.negative.max, value: kValue.f32.negative.max },
    { bits: kBit.f32.negative.min, value: kValue.f32.negative.min },
    { bits: kBit.f32.subnormal.positive.max, value: kValue.f32.subnormal.positive.max },
    { bits: kBit.f32.subnormal.positive.min, value: kValue.f32.subnormal.positive.min },
    { bits: kBit.f32.subnormal.negative.max, value: kValue.f32.subnormal.negative.max },
    { bits: kBit.f32.subnormal.negative.min, value: kValue.f32.subnormal.negative.min },
  ])
  .fn(test => {
    const bits = test.params.bits;
    const value = test.params.value;

    const val_to_bits = bits === float32ToUint32(value);
    const bits_to_val = value === (f32Bits(bits).value as number);
    test.expect(
      val_to_bits && bits_to_val,
      `bits = ${bits}, value = ${value}, returned val_to_bits as ${val_to_bits}, and bits_to_val as ${bits_to_val}, they are expected to be equivalent`
    );
  });
