export const description = `
Floating Point unit tests.
`;

import { makeTestGroup } from '../common/framework/test_group.js';
import { objectEquals, unreachable } from '../common/util/util.js';
import { kValue } from '../webgpu/util/constants.js';
import { FP, IntervalBounds } from '../webgpu/util/floating_point.js';
import { hexToF32, hexToF64, oneULPF32 } from '../webgpu/util/math.js';

import { UnitTest } from './unit_test.js';

export const g = makeTestGroup(UnitTest);

/** Bounds indicating an expectation of an interval of all possible values */
const kAnyBounds: IntervalBounds = [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY];

/** @returns a number N * ULP greater than the provided number, treats input as f32 */
function plusNULPF32(x: number, n: number): number {
  return x + n * oneULPF32(x);
}

/** @returns a number one ULP greater than the provided number, treats input as f32 */
function plusOneULPF32(x: number): number {
  return plusNULPF32(x, 1);
}

/** @returns a number N * ULP less than the provided number, treats input as f32 */
function minusNULPF32(x: number, n: number): number {
  return x - n * oneULPF32(x);
}

/** @returns a number one ULP less than the provided number, treats input as f32 */
function minusOneULPF32(x: number): number {
  return minusNULPF32(x, 1);
}

/** @returns the expected IntervalBounds adjusted by the given error function
 *
 * @param expected the bounds to be adjusted
 * @param error error function to adjust the bounds via
 */
function applyError(
  expected: number | IntervalBounds,
  error: (n: number) => number
): IntervalBounds {
  // Avoiding going through FPInterval to avoid tying this to a specific kind
  const unpack = (n: number | IntervalBounds): [number, number] => {
    if (expected instanceof Array) {
      switch (expected.length) {
        case 1:
          return [expected[0], expected[0]];
        case 2:
          return [expected[0], expected[1]];
      }
      unreachable(`Tried to unpack an IntervalBounds with length other than 1 or 2`);
    } else {
      // TS doesn't narrow this to number automatically
      return [n as number, n as number];
    }
  };

  let [begin, end] = unpack(expected);

  begin -= error(begin);
  end += error(end);

  if (begin === end) {
    return [begin];
  }
  return [begin, end];
}

interface ScalarToIntervalCase {
  input: number;
  expected: number | IntervalBounds;
}

g.test('absInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Common usages
      { input: 1, expected: 1 },
      { input: -1, expected: 1 },
      { input: 0.1, expected: [hexToF32(0x3dcccccc), hexToF32(0x3dcccccd)] },
      { input: -0.1, expected: [hexToF32(0x3dcccccc), hexToF32(0x3dcccccd)] },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.positive.max, expected: kValue.f32.positive.max },
      { input: kValue.f32.positive.min, expected: kValue.f32.positive.min },
      { input: kValue.f32.negative.min, expected: kValue.f32.positive.max },
      { input: kValue.f32.negative.max, expected: kValue.f32.positive.min },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [0, kValue.f32.subnormal.positive.max] },
      { input: kValue.f32.subnormal.positive.min, expected: [0, kValue.f32.subnormal.positive.min] },
      { input: kValue.f32.subnormal.negative.min, expected: [0, kValue.f32.subnormal.positive.max] },
      { input: kValue.f32.subnormal.negative.max, expected: [0, kValue.f32.subnormal.positive.min] },

      // 64-bit subnormals
      { input: hexToF64(0x0000_0000_0000_0001n), expected: [0, kValue.f32.subnormal.positive.min] },
      { input: hexToF64(0x800f_ffff_ffff_ffffn), expected: [0, kValue.f32.subnormal.positive.min] },

      // Zero
      { input: 0, expected: 0 },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.absInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.absInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('acosInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the complexity of
      // their derivation.
      //
      // The acceptance interval @ x = -1 and 1 is kAnyBounds, because
      // sqrt(1 - x*x) = sqrt(0), and sqrt is defined in terms of inverseqrt
      // The acceptance interval @ x = 0 is kAnyBounds, because atan2 is not
      // well-defined/implemented at 0.
      // Near 1, the absolute error should be larger and, away from 1 the atan2
      // inherited error should be larger.
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: -1, expected: kAnyBounds },
      { input: -1/2, expected: [hexToF32(0x4005fa91), hexToF32(0x40061a94)] },  // ~2π/3
      { input: 0, expected: kAnyBounds },
      { input: 1/2, expected: [hexToF32(0x3f85fa8f), hexToF32(0x3f861a94)] },  // ~π/3
      { input: minusOneULPF32(1), expected: [hexToF64(0x3f2f_fdff_6000_0000n), hexToF64(0x3f3b_106f_c933_4fb9n)] },  // ~0.0003
      { input: 1, expected: kAnyBounds },
      { input: kValue.f32.positive.max, expected: kAnyBounds },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.acosInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.acosInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('acoshAlternativeInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: -1, expected: kAnyBounds },
      { input: 0, expected: kAnyBounds },
      { input: 1, expected: kAnyBounds },  // 1/0 occurs in inverseSqrt in this formulation
      { input: 1.1, expected: [hexToF64(0x3fdc_6368_8000_0000n), hexToF64(0x3fdc_636f_2000_0000n)] },  // ~0.443..., differs from the primary in the later digits
      { input: 10, expected: [hexToF64(0x4007_f21e_4000_0000n), hexToF64(0x4007_f21f_6000_0000n)] },  // ~2.993...
      { input: kValue.f32.positive.max, expected: kAnyBounds },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.acoshAlternativeInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.acoshInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('acoshPrimaryInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: -1, expected: kAnyBounds },
      { input: 0, expected: kAnyBounds },
      { input: 1, expected: kAnyBounds },  // 1/0 occurs in inverseSqrt in this formulation
      { input: 1.1, expected: [hexToF64(0x3fdc_6368_2000_0000n), hexToF64(0x3fdc_636f_8000_0000n)] }, // ~0.443..., differs from the alternative in the later digits
      { input: 10, expected: [hexToF64(0x4007_f21e_4000_0000n), hexToF64(0x4007_f21f_6000_0000n)] },  // ~2.993...
      { input: kValue.f32.positive.max, expected: kAnyBounds },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.acoshPrimaryInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.acoshInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('asinInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a simple human-readable form due to the complexity of their derivation.
      //
      // The acceptance interval @ x = -1 and 1 is kAnyBounds, because
      // sqrt(1 - x*x) = sqrt(0), and sqrt is defined in terms of inversqrt.
      // The acceptance interval @ x = 0 is kAnyBounds, because atan2 is not
      // well-defined/implemented at 0.
      // Near 0, but not subnormal the absolute error should be larger, so will
      // be +/- 6.77e-5, away from 0 the atan2 inherited error should be larger.
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: -1, expected: kAnyBounds },
      { input: -1/2, expected: [hexToF64(0xbfe0_c352_c000_0000n), hexToF64(0xbfe0_bf51_c000_0000n)] },  // ~-π/6
      { input: kValue.f32.negative.max, expected: [-6.77e-5, 6.77e-5] },  // ~0
      { input: 0, expected: kAnyBounds },
      { input: kValue.f32.positive.min, expected: [-6.77e-5, 6.77e-5] },  // ~0
      { input: 1/2, expected: [hexToF64(0x3fe0_bf51_c000_0000n), hexToF64(0x3fe0_c352_c000_0000n)] },  // ~π/6
      { input: 1, expected: kAnyBounds },  // ~π/2
      { input: kValue.f32.positive.max, expected: kAnyBounds },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.asinInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.asinInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('asinhInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: -1, expected: [hexToF64(0xbfec_343a_8000_0000n), hexToF64(0xbfec_3432_8000_0000n)] },  // ~-0.88137...
      { input: 0, expected: [hexToF64(0xbeaa_0000_2000_0000n), hexToF64(0x3eb1_ffff_d000_0000n)] },  // ~0
      { input: 1, expected: [hexToF64(0x3fec_3435_4000_0000n), hexToF64(0x3fec_3437_8000_0000n)] },  // ~0.88137...
      { input: kValue.f32.positive.max, expected: kAnyBounds },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.asinhInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.asinhInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('atanInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: hexToF32(0xbfddb3d7), expected: [kValue.f32.negative.pi.third, plusOneULPF32(kValue.f32.negative.pi.third)] }, // x = -√3
      { input: -1, expected: [kValue.f32.negative.pi.quarter, plusOneULPF32(kValue.f32.negative.pi.quarter)] },
      { input: hexToF32(0xbf13cd3a), expected: [kValue.f32.negative.pi.sixth, plusOneULPF32(kValue.f32.negative.pi.sixth)] },  // x = -1/√3
      { input: 0, expected: 0 },
      { input: hexToF32(0x3f13cd3a), expected: [minusOneULPF32(kValue.f32.positive.pi.sixth), kValue.f32.positive.pi.sixth] },  // x = 1/√3
      { input: 1, expected: [minusOneULPF32(kValue.f32.positive.pi.quarter), kValue.f32.positive.pi.quarter] },
      { input: hexToF32(0x3fddb3d7), expected: [minusOneULPF32(kValue.f32.positive.pi.third), kValue.f32.positive.pi.third] }, // x = √3
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const error = (n: number): number => {
      return 4096 * oneULPF32(n);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.atanInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.atanInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('atanhInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature of the errors.
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: -1, expected: kAnyBounds },
      { input: -0.1, expected: [hexToF64(0xbfb9_af9a_6000_0000n), hexToF64(0xbfb9_af8c_c000_0000n)] },  // ~-0.1003...
      { input: 0, expected: [hexToF64(0xbe96_0000_2000_0000n), hexToF64(0x3e98_0000_0000_0000n)] },  // ~0
      { input: 0.1, expected: [hexToF64(0x3fb9_af8b_8000_0000n), hexToF64(0x3fb9_af9b_0000_0000n)] },  // ~0.1003...
      { input: 1, expected: kAnyBounds },
      { input: kValue.f32.positive.max, expected: kAnyBounds },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.atanhInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.atanhInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('ceilInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: 0 },
      { input: 0.1, expected: 1 },
      { input: 0.9, expected: 1 },
      { input: 1.0, expected: 1 },
      { input: 1.1, expected: 2 },
      { input: 1.9, expected: 2 },
      { input: -0.1, expected: 0 },
      { input: -0.9, expected: 0 },
      { input: -1.0, expected: -1 },
      { input: -1.1, expected: -1 },
      { input: -1.9, expected: -1 },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.positive.max, expected: kValue.f32.positive.max },
      { input: kValue.f32.positive.min, expected: 1 },
      { input: kValue.f32.negative.min, expected: kValue.f32.negative.min },
      { input: kValue.f32.negative.max, expected: 0 },
      { input: kValue.powTwo.to30, expected: kValue.powTwo.to30 },
      { input: -kValue.powTwo.to30, expected: -kValue.powTwo.to30 },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [0, 1] },
      { input: kValue.f32.subnormal.positive.min, expected: [0, 1] },
      { input: kValue.f32.subnormal.negative.min, expected: 0 },
      { input: kValue.f32.subnormal.negative.max, expected: 0 },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.ceilInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.ceilInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('cosInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // This test does not include some common cases. i.e. f(x = π/2) = 0,
      // because the difference between true x and x as a f32 is sufficiently
      // large, such that the high slope of f @ x causes the results to be
      // substantially different, so instead of getting 0 you get a value on the
      // order of 10^-8 away from 0, thus difficult to express in a
      // human-readable manner.
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: kValue.f32.negative.pi.whole, expected: [-1, plusOneULPF32(-1)] },
      { input: kValue.f32.negative.pi.third, expected: [minusOneULPF32(1/2), 1/2] },
      { input: 0, expected: [1, 1] },
      { input: kValue.f32.positive.pi.third, expected: [minusOneULPF32(1/2), 1/2] },
      { input: kValue.f32.positive.pi.whole, expected: [-1, plusOneULPF32(-1)] },
      { input: kValue.f32.positive.max, expected: kAnyBounds },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const error = (_: number): number => {
      return 2 ** -11;
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.cosInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.cosInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('coshInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: -1, expected: [ hexToF32(0x3fc583a4), hexToF32(0x3fc583b1)] },  // ~1.1543...
      { input: 0, expected: [hexToF32(0x3f7ffffd), hexToF32(0x3f800002)] },  // ~1
      { input: 1, expected: [ hexToF32(0x3fc583a4), hexToF32(0x3fc583b1)] },  // ~1.1543...
      { input: kValue.f32.positive.max, expected: kAnyBounds },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.coshInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.coshInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('degreesInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: kValue.f32.negative.pi.whole, expected: [minusOneULPF32(-180), plusOneULPF32(-180)] },
      { input: kValue.f32.negative.pi.three_quarters, expected: [minusOneULPF32(-135), plusOneULPF32(-135)] },
      { input: kValue.f32.negative.pi.half, expected: [minusOneULPF32(-90), plusOneULPF32(-90)] },
      { input: kValue.f32.negative.pi.third, expected: [minusOneULPF32(-60), plusOneULPF32(-60)] },
      { input: kValue.f32.negative.pi.quarter, expected: [minusOneULPF32(-45), plusOneULPF32(-45)] },
      { input: kValue.f32.negative.pi.sixth, expected: [minusOneULPF32(-30), plusOneULPF32(-30)] },
      { input: 0, expected: 0 },
      { input: kValue.f32.positive.pi.sixth, expected: [minusOneULPF32(30), plusOneULPF32(30)] },
      { input: kValue.f32.positive.pi.quarter, expected: [minusOneULPF32(45), plusOneULPF32(45)] },
      { input: kValue.f32.positive.pi.third, expected: [minusOneULPF32(60), plusOneULPF32(60)] },
      { input: kValue.f32.positive.pi.half, expected: [minusOneULPF32(90), plusOneULPF32(90)] },
      { input: kValue.f32.positive.pi.three_quarters, expected: [minusOneULPF32(135), plusOneULPF32(135)] },
      { input: kValue.f32.positive.pi.whole, expected: [minusOneULPF32(180), plusOneULPF32(180)] },
      { input: kValue.f32.positive.max, expected: kAnyBounds },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.degreesInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.degreesInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('expInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: 0, expected: 1 },
      { input: 1, expected: [kValue.f32.positive.e, plusOneULPF32(kValue.f32.positive.e)] },
      { input: 89, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const error = (x: number): number => {
      const n = 3 + 2 * Math.abs(t.params.input);
      return n * oneULPF32(x);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.expInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.expInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('exp2Interval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: 0, expected: 1 },
      { input: 1, expected: 2 },
      { input: 128, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const error = (x: number): number => {
      const n = 3 + 2 * Math.abs(t.params.input);
      return n * oneULPF32(x);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.exp2Interval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.exp2Interval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('floorInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: 0 },
      { input: 0.1, expected: 0 },
      { input: 0.9, expected: 0 },
      { input: 1.0, expected: 1 },
      { input: 1.1, expected: 1 },
      { input: 1.9, expected: 1 },
      { input: -0.1, expected: -1 },
      { input: -0.9, expected: -1 },
      { input: -1.0, expected: -1 },
      { input: -1.1, expected: -2 },
      { input: -1.9, expected: -2 },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.positive.max, expected: kValue.f32.positive.max },
      { input: kValue.f32.positive.min, expected: 0 },
      { input: kValue.f32.negative.min, expected: kValue.f32.negative.min },
      { input: kValue.f32.negative.max, expected: -1 },
      { input: kValue.powTwo.to30, expected: kValue.powTwo.to30 },
      { input: -kValue.powTwo.to30, expected: -kValue.powTwo.to30 },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: 0 },
      { input: kValue.f32.subnormal.positive.min, expected: 0 },
      { input: kValue.f32.subnormal.negative.min, expected: [-1, 0] },
      { input: kValue.f32.subnormal.negative.max, expected: [-1, 0] },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.floorInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.floorInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('fractInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: 0 },
      { input: 0.1, expected: [minusOneULPF32(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] }, // ~0.1
      { input: 0.9, expected: [hexToF32(0x3f666666), plusOneULPF32(hexToF32(0x3f666666))] },  // ~0.9
      { input: 1.0, expected: 0 },
      { input: 1.1, expected: [hexToF64(0x3fb9_9998_0000_0000n), hexToF64(0x3fb9_999a_0000_0000n)] }, // ~0.1
      { input: -0.1, expected: [hexToF32(0x3f666666), plusOneULPF32(hexToF32(0x3f666666))] },  // ~0.9
      { input: -0.9, expected: [hexToF64(0x3fb9_9999_0000_0000n), hexToF64(0x3fb9_999a_0000_0000n)] }, // ~0.1
      { input: -1.0, expected: 0 },
      { input: -1.1, expected: [hexToF64(0x3fec_cccc_c000_0000n), hexToF64(0x3fec_cccd_0000_0000n), ] }, // ~0.9

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.positive.max, expected: 0 },
      { input: kValue.f32.positive.min, expected: [kValue.f32.positive.min, kValue.f32.positive.min] },
      { input: kValue.f32.negative.min, expected: 0 },
      { input: kValue.f32.negative.max, expected: [kValue.f32.positive.less_than_one, 1.0] },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.fractInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.fractInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('inverseSqrtInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: -1, expected: kAnyBounds },
      { input: 0, expected: kAnyBounds },
      { input: 0.04, expected: [minusOneULPF32(5), plusOneULPF32(5)] },
      { input: 1, expected: 1 },
      { input: 100, expected: [minusOneULPF32(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: kValue.f32.positive.max, expected: [hexToF32(0x1f800000), plusNULPF32(hexToF32(0x1f800000), 2)] },  // ~5.421...e-20, i.e. 1/√max f32
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const error = (n: number): number => {
      return 2 * oneULPF32(n);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.inverseSqrtInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.inverseSqrtInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('lengthIntervalScalar_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      //
      // length(0) = kAnyBounds, because length uses sqrt, which is defined as 1/inversesqrt
      {input: 0, expected: kAnyBounds },
      {input: 1.0, expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: -1.0, expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: 0.1, expected: [hexToF64(0x3fb9_9998_9000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      {input: -0.1, expected: [hexToF64(0x3fb9_9998_9000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      {input: 10.0, expected: [hexToF64(0x4023_ffff_7000_0000n), hexToF64(0x4024_0000_b000_0000n)] },  // ~10
      {input: -10.0, expected: [hexToF64(0x4023_ffff_7000_0000n), hexToF64(0x4024_0000_b000_0000n)] },  // ~10

      // Subnormal Cases
      { input: kValue.f32.subnormal.negative.min, expected: kAnyBounds },
      { input: kValue.f32.subnormal.negative.max, expected: kAnyBounds },
      { input: kValue.f32.subnormal.positive.min, expected: kAnyBounds },
      { input: kValue.f32.subnormal.positive.max, expected: kAnyBounds },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: kValue.f32.negative.max, expected: kAnyBounds },
      { input: kValue.f32.positive.min, expected: kAnyBounds },
      { input: kValue.f32.positive.max, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.lengthInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.lengthInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('logInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: -1, expected: kAnyBounds },
      { input: 0, expected: kAnyBounds },
      { input: 1, expected: 0 },
      { input: kValue.f32.positive.e, expected: [minusOneULPF32(1), 1] },
      { input: kValue.f32.positive.max, expected: [minusOneULPF32(hexToF32(0x42b17218)), hexToF32(0x42b17218)] },  // ~88.72...
    ]
  )
  .fn(t => {
    const error = (n: number): number => {
      if (t.params.input >= 0.5 && t.params.input <= 2.0) {
        return 2 ** -21;
      }
      return 3 * oneULPF32(n);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.logInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.logInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('log2Interval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: -1, expected: kAnyBounds },
      { input: 0, expected: kAnyBounds },
      { input: 1, expected: 0 },
      { input: 2, expected: 1 },
      { input: kValue.f32.positive.max, expected: [minusOneULPF32(128), 128] },
    ]
  )
  .fn(t => {
    const error = (n: number): number => {
      if (t.params.input >= 0.5 && t.params.input <= 2.0) {
        return 2 ** -21;
      }
      return 3 * oneULPF32(n);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.log2Interval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.log2Interval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('negationInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: 0 },
      { input: 0.1, expected: [hexToF32(0xbdcccccd), plusOneULPF32(hexToF32(0xbdcccccd))] }, // ~-0.1
      { input: 1.0, expected: -1.0 },
      { input: 1.9, expected: [hexToF32(0xbff33334), plusOneULPF32(hexToF32(0xbff33334))] },  // ~-1.9
      { input: -0.1, expected: [minusOneULPF32(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] }, // ~0.1
      { input: -1.0, expected: 1 },
      { input: -1.9, expected: [minusOneULPF32(hexToF32(0x3ff33334)), hexToF32(0x3ff33334)] },  // ~1.9

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.positive.max, expected: kValue.f32.negative.min },
      { input: kValue.f32.positive.min, expected: kValue.f32.negative.max },
      { input: kValue.f32.negative.min, expected: kValue.f32.positive.max },
      { input: kValue.f32.negative.max, expected: kValue.f32.positive.min },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: kValue.f32.subnormal.positive.min, expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: kValue.f32.subnormal.negative.min, expected: [0, kValue.f32.subnormal.positive.max] },
      { input: kValue.f32.subnormal.negative.max, expected: [0, kValue.f32.subnormal.positive.min] },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.negationInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.negationInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('quantizeToF16Interval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: kValue.f16.negative.min, expected: kValue.f16.negative.min },
      { input: -1, expected: -1 },
      { input: -0.1, expected: [hexToF32(0xbdcce000), hexToF32(0xbdccc000)] },  // ~-0.1
      { input: kValue.f16.negative.max, expected: kValue.f16.negative.max },
      { input: kValue.f16.subnormal.negative.min, expected: [kValue.f16.subnormal.negative.min, 0] },
      { input: kValue.f16.subnormal.negative.max, expected: [kValue.f16.subnormal.negative.max, 0] },
      { input: kValue.f32.subnormal.negative.max, expected: [kValue.f16.subnormal.negative.max, 0] },
      { input: 0, expected: 0 },
      { input: kValue.f32.subnormal.positive.min, expected: [0, kValue.f16.subnormal.positive.min] },
      { input: kValue.f16.subnormal.positive.min, expected: [0, kValue.f16.subnormal.positive.min] },
      { input: kValue.f16.subnormal.positive.max, expected: [0, kValue.f16.subnormal.positive.max] },
      { input: kValue.f16.positive.min, expected: kValue.f16.positive.min },
      { input: 0.1, expected: [hexToF32(0x3dccc000), hexToF32(0x3dcce000)] },  // ~0.1
      { input: 1, expected: 1 },
      { input: kValue.f16.positive.max, expected: kValue.f16.positive.max },
      { input: kValue.f32.positive.max, expected: kAnyBounds },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.quantizeToF16Interval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.quantizeToF16Interval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('radiansInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: -180, expected: [minusOneULPF32(kValue.f32.negative.pi.whole), plusOneULPF32(kValue.f32.negative.pi.whole)] },
      { input: -135, expected: [minusOneULPF32(kValue.f32.negative.pi.three_quarters), plusOneULPF32(kValue.f32.negative.pi.three_quarters)] },
      { input: -90, expected: [minusOneULPF32(kValue.f32.negative.pi.half), plusOneULPF32(kValue.f32.negative.pi.half)] },
      { input: -60, expected: [minusOneULPF32(kValue.f32.negative.pi.third), plusOneULPF32(kValue.f32.negative.pi.third)] },
      { input: -45, expected: [minusOneULPF32(kValue.f32.negative.pi.quarter), plusOneULPF32(kValue.f32.negative.pi.quarter)] },
      { input: -30, expected: [minusOneULPF32(kValue.f32.negative.pi.sixth), plusOneULPF32(kValue.f32.negative.pi.sixth)] },
      { input: 0, expected: 0 },
      { input: 30, expected: [minusOneULPF32(kValue.f32.positive.pi.sixth), plusOneULPF32(kValue.f32.positive.pi.sixth)] },
      { input: 45, expected: [minusOneULPF32(kValue.f32.positive.pi.quarter), plusOneULPF32(kValue.f32.positive.pi.quarter)] },
      { input: 60, expected: [minusOneULPF32(kValue.f32.positive.pi.third), plusOneULPF32(kValue.f32.positive.pi.third)] },
      { input: 90, expected: [minusOneULPF32(kValue.f32.positive.pi.half), plusOneULPF32(kValue.f32.positive.pi.half)] },
      { input: 135, expected: [minusOneULPF32(kValue.f32.positive.pi.three_quarters), plusOneULPF32(kValue.f32.positive.pi.three_quarters)] },
      { input: 180, expected: [minusOneULPF32(kValue.f32.positive.pi.whole), plusOneULPF32(kValue.f32.positive.pi.whole)] },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.radiansInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.radiansInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('roundInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: 0 },
      { input: 0.1, expected: 0 },
      { input: 0.5, expected: 0 },  // Testing tie breaking
      { input: 0.9, expected: 1 },
      { input: 1.0, expected: 1 },
      { input: 1.1, expected: 1 },
      { input: 1.5, expected: 2 },  // Testing tie breaking
      { input: 1.9, expected: 2 },
      { input: -0.1, expected: 0 },
      { input: -0.5, expected: 0 },  // Testing tie breaking
      { input: -0.9, expected: -1 },
      { input: -1.0, expected: -1 },
      { input: -1.1, expected: -1 },
      { input: -1.5, expected: -2 },  // Testing tie breaking
      { input: -1.9, expected: -2 },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.positive.max, expected: kValue.f32.positive.max },
      { input: kValue.f32.positive.min, expected: 0 },
      { input: kValue.f32.negative.min, expected: kValue.f32.negative.min },
      { input: kValue.f32.negative.max, expected: 0 },
      { input: kValue.powTwo.to30, expected: kValue.powTwo.to30 },
      { input: -kValue.powTwo.to30, expected: -kValue.powTwo.to30 },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: 0 },
      { input: kValue.f32.subnormal.positive.min, expected: 0 },
      { input: kValue.f32.subnormal.negative.min, expected: 0 },
      { input: kValue.f32.subnormal.negative.max, expected: 0 },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.roundInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.roundInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('saturateInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Normals
      { input: 0, expected: 0 },
      { input: 1, expected: 1.0 },
      { input: -0.1, expected: 0 },
      { input: -1, expected: 0 },
      { input: -10, expected: 0 },
      { input: 0.1, expected: [minusOneULPF32(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: 10, expected: 1.0 },
      { input: 11.1, expected: 1.0 },
      { input: kValue.f32.positive.max, expected: 1.0 },
      { input: kValue.f32.positive.min, expected: kValue.f32.positive.min },
      { input: kValue.f32.negative.max, expected: 0.0 },
      { input: kValue.f32.negative.min, expected: 0.0 },

      // Subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [0.0, kValue.f32.subnormal.positive.max] },
      { input: kValue.f32.subnormal.positive.min, expected: [0.0, kValue.f32.subnormal.positive.min] },
      { input: kValue.f32.subnormal.negative.min, expected: [kValue.f32.subnormal.negative.min, 0.0] },
      { input: kValue.f32.subnormal.negative.max, expected: [kValue.f32.subnormal.negative.max, 0.0] },

      // Infinities
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.saturateInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.saturationInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('signInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: -1 },
      { input: -10, expected: -1 },
      { input: -1, expected: -1 },
      { input: -0.1, expected: -1 },
      { input: kValue.f32.negative.max, expected:  -1 },
      { input: kValue.f32.subnormal.negative.min, expected: [-1, 0] },
      { input: kValue.f32.subnormal.negative.max, expected: [-1, 0] },
      { input: 0, expected: 0 },
      { input: kValue.f32.subnormal.positive.max, expected: [0, 1] },
      { input: kValue.f32.subnormal.positive.min, expected: [0, 1] },
      { input: kValue.f32.positive.min, expected: 1 },
      { input: 0.1, expected: 1 },
      { input: 1, expected: 1 },
      { input: 10, expected: 1 },
      { input: kValue.f32.positive.max, expected: 1 },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.signInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.signInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('sinInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // This test does not include some common cases, i.e. f(x = -π|π) = 0,
      // because the difference between true x and x as a f32 is sufficiently
      // large, such that the high slope of f @ x causes the results to be
      // substantially different, so instead of getting 0 you get a value on the
      // order of 10^-8 away from it, thus difficult to express in a
      // human-readable manner.
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: kValue.f32.negative.pi.half, expected: [-1, plusOneULPF32(-1)] },
      { input: 0, expected: 0 },
      { input: kValue.f32.positive.pi.half, expected: [minusOneULPF32(1), 1] },
      { input: kValue.f32.positive.max, expected: kAnyBounds },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const error = (_: number): number => {
      return 2 ** -11;
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.sinInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.sinInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('sinhInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: -1, expected: [ hexToF32(0xbf966d05), hexToF32(0xbf966cf8)] },  // ~-1.175...
      { input: 0, expected: [hexToF32(0xb4600000), hexToF32(0x34600000)] },  // ~0
      { input: 1, expected: [ hexToF32(0x3f966cf8), hexToF32(0x3f966d05)] },  // ~1.175...
      { input: kValue.f32.positive.max, expected: kAnyBounds },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.sinhInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.sinhInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('sqrtInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: -1, expected: kAnyBounds },
      { input: 0, expected: kAnyBounds },
      { input: 0.01, expected: [hexToF64(0x3fb9_9998_b000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      { input: 1, expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: 4, expected: [hexToF64(0x3fff_ffff_7000_0000n), hexToF64(0x4000_0000_9000_0000n)] },  // ~2
      { input: 100, expected: [hexToF64(0x4023_ffff_7000_0000n), hexToF64(0x4024_0000_b000_0000n)] },  // ~10
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.sqrtInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.sqrtInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('tanInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // All of these are hard coded, since the error intervals are difficult to
      // express in a closed human--readable form.
      // Some easy looking cases like f(x = -π|π) = 0 are actually quite
      // difficult. This is because the interval is calculated from the results
      // of sin(x)/cos(x), which becomes very messy at x = -π|π, since π is
      // irrational, thus does not have an exact representation as a f32.
      //
      // Even at 0, which has a precise f32 value, there is still the problem
      // that result of sin(0) and cos(0) will be intervals due to the inherited
      // nature of errors, so the proper interval will be an interval calculated
      // from dividing an interval by another interval and applying an error
      // function to that.
      //
      // This complexity is why the entire interval framework was developed.
      //
      // The examples here have been manually traced to confirm the expectation
      // values are correct.
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: kValue.f32.negative.pi.whole, expected: [hexToF64(0xbf40_02bc_9000_0000n), hexToF64(0x3f40_0144_f000_0000n)] },  // ~0.0
      { input: kValue.f32.negative.pi.half, expected: kAnyBounds },
      { input: 0, expected: [hexToF64(0xbf40_0200_b000_0000n), hexToF64(0x3f40_0200_b000_0000n)] },  // ~0.0
      { input: kValue.f32.positive.pi.half, expected: kAnyBounds },
      { input: kValue.f32.positive.pi.whole, expected: [hexToF64(0xbf40_0144_f000_0000n), hexToF64(0x3f40_02bc_9000_0000n)] },  // ~0.0
      { input: kValue.f32.positive.max, expected: kAnyBounds },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.tanInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.tanInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('tanhInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.negative.min, expected: kAnyBounds },
      { input: -1, expected: [hexToF64(0xbfe8_5efd_1000_0000n), hexToF64(0xbfe8_5ef8_9000_0000n)] },  // ~-0.7615...
      { input: 0, expected: [hexToF64(0xbe8c_0000_b000_0000n), hexToF64(0x3e8c_0000_b000_0000n)] },  // ~0
      { input: 1, expected: [hexToF64(0x3fe8_5ef8_9000_0000n), hexToF64(0x3fe8_5efd_1000_0000n)] },  // ~0.7615...
      { input: kValue.f32.positive.max, expected: kAnyBounds },
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.tanhInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.tanhInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('truncInterval_f32')
  .paramsSubcasesOnly<ScalarToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: 0 },
      { input: 0.1, expected: 0 },
      { input: 0.9, expected: 0 },
      { input: 1.0, expected: 1 },
      { input: 1.1, expected: 1 },
      { input: 1.9, expected: 1 },
      { input: -0.1, expected: 0 },
      { input: -0.9, expected: 0 },
      { input: -1.0, expected: -1 },
      { input: -1.1, expected: -1 },
      { input: -1.9, expected: -1 },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAnyBounds },
      { input: kValue.f32.infinity.negative, expected: kAnyBounds },
      { input: kValue.f32.positive.max, expected: kValue.f32.positive.max },
      { input: kValue.f32.positive.min, expected: 0 },
      { input: kValue.f32.negative.min, expected: kValue.f32.negative.min },
      { input: kValue.f32.negative.max, expected: 0 },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: 0 },
      { input: kValue.f32.subnormal.positive.min, expected: 0 },
      { input: kValue.f32.subnormal.negative.min, expected: 0 },
      { input: kValue.f32.subnormal.negative.max, expected: 0 },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.truncInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.truncInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

interface ScalarPairToIntervalCase {
  // input is a pair of independent values, not a range, so should not be
  // converted to a FPInterval.
  input: [number, number];
  expected: number | IntervalBounds;
}

g.test('additionInterval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: 0 },
      { input: [1, 0], expected: 1 },
      { input: [0, 1], expected: 1 },
      { input: [-1, 0], expected: -1 },
      { input: [0, -1], expected: -1 },
      { input: [1, 1], expected: 2 },
      { input: [1, -1], expected: 0 },
      { input: [-1, 1], expected: 0 },
      { input: [-1, -1], expected: -2 },

      // 64-bit normals
      { input: [0.1, 0], expected: [minusOneULPF32(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0, 0.1], expected: [minusOneULPF32(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [-0.1, 0], expected: [hexToF32(0xbdcccccd), plusOneULPF32(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [0, -0.1], expected: [hexToF32(0xbdcccccd), plusOneULPF32(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [0.1, 0.1], expected: [minusOneULPF32(hexToF32(0x3e4ccccd)), hexToF32(0x3e4ccccd)] },  // ~0.2
      { input: [0.1, -0.1], expected: [minusOneULPF32(hexToF32(0x3dcccccd)) - hexToF32(0x3dcccccd), hexToF32(0x3dcccccd) - minusOneULPF32(hexToF32(0x3dcccccd))] }, // ~0
      { input: [-0.1, 0.1], expected: [minusOneULPF32(hexToF32(0x3dcccccd)) - hexToF32(0x3dcccccd), hexToF32(0x3dcccccd) - minusOneULPF32(hexToF32(0x3dcccccd))] }, // ~0
      { input: [-0.1, -0.1], expected: [hexToF32(0xbe4ccccd), plusOneULPF32(hexToF32(0xbe4ccccd))] },  // ~-0.2

      // 32-bit subnormals
      { input: [kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.min, 0], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, kValue.f32.subnormal.positive.min], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [kValue.f32.subnormal.negative.max, 0], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [0, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [kValue.f32.subnormal.negative.min, 0], expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: [0, kValue.f32.subnormal.negative.min], expected: [kValue.f32.subnormal.negative.min, 0] },

      // Infinities
      { input: [0, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, 0], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [0, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, 0], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.additionInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.additionInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

// Note: atan2's parameters are labelled (y, x) instead of (x, y)
g.test('atan2Interval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      //
      // The positive x & y quadrant is tested in more detail, and the other
      // quadrants are spot checked that values are pointing in the right
      // direction.
      //
      // Some of the intervals appear slightly asymmetric,
      // i.e. [π/4 - 4097 * ULPF32(π/4), π/4 + 4096 * ULPF32(π/4)],
      // this is because π/4 is not precisely expressible as a f32, so the
      // higher precision value can be rounded up or down when converting to
      // f32. Thus, one option will be 1 ULP off of the constant value being
      // used.

      // positive y, positive x
      { input: [1, hexToF32(0x3fddb3d7)], expected: [minusNULPF32(kValue.f32.positive.pi.sixth, 4097), plusNULPF32(kValue.f32.positive.pi.sixth, 4096)] },  // x = √3
      { input: [1, 1], expected: [minusNULPF32(kValue.f32.positive.pi.quarter, 4097), plusNULPF32(kValue.f32.positive.pi.quarter, 4096)] },
      // { input: [hexToF32(0x3fddb3d7), 1], expected: [hexToF64(0x3ff0_bf52_0000_0000n), hexToF64(0x3ff0_c352_6000_0000n)] },  // y = √3
      { input: [Number.POSITIVE_INFINITY, 1], expected: kAnyBounds },

      // positive y, negative x
      { input: [1, -1], expected: [minusNULPF32(kValue.f32.positive.pi.three_quarters, 4096), plusNULPF32(kValue.f32.positive.pi.three_quarters, 4097)] },
      { input: [Number.POSITIVE_INFINITY, -1], expected: kAnyBounds },

      // negative y, negative x
      { input: [-1, -1], expected: [minusNULPF32(kValue.f32.negative.pi.three_quarters, 4097), plusNULPF32(kValue.f32.negative.pi.three_quarters, 4096)] },
      { input: [Number.NEGATIVE_INFINITY, -1], expected: kAnyBounds },

      // negative y, positive x
      { input: [-1, 1], expected: [minusNULPF32(kValue.f32.negative.pi.quarter, 4096), plusNULPF32(kValue.f32.negative.pi.quarter, 4097)] },
      { input: [Number.NEGATIVE_INFINITY, 1], expected: kAnyBounds },

      // Discontinuity @ origin (0,0)
      { input: [0, 0], expected: kAnyBounds },
      { input: [0, kValue.f32.subnormal.positive.max], expected: kAnyBounds },
      { input: [0, kValue.f32.subnormal.negative.min], expected: kAnyBounds },
      { input: [0, kValue.f32.positive.min], expected: kAnyBounds },
      { input: [0, kValue.f32.negative.max], expected: kAnyBounds },
      { input: [0, kValue.f32.positive.max], expected: kAnyBounds },
      { input: [0, kValue.f32.negative.min], expected: kAnyBounds },
      { input: [0, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [0, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [0, 1], expected: kAnyBounds },
      { input: [kValue.f32.subnormal.positive.max, 1], expected: kAnyBounds },
      { input: [kValue.f32.subnormal.negative.min, 1], expected: kAnyBounds },

      // When atan(y/x) ~ 0, test that ULP applied to result of atan2, not the intermediate atan(y/x) value
      {input: [hexToF32(0x80800000), hexToF32(0xbf800000)], expected: [minusNULPF32(kValue.f32.negative.pi.whole, 4096), plusNULPF32(kValue.f32.negative.pi.whole, 4096)] },
      {input: [hexToF32(0x00800000), hexToF32(0xbf800000)], expected: [minusNULPF32(kValue.f32.positive.pi.whole, 4096), plusNULPF32(kValue.f32.positive.pi.whole, 4096)] },

      // Very large |x| values should cause kAnyBounds to be returned, due to the restrictions on division
      { input: [1, kValue.f32.positive.max], expected: kAnyBounds },
      { input: [1, kValue.f32.positive.nearest_max], expected: kAnyBounds },
      { input: [1, kValue.f32.negative.min], expected: kAnyBounds },
      { input: [1, kValue.f32.negative.nearest_min], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const [y, x] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.atan2Interval(y, x);
    t.expect(
      objectEquals(expected, got),
      `f32.atan2Interval(${y}, ${x}) returned ${got}. Expected ${expected}`
    );
  });

g.test('distanceIntervalScalar_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable  form due to the inherited nature
      // of the errors.
      //
      // distance(x, y), where x - y = 0 has an acceptance interval of kAnyBounds,
      // because distance(x, y) = length(x - y), and length(0) = kAnyBounds
      { input: [0, 0], expected: kAnyBounds },
      { input: [1.0, 0], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [0.0, 1.0], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [1.0, 1.0], expected: kAnyBounds },
      { input: [-0.0, -1.0], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [0.0, -1.0], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [-1.0, -1.0], expected: kAnyBounds },
      { input: [0.1, 0], expected: [hexToF64(0x3fb9_9998_9000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      { input: [0, 0.1], expected: [hexToF64(0x3fb9_9998_9000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      { input: [-0.1, 0], expected: [hexToF64(0x3fb9_9998_9000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      { input: [0, -0.1], expected: [hexToF64(0x3fb9_9998_9000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      { input: [10.0, 0], expected: [hexToF64(0x4023_ffff_7000_0000n), hexToF64(0x4024_0000_b000_0000n)] },  // ~10
      { input: [0, 10.0], expected: [hexToF64(0x4023_ffff_7000_0000n), hexToF64(0x4024_0000_b000_0000n)] },  // ~10
      { input: [-10.0, 0], expected: [hexToF64(0x4023_ffff_7000_0000n), hexToF64(0x4024_0000_b000_0000n)] },  // ~10
      { input: [0, -10.0], expected: [hexToF64(0x4023_ffff_7000_0000n), hexToF64(0x4024_0000_b000_0000n)] },  // ~10

      // Subnormal Cases
      { input: [kValue.f32.subnormal.negative.min, 0], expected: kAnyBounds },
      { input: [kValue.f32.subnormal.negative.max, 0], expected: kAnyBounds },
      { input: [kValue.f32.subnormal.positive.min, 0], expected: kAnyBounds },
      { input: [kValue.f32.subnormal.positive.max, 0], expected: kAnyBounds },

      // Edge cases
      { input: [kValue.f32.infinity.positive, 0], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, 0], expected: kAnyBounds },
      { input: [kValue.f32.negative.min, 0], expected: kAnyBounds },
      { input: [kValue.f32.negative.max, 0], expected: kAnyBounds },
      { input: [kValue.f32.positive.min, 0], expected: kAnyBounds },
      { input: [kValue.f32.positive.max, 0], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.distanceInterval(...t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.distanceInterval(${t.params.input[0]}, ${t.params.input[1]}) returned ${got}. Expected ${expected}`
    );
  });

g.test('divisionInterval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 1], expected: 0 },
      { input: [0, -1], expected: 0 },
      { input: [1, 1], expected: 1 },
      { input: [1, -1], expected: -1 },
      { input: [-1, 1], expected: -1 },
      { input: [-1, -1], expected: 1 },
      { input: [4, 2], expected: 2 },
      { input: [-4, 2], expected: -2 },
      { input: [4, -2], expected: -2 },
      { input: [-4, -2], expected: 2 },

      // 64-bit normals
      { input: [0, 0.1], expected: 0 },
      { input: [0, -0.1], expected: 0 },
      { input: [1, 0.1], expected: [minusOneULPF32(10), plusOneULPF32(10)] },
      { input: [-1, 0.1], expected: [minusOneULPF32(-10), plusOneULPF32(-10)] },
      { input: [1, -0.1], expected: [minusOneULPF32(-10), plusOneULPF32(-10)] },
      { input: [-1, -0.1], expected: [minusOneULPF32(10), plusOneULPF32(10)] },

      // Denominator out of range
      { input: [1, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [1, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [1, kValue.f32.positive.max], expected: kAnyBounds },
      { input: [1, kValue.f32.negative.min], expected: kAnyBounds },
      { input: [1, 0], expected: kAnyBounds },
      { input: [1, kValue.f32.subnormal.positive.max], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const error = (n: number): number => {
      return 2.5 * oneULPF32(n);
    };

    const [x, y] = t.params.input;
    t.params.expected = applyError(t.params.expected, error);
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.divisionInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.divisionInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('ldexpInterval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: 0 },
      { input: [0, 1], expected: 0 },
      { input: [0, -1], expected: 0 },
      { input: [1, 1], expected: 2 },
      { input: [1, -1], expected: 0.5 },
      { input: [-1, 1], expected: -2 },
      { input: [-1, -1], expected: -0.5 },

      // 64-bit normals
      { input: [0, 0.1], expected: 0 },
      { input: [0, -0.1], expected: 0 },
      { input: [1.0000000001, 1], expected: [2, plusNULPF32(2, 2)] },  // ~2, additional ULP error due to first param not being f32 precise
      { input: [-1.0000000001, 1], expected: [minusNULPF32(-2, 2), -2] },  // ~-2, additional ULP error due to first param not being f32 precise

      // Edge Cases
      { input: [1.9999998807907104, 127], expected: kValue.f32.positive.max },
      { input: [1, -126], expected: kValue.f32.positive.min },
      { input: [0.9999998807907104, -126], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [1.1920928955078125e-07, -126], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [-1.1920928955078125e-07, -126], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [-0.9999998807907104, -126], expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: [-1, -126], expected: kValue.f32.negative.max },
      { input: [-1.9999998807907104, 127], expected: kValue.f32.negative.min },

      // Out of Bounds
      { input: [1, 128], expected: kAnyBounds },
      { input: [-1, 128], expected: kAnyBounds },
      { input: [100, 126], expected: kAnyBounds },
      { input: [-100, 126], expected: kAnyBounds },
      { input: [kValue.f32.positive.max, kValue.i32.positive.max], expected: kAnyBounds },
      { input: [kValue.f32.negative.min, kValue.i32.positive.max], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.ldexpInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.ldexpInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('maxInterval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: 0 },
      { input: [1, 0], expected: 1 },
      { input: [0, 1], expected: 1 },
      { input: [-1, 0], expected: 0 },
      { input: [0, -1], expected: 0 },
      { input: [1, 1], expected: 1 },
      { input: [1, -1], expected: 1 },
      { input: [-1, 1], expected: 1 },
      { input: [-1, -1], expected: -1 },

      // 64-bit normals
      { input: [0.1, 0], expected: [minusOneULPF32(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0, 0.1], expected: [minusOneULPF32(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [-0.1, 0], expected: 0 },
      { input: [0, -0.1], expected: 0 },
      { input: [0.1, 0.1], expected: [minusOneULPF32(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0.1, -0.1], expected: [minusOneULPF32(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [-0.1, 0.1], expected: [minusOneULPF32(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [-0.1, -0.1], expected: [hexToF32(0xbdcccccd), plusOneULPF32(hexToF32(0xbdcccccd))] },  // ~-0.1

      // 32-bit subnormals
      { input: [kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.min, 0], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, kValue.f32.subnormal.positive.min], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [kValue.f32.subnormal.negative.max, 0], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [0, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [kValue.f32.subnormal.negative.min, 0], expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: [0, kValue.f32.subnormal.negative.min], expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: [1, kValue.f32.subnormal.positive.max], expected: 1 },
      { input: [kValue.f32.subnormal.negative.min, kValue.f32.subnormal.positive.max], expected: [kValue.f32.subnormal.negative.min, kValue.f32.subnormal.positive.max] },

      // Infinities
      { input: [0, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, 0], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [0, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, 0], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.maxInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.maxInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('minInterval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: 0 },
      { input: [1, 0], expected: 0 },
      { input: [0, 1], expected: 0 },
      { input: [-1, 0], expected: -1 },
      { input: [0, -1], expected: -1 },
      { input: [1, 1], expected: 1 },
      { input: [1, -1], expected: -1 },
      { input: [-1, 1], expected: -1 },
      { input: [-1, -1], expected: -1 },

      // 64-bit normals
      { input: [0.1, 0], expected: 0 },
      { input: [0, 0.1], expected: 0 },
      { input: [-0.1, 0], expected: [hexToF32(0xbdcccccd), plusOneULPF32(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [0, -0.1], expected: [hexToF32(0xbdcccccd), plusOneULPF32(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [0.1, 0.1], expected: [minusOneULPF32(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0.1, -0.1], expected: [hexToF32(0xbdcccccd), plusOneULPF32(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [-0.1, 0.1], expected: [hexToF32(0xbdcccccd), plusOneULPF32(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [-0.1, -0.1], expected: [hexToF32(0xbdcccccd), plusOneULPF32(hexToF32(0xbdcccccd))] },  // ~-0.1

      // 32-bit subnormals
      { input: [kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.min, 0], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, kValue.f32.subnormal.positive.min], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [kValue.f32.subnormal.negative.max, 0], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [0, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [kValue.f32.subnormal.negative.min, 0], expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: [0, kValue.f32.subnormal.negative.min], expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: [-1, kValue.f32.subnormal.positive.max], expected: -1 },
      { input: [kValue.f32.subnormal.negative.min, kValue.f32.subnormal.positive.max], expected: [kValue.f32.subnormal.negative.min, kValue.f32.subnormal.positive.max] },

      // Infinities
      { input: [0, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, 0], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [0, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, 0], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.minInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.minInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('multiplicationInterval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: 0 },
      { input: [1, 0], expected: 0 },
      { input: [0, 1], expected: 0 },
      { input: [-1, 0], expected: 0 },
      { input: [0, -1], expected: 0 },
      { input: [1, 1], expected: 1 },
      { input: [1, -1], expected: -1 },
      { input: [-1, 1], expected: -1 },
      { input: [-1, -1], expected: 1 },
      { input: [2, 1], expected: 2 },
      { input: [1, -2], expected: -2 },
      { input: [-2, 1], expected: -2 },
      { input: [-2, -1], expected: 2 },
      { input: [2, 2], expected: 4 },
      { input: [2, -2], expected: -4 },
      { input: [-2, 2], expected: -4 },
      { input: [-2, -2], expected: 4 },

      // 64-bit normals
      { input: [0.1, 0], expected: 0 },
      { input: [0, 0.1], expected: 0 },
      { input: [-0.1, 0], expected: 0 },
      { input: [0, -0.1], expected: 0 },
      { input: [0.1, 0.1], expected: [minusNULPF32(hexToF32(0x3c23d70a), 2), plusOneULPF32(hexToF32(0x3c23d70a))] },  // ~0.01
      { input: [0.1, -0.1], expected: [minusOneULPF32(hexToF32(0xbc23d70a)), plusNULPF32(hexToF32(0xbc23d70a), 2)] },  // ~-0.01
      { input: [-0.1, 0.1], expected: [minusOneULPF32(hexToF32(0xbc23d70a)), plusNULPF32(hexToF32(0xbc23d70a), 2)] },  // ~-0.01
      { input: [-0.1, -0.1], expected: [minusNULPF32(hexToF32(0x3c23d70a), 2), plusOneULPF32(hexToF32(0x3c23d70a))] },  // ~0.01

      // Infinities
      { input: [0, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [1, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [-1, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [0, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [1, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [-1, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAnyBounds },

      // Edge of f32
      { input: [kValue.f32.positive.max, kValue.f32.positive.max], expected: kAnyBounds },
      { input: [kValue.f32.negative.min, kValue.f32.negative.min], expected: kAnyBounds },
      { input: [kValue.f32.positive.max, kValue.f32.negative.min], expected: kAnyBounds },
      { input: [kValue.f32.negative.min, kValue.f32.positive.max], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.multiplicationInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.multiplicationInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('powInterval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      { input: [-1, 0], expected: kAnyBounds },
      { input: [0, 0], expected: kAnyBounds },
      { input: [1, 0], expected: [minusNULPF32(1, 3), hexToF64(0x3ff0_0000_3000_0000n)] },  // ~1
      { input: [2, 0], expected: [minusNULPF32(1, 3), hexToF64(0x3ff0_0000_3000_0000n)] },  // ~1
      { input: [kValue.f32.positive.max, 0], expected: [minusNULPF32(1, 3), hexToF64(0x3ff0_0000_3000_0000n)] },  // ~1
      { input: [0, 1], expected: kAnyBounds },
      { input: [1, 1], expected: [hexToF64(0x3fef_fffe_dfff_fe00n), hexToF64(0x3ff0_0000_c000_0200n)] },  // ~1
      { input: [1, 100], expected: [hexToF64(0x3fef_ffba_3fff_3800n), hexToF64(0x3ff0_0023_2000_c800n)] },  // ~1
      { input: [1, kValue.f32.positive.max], expected: kAnyBounds },
      { input: [2, 1], expected: [hexToF64(0x3fff_fffe_a000_0200n), hexToF64(0x4000_0001_0000_0200n)] },  // ~2
      { input: [2, 2], expected: [hexToF64(0x400f_fffd_a000_0400n), hexToF64(0x4010_0001_a000_0400n)] },  // ~4
      { input: [10, 10], expected: [hexToF64(0x4202_a04f_51f7_7000n), hexToF64(0x4202_a070_ee08_e000n)] },  // ~10000000000
      { input: [10, 1], expected: [hexToF64(0x4023_fffe_0b65_8b00n), hexToF64(0x4024_0002_149a_7c00n)] },  // ~10
      { input: [kValue.f32.positive.max, 1], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.powInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.powInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('remainderInterval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 1], expected: [0, 0] },
      { input: [0, -1], expected: [0, 0] },
      { input: [1, 1], expected: [0, 1] },
      { input: [1, -1], expected: [0, 1] },
      { input: [-1, 1], expected: [-1, 0] },
      { input: [-1, -1], expected: [-1, 0] },
      { input: [4, 2], expected: [0, 2] },
      { input: [-4, 2], expected: [-2, 0] },
      { input: [4, -2], expected: [0, 2] },
      { input: [-4, -2], expected: [-2, 0] },
      { input: [2, 4], expected: [2, 2] },
      { input: [-2, 4], expected: [-2, -2] },
      { input: [2, -4], expected: [2, 2] },
      { input: [-2, -4], expected: [-2, -2] },

      // 64-bit normals
      { input: [0, 0.1], expected: [0, 0] },
      { input: [0, -0.1], expected: [0, 0] },
      { input: [1, 0.1], expected: [hexToF32(0xb4000000), hexToF32(0x3dccccd8)] }, // ~[0, 0.1]
      { input: [-1, 0.1], expected: [hexToF32(0xbdccccd8), hexToF32(0x34000000)] }, // ~[-0.1, 0]
      { input: [1, -0.1], expected: [hexToF32(0xb4000000), hexToF32(0x3dccccd8)] }, // ~[0, 0.1]
      { input: [-1, -0.1], expected: [hexToF32(0xbdccccd8), hexToF32(0x34000000)] }, // ~[-0.1, 0]

      // Denominator out of range
      { input: [1, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [1, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [1, kValue.f32.positive.max], expected: kAnyBounds },
      { input: [1, kValue.f32.negative.min], expected: kAnyBounds },
      { input: [1, 0], expected: kAnyBounds },
      { input: [1, kValue.f32.subnormal.positive.max], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.remainderInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.remainderInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('stepInterval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: 1 },
      { input: [1, 1], expected: 1 },
      { input: [0, 1], expected: 1 },
      { input: [1, 0], expected: 0 },
      { input: [-1, -1], expected: 1 },
      { input: [0, -1], expected: 0 },
      { input: [-1, 0], expected: 1 },
      { input: [-1, 1], expected: 1 },
      { input: [1, -1], expected: 0 },

      // 64-bit normals
      { input: [0.1, 0.1], expected: [0, 1] },
      { input: [0, 0.1], expected: 1 },
      { input: [0.1, 0], expected: 0 },
      { input: [0.1, 1], expected: 1 },
      { input: [1, 0.1], expected: 0 },
      { input: [-0.1, -0.1], expected: [0, 1] },
      { input: [0, -0.1], expected: 0 },
      { input: [-0.1, 0], expected: 1 },
      { input: [-0.1, -1], expected: 0 },
      { input: [-1, -0.1], expected: 1 },

      // Subnormals
      { input: [0, kValue.f32.subnormal.positive.max], expected: 1 },
      { input: [0, kValue.f32.subnormal.positive.min], expected: 1 },
      { input: [0, kValue.f32.subnormal.negative.max], expected: [0, 1] },
      { input: [0, kValue.f32.subnormal.negative.min], expected: [0, 1] },
      { input: [1, kValue.f32.subnormal.positive.max], expected: 0 },
      { input: [1, kValue.f32.subnormal.positive.min], expected: 0 },
      { input: [1, kValue.f32.subnormal.negative.max], expected: 0 },
      { input: [1, kValue.f32.subnormal.negative.min], expected: 0 },
      { input: [-1, kValue.f32.subnormal.positive.max], expected: 1 },
      { input: [-1, kValue.f32.subnormal.positive.min], expected: 1 },
      { input: [-1, kValue.f32.subnormal.negative.max], expected: 1 },
      { input: [-1, kValue.f32.subnormal.negative.min], expected: 1 },
      { input: [kValue.f32.subnormal.positive.max, 0], expected: [0, 1] },
      { input: [kValue.f32.subnormal.positive.min, 0], expected: [0, 1] },
      { input: [kValue.f32.subnormal.negative.max, 0], expected: 1 },
      { input: [kValue.f32.subnormal.negative.min, 0], expected: 1 },
      { input: [kValue.f32.subnormal.positive.max, 1], expected: 1 },
      { input: [kValue.f32.subnormal.positive.min, 1], expected: 1 },
      { input: [kValue.f32.subnormal.negative.max, 1], expected: 1 },
      { input: [kValue.f32.subnormal.negative.min, 1], expected: 1 },
      { input: [kValue.f32.subnormal.positive.max, -1], expected: 0 },
      { input: [kValue.f32.subnormal.positive.min, -1], expected: 0 },
      { input: [kValue.f32.subnormal.negative.max, -1], expected: 0 },
      { input: [kValue.f32.subnormal.negative.min, -1], expected: 0 },
      { input: [kValue.f32.subnormal.negative.min, kValue.f32.subnormal.positive.max], expected: 1 },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.min], expected: [0, 1] },

      // Infinities
      { input: [0, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, 0], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [0, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, 0], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const [edge, x] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.stepInterval(edge, x);
    t.expect(
      objectEquals(expected, got),
      `f32.stepInterval(${edge}, ${x}) returned ${got}. Expected ${expected}`
    );
  });

g.test('subtractionInterval_f32')
  .paramsSubcasesOnly<ScalarPairToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: 0 },
      { input: [1, 0], expected: 1 },
      { input: [0, 1], expected: -1 },
      { input: [-1, 0], expected: -1 },
      { input: [0, -1], expected: 1 },
      { input: [1, 1], expected: 0 },
      { input: [1, -1], expected: 2 },
      { input: [-1, 1], expected: -2 },
      { input: [-1, -1], expected: 0 },

      // 64-bit normals
      { input: [0.1, 0], expected: [minusOneULPF32(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0, 0.1], expected: [hexToF32(0xbdcccccd), plusOneULPF32(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [-0.1, 0], expected: [hexToF32(0xbdcccccd), plusOneULPF32(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [0, -0.1], expected: [minusOneULPF32(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0.1, 0.1], expected: [minusOneULPF32(hexToF32(0x3dcccccd)) - hexToF32(0x3dcccccd), hexToF32(0x3dcccccd) - minusOneULPF32(hexToF32(0x3dcccccd))] },  // ~0.0
      { input: [0.1, -0.1], expected: [minusOneULPF32(hexToF32(0x3e4ccccd)), hexToF32(0x3e4ccccd)] }, // ~0.2
      { input: [-0.1, 0.1], expected: [hexToF32(0xbe4ccccd), plusOneULPF32(hexToF32(0xbe4ccccd))] },  // ~-0.2
      { input: [-0.1, -0.1], expected: [minusOneULPF32(hexToF32(0x3dcccccd)) - hexToF32(0x3dcccccd), hexToF32(0x3dcccccd) - minusOneULPF32(hexToF32(0x3dcccccd))] }, // ~0

      // // 32-bit normals
      { input: [kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, kValue.f32.subnormal.positive.max], expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: [kValue.f32.subnormal.positive.min, 0], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, kValue.f32.subnormal.positive.min], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [kValue.f32.subnormal.negative.max, 0], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [0, kValue.f32.subnormal.negative.max], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [kValue.f32.subnormal.negative.min, 0], expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: [0, kValue.f32.subnormal.negative.min], expected: [0, kValue.f32.subnormal.positive.max] },

      // Infinities
      { input: [0, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, 0], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [0, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, 0], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.subtractionInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.subtractionInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

interface ScalarTripleToIntervalCase {
  input: [number, number, number];
  expected: number | IntervalBounds;
}

g.test('clampMedianInterval_f32')
  .paramsSubcasesOnly<ScalarTripleToIntervalCase>(
    // prettier-ignore
    [
      // Normals
      { input: [0, 0, 0], expected: 0 },
      { input: [1, 0, 0], expected: 0 },
      { input: [0, 1, 0], expected: 0 },
      { input: [0, 0, 1], expected: 0 },
      { input: [1, 0, 1], expected: 1 },
      { input: [1, 1, 0], expected: 1 },
      { input: [0, 1, 1], expected: 1 },
      { input: [1, 1, 1], expected: 1 },
      { input: [1, 10, 100], expected: 10 },
      { input: [10, 1, 100], expected: 10 },
      { input: [100, 1, 10], expected: 10 },
      { input: [-10, 1, 100], expected: 1 },
      { input: [10, 1, -100], expected: 1 },
      { input: [-10, 1, -100], expected: -10 },
      { input: [-10, -10, -10], expected: -10 },

      // Subnormals
      { input: [kValue.f32.subnormal.positive.max, 0, 0], expected: 0 },
      { input: [0, kValue.f32.subnormal.positive.max, 0], expected: 0 },
      { input: [0, 0, kValue.f32.subnormal.positive.max], expected: 0 },
      { input: [kValue.f32.subnormal.positive.max, 0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.negative.max], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [kValue.f32.positive.max, kValue.f32.positive.max, kValue.f32.subnormal.positive.min], expected: kValue.f32.positive.max },

      // Infinities
      { input: [0, 1, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [0, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.clampMedianInterval(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `f32.clampMedianInterval(${x}, ${y}, ${z}) returned ${got}. Expected ${expected}`
    );
  });

g.test('clampMinMaxInterval_f32')
  .paramsSubcasesOnly<ScalarTripleToIntervalCase>(
    // prettier-ignore
    [
      // Normals
      { input: [0, 0, 0], expected: 0 },
      { input: [1, 0, 0], expected: 0 },
      { input: [0, 1, 0], expected: 0 },
      { input: [0, 0, 1], expected: 0 },
      { input: [1, 0, 1], expected: 1 },
      { input: [1, 1, 0], expected: 0 },
      { input: [0, 1, 1], expected: 1 },
      { input: [1, 1, 1], expected: 1 },
      { input: [1, 10, 100], expected: 10 },
      { input: [10, 1, 100], expected: 10 },
      { input: [100, 1, 10], expected: 10 },
      { input: [-10, 1, 100], expected: 1 },
      { input: [10, 1, -100], expected: -100 },
      { input: [-10, 1, -100], expected: -100 },
      { input: [-10, -10, -10], expected: -10 },

      // Subnormals
      { input: [kValue.f32.subnormal.positive.max, 0, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, 0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, 0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.min, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.positive.max, kValue.f32.positive.max, kValue.f32.subnormal.positive.min], expected: [0, kValue.f32.subnormal.positive.min] },

      // Infinities
      { input: [0, 1, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [0, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.clampMinMaxInterval(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `f32.clampMinMaxInterval(${x}, ${y}, ${z}) returned ${got}. Expected ${expected}`
    );
  });

g.test('fmaInterval_f32')
  .paramsSubcasesOnly<ScalarTripleToIntervalCase>(
    // prettier-ignore
    [
      // Normals
      { input: [0, 0, 0], expected: 0 },
      { input: [1, 0, 0], expected: 0 },
      { input: [0, 1, 0], expected: 0 },
      { input: [0, 0, 1], expected: 1 },
      { input: [1, 0, 1], expected: 1 },
      { input: [1, 1, 0], expected: 1 },
      { input: [0, 1, 1], expected: 1 },
      { input: [1, 1, 1], expected: 2 },
      { input: [1, 10, 100], expected: 110 },
      { input: [10, 1, 100], expected: 110 },
      { input: [100, 1, 10], expected: 110 },
      { input: [-10, 1, 100], expected: 90 },
      { input: [10, 1, -100], expected: -90 },
      { input: [-10, 1, -100], expected: -110 },
      { input: [-10, -10, -10], expected: 90 },

      // Subnormals
      { input: [kValue.f32.subnormal.positive.max, 0, 0], expected: 0 },
      { input: [0, kValue.f32.subnormal.positive.max, 0], expected: 0 },
      { input: [0, 0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, 0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.positive.min] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, kValue.f32.subnormal.positive.min] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max], expected: [hexToF32(0x80000002), 0] },

      // Infinities
      { input: [0, 1, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [0, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [kValue.f32.positive.max, kValue.f32.positive.max, kValue.f32.subnormal.positive.min], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.fmaInterval(...t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.fmaInterval(${t.params.input.join(',')}) returned ${got}. Expected ${expected}`
    );
  });

g.test('mixImpreciseInterval_f32')
  .paramsSubcasesOnly<ScalarTripleToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.

      // [0.0, 1.0] cases
      { input: [0.0, 1.0, -1.0], expected: -1.0 },
      { input: [0.0, 1.0, 0.0], expected: 0.0 },
      { input: [0.0, 1.0, 0.1], expected: [hexToF64(0x3fb9_9999_8000_0000n), hexToF64(0x3fb9_9999_a000_0000n)] },  // ~0.1
      { input: [0.0, 1.0, 0.5], expected: 0.5 },
      { input: [0.0, 1.0, 0.9], expected: [hexToF64(0x3fec_cccc_c000_0000n), hexToF64(0x3fec_cccc_e000_0000n)] },  // ~0.9
      { input: [0.0, 1.0, 1.0], expected: 1.0 },
      { input: [0.0, 1.0, 2.0], expected: 2.0 },

      // [1.0, 0.0] cases
      { input: [1.0, 0.0, -1.0], expected: 2.0 },
      { input: [1.0, 0.0, 0.0], expected: 1.0 },
      { input: [1.0, 0.0, 0.1], expected: [hexToF64(0x3fec_cccc_c000_0000n), hexToF64(0x3fec_cccc_e000_0000n)] },  // ~0.9
      { input: [1.0, 0.0, 0.5], expected: 0.5 },
      { input: [1.0, 0.0, 0.9], expected: [hexToF64(0x3fb9_9999_0000_0000n), hexToF64(0x3fb9_999a_0000_0000n)] },  // ~0.1
      { input: [1.0, 0.0, 1.0], expected: 0.0 },
      { input: [1.0, 0.0, 2.0], expected: -1.0 },

      // [0.0, 10.0] cases
      { input: [0.0, 10.0, -1.0], expected: -10.0 },
      { input: [0.0, 10.0, 0.0], expected: 0.0 },
      { input: [0.0, 10.0, 0.1], expected: [hexToF64(0x3fef_ffff_e000_0000n), hexToF64(0x3ff0_0000_2000_0000n)] },  // ~1
      { input: [0.0, 10.0, 0.5], expected: 5.0 },
      { input: [0.0, 10.0, 0.9], expected: [hexToF64(0x4021_ffff_e000_0000n), hexToF64(0x4022_0000_2000_0000n)] },  // ~9
      { input: [0.0, 10.0, 1.0], expected: 10.0 },
      { input: [0.0, 10.0, 2.0], expected: 20.0 },

      // [2.0, 10.0] cases
      { input: [2.0, 10.0, -1.0], expected: -6.0 },
      { input: [2.0, 10.0, 0.0], expected: 2.0 },
      { input: [2.0, 10.0, 0.1], expected: [hexToF64(0x4006_6666_6000_0000n), hexToF64(0x4006_6666_8000_0000n)] },  // ~2.8
      { input: [2.0, 10.0, 0.5], expected: 6.0 },
      { input: [2.0, 10.0, 0.9], expected: [hexToF64(0x4022_6666_6000_0000n), hexToF64(0x4022_6666_8000_0000n)] },  // ~9.2
      { input: [2.0, 10.0, 1.0], expected: 10.0 },
      { input: [2.0, 10.0, 2.0], expected: 18.0 },

      // [-1.0, 1.0] cases
      { input: [-1.0, 1.0, -2.0], expected: -5.0 },
      { input: [-1.0, 1.0, 0.0], expected: -1.0 },
      { input: [-1.0, 1.0, 0.1], expected: [hexToF64(0xbfe9_9999_a000_0000n), hexToF64(0xbfe9_9999_8000_0000n)] },  // ~-0.8
      { input: [-1.0, 1.0, 0.5], expected: 0.0 },
      { input: [-1.0, 1.0, 0.9], expected: [hexToF64(0x3fe9_9999_8000_0000n), hexToF64(0x3fe9_9999_c000_0000n)] },  // ~0.8
      { input: [-1.0, 1.0, 1.0], expected: 1.0 },
      { input: [-1.0, 1.0, 2.0], expected: 3.0 },

      // Infinities
      { input: [0.0, kValue.f32.infinity.positive, 0.5], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, 0.0, 0.5], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, 1.0, 0.5], expected: kAnyBounds },
      { input: [1.0, kValue.f32.infinity.negative, 0.5], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, 0.5], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative, 0.5], expected: kAnyBounds },
      { input: [0.0, 1.0, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [1.0, 0.0, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [0.0, 1.0, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [1.0, 0.0, kValue.f32.infinity.positive], expected: kAnyBounds },

      // Showing how precise and imprecise versions diff
      { input: [kValue.f32.negative.min, 10.0, 1.0], expected: 0.0 },
    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.mixImpreciseInterval(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `f32.mixImpreciseInterval(${x}, ${y}, ${z}) returned ${got}. Expected ${expected}`
    );
  });

g.test('mixPreciseInterval_f32')
  .paramsSubcasesOnly<ScalarTripleToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.

      // [0.0, 1.0] cases
      { input: [0.0, 1.0, -1.0], expected: -1.0 },
      { input: [0.0, 1.0, 0.0], expected: 0.0 },
      { input: [0.0, 1.0, 0.1], expected: [hexToF64(0x3fb9_9999_8000_0000n), hexToF64(0x3fb9_9999_a000_0000n)] },  // ~0.1
      { input: [0.0, 1.0, 0.5], expected: 0.5 },
      { input: [0.0, 1.0, 0.9], expected: [hexToF64(0x3fec_cccc_c000_0000n), hexToF64(0x3fec_cccc_e000_0000n)] },  // ~0.9
      { input: [0.0, 1.0, 1.0], expected: 1.0 },
      { input: [0.0, 1.0, 2.0], expected: 2.0 },

      // [1.0, 0.0] cases
      { input: [1.0, 0.0, -1.0], expected: 2.0 },
      { input: [1.0, 0.0, 0.0], expected: 1.0 },
      { input: [1.0, 0.0, 0.1], expected: [hexToF64(0x3fec_cccc_c000_0000n), hexToF64(0x3fec_cccc_e000_0000n)] },  // ~0.9
      { input: [1.0, 0.0, 0.5], expected: 0.5 },
      { input: [1.0, 0.0, 0.9], expected: [hexToF64(0x3fb9_9999_0000_0000n), hexToF64(0x3fb9_999a_0000_0000n)] },  // ~0.1
      { input: [1.0, 0.0, 1.0], expected: 0.0 },
      { input: [1.0, 0.0, 2.0], expected: -1.0 },

      // [0.0, 10.0] cases
      { input: [0.0, 10.0, -1.0], expected: -10.0 },
      { input: [0.0, 10.0, 0.0], expected: 0.0 },
      { input: [0.0, 10.0, 0.1], expected: [hexToF64(0x3fef_ffff_e000_0000n), hexToF64(0x3ff0_0000_2000_0000n)] },  // ~1
      { input: [0.0, 10.0, 0.5], expected: 5.0 },
      { input: [0.0, 10.0, 0.9], expected: [hexToF64(0x4021_ffff_e000_0000n), hexToF64(0x4022_0000_2000_0000n)] },  // ~9
      { input: [0.0, 10.0, 1.0], expected: 10.0 },
      { input: [0.0, 10.0, 2.0], expected: 20.0 },

      // [2.0, 10.0] cases
      { input: [2.0, 10.0, -1.0], expected: -6.0 },
      { input: [2.0, 10.0, 0.0], expected: 2.0 },
      { input: [2.0, 10.0, 0.1], expected: [hexToF64(0x4006_6666_4000_0000n), hexToF64(0x4006_6666_8000_0000n)] },  // ~2.8
      { input: [2.0, 10.0, 0.5], expected: 6.0 },
      { input: [2.0, 10.0, 0.9], expected: [hexToF64(0x4022_6666_4000_0000n), hexToF64(0x4022_6666_a000_0000n)] },  // ~9.2
      { input: [2.0, 10.0, 1.0], expected: 10.0 },
      { input: [2.0, 10.0, 2.0], expected: 18.0 },

      // [-1.0, 1.0] cases
      { input: [-1.0, 1.0, -2.0], expected: -5.0 },
      { input: [-1.0, 1.0, 0.0], expected: -1.0 },
      { input: [-1.0, 1.0, 0.1], expected: [hexToF64(0xbfe9_9999_c000_0000n), hexToF64(0xbfe9_9999_8000_0000n)] },  // ~-0.8
      { input: [-1.0, 1.0, 0.5], expected: 0.0 },
      { input: [-1.0, 1.0, 0.9], expected: [hexToF64(0x3fe9_9999_8000_0000n), hexToF64(0x3fe9_9999_c000_0000n)] },  // ~0.8
      { input: [-1.0, 1.0, 1.0], expected: 1.0 },
      { input: [-1.0, 1.0, 2.0], expected: 3.0 },

      // Infinities
      { input: [0.0, kValue.f32.infinity.positive, 0.5], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, 0.0, 0.5], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, 1.0, 0.5], expected: kAnyBounds },
      { input: [1.0, kValue.f32.infinity.negative, 0.5], expected: kAnyBounds },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, 0.5], expected: kAnyBounds },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative, 0.5], expected: kAnyBounds },
      { input: [0.0, 1.0, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [1.0, 0.0, kValue.f32.infinity.negative], expected: kAnyBounds },
      { input: [0.0, 1.0, kValue.f32.infinity.positive], expected: kAnyBounds },
      { input: [1.0, 0.0, kValue.f32.infinity.positive], expected: kAnyBounds },

      // Showing how precise and imprecise versions diff
      { input: [kValue.f32.negative.min, 10.0, 1.0], expected: 10.0 },
    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.mixPreciseInterval(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `f32.mixPreciseInterval(${x}, ${y}, ${z}) returned ${got}. Expected ${expected}`
    );
  });

g.test('smoothStepInterval_f32')
  .paramsSubcasesOnly<ScalarTripleToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.

      // Normals
      { input: [0, 1, 0], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, 1, 1], expected: [hexToF32(0x3f7ffffa), hexToF32(0x3f800003)] },  // ~1
      { input: [0, 1, 10], expected: 1 },
      { input: [0, 1, -10], expected: 0 },
      { input: [0, 2, 1], expected: [hexToF32(0x3efffff8), hexToF32(0x3f000007)] },  // ~0.5
      { input: [0, 2, 0.5], expected: [hexToF32(0x3e1ffffb), hexToF32(0x3e200007)] },  // ~0.15625...
      { input: [2, 0, 1], expected: [hexToF32(0x3efffff8), hexToF32(0x3f000007)] },  // ~0.5
      { input: [2, 0, 1.5], expected: [hexToF32(0x3e1ffffb), hexToF32(0x3e200007)] },  // ~0.15625...
      { input: [0, 100, 50], expected: [hexToF32(0x3efffff8), hexToF32(0x3f000007)] },  // ~0.5
      { input: [0, 100, 25], expected: [hexToF32(0x3e1ffffb), hexToF32(0x3e200007)] },  // ~0.15625...
      { input: [0, -2, -1], expected: [hexToF32(0x3efffff8), hexToF32(0x3f000007)] },  // ~0.5
      { input: [0, -2, -0.5], expected: [hexToF32(0x3e1ffffb), hexToF32(0x3e200007)] },  // ~0.15625...

      // Subnormals
      { input: [0, 2, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, 2, kValue.f32.subnormal.positive.min], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, 2, kValue.f32.subnormal.negative.max], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, 2, kValue.f32.subnormal.negative.min], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [kValue.f32.subnormal.positive.max, 2, 1], expected: [hexToF32(0x3efffff8), hexToF32(0x3f000007)] },  // ~0.5
      { input: [kValue.f32.subnormal.positive.min, 2, 1], expected: [hexToF32(0x3efffff8), hexToF32(0x3f000007)] },  // ~0.5
      { input: [kValue.f32.subnormal.negative.max, 2, 1], expected: [hexToF32(0x3efffff8), hexToF32(0x3f000007)] },  // ~0.5
      { input: [kValue.f32.subnormal.negative.min, 2, 1], expected: [hexToF32(0x3efffff8), hexToF32(0x3f000007)] },  // ~0.5
      { input: [0, kValue.f32.subnormal.positive.max, 1], expected: kAnyBounds },
      { input: [0, kValue.f32.subnormal.positive.min, 1], expected: kAnyBounds },
      { input: [0, kValue.f32.subnormal.negative.max, 1], expected: kAnyBounds },
      { input: [0, kValue.f32.subnormal.negative.min, 1], expected: kAnyBounds },

      // Infinities
      { input: [0, 2, Number.POSITIVE_INFINITY], expected: kAnyBounds },
      { input: [0, 2, Number.NEGATIVE_INFINITY], expected: kAnyBounds },
      { input: [Number.POSITIVE_INFINITY, 2, 1], expected: kAnyBounds },
      { input: [Number.NEGATIVE_INFINITY, 2, 1], expected: kAnyBounds },
      { input: [0, Number.POSITIVE_INFINITY, 1], expected: kAnyBounds },
      { input: [0, Number.NEGATIVE_INFINITY, 1], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const [low, high, x] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.smoothStepInterval(low, high, x);
    t.expect(
      objectEquals(expected, got),
      `f32.smoothStepInterval(${low}, ${high}, ${x}) returned ${got}. Expected ${expected}`
    );
  });

interface ScalarToVectorCase {
  input: number;
  expected: (number | IntervalBounds)[];
}

// Scope for unpack* tests so that they can have constants for magic numbers
// that don't pollute the global namespace or have unwieldy long names.
{
  const kZeroBounds: IntervalBounds = [hexToF32(0x81200000), hexToF32(0x01200000)];
  const kOneBoundsSnorm: IntervalBounds = [
    hexToF64(0x3fef_ffff_a000_0000n),
    hexToF64(0x3ff0_0000_4000_0000n),
  ];
  const kOneBoundsUnorm: IntervalBounds = [
    hexToF64(0x3fef_ffff_b000_0000n),
    hexToF64(0x3ff0_0000_2800_0000n),
  ];
  const kNegOneBoundsSnorm: IntervalBounds = [
    hexToF64(0xbff0_0000_0000_0000n),
    hexToF64(0xbfef_ffff_a000_0000n),
  ];

  const kHalfBounds2x16snorm: IntervalBounds = [
    hexToF64(0x3fe0_001f_a000_0000n),
    hexToF64(0x3fe0_0020_8000_0000n),
  ]; // ~0.5..., due to lack of precision in i16
  const kNegHalfBounds2x16snorm: IntervalBounds = [
    hexToF64(0xbfdf_ffc0_6000_0000n),
    hexToF64(0xbfdf_ffbf_8000_0000n),
  ]; // ~-0.5..., due to lack of precision in i16

  g.test('unpack2x16snormInterval')
    .paramsSubcasesOnly<ScalarToVectorCase>(
      // prettier-ignore
      [
        { input: 0x00000000, expected: [kZeroBounds, kZeroBounds] },
        { input: 0x00007fff, expected: [kOneBoundsSnorm, kZeroBounds] },
        { input: 0x7fff0000, expected: [kZeroBounds, kOneBoundsSnorm] },
        { input: 0x7fff7fff, expected: [kOneBoundsSnorm, kOneBoundsSnorm] },
        { input: 0x80018001, expected: [kNegOneBoundsSnorm, kNegOneBoundsSnorm] },
        { input: 0x40004000, expected: [kHalfBounds2x16snorm, kHalfBounds2x16snorm] },
        { input: 0xc001c001, expected: [kNegHalfBounds2x16snorm, kNegHalfBounds2x16snorm] },
      ]
    )
    .fn(t => {
      const expected = FP.f32.toVector(t.params.expected);
      const got = FP.f32.unpack2x16snormInterval(t.params.input);
      t.expect(
        objectEquals(expected, got),
        `unpack2x16snormInterval(${t.params.input}) returned [${got}]. Expected [${expected}]`
      );
    });

  g.test('unpack2x16floatInterval')
    .paramsSubcasesOnly<ScalarToVectorCase>(
      // prettier-ignore
      [
        // f16 normals
        { input: 0x00000000, expected: [0, 0] },
        { input: 0x80000000, expected: [0, 0] },
        { input: 0x00008000, expected: [0, 0] },
        { input: 0x80008000, expected: [0, 0] },
        { input: 0x00003c00, expected: [1, 0] },
        { input: 0x3c000000, expected: [0, 1] },
        { input: 0x3c003c00, expected: [1, 1] },
        { input: 0xbc00bc00, expected: [-1, -1] },
        { input: 0x49004900, expected: [10, 10] },
        { input: 0xc900c900, expected: [-10, -10] },

        // f16 subnormals
        { input: 0x000003ff, expected: [[0, kValue.f16.subnormal.positive.max], 0] },
        { input: 0x000083ff, expected: [[kValue.f16.subnormal.negative.min, 0], 0] },

        // f16 out of bounds
        { input: 0x7c000000, expected: [kAnyBounds, kAnyBounds] },
        { input: 0xffff0000, expected: [kAnyBounds, kAnyBounds] },
      ]
    )
    .fn(t => {
      const expected = FP.f32.toVector(t.params.expected);
      const got = FP.f32.unpack2x16floatInterval(t.params.input);
      t.expect(
        objectEquals(expected, got),
        `unpack2x16floatInterval(${t.params.input}) returned [${got}]. Expected [${expected}]`
      );
    });

  const kHalfBounds2x16unorm: IntervalBounds = [
    hexToF64(0x3fe0_000f_b000_0000n),
    hexToF64(0x3fe0_0010_7000_0000n),
  ]; // ~0.5..., due to lack of precision in u16

  g.test('unpack2x16unormInterval')
    .paramsSubcasesOnly<ScalarToVectorCase>(
      // prettier-ignore
      [
        { input: 0x00000000, expected: [kZeroBounds, kZeroBounds] },
        { input: 0x0000ffff, expected: [kOneBoundsUnorm, kZeroBounds] },
        { input: 0xffff0000, expected: [kZeroBounds, kOneBoundsUnorm] },
        { input: 0xffffffff, expected: [kOneBoundsUnorm, kOneBoundsUnorm] },
        { input: 0x80008000, expected: [kHalfBounds2x16unorm, kHalfBounds2x16unorm] },
      ]
    )
    .fn(t => {
      const expected = FP.f32.toVector(t.params.expected);
      const got = FP.f32.unpack2x16unormInterval(t.params.input);
      t.expect(
        objectEquals(expected, got),
        `unpack2x16unormInterval(${t.params.input}) returned [${got}]. Expected [${expected}]`
      );
    });

  const kHalfBounds4x8snorm: IntervalBounds = [
    hexToF64(0x3fe0_2040_2000_0000n),
    hexToF64(0x3fe0_2041_0000_0000n),
  ]; // ~0.50196..., due to lack of precision in i8
  const kNegHalfBounds4x8snorm: IntervalBounds = [
    hexToF64(0xbfdf_bf7f_6000_0000n),
    hexToF64(0xbfdf_bf7e_8000_0000n),
  ]; // ~-0.49606..., due to lack of precision in i8

  g.test('unpack4x8snormInterval')
    .paramsSubcasesOnly<ScalarToVectorCase>(
      // prettier-ignore
      [
        { input: 0x00000000, expected: [kZeroBounds, kZeroBounds, kZeroBounds, kZeroBounds] },
        { input: 0x0000007f, expected: [kOneBoundsSnorm, kZeroBounds, kZeroBounds, kZeroBounds] },
        { input: 0x00007f00, expected: [kZeroBounds, kOneBoundsSnorm, kZeroBounds, kZeroBounds] },
        { input: 0x007f0000, expected: [kZeroBounds, kZeroBounds, kOneBoundsSnorm, kZeroBounds] },
        { input: 0x7f000000, expected: [kZeroBounds, kZeroBounds, kZeroBounds, kOneBoundsSnorm] },
        { input: 0x00007f7f, expected: [kOneBoundsSnorm, kOneBoundsSnorm, kZeroBounds, kZeroBounds] },
        { input: 0x7f7f0000, expected: [kZeroBounds, kZeroBounds, kOneBoundsSnorm, kOneBoundsSnorm] },
        { input: 0x7f007f00, expected: [kZeroBounds, kOneBoundsSnorm, kZeroBounds, kOneBoundsSnorm] },
        { input: 0x007f007f, expected: [kOneBoundsSnorm, kZeroBounds, kOneBoundsSnorm, kZeroBounds] },
        { input: 0x7f7f7f7f, expected: [kOneBoundsSnorm, kOneBoundsSnorm, kOneBoundsSnorm, kOneBoundsSnorm] },
        { input: 0x81818181, expected: [kNegOneBoundsSnorm, kNegOneBoundsSnorm, kNegOneBoundsSnorm, kNegOneBoundsSnorm] },
        { input: 0x40404040, expected: [kHalfBounds4x8snorm, kHalfBounds4x8snorm, kHalfBounds4x8snorm, kHalfBounds4x8snorm] },
        { input: 0xc1c1c1c1, expected: [kNegHalfBounds4x8snorm, kNegHalfBounds4x8snorm, kNegHalfBounds4x8snorm, kNegHalfBounds4x8snorm] },
      ]
    )
    .fn(t => {
      const expected = FP.f32.toVector(t.params.expected);
      const got = FP.f32.unpack4x8snormInterval(t.params.input);
      t.expect(
        objectEquals(expected, got),
        `unpack4x8snormInterval(${t.params.input}) returned [${got}]. Expected [${expected}]`
      );
    });

  const kHalfBounds4x8unorm: IntervalBounds = [
    hexToF64(0x3fe0_100f_b000_0000n),
    hexToF64(0x3fe0_1010_7000_0000n),
  ]; // ~0.50196..., due to lack of precision in u8

  g.test('unpack4x8unormInterval')
    .paramsSubcasesOnly<ScalarToVectorCase>(
      // prettier-ignore
      [
        { input: 0x00000000, expected: [kZeroBounds, kZeroBounds, kZeroBounds, kZeroBounds] },
        { input: 0x000000ff, expected: [kOneBoundsUnorm, kZeroBounds, kZeroBounds, kZeroBounds] },
        { input: 0x0000ff00, expected: [kZeroBounds, kOneBoundsUnorm, kZeroBounds, kZeroBounds] },
        { input: 0x00ff0000, expected: [kZeroBounds, kZeroBounds, kOneBoundsUnorm, kZeroBounds] },
        { input: 0xff000000, expected: [kZeroBounds, kZeroBounds, kZeroBounds, kOneBoundsUnorm] },
        { input: 0x0000ffff, expected: [kOneBoundsUnorm, kOneBoundsUnorm, kZeroBounds, kZeroBounds] },
        { input: 0xffff0000, expected: [kZeroBounds, kZeroBounds, kOneBoundsUnorm, kOneBoundsUnorm] },
        { input: 0xff00ff00, expected: [kZeroBounds, kOneBoundsUnorm, kZeroBounds, kOneBoundsUnorm] },
        { input: 0x00ff00ff, expected: [kOneBoundsUnorm, kZeroBounds, kOneBoundsUnorm, kZeroBounds] },
        { input: 0xffffffff, expected: [kOneBoundsUnorm, kOneBoundsUnorm, kOneBoundsUnorm, kOneBoundsUnorm] },
        { input: 0x80808080, expected: [kHalfBounds4x8unorm, kHalfBounds4x8unorm, kHalfBounds4x8unorm, kHalfBounds4x8unorm] },
      ]
    )
    .fn(t => {
      const expected = FP.f32.toVector(t.params.expected);
      const got = FP.f32.unpack4x8unormInterval(t.params.input);
      t.expect(
        objectEquals(expected, got),
        `unpack4x8unormInterval(${t.params.input}) returned [${got}]. Expected [${expected}]`
      );
    });
}

interface VectorToIntervalCase {
  input: number[];
  expected: number | IntervalBounds;
}

g.test('lengthIntervalVector_f32')
  .paramsSubcasesOnly<VectorToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.

      // vec2
      {input: [1.0, 0.0], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [0.0, 1.0], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [1.0, 1.0], expected: [hexToF64(0x3ff6_a09d_b000_0000n), hexToF64(0x3ff6_a09f_1000_0000n)] },  // ~√2
      {input: [-1.0, -1.0], expected: [hexToF64(0x3ff6_a09d_b000_0000n), hexToF64(0x3ff6_a09f_1000_0000n)] },  // ~√2
      {input: [-1.0, 1.0], expected: [hexToF64(0x3ff6_a09d_b000_0000n), hexToF64(0x3ff6_a09f_1000_0000n)] },  // ~√2
      {input: [0.1, 0.0], expected: [hexToF64(0x3fb9_9998_9000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1

      // vec3
      {input: [1.0, 0.0, 0.0], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [0.0, 1.0, 0.0], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [0.0, 0.0, 1.0], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [1.0, 1.0, 1.0], expected: [hexToF64(0x3ffb_b67a_1000_0000n), hexToF64(0x3ffb_b67b_b000_0000n)] },  // ~√3
      {input: [-1.0, -1.0, -1.0], expected: [hexToF64(0x3ffb_b67a_1000_0000n), hexToF64(0x3ffb_b67b_b000_0000n)] },  // ~√3
      {input: [1.0, -1.0, -1.0], expected: [hexToF64(0x3ffb_b67a_1000_0000n), hexToF64(0x3ffb_b67b_b000_0000n)] },  // ~√3
      {input: [0.1, 0.0, 0.0], expected: [hexToF64(0x3fb9_9998_9000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1

      // vec4
      {input: [1.0, 0.0, 0.0, 0.0], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [0.0, 1.0, 0.0, 0.0], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [0.0, 0.0, 1.0, 0.0], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [0.0, 0.0, 0.0, 1.0], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      {input: [1.0, 1.0, 1.0, 1.0], expected: [hexToF64(0x3fff_ffff_7000_0000n), hexToF64(0x4000_0000_9000_0000n)] },  // ~2
      {input: [-1.0, -1.0, -1.0, -1.0], expected: [hexToF64(0x3fff_ffff_7000_0000n), hexToF64(0x4000_0000_9000_0000n)] },  // ~2
      {input: [-1.0, 1.0, -1.0, 1.0], expected: [hexToF64(0x3fff_ffff_7000_0000n), hexToF64(0x4000_0000_9000_0000n)] },  // ~2
      {input: [0.1, 0.0, 0.0, 0.0], expected: [hexToF64(0x3fb9_9998_9000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1

      // Test that dot going OOB bounds in the intermediate calculations propagates
      { input: [kValue.f32.positive.nearest_max, kValue.f32.positive.max, kValue.f32.negative.min], expected: kAnyBounds },
      { input: [kValue.f32.positive.max, kValue.f32.positive.nearest_max, kValue.f32.negative.min], expected: kAnyBounds },
      { input: [kValue.f32.negative.min, kValue.f32.positive.max, kValue.f32.positive.nearest_max], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.lengthInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.lengthInterval([${t.params.input}]) returned ${got}. Expected ${expected}`
    );
  });

interface VectorPairToIntervalCase {
  input: [number[], number[]];
  expected: number | IntervalBounds;
}

g.test('distanceIntervalVector_f32')
  .paramsSubcasesOnly<VectorPairToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.
      //
      // distance(x, y), where x - y = 0 has an acceptance interval of kAnyBounds,
      // because distance(x, y) = length(x - y), and length(0) = kAnyBounds

      // vec2
      { input: [[1.0, 0.0], [1.0, 0.0]], expected: kAnyBounds },
      { input: [[1.0, 0.0], [0.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0], [1.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[-1.0, 0.0], [0.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0], [-1.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 1.0], [-1.0, 0.0]], expected: [hexToF64(0x3ff6_a09d_b000_0000n), hexToF64(0x3ff6_a09f_1000_0000n)] },  // ~√2
      { input: [[0.1, 0.0], [0.0, 0.0]], expected: [hexToF64(0x3fb9_9998_9000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1

      // vec3
      { input: [[1.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: kAnyBounds },
      { input: [[1.0, 0.0, 0.0], [0.0, 0.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 1.0, 0.0], [0.0, 0.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 1.0], [0.0, 0.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0], [0.0, 1.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0], [0.0, 0.0, 1.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[1.0, 1.0, 1.0], [0.0, 0.0, 0.0]], expected: [hexToF64(0x3ffb_b67a_1000_0000n), hexToF64(0x3ffb_b67b_b000_0000n)] },  // ~√3
      { input: [[0.0, 0.0, 0.0], [1.0, 1.0, 1.0]], expected: [hexToF64(0x3ffb_b67a_1000_0000n), hexToF64(0x3ffb_b67b_b000_0000n)] },  // ~√3
      { input: [[-1.0, -1.0, -1.0], [0.0, 0.0, 0.0]], expected: [hexToF64(0x3ffb_b67a_1000_0000n), hexToF64(0x3ffb_b67b_b000_0000n)] },  // ~√3
      { input: [[0.0, 0.0, 0.0], [-1.0, -1.0, -1.0]], expected: [hexToF64(0x3ffb_b67a_1000_0000n), hexToF64(0x3ffb_b67b_b000_0000n)] },  // ~√3
      { input: [[0.1, 0.0, 0.0], [0.0, 0.0, 0.0]], expected: [hexToF64(0x3fb9_9998_9000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      { input: [[0.0, 0.0, 0.0], [0.1, 0.0, 0.0]], expected: [hexToF64(0x3fb9_9998_9000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1

      // vec4
      { input: [[1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: kAnyBounds },
      { input: [[1.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0, 1.0], [0.0, 0.0, 0.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 1.0]], expected: [hexToF64(0x3fef_ffff_7000_0000n), hexToF64(0x3ff0_0000_9000_0000n)] },  // ~1
      { input: [[1.0, 1.0, 1.0, 1.0], [0.0, 0.0, 0.0, 0.0]], expected: [hexToF64(0x3fff_ffff_7000_0000n), hexToF64(0x4000_0000_9000_0000n)] },  // ~2
      { input: [[0.0, 0.0, 0.0, 0.0], [1.0, 1.0, 1.0, 1.0]], expected: [hexToF64(0x3fff_ffff_7000_0000n), hexToF64(0x4000_0000_9000_0000n)] },  // ~2
      { input: [[-1.0, 1.0, -1.0, 1.0], [0.0, 0.0, 0.0, 0.0]], expected: [hexToF64(0x3fff_ffff_7000_0000n), hexToF64(0x4000_0000_9000_0000n)] },  // ~2
      { input: [[0.0, 0.0, 0.0, 0.0], [1.0, -1.0, 1.0, -1.0]], expected: [hexToF64(0x3fff_ffff_7000_0000n), hexToF64(0x4000_0000_9000_0000n)] },  // ~2
      { input: [[0.1, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0]], expected: [hexToF64(0x3fb9_9998_9000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
      { input: [[0.0, 0.0, 0.0, 0.0], [0.1, 0.0, 0.0, 0.0]], expected: [hexToF64(0x3fb9_9998_9000_0000n), hexToF64(0x3fb9_999a_7000_0000n)] },  // ~0.1
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.distanceInterval(...t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.distanceInterval([${t.params.input[0]}, ${t.params.input[1]}]) returned ${got}. Expected ${expected}`
    );
  });

g.test('dotInterval_f32')
  .paramsSubcasesOnly<VectorPairToIntervalCase>(
    // prettier-ignore
    [
      // vec2
      { input: [[1.0, 0.0], [1.0, 0.0]], expected: 1.0 },
      { input: [[0.0, 1.0], [0.0, 1.0]], expected: 1.0 },
      { input: [[1.0, 1.0], [1.0, 1.0]], expected: 2.0 },
      { input: [[-1.0, -1.0], [-1.0, -1.0]], expected: 2.0 },
      { input: [[-1.0, 1.0], [1.0, -1.0]], expected: -2.0 },
      { input: [[0.1, 0.0], [1.0, 0.0]], expected: [hexToF64(0x3fb9_9999_8000_0000n), hexToF64(0x3fb9_9999_a000_0000n)]},  // ~0.1

      // vec3
      { input: [[1.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: 1.0 },
      { input: [[0.0, 1.0, 0.0], [0.0, 1.0, 0.0]], expected: 1.0 },
      { input: [[0.0, 0.0, 1.0], [0.0, 0.0, 1.0]], expected: 1.0 },
      { input: [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0]], expected: 3.0 },
      { input: [[-1.0, -1.0, -1.0], [-1.0, -1.0, -1.0]], expected: 3.0 },
      { input: [[1.0, -1.0, -1.0], [-1.0, 1.0, -1.0]], expected: -1.0 },
      { input: [[0.1, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [hexToF64(0x3fb9_9999_8000_0000n), hexToF64(0x3fb9_9999_a000_0000n)]},  // ~0.1

      // vec4
      { input: [[1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: 1.0 },
      { input: [[0.0, 1.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0]], expected: 1.0 },
      { input: [[0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 1.0, 0.0]], expected: 1.0 },
      { input: [[0.0, 0.0, 0.0, 1.0], [0.0, 0.0, 0.0, 1.0]], expected: 1.0 },
      { input: [[1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 1.0, 1.0]], expected: 4.0 },
      { input: [[-1.0, -1.0, -1.0, -1.0], [-1.0, -1.0, -1.0, -1.0]], expected: 4.0 },
      { input: [[-1.0, 1.0, -1.0, 1.0], [1.0, -1.0, 1.0, -1.0]], expected: -4.0 },
      { input: [[0.1, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [hexToF64(0x3fb9_9999_8000_0000n), hexToF64(0x3fb9_9999_a000_0000n)]},  // ~0.1

      // Test that going out of bounds in the intermediate calculations is caught correctly.
      { input: [[kValue.f32.positive.nearest_max, kValue.f32.positive.max, kValue.f32.negative.min], [1.0, 1.0, 1.0]], expected: kAnyBounds },
      { input: [[kValue.f32.positive.nearest_max, kValue.f32.negative.min, kValue.f32.positive.max], [1.0, 1.0, 1.0]], expected: kAnyBounds },
      { input: [[kValue.f32.positive.max, kValue.f32.positive.nearest_max, kValue.f32.negative.min], [1.0, 1.0, 1.0]], expected: kAnyBounds },
      { input: [[kValue.f32.negative.min, kValue.f32.positive.nearest_max, kValue.f32.positive.max], [1.0, 1.0, 1.0]], expected: kAnyBounds },
      { input: [[kValue.f32.positive.max, kValue.f32.negative.min, kValue.f32.positive.nearest_max], [1.0, 1.0, 1.0]], expected: kAnyBounds },
      { input: [[kValue.f32.negative.min, kValue.f32.positive.max, kValue.f32.positive.nearest_max], [1.0, 1.0, 1.0]], expected: kAnyBounds },

      // https://github.com/gpuweb/cts/issues/2155
      { input: [[kValue.f32.positive.max, 1.0, 2.0, 3.0], [-1.0, kValue.f32.positive.max, -2.0, -3.0]], expected: [-13, 0] },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.dotInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.dotInterval([${x}], [${y}]) returned ${got}. Expected ${expected}`
    );
  });

interface VectorToVectorCase {
  input: number[];
  expected: (number | IntervalBounds)[];
}

g.test('normalizeInterval_f32')
  .paramsSubcasesOnly<VectorToVectorCase>(
    // prettier-ignore
    [
      // vec2
      {input: [1.0, 0.0], expected: [[hexToF64(0x3fef_fffe_7000_0000n), hexToF64(0x3ff0_0000_b000_0000n)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~1.0, ~0.0]
      {input: [0.0, 1.0], expected: [[hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF64(0x3fef_fffe_7000_0000n), hexToF64(0x3ff0_0000_b000_0000n)]] },  // [ ~0.0, ~1.0]
      {input: [-1.0, 0.0], expected: [[hexToF64(0xbff0_0000_b000_0000n), hexToF64(0xbfef_fffe_7000_0000n)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~1.0, ~0.0]
      {input: [1.0, 1.0], expected: [[hexToF64(0x3fe6_a09d_5000_0000n), hexToF64(0x3fe6_a09f_9000_0000n)], [hexToF64(0x3fe6_a09d_5000_0000n), hexToF64(0x3fe6_a09f_9000_0000n)]] },  // [ ~1/√2, ~1/√2]

      // vec3
      {input: [1.0, 0.0, 0.0], expected: [[hexToF64(0x3fef_fffe_7000_0000n), hexToF64(0x3ff0_0000_b000_0000n)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~1.0, ~0.0, ~0.0]
      {input: [0.0, 1.0, 0.0], expected: [[hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF64(0x3fef_fffe_7000_0000n), hexToF64(0x3ff0_0000_b000_0000n)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~0.0, ~1.0, ~0.0]
      {input: [0.0, 0.0, 1.0], expected: [[hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF64(0x3fef_fffe_7000_0000n), hexToF64(0x3ff0_0000_b000_0000n)]] },  // [ ~0.0, ~0.0, ~1.0]
      {input: [-1.0, 0.0, 0.0], expected: [[hexToF64(0xbff0_0000_b000_0000n), hexToF64(0xbfef_fffe_7000_0000n)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~1.0, ~0.0, ~0.0]
      {input: [1.0, 1.0, 1.0], expected: [[hexToF64(0x3fe2_79a6_5000_0000n), hexToF64(0x3fe2_79a8_5000_0000n)], [hexToF64(0x3fe2_79a6_5000_0000n), hexToF64(0x3fe2_79a8_5000_0000n)], [hexToF64(0x3fe2_79a6_5000_0000n), hexToF64(0x3fe2_79a8_5000_0000n)]] },  // [ ~1/√3, ~1/√3, ~1/√3]

      // vec4
      {input: [1.0, 0.0, 0.0, 0.0], expected: [[hexToF64(0x3fef_fffe_7000_0000n), hexToF64(0x3ff0_0000_b000_0000n)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~1.0, ~0.0, ~0.0, ~0.0]
      {input: [0.0, 1.0, 0.0, 0.0], expected: [[hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF64(0x3fef_fffe_7000_0000n), hexToF64(0x3ff0_0000_b000_0000n)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~0.0, ~1.0, ~0.0, ~0.0]
      {input: [0.0, 0.0, 1.0, 0.0], expected: [[hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF64(0x3fef_fffe_7000_0000n), hexToF64(0x3ff0_0000_b000_0000n)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~0.0, ~0.0, ~1.0, ~0.0]
      {input: [0.0, 0.0, 0.0, 1.0], expected: [[hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF64(0x3fef_fffe_7000_0000n), hexToF64(0x3ff0_0000_b000_0000n)]] },  // [ ~0.0, ~0.0, ~0.0, ~1.0]
      {input: [-1.0, 0.0, 0.0, 0.0], expected: [[hexToF64(0xbff0_0000_b000_0000n), hexToF64(0xbfef_fffe_7000_0000n)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~1.0, ~0.0, ~0.0, ~0.0]
      {input: [1.0, 1.0, 1.0, 1.0], expected: [[hexToF64(0x3fdf_fffe_7000_0000n), hexToF64(0x3fe0_0000_b000_0000n)], [hexToF64(0x3fdf_fffe_7000_0000n), hexToF64(0x3fe0_0000_b000_0000n)], [hexToF64(0x3fdf_fffe_7000_0000n), hexToF64(0x3fe0_0000_b000_0000n)], [hexToF64(0x3fdf_fffe_7000_0000n), hexToF64(0x3fe0_0000_b000_0000n)]] },  // [ ~1/√4, ~1/√4, ~1/√4]
    ]
  )
  .fn(t => {
    const x = t.params.input;
    const expected = FP.f32.toVector(t.params.expected);
    const got = FP.f32.normalizeInterval(x);
    t.expect(
      objectEquals(expected, got),
      `f32.normalizeInterval([${x}]) returned ${got}. Expected ${expected}`
    );
  });

interface VectorPairToVectorCase {
  input: [number[], number[]];
  expected: (number | IntervalBounds)[];
}

g.test('crossInterval_f32')
  .paramsSubcasesOnly<VectorPairToVectorCase>(
    // prettier-ignore
    [
      // parallel vectors, AXB == 0
      { input: [[1.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [0.0, 0.0, 0.0] },
      { input: [[0.0, 1.0, 0.0], [0.0, 1.0, 0.0]], expected: [0.0, 0.0, 0.0] },
      { input: [[0.0, 0.0, 1.0], [0.0, 0.0, 1.0]], expected: [0.0, 0.0, 0.0] },
      { input: [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0]], expected: [0.0, 0.0, 0.0] },
      { input: [[-1.0, -1.0, -1.0], [-1.0, -1.0, -1.0]], expected: [0.0, 0.0, 0.0] },
      { input: [[0.1, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [0.0, 0.0, 0.0] },
      { input: [[kValue.f32.subnormal.positive.max, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [0.0, 0.0, 0.0] },

      // non-parallel vectors, AXB != 0
      // f32 normals
      { input: [[1.0, -1.0, -1.0], [-1.0, 1.0, -1.0]], expected: [2.0, 2.0, 0.0] },
      { input: [[1.0, 2, 3], [1.0, 5.0, 7.0]], expected: [-1, -4, 3] },

      // f64 normals
      { input: [[0.1, -0.1, -0.1], [-0.1, 0.1, -0.1]],
        expected: [[hexToF32(0x3ca3d708), hexToF32(0x3ca3d70b)],  // ~0.02
          [hexToF32(0x3ca3d708), hexToF32(0x3ca3d70b)],  // ~0.02
          [hexToF32(0xb1400000), hexToF32(0x31400000)]] },  // ~0

      // f32 subnormals
      { input: [[kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.max, kValue.f32.subnormal.negative.min],
          [kValue.f32.subnormal.negative.min, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.negative.max]],
        expected: [[0.0, hexToF32(0x00000002)],  // ~0
          [0.0, hexToF32(0x00000002)],  // ~0
          [hexToF32(0x80000001), hexToF32(0x00000001)]] },  // ~0
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toVector(t.params.expected);
    const got = FP.f32.crossInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.crossInterval([${x}], [${y}]) returned ${got}. Expected ${expected}`
    );
  });

g.test('reflectInterval_f32')
  .paramsSubcasesOnly<VectorPairToVectorCase>(
    // prettier-ignore
    [
      // vec2s
      { input: [[1.0, 0.0], [1.0, 0.0]], expected: [-1.0, 0.0] },
      { input: [[1.0, 0.0], [0.0, 1.0]], expected: [1.0, 0.0] },
      { input: [[0.0, 1.0], [0.0, 1.0]], expected: [0.0, -1.0] },
      { input: [[0.0, 1.0], [1.0, 0.0]], expected: [0.0, 1.0] },
      { input: [[1.0, 1.0], [1.0, 1.0]], expected: [-3.0, -3.0] },
      { input: [[-1.0, -1.0], [1.0, 1.0]], expected: [3.0, 3.0] },
      { input: [[0.1, 0.1], [1.0, 1.0]], expected: [[hexToF32(0xbe99999a), hexToF32(0xbe999998)], [hexToF32(0xbe99999a), hexToF32(0xbe999998)]] },  // [~-0.3, ~-0.3]
      { input: [[kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.max], [1.0, 1.0]], expected: [[hexToF32(0x80fffffe), hexToF32(0x00800001)], [hexToF32(0x80ffffff), hexToF32(0x00000002)]] },  // [~0.0, ~0.0]

      // vec3s
      { input: [[1.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [-1.0, 0.0, 0.0] },
      { input: [[0.0, 1.0, 0.0], [1.0, 0.0, 0.0]], expected: [0.0, 1.0, 0.0] },
      { input: [[0.0, 0.0, 1.0], [1.0, 0.0, 0.0]], expected: [0.0, 0.0, 1.0] },
      { input: [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0]], expected: [1.0, 0.0, 0.0] },
      { input: [[1.0, 0.0, 0.0], [0.0, 0.0, 1.0]], expected: [1.0, 0.0, 0.0] },
      { input: [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0]], expected: [-5.0, -5.0, -5.0] },
      { input: [[-1.0, -1.0, -1.0], [1.0, 1.0, 1.0]], expected: [5.0, 5.0, 5.0] },
      { input: [[0.1, 0.1, 0.1], [1.0, 1.0, 1.0]], expected: [[hexToF32(0xbf000001), hexToF32(0xbefffffe)], [hexToF32(0xbf000001), hexToF32(0xbefffffe)], [hexToF32(0xbf000001), hexToF32(0xbefffffe)]] },  // [~-0.5, ~-0.5, ~-0.5]
      { input: [[kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.max, 0.0], [1.0, 1.0, 1.0]], expected: [[hexToF32(0x80fffffe), hexToF32(0x00800001)], [hexToF32(0x80ffffff), hexToF32(0x00000002)], [hexToF32(0x80fffffe), hexToF32(0x00000002)]] },  // [~0.0, ~0.0, ~0.0]

      // vec4s
      { input: [[1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [-1.0, 0.0, 0.0, 0.0] },
      { input: [[0.0, 1.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [0.0, 1.0, 0.0, 0.0] },
      { input: [[0.0, 0.0, 1.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [0.0, 0.0, 1.0, 0.0] },
      { input: [[0.0, 0.0, 0.0, 1.0], [1.0, 0.0, 0.0, 0.0]], expected: [0.0, 0.0, 0.0, 1.0] },
      { input: [[1.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0]], expected: [1.0, 0.0, 0.0, 0.0] },
      { input: [[1.0, 0.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0]], expected: [1.0, 0.0, 0.0, 0.0] },
      { input: [[1.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 1.0]], expected: [1.0, 0.0, 0.0, 0.0] },
      { input: [[-1.0, -1.0, -1.0, -1.0], [1.0, 1.0, 1.0, 1.0]], expected: [7.0, 7.0, 7.0, 7.0] },
      { input: [[0.1, 0.1, 0.1, 0.1], [1.0, 1.0, 1.0, 1.0]], expected: [[hexToF32(0xbf333335), hexToF32(0xbf333332)], [hexToF32(0xbf333335), hexToF32(0xbf333332)], [hexToF32(0xbf333335), hexToF32(0xbf333332)], [hexToF32(0xbf333335), hexToF32(0xbf333332)]] },  // [~-0.7, ~-0.7, ~-0.7, ~-0.7]
      { input: [[kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.max, 0.0, 0.0], [1.0, 1.0, 1.0, 1.0]], expected: [[hexToF32(0x80fffffe), hexToF32(0x00800001)], [hexToF32(0x80ffffff), hexToF32(0x00000002)], [hexToF32(0x80fffffe), hexToF32(0x00000002)], [hexToF32(0x80fffffe), hexToF32(0x00000002)]] },  // [~0.0, ~0.0, ~0.0, ~0.0]

      // Test that dot going OOB bounds in the intermediate calculations propagates
      { input: [[kValue.f32.positive.nearest_max, kValue.f32.positive.max, kValue.f32.negative.min], [1.0, 1.0, 1.0]], expected: [kAnyBounds, kAnyBounds, kAnyBounds] },
      { input: [[kValue.f32.positive.nearest_max, kValue.f32.negative.min, kValue.f32.positive.max], [1.0, 1.0, 1.0]], expected: [kAnyBounds, kAnyBounds, kAnyBounds] },
      { input: [[kValue.f32.positive.max, kValue.f32.positive.nearest_max, kValue.f32.negative.min], [1.0, 1.0, 1.0]], expected: [kAnyBounds, kAnyBounds, kAnyBounds] },
      { input: [[kValue.f32.negative.min, kValue.f32.positive.nearest_max, kValue.f32.positive.max], [1.0, 1.0, 1.0]], expected: [kAnyBounds, kAnyBounds, kAnyBounds] },
      { input: [[kValue.f32.positive.max, kValue.f32.negative.min, kValue.f32.positive.nearest_max], [1.0, 1.0, 1.0]], expected: [kAnyBounds, kAnyBounds, kAnyBounds] },
      { input: [[kValue.f32.negative.min, kValue.f32.positive.max, kValue.f32.positive.nearest_max], [1.0, 1.0, 1.0]], expected: [kAnyBounds, kAnyBounds, kAnyBounds] },

      // Test that post-dot going OOB propagates
      { input: [[kValue.f32.positive.max, 1.0, 2.0, 3.0], [-1.0, kValue.f32.positive.max, -2.0, -3.0]], expected: [kAnyBounds, kAnyBounds, kAnyBounds, kAnyBounds] },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toVector(t.params.expected);
    const got = FP.f32.reflectInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.reflectInterval([${x}], [${y}]) returned ${JSON.stringify(
        got
      )}. Expected ${JSON.stringify(expected)}`
    );
  });
