export const description = `
Util math unit tests.
`;

import { makeTestGroup } from '../common/framework/test_group.js';
import { kBit } from '../webgpu/shader/execution/builtin/builtin.js';
import { f32, f32Bits, Scalar } from '../webgpu/util/conversion.js';
import { diffULP, nextAfter } from '../webgpu/util/math.js';

import { UnitTest } from './unit_test.js';

export const g = makeTestGroup(UnitTest);

interface DiffULPCase {
  a: number;
  b: number;
  ulp: number;
}

function hexToF32(hex: number): number {
  return new Float32Array(new Uint32Array([hex]).buffer)[0];
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

g.test('test,math,nextAfter')
  .paramsSimple<nextAfterCase>([
    // Edge Cases
    { val: NaN, dir: true, result: f32Bits(0x7fffffff) },
    { val: NaN, dir: false, result: f32Bits(0x7fffffff) },
    { val: Number.POSITIVE_INFINITY, dir: true, result: f32Bits(kBit.f32.infinity.positive) },
    { val: Number.POSITIVE_INFINITY, dir: false, result: f32Bits(kBit.f32.infinity.positive) },
    { val: Number.NEGATIVE_INFINITY, dir: true, result: f32Bits(kBit.f32.infinity.negative) },
    { val: Number.NEGATIVE_INFINITY, dir: false, result: f32Bits(kBit.f32.infinity.negative) },

    // Zeroes
    { val: -0, dir: true, result: f32Bits(kBit.f32.subnormal.positive.min) },
    { val: +0, dir: false, result: f32Bits(kBit.f32.subnormal.negative.max) },
    // Skipping these, since the testing framework does not distinguish between
    // +0 and -0, so throws an error about duplicate cases.
    // { val: +0, dir: true, result: f32Bits(kBit.f32.subnormal.positive.min) },
    // { val: -0, dir: false, result: f32Bits(kBit.f32.subnormal.negative.max) },

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
    const got = nextAfter(val, dir);
    const got_type = typeof got;
    t.expect(
      got.value === expect.value || (Number.isNaN(got.value) && Number.isNaN(expect.value)),
      `nextAfter(${val}, ${dir}) returned ${got} (${got_type}). Expected ${expect} (${expect_type})`
    );
  });
