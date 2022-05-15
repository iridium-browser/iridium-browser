export const description = `
Util math unit tests.
`;

import { makeTestGroup } from '../common/framework/test_group.js';
import { kBit, kValue } from '../webgpu/util/constants.js';
import { f32, f32Bits, float32ToUint32, Scalar } from '../webgpu/util/conversion.js';
import {
  biasedRange,
  correctlyRounded,
  fullF32Range,
  lerp,
  linearRange,
  nextAfter,
  oneULP,
  withinULP,
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
 * @returns true if arrays are equal within 1ULP, doing element-wise comparison as needed, and considering NaNs to be equal.
 *
 * Depends on the correctness of withinULP, which is tested in this file.
 **/
function compareArrayOfNumbers(got: Array<number>, expect: Array<number>): boolean {
  return (
    got.length === expect.length &&
    got.every((value, index) => {
      const expected = expect[index];
      return (Number.isNaN(value) && Number.isNaN(expected)) || withinULP(value, expected);
    })
  );
}

interface nextAfterCase {
  val: number;
  dir: boolean;
  result: Scalar;
}

g.test('nextAfterFlushToZero')
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

    // Not precisely expressible as float32
    { val: 0.001, dir: true, result: f32Bits(0x3a83126f) }, // positive normal
    { val: 0.001, dir: false, result: f32Bits(0x3a83126e) }, // positive normal
    { val: -0.001, dir: true, result: f32Bits(0xba83126e) }, // negative normal
    { val: -0.001, dir: false, result: f32Bits(0xba83126f) }, // negative normal
    { val: 2.82E-40, dir: true, result: f32Bits(kBit.f32.positive.min) }, // positive subnormal
    { val: 2.82E-40, dir: false, result: f32Bits(kBit.f32.negative.max) }, // positive subnormal
    { val: -2.82E-40, dir: true, result: f32Bits(kBit.f32.positive.min) }, // negative subnormal
    { val: -2.82E-40, dir: false, result: f32Bits(kBit.f32.negative.max) }, // negative subnormal
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

g.test('nextAfterNoFlush')
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

    // Not precisely expressible as float32
    { val: 0.001, dir: true, result: f32Bits(0x3a83126f) }, // positive normal
    { val: 0.001, dir: false, result: f32Bits(0x3a83126e) }, // positive normal
    { val: -0.001, dir: true, result: f32Bits(0xba83126e) }, // negative normal
    { val: -0.001, dir: false, result: f32Bits(0xba83126f) }, // negative normal
    { val: 2.82E-40, dir: true, result: f32Bits(0x0003121a) }, // positive subnormal
    { val: 2.82E-40, dir: false, result: f32Bits(0x00031219) }, // positive subnormal
    { val: -2.82E-40, dir: true, result: f32Bits(0x80031219) }, // negative subnormal
    { val: -2.82E-40, dir: false, result: f32Bits(0x8003121a) }, // negative subnormal
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

interface OneULPCase {
  target: number;
  expect: number;
}

g.test('oneULPFlushToZero')
  .paramsSimple<OneULPCase>([
    // Edge Cases
    { target: Number.NaN, expect: Number.NaN },
    { target: Number.POSITIVE_INFINITY, expect: hexToF32(0x73800000) },
    { target: Number.NEGATIVE_INFINITY, expect: hexToF32(0x73800000) },

    // Zeroes
    { target: +0, expect: hexToF32(0x00800000) },
    { target: -0, expect: hexToF32(0x00800000) },

    // Subnormals
    { target: hexToF32(kBit.f32.subnormal.positive.min), expect: hexToF32(0x00800000) },
    { target: hexToF32(kBit.f32.subnormal.positive.max), expect: hexToF32(0x00800000) },
    { target: hexToF32(kBit.f32.subnormal.negative.min), expect: hexToF32(0x00800000) },
    { target: hexToF32(kBit.f32.subnormal.negative.max), expect: hexToF32(0x00800000) },

    // Normals
    { target: hexToF32(kBit.f32.positive.max), expect: hexToF32(0x73800000) },
    { target: hexToF32(kBit.f32.positive.min), expect: hexToF32(0x00800000) },
    { target: hexToF32(kBit.f32.negative.max), expect: hexToF32(0x00800000) },
    { target: hexToF32(kBit.f32.negative.min), expect: hexToF32(0x73800000) },
    { target: 1, expect: hexToF32(0x33800000) },
    { target: 2, expect: hexToF32(0x34000000) },
    { target: 4, expect: hexToF32(0x34800000) },
    { target: 1000000, expect: hexToF32(0x3d800000) },
    { target: -1, expect: hexToF32(0x33800000) },
    { target: -2, expect: hexToF32(0x34000000) },
    { target: -4, expect: hexToF32(0x34800000) },
    { target: -1000000, expect: hexToF32(0x3d800000) },

    // Not precisely expressible as float32
    { target: 2.82e-40, expect: hexToF32(0x00800000) }, // positive subnormal
    { target: -2.82e-40, expect: hexToF32(0x00800000) }, // negative subnormal
    { target: 0.001, expect: hexToF32(0x2f000000) }, // positive normal
    { target: -0.001, expect: hexToF32(0x2f000000) }, // negative normal
    { target: 1e40, expect: hexToF32(0x73800000) }, // positive out of range
    { target: -1e40, expect: hexToF32(0x73800000) }, // negative out of range
  ])
  .fn(t => {
    const target = t.params.target;
    const got = oneULP(target, true);
    const expect = t.params.expect;
    t.expect(
      got === expect || (Number.isNaN(got) && Number.isNaN(expect)),
      `oneULP(${target}, true) returned ${got}. Expected ${expect}`
    );
  });

g.test('oneULPNoFlush')
  .paramsSimple<OneULPCase>([
    // Edge Cases
    { target: Number.NaN, expect: Number.NaN },
    { target: Number.POSITIVE_INFINITY, expect: hexToF32(0x73800000) },
    { target: Number.NEGATIVE_INFINITY, expect: hexToF32(0x73800000) },

    // Zeroes
    { target: +0, expect: hexToF32(0x00000001) },
    { target: -0, expect: hexToF32(0x00000001) },

    // Subnormals
    { target: hexToF32(kBit.f32.subnormal.positive.min), expect: hexToF32(0x00000001) },
    { target: hexToF32(kBit.f32.subnormal.positive.max), expect: hexToF32(0x00000001) },
    { target: hexToF32(kBit.f32.subnormal.negative.min), expect: hexToF32(0x00000001) },
    { target: hexToF32(kBit.f32.subnormal.negative.max), expect: hexToF32(0x00000001) },

    // Normals
    { target: 1, expect: hexToF32(0x33800000) },
    { target: 2, expect: hexToF32(0x34000000) },
    { target: 4, expect: hexToF32(0x34800000) },
    { target: 1000000, expect: hexToF32(0x3d800000) },
    { target: -1, expect: hexToF32(0x33800000) },
    { target: -2, expect: hexToF32(0x34000000) },
    { target: -4, expect: hexToF32(0x34800000) },
    { target: -1000000, expect: hexToF32(0x3d800000) },

    // Non-f32 expressible
    { target: 2.82e-40, expect: hexToF32(0x00000001) }, // positive subnormal
    { target: -2.82e-40, expect: hexToF32(0x00000001) }, // negative subnormal
    { target: 0.001, expect: hexToF32(0x2f000000) }, // positive normal
    { target: -0.001, expect: hexToF32(0x2f000000) }, // negative normal
    { target: 1e40, expect: hexToF32(0x73800000) }, // positive out of range
    { target: -1e40, expect: hexToF32(0x73800000) }, // negative out of range
  ])
  .fn(t => {
    const target = t.params.target;
    const got = oneULP(target, false);
    const expect = t.params.expect;
    t.expect(
      got === expect || (Number.isNaN(got) && Number.isNaN(expect)),
      `oneULP(${target}, true) returned ${got}. Expected ${expect}`
    );
  });

interface withinULPCase {
  val: number;
  target: number;
  expect: boolean;
}
g.test('withinULP')
  .paramsSubcasesOnly<withinULPCase>(
    // prettier-ignore
    [
    // Edge Cases
    { val: Number.NaN, target: Number.NaN, expect: false },
    { val: Number.POSITIVE_INFINITY, target: Number.NaN, expect: false },
    { val: Number.NEGATIVE_INFINITY, target: Number.NaN, expect: false },
    { val: 0, target: Number.NaN, expect: false },
    { val: 10, target: Number.NaN, expect: false },
    { val: -10, target: Number.NaN, expect: false },
    { val: hexToF32(kBit.f32.subnormal.positive.max), target: Number.NaN, expect: false },
    { val: hexToF32(kBit.f32.subnormal.negative.min), target: Number.NaN, expect: false },
    { val: hexToF32(kBit.f32.positive.max), target: Number.NaN, expect: false },
    { val: hexToF32(kBit.f32.negative.min), target: Number.NaN, expect: false },

    { val: Number.NaN, target: Number.POSITIVE_INFINITY, expect: false },
    { val: Number.POSITIVE_INFINITY, target: Number.POSITIVE_INFINITY, expect: true },
    { val: Number.NEGATIVE_INFINITY, target: Number.POSITIVE_INFINITY, expect: false },
    { val: 0, target: Number.POSITIVE_INFINITY, expect: false },
    { val: 10, target: Number.POSITIVE_INFINITY, expect: false },
    { val: -10, target: Number.POSITIVE_INFINITY, expect: false },
    { val: hexToF32(kBit.f32.subnormal.positive.max), target: Number.POSITIVE_INFINITY, expect: false },
    { val: hexToF32(kBit.f32.subnormal.negative.min), target: Number.POSITIVE_INFINITY, expect: false },
    { val: hexToF32(kBit.f32.positive.max), target: Number.POSITIVE_INFINITY, expect: false },
    { val: hexToF32(kBit.f32.negative.min), target: Number.POSITIVE_INFINITY, expect: false },

    { val: Number.NaN, target: Number.NEGATIVE_INFINITY, expect: false },
    { val: Number.POSITIVE_INFINITY, target: Number.NEGATIVE_INFINITY, expect: false },
    { val: Number.NEGATIVE_INFINITY, target: Number.NEGATIVE_INFINITY, expect: true },
    { val: 0, target: Number.NEGATIVE_INFINITY, expect: false },
    { val: 10, target: Number.NEGATIVE_INFINITY, expect: false },
    { val: -10, target: Number.NEGATIVE_INFINITY, expect: false },
    { val: hexToF32(kBit.f32.subnormal.positive.max), target: Number.NEGATIVE_INFINITY, expect: false },
    { val: hexToF32(kBit.f32.subnormal.negative.min), target: Number.NEGATIVE_INFINITY, expect: false },
    { val: hexToF32(kBit.f32.positive.max), target: Number.NEGATIVE_INFINITY, expect: false },
    { val: hexToF32(kBit.f32.negative.min), target: Number.NEGATIVE_INFINITY, expect: false },

    // Zero
    { val: 0, target: 0, expect: true },
    { val: 10, target: 0, expect: false },
    { val: -10, target: 0, expect: false },
    { val: hexToF32(kBit.f32.subnormal.positive.max), target: 0, expect: true },
    { val: hexToF32(kBit.f32.subnormal.negative.min), target: 0, expect: true },
    { val: hexToF32(kBit.f32.positive.max), target: 0, expect: false },
    { val: hexToF32(kBit.f32.negative.min), target: 0, expect: false },
  ]
  )
  .fn(t => {
    const val = t.params.val;
    const target = t.params.target;
    const got = withinULP(val, target);
    const expect = t.params.expect;
    t.expect(got === expect, `withinULP(${val}, ${target}) returned ${got}. Expected ${expect}`);
  });

interface correctlyRoundedCase {
  test_val: Scalar;
  target: number;
  is_correct: boolean;
}

g.test('correctlyRounded')
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
    { test_val: f32Bits(kBit.f32.positive.zero), target: kValue.f32.subnormal.positive.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: kValue.f32.subnormal.positive.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: kValue.f32.subnormal.positive.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: kValue.f32.subnormal.positive.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: kValue.f32.subnormal.positive.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: kValue.f32.subnormal.positive.min, is_correct: true },

    { test_val: f32Bits(kBit.f32.positive.zero), target: kValue.f32.subnormal.positive.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: kValue.f32.subnormal.positive.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: kValue.f32.subnormal.positive.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: kValue.f32.subnormal.positive.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: kValue.f32.subnormal.positive.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: kValue.f32.subnormal.positive.max, is_correct: true },

    { test_val: f32Bits(kBit.f32.positive.zero), target: kValue.f32.subnormal.negative.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: kValue.f32.subnormal.negative.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: kValue.f32.subnormal.negative.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: kValue.f32.subnormal.negative.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: kValue.f32.subnormal.negative.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: kValue.f32.subnormal.negative.max, is_correct: true },

    { test_val: f32Bits(kBit.f32.positive.zero), target: kValue.f32.subnormal.negative.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: kValue.f32.subnormal.negative.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: kValue.f32.subnormal.negative.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: kValue.f32.subnormal.negative.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: kValue.f32.subnormal.negative.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: kValue.f32.subnormal.negative.min, is_correct: true },

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

g.test('correctlyRoundedNoFlushOnly')
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
    { test_val: f32Bits(kBit.f32.positive.zero), target: kValue.f32.subnormal.positive.min, is_correct: false },
    { test_val: f32Bits(kBit.f32.negative.zero), target: kValue.f32.subnormal.positive.min, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: kValue.f32.subnormal.positive.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: kValue.f32.subnormal.positive.min, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: kValue.f32.subnormal.positive.min, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: kValue.f32.subnormal.positive.min, is_correct: false },

    { test_val: f32Bits(kBit.f32.positive.zero), target: kValue.f32.subnormal.positive.max, is_correct: false },
    { test_val: f32Bits(kBit.f32.negative.zero), target: kValue.f32.subnormal.positive.max, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: kValue.f32.subnormal.positive.max, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: kValue.f32.subnormal.positive.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: kValue.f32.subnormal.positive.max, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: kValue.f32.subnormal.positive.max, is_correct: false },

    { test_val: f32Bits(kBit.f32.positive.zero), target: kValue.f32.subnormal.negative.max, is_correct: false },
    { test_val: f32Bits(kBit.f32.negative.zero), target: kValue.f32.subnormal.negative.max, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: kValue.f32.subnormal.negative.max, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: kValue.f32.subnormal.negative.max, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: kValue.f32.subnormal.negative.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: kValue.f32.subnormal.negative.max, is_correct: false },

    { test_val: f32Bits(kBit.f32.positive.zero), target: kValue.f32.subnormal.negative.min, is_correct: false },
    { test_val: f32Bits(kBit.f32.negative.zero), target: kValue.f32.subnormal.negative.min, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: kValue.f32.subnormal.negative.min, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: kValue.f32.subnormal.negative.min, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: kValue.f32.subnormal.negative.min, is_correct: false },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: kValue.f32.subnormal.negative.min, is_correct: true },

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

g.test('correctlyRoundedFlushToZeroOnly')
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
    { test_val: f32Bits(kBit.f32.positive.zero), target: kValue.f32.subnormal.positive.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: kValue.f32.subnormal.positive.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: kValue.f32.subnormal.positive.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: kValue.f32.subnormal.positive.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: kValue.f32.subnormal.positive.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: kValue.f32.subnormal.positive.min, is_correct: true },

    { test_val: f32Bits(kBit.f32.positive.zero), target: kValue.f32.subnormal.positive.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: kValue.f32.subnormal.positive.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: kValue.f32.subnormal.positive.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: kValue.f32.subnormal.positive.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: kValue.f32.subnormal.positive.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: kValue.f32.subnormal.positive.max, is_correct: true },

    { test_val: f32Bits(kBit.f32.positive.zero), target: kValue.f32.subnormal.negative.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: kValue.f32.subnormal.negative.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: kValue.f32.subnormal.negative.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: kValue.f32.subnormal.negative.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: kValue.f32.subnormal.negative.max, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: kValue.f32.subnormal.negative.max, is_correct: true },

    { test_val: f32Bits(kBit.f32.positive.zero), target: kValue.f32.subnormal.negative.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.negative.zero), target: kValue.f32.subnormal.negative.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.min), target: kValue.f32.subnormal.negative.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.positive.max), target: kValue.f32.subnormal.negative.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.max), target: kValue.f32.subnormal.negative.min, is_correct: true },
    { test_val: f32Bits(kBit.f32.subnormal.negative.min), target: kValue.f32.subnormal.negative.min, is_correct: true },

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

g.test('lerp')
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
      (Number.isNaN(got) && Number.isNaN(expect)) || withinULP(got, expect),
      `lerp(${a}, ${b}, ${t}) returned ${got}. Expected ${expect}`
    );
  });

interface rangeCase {
  a: number;
  b: number;
  num_steps: number;
  result: Array<number>;
}

g.test('linearRange')
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

g.test('biasedRange')
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

interface fullF32RangeCase {
  neg_norm: number;
  neg_sub: number;
  pos_sub: number;
  pos_norm: number;
  expect: Array<number>;
}

g.test('fullF32RangeCase')
  .paramsSimple<fullF32RangeCase>(
    // prettier-ignore
    [
        { neg_norm: 0, neg_sub: 0, pos_sub: 0, pos_norm: 0, expect: [ 0.0 ] },
        { neg_norm: 1, neg_sub: 0, pos_sub: 0, pos_norm: 0, expect: [ kValue.f32.negative.max, 0.0] },
        { neg_norm: 2, neg_sub: 0, pos_sub: 0, pos_norm: 0, expect: [ kValue.f32.negative.max, kValue.f32.negative.min, 0.0 ] },
        { neg_norm: 0, neg_sub: 1, pos_sub: 0, pos_norm: 0, expect: [ kValue.f32.subnormal.negative.max, 0.0 ] },
        { neg_norm: 0, neg_sub: 2, pos_sub: 0, pos_norm: 0, expect: [ kValue.f32.subnormal.negative.max, kValue.f32.subnormal.negative.min, 0.0 ] },
        { neg_norm: 0, neg_sub: 0, pos_sub: 1, pos_norm: 0, expect: [ 0.0, kValue.f32.subnormal.positive.min ] },
        { neg_norm: 0, neg_sub: 0, pos_sub: 2, pos_norm: 0, expect: [ 0.0, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.positive.max ] },
        { neg_norm: 0, neg_sub: 0, pos_sub: 0, pos_norm: 1, expect: [ 0.0, kValue.f32.positive.min ] },
        { neg_norm: 0, neg_sub: 0, pos_sub: 0, pos_norm: 2, expect: [ 0.0, kValue.f32.positive.min, kValue.f32.positive.max ] },
        { neg_norm: 1, neg_sub: 1, pos_sub: 1, pos_norm: 1, expect: [ kValue.f32.negative.max, kValue.f32.subnormal.negative.max, 0.0, kValue.f32.subnormal.positive.min, kValue.f32.positive.min ] },
        { neg_norm: 2, neg_sub: 2, pos_sub: 2, pos_norm: 2, expect: [ kValue.f32.negative.max, kValue.f32.negative.min, kValue.f32.subnormal.negative.max, kValue.f32.subnormal.negative.min, 0.0, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.positive.max, kValue.f32.positive.min, kValue.f32.positive.max ] },
    ]
  )
  .fn(test => {
    const neg_norm = test.params.neg_norm;
    const neg_sub = test.params.neg_sub;
    const pos_sub = test.params.pos_sub;
    const pos_norm = test.params.pos_norm;
    const got = fullF32Range({ neg_norm, neg_sub, pos_sub, pos_norm });
    const expect = test.params.expect;

    test.expect(
      compareArrayOfNumbers(got, expect),
      `fullF32Range(${neg_norm}, ${neg_sub}, ${pos_sub}, ${pos_norm}) returned ${got}. Expected ${expect}`
    );
  });

interface limitsCase {
  bits: number;
  value: number;
}

// Test to confirm kBit and kValue constants are equivalent for f32
g.test('f32LimitsEquivalency')
  .paramsSimple<limitsCase>([
    { bits: kBit.f32.positive.max, value: kValue.f32.positive.max },
    { bits: kBit.f32.positive.min, value: kValue.f32.positive.min },
    { bits: kBit.f32.positive.nearest_max, value: kValue.f32.positive.nearest_max },
    { bits: kBit.f32.negative.max, value: kValue.f32.negative.max },
    { bits: kBit.f32.negative.min, value: kValue.f32.negative.min },
    { bits: kBit.f32.negative.nearest_min, value: kValue.f32.negative.nearest_min },
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
