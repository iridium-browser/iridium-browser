import { assert } from '../../common/util/util.js';

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
 * Return the Units of Last Place difference between the numbers a and b.
 * Requires `a` and `b` to be finite numbers.
 */
export function diffULP(a: number, b: number): number {
  const arr = new Uint32Array(new Float32Array([a, b]).buffer);
  const u32_a = arr[0];
  const u32_b = arr[1];

  const sign_a = (u32_a & 0x80000000) !== 0;
  const sign_b = (u32_b & 0x80000000) !== 0;
  const masked_a = u32_a & 0x7fffffff;
  const masked_b = u32_b & 0x7fffffff;

  if (sign_a === sign_b) {
    return Math.max(masked_a, masked_b) - Math.min(masked_a, masked_b);
  }
  return masked_a + masked_b;
}
