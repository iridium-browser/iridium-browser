import { assert } from '../../common/util/util.js';
import { kBit } from '../shader/execution/builtin/builtin.js';

import { f32Bits, Scalar } from './conversion.js';

/**
 * A multiple of 8 guaranteed to be way too large to allocate (just under 8 pebibytes).
 * This is a "safe" integer (ULP <= 1.0) very close to MAX_SAFE_INTEGER.
 *
 * Note: allocations of this size are likely to exceed limitations other than just the system's
 * physical memory, so test cases are also needed to try to trigger "true" OOM.
 */
export const kMaxSafeMultipleOf8 = Number.MAX_SAFE_INTEGER - 7;

/** Round `n` up to the next multiple of `alignment` (inclusive). */
// TODO: Rename to `roundUp`
export function align(n: number, alignment: number): number {
  assert(Number.isInteger(n) && n >= 0, 'n must be a non-negative integer');
  assert(Number.isInteger(alignment) && alignment > 0, 'alignment must be a positive integer');
  return Math.ceil(n / alignment) * alignment;
}

/** Round `n` down to the next multiple of `alignment` (inclusive). */
export function roundDown(n: number, alignment: number): number {
  assert(Number.isInteger(n) && n >= 0, 'n must be a non-negative integer');
  assert(Number.isInteger(alignment) && alignment > 0, 'alignment must be a positive integer');
  return Math.floor(n / alignment) * alignment;
}

/** Clamp a number to the provided range. */
export function clamp(n: number, { min, max }: { min: number; max: number }): number {
  assert(max >= min);
  return Math.min(Math.max(n, min), max);
}

/**
 * @returns the Units of Last Place difference between the numbers a and b.
 * If either `a` or `b` are not finite numbers, then diffULP() returns Infinity.
 */
export function diffULP(a: number, b: number): number {
  if (!Number.isFinite(a) || !Number.isFinite(b)) {
    return Infinity;
  }

  const arr = new Uint32Array(new Float32Array([a, b]).buffer);
  const u32_a = arr[0];
  const u32_b = arr[1];

  const sign_a = (u32_a & 0x80000000) !== 0;
  const sign_b = (u32_b & 0x80000000) !== 0;
  const masked_a = u32_a & 0x7fffffff;
  const masked_b = u32_b & 0x7fffffff;
  const subnormal_or_zero_a = (u32_a & 0x7f800000) === 0;
  const subnormal_or_zero_b = (u32_b & 0x7f800000) === 0;

  // If the number is subnormal, then reduce it to 0 for ULP comparison.
  // If the number is normal then reduce its bits-representation so to that we
  // can pretend that the subnormal numbers don't exist, for the purposes of
  // counting ULP steps from zero (or any subnormal) to any of the normal numbers.
  const bits_a = subnormal_or_zero_a ? 0 : masked_a - 0x7fffff;
  const bits_b = subnormal_or_zero_b ? 0 : masked_b - 0x7fffff;

  if (sign_a === sign_b) {
    return Math.max(bits_a, bits_b) - Math.min(bits_a, bits_b);
  }
  return bits_a + bits_b;
}

/**
 * @returns the next single precision floating point value after |val|,
 * towards +inf if |dir| is true, otherwise towards -inf.
 * For -/+0 the nextAfter will be the closest subnormal in the correct
 * direction, since -0 === +0.
 */
export function nextAfter(val: number, dir: boolean = true): Scalar {
  if (Number.isNaN(val)) {
    return f32Bits(kBit.f32.nan.positive.s);
  }

  if (val === Number.POSITIVE_INFINITY) {
    return f32Bits(kBit.f32.infinity.positive);
  }

  if (val === Number.NEGATIVE_INFINITY) {
    return f32Bits(kBit.f32.infinity.negative);
  }

  const u32_val = new Uint32Array(new Float32Array([val]).buffer)[0];
  if (u32_val === kBit.f32.positive.zero || u32_val === kBit.f32.negative.zero) {
    if (dir) {
      return f32Bits(kBit.f32.subnormal.positive.min);
    } else {
      return f32Bits(kBit.f32.subnormal.negative.max);
    }
  }

  let result = u32_val;
  const is_positive = (u32_val & 0x80000000) === 0;
  if (dir === is_positive) {
    result += 1;
  } else {
    result -= 1;
  }

  // Checking for overflow
  if ((result & 0x7f800000) === 0x7f800000) {
    if (dir) {
      return f32Bits(kBit.f32.infinity.positive);
    } else {
      return f32Bits(kBit.f32.infinity.negative);
    }
  }
  return f32Bits(result);
}
