import { assert } from '../../common/util/util.js';

import { kBit, kValue } from './constants.js';
import { f32, f32Bits, i32, Scalar } from './conversion.js';

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
export function flushSubnormalNumber(val: number): number {
  const u32_val = new Uint32Array(new Float32Array([val]).buffer)[0];
  return (u32_val & 0x7f800000) === 0 ? 0 : val;
}

/**
 * @returns 0 if |val| is a bit field for a subnormal f32 number, otherwise
 * returns |val|
 * |val| is assumed to be a u32 value representing a f32
 */
function flushSubnormalBits(val: number): number {
  return (val & 0x7f800000) === 0 ? 0 : val;
}

/** @returns 0 if |val| is a subnormal f32 number, otherwise returns |val| */
export function flushSubnormalScalar(val: Scalar): Scalar {
  return isSubnormalScalar(val) ? f32(0) : val;
}

/**
 * @returns true if |val| is a subnormal f32 number, otherwise returns false
 * 0 is considered a non-subnormal number by this function.
 */
export function isSubnormalScalar(val: Scalar): boolean {
  if (val.type.kind !== 'f32') {
    return false;
  }

  if (val === f32(0)) {
    return false;
  }

  const u32_val = new Uint32Array(new Float32Array([val.value.valueOf() as number]).buffer)[0];
  return (u32_val & 0x7f800000) === 0;
}

/** Utility to pass TS numbers into |isSubnormalNumber| */
export function isSubnormalNumber(val: number): boolean {
  return isSubnormalScalar(f32(val));
}

/**
 * @returns the next single precision floating point value after |val|,
 * towards +inf if |dir| is true, otherwise towards -inf.
 * If |flush| is true, all subnormal values will be flushed to 0,
 * before processing.
 * If |flush| is false, the next subnormal will be calculated when appropriate,
 * and for -/+0 the nextAfter will be the closest subnormal in the correct
 * direction.
 * val needs to be in [min f32, max f32]
 */
export function nextAfter(val: number, dir: boolean = true, flush: boolean): Scalar {
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

  val = flush ? flushSubnormalNumber(val) : val;

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
    let u32_val: number = new Uint32Array(new Float32Array([val]).buffer)[0];
    const is_positive = (u32_val & 0x80000000) === 0;
    if (dir === is_positive) {
      u32_val += 1;
    } else {
      u32_val -= 1;
    }
    u32_result = flush ? flushSubnormalBits(u32_val) : u32_val;
  } else {
    // val had to be rounded to be expressed as a float32
    if (dir === converted > val) {
      // Rounding was in the direction requested
      u32_result = new Uint32Array(new Float32Array([converted]).buffer)[0];
    } else {
      // Round was opposite of the direction requested, so need nextAfter in the requested direction.
      // This will not recurse since converted is guaranteed to be a float32 due to the conversion above.
      const next = nextAfter(converted, dir, flush).value.valueOf() as number;
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

  return f32Bits(u32_result);
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
 * @param flush should subnormals be flushed to zero
 */
export function oneULP(target: number, flush: boolean): number {
  if (Number.isNaN(target)) {
    return Number.NaN;
  }

  if (flush) {
    target = flushSubnormalNumber(target);
  }

  // For values at the edge of the range or beyond ulp(x) is defined as  the distance between the two nearest
  // representable numbers to the appropriate edge.
  if (target === Number.POSITIVE_INFINITY || target >= kValue.f32.positive.max) {
    return kValue.f32.positive.max - kValue.f32.positive.nearest_max;
  } else if (target === Number.NEGATIVE_INFINITY || target <= kValue.f32.negative.min) {
    return kValue.f32.negative.nearest_min - kValue.f32.negative.min;
  } else {
    const converted: number = new Float32Array([target])[0];
    if (converted === target) {
      // |target| is precisely representable as a f32 so taking distance between it and the nearest neighbour in the
      // direction of 0.
      if (target > 0) {
        const a = nextAfter(target, false, flush).value.valueOf() as number;
        return target - a;
      } else if (target < 0) {
        const b = nextAfter(target, true, flush).value.valueOf() as number;
        return b - target;
      } else {
        // For 0 both neighbours should be the same distance, so just using the positive value and simplifying.
        return nextAfter(target, true, flush).value.valueOf() as number;
      }
    } else {
      // |target| is not precisely representable as a f32 so taking distance of neighbouring f32s.
      const b = nextAfter(target, true, flush).value.valueOf() as number;
      const a = nextAfter(target, false, flush).value.valueOf() as number;
      return b - a;
    }
  }
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

  const ulp_flush = oneULP(target, true);
  const ulp_noflush = oneULP(target, false);
  if (Number.isNaN(ulp_flush) || Number.isNaN(ulp_noflush)) {
    return false;
  }

  if (val === target) {
    return true;
  }

  const ulp = Math.max(ulp_flush, ulp_noflush);
  const diff = val > target ? val - target : target - val;
  return diff <= n * ulp;
}

/**
 * @returns if a test value is correctly rounded to an target value. Only
 * defined for |test_values| being a float32. target values may be any number.
 *
 * Correctly rounded means that if the target value is precisely expressible
 * as a float32, then |test_value| === |target|.
 * Otherwise |test_value| needs to be either the closest expressible number
 * greater or less than |target|.
 *
 * By default internally tests with both subnormals being flushed to 0 and not
 * being flushed, but |accept_to_zero| and |accept_no_flush| can be used to
 * control that behaviour. At least one accept flag must be true.
 */
export function correctlyRounded(
  test_value: Scalar,
  target: number,
  accept_to_zero: boolean = true,
  accept_no_flush: boolean = true
): boolean {
  assert(
    accept_to_zero || accept_no_flush,
    `At least one of |accept_to_zero| & |accept_no_flush| must be true`
  );

  let result: boolean = false;
  if (accept_to_zero) {
    result = result || correctlyRoundedImpl(test_value, target, true);
  }
  if (accept_no_flush) {
    result = result || correctlyRoundedImpl(test_value, target, false);
  }
  return result;
}

function correctlyRoundedImpl(test_value: Scalar, target: number, flush: boolean): boolean {
  assert(test_value.type.kind === 'f32', `${test_value} is expected to be a 'f32'`);

  if (Number.isNaN(target)) {
    return Number.isNaN(test_value.value.valueOf() as number);
  }

  if (target === Number.POSITIVE_INFINITY) {
    return test_value.value === f32Bits(kBit.f32.infinity.positive).value;
  }

  if (target === Number.NEGATIVE_INFINITY) {
    return test_value.value === f32Bits(kBit.f32.infinity.negative).value;
  }

  test_value = flush ? flushSubnormalScalar(test_value) : test_value;
  target = flush ? flushSubnormalNumber(target) : target;

  const target32 = new Float32Array([target])[0];
  const converted: number = target32;
  if (target === converted) {
    // expected is precisely expressible in float32
    return test_value.value === f32(target32).value;
  }

  let after_target: Scalar;
  let before_target: Scalar;

  if (converted > target) {
    // target32 is rounded towards +inf, so is after_target
    after_target = f32(target32);
    before_target = nextAfter(target32, false, flush);
  } else {
    // target32 is rounded towards -inf, so is before_target
    after_target = nextAfter(target32, true, flush);
    before_target = f32(target32);
  }

  return test_value.value === before_target.value || test_value.value === after_target.value;
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
 * Zero is included. The normal number regions are biased towards zero, and the subnormal regions are linearly spread.
 *
 * @param counts structure param with 4 entries indicating the number of entries to be generated each region, values must be 0 or greater.
 */
export function fullF32Range(
  counts: {
    neg_norm?: number;
    neg_sub?: number;
    pos_sub: number;
    pos_norm: number;
  } = { pos_sub: 10, pos_norm: 50 }
): Array<number> {
  counts.neg_norm = counts.neg_norm === undefined ? counts.pos_norm : (counts.neg_norm as number);
  counts.neg_sub = counts.neg_sub === undefined ? counts.pos_sub : (counts.neg_sub as number);
  return [
    ...biasedRange(kValue.f32.negative.max, kValue.f32.negative.min, counts.neg_norm),
    ...linearRange(
      kValue.f32.subnormal.negative.min,
      kValue.f32.subnormal.negative.max,
      counts.neg_sub
    ),
    0.0,
    ...linearRange(
      kValue.f32.subnormal.positive.min,
      kValue.f32.subnormal.positive.max,
      counts.pos_sub
    ),
    ...biasedRange(kValue.f32.positive.min, kValue.f32.positive.max, counts.pos_norm),
  ];
}

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
