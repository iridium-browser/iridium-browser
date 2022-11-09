export const description = `
F32Interval unit tests.
`;

import { makeTestGroup } from '../common/framework/test_group.js';
import { objectEquals } from '../common/util/util.js';
import { kValue } from '../webgpu/util/constants.js';
import {
  absInterval,
  absoluteErrorInterval,
  acoshAlternativeInterval,
  acoshPrimaryInterval,
  additionInterval,
  asinhInterval,
  atanInterval,
  atan2Interval,
  atanhInterval,
  ceilInterval,
  clampMedianInterval,
  clampMinMaxInterval,
  correctlyRoundedInterval,
  cosInterval,
  coshInterval,
  degreesInterval,
  divisionInterval,
  dotInterval,
  expInterval,
  exp2Interval,
  F32Interval,
  floorInterval,
  fractInterval,
  IntervalBounds,
  inverseSqrtInterval,
  ldexpInterval,
  lengthInterval,
  logInterval,
  log2Interval,
  maxInterval,
  minInterval,
  mixImpreciseInterval,
  mixPreciseInterval,
  multiplicationInterval,
  negationInterval,
  normalizeInterval,
  powInterval,
  quantizeToF16Interval,
  radiansInterval,
  remainderInterval,
  roundInterval,
  saturateInterval,
  signInterval,
  sinInterval,
  sinhInterval,
  smoothStepInterval,
  sqrtInterval,
  stepInterval,
  subtractionInterval,
  tanInterval,
  tanhInterval,
  truncInterval,
  ulpInterval,
} from '../webgpu/util/f32_interval.js';
import { hexToF32, hexToF64, oneULP } from '../webgpu/util/math.js';

import { UnitTest } from './unit_test.js';

export const g = makeTestGroup(UnitTest);

/** Bounds indicating an expectation of an interval of all possible values */
const kAny: IntervalBounds = [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY];

/** @returns a number N * ULP greater than the provided number */
function plusNULP(x: number, n: number): number {
  return x + n * oneULP(x);
}

/** @returns a number one ULP greater than the provided number */
function plusOneULP(x: number): number {
  return plusNULP(x, 1);
}

/** @returns a number N * ULP less than the provided number */
function minusNULP(x: number, n: number): number {
  return x - n * oneULP(x);
}

/** @returns a number one ULP less than the provided number */
function minusOneULP(x: number): number {
  return minusNULP(x, 1);
}

/** @returns the expected IntervalBounds adjusted by the given error function
 *
 * @param expected the bounds to be adjusted
 * @param error error function to adjust the bounds via
 */
function applyError(expected: IntervalBounds, error: (n: number) => number): IntervalBounds {
  if (expected !== kAny) {
    const begin = expected[0];
    const end = expected.length === 2 ? expected[1] : begin;
    expected = [begin - error(begin), end + error(end)];
  }

  return expected;
}

interface ConstructorCase {
  input: IntervalBounds;
  expected: IntervalBounds;
}

g.test('constructor')
  .paramsSubcasesOnly<ConstructorCase>(
    // prettier-ignore
    [
      // Common cases
      { input: [0, 10], expected: [0, 10]},
      { input: [-5, 0], expected: [-5, 0]},
      { input: [-5, 10], expected: [-5, 10]},
      { input: [0], expected: [0]},
      { input: [10], expected: [10]},
      { input: [-5], expected: [-5]},

      // Edges
      { input: [0, kValue.f32.positive.max], expected: [0, kValue.f32.positive.max]},
      { input: [kValue.f32.negative.min, 0], expected: [kValue.f32.negative.min, 0]},
      { input: [kValue.f32.negative.min, kValue.f32.positive.max], expected: [kValue.f32.negative.min, kValue.f32.positive.max]},

      // Out of range
      { input: [0, 2 * kValue.f32.positive.max], expected: [0, 2 * kValue.f32.positive.max]},
      { input: [2 * kValue.f32.negative.min, 0], expected: [2 * kValue.f32.negative.min, 0]},
      { input: [2 * kValue.f32.negative.min, 2 * kValue.f32.positive.max], expected: [2 * kValue.f32.negative.min, 2 * kValue.f32.positive.max]},

      // Infinities
      { input: [0, kValue.f32.infinity.positive], expected: [0, Number.POSITIVE_INFINITY]},
      { input: [kValue.f32.infinity.negative, 0], expected: [Number.NEGATIVE_INFINITY, 0]},
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAny},
    ]
  )
  .fn(t => {
    const i = new F32Interval(...t.params.input);
    t.expect(
      objectEquals(i.bounds(), t.params.expected),
      `F32Interval([${t.params.input}]) returned ${i}. Expected [${t.params.expected}]`
    );
  });

interface ContainsNumberCase {
  bounds: IntervalBounds;
  value: number;
  expected: boolean;
}

g.test('contains_number')
  .paramsSubcasesOnly<ContainsNumberCase>(
    // prettier-ignore
    [
    // Common usage
    { bounds: [0, 10], value: 0, expected: true },
    { bounds: [0, 10], value: 10, expected: true },
    { bounds: [0, 10], value: 5, expected: true },
    { bounds: [0, 10], value: -5, expected: false },
    { bounds: [0, 10], value: 50, expected: false },
    { bounds: [0, 10], value: Number.NaN, expected: false },
    { bounds: [-5, 10], value: 0, expected: true },
    { bounds: [-5, 10], value: 10, expected: true },
    { bounds: [-5, 10], value: 5, expected: true },
    { bounds: [-5, 10], value: -5, expected: true },
    { bounds: [-5, 10], value: -6, expected: false },
    { bounds: [-5, 10], value: 50, expected: false },
    { bounds: [-5, 10], value: -10, expected: false },

    // Point
    { bounds: [0], value: 0, expected: true },
    { bounds: [0], value: 10, expected: false },
    { bounds: [0], value: -1000, expected: false },
    { bounds: [10], value: 10, expected: true },
    { bounds: [10], value: 0, expected: false },
    { bounds: [10], value: -10, expected: false },
    { bounds: [10], value: 11, expected: false },

    // Upper infinity
    { bounds: [0, kValue.f32.infinity.positive], value: kValue.f32.positive.min, expected: true },
    { bounds: [0, kValue.f32.infinity.positive], value: kValue.f32.positive.max, expected: true },
    { bounds: [0, kValue.f32.infinity.positive], value: kValue.f32.infinity.positive, expected: true },
    { bounds: [0, kValue.f32.infinity.positive], value: kValue.f32.negative.min, expected: false },
    { bounds: [0, kValue.f32.infinity.positive], value: kValue.f32.negative.max, expected: false },
    { bounds: [0, kValue.f32.infinity.positive], value: kValue.f32.infinity.negative, expected: false },

    // Lower infinity
    { bounds: [kValue.f32.infinity.negative, 0], value: kValue.f32.positive.min, expected: false },
    { bounds: [kValue.f32.infinity.negative, 0], value: kValue.f32.positive.max, expected: false },
    { bounds: [kValue.f32.infinity.negative, 0], value: kValue.f32.infinity.positive, expected: false },
    { bounds: [kValue.f32.infinity.negative, 0], value: kValue.f32.negative.min, expected: true },
    { bounds: [kValue.f32.infinity.negative, 0], value: kValue.f32.negative.max, expected: true },
    { bounds: [kValue.f32.infinity.negative, 0], value: kValue.f32.infinity.negative, expected: true },

    // Full infinity
    { bounds: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], value: kValue.f32.positive.min, expected: true },
    { bounds: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], value: kValue.f32.positive.max, expected: true },
    { bounds: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], value: kValue.f32.infinity.positive, expected: true },
    { bounds: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], value: kValue.f32.negative.min, expected: true },
    { bounds: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], value: kValue.f32.negative.max, expected: true },
    { bounds: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], value: kValue.f32.infinity.negative, expected: true },
    { bounds: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], value: Number.NaN, expected: true },

    // Maximum f32 boundary
    { bounds: [0, kValue.f32.positive.max], value: kValue.f32.positive.min, expected: true },
    { bounds: [0, kValue.f32.positive.max], value: kValue.f32.positive.max, expected: true },
    { bounds: [0, kValue.f32.positive.max], value: kValue.f32.infinity.positive, expected: false },
    { bounds: [0, kValue.f32.positive.max], value: kValue.f32.negative.min, expected: false },
    { bounds: [0, kValue.f32.positive.max], value: kValue.f32.negative.max, expected: false },
    { bounds: [0, kValue.f32.positive.max], value: kValue.f32.infinity.negative, expected: false },

    // Minimum f32 boundary
    { bounds: [kValue.f32.negative.min, 0], value: kValue.f32.positive.min, expected: false },
    { bounds: [kValue.f32.negative.min, 0], value: kValue.f32.positive.max, expected: false },
    { bounds: [kValue.f32.negative.min, 0], value: kValue.f32.infinity.positive, expected: false },
    { bounds: [kValue.f32.negative.min, 0], value: kValue.f32.negative.min, expected: true },
    { bounds: [kValue.f32.negative.min, 0], value: kValue.f32.negative.max, expected: true },
    { bounds: [kValue.f32.negative.min, 0], value: kValue.f32.infinity.negative, expected: false },

    // Out of range high
    { bounds: [0, 2 * kValue.f32.positive.max], value: kValue.f32.positive.min, expected: true },
    { bounds: [0, 2 * kValue.f32.positive.max], value: kValue.f32.positive.max, expected: true },
    { bounds: [0, 2 * kValue.f32.positive.max], value: kValue.f32.infinity.positive, expected: false },
    { bounds: [0, 2 * kValue.f32.positive.max], value: kValue.f32.negative.min, expected: false },
    { bounds: [0, 2 * kValue.f32.positive.max], value: kValue.f32.negative.max, expected: false },
    { bounds: [0, 2 * kValue.f32.positive.max], value: kValue.f32.infinity.negative, expected: false },

    // Out of range low
    { bounds: [2 * kValue.f32.negative.min, 0], value: kValue.f32.positive.min, expected: false },
    { bounds: [2 * kValue.f32.negative.min, 0], value: kValue.f32.positive.max, expected: false },
    { bounds: [2 * kValue.f32.negative.min, 0], value: kValue.f32.infinity.positive, expected: false },
    { bounds: [2 * kValue.f32.negative.min, 0], value: kValue.f32.negative.min, expected: true },
    { bounds: [2 * kValue.f32.negative.min, 0], value: kValue.f32.negative.max, expected: true },
    { bounds: [2 * kValue.f32.negative.min, 0], value: kValue.f32.infinity.negative, expected: false },

    // Subnormals
    { bounds: [0, kValue.f32.positive.min], value: kValue.f32.subnormal.positive.min, expected: true },
    { bounds: [0, kValue.f32.positive.min], value: kValue.f32.subnormal.positive.max, expected: true },
    { bounds: [0, kValue.f32.positive.min], value: kValue.f32.subnormal.negative.min, expected: false },
    { bounds: [0, kValue.f32.positive.min], value: kValue.f32.subnormal.negative.max, expected: false },
    { bounds: [kValue.f32.negative.max, 0], value: kValue.f32.subnormal.positive.min, expected: false },
    { bounds: [kValue.f32.negative.max, 0], value: kValue.f32.subnormal.positive.max, expected: false },
    { bounds: [kValue.f32.negative.max, 0], value: kValue.f32.subnormal.negative.min, expected: true },
    { bounds: [kValue.f32.negative.max, 0], value: kValue.f32.subnormal.negative.max, expected: true },
    { bounds: [0, kValue.f32.subnormal.positive.min], value: kValue.f32.subnormal.positive.min, expected: true },
    { bounds: [0, kValue.f32.subnormal.positive.min], value: kValue.f32.subnormal.positive.max, expected: false },
    { bounds: [0, kValue.f32.subnormal.positive.min], value: kValue.f32.subnormal.negative.min, expected: false },
    { bounds: [0, kValue.f32.subnormal.positive.min], value: kValue.f32.subnormal.negative.max, expected: false },
    { bounds: [kValue.f32.subnormal.negative.max, 0], value: kValue.f32.subnormal.positive.min, expected: false },
    { bounds: [kValue.f32.subnormal.negative.max, 0], value: kValue.f32.subnormal.positive.max, expected: false },
    { bounds: [kValue.f32.subnormal.negative.max, 0], value: kValue.f32.subnormal.negative.min, expected: false },
    { bounds: [kValue.f32.subnormal.negative.max, 0], value: kValue.f32.subnormal.negative.max, expected: true },
    ]
  )
  .fn(t => {
    const i = new F32Interval(...t.params.bounds);
    const value = t.params.value;
    const expected = t.params.expected;

    const got = i.contains(value);
    t.expect(expected === got, `${i}.contains(${value}) returned ${got}. Expected ${expected}`);
  });

interface ContainsIntervalCase {
  lhs: IntervalBounds;
  rhs: IntervalBounds;
  expected: boolean;
}

g.test('contains_interval')
  .paramsSubcasesOnly<ContainsIntervalCase>(
    // prettier-ignore
    [
      // Common usage
      { lhs: [-10, 10], rhs: [0], expected: true},
      { lhs: [-10, 10], rhs: [-1, 0], expected: true},
      { lhs: [-10, 10], rhs: [0, 2], expected: true},
      { lhs: [-10, 10], rhs: [-1, 2], expected: true},
      { lhs: [-10, 10], rhs: [0, 10], expected: true},
      { lhs: [-10, 10], rhs: [-10, 2], expected: true},
      { lhs: [-10, 10], rhs: [-10, 10], expected: true},
      { lhs: [-10, 10], rhs: [-100, 10], expected: false},

      // Upper infinity
      { lhs: [0, kValue.f32.infinity.positive], rhs: [0], expected: true},
      { lhs: [0, kValue.f32.infinity.positive], rhs: [-1, 0], expected: false},
      { lhs: [0, kValue.f32.infinity.positive], rhs: [0, 1], expected: true},
      { lhs: [0, kValue.f32.infinity.positive], rhs: [0, kValue.f32.positive.max], expected: true},
      { lhs: [0, kValue.f32.infinity.positive], rhs: [0, kValue.f32.infinity.positive], expected: true},
      { lhs: [0, kValue.f32.infinity.positive], rhs: [100, kValue.f32.infinity.positive], expected: true},
      { lhs: [0, kValue.f32.infinity.positive], rhs: [Number.NEGATIVE_INFINITY, kValue.f32.infinity.positive], expected: false},

      // Lower infinity
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [0], expected: true},
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [-1, 0], expected: true},
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [kValue.f32.negative.min, 0], expected: true},
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [0, 1], expected: false},
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [kValue.f32.infinity.negative, 0], expected: true},
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [kValue.f32.infinity.negative, -100 ], expected: true},
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: false},

      // Full infinity
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [0], expected: true},
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [-1, 0], expected: true},
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [0, 1], expected: true},
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [0, kValue.f32.infinity.positive], expected: true},
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [100, kValue.f32.infinity.positive], expected: true},
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [kValue.f32.infinity.negative, 0], expected: true},
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [kValue.f32.infinity.negative, -100 ], expected: true},
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: true},

      // Maximum f32 boundary
      { lhs: [0, kValue.f32.positive.max], rhs: [0], expected: true},
      { lhs: [0, kValue.f32.positive.max], rhs: [-1, 0], expected: false},
      { lhs: [0, kValue.f32.positive.max], rhs: [0, 1], expected: true},
      { lhs: [0, kValue.f32.positive.max], rhs: [0, kValue.f32.positive.max], expected: true},
      { lhs: [0, kValue.f32.positive.max], rhs: [0, kValue.f32.infinity.positive], expected: false},
      { lhs: [0, kValue.f32.positive.max], rhs: [100, kValue.f32.infinity.positive], expected: false},
      { lhs: [0, kValue.f32.positive.max], rhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: false},

      // Minimum f32 boundary
      { lhs: [kValue.f32.negative.min, 0], rhs: [0, 0], expected: true},
      { lhs: [kValue.f32.negative.min, 0], rhs: [-1, 0], expected: true},
      { lhs: [kValue.f32.negative.min, 0], rhs: [kValue.f32.negative.min, 0], expected: true},
      { lhs: [kValue.f32.negative.min, 0], rhs: [0, 1], expected: false},
      { lhs: [kValue.f32.negative.min, 0], rhs: [kValue.f32.infinity.negative, 0], expected: false},
      { lhs: [kValue.f32.negative.min, 0], rhs: [kValue.f32.infinity.negative, -100 ], expected: false},
      { lhs: [kValue.f32.negative.min, 0], rhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: false},

      // Out of range high
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [0], expected: true},
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [-1, 0], expected: false},
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [0, 1], expected: true},
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [0, kValue.f32.positive.max], expected: true},
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [0, kValue.f32.infinity.positive], expected: false},
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [100, kValue.f32.infinity.positive], expected: false},
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: false},

      // Out of range low
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [0], expected: true},
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [-1, 0], expected: true},
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [kValue.f32.negative.min, 0], expected: true},
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [0, 1], expected: false},
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [kValue.f32.infinity.negative, 0], expected: false},
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [kValue.f32.infinity.negative, -100 ], expected: false},
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: false},
    ]
  )
  .fn(t => {
    const lhs = new F32Interval(...t.params.lhs);
    const rhs = new F32Interval(...t.params.rhs);
    const expected = t.params.expected;

    const got = lhs.contains(rhs);
    t.expect(expected === got, `${lhs}.contains(${rhs}) returned ${got}. Expected ${expected}`);
  });

interface SpanCase {
  intervals: IntervalBounds[];
  expected: IntervalBounds;
}

g.test('span')
  .paramsSubcasesOnly<SpanCase>(
    // prettier-ignore
    [
      // Single Intervals
      { intervals: [[0, 10]], expected: [0, 10]},
      { intervals: [[0, kValue.f32.positive.max]], expected: [0, kValue.f32.positive.max]},
      { intervals: [[0, kValue.f32.positive.nearest_max]], expected: [0, kValue.f32.positive.nearest_max]},
      { intervals: [[0, kValue.f32.infinity.positive]], expected: [0, Number.POSITIVE_INFINITY]},
      { intervals: [[kValue.f32.negative.min, 0]], expected: [kValue.f32.negative.min, 0]},
      { intervals: [[kValue.f32.negative.nearest_min, 0]], expected: [kValue.f32.negative.nearest_min, 0]},
      { intervals: [[kValue.f32.infinity.negative, 0]], expected: [Number.NEGATIVE_INFINITY, 0]},

      // Double Intervals
      { intervals: [[0, 1], [2, 5]], expected: [0, 5]},
      { intervals: [[2, 5], [0, 1]], expected: [0, 5]},
      { intervals: [[0, 2], [1, 5]], expected: [0, 5]},
      { intervals: [[0, 5], [1, 2]], expected: [0, 5]},
      { intervals: [[kValue.f32.infinity.negative, 0], [0, kValue.f32.infinity.positive]], expected: kAny},

      // Multiple Intervals
      { intervals: [[0, 1], [2, 3], [4, 5]], expected: [0, 5]},
      { intervals: [[0, 1], [4, 5], [2, 3]], expected: [0, 5]},
      { intervals: [[0, 1], [0, 1], [0, 1]], expected: [0, 1]},
    ]
  )
  .fn(t => {
    const intervals = t.params.intervals.map(x => new F32Interval(...x));
    const expected = new F32Interval(...t.params.expected);

    const got = F32Interval.span(...intervals);
    t.expect(
      objectEquals(got, expected),
      `span({${intervals}}) returned ${got}. Expected ${expected}`
    );
  });

interface CorrectlyRoundedCase {
  value: number;
  expected: IntervalBounds;
}

g.test('correctlyRoundedInterval')
  .paramsSubcasesOnly<CorrectlyRoundedCase>(
    // prettier-ignore
    [
      // Edge Cases
      { value: kValue.f32.infinity.positive, expected: kAny },
      { value: kValue.f32.infinity.negative, expected: kAny },
      { value: kValue.f32.positive.max, expected: [kValue.f32.positive.max] },
      { value: kValue.f32.negative.min, expected: [kValue.f32.negative.min] },
      { value: kValue.f32.positive.min, expected: [kValue.f32.positive.min] },
      { value: kValue.f32.negative.max, expected: [kValue.f32.negative.max] },

      // 32-bit subnormals
      { value: kValue.f32.subnormal.positive.min, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: kValue.f32.subnormal.positive.max, expected: [0, kValue.f32.subnormal.positive.max] },
      { value: kValue.f32.subnormal.negative.min, expected: [kValue.f32.subnormal.negative.min, 0] },
      { value: kValue.f32.subnormal.negative.max, expected: [kValue.f32.subnormal.negative.max, 0] },

      // 64-bit subnormals
      { value: hexToF64(0x00000000, 0x00000001), expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x00000000, 0x00000002), expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x800fffff, 0xffffffff), expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: hexToF64(0x800fffff, 0xfffffffe), expected: [kValue.f32.subnormal.negative.max, 0] },

      // 32-bit normals
      { value: 0, expected: [0, 0] },
      { value: hexToF32(0x03800000), expected: [hexToF32(0x03800000)] },
      { value: hexToF32(0x03800001), expected: [hexToF32(0x03800001)] },
      { value: hexToF32(0x83800000), expected: [hexToF32(0x83800000)] },
      { value: hexToF32(0x83800001), expected: [hexToF32(0x83800001)] },

      // 64-bit normals
      { value: hexToF64(0x3ff00000, 0x00000001), expected: [hexToF32(0x3f800000), hexToF32(0x3f800001)] },
      { value: hexToF64(0x3ff00000, 0x00000002), expected: [hexToF32(0x3f800000), hexToF32(0x3f800001)] },
      { value: hexToF64(0x3ff00010, 0x00000010), expected: [hexToF32(0x3f800080), hexToF32(0x3f800081)] },
      { value: hexToF64(0x3ff00020, 0x00000020), expected: [hexToF32(0x3f800100), hexToF32(0x3f800101)] },
      { value: hexToF64(0xbff00000, 0x00000001), expected: [hexToF32(0xbf800001), hexToF32(0xbf800000)] },
      { value: hexToF64(0xbff00000, 0x00000002), expected: [hexToF32(0xbf800001), hexToF32(0xbf800000)] },
      { value: hexToF64(0xbff00010, 0x00000010), expected: [hexToF32(0xbf800081), hexToF32(0xbf800080)] },
      { value: hexToF64(0xbff00020, 0x00000020), expected: [hexToF32(0xbf800101), hexToF32(0xbf800100)] },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = correctlyRoundedInterval(t.params.value);
    t.expect(
      objectEquals(expected, got),
      `correctlyRoundedInterval(${t.params.value}) returned ${got}. Expected ${expected}`
    );
  });

interface AbsoluteErrorCase {
  value: number;
  error: number;
  expected: IntervalBounds;
}

g.test('absoluteErrorInterval')
  .paramsSubcasesOnly<AbsoluteErrorCase>(
    // prettier-ignore
    [
      // Edge Cases
      { value: kValue.f32.infinity.positive, error: 0, expected: kAny },
      { value: kValue.f32.infinity.positive, error: 2 ** -11, expected: kAny },
      { value: kValue.f32.infinity.positive, error: 1, expected: kAny },
      { value: kValue.f32.infinity.negative, error: 0, expected: kAny },
      { value: kValue.f32.infinity.negative, error: 2 ** -11, expected: kAny },
      { value: kValue.f32.infinity.negative, error: 1, expected: kAny },
      { value: kValue.f32.positive.max, error: 0, expected: [kValue.f32.positive.max] },
      { value: kValue.f32.positive.max, error: 2 ** -11, expected: [kValue.f32.positive.max] },
      { value: kValue.f32.positive.max, error: kValue.f32.positive.max, expected: kAny },
      { value: kValue.f32.positive.min, error: 0, expected: [kValue.f32.positive.min] },
      { value: kValue.f32.positive.min, error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: kValue.f32.positive.min, error: 1, expected: [-1, 1] },
      { value: kValue.f32.negative.min, error: 0, expected: [kValue.f32.negative.min] },
      { value: kValue.f32.negative.min, error: 2 ** -11, expected: [kValue.f32.negative.min] },
      { value: kValue.f32.negative.min, error: kValue.f32.positive.max, expected: kAny },
      { value: kValue.f32.negative.max, error: 0, expected: [kValue.f32.negative.max] },
      { value: kValue.f32.negative.max, error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: kValue.f32.negative.max, error: 1, expected: [-1, 1] },

      // 32-bit subnormals
      { value: kValue.f32.subnormal.positive.max, error: 0, expected: [0, kValue.f32.subnormal.positive.max] },
      { value: kValue.f32.subnormal.positive.max, error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: kValue.f32.subnormal.positive.max, error: 1, expected: [-1, 1] },
      { value: kValue.f32.subnormal.positive.min, error: 0, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: kValue.f32.subnormal.positive.min, error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: kValue.f32.subnormal.positive.min, error: 1, expected: [-1, 1] },
      { value: kValue.f32.subnormal.negative.min, error: 0, expected: [kValue.f32.subnormal.negative.min, 0] },
      { value: kValue.f32.subnormal.negative.min, error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: kValue.f32.subnormal.negative.min, error: 1, expected: [-1, 1] },
      { value: kValue.f32.subnormal.negative.max, error: 0, expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: kValue.f32.subnormal.negative.max, error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: kValue.f32.subnormal.negative.max, error: 1, expected: [-1, 1] },

      // 64-bit subnormals
      { value: hexToF64(0x00000000, 0x00000001), error: 0, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x00000000, 0x00000001), error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: hexToF64(0x00000000, 0x00000001), error: 1, expected: [-1, 1] },
      { value: hexToF64(0x00000000, 0x00000002), error: 0, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x00000000, 0x00000002), error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: hexToF64(0x00000000, 0x00000002), error: 1, expected: [-1, 1] },
      { value: hexToF64(0x800fffff, 0xffffffff), error: 0, expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: hexToF64(0x800fffff, 0xffffffff), error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: hexToF64(0x800fffff, 0xffffffff), error: 1, expected: [-1, 1] },
      { value: hexToF64(0x800fffff, 0xfffffffe), error: 0, expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: hexToF64(0x800fffff, 0xfffffffe), error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: hexToF64(0x800fffff, 0xfffffffe), error: 1, expected: [-1, 1] },

      // Zero
      { value: 0, error: 0, expected: [0] },
      { value: 0, error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: 0, error: 1, expected: [-1, 1] },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = absoluteErrorInterval(t.params.value, t.params.error);
    t.expect(
      objectEquals(expected, got),
      `absoluteErrorInterval(${t.params.value}, ${t.params.error}) returned ${got}. Expected ${expected}`
    );
  });

interface ULPCase {
  value: number;
  num_ulp: number;
  expected: IntervalBounds;
}

g.test('ulpInterval')
  .paramsSubcasesOnly<ULPCase>(
    // prettier-ignore
    [
      // Edge Cases
      { value: kValue.f32.infinity.positive, num_ulp: 0, expected: kAny },
      { value: kValue.f32.infinity.positive, num_ulp: 1, expected: kAny },
      { value: kValue.f32.infinity.positive, num_ulp: 4096, expected: kAny },
      { value: kValue.f32.infinity.negative, num_ulp: 0, expected: kAny },
      { value: kValue.f32.infinity.negative, num_ulp: 1, expected: kAny },
      { value: kValue.f32.infinity.negative, num_ulp: 4096, expected: kAny },
      { value: kValue.f32.positive.max, num_ulp: 0, expected: [kValue.f32.positive.max] },
      { value: kValue.f32.positive.max, num_ulp: 1, expected: kAny },
      { value: kValue.f32.positive.max, num_ulp: 4096, expected: kAny },
      { value: kValue.f32.positive.min, num_ulp: 0, expected: [kValue.f32.positive.min] },
      { value: kValue.f32.positive.min, num_ulp: 1, expected: [0, plusOneULP(kValue.f32.positive.min)] },
      { value: kValue.f32.positive.min, num_ulp: 4096, expected: [0, plusNULP(kValue.f32.positive.min, 4096)] },
      { value: kValue.f32.negative.min, num_ulp: 0, expected: [kValue.f32.negative.min] },
      { value: kValue.f32.negative.min, num_ulp: 1, expected: kAny },
      { value: kValue.f32.negative.min, num_ulp: 4096, expected: kAny },
      { value: kValue.f32.negative.max, num_ulp: 0, expected: [kValue.f32.negative.max] },
      { value: kValue.f32.negative.max, num_ulp: 1, expected: [minusOneULP(kValue.f32.negative.max), 0] },
      { value: kValue.f32.negative.max, num_ulp: 4096, expected: [minusNULP(kValue.f32.negative.max, 4096), 0] },

      // 32-bit subnormals
      { value: kValue.f32.subnormal.positive.max, num_ulp: 0, expected: [0, kValue.f32.subnormal.positive.max] },
      { value: kValue.f32.subnormal.positive.max, num_ulp: 1, expected: [minusOneULP(0), plusOneULP(kValue.f32.subnormal.positive.max)] },
      { value: kValue.f32.subnormal.positive.max, num_ulp: 4096, expected: [minusNULP(0, 4096), plusNULP(kValue.f32.subnormal.positive.max, 4096)] },
      { value: kValue.f32.subnormal.positive.min, num_ulp: 0, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: kValue.f32.subnormal.positive.min, num_ulp: 1, expected: [minusOneULP(0), plusOneULP(kValue.f32.subnormal.positive.min)] },
      { value: kValue.f32.subnormal.positive.min, num_ulp: 4096, expected: [minusNULP(0, 4096), plusNULP(kValue.f32.subnormal.positive.min, 4096)] },
      { value: kValue.f32.subnormal.negative.min, num_ulp: 0, expected: [kValue.f32.subnormal.negative.min, 0] },
      { value: kValue.f32.subnormal.negative.min, num_ulp: 1, expected: [minusOneULP(kValue.f32.subnormal.negative.min), plusOneULP(0)] },
      { value: kValue.f32.subnormal.negative.min, num_ulp: 4096, expected: [minusNULP(kValue.f32.subnormal.negative.min, 4096), plusNULP(0, 4096)] },
      { value: kValue.f32.subnormal.negative.max, num_ulp: 0, expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: kValue.f32.subnormal.negative.max, num_ulp: 1, expected: [minusOneULP(kValue.f32.subnormal.negative.max), plusOneULP(0)] },
      { value: kValue.f32.subnormal.negative.max, num_ulp: 4096, expected: [minusNULP(kValue.f32.subnormal.negative.max, 4096), plusNULP(0, 4096)] },

      // 64-bit subnormals
      { value: hexToF64(0x00000000, 0x00000001), num_ulp: 0, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x00000000, 0x00000001), num_ulp: 1, expected: [minusOneULP(0), plusOneULP(kValue.f32.subnormal.positive.min)] },
      { value: hexToF64(0x00000000, 0x00000001), num_ulp: 4096, expected: [minusNULP(0, 4096), plusNULP(kValue.f32.subnormal.positive.min, 4096)] },
      { value: hexToF64(0x00000000, 0x00000002), num_ulp: 0, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x00000000, 0x00000002), num_ulp: 1, expected: [minusOneULP(0), plusOneULP(kValue.f32.subnormal.positive.min)] },
      { value: hexToF64(0x00000000, 0x00000002), num_ulp: 4096, expected: [minusNULP(0, 4096), plusNULP(kValue.f32.subnormal.positive.min, 4096)] },
      { value: hexToF64(0x800fffff, 0xffffffff), num_ulp: 0, expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: hexToF64(0x800fffff, 0xffffffff), num_ulp: 1, expected: [minusOneULP(kValue.f32.subnormal.negative.max), plusOneULP(0)] },
      { value: hexToF64(0x800fffff, 0xffffffff), num_ulp: 4096, expected: [minusNULP(kValue.f32.subnormal.negative.max, 4096), plusNULP(0, 4096)] },
      { value: hexToF64(0x800fffff, 0xfffffffe), num_ulp: 0, expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: hexToF64(0x800fffff, 0xfffffffe), num_ulp: 1, expected: [minusOneULP(kValue.f32.subnormal.negative.max), plusOneULP(0)] },
      { value: hexToF64(0x800fffff, 0xfffffffe), num_ulp: 4096, expected: [minusNULP(kValue.f32.subnormal.negative.max, 4096), plusNULP(0, 4096)] },

      // Zero
      { value: 0, num_ulp: 0, expected: [0] },
      { value: 0, num_ulp: 1, expected: [minusOneULP(0), plusOneULP(0)] },
      { value: 0, num_ulp: 4096, expected: [minusNULP(0, 4096), plusNULP(0, 4096)] },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = ulpInterval(t.params.value, t.params.num_ulp);
    t.expect(
      objectEquals(expected, got),
      `ulpInterval(${t.params.value}, ${t.params.num_ulp}) returned ${got}. Expected ${expected}`
    );
  });

interface PointToIntervalCase {
  input: number;
  expected: IntervalBounds;
}

g.test('absInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // Common usages
      { input: 1, expected: [1] },
      { input: -1, expected: [1] },
      { input: 0.1, expected: [hexToF32(0x3dcccccc), hexToF32(0x3dcccccd)] },
      { input: -0.1, expected: [hexToF32(0x3dcccccc), hexToF32(0x3dcccccd)] },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAny },
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.positive.max, expected: [kValue.f32.positive.max] },
      { input: kValue.f32.positive.min, expected: [kValue.f32.positive.min] },
      { input: kValue.f32.negative.min, expected: [kValue.f32.positive.max] },
      { input: kValue.f32.negative.max, expected: [kValue.f32.positive.min] },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [0, kValue.f32.subnormal.positive.max] },
      { input: kValue.f32.subnormal.positive.min, expected: [0, kValue.f32.subnormal.positive.min] },
      { input: kValue.f32.subnormal.negative.min, expected: [0, kValue.f32.subnormal.positive.max] },
      { input: kValue.f32.subnormal.negative.max, expected: [0, kValue.f32.subnormal.positive.min] },

      // 64-bit subnormals
      { input: hexToF64(0x00000000, 0x00000001), expected: [0, kValue.f32.subnormal.positive.min] },
      { input: hexToF64(0x800fffff, 0xffffffff), expected: [0, kValue.f32.subnormal.positive.min] },

      // Zero
      { input: 0, expected: [0]},
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = absInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `absInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('acoshAlternativeInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form due to the inherited nature of the errors.
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.negative.min, expected: kAny },
      { input: -1, expected: kAny },
      { input: 0, expected: kAny },
      { input: 1, expected: kAny },  // 1/0 occurs in inverseSqrt in this formulation
      { input: 1.1, expected: [hexToF64(0x3fdc6368, 0x80000000), hexToF64(0x3fdc636f, 0x20000000)] },  // ~0.443..., differs from the primary in the later digits
      { input: 10, expected: [hexToF64(0x4007f21e, 0x40000000), hexToF64(0x4007f21f, 0x60000000)] },  // ~2.993...
      { input: kValue.f32.positive.max, expected: kAny },
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = acoshAlternativeInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `acoshInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('acoshPrimaryInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form due to the inherited nature of the errors.
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.negative.min, expected: kAny },
      { input: -1, expected: kAny },
      { input: 0, expected: kAny },
      { input: 1, expected: kAny },  // 1/0 occurs in inverseSqrt in this formulation
      { input: 1.1, expected: [hexToF64(0x3fdc6368, 0x20000000), hexToF64(0x3fdc636f, 0x80000000)] }, // ~0.443..., differs from the alternative in the later digits
      { input: 10, expected: [hexToF64(0x4007f21e, 0x40000000), hexToF64(0x4007f21f, 0x60000000)] },  // ~2.993...
      { input: kValue.f32.positive.max, expected: kAny },
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = acoshPrimaryInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `acoshInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('asinhInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form due to the inherited nature of the errors.
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.negative.min, expected: kAny },
      { input: -1, expected: [hexToF64(0xbfec343a, 0x80000000), hexToF64(0xbfec3432, 0x80000000)] },  // ~-0.88137...
      { input: 0, expected: [hexToF64(0xbeaa0000, 0x20000000), hexToF64(0x3eb1ffff, 0xd0000000)] },  // ~0
      { input: 1, expected: [hexToF64(0x3fec3435, 0x40000000), hexToF64(0x3fec3437, 0x80000000)] },  // ~0.88137...
      { input: kValue.f32.positive.max, expected: kAny },
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = asinhInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `asinhInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('atanInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: hexToF32(0xbfddb3d7), expected: [kValue.f32.negative.pi.third, plusOneULP(kValue.f32.negative.pi.third)] }, // x = -√3
      { input: -1, expected: [kValue.f32.negative.pi.quarter, plusOneULP(kValue.f32.negative.pi.quarter)] },
      { input: hexToF32(0xbf13cd3a), expected: [kValue.f32.negative.pi.sixth, plusOneULP(kValue.f32.negative.pi.sixth)] },  // x = -1/√3
      { input: 0, expected: [0] },
      { input: hexToF32(0x3f13cd3a), expected: [minusOneULP(kValue.f32.positive.pi.sixth), kValue.f32.positive.pi.sixth] },  // x = 1/√3
      { input: 1, expected: [minusOneULP(kValue.f32.positive.pi.quarter), kValue.f32.positive.pi.quarter] },
      { input: hexToF32(0x3fddb3d7), expected: [minusOneULP(kValue.f32.positive.pi.third), kValue.f32.positive.pi.third] }, // x = √3
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const error = (n: number): number => {
      return 4096 * oneULP(n);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = new F32Interval(...t.params.expected);

    const got = atanInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `atanInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('atanhInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form due to the inherited nature of the errors.
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.negative.min, expected: kAny },
      { input: -1, expected: kAny },
      { input: -0.1, expected: [hexToF64(0xbfb9af9a, 0x60000000), hexToF64(0xbfb9af8c, 0xc0000000)] },  // ~-0.1003...
      { input: 0, expected: [hexToF64(0xbe960000, 0x20000000), hexToF64(0x3e980000, 0x00000000)] },  // ~0
      { input: 0.1, expected: [hexToF64(0x3fb9af8b, 0x80000000), hexToF64(0x3fb9af9b, 0x00000000)] },  // ~0.1003...
      { input: 1, expected: kAny },
      { input: kValue.f32.positive.max, expected: kAny },
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = atanhInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `atanhInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('ceilInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: [0] },
      { input: 0.1, expected: [1] },
      { input: 0.9, expected: [1] },
      { input: 1.0, expected: [1] },
      { input: 1.1, expected: [2] },
      { input: 1.9, expected: [2] },
      { input: -0.1, expected: [0] },
      { input: -0.9, expected: [0] },
      { input: -1.0, expected: [-1] },
      { input: -1.1, expected: [-1] },
      { input: -1.9, expected: [-1] },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAny },
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.positive.max, expected: [kValue.f32.positive.max] },
      { input: kValue.f32.positive.min, expected: [1] },
      { input: kValue.f32.negative.min, expected: [kValue.f32.negative.min] },
      { input: kValue.f32.negative.max, expected: [0] },
      { input: kValue.powTwo.to30, expected: [kValue.powTwo.to30] },
      { input: -kValue.powTwo.to30, expected: [-kValue.powTwo.to30] },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [0, 1] },
      { input: kValue.f32.subnormal.positive.min, expected: [0, 1] },
      { input: kValue.f32.subnormal.negative.min, expected: [0] },
      { input: kValue.f32.subnormal.negative.max, expected: [0] },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = ceilInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `ceilInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('cosInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // This test does not include some common cases. i.e. f(x = π/2) = 0, because the difference between true x
      // and x as a f32 is sufficiently large, such that the high slope of f @ x causes the results to be substantially
      // different, so instead of getting 0 you get a value on the order of 10^-8 away from 0, thus difficult to express
      // in a human readable manner.
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.negative.min, expected: kAny },
      { input: kValue.f32.negative.pi.whole, expected: [-1, plusOneULP(-1)] },
      { input: kValue.f32.negative.pi.third, expected: [minusOneULP(1/2), 1/2] },
      { input: 0, expected: [1, 1] },
      { input: kValue.f32.positive.pi.third, expected: [minusOneULP(1/2), 1/2] },
      { input: kValue.f32.positive.pi.whole, expected: [-1, plusOneULP(-1)] },
      { input: kValue.f32.positive.max, expected: kAny },
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const error = (_: number): number => {
      return 2 ** -11;
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = new F32Interval(...t.params.expected);

    const got = cosInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `cosInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('coshInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form due to the inherited nature of the errors.
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.negative.min, expected: kAny },
      { input: -1, expected: [ hexToF32(0x3fc583a4), hexToF32(0x3fc583b1)] },  // ~1.1543...
      { input: 0, expected: [hexToF32(0x3f7ffffd), hexToF32(0x3f800002)] },  // ~1
      { input: 1, expected: [ hexToF32(0x3fc583a4), hexToF32(0x3fc583b1)] },  // ~1.1543...
      { input: kValue.f32.positive.max, expected: kAny },
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = coshInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `coshInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('degreesInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.negative.min, expected: kAny },
      { input: kValue.f32.negative.pi.whole, expected: [minusOneULP(-180), plusOneULP(-180)] },
      { input: kValue.f32.negative.pi.three_quarters, expected: [minusOneULP(-135), plusOneULP(-135)] },
      { input: kValue.f32.negative.pi.half, expected: [minusOneULP(-90), plusOneULP(-90)] },
      { input: kValue.f32.negative.pi.third, expected: [minusOneULP(-60), plusOneULP(-60)] },
      { input: kValue.f32.negative.pi.quarter, expected: [minusOneULP(-45), plusOneULP(-45)] },
      { input: kValue.f32.negative.pi.sixth, expected: [minusOneULP(-30), plusOneULP(-30)] },
      { input: 0, expected: [0] },
      { input: kValue.f32.positive.pi.sixth, expected: [minusOneULP(30), plusOneULP(30)] },
      { input: kValue.f32.positive.pi.quarter, expected: [minusOneULP(45), plusOneULP(45)] },
      { input: kValue.f32.positive.pi.third, expected: [minusOneULP(60), plusOneULP(60)] },
      { input: kValue.f32.positive.pi.half, expected: [minusOneULP(90), plusOneULP(90)] },
      { input: kValue.f32.positive.pi.three_quarters, expected: [minusOneULP(135), plusOneULP(135)] },
      { input: kValue.f32.positive.pi.whole, expected: [minusOneULP(180), plusOneULP(180)] },
      { input: kValue.f32.positive.max, expected: kAny },
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = degreesInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `degreesInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('expInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: 0, expected: [1] },
      { input: 1, expected: [kValue.f32.positive.e, plusOneULP(kValue.f32.positive.e)] },
      { input: 89, expected: kAny },
    ]
  )
  .fn(t => {
    const error = (x: number): number => {
      const n = 3 + 2 * Math.abs(t.params.input);
      return n * oneULP(x);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = new F32Interval(...t.params.expected);

    const got = expInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `expInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('exp2Interval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: 0, expected: [1] },
      { input: 1, expected: [2] },
      { input: 128, expected: kAny },
    ]
  )
  .fn(t => {
    const error = (x: number): number => {
      const n = 3 + 2 * Math.abs(t.params.input);
      return n * oneULP(x);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = new F32Interval(...t.params.expected);

    const got = exp2Interval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `exp2Interval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('floorInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: [0] },
      { input: 0.1, expected: [0] },
      { input: 0.9, expected: [0] },
      { input: 1.0, expected: [1] },
      { input: 1.1, expected: [1] },
      { input: 1.9, expected: [1] },
      { input: -0.1, expected: [-1] },
      { input: -0.9, expected: [-1] },
      { input: -1.0, expected: [-1] },
      { input: -1.1, expected: [-2] },
      { input: -1.9, expected: [-2] },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAny },
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.positive.max, expected: [kValue.f32.positive.max] },
      { input: kValue.f32.positive.min, expected: [0] },
      { input: kValue.f32.negative.min, expected: [kValue.f32.negative.min] },
      { input: kValue.f32.negative.max, expected: [-1] },
      { input: kValue.powTwo.to30, expected: [kValue.powTwo.to30] },
      { input: -kValue.powTwo.to30, expected: [-kValue.powTwo.to30] },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [0] },
      { input: kValue.f32.subnormal.positive.min, expected: [0] },
      { input: kValue.f32.subnormal.negative.min, expected: [-1, 0] },
      { input: kValue.f32.subnormal.negative.max, expected: [-1, 0] },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = floorInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `floorInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('fractInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: [0] },
      { input: 0.1, expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] }, // ~0.1
      { input: 0.9, expected: [hexToF32(0x3f666666), plusOneULP(hexToF32(0x3f666666))] },  // ~0.9
      { input: 1.0, expected: [0] },
      { input: 1.1, expected: [hexToF64(0x3fb99998, 0x00000000), hexToF64(0x3fb9999a, 0x00000000)] }, // ~0.1
      { input: -0.1, expected: [hexToF32(0x3f666666), plusOneULP(hexToF32(0x3f666666))] },  // ~0.9
      { input: -0.9, expected: [hexToF64(0x3fb99999, 0x00000000), hexToF64(0x3fb9999a, 0x00000000)] }, // ~0.1
      { input: -1.0, expected: [0] },
      { input: -1.1, expected: [hexToF64(0x3feccccc, 0xc0000000), hexToF64(0x3feccccd, 0x00000000), ] }, // ~0.9

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAny },
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.positive.max, expected: [0] },
      { input: kValue.f32.positive.min, expected: [kValue.f32.positive.min, kValue.f32.positive.min] },
      { input: kValue.f32.negative.min, expected: [0] },
      { input: kValue.f32.negative.max, expected: [kValue.f32.positive.less_than_one, 1.0] },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = fractInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `fractInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('inverseSqrtInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: -1, expected: kAny },
      { input: 0, expected: kAny },
      { input: 0.04, expected: [minusOneULP(5), plusOneULP(5)] },
      { input: 1, expected: [1] },
      { input: 100, expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: kValue.f32.positive.max, expected: [hexToF32(0x1f800000), plusNULP(hexToF32(0x1f800000), 2)] },  // ~5.421...e-20, i.e. 1/√max f32
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const error = (n: number): number => {
      return 2 * oneULP(n);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = new F32Interval(...t.params.expected);

    const got = inverseSqrtInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `inverseSqrtInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('lengthIntervalScalar')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form due to the inherited nature of the errors.
      {input: 0, expected: kAny },  // ~0
      {input: 1.0, expected: [hexToF64(0x3fefffff, 0x70000000), hexToF64(0x3ff00000, 0x90000000)] },  // ~1
      {input: -1.0, expected: [hexToF64(0x3fefffff, 0x70000000), hexToF64(0x3ff00000, 0x90000000)] },  // ~1
      {input: 0.1, expected: [hexToF64(0x3fb99998, 0x90000000), hexToF64(0x3fb9999a, 0x70000000)] },  // ~0.1
      {input: -0.1, expected: [hexToF64(0x3fb99998, 0x90000000), hexToF64(0x3fb9999a, 0x70000000)] },  // ~0.1
      {input: 10.0, expected: [hexToF64(0x4023ffff, 0x70000000), hexToF64(0x40240000, 0xb0000000)] },  // ~10
      {input: -10.0, expected: [hexToF64(0x4023ffff, 0x70000000), hexToF64(0x40240000, 0xb0000000)] },  // ~10

      // Subnormal Cases
      { input: kValue.f32.subnormal.negative.min, expected: kAny },
      { input: kValue.f32.subnormal.negative.max, expected: kAny },
      { input: kValue.f32.subnormal.positive.min, expected: kAny },
      { input: kValue.f32.subnormal.positive.max, expected: kAny },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAny },
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.negative.min, expected: kAny },
      { input: kValue.f32.negative.max, expected: kAny },
      { input: kValue.f32.positive.min, expected: kAny },
      { input: kValue.f32.positive.max, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = lengthInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `lengthInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('logInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: -1, expected: kAny },
      { input: 0, expected: kAny },
      { input: 1, expected: [0] },
      { input: kValue.f32.positive.e, expected: [minusOneULP(1), 1] },
      { input: kValue.f32.positive.max, expected: [minusOneULP(hexToF32(0x42b17218)), hexToF32(0x42b17218)] },  // ~88.72...
    ]
  )
  .fn(t => {
    const error = (n: number): number => {
      if (t.params.input >= 0.5 && t.params.input <= 2.0) {
        return 2 ** -21;
      }
      return 3 * oneULP(n);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = new F32Interval(...t.params.expected);

    const got = logInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `logInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('log2Interval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: -1, expected: kAny },
      { input: 0, expected: kAny },
      { input: 1, expected: [0] },
      { input: 2, expected: [1] },
      { input: kValue.f32.positive.max, expected: [minusOneULP(128), 128] },
    ]
  )
  .fn(t => {
    const error = (n: number): number => {
      if (t.params.input >= 0.5 && t.params.input <= 2.0) {
        return 2 ** -21;
      }
      return 3 * oneULP(n);
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = new F32Interval(...t.params.expected);

    const got = log2Interval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `log2Interval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('negationInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: [0] },
      { input: 0.1, expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] }, // ~-0.1
      { input: 1.0, expected: [-1.0] },
      { input: 1.9, expected: [hexToF32(0xbff33334), plusOneULP(hexToF32(0xbff33334))] },  // ~-1.9
      { input: -0.1, expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] }, // ~0.1
      { input: -1.0, expected: [1] },
      { input: -1.9, expected: [minusOneULP(hexToF32(0x3ff33334)), hexToF32(0x3ff33334)] },  // ~1.9

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAny },
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.positive.max, expected: [kValue.f32.negative.min] },
      { input: kValue.f32.positive.min, expected: [kValue.f32.negative.max] },
      { input: kValue.f32.negative.min, expected: [kValue.f32.positive.max] },
      { input: kValue.f32.negative.max, expected: [kValue.f32.positive.min] },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: kValue.f32.subnormal.positive.min, expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: kValue.f32.subnormal.negative.min, expected: [0, kValue.f32.subnormal.positive.max] },
      { input: kValue.f32.subnormal.negative.max, expected: [0, kValue.f32.subnormal.positive.min] },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = negationInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `negationInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('quantizeToF16Interval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.negative.min, expected: kAny },
      { input: kValue.f16.negative.min, expected: [kValue.f16.negative.min] },
      { input: -1, expected: [-1] },
      { input: -0.1, expected: [hexToF32(0xbdcce000), hexToF32(0xbdccc000)] },  // ~-0.1
      { input: kValue.f16.negative.max, expected: [kValue.f16.negative.max] },
      { input: kValue.f16.subnormal.negative.min, expected: [kValue.f16.subnormal.negative.min] },
      { input: kValue.f16.subnormal.negative.max, expected: [kValue.f16.subnormal.negative.max] },
      { input: kValue.f32.subnormal.negative.max, expected: [kValue.f16.subnormal.negative.max, 0] },
      { input: 0, expected: [0] },
      { input: kValue.f32.subnormal.positive.min, expected: [0, kValue.f16.subnormal.positive.min] },
      { input: kValue.f16.subnormal.positive.min, expected: [kValue.f16.subnormal.positive.min] },
      { input: kValue.f16.subnormal.positive.max, expected: [kValue.f16.subnormal.positive.max] },
      { input: kValue.f16.positive.min, expected: [kValue.f16.positive.min] },
      { input: 0.1, expected: [hexToF32(0x3dccc000), hexToF32(0x3dcce000)] },  // ~0.1
      { input: 1, expected: [1] },
      { input: kValue.f16.positive.max, expected: [kValue.f16.positive.max] },
      { input: kValue.f32.positive.max, expected: kAny },
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = quantizeToF16Interval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `quantizeToF16Interval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('radiansInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: -180, expected: [minusOneULP(kValue.f32.negative.pi.whole), plusOneULP(kValue.f32.negative.pi.whole)] },
      { input: -135, expected: [minusOneULP(kValue.f32.negative.pi.three_quarters), plusOneULP(kValue.f32.negative.pi.three_quarters)] },
      { input: -90, expected: [minusOneULP(kValue.f32.negative.pi.half), plusOneULP(kValue.f32.negative.pi.half)] },
      { input: -60, expected: [minusOneULP(kValue.f32.negative.pi.third), plusOneULP(kValue.f32.negative.pi.third)] },
      { input: -45, expected: [minusOneULP(kValue.f32.negative.pi.quarter), plusOneULP(kValue.f32.negative.pi.quarter)] },
      { input: -30, expected: [minusOneULP(kValue.f32.negative.pi.sixth), plusOneULP(kValue.f32.negative.pi.sixth)] },
      { input: 0, expected: [0] },
      { input: 30, expected: [minusOneULP(kValue.f32.positive.pi.sixth), plusOneULP(kValue.f32.positive.pi.sixth)] },
      { input: 45, expected: [minusOneULP(kValue.f32.positive.pi.quarter), plusOneULP(kValue.f32.positive.pi.quarter)] },
      { input: 60, expected: [minusOneULP(kValue.f32.positive.pi.third), plusOneULP(kValue.f32.positive.pi.third)] },
      { input: 90, expected: [minusOneULP(kValue.f32.positive.pi.half), plusOneULP(kValue.f32.positive.pi.half)] },
      { input: 135, expected: [minusOneULP(kValue.f32.positive.pi.three_quarters), plusOneULP(kValue.f32.positive.pi.three_quarters)] },
      { input: 180, expected: [minusOneULP(kValue.f32.positive.pi.whole), plusOneULP(kValue.f32.positive.pi.whole)] },
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = radiansInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `radiansInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('roundInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: [0] },
      { input: 0.1, expected: [0] },
      { input: 0.5, expected: [0] },  // Testing tie breaking
      { input: 0.9, expected: [1] },
      { input: 1.0, expected: [1] },
      { input: 1.1, expected: [1] },
      { input: 1.5, expected: [2] },  // Testing tie breaking
      { input: 1.9, expected: [2] },
      { input: -0.1, expected: [0] },
      { input: -0.5, expected: [0] },  // Testing tie breaking
      { input: -0.9, expected: [-1] },
      { input: -1.0, expected: [-1] },
      { input: -1.1, expected: [-1] },
      { input: -1.5, expected: [-2] },  // Testing tie breaking
      { input: -1.9, expected: [-2] },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAny },
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.positive.max, expected: [kValue.f32.positive.max] },
      { input: kValue.f32.positive.min, expected: [0] },
      { input: kValue.f32.negative.min, expected: [kValue.f32.negative.min] },
      { input: kValue.f32.negative.max, expected: [0] },
      { input: kValue.powTwo.to30, expected: [kValue.powTwo.to30] },
      { input: -kValue.powTwo.to30, expected: [-kValue.powTwo.to30] },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [0] },
      { input: kValue.f32.subnormal.positive.min, expected: [0] },
      { input: kValue.f32.subnormal.negative.min, expected: [0] },
      { input: kValue.f32.subnormal.negative.max, expected: [0] },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = roundInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `roundInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('saturateInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // Normals
      { input: 0, expected: [0] },
      { input: 1, expected: [1.0] },
      { input: -0.1, expected: [0] },
      { input: -1, expected: [0] },
      { input: -10, expected: [0] },
      { input: 0.1, expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: 10, expected: [1.0] },
      { input: 11.1, expected: [1.0] },
      { input: kValue.f32.positive.max, expected: [1.0] },
      { input: kValue.f32.positive.min, expected: [kValue.f32.positive.min] },
      { input: kValue.f32.negative.max, expected: [0.0] },
      { input: kValue.f32.negative.min, expected: [0.0] },

      // Subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [0.0, kValue.f32.subnormal.positive.max] },
      { input: kValue.f32.subnormal.positive.min, expected: [0.0, kValue.f32.subnormal.positive.min] },
      { input: kValue.f32.subnormal.negative.min, expected: [0.0] },
      { input: kValue.f32.subnormal.negative.max, expected: [0.0] },

      // Infinities
      { input: kValue.f32.infinity.positive, expected: kAny },
      { input: kValue.f32.infinity.negative, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = saturateInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `saturationInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('signInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.negative.min, expected: [-1] },
      { input: -10, expected: [-1] },
      { input: -1, expected: [-1] },
      { input: -0.1, expected: [-1] },
      { input: kValue.f32.negative.max, expected:  [-1] },
      { input: kValue.f32.subnormal.negative.min, expected: [-1, 0] },
      { input: kValue.f32.subnormal.negative.max, expected: [-1, 0] },
      { input: 0, expected: [0] },
      { input: kValue.f32.subnormal.positive.max, expected: [0, 1] },
      { input: kValue.f32.subnormal.positive.min, expected: [0, 1] },
      { input: kValue.f32.positive.min, expected: [1] },
      { input: 0.1, expected: [1] },
      { input: 1, expected: [1] },
      { input: 10, expected: [1] },
      { input: kValue.f32.positive.max, expected: [1] },
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = signInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `signInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('sinInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // This test does not include some common cases, i.e. f(x = -π|π) = 0, because the difference between true x and x
      // as a f32 is sufficiently large, such that the high slope of f @ x causes the results to be substantially
      // different, so instead of getting 0 you get a value on the order of 10^-8 away from it, thus difficult to
      // express in a human readable manner.
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.negative.min, expected: kAny },
      { input: kValue.f32.negative.pi.half, expected: [-1, plusOneULP(-1)] },
      { input: 0, expected: [0] },
      { input: kValue.f32.positive.pi.half, expected: [minusOneULP(1), 1] },
      { input: kValue.f32.positive.max, expected: kAny },
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const error = (_: number): number => {
      return 2 ** -11;
    };

    t.params.expected = applyError(t.params.expected, error);
    const expected = new F32Interval(...t.params.expected);

    const got = sinInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `sinInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('sinhInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form due to the inherited nature of the errors.
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.negative.min, expected: kAny },
      { input: -1, expected: [ hexToF32(0xbf966d05), hexToF32(0xbf966cf8)] },  // ~-1.175...
      { input: 0, expected: [hexToF32(0xb4600000), hexToF32(0x34600000)] },  // ~0
      { input: 1, expected: [ hexToF32(0x3f966cf8), hexToF32(0x3f966d05)] },  // ~1.175...
      { input: kValue.f32.positive.max, expected: kAny },
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = sinhInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `sinhInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('sqrtInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form due to the inherited nature of the errors.
      { input: -1, expected: kAny },
      { input: 0, expected: kAny },
      { input: 0.01, expected: [hexToF64(0x3fb99998, 0xb0000000), hexToF64(0x3fb9999a, 0x70000000)] },  // ~0.1
      { input: 1, expected: [hexToF64(0x3fefffff, 0x70000000), hexToF64(0x3ff00000, 0x90000000)] },  // ~1
      { input: 4, expected: [hexToF64(0x3fffffff, 0x70000000), hexToF64(0x40000000, 0x90000000)] },  // ~2
      { input: 100, expected: [hexToF64(0x4023ffff, 0x70000000), hexToF64(0x40240000, 0xb0000000)] },  // ~10
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = sqrtInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `sqrtInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('tanInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // All of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form. Some easy looking cases like f(x = -π|π) = 0 are actually quite difficult. This is because the interval
      // is calculated from the results of sin(x)/cos(x), which becomes very messy at x = -π|π, since π is irrational,
      // thus does not have an exact representation as a f32.
      // Even at 0, which has a precise f32 value, there is still the problem that result of sin(0) and cos(0) will be
      // intervals due to the inherited nature of errors, so the proper interval will be an interval calculated from
      // dividing an interval by another interval and applying an error function to that. This complexity is why the
      // entire interval framework was developed.
      // The examples here have been manually traced to confirm the expectation values are correct.
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.negative.min, expected: kAny },
      { input: kValue.f32.negative.pi.whole, expected: [hexToF64(0xbf4002bc, 0x90000000), hexToF64(0x3f400144, 0xf0000000)] },  // ~0.0
      { input: kValue.f32.negative.pi.half, expected: kAny },
      { input: 0, expected: [hexToF64(0xbf400200, 0xb0000000), hexToF64(0x3f400200, 0xb0000000)] },  // ~0.0
      { input: kValue.f32.positive.pi.half, expected: kAny },
      { input: kValue.f32.positive.pi.whole, expected: [hexToF64(0xbf400144, 0xf0000000), hexToF64(0x3f4002bc, 0x90000000)] },  // ~0.0
      { input: kValue.f32.positive.max, expected: kAny },
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = tanInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `tanInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('tanhInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form due to the inherited nature of the errors.
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.negative.min, expected: kAny },
      { input: -1, expected: [hexToF64(0xbfe85efd, 0x10000000), hexToF64(0xbfe85ef8, 0x90000000)] },  // ~-0.7615...
      { input: 0, expected: [hexToF64(0xbe8c0000, 0xb0000000), hexToF64(0x3e8c0000, 0xb0000000)] },  // ~0
      { input: 1, expected: [hexToF64(0x3fe85ef8, 0x90000000), hexToF64(0x3fe85efd, 0x10000000)] },  // ~0.7615...
      { input: kValue.f32.positive.max, expected: kAny },
      { input: kValue.f32.infinity.positive, expected: kAny },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = tanhInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `tanhInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('truncInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: [0] },
      { input: 0.1, expected: [0] },
      { input: 0.9, expected: [0] },
      { input: 1.0, expected: [1] },
      { input: 1.1, expected: [1] },
      { input: 1.9, expected: [1] },
      { input: -0.1, expected: [0] },
      { input: -0.9, expected: [0] },
      { input: -1.0, expected: [-1] },
      { input: -1.1, expected: [-1] },
      { input: -1.9, expected: [-1] },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: kAny },
      { input: kValue.f32.infinity.negative, expected: kAny },
      { input: kValue.f32.positive.max, expected: [kValue.f32.positive.max] },
      { input: kValue.f32.positive.min, expected: [0] },
      { input: kValue.f32.negative.min, expected: [kValue.f32.negative.min] },
      { input: kValue.f32.negative.max, expected: [0] },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [0] },
      { input: kValue.f32.subnormal.positive.min, expected: [0] },
      { input: kValue.f32.subnormal.negative.min, expected: [0] },
      { input: kValue.f32.subnormal.negative.max, expected: [0] },
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = truncInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `truncInterval(${t.params.input}) returned ${got}. Expected ${expected}`
    );
  });

interface BinaryToIntervalCase {
  // input is a pair of independent values, not an range, so should not be
  // converted to a F32Interval.
  input: [number, number];
  expected: IntervalBounds;
}

g.test('additionInterval')
  .paramsSubcasesOnly<BinaryToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: [0] },
      { input: [1, 0], expected: [1] },
      { input: [0, 1], expected: [1] },
      { input: [-1, 0], expected: [-1] },
      { input: [0, -1], expected: [-1] },
      { input: [1, 1], expected: [2] },
      { input: [1, -1], expected: [0] },
      { input: [-1, 1], expected: [0] },
      { input: [-1, -1], expected: [-2] },

      // 64-bit normals
      { input: [0.1, 0], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0, 0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [-0.1, 0], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [0, -0.1], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [0.1, 0.1], expected: [minusOneULP(hexToF32(0x3e4ccccd)), hexToF32(0x3e4ccccd)] },  // ~0.2
      { input: [0.1, -0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)) - hexToF32(0x3dcccccd), hexToF32(0x3dcccccd) - minusOneULP(hexToF32(0x3dcccccd))] }, // ~0
      { input: [-0.1, 0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)) - hexToF32(0x3dcccccd), hexToF32(0x3dcccccd) - minusOneULP(hexToF32(0x3dcccccd))] }, // ~0
      { input: [-0.1, -0.1], expected: [hexToF32(0xbe4ccccd), plusOneULP(hexToF32(0xbe4ccccd))] },  // ~-0.2

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
      { input: [0, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.positive, 0], expected: kAny},
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAny },
      { input: [0, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, 0], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAny },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = additionInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `additionInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

// Note: atan2's parameters are labelled (y, x) instead of (x, y)
g.test('atan2Interval')
  .paramsSubcasesOnly<BinaryToIntervalCase>(
    // prettier-ignore
    [
      // Not all of the common cases for each of the quadrants are iterated here, b/c π is not precisely expressible as
      // a f32, so fractions like 5* π/6 require special constants for each value. Positive x & y are tested using
      // values that already have constants, and then the other quadrants are spot checked.

      // positive y, positive x
      { input: [1, hexToF32(0x3fddb3d7)], expected: [minusOneULP(kValue.f32.positive.pi.sixth), kValue.f32.positive.pi.sixth] },  // x = √3
      { input: [1, 1], expected: [minusOneULP(kValue.f32.positive.pi.quarter), kValue.f32.positive.pi.quarter] },
      { input: [hexToF32(0x3fddb3d7), 1], expected: [minusOneULP(kValue.f32.positive.pi.third), kValue.f32.positive.pi.third] },  // y = √3
      { input: [Number.POSITIVE_INFINITY, 1], expected: kAny },

      // positive y, negative x
      { input: [1, -1], expected: [minusOneULP(kValue.f32.positive.pi.three_quarters), kValue.f32.positive.pi.three_quarters] },
      { input: [Number.POSITIVE_INFINITY, -1], expected: kAny },

      // negative y, negative x
      { input: [-1, -1], expected: [kValue.f32.negative.pi.three_quarters, plusOneULP(kValue.f32.negative.pi.three_quarters)] },
      { input: [Number.NEGATIVE_INFINITY, -1], expected: kAny },

      // negative y, positive x
      { input: [-1, 1], expected: [kValue.f32.negative.pi.quarter, plusOneULP(kValue.f32.negative.pi.quarter)] },
      { input: [Number.NEGATIVE_INFINITY, 1], expected: kAny },

      // Discontinuity @ y = 0
      { input: [0, 0], expected: kAny },
      { input: [0, kValue.f32.subnormal.positive.max], expected: kAny },
      { input: [0, kValue.f32.subnormal.negative.min], expected: kAny },
      { input: [0, kValue.f32.positive.min], expected: [kValue.f32.negative.pi.whole, kValue.f32.positive.pi.whole] },
      { input: [0, kValue.f32.negative.max], expected: [kValue.f32.negative.pi.whole, kValue.f32.positive.pi.whole] },
      { input: [0, kValue.f32.positive.max], expected: [kValue.f32.negative.pi.whole, kValue.f32.positive.pi.whole] },
      { input: [0, kValue.f32.negative.min], expected: [kValue.f32.negative.pi.whole, kValue.f32.positive.pi.whole] },
      { input: [0, kValue.f32.infinity.positive], expected: kAny },
      { input: [0, kValue.f32.infinity.negative], expected: kAny },
    ]
  )
  .fn(t => {
    const error = (n: number): number => {
      return 4096 * oneULP(n);
    };

    const [y, x] = t.params.input;
    t.params.expected = applyError(t.params.expected, error);
    const expected = new F32Interval(...t.params.expected);

    const got = atan2Interval(y, x);
    t.expect(
      objectEquals(expected, got),
      `atan2Interval(${y}, ${x}) returned ${got}. Expected ${expected}`
    );
  });

g.test('divisionInterval')
  .paramsSubcasesOnly<BinaryToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 1], expected: [0] },
      { input: [0, -1], expected: [0] },
      { input: [1, 1], expected: [1] },
      { input: [1, -1], expected: [-1] },
      { input: [-1, 1], expected: [-1] },
      { input: [-1, -1], expected: [1] },
      { input: [4, 2], expected: [2] },
      { input: [-4, 2], expected: [-2] },
      { input: [4, -2], expected: [-2] },
      { input: [-4, -2], expected: [2] },

      // 64-bit normals
      { input: [0, 0.1], expected: [0] },
      { input: [0, -0.1], expected: [0] },
      { input: [1, 0.1], expected: [minusOneULP(10), plusOneULP(10)] },
      { input: [-1, 0.1], expected: [minusOneULP(-10), plusOneULP(-10)] },
      { input: [1, -0.1], expected: [minusOneULP(-10), plusOneULP(-10)] },
      { input: [-1, -0.1], expected: [minusOneULP(10), plusOneULP(10)] },

      // Denominator out of range
      { input: [1, kValue.f32.infinity.positive], expected: kAny },
      { input: [1, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAny },
      { input: [1, kValue.f32.positive.max], expected: kAny },
      { input: [1, kValue.f32.negative.min], expected: kAny },
      { input: [1, 0], expected: kAny },
      { input: [1, kValue.f32.subnormal.positive.max], expected: kAny },
    ]
  )
  .fn(t => {
    const error = (n: number): number => {
      return 2.5 * oneULP(n);
    };

    const [x, y] = t.params.input;
    t.params.expected = applyError(t.params.expected, error);
    const expected = new F32Interval(...t.params.expected);

    const got = divisionInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `divisionInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('ldexpInterval')
  .paramsSubcasesOnly<BinaryToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: [0] },
      { input: [0, 1], expected: [0] },
      { input: [0, -1], expected: [0] },
      { input: [1, 1], expected: [2] },
      { input: [1, -1], expected: [0.5] },
      { input: [-1, 1], expected: [-2] },
      { input: [-1, -1], expected: [-0.5] },

      // 64-bit normals
      { input: [0, 0.1], expected: [0] },
      { input: [0, -0.1], expected: [0] },
      { input: [1.0000000001, 1], expected: [2, plusNULP(2, 2)] },  // ~2, additional ULP error due to first param not being f32 precise
      { input: [-1.0000000001, 1], expected: [minusNULP(-2, 2), -2] },  // ~-2, additional ULP error due to first param not being f32 precise

      // Edge Cases
      { input: [1.9999998807907104, 127], expected: [kValue.f32.positive.max] },
      { input: [1, -126], expected: [kValue.f32.positive.min] },
      { input: [0.9999998807907104, -126], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [1.1920928955078125e-07, -126], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [-1.1920928955078125e-07, -126], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [-0.9999998807907104, -126], expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: [-1, -126], expected: [kValue.f32.negative.max] },
      { input: [-1.9999998807907104, 127], expected: [kValue.f32.negative.min] },

      // Out of Bounds
      { input: [1, 128], expected: kAny },
      { input: [-1, 128], expected: kAny },
      { input: [100, 126], expected: kAny },
      { input: [-100, 126], expected: kAny },
      { input: [kValue.f32.positive.max, kValue.i32.positive.max], expected: kAny },
      { input: [kValue.f32.negative.min, kValue.i32.positive.max], expected: kAny },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = ldexpInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `divisionInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('maxInterval')
  .paramsSubcasesOnly<BinaryToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: [0] },
      { input: [1, 0], expected: [1] },
      { input: [0, 1], expected: [1] },
      { input: [-1, 0], expected: [0] },
      { input: [0, -1], expected: [0] },
      { input: [1, 1], expected: [1] },
      { input: [1, -1], expected: [1] },
      { input: [-1, 1], expected: [1] },
      { input: [-1, -1], expected: [-1] },

      // 64-bit normals
      { input: [0.1, 0], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0, 0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [-0.1, 0], expected: [0] },
      { input: [0, -0.1], expected: [0] },
      { input: [0.1, 0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0.1, -0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [-0.1, 0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [-0.1, -0.1], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1

      // 32-bit normals
      { input: [kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.min, 0], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, kValue.f32.subnormal.positive.min], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [kValue.f32.subnormal.negative.max, 0], expected: [0] },
      { input: [0, kValue.f32.subnormal.negative.max], expected: [0] },
      { input: [kValue.f32.subnormal.negative.min, 0], expected: [0] },
      { input: [0, kValue.f32.subnormal.negative.min], expected: [0] },

      // Infinities
      { input: [0, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.positive, 0], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAny },
      { input: [0, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, 0], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAny },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = maxInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `maxInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('minInterval')
  .paramsSubcasesOnly<BinaryToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: [0] },
      { input: [1, 0], expected: [0] },
      { input: [0, 1], expected: [0] },
      { input: [-1, 0], expected: [-1] },
      { input: [0, -1], expected: [-1] },
      { input: [1, 1], expected: [1] },
      { input: [1, -1], expected: [-1] },
      { input: [-1, 1], expected: [-1] },
      { input: [-1, -1], expected: [-1] },

      // 64-bit normals
      { input: [0.1, 0], expected: [0] },
      { input: [0, 0.1], expected: [0] },
      { input: [-0.1, 0], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [0, -0.1], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [0.1, 0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0.1, -0.1], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [-0.1, 0.1], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [-0.1, -0.1], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1

      // 32-bit normals
      { input: [kValue.f32.subnormal.positive.max, 0], expected: [0] },
      { input: [0, kValue.f32.subnormal.positive.max], expected: [0] },
      { input: [kValue.f32.subnormal.positive.min, 0], expected: [0] },
      { input: [0, kValue.f32.subnormal.positive.min], expected: [0] },
      { input: [kValue.f32.subnormal.negative.max, 0], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [0, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [kValue.f32.subnormal.negative.min, 0], expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: [0, kValue.f32.subnormal.negative.min], expected: [kValue.f32.subnormal.negative.min, 0] },

      // Infinities
      { input: [0, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.positive, 0], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAny },
      { input: [0, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, 0], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAny },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = minInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `minInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('multiplicationInterval')
  .paramsSubcasesOnly<BinaryToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: [0] },
      { input: [1, 0], expected: [0] },
      { input: [0, 1], expected: [0] },
      { input: [-1, 0], expected: [0] },
      { input: [0, -1], expected: [0] },
      { input: [1, 1], expected: [1] },
      { input: [1, -1], expected: [-1] },
      { input: [-1, 1], expected: [-1] },
      { input: [-1, -1], expected: [1] },
      { input: [2, 1], expected: [2] },
      { input: [1, -2], expected: [-2] },
      { input: [-2, 1], expected: [-2] },
      { input: [-2, -1], expected: [2] },
      { input: [2, 2], expected: [4] },
      { input: [2, -2], expected: [-4] },
      { input: [-2, 2], expected: [-4] },
      { input: [-2, -2], expected: [4] },

      // 64-bit normals
      { input: [0.1, 0], expected: [0] },
      { input: [0, 0.1], expected: [0] },
      { input: [-0.1, 0], expected: [0] },
      { input: [0, -0.1], expected: [0] },
      { input: [0.1, 0.1], expected: [minusNULP(hexToF32(0x3c23d70a), 2), plusOneULP(hexToF32(0x3c23d70a))] },  // ~0.01
      { input: [0.1, -0.1], expected: [minusOneULP(hexToF32(0xbc23d70a)), plusNULP(hexToF32(0xbc23d70a), 2)] },  // ~-0.01
      { input: [-0.1, 0.1], expected: [minusOneULP(hexToF32(0xbc23d70a)), plusNULP(hexToF32(0xbc23d70a), 2)] },  // ~-0.01
      { input: [-0.1, -0.1], expected: [minusNULP(hexToF32(0x3c23d70a), 2), plusOneULP(hexToF32(0x3c23d70a))] },  // ~0.01

      // Infinities
      { input: [0, kValue.f32.infinity.positive], expected: kAny },
      { input: [1, kValue.f32.infinity.positive], expected: kAny },
      { input: [-1, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAny },
      { input: [0, kValue.f32.infinity.negative], expected: kAny },
      { input: [1, kValue.f32.infinity.negative], expected: kAny },
      { input: [-1, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAny },

      // Edge of f32
      { input: [kValue.f32.positive.max, kValue.f32.positive.max], expected: kAny },
      { input: [kValue.f32.negative.min, kValue.f32.negative.min], expected: kAny },
      { input: [kValue.f32.positive.max, kValue.f32.negative.min], expected: kAny },
      { input: [kValue.f32.negative.min, kValue.f32.positive.max], expected: kAny },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = multiplicationInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `multiplicationInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('remainderInterval')
  .paramsSubcasesOnly<BinaryToIntervalCase>(
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
      { input: [1, kValue.f32.infinity.positive], expected: kAny },
      { input: [1, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAny },
      { input: [1, kValue.f32.positive.max], expected: kAny },
      { input: [1, kValue.f32.negative.min], expected: kAny },
      { input: [1, 0], expected: kAny },
      { input: [1, kValue.f32.subnormal.positive.max], expected: kAny },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = remainderInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `remainderInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('powInterval')
  .paramsSubcasesOnly<BinaryToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form due to the inherited nature of the errors.
      { input: [-1, 0], expected: kAny },
      { input: [0, 0], expected: kAny },
      { input: [1, 0], expected: [minusNULP(1, 3), hexToF64(0x3ff00000, 0x30000000)] },  // ~1
      { input: [2, 0], expected: [minusNULP(1, 3), hexToF64(0x3ff00000, 0x30000000)] },  // ~1
      { input: [kValue.f32.positive.max, 0], expected: [minusNULP(1, 3), hexToF64(0x3ff00000, 0x30000000)] },  // ~1
      { input: [0, 1], expected: kAny },
      { input: [1, 1], expected: [hexToF64(0x3feffffe, 0xdffffe00), hexToF64(0x3ff00000, 0xc0000200)] },  // ~1
      { input: [1, 100], expected: [hexToF64(0x3fefffba, 0x3fff3800), hexToF64(0x3ff00023, 0x2000c800)] },  // ~1
      { input: [1, kValue.f32.positive.max], expected: kAny },
      { input: [2, 1], expected: [hexToF64(0x3ffffffe, 0xa0000200), hexToF64(0x40000001, 0x00000200)] },  // ~2
      { input: [2, 2], expected: [hexToF64(0x400ffffd, 0xa0000400), hexToF64(0x40100001, 0xa0000400)] },  // ~4
      { input: [10, 10], expected: [hexToF64(0x4202a04f, 0x51f77000), hexToF64(0x4202a070, 0xee08e000)] },  // ~10000000000
      { input: [10, 1], expected: [hexToF64(0x4023fffe, 0x0b658b00), hexToF64(0x40240002, 0x149a7c00)] },  // ~10
      { input: [kValue.f32.positive.max, 1], expected: kAny },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = powInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `powInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('stepInterval')
  .paramsSubcasesOnly<BinaryToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: [1] },
      { input: [1, 1], expected: [1] },
      { input: [0, 1], expected: [1] },
      { input: [1, 0], expected: [0] },
      { input: [-1, -1], expected: [1] },
      { input: [0, -1], expected: [0] },
      { input: [-1, 0], expected: [1] },
      { input: [-1, 1], expected: [1] },
      { input: [1, -1], expected: [0] },

      // 64-bit normals
      { input: [0.1, 0.1], expected: [0, 1] },
      { input: [0, 0.1], expected: [1] },
      { input: [0.1, 0], expected: [0] },
      { input: [0.1, 1], expected: [1] },
      { input: [1, 0.1], expected: [0] },
      { input: [-0.1, -0.1], expected: [0, 1] },
      { input: [0, -0.1], expected: [0] },
      { input: [-0.1, 0], expected: [1] },
      { input: [-0.1, -1], expected: [0] },
      { input: [-1, -0.1], expected: [1] },

      // Subnormals
      { input: [0, kValue.f32.subnormal.positive.max], expected: [1] },
      { input: [0, kValue.f32.subnormal.positive.min], expected: [1] },
      { input: [0, kValue.f32.subnormal.negative.max], expected: [0, 1] },
      { input: [0, kValue.f32.subnormal.negative.min], expected: [0, 1] },
      { input: [1, kValue.f32.subnormal.positive.max], expected: [0] },
      { input: [1, kValue.f32.subnormal.positive.min], expected: [0] },
      { input: [1, kValue.f32.subnormal.negative.max], expected: [0] },
      { input: [1, kValue.f32.subnormal.negative.min], expected: [0] },
      { input: [-1, kValue.f32.subnormal.positive.max], expected: [1] },
      { input: [-1, kValue.f32.subnormal.positive.min], expected: [1] },
      { input: [-1, kValue.f32.subnormal.negative.max], expected: [1] },
      { input: [-1, kValue.f32.subnormal.negative.min], expected: [1] },
      { input: [kValue.f32.subnormal.positive.max, 0], expected: [0, 1] },
      { input: [kValue.f32.subnormal.positive.min, 0], expected: [0, 1] },
      { input: [kValue.f32.subnormal.negative.max, 0], expected: [1] },
      { input: [kValue.f32.subnormal.negative.min, 0], expected: [1] },
      { input: [kValue.f32.subnormal.positive.max, 1], expected: [1] },
      { input: [kValue.f32.subnormal.positive.min, 1], expected: [1] },
      { input: [kValue.f32.subnormal.negative.max, 1], expected: [1] },
      { input: [kValue.f32.subnormal.negative.min, 1], expected: [1] },
      { input: [kValue.f32.subnormal.positive.max, -1], expected: [0] },
      { input: [kValue.f32.subnormal.positive.min, -1], expected: [0] },
      { input: [kValue.f32.subnormal.negative.max, -1], expected: [0] },
      { input: [kValue.f32.subnormal.negative.min, -1], expected: [0] },
      { input: [kValue.f32.subnormal.negative.min, kValue.f32.subnormal.positive.max], expected: [1] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.min], expected: [0, 1] },

      // Infinities
      { input: [0, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.positive, 0], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAny },
      { input: [0, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, 0], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAny },
    ]
  )
  .fn(t => {
    const [edge, x] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = stepInterval(edge, x);
    t.expect(
      objectEquals(expected, got),
      `stepInterval(${edge}, ${x}) returned ${got}. Expected ${expected}`
    );
  });

g.test('subtractionInterval')
  .paramsSubcasesOnly<BinaryToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: [0] },
      { input: [1, 0], expected: [1] },
      { input: [0, 1], expected: [-1] },
      { input: [-1, 0], expected: [-1] },
      { input: [0, -1], expected: [1] },
      { input: [1, 1], expected: [0] },
      { input: [1, -1], expected: [2] },
      { input: [-1, 1], expected: [-2] },
      { input: [-1, -1], expected: [0] },

      // 64-bit normals
      { input: [0.1, 0], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0, 0.1], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [-0.1, 0], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [0, -0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0.1, 0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)) - hexToF32(0x3dcccccd), hexToF32(0x3dcccccd) - minusOneULP(hexToF32(0x3dcccccd))] },  // ~0.0
      { input: [0.1, -0.1], expected: [minusOneULP(hexToF32(0x3e4ccccd)), hexToF32(0x3e4ccccd)] }, // ~0.2
      { input: [-0.1, 0.1], expected: [hexToF32(0xbe4ccccd), plusOneULP(hexToF32(0xbe4ccccd))] },  // ~-0.2
      { input: [-0.1, -0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)) - hexToF32(0x3dcccccd), hexToF32(0x3dcccccd) - minusOneULP(hexToF32(0x3dcccccd))] }, // ~0

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
      { input: [0, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.positive, 0], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAny },
      { input: [0, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, 0], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.negative], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAny },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = subtractionInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `subtractionInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

interface TernaryToIntervalCase {
  input: [number, number, number];
  expected: IntervalBounds;
}

g.test('clampMedianInterval')
  .paramsSubcasesOnly<TernaryToIntervalCase>(
    // prettier-ignore
    [
      // Normals
      { input: [0, 0, 0], expected: [0] },
      { input: [1, 0, 0], expected: [0] },
      { input: [0, 1, 0], expected: [0] },
      { input: [0, 0, 1], expected: [0] },
      { input: [1, 0, 1], expected: [1] },
      { input: [1, 1, 0], expected: [1] },
      { input: [0, 1, 1], expected: [1] },
      { input: [1, 1, 1], expected: [1] },
      { input: [1, 10, 100], expected: [10] },
      { input: [10, 1, 100], expected: [10] },
      { input: [100, 1, 10], expected: [10] },
      { input: [-10, 1, 100], expected: [1] },
      { input: [10, 1, -100], expected: [1] },
      { input: [-10, 1, -100], expected: [-10] },
      { input: [-10, -10, -10], expected: [-10] },

      // Subnormals
      { input: [kValue.f32.subnormal.positive.max, 0, 0], expected: [0] },
      { input: [0, kValue.f32.subnormal.positive.max, 0], expected: [0] },
      { input: [0, 0, kValue.f32.subnormal.positive.max], expected: [0] },
      { input: [kValue.f32.subnormal.positive.max, 0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.negative.max], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [kValue.f32.positive.max, kValue.f32.positive.max, kValue.f32.subnormal.positive.min], expected: [kValue.f32.positive.max] },

      // Infinities
      { input: [0, 1, kValue.f32.infinity.positive], expected: kAny },
      { input: [0, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAny },
    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = clampMedianInterval(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `clampMedianInterval(${x}, ${y}, ${z}) returned ${got}. Expected ${expected}`
    );
  });

g.test('clampMinMaxInterval')
  .paramsSubcasesOnly<TernaryToIntervalCase>(
    // prettier-ignore
    [
      // Normals
      { input: [0, 0, 0], expected: [0] },
      { input: [1, 0, 0], expected: [0] },
      { input: [0, 1, 0], expected: [0] },
      { input: [0, 0, 1], expected: [0] },
      { input: [1, 0, 1], expected: [1] },
      { input: [1, 1, 0], expected: [0] },
      { input: [0, 1, 1], expected: [1] },
      { input: [1, 1, 1], expected: [1] },
      { input: [1, 10, 100], expected: [10] },
      { input: [10, 1, 100], expected: [10] },
      { input: [100, 1, 10], expected: [10] },
      { input: [-10, 1, 100], expected: [1] },
      { input: [10, 1, -100], expected: [-100] },
      { input: [-10, 1, -100], expected: [-100] },
      { input: [-10, -10, -10], expected: [-10] },

      // Subnormals
      { input: [kValue.f32.subnormal.positive.max, 0, 0], expected: [0] },
      { input: [0, kValue.f32.subnormal.positive.max, 0], expected: [0] },
      { input: [0, 0, kValue.f32.subnormal.positive.max], expected: [0] },
      { input: [kValue.f32.subnormal.positive.max, 0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, 0], expected: [0] },
      { input: [0, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [kValue.f32.positive.max, kValue.f32.positive.max, kValue.f32.subnormal.positive.min], expected: [0, kValue.f32.subnormal.positive.min] },

      // Infinities
      { input: [0, 1, kValue.f32.infinity.positive], expected: kAny },
      { input: [0, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.positive], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, kValue.f32.infinity.negative], expected: kAny },
    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = clampMinMaxInterval(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `clampMinMaxInterval(${x}, ${y}, ${z}) returned ${got}. Expected ${expected}`
    );
  });

g.test('mixImpreciseInterval')
  .paramsSubcasesOnly<TernaryToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form due to the inherited nature of the errors.
      // [0.0, 1.0] cases
      { input: [0.0, 1.0, -1.0], expected: [-1.0] },
      { input: [0.0, 1.0, 0.0], expected: [0.0] },
      { input: [0.0, 1.0, 0.1], expected: [hexToF64(0x3fb99999,0x80000000), hexToF64(0x3fb99999,0xa0000000)] },  // ~0.1
      { input: [0.0, 1.0, 0.5], expected: [0.5] },
      { input: [0.0, 1.0, 0.9], expected: [hexToF64(0x3feccccc,0xc0000000), hexToF64(0x3feccccc,0xe0000000)] },  // ~0.9
      { input: [0.0, 1.0, 1.0], expected: [1.0] },
      { input: [0.0, 1.0, 2.0], expected: [2.0] },

      // [1.0, 0.0] cases
      { input: [1.0, 0.0, -1.0], expected: [2.0] },
      { input: [1.0, 0.0, 0.0], expected: [1.0] },
      { input: [1.0, 0.0, 0.1], expected: [hexToF64(0x3feccccc,0xc0000000), hexToF64(0x3feccccc,0xe0000000)] },  // ~0.9
      { input: [1.0, 0.0, 0.5], expected: [0.5] },
      { input: [1.0, 0.0, 0.9], expected: [hexToF64(0x3fb99999,0x00000000), hexToF64(0x3fb9999a,0x00000000)] },  // ~0.1
      { input: [1.0, 0.0, 1.0], expected: [0.0] },
      { input: [1.0, 0.0, 2.0], expected: [-1.0] },

      // [0.0, 10.0] cases
      { input: [0.0, 10.0, -1.0], expected: [-10.0] },
      { input: [0.0, 10.0, 0.0], expected: [0.0] },
      { input: [0.0, 10.0, 0.1], expected: [hexToF64(0x3fefffff,0xe0000000), hexToF64(0x3ff00000,0x20000000)] },  // ~1
      { input: [0.0, 10.0, 0.5], expected: [5.0] },
      { input: [0.0, 10.0, 0.9], expected: [hexToF64(0x4021ffff,0xe0000000), hexToF64(0x40220000,0x20000000)] },  // ~9
      { input: [0.0, 10.0, 1.0], expected: [10.0] },
      { input: [0.0, 10.0, 2.0], expected: [20.0] },

      // [2.0, 10.0] cases
      { input: [2.0, 10.0, -1.0], expected: [-6.0] },
      { input: [2.0, 10.0, 0.0], expected: [2.0] },
      { input: [2.0, 10.0, 0.1], expected: [hexToF64(0x40066666,0x60000000), hexToF64(0x40066666,0x80000000)] },  // ~2.8
      { input: [2.0, 10.0, 0.5], expected: [6.0] },
      { input: [2.0, 10.0, 0.9], expected: [hexToF64(0x40226666,0x60000000), hexToF64(0x40226666,0x80000000)] },  // ~9.2
      { input: [2.0, 10.0, 1.0], expected: [10.0] },
      { input: [2.0, 10.0, 2.0], expected: [18.0] },

      // [-1.0, 1.0] cases
      { input: [-1.0, 1.0, -2.0], expected: [-5.0] },
      { input: [-1.0, 1.0, 0.0], expected: [-1.0] },
      { input: [-1.0, 1.0, 0.1], expected: [hexToF64(0xbfe99999,0xa0000000), hexToF64(0xbfe99999,0x80000000)] },  // ~-0.8
      { input: [-1.0, 1.0, 0.5], expected: [0.0] },
      { input: [-1.0, 1.0, 0.9], expected: [hexToF64(0x3fe99999,0x80000000), hexToF64(0x3fe99999,0xc0000000)] },  // ~0.8
      { input: [-1.0, 1.0, 1.0], expected: [1.0] },
      { input: [-1.0, 1.0, 2.0], expected: [3.0] },

      // Infinities
      { input: [0.0, kValue.f32.infinity.positive, 0.5], expected: kAny },
      { input: [kValue.f32.infinity.positive, 0.0, 0.5], expected: kAny },
      { input: [kValue.f32.infinity.negative, 1.0, 0.5], expected: kAny },
      { input: [1.0, kValue.f32.infinity.negative, 0.5], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, 0.5], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative, 0.5], expected: kAny },
      { input: [0.0, 1.0, kValue.f32.infinity.negative], expected: kAny },
      { input: [1.0, 0.0, kValue.f32.infinity.negative], expected: kAny },
      { input: [0.0, 1.0, kValue.f32.infinity.positive], expected: kAny },
      { input: [1.0, 0.0, kValue.f32.infinity.positive], expected: kAny },

      // Showing how precise and imprecise versions diff
      { input: [kValue.f32.negative.min, 10.0, 1.0], expected: [0.0]},
    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = mixImpreciseInterval(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `mixImpreciseInterval(${x}, ${y}, ${z}) returned ${got}. Expected ${expected}`
    );
  });

g.test('mixPreciseInterval')
  .paramsSubcasesOnly<TernaryToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form due to the inherited nature of the errors.
      // [0.0, 1.0] cases
      { input: [0.0, 1.0, -1.0], expected: [-1.0] },
      { input: [0.0, 1.0, 0.0], expected: [0.0] },
      { input: [0.0, 1.0, 0.1], expected: [hexToF64(0x3fb99999,0x80000000), hexToF64(0x3fb99999,0xa0000000)] },  // ~0.1
      { input: [0.0, 1.0, 0.5], expected: [0.5] },
      { input: [0.0, 1.0, 0.9], expected: [hexToF64(0x3feccccc,0xc0000000), hexToF64(0x3feccccc,0xe0000000)] },  // ~0.9
      { input: [0.0, 1.0, 1.0], expected: [1.0] },
      { input: [0.0, 1.0, 2.0], expected: [2.0] },

      // [1.0, 0.0] cases
      { input: [1.0, 0.0, -1.0], expected: [2.0] },
      { input: [1.0, 0.0, 0.0], expected: [1.0] },
      { input: [1.0, 0.0, 0.1], expected: [hexToF64(0x3feccccc,0xc0000000), hexToF64(0x3feccccc,0xe0000000)] },  // ~0.9
      { input: [1.0, 0.0, 0.5], expected: [0.5] },
      { input: [1.0, 0.0, 0.9], expected: [hexToF64(0x3fb99999,0x00000000), hexToF64(0x3fb9999a,0x00000000)] },  // ~0.1
      { input: [1.0, 0.0, 1.0], expected: [0.0] },
      { input: [1.0, 0.0, 2.0], expected: [-1.0] },

      // [0.0, 10.0] cases
      { input: [0.0, 10.0, -1.0], expected: [-10.0] },
      { input: [0.0, 10.0, 0.0], expected: [0.0] },
      { input: [0.0, 10.0, 0.1], expected: [hexToF64(0x3fefffff,0xe0000000), hexToF64(0x3ff00000,0x20000000)] },  // ~1
      { input: [0.0, 10.0, 0.5], expected: [5.0] },
      { input: [0.0, 10.0, 0.9], expected: [hexToF64(0x4021ffff,0xe0000000), hexToF64(0x40220000,0x20000000)] },  // ~9
      { input: [0.0, 10.0, 1.0], expected: [10.0] },
      { input: [0.0, 10.0, 2.0], expected: [20.0] },

      // [2.0, 10.0] cases
      { input: [2.0, 10.0, -1.0], expected: [-6.0] },
      { input: [2.0, 10.0, 0.0], expected: [2.0] },
      { input: [2.0, 10.0, 0.1], expected: [hexToF64(0x40066666,0x40000000), hexToF64(0x40066666,0x80000000)] },  // ~2.8
      { input: [2.0, 10.0, 0.5], expected: [6.0] },
      { input: [2.0, 10.0, 0.9], expected: [hexToF64(0x40226666,0x40000000), hexToF64(0x40226666,0xa0000000)] },  // ~9.2
      { input: [2.0, 10.0, 1.0], expected: [10.0] },
      { input: [2.0, 10.0, 2.0], expected: [18.0] },

      // [-1.0, 1.0] cases
      { input: [-1.0, 1.0, -2.0], expected: [-5.0] },
      { input: [-1.0, 1.0, 0.0], expected: [-1.0] },
      { input: [-1.0, 1.0, 0.1], expected: [hexToF64(0xbfe99999,0xc0000000), hexToF64(0xbfe99999,0x80000000)] },  // ~-0.8
      { input: [-1.0, 1.0, 0.5], expected: [0.0] },
      { input: [-1.0, 1.0, 0.9], expected: [hexToF64(0x3fe99999,0x80000000), hexToF64(0x3fe99999,0xc0000000)] },  // ~0.8
      { input: [-1.0, 1.0, 1.0], expected: [1.0] },
      { input: [-1.0, 1.0, 2.0], expected: [3.0] },

      // Infinities
      { input: [0.0, kValue.f32.infinity.positive, 0.5], expected: kAny },
      { input: [kValue.f32.infinity.positive, 0.0, 0.5], expected: kAny },
      { input: [kValue.f32.infinity.negative, 1.0, 0.5], expected: kAny },
      { input: [1.0, kValue.f32.infinity.negative, 0.5], expected: kAny },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive, 0.5], expected: kAny },
      { input: [kValue.f32.infinity.positive, kValue.f32.infinity.negative, 0.5], expected: kAny },
      { input: [0.0, 1.0, kValue.f32.infinity.negative], expected: kAny },
      { input: [1.0, 0.0, kValue.f32.infinity.negative], expected: kAny },
      { input: [0.0, 1.0, kValue.f32.infinity.positive], expected: kAny },
      { input: [1.0, 0.0, kValue.f32.infinity.positive], expected: kAny },

      // Showing how precise and imprecise versions diff
      { input: [kValue.f32.negative.min, 10.0, 1.0], expected: [10.0]},
    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = mixPreciseInterval(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `mixPreciseInterval(${x}, ${y}, ${z}) returned ${got}. Expected ${expected}`
    );
  });

g.test('smoothStepInterval')
  .paramsSubcasesOnly<TernaryToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form due to the inherited nature of the errors.
      // Normals
      { input: [0, 1, 0], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, 1, 1], expected: [hexToF32(0x3f7ffffa), hexToF32(0x3f800003)] },  // ~1
      { input: [0, 1, 10], expected: [1] },
      { input: [0, 1, -10], expected: [0] },
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
      { input: [0, kValue.f32.subnormal.positive.max, 1], expected: kAny },
      { input: [0, kValue.f32.subnormal.positive.min, 1], expected: kAny },
      { input: [0, kValue.f32.subnormal.negative.max, 1], expected: kAny },
      { input: [0, kValue.f32.subnormal.negative.min, 1], expected: kAny },

      // Infinities
      { input: [0, 2, Number.POSITIVE_INFINITY], expected: kAny },
      { input: [0, 2, Number.NEGATIVE_INFINITY], expected: kAny },
      { input: [Number.POSITIVE_INFINITY, 2, 1], expected: kAny },
      { input: [Number.NEGATIVE_INFINITY, 2, 1], expected: kAny },
      { input: [0, Number.POSITIVE_INFINITY, 1], expected: kAny },
      { input: [0, Number.NEGATIVE_INFINITY, 1], expected: kAny },
    ]
  )
  .fn(t => {
    const [low, high, x] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = smoothStepInterval(low, high, x);
    t.expect(
      objectEquals(expected, got),
      `smoothStepInterval(${low}, ${high}, ${x}) returned ${got}. Expected ${expected}`
    );
  });

interface VectorToIntervalCase {
  input: number[];
  expected: IntervalBounds;
}

g.test('lengthIntervalVector')
  .paramsSubcasesOnly<VectorToIntervalCase>(
    // prettier-ignore
    [
      // Some of these are hard coded, since the error intervals are difficult to express in a closed human readable
      // form due to the inherited nature of the errors.
      // vec2
      {input: [1.0, 0.0], expected: [hexToF64(0x3fefffff, 0x70000000), hexToF64(0x3ff00000, 0x90000000)] },  // ~1
      {input: [0.0, 1.0], expected: [hexToF64(0x3fefffff, 0x70000000), hexToF64(0x3ff00000, 0x90000000)] },  // ~1
      {input: [1.0, 1.0], expected: [hexToF64(0x3ff6a09d, 0xb0000000), hexToF64(0x3ff6a09f, 0x10000000)] },  // ~√2
      {input: [-1.0, -1.0], expected: [hexToF64(0x3ff6a09d, 0xb0000000), hexToF64(0x3ff6a09f, 0x10000000)] },  // ~√2
      {input: [-1.0, 1.0], expected: [hexToF64(0x3ff6a09d, 0xb0000000), hexToF64(0x3ff6a09f, 0x10000000)] },  // ~√2
      {input: [0.1, 0.0], expected: [hexToF64(0x3fb99998, 0x90000000), hexToF64(0x3fb9999a, 0x70000000)] },  // ~0.1

      // vec3
      {input: [1.0, 0.0, 0.0], expected: [hexToF64(0x3fefffff, 0x70000000), hexToF64(0x3ff00000, 0x90000000)] },  // ~1
      {input: [0.0, 1.0, 0.0], expected: [hexToF64(0x3fefffff, 0x70000000), hexToF64(0x3ff00000, 0x90000000)] },  // ~1
      {input: [0.0, 0.0, 1.0], expected: [hexToF64(0x3fefffff, 0x70000000), hexToF64(0x3ff00000, 0x90000000)] },  // ~1
      {input: [1.0, 1.0, 1.0], expected: [hexToF64(0x3ffbb67a, 0x10000000), hexToF64(0x3ffbb67b, 0xb0000000)] },  // ~√3
      {input: [-1.0, -1.0, -1.0], expected: [hexToF64(0x3ffbb67a, 0x10000000), hexToF64(0x3ffbb67b, 0xb0000000)] },  // ~√3
      {input: [1.0, -1.0, -1.0], expected: [hexToF64(0x3ffbb67a, 0x10000000), hexToF64(0x3ffbb67b, 0xb0000000)] },  // ~√3
      {input: [0.1, 0.0, 0.0], expected: [hexToF64(0x3fb99998, 0x90000000), hexToF64(0x3fb9999a, 0x70000000)] },  // ~0.1

      // vec4
      {input: [1.0, 0.0, 0.0, 0.0], expected: [hexToF64(0x3fefffff, 0x70000000), hexToF64(0x3ff00000, 0x90000000)] },  // ~1
      {input: [0.0, 1.0, 0.0, 0.0], expected: [hexToF64(0x3fefffff, 0x70000000), hexToF64(0x3ff00000, 0x90000000)] },  // ~1
      {input: [0.0, 0.0, 1.0, 0.0], expected: [hexToF64(0x3fefffff, 0x70000000), hexToF64(0x3ff00000, 0x90000000)] },  // ~1
      {input: [0.0, 0.0, 0.0, 1.0], expected: [hexToF64(0x3fefffff, 0x70000000), hexToF64(0x3ff00000, 0x90000000)] },  // ~1
      {input: [1.0, 1.0, 1.0, 1.0], expected: [hexToF64(0x3fffffff, 0x70000000), hexToF64(0x40000000, 0x90000000)] },  // ~2
      {input: [-1.0, -1.0, -1.0, -1.0], expected: [hexToF64(0x3fffffff, 0x70000000), hexToF64(0x40000000, 0x90000000)] },  // ~2
      {input: [-1.0, 1.0, -1.0, 1.0], expected: [hexToF64(0x3fffffff, 0x70000000), hexToF64(0x40000000, 0x90000000)] },  // ~2
      {input: [0.1, 0.0, 0.0, 0.0], expected: [hexToF64(0x3fb99998, 0x90000000), hexToF64(0x3fb9999a, 0x70000000)] },  // ~0.1
    ]
  )
  .fn(t => {
    const expected = new F32Interval(...t.params.expected);

    const got = lengthInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `lengthInterval([${t.params.input}]) returned ${got}. Expected ${expected}`
    );
  });

interface VectorPairToIntervalCase {
  input: [number[], number[]];
  expected: IntervalBounds;
}

g.test('dotInterval')
  .paramsSubcasesOnly<VectorPairToIntervalCase>(
    // prettier-ignore
    [
      // vec2
      {input: [[1.0, 0.0], [1.0, 0.0]], expected: [1.0] },
      {input: [[0.0, 1.0], [0.0, 1.0]], expected: [1.0] },
      {input: [[1.0, 1.0], [1.0, 1.0]], expected: [2.0] },
      {input: [[-1.0, -1.0], [-1.0, -1.0]], expected: [2.0] },
      {input: [[-1.0, 1.0], [1.0, -1.0]], expected: [-2.0] },
      {input: [[0.1, 0.0], [1.0, 0.0]], expected: [hexToF64(0x3fb99999, 0x80000000), hexToF64(0x3fb99999, 0xa0000000)]},  // ~0.1

      // vec3
      {input: [[1.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [1.0] },
      {input: [[0.0, 1.0, 0.0], [0.0, 1.0, 0.0]], expected: [1.0] },
      {input: [[0.0, 0.0, 1.0], [0.0, 0.0, 1.0]], expected: [1.0] },
      {input: [[1.0, 1.0, 1.0], [1.0, 1.0, 1.0]], expected: [3.0] },
      {input: [[-1.0, -1.0, -1.0], [-1.0, -1.0, -1.0]], expected: [3.0] },
      {input: [[1.0, -1.0, -1.0], [-1.0, 1.0, -1.0]], expected: [-1.0] },
      {input: [[0.1, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [hexToF64(0x3fb99999, 0x80000000), hexToF64(0x3fb99999, 0xa0000000)]},  // ~0.1

      // vec4
      {input: [[1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [1.0] },
      {input: [[0.0, 1.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0]], expected: [1.0] },
      {input: [[0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 1.0, 0.0]], expected: [1.0] },
      {input: [[0.0, 0.0, 0.0, 1.0], [0.0, 0.0, 0.0, 1.0]], expected: [1.0] },
      {input: [[1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 1.0, 1.0]], expected: [4.0] },
      {input: [[-1.0, -1.0, -1.0, -1.0], [-1.0, -1.0, -1.0, -1.0]], expected: [4.0] },
      {input: [[-1.0, 1.0, -1.0, 1.0], [1.0, -1.0, 1.0, -1.0]], expected: [-4.0] },
      {input: [[0.1, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [hexToF64(0x3fb99999, 0x80000000), hexToF64(0x3fb99999, 0xa0000000)]},  // ~0.1
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = new F32Interval(...t.params.expected);

    const got = dotInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `dotInterval([${x}], [${y}]) returned ${got}. Expected ${expected}`
    );
  });

interface VectorToVectorCase {
  input: number[];
  expected: IntervalBounds[];
}

g.test('normalizeInterval')
  .paramsSubcasesOnly<VectorToVectorCase>(
    // prettier-ignore
    [
      // vec2
      {input: [1.0, 0.0], expected: [[hexToF64(0x3feffffe, 0x70000000), hexToF64(0x3ff00000, 0xb0000000)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~1.0, ~0.0]
      {input: [0.0, 1.0], expected: [[hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF64(0x3feffffe, 0x70000000), hexToF64(0x3ff00000, 0xb0000000)]] },  // [ ~0.0, ~1.0]
      {input: [-1.0, 0.0], expected: [[hexToF64(0xbff00000, 0xb0000000), hexToF64(0xbfeffffe, 0x70000000)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~1.0, ~0.0]
      {input: [1.0, 1.0], expected: [[hexToF64(0x3fe6a09d, 0x50000000), hexToF64(0x3fe6a09f, 0x90000000)], [hexToF64(0x3fe6a09d, 0x50000000), hexToF64(0x3fe6a09f, 0x90000000)]] },  // [ ~1/√2, ~1/√2]

      // vec3
      {input: [1.0, 0.0, 0.0], expected: [[hexToF64(0x3feffffe, 0x70000000), hexToF64(0x3ff00000, 0xb0000000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~1.0, ~0.0, ~0.0]
      {input: [0.0, 1.0, 0.0], expected: [[hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF64(0x3feffffe, 0x70000000), hexToF64(0x3ff00000, 0xb0000000)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~0.0, ~1.0, ~0.0]
      {input: [0.0, 0.0, 1.0], expected: [[hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF64(0x3feffffe, 0x70000000), hexToF64(0x3ff00000, 0xb0000000)]] },  // [ ~0.0, ~0.0, ~1.0]
      {input: [-1.0, 0.0, 0.0], expected: [[hexToF64(0xbff00000, 0xb0000000), hexToF64(0xbfeffffe, 0x70000000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~1.0, ~0.0, ~0.0]
      {input: [1.0, 1.0, 1.0], expected: [[hexToF64(0x3fe279a6, 0x50000000), hexToF64(0x3fe279a8, 0x50000000)], [hexToF64(0x3fe279a6, 0x50000000), hexToF64(0x3fe279a8, 0x50000000)], [hexToF64(0x3fe279a6, 0x50000000), hexToF64(0x3fe279a8, 0x50000000)]] },  // [ ~1/√3, ~1/√3, ~1/√3]

      // vec4
      {input: [1.0, 0.0, 0.0, 0.0], expected: [[hexToF64(0x3feffffe, 0x70000000), hexToF64(0x3ff00000, 0xb0000000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~1.0, ~0.0, ~0.0, ~0.0]
      {input: [0.0, 1.0, 0.0, 0.0], expected: [[hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF64(0x3feffffe, 0x70000000), hexToF64(0x3ff00000, 0xb0000000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~0.0, ~1.0, ~0.0, ~0.0]
      {input: [0.0, 0.0, 1.0, 0.0], expected: [[hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF64(0x3feffffe, 0x70000000), hexToF64(0x3ff00000, 0xb0000000)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~0.0, ~0.0, ~1.0, ~0.0]
      {input: [0.0, 0.0, 0.0, 1.0], expected: [[hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF64(0x3feffffe, 0x70000000), hexToF64(0x3ff00000, 0xb0000000)]] },  // [ ~0.0, ~0.0, ~0.0, ~1.0]
      {input: [-1.0, 0.0, 0.0, 0.0], expected: [[hexToF64(0xbff00000, 0xb0000000), hexToF64(0xbfeffffe, 0x70000000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)], [hexToF32(0x81200000), hexToF32(0x01200000)]] },  // [ ~1.0, ~0.0, ~0.0, ~0.0]
      {input: [1.0, 1.0, 1.0, 1.0], expected: [[hexToF64(0x3fdffffe, 0x70000000), hexToF64(0x3fe00000, 0xb0000000)], [hexToF64(0x3fdffffe, 0x70000000), hexToF64(0x3fe00000, 0xb0000000)], [hexToF64(0x3fdffffe, 0x70000000), hexToF64(0x3fe00000, 0xb0000000)], [hexToF64(0x3fdffffe, 0x70000000), hexToF64(0x3fe00000, 0xb0000000)]] },  // [ ~1/√4, ~1/√4, ~1/√4]
    ]
  )
  .fn(t => {
    const x = t.params.input;
    const expected = t.params.expected.map(e => new F32Interval(...e));

    const got = normalizeInterval(x);
    t.expect(
      objectEquals(expected, got),
      `normalizeInterval([${x}]) returned ${got}. Expected ${expected}`
    );
  });
