export const description = `
Util math unit tests.
`;

import { makeTestGroup } from '../common/framework/test_group.js';
import { kBit } from '../webgpu/shader/execution/builtin/builtin.js';
import { f32, f32Bits, Scalar } from '../webgpu/util/conversion.js';
import { correctlyRounded, diffULP, nextAfter } from '../webgpu/util/math.js';

import { UnitTest } from './unit_test.js';

export const g = makeTestGroup(UnitTest);

interface DiffULPCase {
  a: number;
  b: number;
  ulp: number;
}

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
  .paramsSubcasesOnly<nextAfterCase>([
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
    // prettier-ignore
    { val: hexToF32(kBit.f32.subnormal.positive.min), dir: true, result: f32Bits(kBit.f32.positive.min) },
    // prettier-ignore
    { val: hexToF32(kBit.f32.subnormal.positive.min), dir: false, result: f32Bits(kBit.f32.negative.max) },
    // prettier-ignore
    { val: hexToF32(kBit.f32.subnormal.positive.max), dir: true, result: f32Bits(kBit.f32.positive.min) },
    // prettier-ignore
    { val: hexToF32(kBit.f32.subnormal.positive.max), dir: false, result: f32Bits(kBit.f32.negative.max) },
    // prettier-ignore
    { val: hexToF32(kBit.f32.subnormal.negative.min), dir: true, result: f32Bits(kBit.f32.positive.min) },
    // prettier-ignore
    { val: hexToF32(kBit.f32.subnormal.negative.min), dir: false, result: f32Bits(kBit.f32.negative.max) },
    // prettier-ignore
    { val: hexToF32(kBit.f32.subnormal.negative.max), dir: true, result: f32Bits(kBit.f32.positive.min) },
    // prettier-ignore
    { val: hexToF32(kBit.f32.subnormal.negative.max), dir: false, result: f32Bits(kBit.f32.negative.max) },

    // Normals
    // prettier-ignore
    { val: hexToF32(kBit.f32.positive.max), dir: true, result: f32Bits(kBit.f32.infinity.positive) },
    { val: hexToF32(kBit.f32.positive.max), dir: false, result: f32Bits(0x7f7ffffe) },
    { val: hexToF32(kBit.f32.positive.min), dir: true, result: f32Bits(0x00800001) },
    { val: hexToF32(kBit.f32.positive.min), dir: false, result: f32(0) },
    { val: hexToF32(kBit.f32.negative.max), dir: true, result: f32(0) },
    { val: hexToF32(kBit.f32.negative.max), dir: false, result: f32Bits(0x80800001) },
    { val: hexToF32(kBit.f32.negative.min), dir: true, result: f32Bits(0xff7ffffe) },
    // prettier-ignore
    { val: hexToF32(kBit.f32.negative.min), dir: false, result: f32Bits(kBit.f32.infinity.negative) },
    { val: hexToF32(0x03800000), dir: true, result: f32Bits(0x03800001) },
    { val: hexToF32(0x03800000), dir: false, result: f32Bits(0x037fffff) },
    { val: hexToF32(0x83800000), dir: true, result: f32Bits(0x837fffff) },
    { val: hexToF32(0x83800000), dir: false, result: f32Bits(0x83800001) },
  ])
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
  .paramsSubcasesOnly<nextAfterCase>([
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
    // prettier-ignore
    { val: hexToF32(kBit.f32.subnormal.positive.max), dir: true, result: f32Bits(kBit.f32.positive.min) },
    { val: hexToF32(kBit.f32.subnormal.positive.max), dir: false, result: f32Bits(0x007ffffe) },
    { val: hexToF32(kBit.f32.subnormal.negative.min), dir: true, result: f32Bits(0x807ffffe) },
    // prettier-ignore
    { val: hexToF32(kBit.f32.subnormal.negative.min), dir: false, result: f32Bits(kBit.f32.negative.max) },
    { val: hexToF32(kBit.f32.subnormal.negative.max), dir: true, result: f32(0) },
    { val: hexToF32(kBit.f32.subnormal.negative.max), dir: false, result: f32Bits(0x80000002) },

    // Normals
    // prettier-ignore
    { val: hexToF32(kBit.f32.positive.max), dir: true, result: f32Bits(kBit.f32.infinity.positive) },
    { val: hexToF32(kBit.f32.positive.max), dir: false, result: f32Bits(0x7f7ffffe) },
    { val: hexToF32(kBit.f32.positive.min), dir: true, result: f32Bits(0x00800001) },
    // prettier-ignore
    { val: hexToF32(kBit.f32.positive.min), dir: false, result: f32Bits(kBit.f32.subnormal.positive.max) },
    // prettier-ignore
    { val: hexToF32(kBit.f32.negative.max), dir: true, result: f32Bits(kBit.f32.subnormal.negative.min) },
    { val: hexToF32(kBit.f32.negative.max), dir: false, result: f32Bits(0x80800001) },
    { val: hexToF32(kBit.f32.negative.min), dir: true, result: f32Bits(0xff7ffffe) },
    // prettier-ignore
    { val: hexToF32(kBit.f32.negative.min), dir: false, result: f32Bits(kBit.f32.infinity.negative) },
    { val: hexToF32(0x03800000), dir: true, result: f32Bits(0x03800001) },
    { val: hexToF32(0x03800000), dir: false, result: f32Bits(0x037fffff) },
    { val: hexToF32(0x83800000), dir: true, result: f32Bits(0x837fffff) },
    { val: hexToF32(0x83800000), dir: false, result: f32Bits(0x83800001) },
  ])
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
  .paramsSubcasesOnly<correctlyRoundedCase>([
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
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.positive.s), target: Number.POSITIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: Number.POSITIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: Number.POSITIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0x7fffffff), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0xffffffff), target: Number.POSITIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.infinity.positive), target: Number.POSITIVE_INFINITY, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.infinity.negative), target: Number.POSITIVE_INFINITY, is_correct: false },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.positive.s), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0x7fffffff), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0xffffffff), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.infinity.positive), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
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
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },

    // 64-bit subnormals
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },

    // 32-bit normals
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.max), target: hexToF32(kBit.f32.positive.max), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.min), target: hexToF32(kBit.f32.positive.min), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.max), target: hexToF32(kBit.f32.negative.max), is_correct: true },
    // prettier-ignore
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
    // prettier-ignore
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00010, 0x00000001), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00020, 0x00000001), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00030, 0x00000002), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00040, 0x00000002), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00050, 0x00000001), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00060, 0x00000001), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00070, 0x00000002), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00080, 0x00000002), is_correct: false },
  ])
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
  .paramsSubcasesOnly<correctlyRoundedCase>([
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
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.positive.s), target: Number.POSITIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: Number.POSITIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: Number.POSITIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0x7fffffff), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0xffffffff), target: Number.POSITIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.infinity.positive), target: Number.POSITIVE_INFINITY, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.infinity.negative), target: Number.POSITIVE_INFINITY, is_correct: false },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.positive.s), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0x7fffffff), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0xffffffff), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.infinity.positive), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
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
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: false },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: false },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: false },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },

    // 64-bit subnormals
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },

    // 32-bit normals
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.max), target: hexToF32(kBit.f32.positive.max), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.min), target: hexToF32(kBit.f32.positive.min), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.max), target: hexToF32(kBit.f32.negative.max), is_correct: true },
    // prettier-ignore
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
    // prettier-ignore
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00010, 0x00000001), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00020, 0x00000001), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00030, 0x00000002), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00040, 0x00000002), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00050, 0x00000001), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00060, 0x00000001), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00070, 0x00000002), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00080, 0x00000002), is_correct: false },
  ])
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
  .paramsSubcasesOnly<correctlyRoundedCase>([
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
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.positive.s), target: Number.POSITIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: Number.POSITIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: Number.POSITIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0x7fffffff), target: Number.POSITIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0xffffffff), target: Number.POSITIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.infinity.positive), target: Number.POSITIVE_INFINITY, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.infinity.negative), target: Number.POSITIVE_INFINITY, is_correct: false },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.positive.s), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.positive.q), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.negative.s), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.nan.negative.q), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0x7fffffff), target: Number.NEGATIVE_INFINITY, is_correct: false },
    { test_val: f32Bits(0xffffffff), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.infinity.positive), target: Number.NEGATIVE_INFINITY, is_correct: false },
    // prettier-ignore
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
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.positive.min).value as number, is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.positive.max).value as number, is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.negative.max).value as number, is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: f32Bits(kBit.f32.subnormal.negative.min).value as number, is_correct: true },

    // 64-bit subnormals
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x00000000, 0x00000001), is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x00000000, 0x00000002), is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x800fffff, 0xffffffff), is_correct: true },

    // prettier-ignore
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.zero), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.zero), target: hexToFloat64(0x800fffff, 0xfffffffe), is_correct: true },

    // 32-bit normals
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.max), target: hexToF32(kBit.f32.positive.max), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.positive.min), target: hexToF32(kBit.f32.positive.min), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(kBit.f32.negative.max), target: hexToF32(kBit.f32.negative.max), is_correct: true },
    // prettier-ignore
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
    // prettier-ignore
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00010, 0x00000001), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00020, 0x00000001), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0x3f800000), target: hexToFloat64(0x3ff00030, 0x00000002), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0x3f800001), target: hexToFloat64(0x3ff00040, 0x00000002), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00050, 0x00000001), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00000, 0x00000001), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00060, 0x00000001), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0xbf800000), target: hexToFloat64(0xbff00070, 0x00000002), is_correct: false },
    // prettier-ignore
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00000, 0x00000002), is_correct: true },
    // prettier-ignore
    { test_val: f32Bits(0xbf800001), target: hexToFloat64(0xbff00080, 0x00000002), is_correct: false },
  ])
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
