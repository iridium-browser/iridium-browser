import { assert } from '../../common/util/util.js';
import { Float16Array } from '../../external/petamoriken/float16/float16.js';

import { kBit, kValue } from './constants.js';
import {
  f16,
  f16Bits,
  f32,
  f32Bits,
  floatBitsToNumber,
  i32,
  kFloat16Format,
  kFloat32Format,
  Scalar,
} from './conversion.js';

/**
 * A multiple of 8 guaranteed to be way too large to allocate (just under 8 pebibytes).
 * This is a "safe" integer (ULP <= 1.0) very close to MAX_SAFE_INTEGER.
 *
 * Note: allocations of this size are likely to exceed limitations other than just the system's
 * physical memory, so test cases are also needed to try to trigger "true" OOM.
 */
export const kMaxSafeMultipleOf8 = Number.MAX_SAFE_INTEGER - 7;

/** Round `n` up to the next multiple of `alignment` (inclusive). */
// MAINTENANCE_TODO: Rename to `roundUp`
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

/** @returns 0 if |val| is a subnormal f32 number, otherwise returns |val| */
export function flushSubnormalNumberF32(val: number): number {
  return isSubnormalNumberF32(val) ? 0 : val;
}

/** @returns 0 if |val| is a subnormal f32 number, otherwise returns |val| */
export function flushSubnormalScalarF32(val: Scalar): Scalar {
  return isSubnormalScalarF32(val) ? f32(0) : val;
}

/**
 * @returns true if |val| is a subnormal f32 number, otherwise returns false
 * 0 is considered a non-subnormal number by this function.
 */
export function isSubnormalScalarF32(val: Scalar): boolean {
  if (val.type.kind !== 'f32') {
    return false;
  }

  if (val === f32(0)) {
    return false;
  }

  const u32_val = new Uint32Array(new Float32Array([val.value.valueOf() as number]).buffer)[0];
  return (u32_val & 0x7f800000) === 0;
}

/** U/** @returns if number is within subnormal range of f32 */
export function isSubnormalNumberF32(n: number): boolean {
  return n > kValue.f32.negative.max && n < kValue.f32.positive.min;
}

/** @returns if number is in the finite range of f32 */
export function isFiniteF32(n: number) {
  return n >= kValue.f32.negative.min && n <= kValue.f32.positive.max;
}

/** @returns 0 if |val| is a subnormal f16 number, otherwise returns |val| */
export function flushSubnormalNumberF16(val: number): number {
  return isSubnormalNumberF16(val) ? 0 : val;
}

/** @returns 0 if |val| is a subnormal f16 number, otherwise returns |val| */
export function flushSubnormalScalarF16(val: Scalar): Scalar {
  return isSubnormalScalarF16(val) ? f16(0) : val;
}

/**
 * @returns true if |val| is a subnormal f16 number, otherwise returns false
 * 0 is considered a non-subnormal number by this function.
 */
export function isSubnormalScalarF16(val: Scalar): boolean {
  if (val.type.kind !== 'f16') {
    return false;
  }

  if (val === f16(0)) {
    return false;
  }

  const u16_val = new Uint16Array(new Float16Array([val.value.valueOf() as number]).buffer)[0];
  return (u16_val & 0x7f800000) === 0;
}

/** @returns if number is within subnormal range of f16 */
export function isSubnormalNumberF16(n: number): boolean {
  return n > kValue.f16.negative.max && n < kValue.f16.positive.min;
}

/** @returns if number is in the finite range of f16 */
export function isFiniteF16(n: number) {
  return n >= kValue.f16.negative.min && n <= kValue.f16.positive.max;
}

/**
 * @returns the next f32 value after |val|,
 * towards +inf if |dir| is true, otherwise towards -inf.
 * If |flush| is true, all subnormal values will be flushed to 0,
 * before processing.
 * If |flush| is false, the next subnormal will be calculated when appropriate,
 * and for -/+0 the nextAfterF32 will be the closest subnormal in the correct
 * direction.
 * val needs to be in [min f32, max f32]
 */
export function nextAfterF32(val: number, dir: boolean = true, flush: boolean): Scalar {
  if (Number.isNaN(val)) {
    return f32Bits(kBit.f32.nan.positive.s);
  }

  if (val === Number.POSITIVE_INFINITY) {
    return f32Bits(kBit.f32.infinity.positive);
  }

  if (val === Number.NEGATIVE_INFINITY) {
    return f32Bits(kBit.f32.infinity.negative);
  }

  assert(
    val <= kValue.f32.positive.max && val >= kValue.f32.negative.min,
    `${val} is not in the range of float32`
  );

  val = flush ? flushSubnormalNumberF32(val) : val;

  // -/+0 === 0 returns true
  if (val === 0) {
    if (dir) {
      return flush ? f32Bits(kBit.f32.positive.min) : f32Bits(kBit.f32.subnormal.positive.min);
    } else {
      return flush ? f32Bits(kBit.f32.negative.max) : f32Bits(kBit.f32.subnormal.negative.max);
    }
  }

  const converted: number = new Float32Array([val])[0];
  let u32_result: number;
  if (val === converted) {
    // val is expressible precisely as a float32
    u32_result = new Uint32Array(new Float32Array([val]).buffer)[0];
    const is_positive = (u32_result & 0x80000000) === 0;
    if (dir === is_positive) {
      u32_result += 1;
    } else {
      u32_result -= 1;
    }
  } else {
    // val had to be rounded to be expressed as a float32
    if (dir === converted > val) {
      // Rounding was in the direction requested
      u32_result = new Uint32Array(new Float32Array([converted]).buffer)[0];
    } else {
      // Round was opposite of the direction requested, so need nextAfterF32 in the requested direction.
      // This will not recurse since converted is guaranteed to be a float32 due to the conversion above.
      const next = nextAfterF32(converted, dir, flush).value.valueOf() as number;
      u32_result = new Uint32Array(new Float32Array([next]).buffer)[0];
    }
  }

  // Checking for overflow
  if ((u32_result & 0x7f800000) === 0x7f800000) {
    if (dir) {
      return f32Bits(kBit.f32.infinity.positive);
    } else {
      return f32Bits(kBit.f32.infinity.negative);
    }
  }

  const f32_result = f32Bits(u32_result);
  return flush ? flushSubnormalScalarF32(f32_result) : f32_result;
}

/**
 * @returns the next f16 value after |val|,
 * towards +inf if |dir| is true, otherwise towards -inf.
 * If |flush| is true, all subnormal values will be flushed to 0,
 * before processing.
 * If |flush| is false, the next subnormal will be calculated when appropriate,
 * and for -/+0 the nextAfterF16 will be the closest subnormal in the correct
 * direction.
 * val needs to be in [min f16, max f16]
 */
export function nextAfterF16(val: number, dir: boolean = true, flush: boolean): Scalar {
  if (Number.isNaN(val)) {
    return f16Bits(kBit.f16.nan.positive.s);
  }

  if (val === Number.POSITIVE_INFINITY) {
    return f16Bits(kBit.f16.infinity.positive);
  }

  if (val === Number.NEGATIVE_INFINITY) {
    return f16Bits(kBit.f16.infinity.negative);
  }

  assert(
    val <= kValue.f16.positive.max && val >= kValue.f16.negative.min,
    `${val} is not in the range of float16`
  );

  val = flush ? flushSubnormalNumberF16(val) : val;

  // -/+0 === 0 returns true
  if (val === 0) {
    if (dir) {
      return flush ? f16Bits(kBit.f16.positive.min) : f16Bits(kBit.f16.subnormal.positive.min);
    } else {
      return flush ? f16Bits(kBit.f16.negative.max) : f16Bits(kBit.f16.subnormal.negative.max);
    }
  }

  const converted: number = new Float16Array([val])[0];
  let u16_result: number;
  if (val === converted) {
    // val is expressible precisely as a float16
    u16_result = new Uint16Array(new Float16Array([val]).buffer)[0];
    const is_positive = (u16_result & 0x8000) === 0;
    if (dir === is_positive) {
      u16_result += 1;
    } else {
      u16_result -= 1;
    }
  } else {
    // val had to be rounded to be expressed as a float16
    if (dir === converted > val) {
      // Rounding was in the direction requested
      u16_result = new Uint16Array(new Float16Array([converted]).buffer)[0];
    } else {
      // Round was opposite of the direction requested, so need nextAfterF16 in the requested direction.
      // This will not recurse since converted is guaranteed to be a float16 due to the conversion above.
      const next = nextAfterF16(converted, dir, flush).value.valueOf() as number;
      u16_result = new Uint16Array(new Float16Array([next]).buffer)[0];
    }
  }

  // Checking for overflow
  if ((u16_result & 0x7f800000) === 0x7f800000) {
    if (dir) {
      return f16Bits(kBit.f16.infinity.positive);
    } else {
      return f16Bits(kBit.f16.infinity.negative);
    }
  }

  const f16_result = f16Bits(u16_result);
  return flush ? flushSubnormalScalarF16(f16_result) : f16_result;
}

/**
 * @returns ulp(x) for a specific flushing mode
 *
 * This is the main implementation of oneULP, which is normally what should be
 * used. This should only be called directly if a specific flushing mode is
 * required.
 *
 * @param target number to calculate ULP for
 * @param flush should subnormals be flushed to zero
 */
function oneULPImpl(target: number, flush: boolean): number {
  if (Number.isNaN(target)) {
    return Number.NaN;
  }

  target = flush ? flushSubnormalNumberF32(target) : target;

  // For values at the edge of the range or beyond ulp(x) is defined as the distance between the two nearest
  // f32 representable numbers to the appropriate edge.
  if (target === Number.POSITIVE_INFINITY || target >= kValue.f32.positive.max) {
    return kValue.f32.positive.max - kValue.f32.positive.nearest_max;
  } else if (target === Number.NEGATIVE_INFINITY || target <= kValue.f32.negative.min) {
    return kValue.f32.negative.nearest_min - kValue.f32.negative.min;
  }

  // ulp(x) is min(after - before), where
  //     before <= x <= after
  //     before =/= after
  //     before and after are f32 representable
  const before = nextAfterF32(target, false, flush).value.valueOf() as number;
  const after = nextAfterF32(target, true, flush).value.valueOf() as number;
  const converted: number = new Float32Array([target])[0];
  if (converted === target) {
    // |target| is f32 representable, so either before or after will be x
    return Math.min(target - before, after - target);
  } else {
    // |target| is not f32 representable so taking distance of neighbouring f32s.
    return after - before;
  }
}

/**
 * @returns ulp(x), the unit of least precision for a specific number as a 32-bit float
 *
 * ulp(x) is the distance between the two floating point numbers nearest x.
 * This value is also called unit of last place, ULP, and 1 ULP.
 * See the WGSL spec and http://www.ens-lyon.fr/LIP/Pub/Rapports/RR/RR2005/RR2005-09.pdf
 * for a more detailed/nuanced discussion of the definition of ulp(x).
 *
 * @param target number to calculate ULP for
 * @param flush should subnormals be flushed to zero, if not set both flushed
 *              and non-flush values are considered.
 */
export function oneULP(target: number, flush?: boolean): number {
  if (flush === undefined) {
    return Math.max(oneULPImpl(target, false), oneULPImpl(target, true));
  }

  return oneULPImpl(target, flush);
}

/**
 * @returns if a number is within N * ulp(x) of a target value
 * @param val number to test
 * @param target expected number
 * @param n acceptance range
 * @param flush should subnormals be flushed to zero
 */
export function withinULP(val: number, target: number, n: number = 1) {
  if (Number.isNaN(val) || Number.isNaN(target)) {
    return false;
  }

  const ulp = oneULP(target);
  if (Number.isNaN(ulp)) {
    return false;
  }

  if (val === target) {
    return true;
  }

  const diff = val > target ? val - target : target - val;
  return diff <= n * ulp;
}

/**
 * Calculate the valid roundings when quantizing to 32-bit floats
 *
 * TS/JS's number type is internally a f64, so quantization needs to occur when
 * converting to f32 for WGSL. WGSL does not specify a specific rounding mode,
 * so if a number is not precisely representable in 32-bits, but in the
 * range, there are two possible valid quantizations. If it is precisely
 * representable, there is only one valid quantization. This function calculates
 * the valid roundings and returns them in an array.
 *
 * This function does not consider flushing mode, so subnormals are maintained.
 * The caller is responsible to flushing before and after as appropriate.
 *
 * Out of range values return the appropriate infinity and edge value.
 *
 * @param n number to be quantized
 * @returns all of the acceptable roundings for quantizing to 32-bits in
 *          ascending order.
 */
export function correctlyRoundedF32(n: number): number[] {
  assert(!Number.isNaN(n), `correctlyRoundedF32 not defined for NaN`);
  // Above f32 range
  if (n === Number.POSITIVE_INFINITY || n > kValue.f32.positive.max) {
    return [kValue.f32.positive.max, Number.POSITIVE_INFINITY];
  }

  // Below f32 range
  if (n === Number.NEGATIVE_INFINITY || n < kValue.f32.negative.min) {
    return [Number.NEGATIVE_INFINITY, kValue.f32.negative.min];
  }

  const n_32 = new Float32Array([n])[0];
  const converted: number = n_32;
  if (n === converted) {
    // n is precisely expressible as a f32, so should not be rounded
    return [n];
  }

  if (converted > n) {
    // n_32 rounded towards +inf, so is after n
    const other = nextAfterF32(n_32, false, false).value as number;
    return [other, converted];
  } else {
    // n_32 rounded towards -inf, so is before n
    const other = nextAfterF32(n_32, true, false).value as number;
    return [converted, other];
  }
}

/**
 * Calculate the valid roundings when quantizing to 16-bit floats
 *
 * TS/JS's number type is internally a f64, so quantization needs to occur when
 * converting to f16 for WGSL. WGSL does not specify a specific rounding mode,
 * so if a number is not precisely representable in 16-bits, but in the
 * range, there are two possible valid quantizations. If it is precisely
 * representable, there is only one valid quantization. This function calculates
 * the valid roundings and returns them in an array.
 *
 * This function does not consider flushing mode, so subnormals are maintained.
 * The caller is responsible to flushing before and after as appropriate.
 *
 * Out of range values return the appropriate infinity and edge value.
 *
 * @param n number to be quantized
 * @returns all of the acceptable roundings for quantizing to 16-bits in
 *          ascending order.
 */
export function correctlyRoundedF16(n: number): number[] {
  assert(!Number.isNaN(n), `correctlyRoundedF16 not defined for NaN`);
  // Above f16 range
  if (n === Number.POSITIVE_INFINITY || n > kValue.f16.positive.max) {
    return [kValue.f16.positive.max, Number.POSITIVE_INFINITY];
  }

  // Below f16 range
  if (n === Number.NEGATIVE_INFINITY || n < kValue.f16.negative.min) {
    return [Number.NEGATIVE_INFINITY, kValue.f16.negative.min];
  }

  const n_16 = new Float16Array([n])[0];
  const converted: number = n_16;
  if (n === converted) {
    // n is precisely expressible as a f16, so should not be rounded
    return [n];
  }

  if (converted > n) {
    // n_16 rounded towards +inf, so is after n
    const other = nextAfterF16(n_16, false, false).value as number;
    return [other, converted];
  } else {
    // n_16 rounded towards -inf, so is before n
    const other = nextAfterF16(n_16, true, false).value as number;
    return [converted, other];
  }
}

/**
 * Calculates the linear interpolation between two values of a given fractional.
 *
 * If |t| is 0, |a| is returned, if |t| is 1, |b| is returned, otherwise
 * interpolation/extrapolation equivalent to a + t(b - a) is performed.
 *
 * Numerical stable version is adapted from http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0811r2.html
 */
export function lerp(a: number, b: number, t: number): number {
  if (!Number.isFinite(a) || !Number.isFinite(b)) {
    return Number.NaN;
  }

  if ((a <= 0.0 && b >= 0.0) || (a >= 0.0 && b <= 0.0)) {
    return t * b + (1 - t) * a;
  }

  if (t === 1.0) {
    return b;
  }

  const x = a + t * (b - a);
  return t > 1.0 === b > a ? Math.max(b, x) : Math.min(b, x);
}

/** @returns a linear increasing range of numbers. */
export function linearRange(a: number, b: number, num_steps: number): Array<number> {
  if (num_steps <= 0) {
    return Array<number>();
  }

  // Avoid division by 0
  if (num_steps === 1) {
    return [a];
  }

  return Array.from(Array(num_steps).keys()).map(i => lerp(a, b, i / (num_steps - 1)));
}

/**
 * @returns a non-linear increasing range of numbers, with a bias towards the beginning.
 *
 * Generates a linear range on [0,1] with |num_steps|, then squares all the values to make the curve be quadratic,
 * thus biasing towards 0, but remaining on the [0, 1] range.
 * This biased range is then scaled to the desired range using lerp.
 * Different curves could be generated by changing c, where greater values of c will bias more towards 0.
 */
export function biasedRange(a: number, b: number, num_steps: number): Array<number> {
  const c = 2;
  if (num_steps <= 0) {
    return Array<number>();
  }

  // Avoid division by 0
  if (num_steps === 1) {
    return [a];
  }

  return Array.from(Array(num_steps).keys()).map(i =>
    lerp(a, b, Math.pow(lerp(0, 1, i / (num_steps - 1)), c))
  );
}

/**
 * @returns an ascending sorted array of numbers spread over the entire range of 32-bit floats
 *
 * Numbers are divided into 4 regions: negative normals, negative subnormals, positive subnormals & positive normals.
 * Zero is included.
 *
 * Numbers are generated via taking a linear spread of the bit field representations of the values in each region. This
 * means that number of precise f32 values between each returned value in a region should be about the same. This allows
 * for a wide range of magnitudes to be generated, instead of being extremely biased towards the edges of the f32 range.
 *
 * This function is intended to provide dense coverage of the f32 range, for a minimal list of values to use to cover
 * f32 behaviour, use sparseF32Range instead.
 *
 * @param counts structure param with 4 entries indicating the number of entries to be generated each region, entries
 *               must be 0 or greater.
 */
export function fullF32Range(
  counts: {
    neg_norm?: number;
    neg_sub?: number;
    pos_sub: number;
    pos_norm: number;
  } = { pos_sub: 10, pos_norm: 50 }
): Array<number> {
  counts.neg_norm = counts.neg_norm === undefined ? counts.pos_norm : counts.neg_norm;
  counts.neg_sub = counts.neg_sub === undefined ? counts.pos_sub : counts.neg_sub;

  // Generating bit fields first and then converting to f32, so that the spread across the possible f32 values is more
  // even. Generating against the bounds of f32 values directly results in the values being extremely biased towards the
  // extremes, since they are so much larger.
  const bit_fields = [
    ...linearRange(kBit.f32.negative.min, kBit.f32.negative.max, counts.neg_norm),
    ...linearRange(
      kBit.f32.subnormal.negative.min,
      kBit.f32.subnormal.negative.max,
      counts.neg_sub
    ),
    0,
    ...linearRange(
      kBit.f32.subnormal.positive.min,
      kBit.f32.subnormal.positive.max,
      counts.pos_sub
    ),
    ...linearRange(kBit.f32.positive.min, kBit.f32.positive.max, counts.pos_norm),
  ].map(Math.trunc);
  return bit_fields.map(hexToF32);
}

/**
 * @returns an ascending sorted array of numbers spread over the entire range of 32-bit signed ints
 *
 * Numbers are divided into 2 regions: negatives, and positives, with their spreads biased towards 0
 * Zero is included in range.
 *
 * @param counts structure param with 2 entries indicating the number of entries to be generated each region, values must be 0 or greater.
 */
export function fullI32Range(
  counts: {
    negative?: number;
    positive: number;
  } = { positive: 50 }
): Array<number> {
  counts.negative = counts.negative === undefined ? counts.positive : counts.negative;
  return [
    ...biasedRange(kValue.i32.negative.min, -1, counts.negative),
    0,
    ...biasedRange(1, kValue.i32.positive.max, counts.positive),
  ].map(Math.trunc);
}

/** Short list of f32 values of interest to test against */
const kInterestingF32Values: number[] = [
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

/** @returns minimal f32 values that cover the entire range of f32 behaviours
 *
 * Has specially selected values that cover edge cases, normals, and subnormals.
 * This is used instead of fullF32Range when the number of test cases being
 * generated is a super linear function of the length of f32 values which is
 * leading to time outs.
 *
 * These values have been chosen to attempt to test the widest range of f32
 * behaviours in the lowest number of entries, so may potentially miss function
 * specific values of interest. If there are known values of interest they
 * should be appended to this list in the test generation code.
 */
export function sparseF32Range(): Array<number> {
  return kInterestingF32Values;
}

/**
 * Set of vectors, indexed by dimension, that contain interesting float values
 *
 * The tests do not do the simple option for coverage of computing the cartesian
 * product of all of the interesting float values N times for vecN tests,
 * because that creates a huge number of tests for vec3 and vec4, leading to
 * time outs.
 * Instead they insert the interesting f32 values into each location of the
 * vector to get a spread of testing over the entire range. This reduces the
 * number of cases being run substantially, but maintains coverage.
 */
export const kVectorTestValues = {
  2: sparseF32Range().flatMap(f => [
    [f, 1.0],
    [1.0, f],
    [f, -1.0],
    [-1.0, f],
  ]),
  3: sparseF32Range().flatMap(f => [
    [f, 1.0, 2.0],
    [1.0, f, 2.0],
    [1.0, 2.0, f],
    [f, -1.0, -2.0],
    [-1.0, f, -2.0],
    [-1.0, -2.0, f],
  ]),
  4: sparseF32Range().flatMap(f => [
    [f, 1.0, 2.0, 3.0],
    [1.0, f, 2.0, 3.0],
    [1.0, 2.0, f, 3.0],
    [1.0, 2.0, 3.0, f],
    [f, -1.0, -2.0, -3.0],
    [-1.0, f, -2.0, -3.0],
    [-1.0, -2.0, f, -3.0],
    [-1.0, -2.0, -3.0, f],
  ]),
};

/**
 * @returns the result matrix in Array<Array<number>> type.
 *
 * Matrix multiplication. A is m x n and B is n x p. Returns
 * m x p result.
 */
// A is m x n. B is n x p. product is m x p.
export function multiplyMatrices(
  A: Array<Array<number>>,
  B: Array<Array<number>>
): Array<Array<number>> {
  assert(A.length > 0 && B.length > 0 && B[0].length > 0 && A[0].length === B.length);
  const product = new Array<Array<number>>(A.length);
  for (let i = 0; i < product.length; ++i) {
    product[i] = new Array<number>(B[0].length).fill(0);
  }

  for (let m = 0; m < A.length; ++m) {
    for (let p = 0; p < B[0].length; ++p) {
      for (let n = 0; n < B.length; ++n) {
        product[m][p] += A[m][n] * B[n][p];
      }
    }
  }

  return product;
}

/** Sign-extend the `bits`-bit number `n` to a 32-bit signed integer. */
export function signExtend(n: number, bits: number): number {
  const shift = 32 - bits;
  return (n << shift) >> shift;
}

/** @returns the closest 32-bit floating point value to the input */
export function quantizeToF32(num: number): number {
  return f32(num).value as number;
}

/** @returns the closest 32-bit signed integer value to the input */
export function quantizeToI32(num: number): number {
  return i32(num).value as number;
}

/** @returns whether the number is an integer and a power of two */
export function isPowerOfTwo(n: number): boolean {
  if (!Number.isInteger(n)) {
    return false;
  }
  return n !== 0 && (n & (n - 1)) === 0;
}

/** @returns the Greatest Common Divisor (GCD) of the inputs */
export function gcd(a: number, b: number): number {
  assert(Number.isInteger(a) && a > 0);
  assert(Number.isInteger(b) && b > 0);

  while (b !== 0) {
    const bTemp = b;
    b = a % b;
    a = bTemp;
  }

  return a;
}

/** @returns the Least Common Multiplier (LCM) of the inputs */
export function lcm(a: number, b: number): number {
  return (a * b) / gcd(a, b);
}

/** Converts a 32-bit hex value to a 32-bit float value */
export function hexToF32(hex: number): number {
  return floatBitsToNumber(hex, kFloat32Format);
}

/** Converts a 16-bit hex value to a 16-bit float value */
export function hexToF16(hex: number): number {
  return floatBitsToNumber(hex, kFloat16Format);
}

/** Converts two 32-bit hex values to a 64-bit float value */
export function hexToF64(h32: number, l32: number): number {
  const u32Arr = new Uint32Array(2);
  u32Arr[0] = l32;
  u32Arr[1] = h32;
  const f64Arr = new Float64Array(u32Arr.buffer);
  return f64Arr[0];
}

/** @returns the cross of an array with the intermediate result of cartesianProduct
 *
 * @param elements array of values to cross with the intermediate result of
 *                 cartesianProduct
 * @param intermediate arrays of values representing the partial result of
 *                     cartesianProduct
 */
function cartesianProductImpl<T>(elements: T[], intermediate: T[][]): T[][] {
  const result: T[][] = [];
  elements.forEach(e => {
    if (intermediate.length > 0) {
      intermediate.forEach(a => {
        result.push(a.concat(e));
      });
    } else {
      result.push([e]);
    }
  });
  return result;
}

/** @returns the cartesian product (NxMx...) of a set of arrays
 *
 * This is implemented by calculating the cross of a single input against an
 * intermediate result for each input to build up the final array of arrays.
 *
 * There are examples of doing this more succinctly using map & reduce online,
 * but they are a bit more opaque to read.
 *
 * @param inputs arrays of numbers to calculate cartesian product over
 */
export function cartesianProduct<T>(...inputs: T[][]): T[][] {
  let result: T[][] = [];
  inputs.forEach(i => {
    result = cartesianProductImpl<T>(i, result);
  });

  return result;
}
