export const description = `
Floating Point unit tests.
`;

import { makeTestGroup } from '../common/framework/test_group.js';
import { objectEquals, unreachable } from '../common/util/util.js';
import { kValue } from '../webgpu/util/constants.js';
import { FP, FPInterval, IntervalBounds } from '../webgpu/util/floating_point.js';
import { hexToF32, hexToF64, map2DArray, oneULPF32 } from '../webgpu/util/math.js';

import { UnitTest } from './unit_test.js';

export const g = makeTestGroup(UnitTest);

/** Bounds indicating an expectation of an interval of all possible values */
const kAnyBounds: IntervalBounds = [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY];

/** Interval from kAnyBounds for f32 */
const kAnyIntervalF32: FPInterval = FP.f32.toInterval(kAnyBounds);

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

// FPInterval

interface ConstructorCase {
  input: IntervalBounds;
  expected: IntervalBounds;
}

g.test('constructor_f32')
  .paramsSubcasesOnly<ConstructorCase>(
    // prettier-ignore
    [
      // Common cases
      { input: [0, 10], expected: [0, 10] },
      { input: [-5, 0], expected: [-5, 0] },
      { input: [-5, 10], expected: [-5, 10] },
      { input: [0], expected: [0] },
      { input: [10], expected: [10] },
      { input: [-5], expected: [-5] },

      // Edges
      { input: [0, kValue.f32.positive.max], expected: [0, kValue.f32.positive.max] },
      { input: [kValue.f32.negative.min, 0], expected: [kValue.f32.negative.min, 0] },
      { input: [kValue.f32.negative.min, kValue.f32.positive.max], expected: [kValue.f32.negative.min, kValue.f32.positive.max] },

      // Out of range
      { input: [0, 2 * kValue.f32.positive.max], expected: [0, 2 * kValue.f32.positive.max] },
      { input: [2 * kValue.f32.negative.min, 0], expected: [2 * kValue.f32.negative.min, 0] },
      { input: [2 * kValue.f32.negative.min, 2 * kValue.f32.positive.max], expected: [2 * kValue.f32.negative.min, 2 * kValue.f32.positive.max] },

      // Infinities
      { input: [0, kValue.f32.infinity.positive], expected: [0, Number.POSITIVE_INFINITY] },
      { input: [kValue.f32.infinity.negative, 0], expected: [Number.NEGATIVE_INFINITY, 0] },
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: kAnyBounds },
    ]
  )
  .fn(t => {
    const i = new FPInterval('f32', ...t.params.input);
    t.expect(
      objectEquals(i.bounds(), t.params.expected),
      `new FPInterval('f32', [${t.params.input}]) returned ${i}. Expected [${t.params.expected}]`
    );
  });

interface ContainsNumberCase {
  bounds: number | IntervalBounds;
  value: number;
  expected: boolean;
}

g.test('contains_number_f32')
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
      { bounds: 0, value: 0, expected: true },
      { bounds: 0, value: 10, expected: false },
      { bounds: 0, value: -1000, expected: false },
      { bounds: 10, value: 10, expected: true },
      { bounds: 10, value: 0, expected: false },
      { bounds: 10, value: -10, expected: false },
      { bounds: 10, value: 11, expected: false },

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
    const i = FP.f32.toInterval(t.params.bounds);
    const value = t.params.value;
    const expected = t.params.expected;

    const got = i.contains(value);
    t.expect(expected === got, `${i}.contains(${value}) returned ${got}. Expected ${expected}`);
  });

interface ContainsIntervalCase {
  lhs: number | IntervalBounds;
  rhs: number | IntervalBounds;
  expected: boolean;
}

g.test('contains_interval_f32')
  .paramsSubcasesOnly<ContainsIntervalCase>(
    // prettier-ignore
    [
      // Common usage
      { lhs: [-10, 10], rhs: 0, expected: true },
      { lhs: [-10, 10], rhs: [-1, 0], expected: true },
      { lhs: [-10, 10], rhs: [0, 2], expected: true },
      { lhs: [-10, 10], rhs: [-1, 2], expected: true },
      { lhs: [-10, 10], rhs: [0, 10], expected: true },
      { lhs: [-10, 10], rhs: [-10, 2], expected: true },
      { lhs: [-10, 10], rhs: [-10, 10], expected: true },
      { lhs: [-10, 10], rhs: [-100, 10], expected: false },

      // Upper infinity
      { lhs: [0, kValue.f32.infinity.positive], rhs: 0, expected: true },
      { lhs: [0, kValue.f32.infinity.positive], rhs: [-1, 0], expected: false },
      { lhs: [0, kValue.f32.infinity.positive], rhs: [0, 1], expected: true },
      { lhs: [0, kValue.f32.infinity.positive], rhs: [0, kValue.f32.positive.max], expected: true },
      { lhs: [0, kValue.f32.infinity.positive], rhs: [0, kValue.f32.infinity.positive], expected: true },
      { lhs: [0, kValue.f32.infinity.positive], rhs: [100, kValue.f32.infinity.positive], expected: true },
      { lhs: [0, kValue.f32.infinity.positive], rhs: [Number.NEGATIVE_INFINITY, kValue.f32.infinity.positive], expected: false },

      // Lower infinity
      { lhs: [kValue.f32.infinity.negative, 0], rhs: 0, expected: true },
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [-1, 0], expected: true },
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [kValue.f32.negative.min, 0], expected: true },
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [0, 1], expected: false },
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [kValue.f32.infinity.negative, 0], expected: true },
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [kValue.f32.infinity.negative, -100 ], expected: true },
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: false },

      // Full infinity
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: 0, expected: true },
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [-1, 0], expected: true },
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [0, 1], expected: true },
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [0, kValue.f32.infinity.positive], expected: true },
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [100, kValue.f32.infinity.positive], expected: true },
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [kValue.f32.infinity.negative, 0], expected: true },
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [kValue.f32.infinity.negative, -100 ], expected: true },
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: true },

      // Maximum f32 boundary
      { lhs: [0, kValue.f32.positive.max], rhs: 0, expected: true },
      { lhs: [0, kValue.f32.positive.max], rhs: [-1, 0], expected: false },
      { lhs: [0, kValue.f32.positive.max], rhs: [0, 1], expected: true },
      { lhs: [0, kValue.f32.positive.max], rhs: [0, kValue.f32.positive.max], expected: true },
      { lhs: [0, kValue.f32.positive.max], rhs: [0, kValue.f32.infinity.positive], expected: false },
      { lhs: [0, kValue.f32.positive.max], rhs: [100, kValue.f32.infinity.positive], expected: false },
      { lhs: [0, kValue.f32.positive.max], rhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: false },

      // Minimum f32 boundary
      { lhs: [kValue.f32.negative.min, 0], rhs: [0, 0], expected: true },
      { lhs: [kValue.f32.negative.min, 0], rhs: [-1, 0], expected: true },
      { lhs: [kValue.f32.negative.min, 0], rhs: [kValue.f32.negative.min, 0], expected: true },
      { lhs: [kValue.f32.negative.min, 0], rhs: [0, 1], expected: false },
      { lhs: [kValue.f32.negative.min, 0], rhs: [kValue.f32.infinity.negative, 0], expected: false },
      { lhs: [kValue.f32.negative.min, 0], rhs: [kValue.f32.infinity.negative, -100 ], expected: false },
      { lhs: [kValue.f32.negative.min, 0], rhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: false },

      // Out of range high
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: 0, expected: true },
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [-1, 0], expected: false },
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [0, 1], expected: true },
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [0, kValue.f32.positive.max], expected: true },
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [0, kValue.f32.infinity.positive], expected: false },
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [100, kValue.f32.infinity.positive], expected: false },
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: false },

      // Out of range low
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: 0, expected: true },
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [-1, 0], expected: true },
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [kValue.f32.negative.min, 0], expected: true },
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [0, 1], expected: false },
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [kValue.f32.infinity.negative, 0], expected: false },
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [kValue.f32.infinity.negative, -100 ], expected: false },
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: false },
    ]
  )
  .fn(t => {
    const lhs = FP.f32.toInterval(t.params.lhs);
    const rhs = FP.f32.toInterval(t.params.rhs);
    const expected = t.params.expected;

    const got = lhs.contains(rhs);
    t.expect(expected === got, `${lhs}.contains(${rhs}) returned ${got}. Expected ${expected}`);
  });

// Utilities

interface SpanIntervalsCase {
  intervals: (number | IntervalBounds)[];
  expected: number | IntervalBounds;
}

g.test('spanIntervals_f32')
  .paramsSubcasesOnly<SpanIntervalsCase>(
    // prettier-ignore
    [
      // Single Intervals
      { intervals: [[0, 10]], expected: [0, 10] },
      { intervals: [[0, kValue.f32.positive.max]], expected: [0, kValue.f32.positive.max] },
      { intervals: [[0, kValue.f32.positive.nearest_max]], expected: [0, kValue.f32.positive.nearest_max] },
      { intervals: [[0, kValue.f32.infinity.positive]], expected: [0, Number.POSITIVE_INFINITY] },
      { intervals: [[kValue.f32.negative.min, 0]], expected: [kValue.f32.negative.min, 0] },
      { intervals: [[kValue.f32.negative.nearest_min, 0]], expected: [kValue.f32.negative.nearest_min, 0] },
      { intervals: [[kValue.f32.infinity.negative, 0]], expected: [Number.NEGATIVE_INFINITY, 0] },

      // Double Intervals
      { intervals: [[0, 1], [2, 5]], expected: [0, 5] },
      { intervals: [[2, 5], [0, 1]], expected: [0, 5] },
      { intervals: [[0, 2], [1, 5]], expected: [0, 5] },
      { intervals: [[0, 5], [1, 2]], expected: [0, 5] },
      { intervals: [[kValue.f32.infinity.negative, 0], [0, kValue.f32.infinity.positive]], expected: kAnyBounds },

      // Multiple Intervals
      { intervals: [[0, 1], [2, 3], [4, 5]], expected: [0, 5] },
      { intervals: [[0, 1], [4, 5], [2, 3]], expected: [0, 5] },
      { intervals: [[0, 1], [0, 1], [0, 1]], expected: [0, 1] },

      // Point Intervals
      { intervals: [1], expected: 1 },
      { intervals: [1, 2], expected: [1, 2] },
      { intervals: [-10, 2], expected: [-10, 2] },
    ]
  )
  .fn(t => {
    const intervals = t.params.intervals.map(i => FP.f32.toInterval(i));
    const expected = FP.f32.toInterval(t.params.expected);

    const got = FP.f32.spanIntervals(...intervals);
    t.expect(
      objectEquals(got, expected),
      `f32.span({${intervals}}) returned ${got}. Expected ${expected}`
    );
  });

interface isVectorCase {
  input: (number | IntervalBounds | FPInterval)[];
  expected: boolean;
}

g.test('isVector_f32')
  .paramsSubcasesOnly<isVectorCase>([
    // numbers
    { input: [1, 2], expected: false },
    { input: [1, 2, 3], expected: false },
    { input: [1, 2, 3, 4], expected: false },

    // IntervalBounds
    { input: [[1], [2]], expected: false },
    { input: [[1], [2], [3]], expected: false },
    { input: [[1], [2], [3], [4]], expected: false },
    {
      input: [
        [1, 2],
        [2, 3],
      ],
      expected: false,
    },
    {
      input: [
        [1, 2],
        [2, 3],
        [3, 4],
      ],
      expected: false,
    },
    {
      input: [
        [1, 2],
        [2, 3],
        [3, 4],
        [4, 5],
      ],
      expected: false,
    },

    // F32Interval, valid dimensions
    { input: [FP.f32.toInterval([1]), FP.f32.toInterval([2])], expected: true },
    { input: [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3])], expected: true },
    {
      input: [FP.f32.toInterval([1]), FP.f32.toInterval([2]), FP.f32.toInterval([3])],
      expected: true,
    },
    {
      input: [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3]), FP.f32.toInterval([3, 4])],
      expected: true,
    },
    {
      input: [
        FP.f32.toInterval([1]),
        FP.f32.toInterval([2]),
        FP.f32.toInterval([3]),
        FP.f32.toInterval([4]),
      ],
      expected: true,
    },
    {
      input: [
        FP.f32.toInterval([1, 2]),
        FP.f32.toInterval([2, 3]),
        FP.f32.toInterval([3, 4]),
        FP.f32.toInterval([4, 5]),
      ],
      expected: true,
    },

    // FPInterval, invalid dimensions
    { input: [FP.f32.toInterval([1])], expected: false },
    {
      input: [
        FP.f32.toInterval([1]),
        FP.f32.toInterval([2]),
        FP.f32.toInterval([3]),
        FP.f32.toInterval([4]),
        FP.f32.toInterval([5]),
      ],
      expected: false,
    },

    // Mixed
    { input: [1, [2]], expected: false },
    { input: [1, [2], FP.f32.toInterval([3])], expected: false },
    { input: [1, FP.f32.toInterval([2]), [3], 4], expected: false },
    { input: [FP.f32.toInterval(1), 2], expected: false },
    { input: [FP.f32.toInterval(1), [2]], expected: false },
  ])
  .fn(t => {
    const input = t.params.input;
    const expected = t.params.expected;

    const got = FP.f32.isVector(input);
    t.expect(got === expected, `f32.isVector([${input}]) returned ${got}. Expected ${expected}`);
  });

interface toVectorCase {
  input: (number | IntervalBounds | FPInterval)[];
  expected: (number | IntervalBounds)[];
}

g.test('toVector_f32')
  .paramsSubcasesOnly<toVectorCase>([
    // numbers
    { input: [1, 2], expected: [1, 2] },
    { input: [1, 2, 3], expected: [1, 2, 3] },
    { input: [1, 2, 3, 4], expected: [1, 2, 3, 4] },

    // IntervalBounds
    { input: [[1], [2]], expected: [1, 2] },
    { input: [[1], [2], [3]], expected: [1, 2, 3] },
    { input: [[1], [2], [3], [4]], expected: [1, 2, 3, 4] },
    {
      input: [
        [1, 2],
        [2, 3],
      ],
      expected: [
        [1, 2],
        [2, 3],
      ],
    },
    {
      input: [
        [1, 2],
        [2, 3],
        [3, 4],
      ],
      expected: [
        [1, 2],
        [2, 3],
        [3, 4],
      ],
    },
    {
      input: [
        [1, 2],
        [2, 3],
        [3, 4],
        [4, 5],
      ],
      expected: [
        [1, 2],
        [2, 3],
        [3, 4],
        [4, 5],
      ],
    },

    // F32Interval
    { input: [FP.f32.toInterval([1]), FP.f32.toInterval([2])], expected: [1, 2] },
    {
      input: [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3])],
      expected: [
        [1, 2],
        [2, 3],
      ],
    },
    {
      input: [FP.f32.toInterval([1]), FP.f32.toInterval([2]), FP.f32.toInterval([3])],
      expected: [1, 2, 3],
    },
    {
      input: [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3]), FP.f32.toInterval([3, 4])],
      expected: [
        [1, 2],
        [2, 3],
        [3, 4],
      ],
    },
    {
      input: [
        FP.f32.toInterval([1]),
        FP.f32.toInterval([2]),
        FP.f32.toInterval([3]),
        FP.f32.toInterval([4]),
      ],
      expected: [1, 2, 3, 4],
    },
    {
      input: [
        FP.f32.toInterval([1, 2]),
        FP.f32.toInterval([2, 3]),
        FP.f32.toInterval([3, 4]),
        FP.f32.toInterval([4, 5]),
      ],
      expected: [
        [1, 2],
        [2, 3],
        [3, 4],
        [4, 5],
      ],
    },

    // Mixed
    { input: [1, [2]], expected: [1, 2] },
    { input: [1, [2], FP.f32.toInterval([3])], expected: [1, 2, 3] },
    { input: [1, FP.f32.toInterval([2]), [3], 4], expected: [1, 2, 3, 4] },
    {
      input: [1, [2], [2, 3], kAnyIntervalF32],
      expected: [1, 2, [2, 3], kAnyBounds],
    },
  ])
  .fn(t => {
    const input = t.params.input;
    const expected = t.params.expected.map(e => FP.f32.toInterval(e));

    const got = FP.f32.toVector(input);
    t.expect(
      objectEquals(got, expected),
      `f32.toVector([${input}]) returned [${got}]. Expected [${expected}]`
    );
  });

interface isMatrixCase {
  input: (number | IntervalBounds | FPInterval)[][];
  expected: boolean;
}

g.test('isMatrix_f32')
  .paramsSubcasesOnly<isMatrixCase>([
    // numbers
    {
      input: [
        [1, 2],
        [3, 4],
      ],
      expected: false,
    },
    {
      input: [
        [1, 2],
        [3, 4],
        [5, 6],
      ],
      expected: false,
    },
    {
      input: [
        [1, 2],
        [3, 4],
        [5, 6],
        [7, 8],
      ],
      expected: false,
    },
    {
      input: [
        [1, 2, 3],
        [4, 5, 6],
      ],
      expected: false,
    },
    {
      input: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
      ],
      expected: false,
    },
    {
      input: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
        [10, 11, 12],
      ],
      expected: false,
    },
    {
      input: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
      ],
      expected: false,
    },
    {
      input: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
      ],
      expected: false,
    },
    {
      input: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
        [13, 14, 15, 16],
      ],
      expected: false,
    },

    // IntervalBounds
    {
      input: [
        [[1], [2]],
        [[3], [4]],
      ],
      expected: false,
    },
    {
      input: [
        [[1], [2]],
        [[3], [4]],
        [[5], [6]],
      ],
      expected: false,
    },
    {
      input: [
        [[1], [2]],
        [[3], [4]],
        [[5], [6]],
        [[7], [8]],
      ],
      expected: false,
    },
    {
      input: [
        [[1], [2], [3]],
        [[4], [5], [6]],
      ],
      expected: false,
    },
    {
      input: [
        [[1], [2], [3]],
        [[4], [5], [6]],
        [[7], [8], [9]],
      ],
      expected: false,
    },
    {
      input: [
        [[1], [2], [3]],
        [[4], [5], [6]],
        [[7], [8], [9]],
        [[10], [11], [12]],
      ],
      expected: false,
    },
    {
      input: [
        [[1], [2], [3], [4]],
        [[5], [6], [7], [8]],
      ],
      expected: false,
    },
    {
      input: [
        [[1], [2], [3], [4]],
        [[5], [6], [7], [8]],
        [[9], [10], [11], [12]],
      ],
      expected: false,
    },
    {
      input: [
        [[1], [2], [3], [4]],
        [[5], [6], [7], [8]],
        [[9], [10], [11], [12]],
        [[13], [14], [15], [16]],
      ],
      expected: false,
    },

    // FPInterval, valid dimensions
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2)],
        [FP.f32.toInterval(3), FP.f32.toInterval(4)],
      ],
      expected: true,
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2)],
        [FP.f32.toInterval(3), FP.f32.toInterval(4)],
        [FP.f32.toInterval(5), FP.f32.toInterval(6)],
      ],
      expected: true,
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2)],
        [FP.f32.toInterval(3), FP.f32.toInterval(4)],
        [FP.f32.toInterval(5), FP.f32.toInterval(6)],
        [FP.f32.toInterval(7), FP.f32.toInterval(8)],
      ],
      expected: true,
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2), FP.f32.toInterval(3)],
        [FP.f32.toInterval(4), FP.f32.toInterval(5), FP.f32.toInterval(6)],
      ],
      expected: true,
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2), FP.f32.toInterval(3)],
        [FP.f32.toInterval(4), FP.f32.toInterval(5), FP.f32.toInterval(6)],
        [FP.f32.toInterval(7), FP.f32.toInterval(8), FP.f32.toInterval(9)],
      ],
      expected: true,
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2), FP.f32.toInterval(3)],
        [FP.f32.toInterval(4), FP.f32.toInterval(5), FP.f32.toInterval(6)],
        [FP.f32.toInterval(7), FP.f32.toInterval(8), FP.f32.toInterval(9)],
        [FP.f32.toInterval(10), FP.f32.toInterval(11), FP.f32.toInterval(12)],
      ],
      expected: true,
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2), FP.f32.toInterval(3), FP.f32.toInterval(4)],
        [FP.f32.toInterval(5), FP.f32.toInterval(6), FP.f32.toInterval(7), FP.f32.toInterval(8)],
      ],
      expected: true,
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2), FP.f32.toInterval(3), FP.f32.toInterval(4)],
        [FP.f32.toInterval(5), FP.f32.toInterval(6), FP.f32.toInterval(7), FP.f32.toInterval(8)],
        [FP.f32.toInterval(9), FP.f32.toInterval(10), FP.f32.toInterval(11), FP.f32.toInterval(12)],
      ],
      expected: true,
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2), FP.f32.toInterval(3), FP.f32.toInterval(4)],
        [FP.f32.toInterval(5), FP.f32.toInterval(6), FP.f32.toInterval(7), FP.f32.toInterval(8)],
        [FP.f32.toInterval(9), FP.f32.toInterval(10), FP.f32.toInterval(11), FP.f32.toInterval(12)],
        [
          FP.f32.toInterval(13),
          FP.f32.toInterval(14),
          FP.f32.toInterval(15),
          FP.f32.toInterval(16),
        ],
      ],
      expected: true,
    },
    {
      input: [
        [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3])],
        [FP.f32.toInterval([3, 4]), FP.f32.toInterval([4, 5])],
      ],
      expected: true,
    },
    {
      input: [
        [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3])],
        [FP.f32.toInterval([3, 4]), FP.f32.toInterval([4, 5])],
        [FP.f32.toInterval([5, 6]), FP.f32.toInterval([6, 7])],
      ],
      expected: true,
    },
    {
      input: [
        [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3])],
        [FP.f32.toInterval([3, 4]), FP.f32.toInterval([4, 5])],
        [FP.f32.toInterval([5, 6]), FP.f32.toInterval([6, 7])],
        [FP.f32.toInterval([7, 8]), FP.f32.toInterval([8, 9])],
      ],
      expected: true,
    },
    {
      input: [
        [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3]), FP.f32.toInterval([3, 4])],
        [FP.f32.toInterval([4, 5]), FP.f32.toInterval([5, 6]), FP.f32.toInterval([6, 7])],
      ],
      expected: true,
    },
    {
      input: [
        [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3]), FP.f32.toInterval([3, 4])],
        [FP.f32.toInterval([4, 5]), FP.f32.toInterval([5, 6]), FP.f32.toInterval([6, 7])],
        [FP.f32.toInterval([7, 8]), FP.f32.toInterval([8, 9]), FP.f32.toInterval([9, 10])],
      ],
      expected: true,
    },
    {
      input: [
        [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3]), FP.f32.toInterval([3, 4])],
        [FP.f32.toInterval([4, 5]), FP.f32.toInterval([5, 6]), FP.f32.toInterval([6, 7])],
        [FP.f32.toInterval([7, 8]), FP.f32.toInterval([8, 9]), FP.f32.toInterval([9, 10])],
        [FP.f32.toInterval([10, 11]), FP.f32.toInterval([11, 12]), FP.f32.toInterval([12, 13])],
      ],
      expected: true,
    },
    {
      input: [
        [
          FP.f32.toInterval([1, 2]),
          FP.f32.toInterval([2, 3]),
          FP.f32.toInterval([3, 4]),
          FP.f32.toInterval([4, 5]),
        ],
        [
          FP.f32.toInterval([5, 6]),
          FP.f32.toInterval([6, 7]),
          FP.f32.toInterval([7, 8]),
          FP.f32.toInterval([8, 9]),
        ],
      ],
      expected: true,
    },
    {
      input: [
        [
          FP.f32.toInterval([1, 2]),
          FP.f32.toInterval([2, 3]),
          FP.f32.toInterval([3, 4]),
          FP.f32.toInterval([4, 5]),
        ],
        [
          FP.f32.toInterval([5, 6]),
          FP.f32.toInterval([6, 7]),
          FP.f32.toInterval([7, 8]),
          FP.f32.toInterval([8, 9]),
        ],
        [
          FP.f32.toInterval([9, 10]),
          FP.f32.toInterval([10, 11]),
          FP.f32.toInterval([11, 12]),
          FP.f32.toInterval([12, 13]),
        ],
      ],
      expected: true,
    },
    {
      input: [
        [
          FP.f32.toInterval([1, 2]),
          FP.f32.toInterval([2, 3]),
          FP.f32.toInterval([3, 4]),
          FP.f32.toInterval([4, 5]),
        ],
        [
          FP.f32.toInterval([5, 6]),
          FP.f32.toInterval([6, 7]),
          FP.f32.toInterval([7, 8]),
          FP.f32.toInterval([8, 9]),
        ],
        [
          FP.f32.toInterval([9, 10]),
          FP.f32.toInterval([10, 11]),
          FP.f32.toInterval([11, 12]),
          FP.f32.toInterval([12, 13]),
        ],
        [
          FP.f32.toInterval([13, 14]),
          FP.f32.toInterval([14, 15]),
          FP.f32.toInterval([15, 16]),
          FP.f32.toInterval([16, 17]),
        ],
      ],
      expected: true,
    },

    // FPInterval, invalid dimensions
    { input: [[FP.f32.toInterval(1)]], expected: false },
    {
      input: [[FP.f32.toInterval(1)], [FP.f32.toInterval(3), FP.f32.toInterval(4)]],
      expected: false,
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2)],
        [FP.f32.toInterval(3), FP.f32.toInterval(4), FP.f32.toInterval(5)],
      ],
      expected: false,
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2)],
        [FP.f32.toInterval(3), FP.f32.toInterval(4)],
        [FP.f32.toInterval(5)],
      ],
      expected: false,
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2)],
        [FP.f32.toInterval(3), FP.f32.toInterval(4)],
        [FP.f32.toInterval(5), FP.f32.toInterval(6)],
        [FP.f32.toInterval(7), FP.f32.toInterval(8)],
        [FP.f32.toInterval(9), FP.f32.toInterval(10)],
      ],
      expected: false,
    },

    // Mixed
    {
      input: [
        [1, [2]],
        [3, 4],
      ],
      expected: false,
    },
    {
      input: [
        [[1], [2]],
        [[3], 4],
      ],
      expected: false,
    },
    {
      input: [
        [1, 2],
        [FP.f32.toInterval([3]), 4],
      ],
      expected: false,
    },
    {
      input: [
        [[1], FP.f32.toInterval([2])],
        [FP.f32.toInterval([3]), FP.f32.toInterval([4])],
      ],
      expected: false,
    },
    {
      input: [
        [FP.f32.toInterval(1), [2]],
        [3, 4],
      ],
      expected: false,
    },
  ])
  .fn(t => {
    const input = t.params.input;
    const expected = t.params.expected;

    const got = FP.f32.isMatrix(input);
    t.expect(got === expected, `f32.isMatrix([${input}]) returned ${got}. Expected ${expected}`);
  });

interface toMatrixCase {
  input: (number | IntervalBounds | FPInterval)[][];
  expected: (number | IntervalBounds)[][];
}

g.test('toMatrix_f32')
  .paramsSubcasesOnly<toMatrixCase>([
    // numbers
    {
      input: [
        [1, 2],
        [3, 4],
      ],
      expected: [
        [1, 2],
        [3, 4],
      ],
    },
    {
      input: [
        [1, 2],
        [3, 4],
        [5, 6],
      ],
      expected: [
        [1, 2],
        [3, 4],
        [5, 6],
      ],
    },
    {
      input: [
        [1, 2],
        [3, 4],
        [5, 6],
        [7, 8],
      ],
      expected: [
        [1, 2],
        [3, 4],
        [5, 6],
        [7, 8],
      ],
    },
    {
      input: [
        [1, 2, 3],
        [4, 5, 6],
      ],
      expected: [
        [1, 2, 3],
        [4, 5, 6],
      ],
    },
    {
      input: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
      ],
      expected: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
      ],
    },
    {
      input: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
        [10, 11, 12],
      ],
      expected: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
        [10, 11, 12],
      ],
    },
    {
      input: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
      ],
      expected: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
      ],
    },
    {
      input: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
      ],
      expected: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
      ],
    },
    {
      input: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
        [13, 14, 15, 16],
      ],
      expected: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
        [13, 14, 15, 16],
      ],
    },

    // IntervalBounds
    {
      input: [
        [[1], [2]],
        [[3], [4]],
      ],
      expected: [
        [1, 2],
        [3, 4],
      ],
    },
    {
      input: [
        [[1], [2]],
        [[3], [4]],
        [[5], [6]],
      ],
      expected: [
        [1, 2],
        [3, 4],
        [5, 6],
      ],
    },
    {
      input: [
        [[1], [2]],
        [[3], [4]],
        [[5], [6]],
        [[7], [8]],
      ],
      expected: [
        [1, 2],
        [3, 4],
        [5, 6],
        [7, 8],
      ],
    },
    {
      input: [
        [[1], [2], [3]],
        [[4], [5], [6]],
      ],
      expected: [
        [1, 2, 3],
        [4, 5, 6],
      ],
    },
    {
      input: [
        [[1], [2], [3]],
        [[4], [5], [6]],
        [[7], [8], [9]],
      ],
      expected: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
      ],
    },
    {
      input: [
        [[1], [2], [3]],
        [[4], [5], [6]],
        [[7], [8], [9]],
        [[10], [11], [12]],
      ],
      expected: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
        [10, 11, 12],
      ],
    },
    {
      input: [
        [[1], [2], [3], [4]],
        [[5], [6], [7], [8]],
      ],
      expected: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
      ],
    },
    {
      input: [
        [[1], [2], [3], [4]],
        [[5], [6], [7], [8]],
        [[9], [10], [11], [12]],
      ],
      expected: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
      ],
    },
    {
      input: [
        [[1], [2], [3], [4]],
        [[5], [6], [7], [8]],
        [[9], [10], [11], [12]],
        [[13], [14], [15], [16]],
      ],
      expected: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
        [13, 14, 15, 16],
      ],
    },

    // FPInterval
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2)],
        [FP.f32.toInterval(3), FP.f32.toInterval(4)],
      ],
      expected: [
        [1, 2],
        [3, 4],
      ],
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2)],
        [FP.f32.toInterval(3), FP.f32.toInterval(4)],
        [FP.f32.toInterval(5), FP.f32.toInterval(6)],
      ],
      expected: [
        [1, 2],
        [3, 4],
        [5, 6],
      ],
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2)],
        [FP.f32.toInterval(3), FP.f32.toInterval(4)],
        [FP.f32.toInterval(5), FP.f32.toInterval(6)],
        [FP.f32.toInterval(7), FP.f32.toInterval(8)],
      ],
      expected: [
        [1, 2],
        [3, 4],
        [5, 6],
        [7, 8],
      ],
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2), FP.f32.toInterval(3)],
        [FP.f32.toInterval(4), FP.f32.toInterval(5), FP.f32.toInterval(6)],
      ],
      expected: [
        [1, 2, 3],
        [4, 5, 6],
      ],
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2), FP.f32.toInterval(3)],
        [FP.f32.toInterval(4), FP.f32.toInterval(5), FP.f32.toInterval(6)],
        [FP.f32.toInterval(7), FP.f32.toInterval(8), FP.f32.toInterval(9)],
      ],
      expected: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
      ],
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2), FP.f32.toInterval(3)],
        [FP.f32.toInterval(4), FP.f32.toInterval(5), FP.f32.toInterval(6)],
        [FP.f32.toInterval(7), FP.f32.toInterval(8), FP.f32.toInterval(9)],
        [FP.f32.toInterval(10), FP.f32.toInterval(11), FP.f32.toInterval(12)],
      ],
      expected: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
        [10, 11, 12],
      ],
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2), FP.f32.toInterval(3), FP.f32.toInterval(4)],
        [FP.f32.toInterval(5), FP.f32.toInterval(6), FP.f32.toInterval(7), FP.f32.toInterval(8)],
      ],
      expected: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
      ],
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2), FP.f32.toInterval(3), FP.f32.toInterval(4)],
        [FP.f32.toInterval(5), FP.f32.toInterval(6), FP.f32.toInterval(7), FP.f32.toInterval(8)],
        [FP.f32.toInterval(9), FP.f32.toInterval(10), FP.f32.toInterval(11), FP.f32.toInterval(12)],
      ],
      expected: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
      ],
    },
    {
      input: [
        [FP.f32.toInterval(1), FP.f32.toInterval(2), FP.f32.toInterval(3), FP.f32.toInterval(4)],
        [FP.f32.toInterval(5), FP.f32.toInterval(6), FP.f32.toInterval(7), FP.f32.toInterval(8)],
        [FP.f32.toInterval(9), FP.f32.toInterval(10), FP.f32.toInterval(11), FP.f32.toInterval(12)],
        [
          FP.f32.toInterval(13),
          FP.f32.toInterval(14),
          FP.f32.toInterval(15),
          FP.f32.toInterval(16),
        ],
      ],
      expected: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
        [13, 14, 15, 16],
      ],
    },

    {
      input: [
        [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3])],
        [FP.f32.toInterval([3, 4]), FP.f32.toInterval([4, 5])],
      ],
      expected: [
        [
          [1, 2],
          [2, 3],
        ],
        [
          [3, 4],
          [4, 5],
        ],
      ],
    },
    {
      input: [
        [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3])],
        [FP.f32.toInterval([3, 4]), FP.f32.toInterval([4, 5])],
        [FP.f32.toInterval([5, 6]), FP.f32.toInterval([6, 7])],
      ],
      expected: [
        [
          [1, 2],
          [2, 3],
        ],
        [
          [3, 4],
          [4, 5],
        ],
        [
          [5, 6],
          [6, 7],
        ],
      ],
    },
    {
      input: [
        [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3])],
        [FP.f32.toInterval([3, 4]), FP.f32.toInterval([4, 5])],
        [FP.f32.toInterval([5, 6]), FP.f32.toInterval([6, 7])],
        [FP.f32.toInterval([7, 8]), FP.f32.toInterval([8, 9])],
      ],
      expected: [
        [
          [1, 2],
          [2, 3],
        ],
        [
          [3, 4],
          [4, 5],
        ],
        [
          [5, 6],
          [6, 7],
        ],
        [
          [7, 8],
          [8, 9],
        ],
      ],
    },
    {
      input: [
        [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3]), FP.f32.toInterval([3, 4])],
        [FP.f32.toInterval([4, 5]), FP.f32.toInterval([5, 6]), FP.f32.toInterval([6, 7])],
      ],
      expected: [
        [
          [1, 2],
          [2, 3],
          [3, 4],
        ],
        [
          [4, 5],
          [5, 6],
          [6, 7],
        ],
      ],
    },
    {
      input: [
        [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3]), FP.f32.toInterval([3, 4])],
        [FP.f32.toInterval([4, 5]), FP.f32.toInterval([5, 6]), FP.f32.toInterval([6, 7])],
        [FP.f32.toInterval([7, 8]), FP.f32.toInterval([8, 9]), FP.f32.toInterval([9, 10])],
      ],
      expected: [
        [
          [1, 2],
          [2, 3],
          [3, 4],
        ],
        [
          [4, 5],
          [5, 6],
          [6, 7],
        ],
        [
          [7, 8],
          [8, 9],
          [9, 10],
        ],
      ],
    },
    {
      input: [
        [FP.f32.toInterval([1, 2]), FP.f32.toInterval([2, 3]), FP.f32.toInterval([3, 4])],
        [FP.f32.toInterval([4, 5]), FP.f32.toInterval([5, 6]), FP.f32.toInterval([6, 7])],
        [FP.f32.toInterval([7, 8]), FP.f32.toInterval([8, 9]), FP.f32.toInterval([9, 10])],
        [FP.f32.toInterval([10, 11]), FP.f32.toInterval([11, 12]), FP.f32.toInterval([12, 13])],
      ],
      expected: [
        [
          [1, 2],
          [2, 3],
          [3, 4],
        ],
        [
          [4, 5],
          [5, 6],
          [6, 7],
        ],
        [
          [7, 8],
          [8, 9],
          [9, 10],
        ],
        [
          [10, 11],
          [11, 12],
          [12, 13],
        ],
      ],
    },
    {
      input: [
        [
          FP.f32.toInterval([1, 2]),
          FP.f32.toInterval([2, 3]),
          FP.f32.toInterval([3, 4]),
          FP.f32.toInterval([4, 5]),
        ],
        [
          FP.f32.toInterval([5, 6]),
          FP.f32.toInterval([6, 7]),
          FP.f32.toInterval([7, 8]),
          FP.f32.toInterval([8, 9]),
        ],
      ],
      expected: [
        [
          [1, 2],
          [2, 3],
          [3, 4],
          [4, 5],
        ],
        [
          [5, 6],
          [6, 7],
          [7, 8],
          [8, 9],
        ],
      ],
    },
    {
      input: [
        [
          FP.f32.toInterval([1, 2]),
          FP.f32.toInterval([2, 3]),
          FP.f32.toInterval([3, 4]),
          FP.f32.toInterval([4, 5]),
        ],
        [
          FP.f32.toInterval([5, 6]),
          FP.f32.toInterval([6, 7]),
          FP.f32.toInterval([7, 8]),
          FP.f32.toInterval([8, 9]),
        ],
        [
          FP.f32.toInterval([9, 10]),
          FP.f32.toInterval([10, 11]),
          FP.f32.toInterval([11, 12]),
          FP.f32.toInterval([12, 13]),
        ],
      ],
      expected: [
        [
          [1, 2],
          [2, 3],
          [3, 4],
          [4, 5],
        ],
        [
          [5, 6],
          [6, 7],
          [7, 8],
          [8, 9],
        ],
        [
          [9, 10],
          [10, 11],
          [11, 12],
          [12, 13],
        ],
      ],
    },
    {
      input: [
        [
          FP.f32.toInterval([1, 2]),
          FP.f32.toInterval([2, 3]),
          FP.f32.toInterval([3, 4]),
          FP.f32.toInterval([4, 5]),
        ],
        [
          FP.f32.toInterval([5, 6]),
          FP.f32.toInterval([6, 7]),
          FP.f32.toInterval([7, 8]),
          FP.f32.toInterval([8, 9]),
        ],
        [
          FP.f32.toInterval([9, 10]),
          FP.f32.toInterval([10, 11]),
          FP.f32.toInterval([11, 12]),
          FP.f32.toInterval([12, 13]),
        ],
        [
          FP.f32.toInterval([13, 14]),
          FP.f32.toInterval([14, 15]),
          FP.f32.toInterval([15, 16]),
          FP.f32.toInterval([16, 17]),
        ],
      ],
      expected: [
        [
          [1, 2],
          [2, 3],
          [3, 4],
          [4, 5],
        ],
        [
          [5, 6],
          [6, 7],
          [7, 8],
          [8, 9],
        ],
        [
          [9, 10],
          [10, 11],
          [11, 12],
          [12, 13],
        ],
        [
          [13, 14],
          [14, 15],
          [15, 16],
          [16, 17],
        ],
      ],
    },

    // Mixed
    {
      input: [
        [1, [2]],
        [3, 4],
      ],
      expected: [
        [1, 2],
        [3, 4],
      ],
    },
    {
      input: [
        [[1], [2]],
        [[3], 4],
      ],
      expected: [
        [1, 2],
        [3, 4],
      ],
    },
    {
      input: [
        [1, 2],
        [FP.f32.toInterval([3]), 4],
      ],
      expected: [
        [1, 2],
        [3, 4],
      ],
    },
    {
      input: [
        [[1], FP.f32.toInterval([2])],
        [FP.f32.toInterval([3]), FP.f32.toInterval([4])],
      ],
      expected: [
        [1, 2],
        [3, 4],
      ],
    },
  ])
  .fn(t => {
    const input = t.params.input;
    const expected = map2DArray(t.params.expected, e => FP.f32.toInterval(e));

    const got = FP.f32.toMatrix(input);
    t.expect(
      objectEquals(got, expected),
      `f32.toMatrix([${input}]) returned [${got}]. Expected [${expected}]`
    );
  });

// API - Fundamental Error Intervals

interface AbsoluteErrorCase {
  value: number;
  error: number;
  expected: number | IntervalBounds;
}

g.test('absoluteErrorInterval_f32')
  .paramsSubcasesOnly<AbsoluteErrorCase>(
    // prettier-ignore
    [
      // Edge Cases
      { value: kValue.f32.infinity.positive, error: 0, expected: kAnyBounds },
      { value: kValue.f32.infinity.positive, error: 2 ** -11, expected: kAnyBounds },
      { value: kValue.f32.infinity.positive, error: 1, expected: kAnyBounds },
      { value: kValue.f32.infinity.negative, error: 0, expected: kAnyBounds },
      { value: kValue.f32.infinity.negative, error: 2 ** -11, expected: kAnyBounds },
      { value: kValue.f32.infinity.negative, error: 1, expected: kAnyBounds },
      { value: kValue.f32.positive.max, error: 0, expected: kValue.f32.positive.max },
      { value: kValue.f32.positive.max, error: 2 ** -11, expected: kValue.f32.positive.max },
      { value: kValue.f32.positive.max, error: kValue.f32.positive.max, expected: kAnyBounds },
      { value: kValue.f32.positive.min, error: 0, expected: kValue.f32.positive.min },
      { value: kValue.f32.positive.min, error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: kValue.f32.positive.min, error: 1, expected: [-1, 1] },
      { value: kValue.f32.negative.min, error: 0, expected: kValue.f32.negative.min },
      { value: kValue.f32.negative.min, error: 2 ** -11, expected: kValue.f32.negative.min },
      { value: kValue.f32.negative.min, error: kValue.f32.positive.max, expected: kAnyBounds },
      { value: kValue.f32.negative.max, error: 0, expected: kValue.f32.negative.max },
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
      { value: hexToF64(0x0000_0000_0000_0001n), error: 0, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x0000_0000_0000_0001n), error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: hexToF64(0x0000_0000_0000_0001n), error: 1, expected: [-1, 1] },
      { value: hexToF64(0x0000_0000_0000_0002n), error: 0, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x0000_0000_0000_0002n), error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: hexToF64(0x0000_0000_0000_0002n), error: 1, expected: [-1, 1] },
      { value: hexToF64(0x800f_ffff_ffff_ffffn), error: 0, expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: hexToF64(0x800f_ffff_ffff_ffffn), error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: hexToF64(0x800f_ffff_ffff_ffffn), error: 1, expected: [-1, 1] },
      { value: hexToF64(0x800f_ffff_ffff_fffen), error: 0, expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: hexToF64(0x800f_ffff_ffff_fffen), error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: hexToF64(0x800f_ffff_ffff_fffen), error: 1, expected: [-1, 1] },

      // Zero
      { value: 0, error: 0, expected: 0 },
      { value: 0, error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: 0, error: 1, expected: [-1, 1] },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.absoluteErrorInterval(t.params.value, t.params.error);
    t.expect(
      objectEquals(expected, got),
      `f32.absoluteErrorInterval(${t.params.value}, ${t.params.error}) returned ${got}. Expected ${expected}`
    );
  });

interface CorrectlyRoundedCase {
  value: number;
  expected: number | IntervalBounds;
}

g.test('correctlyRoundedInterval_f32')
  .paramsSubcasesOnly<CorrectlyRoundedCase>(
    // prettier-ignore
    [
      // Edge Cases
      { value: kValue.f32.infinity.positive, expected: kAnyBounds },
      { value: kValue.f32.infinity.negative, expected: kAnyBounds },
      { value: kValue.f32.positive.max, expected: kValue.f32.positive.max },
      { value: kValue.f32.negative.min, expected: kValue.f32.negative.min },
      { value: kValue.f32.positive.min, expected: kValue.f32.positive.min },
      { value: kValue.f32.negative.max, expected: kValue.f32.negative.max },

      // 32-bit subnormals
      { value: kValue.f32.subnormal.positive.min, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: kValue.f32.subnormal.positive.max, expected: [0, kValue.f32.subnormal.positive.max] },
      { value: kValue.f32.subnormal.negative.min, expected: [kValue.f32.subnormal.negative.min, 0] },
      { value: kValue.f32.subnormal.negative.max, expected: [kValue.f32.subnormal.negative.max, 0] },

      // 64-bit subnormals
      { value: hexToF64(0x0000_0000_0000_0001n), expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x0000_0000_0000_0002n), expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x800f_ffff_ffff_ffffn), expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: hexToF64(0x800f_ffff_ffff_fffen), expected: [kValue.f32.subnormal.negative.max, 0] },

      // 32-bit normals
      { value: 0, expected: [0, 0] },
      { value: hexToF32(0x03800000), expected: hexToF32(0x03800000) },
      { value: hexToF32(0x03800001), expected: hexToF32(0x03800001) },
      { value: hexToF32(0x83800000), expected: hexToF32(0x83800000) },
      { value: hexToF32(0x83800001), expected: hexToF32(0x83800001) },

      // 64-bit normals
      { value: hexToF64(0x3ff0_0000_0000_0001n), expected: [hexToF32(0x3f800000), hexToF32(0x3f800001)] },
      { value: hexToF64(0x3ff0_0000_0000_0002n), expected: [hexToF32(0x3f800000), hexToF32(0x3f800001)] },
      { value: hexToF64(0x3ff0_0010_0000_0010n), expected: [hexToF32(0x3f800080), hexToF32(0x3f800081)] },
      { value: hexToF64(0x3ff0_0020_0000_0020n), expected: [hexToF32(0x3f800100), hexToF32(0x3f800101)] },
      { value: hexToF64(0xbff0_0000_0000_0001n), expected: [hexToF32(0xbf800001), hexToF32(0xbf800000)] },
      { value: hexToF64(0xbff0_0000_0000_0002n), expected: [hexToF32(0xbf800001), hexToF32(0xbf800000)] },
      { value: hexToF64(0xbff0_0010_0000_0010n), expected: [hexToF32(0xbf800081), hexToF32(0xbf800080)] },
      { value: hexToF64(0xbff0_0020_0000_0020n), expected: [hexToF32(0xbf800101), hexToF32(0xbf800100)] },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.correctlyRoundedInterval(t.params.value);
    t.expect(
      objectEquals(expected, got),
      `f32.correctlyRoundedInterval(${t.params.value}) returned ${got}. Expected ${expected}`
    );
  });

interface ULPCase {
  value: number;
  num_ulp: number;
  expected: number | IntervalBounds;
}

g.test('ulpInterval_f32')
  .paramsSubcasesOnly<ULPCase>(
    // prettier-ignore
    [
      // Edge Cases
      { value: kValue.f32.infinity.positive, num_ulp: 0, expected: kAnyBounds },
      { value: kValue.f32.infinity.positive, num_ulp: 1, expected: kAnyBounds },
      { value: kValue.f32.infinity.positive, num_ulp: 4096, expected: kAnyBounds },
      { value: kValue.f32.infinity.negative, num_ulp: 0, expected: kAnyBounds },
      { value: kValue.f32.infinity.negative, num_ulp: 1, expected: kAnyBounds },
      { value: kValue.f32.infinity.negative, num_ulp: 4096, expected: kAnyBounds },
      { value: kValue.f32.positive.max, num_ulp: 0, expected: kValue.f32.positive.max },
      { value: kValue.f32.positive.max, num_ulp: 1, expected: kAnyBounds },
      { value: kValue.f32.positive.max, num_ulp: 4096, expected: kAnyBounds },
      { value: kValue.f32.positive.min, num_ulp: 0, expected: kValue.f32.positive.min },
      { value: kValue.f32.positive.min, num_ulp: 1, expected: [0, plusOneULPF32(kValue.f32.positive.min)] },
      { value: kValue.f32.positive.min, num_ulp: 4096, expected: [0, plusNULPF32(kValue.f32.positive.min, 4096)] },
      { value: kValue.f32.negative.min, num_ulp: 0, expected: kValue.f32.negative.min },
      { value: kValue.f32.negative.min, num_ulp: 1, expected: kAnyBounds },
      { value: kValue.f32.negative.min, num_ulp: 4096, expected: kAnyBounds },
      { value: kValue.f32.negative.max, num_ulp: 0, expected: kValue.f32.negative.max },
      { value: kValue.f32.negative.max, num_ulp: 1, expected: [minusOneULPF32(kValue.f32.negative.max), 0] },
      { value: kValue.f32.negative.max, num_ulp: 4096, expected: [minusNULPF32(kValue.f32.negative.max, 4096), 0] },

      // 32-bit subnormals
      { value: kValue.f32.subnormal.positive.max, num_ulp: 0, expected: [0, kValue.f32.subnormal.positive.max] },
      { value: kValue.f32.subnormal.positive.max, num_ulp: 1, expected: [minusOneULPF32(0), plusOneULPF32(kValue.f32.subnormal.positive.max)] },
      { value: kValue.f32.subnormal.positive.max, num_ulp: 4096, expected: [minusNULPF32(0, 4096), plusNULPF32(kValue.f32.subnormal.positive.max, 4096)] },
      { value: kValue.f32.subnormal.positive.min, num_ulp: 0, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: kValue.f32.subnormal.positive.min, num_ulp: 1, expected: [minusOneULPF32(0), plusOneULPF32(kValue.f32.subnormal.positive.min)] },
      { value: kValue.f32.subnormal.positive.min, num_ulp: 4096, expected: [minusNULPF32(0, 4096), plusNULPF32(kValue.f32.subnormal.positive.min, 4096)] },
      { value: kValue.f32.subnormal.negative.min, num_ulp: 0, expected: [kValue.f32.subnormal.negative.min, 0] },
      { value: kValue.f32.subnormal.negative.min, num_ulp: 1, expected: [minusOneULPF32(kValue.f32.subnormal.negative.min), plusOneULPF32(0)] },
      { value: kValue.f32.subnormal.negative.min, num_ulp: 4096, expected: [minusNULPF32(kValue.f32.subnormal.negative.min, 4096), plusNULPF32(0, 4096)] },
      { value: kValue.f32.subnormal.negative.max, num_ulp: 0, expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: kValue.f32.subnormal.negative.max, num_ulp: 1, expected: [minusOneULPF32(kValue.f32.subnormal.negative.max), plusOneULPF32(0)] },
      { value: kValue.f32.subnormal.negative.max, num_ulp: 4096, expected: [minusNULPF32(kValue.f32.subnormal.negative.max, 4096), plusNULPF32(0, 4096)] },

      // 64-bit subnormals
      { value: hexToF64(0x0000_0000_0000_0001n), num_ulp: 0, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x0000_0000_0000_0001n), num_ulp: 1, expected: [minusOneULPF32(0), plusOneULPF32(kValue.f32.subnormal.positive.min)] },
      { value: hexToF64(0x0000_0000_0000_0001n), num_ulp: 4096, expected: [minusNULPF32(0, 4096), plusNULPF32(kValue.f32.subnormal.positive.min, 4096)] },
      { value: hexToF64(0x0000_0000_0000_0002n), num_ulp: 0, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x0000_0000_0000_0002n), num_ulp: 1, expected: [minusOneULPF32(0), plusOneULPF32(kValue.f32.subnormal.positive.min)] },
      { value: hexToF64(0x0000_0000_0000_0002n), num_ulp: 4096, expected: [minusNULPF32(0, 4096), plusNULPF32(kValue.f32.subnormal.positive.min, 4096)] },
      { value: hexToF64(0x800f_ffff_ffff_ffffn), num_ulp: 0, expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: hexToF64(0x800f_ffff_ffff_ffffn), num_ulp: 1, expected: [minusOneULPF32(kValue.f32.subnormal.negative.max), plusOneULPF32(0)] },
      { value: hexToF64(0x800f_ffff_ffff_ffffn), num_ulp: 4096, expected: [minusNULPF32(kValue.f32.subnormal.negative.max, 4096), plusNULPF32(0, 4096)] },
      { value: hexToF64(0x800f_ffff_ffff_fffen), num_ulp: 0, expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: hexToF64(0x800f_ffff_ffff_fffen), num_ulp: 1, expected: [minusOneULPF32(kValue.f32.subnormal.negative.max), plusOneULPF32(0)] },
      { value: hexToF64(0x800f_ffff_ffff_fffen), num_ulp: 4096, expected: [minusNULPF32(kValue.f32.subnormal.negative.max, 4096), plusNULPF32(0, 4096)] },

      // Zero
      { value: 0, num_ulp: 0, expected: 0 },
      { value: 0, num_ulp: 1, expected: [minusOneULPF32(0), plusOneULPF32(0)] },
      { value: 0, num_ulp: 4096, expected: [minusNULPF32(0, 4096), plusNULPF32(0, 4096)] },
    ]
  )
  .fn(t => {
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.ulpInterval(t.params.value, t.params.num_ulp);
    t.expect(
      objectEquals(expected, got),
      `f32.ulpInterval(${t.params.value}, ${t.params.num_ulp}) returned ${got}. Expected ${expected}`
    );
  });

// API - Acceptance Intervals

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

interface MatrixToScalarCase {
  input: number[][];
  expected: number | IntervalBounds;
}

g.test('determinantInterval_f32')
  .paramsSubcasesOnly<MatrixToScalarCase>([
    // Extreme values, i.e. subnormals, very large magnitudes, and those lead to
    // non-precise products, are intentionally not tested, since the accuracy of
    // determinant is restricted to well behaving inputs. Handling all cases
    // requires ~23! options to be calculated in the 4x4 case, so is not
    // feasible.
    {
      input: [
        [1, 2],
        [3, 4],
      ],
      expected: -2,
    },
    {
      input: [
        [-1, 2],
        [-3, 4],
      ],
      expected: 2,
    },
    {
      input: [
        [11, 22],
        [33, 44],
      ],
      expected: -242,
    },
    {
      input: [
        [5, 6],
        [8, 9],
      ],
      expected: -3,
    },
    {
      input: [
        [4, 6],
        [7, 9],
      ],
      expected: -6,
    },
    {
      input: [
        [4, 5],
        [7, 8],
      ],
      expected: -3,
    },
    {
      input: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
      ],
      expected: 0,
    },
    {
      input: [
        [-1, 2, 3],
        [-4, 5, 6],
        [-7, 8, 9],
      ],
      expected: 0,
    },
    {
      input: [
        [11, 22, 33],
        [44, 55, 66],
        [77, 88, 99],
      ],
      expected: 0,
    },
    {
      input: [
        [4, 1, -1],
        [-3, 0, 5],
        [5, 3, 2],
      ],
      expected: -20,
    },
    {
      input: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
        [13, 14, 15, 16],
      ],
      expected: 0,
    },
    {
      input: [
        [4, 0, 0, 0],
        [3, 1, -1, 3],
        [2, -3, 3, 1],
        [2, 3, 3, 1],
      ],
      expected: -240,
    },
  ])
  .fn(t => {
    const input = t.params.input;
    const expected = FP.f32.toInterval(t.params.expected);
    const got = FP.f32.determinantInterval(input);
    t.expect(
      objectEquals(expected, got),
      `f32.determinantInterval([${JSON.stringify(input)}]) returned '${got}. Expected '${expected}'`
    );
  });

interface MatrixToMatrixCase {
  input: number[][];
  expected: (number | IntervalBounds)[][];
}

g.test('transposeInterval_f32')
  .paramsSubcasesOnly<MatrixToMatrixCase>([
    {
      input: [
        [1, 2],
        [3, 4],
      ],
      expected: [
        [1, 3],
        [2, 4],
      ],
    },
    {
      input: [
        [1, 2],
        [3, 4],
        [5, 6],
      ],
      expected: [
        [1, 3, 5],
        [2, 4, 6],
      ],
    },
    {
      input: [
        [1, 2],
        [3, 4],
        [5, 6],
        [7, 8],
      ],
      expected: [
        [1, 3, 5, 7],
        [2, 4, 6, 8],
      ],
    },
    {
      input: [
        [1, 2, 3],
        [4, 5, 6],
      ],
      expected: [
        [1, 4],
        [2, 5],
        [3, 6],
      ],
    },
    {
      input: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
      ],
      expected: [
        [1, 4, 7],
        [2, 5, 8],
        [3, 6, 9],
      ],
    },
    {
      input: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
        [10, 11, 12],
      ],
      expected: [
        [1, 4, 7, 10],
        [2, 5, 8, 11],
        [3, 6, 9, 12],
      ],
    },
    {
      input: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
      ],
      expected: [
        [1, 5],
        [2, 6],
        [3, 7],
        [4, 8],
      ],
    },
    {
      input: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
      ],
      expected: [
        [1, 5, 9],
        [2, 6, 10],
        [3, 7, 11],
        [4, 8, 12],
      ],
    },
    {
      input: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
        [13, 14, 15, 16],
      ],
      expected: [
        [1, 5, 9, 13],
        [2, 6, 10, 14],
        [3, 7, 11, 15],
        [4, 8, 12, 16],
      ],
    },
    {
      input: [
        [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.min],
        [kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max],
      ],
      expected: [
        [
          [0, kValue.f32.subnormal.positive.max],
          [kValue.f32.subnormal.negative.min, 0],
        ],
        [
          [0, kValue.f32.subnormal.positive.min],
          [kValue.f32.subnormal.negative.max, 0],
        ],
      ],
    },
  ])
  .fn(t => {
    const input = t.params.input;
    const expected = FP.f32.toMatrix(t.params.expected);
    const got = FP.f32.transposeInterval(input);
    t.expect(
      objectEquals(expected, got),
      `f32.transposeInterval([${JSON.stringify(input)}]) returned '[${JSON.stringify(
        got
      )}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

interface MatrixPairToMatrixCase {
  input: [number[][], number[][]];
  expected: (number | IntervalBounds)[][];
}

g.test('additionMatrixMatrixInterval_f32')
  .paramsSubcasesOnly<MatrixPairToMatrixCase>([
    // Only testing that different shapes of matrices are handled correctly
    // here, to reduce test duplication.
    // additionMatrixMatrixInterval uses AdditionIntervalOp for calculating intervals,
    // so the testing for additionInterval covers the actual interval
    // calculations.
    {
      input: [
        [
          [1, 2],
          [3, 4],
        ],
        [
          [10, 20],
          [30, 40],
        ],
      ],
      expected: [
        [11, 22],
        [33, 44],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
        ],
        [
          [10, 20],
          [30, 40],
          [50, 60],
        ],
      ],
      expected: [
        [11, 22],
        [33, 44],
        [55, 66],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
          [7, 8],
        ],
        [
          [10, 20],
          [30, 40],
          [50, 60],
          [70, 80],
        ],
      ],
      expected: [
        [11, 22],
        [33, 44],
        [55, 66],
        [77, 88],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
        ],
        [
          [10, 20, 30],
          [40, 50, 60],
        ],
      ],
      expected: [
        [11, 22, 33],
        [44, 55, 66],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
        ],
        [
          [10, 20, 30],
          [40, 50, 60],
          [70, 80, 90],
        ],
      ],
      expected: [
        [11, 22, 33],
        [44, 55, 66],
        [77, 88, 99],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
          [10, 11, 12],
        ],
        [
          [10, 20, 30],
          [40, 50, 60],
          [70, 80, 90],
          [1000, 1100, 1200],
        ],
      ],
      expected: [
        [11, 22, 33],
        [44, 55, 66],
        [77, 88, 99],
        [1010, 1111, 1212],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
        ],
        [
          [10, 20, 30, 40],
          [50, 60, 70, 80],
        ],
      ],
      expected: [
        [11, 22, 33, 44],
        [55, 66, 77, 88],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
          [9, 10, 11, 12],
        ],
        [
          [10, 20, 30, 40],
          [50, 60, 70, 80],
          [90, 1000, 1100, 1200],
        ],
      ],
      expected: [
        [11, 22, 33, 44],
        [55, 66, 77, 88],
        [99, 1010, 1111, 1212],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
          [9, 10, 11, 12],
          [13, 14, 15, 16],
        ],
        [
          [10, 20, 30, 40],
          [50, 60, 70, 80],
          [90, 1000, 1100, 1200],
          [1300, 1400, 1500, 1600],
        ],
      ],
      expected: [
        [11, 22, 33, 44],
        [55, 66, 77, 88],
        [99, 1010, 1111, 1212],
        [1313, 1414, 1515, 1616],
      ],
    },
  ])
  .fn(t => {
    const x = t.params.input[0];
    const y = t.params.input[1];
    const expected = FP.f32.toMatrix(t.params.expected);
    const got = FP.f32.additionMatrixMatrixInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.additionMatrixMatrixInterval([${JSON.stringify(x)}], [${JSON.stringify(
        y
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

g.test('subtractionMatrixMatrixInterval_f32')
  .paramsSubcasesOnly<MatrixPairToMatrixCase>([
    // Only testing that different shapes of matrices are handled correctly
    // here, to reduce test duplication.
    // subtractionMatrixMatrixInterval uses SubtractionIntervalOp for calculating intervals,
    // so the testing for subtractionInterval covers the actual interval
    // calculations.
    {
      input: [
        [
          [-1, -2],
          [-3, -4],
        ],
        [
          [10, 20],
          [30, 40],
        ],
      ],
      expected: [
        [-11, -22],
        [-33, -44],
      ],
    },
    {
      input: [
        [
          [-1, -2],
          [-3, -4],
          [-5, -6],
        ],
        [
          [10, 20],
          [30, 40],
          [50, 60],
        ],
      ],
      expected: [
        [-11, -22],
        [-33, -44],
        [-55, -66],
      ],
    },
    {
      input: [
        [
          [-1, -2],
          [-3, -4],
          [-5, -6],
          [-7, -8],
        ],
        [
          [10, 20],
          [30, 40],
          [50, 60],
          [70, 80],
        ],
      ],
      expected: [
        [-11, -22],
        [-33, -44],
        [-55, -66],
        [-77, -88],
      ],
    },
    {
      input: [
        [
          [-1, -2, -3],
          [-4, -5, -6],
        ],
        [
          [10, 20, 30],
          [40, 50, 60],
        ],
      ],
      expected: [
        [-11, -22, -33],
        [-44, -55, -66],
      ],
    },
    {
      input: [
        [
          [-1, -2, -3],
          [-4, -5, -6],
          [-7, -8, -9],
        ],
        [
          [10, 20, 30],
          [40, 50, 60],
          [70, 80, 90],
        ],
      ],
      expected: [
        [-11, -22, -33],
        [-44, -55, -66],
        [-77, -88, -99],
      ],
    },
    {
      input: [
        [
          [-1, -2, -3],
          [-4, -5, -6],
          [-7, -8, -9],
          [-10, -11, -12],
        ],
        [
          [10, 20, 30],
          [40, 50, 60],
          [70, 80, 90],
          [1000, 1100, 1200],
        ],
      ],
      expected: [
        [-11, -22, -33],
        [-44, -55, -66],
        [-77, -88, -99],
        [-1010, -1111, -1212],
      ],
    },
    {
      input: [
        [
          [-1, -2, -3, -4],
          [-5, -6, -7, -8],
        ],
        [
          [10, 20, 30, 40],
          [50, 60, 70, 80],
        ],
      ],
      expected: [
        [-11, -22, -33, -44],
        [-55, -66, -77, -88],
      ],
    },
    {
      input: [
        [
          [-1, -2, -3, -4],
          [-5, -6, -7, -8],
          [-9, -10, -11, -12],
        ],
        [
          [10, 20, 30, 40],
          [50, 60, 70, 80],
          [90, 1000, 1100, 1200],
        ],
      ],
      expected: [
        [-11, -22, -33, -44],
        [-55, -66, -77, -88],
        [-99, -1010, -1111, -1212],
      ],
    },
    {
      input: [
        [
          [-1, -2, -3, -4],
          [-5, -6, -7, -8],
          [-9, -10, -11, -12],
          [-13, -14, -15, -16],
        ],
        [
          [10, 20, 30, 40],
          [50, 60, 70, 80],
          [90, 1000, 1100, 1200],
          [1300, 1400, 1500, 1600],
        ],
      ],
      expected: [
        [-11, -22, -33, -44],
        [-55, -66, -77, -88],
        [-99, -1010, -1111, -1212],
        [-1313, -1414, -1515, -1616],
      ],
    },
  ])
  .fn(t => {
    const x = t.params.input[0];
    const y = t.params.input[1];
    const expected = FP.f32.toMatrix(t.params.expected);
    const got = FP.f32.subtractionMatrixMatrixInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.subtractionMatrixMatrixInterval([${JSON.stringify(x)}], [${JSON.stringify(
        y
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

g.test('multiplicationMatrixMatrixInterval_f32')
  .paramsSubcasesOnly<MatrixPairToMatrixCase>([
    // Only testing that different shapes of matrices are handled correctly
    // here, to reduce test duplication.
    // multiplicationMatrixMatrixInterval uses and transposeInterval &
    // dotInterval for calculating intervals, so the testing for those functions
    // will cover the actual interval calculations.
    {
      input: [
        [
          [1, 2],
          [3, 4],
        ],
        [
          [11, 22],
          [33, 44],
        ],
      ],
      expected: [
        [77, 110],
        [165, 242],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
        ],
        [
          [11, 22],
          [33, 44],
          [55, 66],
        ],
      ],
      expected: [
        [77, 110],
        [165, 242],
        [253, 374],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
        ],
        [
          [11, 22],
          [33, 44],
          [55, 66],
          [77, 88],
        ],
      ],
      expected: [
        [77, 110],
        [165, 242],
        [253, 374],
        [341, 506],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
        ],
        [
          [11, 22],
          [33, 44],
        ],
      ],
      expected: [
        [99, 132, 165],
        [209, 286, 363],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
        ],
        [
          [11, 22],
          [33, 44],
          [55, 66],
        ],
      ],
      expected: [
        [99, 132, 165],
        [209, 286, 363],
        [319, 440, 561],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
        ],
        [
          [11, 22],
          [33, 44],
          [55, 66],
          [77, 88],
        ],
      ],
      expected: [
        [99, 132, 165],
        [209, 286, 363],
        [319, 440, 561],
        [429, 594, 759],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
        ],
        [
          [11, 22],
          [33, 44],
        ],
      ],
      expected: [
        [121, 154, 187, 220],
        [253, 330, 407, 484],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
        ],
        [
          [11, 22],
          [33, 44],
          [55, 66],
          [77, 88],
        ],
      ],
      expected: [
        [121, 154, 187, 220],
        [253, 330, 407, 484],
        [385, 506, 627, 748],
        [517, 682, 847, 1012],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
        ],
      ],
      expected: [
        [242, 308],
        [539, 704],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
          [77, 88, 99],
        ],
      ],
      expected: [
        [242, 308],
        [539, 704],
        [836, 1100],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
          [77, 88, 99],
          [1010, 1111, 1212],
        ],
      ],
      expected: [
        [242, 308],
        [539, 704],
        [836, 1100],
        [10403, 13736],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
        ],
      ],
      expected: [
        [330, 396, 462],
        [726, 891, 1056],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
          [77, 88, 99],
        ],
      ],
      expected: [
        [330, 396, 462],
        [726, 891, 1056],
        [1122, 1386, 1650],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
          [77, 88, 99],
          [1010, 1111, 1212],
        ],
      ],
      expected: [
        [330, 396, 462],
        [726, 891, 1056],
        [1122, 1386, 1650],
        [13938, 17271, 20604],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
          [9, 11, 11, 12],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
        ],
      ],
      expected: [
        [418, 517, 550, 616],
        [913, 1144, 1243, 1408],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
          [9, 11, 11, 12],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
          [77, 88, 99],
        ],
      ],
      expected: [
        [418, 517, 550, 616],
        [913, 1144, 1243, 1408],
        [1408, 1771, 1936, 2200],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
          [9, 11, 11, 12],
        ],
        [
          [11, 22, 33],
          [44, 55, 66],
          [77, 88, 99],
          [1010, 1111, 1212],
        ],
      ],
      expected: [
        [418, 517, 550, 616],
        [913, 1144, 1243, 1408],
        [1408, 1771, 1936, 2200],
        [17473, 22018, 24139, 27472],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
          [7, 8],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
        ],
      ],
      expected: [
        [550, 660],
        [1254, 1540],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
          [7, 8],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
          [99, 1010, 1111, 1212],
        ],
      ],
      expected: [
        [550, 660],
        [1254, 1540],
        [17168, 20600],
      ],
    },
    {
      input: [
        [
          [1, 2],
          [3, 4],
          [5, 6],
          [7, 8],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
          [99, 1010, 1111, 1212],
          [1313, 1414, 1515, 1616],
        ],
      ],
      expected: [
        [550, 660],
        [1254, 1540],
        [17168, 20600],
        [24442, 30300],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
          [11, 11, 12],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
        ],
      ],
      expected: [
        [814, 880, 990],
        [1826, 2024, 2310],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
          [11, 11, 12],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
          [99, 1010, 1111, 1212],
        ],
      ],
      expected: [
        [814, 880, 990],
        [1826, 2024, 2310],
        [25248, 27468, 30900],
      ],
    },
    {
      input: [
        [
          [1, 2, 3],
          [4, 5, 6],
          [7, 8, 9],
          [11, 11, 12],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
          [99, 1010, 1111, 1212],
          [1313, 1414, 1515, 1616],
        ],
      ],
      expected: [
        [814, 880, 990],
        [1826, 2024, 2310],
        [25248, 27468, 30900],
        [35350, 39592, 45450],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
          [9, 11, 11, 12],
          [13, 14, 15, 16],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
          [99, 1010, 1111, 1212],
        ],
      ],
      expected: [
        [990, 1133, 1210, 1320],
        [2222, 2585, 2794, 3080],
        [30904, 35447, 37768, 41200],
      ],
    },
    {
      input: [
        [
          [1, 2, 3, 4],
          [5, 6, 7, 8],
          [9, 11, 11, 12],
          [13, 14, 15, 16],
        ],
        [
          [11, 22, 33, 44],
          [55, 66, 77, 88],
          [99, 1010, 1111, 1212],
          [1313, 1414, 1515, 1616],
        ],
      ],
      expected: [
        [990, 1133, 1210, 1320],
        [2222, 2585, 2794, 3080],
        [30904, 35447, 37768, 41200],
        [43026, 50399, 54742, 60600],
      ],
    },
  ])
  .fn(t => {
    const [x, y] = t.params.input;
    const expected = FP.f32.toMatrix(t.params.expected);
    const got = FP.f32.multiplicationMatrixMatrixInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `f32.multiplicationMatrixMatrixInterval([${JSON.stringify(x)}], [${JSON.stringify(
        y
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

interface MatrixScalarToMatrixCase {
  matrix: number[][];
  scalar: number;
  expected: (number | IntervalBounds)[][];
}

g.test('multiplicationMatrixScalarInterval_f32')
  .paramsSubcasesOnly<MatrixScalarToMatrixCase>([
    // Only testing that different shapes of matrices are handled correctly
    // here, to reduce test duplication.
    // multiplicationMatrixScalarInterval uses MultiplicationIntervalOp for calculating intervals,
    // so the testing for multiplcationInterval covers the actual interval
    // calculations.
    {
      matrix: [
        [1, 2],
        [3, 4],
      ],
      scalar: 10,
      expected: [
        [10, 20],
        [30, 40],
      ],
    },
    {
      matrix: [
        [1, 2],
        [3, 4],
        [5, 6],
      ],
      scalar: 10,
      expected: [
        [10, 20],
        [30, 40],
        [50, 60],
      ],
    },
    {
      matrix: [
        [1, 2],
        [3, 4],
        [5, 6],
        [7, 8],
      ],
      scalar: 10,
      expected: [
        [10, 20],
        [30, 40],
        [50, 60],
        [70, 80],
      ],
    },
    {
      matrix: [
        [1, 2, 3],
        [4, 5, 6],
      ],
      scalar: 10,
      expected: [
        [10, 20, 30],
        [40, 50, 60],
      ],
    },
    {
      matrix: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
      ],
      scalar: 10,
      expected: [
        [10, 20, 30],
        [40, 50, 60],
        [70, 80, 90],
      ],
    },
    {
      matrix: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
        [10, 11, 12],
      ],
      scalar: 10,
      expected: [
        [10, 20, 30],
        [40, 50, 60],
        [70, 80, 90],
        [100, 110, 120],
      ],
    },
    {
      matrix: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
      ],
      scalar: 10,
      expected: [
        [10, 20, 30, 40],
        [50, 60, 70, 80],
      ],
    },
    {
      matrix: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
      ],
      scalar: 10,
      expected: [
        [10, 20, 30, 40],
        [50, 60, 70, 80],
        [90, 100, 110, 120],
      ],
    },
    {
      matrix: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
        [13, 14, 15, 16],
      ],
      scalar: 10,
      expected: [
        [10, 20, 30, 40],
        [50, 60, 70, 80],
        [90, 100, 110, 120],
        [130, 140, 150, 160],
      ],
    },
  ])
  .fn(t => {
    const matrix = t.params.matrix;
    const scalar = t.params.scalar;
    const expected = FP.f32.toMatrix(t.params.expected);
    const got = FP.f32.multiplicationMatrixScalarInterval(matrix, scalar);
    t.expect(
      objectEquals(expected, got),
      `f32.multiplicationMatrixScalarInterval([${JSON.stringify(
        matrix
      )}], ${scalar}) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

// There are no explicit tests for multiplicationScalarMatrixInterval, since it
// is just a pass-through to multiplicationMatrixScalarInterval

interface MatrixVectorToVectorCase {
  matrix: number[][];
  vector: number[];
  expected: (number | IntervalBounds)[];
}

g.test('multiplicationMatrixVectorInterval_f32')
  .paramsSubcasesOnly<MatrixVectorToVectorCase>([
    // Only testing that different shapes of matrices are handled correctly
    // here, to reduce test duplication.
    // multiplicationMatrixVectorInterval uses DotIntervalOp &
    // TransposeIntervalOp for calculating intervals, so the testing for
    // dotInterval & transposeInterval covers the actual interval
    // calculations.
    {
      matrix: [
        [1, 2],
        [3, 4],
      ],
      vector: [11, 22],
      expected: [77, 110],
    },
    {
      matrix: [
        [1, 2, 3],
        [4, 5, 6],
      ],
      vector: [11, 22],
      expected: [99, 132, 165],
    },
    {
      matrix: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
      ],
      vector: [11, 22],
      expected: [121, 154, 187, 220],
    },
    {
      matrix: [
        [1, 2],
        [3, 4],
        [5, 6],
      ],
      vector: [11, 22, 33],
      expected: [242, 308],
    },
    {
      matrix: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
      ],
      vector: [11, 22, 33],
      expected: [330, 396, 462],
    },
    {
      matrix: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
      ],
      vector: [11, 22, 33],
      expected: [418, 484, 550, 616],
    },
    {
      matrix: [
        [1, 2],
        [3, 4],
        [5, 6],
        [7, 8],
      ],
      vector: [11, 22, 33, 44],
      expected: [550, 660],
    },
    {
      matrix: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
        [10, 11, 12],
      ],
      vector: [11, 22, 33, 44],
      expected: [770, 880, 990],
    },
    {
      matrix: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
        [13, 14, 15, 16],
      ],
      vector: [11, 22, 33, 44],
      expected: [990, 1100, 1210, 1320],
    },
  ])
  .fn(t => {
    const matrix = t.params.matrix;
    const vector = t.params.vector;
    const expected = FP.f32.toVector(t.params.expected);
    const got = FP.f32.multiplicationMatrixVectorInterval(matrix, vector);
    t.expect(
      objectEquals(expected, got),
      `f32multiplicationMatrixVectorInterval([${JSON.stringify(matrix)}], [${JSON.stringify(
        vector
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

interface VectorMatrixToVectorCase {
  vector: number[];
  matrix: number[][];
  expected: (number | IntervalBounds)[];
}

g.test('multiplicationVectorMatrixInterval_f32')
  .paramsSubcasesOnly<VectorMatrixToVectorCase>([
    // Only testing that different shapes of matrices are handled correctly
    // here, to reduce test duplication.
    // multiplicationVectorMatrixInterval uses DotIntervalOp for calculating
    // intervals, so the testing for dotInterval covers the actual interval
    // calculations.
    {
      vector: [1, 2],
      matrix: [
        [11, 22],
        [33, 44],
      ],
      expected: [55, 121],
    },
    {
      vector: [1, 2],
      matrix: [
        [11, 22],
        [33, 44],
        [55, 66],
      ],
      expected: [55, 121, 187],
    },
    {
      vector: [1, 2],
      matrix: [
        [11, 22],
        [33, 44],
        [55, 66],
        [77, 88],
      ],
      expected: [55, 121, 187, 253],
    },
    {
      vector: [1, 2, 3],
      matrix: [
        [11, 22, 33],
        [44, 55, 66],
      ],
      expected: [154, 352],
    },
    {
      vector: [1, 2, 3],
      matrix: [
        [11, 22, 33],
        [44, 55, 66],
        [77, 88, 99],
      ],
      expected: [154, 352, 550],
    },
    {
      vector: [1, 2, 3],
      matrix: [
        [11, 22, 33],
        [44, 55, 66],
        [77, 88, 99],
        [1010, 1111, 1212],
      ],
      expected: [154, 352, 550, 6868],
    },
    {
      vector: [1, 2, 3, 4],
      matrix: [
        [11, 22, 33, 44],
        [55, 66, 77, 88],
      ],
      expected: [330, 770],
    },
    {
      vector: [1, 2, 3, 4],
      matrix: [
        [11, 22, 33, 44],
        [55, 66, 77, 88],
        [99, 1010, 1111, 1212],
      ],
      expected: [330, 770, 10300],
    },
    {
      vector: [1, 2, 3, 4],
      matrix: [
        [11, 22, 33, 44],
        [55, 66, 77, 88],
        [99, 1010, 1111, 1212],
        [1313, 1414, 1515, 1616],
      ],
      expected: [330, 770, 10300, 15150],
    },
  ])
  .fn(t => {
    const vector = t.params.vector;
    const matrix = t.params.matrix;
    const expected = FP.f32.toVector(t.params.expected);
    const got = FP.f32.multiplicationVectorMatrixInterval(vector, matrix);
    t.expect(
      objectEquals(expected, got),
      `f32.multiplicationVectorMatrixInterval([${JSON.stringify(vector)}], [${JSON.stringify(
        matrix
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

// API - Acceptance Intervals w/ bespoke implementations

interface FaceForwardCase {
  input: [number[], number[], number[]];
  expected: ((number | IntervalBounds)[] | undefined)[];
}

g.test('faceForwardIntervals_f32')
  .paramsSubcasesOnly<FaceForwardCase>(
    // prettier-ignore
    [
      // vec2
      { input: [[1.0, 0.0], [1.0, 0.0], [1.0, 0.0]], expected: [[-1.0, 0.0]] },
      { input: [[-1.0, 0.0], [1.0, 0.0], [1.0, 0.0]], expected: [[1.0, 0.0]] },
      { input: [[1.0, 0.0], [-1.0, 1.0], [1.0, -1.0]], expected: [[1.0, 0.0]] },
      { input: [[-1.0, 0.0], [-1.0, 1.0], [1.0, -1.0]], expected: [[-1.0, 0.0]] },
      { input: [[10.0, 0.0], [10.0, 0.0], [10.0, 0.0]], expected: [[-10.0, 0.0]] },
      { input: [[-10.0, 0.0], [10.0, 0.0], [10.0, 0.0]], expected: [[10.0, 0.0]] },
      { input: [[10.0, 0.0], [-10.0, 10.0], [10.0, -10.0]], expected: [[10.0, 0.0]] },
      { input: [[-10.0, 0.0], [-10.0, 10.0], [10.0, -10.0]], expected: [[-10.0, 0.0]] },
      { input: [[0.1, 0.0], [0.1, 0.0], [0.1, 0.0]], expected: [[[hexToF32(0xbdcccccd), hexToF32(0xbdcccccc)], 0.0]] },
      { input: [[-0.1, 0.0], [0.1, 0.0], [0.1, 0.0]], expected: [[[hexToF32(0x3dcccccc), hexToF32(0x3dcccccd)], 0.0]] },
      { input: [[0.1, 0.0], [-0.1, 0.1], [0.1, -0.1]], expected: [[[hexToF32(0x3dcccccc), hexToF32(0x3dcccccd)], 0.0]] },
      { input: [[-0.1, 0.0], [-0.1, 0.1], [0.1, -0.1]], expected: [[[hexToF32(0xbdcccccd), hexToF32(0xbdcccccc)], 0.0]] },

      // vec3
      { input: [[1.0, 0.0, 0.0], [1.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [[-1.0, 0.0, 0.0]] },
      { input: [[-1.0, 0.0, 0.0], [1.0, 0.0, 0.0], [1.0, 0.0, 0.0]], expected: [[1.0, 0.0, 0.0]] },
      { input: [[1.0, 0.0, 0.0], [-1.0, 1.0, 0.0], [1.0, -1.0, 0.0]], expected: [[1.0, 0.0, 0.0]] },
      { input: [[-1.0, 0.0, 0.0], [-1.0, 1.0, 0.0], [1.0, -1.0, 0.0]], expected: [[-1.0, 0.0, 0.0]] },
      { input: [[10.0, 0.0, 0.0], [10.0, 0.0, 0.0], [10.0, 0.0, 0.0]], expected: [[-10.0, 0.0, 0.0]] },
      { input: [[-10.0, 0.0, 0.0], [10.0, 0.0, 0.0], [10.0, 0.0, 0.0]], expected: [[10.0, 0.0, 0.0]] },
      { input: [[10.0, 0.0, 0.0], [-10.0, 10.0, 0.0], [10.0, -10.0, 0.0]], expected: [[10.0, 0.0, 0.0]] },
      { input: [[-10.0, 0.0, 0.0], [-10.0, 10.0, 0.0], [10.0, -10.0, 0.0]], expected: [[-10.0, 0.0, 0.0]] },
      { input: [[0.1, 0.0, 0.0], [0.1, 0.0, 0.0], [0.1, 0.0, 0.0]], expected: [[[hexToF32(0xbdcccccd), hexToF32(0xbdcccccc)], 0.0, 0.0]] },
      { input: [[-0.1, 0.0, 0.0], [0.1, 0.0, 0.0], [0.1, 0.0, 0.0]], expected: [[[hexToF32(0x3dcccccc), hexToF32(0x3dcccccd)], 0.0, 0.0]] },
      { input: [[0.1, 0.0, 0.0], [-0.1, 0.0, 0.0], [0.1, -0.0, 0.0]], expected: [[[hexToF32(0x3dcccccc), hexToF32(0x3dcccccd)], 0.0, 0.0]] },
      { input: [[-0.1, 0.0, 0.0], [-0.1, 0.0, 0.0], [0.1, -0.0, 0.0]], expected: [[[hexToF32(0xbdcccccd), hexToF32(0xbdcccccc)], 0.0, 0.0]] },

      // vec4
      { input: [[1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [[-1.0, 0.0, 0.0, 0.0]] },
      { input: [[-1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0]], expected: [[1.0, 0.0, 0.0, 0.0]] },
      { input: [[1.0, 0.0, 0.0, 0.0], [-1.0, 1.0, 0.0, 0.0], [1.0, -1.0, 0.0, 0.0]], expected: [[1.0, 0.0, 0.0, 0.0]] },
      { input: [[-1.0, 0.0, 0.0, 0.0], [-1.0, 1.0, 0.0, 0.0], [1.0, -1.0, 0.0, 0.0]], expected: [[-1.0, 0.0, 0.0, 0.0]] },
      { input: [[10.0, 0.0, 0.0, 0.0], [10.0, 0.0, 0.0, 0.0], [10.0, 0.0, 0.0, 0.0]], expected: [[-10.0, 0.0, 0.0, 0.0]] },
      { input: [[-10.0, 0.0, 0.0, 0.0], [10.0, 0.0, 0.0, 0.0], [10.0, 0.0, 0.0, 0.0]], expected: [[10.0, 0.0, 0.0, 0.0]] },
      { input: [[10.0, 0.0, 0.0, 0.0], [-10.0, 10.0, 0.0, 0.0], [10.0, -10.0, 0.0, 0.0]], expected: [[10.0, 0.0, 0.0, 0.0]] },
      { input: [[-10.0, 0.0, 0.0, 0.0], [-10.0, 10.0, 0.0, 0.0], [10.0, -10.0, 0.0, 0.0]], expected: [[-10.0, 0.0, 0.0, 0.0]] },
      { input: [[0.1, 0.0, 0.0, 0.0], [0.1, 0.0, 0.0, 0.0], [0.1, 0.0, 0.0, 0.0]], expected: [[[hexToF32(0xbdcccccd), hexToF32(0xbdcccccc)], 0.0, 0.0, 0.0]] },
      { input: [[-0.1, 0.0, 0.0, 0.0], [0.1, 0.0, 0.0, 0.0], [0.1, 0.0, 0.0, 0.0]], expected: [[[hexToF32(0x3dcccccc), hexToF32(0x3dcccccd)], 0.0, 0.0, 0.0]] },
      { input: [[0.1, 0.0, 0.0, 0.0], [-0.1, 0.0, 0.0, 0.0], [0.1, -0.0, 0.0, 0.0]], expected: [[[hexToF32(0x3dcccccc), hexToF32(0x3dcccccd)], 0.0, 0.0, 0.0]] },
      { input: [[-0.1, 0.0, 0.0, 0.0], [-0.1, 0.0, 0.0, 0.0], [0.1, -0.0, 0.0, 0.0]], expected: [[[hexToF32(0xbdcccccd), hexToF32(0xbdcccccc)], 0.0, 0.0, 0.0]] },

      // dot(y, z) === 0
      { input: [[1.0, 1.0], [1.0, 0.0], [0.0, 1.0]], expected:  [[-1.0, -1.0]] },

      // subnormals, also dot(y, z) spans 0
      { input: [[kValue.f32.subnormal.positive.max, 0.0], [kValue.f32.subnormal.positive.min, 0.0], [kValue.f32.subnormal.negative.min, 0.0]], expected:  [[[0.0, kValue.f32.subnormal.positive.max], 0.0], [[kValue.f32.subnormal.negative.min, 0], 0.0]] },

      // dot going OOB returns [undefined, x, -x]
      { input: [[1.0, 1.0], [kValue.f32.positive.max, kValue.f32.positive.max], [kValue.f32.positive.max, kValue.f32.positive.max]], expected: [undefined, [1, 1], [-1, -1]] },

    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;
    const expected = t.params.expected.map(e => (e !== undefined ? FP.f32.toVector(e) : undefined));
    const got = FP.f32.faceForwardIntervals(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `f32.faceForwardInterval([${x}], [${y}], [${z}]) returned [${got}]. Expected [${expected}]`
    );
  });

interface ModfCase {
  input: number;
  fract: number | IntervalBounds;
  whole: number | IntervalBounds;
}

g.test('modfInterval_f32')
  .paramsSubcasesOnly<ModfCase>(
    // prettier-ignore
    [
      // Normals
      { input: 0, fract: 0, whole: 0 },
      { input: 1, fract: 0, whole: 1 },
      { input: -1, fract: 0, whole: -1 },
      { input: 0.5, fract: 0.5, whole: 0 },
      { input: -0.5, fract: -0.5, whole: 0 },
      { input: 2.5, fract: 0.5, whole: 2 },
      { input: -2.5, fract: -0.5, whole: -2 },
      { input: 10.0, fract: 0, whole: 10 },
      { input: -10.0, fract: 0, whole: -10 },

      // Subnormals
      { input: kValue.f32.subnormal.negative.min, fract: [kValue.f32.subnormal.negative.min, 0], whole: 0 },
      { input: kValue.f32.subnormal.negative.max, fract: [kValue.f32.subnormal.negative.max, 0], whole: 0 },
      { input: kValue.f32.subnormal.positive.min, fract: [0, kValue.f32.subnormal.positive.min], whole: 0 },
      { input: kValue.f32.subnormal.positive.max, fract: [0, kValue.f32.subnormal.positive.max], whole: 0 },

      // Boundaries
      { input: kValue.f32.negative.min, fract: 0, whole: kValue.f32.negative.min },
      { input: kValue.f32.negative.max, fract: kValue.f32.negative.max, whole: 0 },
      { input: kValue.f32.positive.min, fract: kValue.f32.positive.min, whole: 0 },
      { input: kValue.f32.positive.max, fract: 0, whole: kValue.f32.positive.max },
    ]
  )
  .fn(t => {
    const expected = {
      fract: FP.f32.toInterval(t.params.fract),
      whole: FP.f32.toInterval(t.params.whole),
    };

    const got = FP.f32.modfInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `f32.modfInterval([${t.params.input}) returned { fract: [${got.fract}], whole: [${got.whole}] }. Expected { fract: [${expected.fract}], whole: [${expected.whole}] }`
    );
  });

interface RefractCase {
  input: [number[], number[], number];
  expected: (number | IntervalBounds)[];
}

// Scope for refractInterval tests so that they can have constants for magic
// numbers that don't pollute the global namespace or have unwieldy long names.
{
  const kNegativeOneBounds: IntervalBounds = [
    hexToF64(0xbff0_0000_c000_0000n),
    hexToF64(0xbfef_ffff_4000_0000n),
  ];

  g.test('refractInterval_f32')
    .paramsSubcasesOnly<RefractCase>(
      // Some of these are hard coded, since the error intervals are difficult
      // to express in a closed human-readable form due to the inherited nature
      // of the errors.

      // prettier-ignore
      [
        // k < 0
        { input: [[1, 1], [0.1, 0], 10], expected: [0, 0] },

        // k contains 0
        { input: [[1, 1], [0.1, 0], 1.005038], expected: [kAnyBounds, kAnyBounds] },

        // k > 0
        // vec2
        { input: [[1, 1], [1, 0], 1], expected: [kNegativeOneBounds, 1] },
        { input: [[1, -2], [3, 4], 5], expected: [[hexToF32(0x40ce87a4), hexToF32(0x40ce8840)],  // ~6.454...
            [hexToF32(0xc100fae8), hexToF32(0xc100fa80)]] },  // ~-8.061...

        // vec3
        { input: [[1, 1, 1], [1, 0, 0], 1], expected: [kNegativeOneBounds, 1, 1] },
        { input: [[1, -2, 3], [-4, 5, -6], 7], expected: [[hexToF32(0x40d24480), hexToF32(0x40d24c00)],  // ~6.571...
            [hexToF32(0xc1576f80), hexToF32(0xc1576ad0)],  // ~-13.464...
            [hexToF32(0x41a2d9b0), hexToF32(0x41a2dc80)]] },  // ~20.356...

        // vec4
        { input: [[1, 1, 1, 1], [1, 0, 0, 0], 1], expected: [kNegativeOneBounds, 1, 1, 1] },
        { input: [[1, -2, 3,-4], [-5, 6, -7, 8], 9], expected: [[hexToF32(0x410ae480), hexToF32(0x410af240)],  // ~8.680...
            [hexToF32(0xc18cf7c0), hexToF32(0xc18cef80)],  // ~-17.620...
            [hexToF32(0x41d46cc0), hexToF32(0x41d47660)],  // ~26.553...
            [hexToF32(0xc20dfa80), hexToF32(0xc20df500)]] },  // ~-35.494...

        // Test that dot going OOB bounds in the intermediate calculations propagates
        { input: [[kValue.f32.positive.nearest_max, kValue.f32.positive.max, kValue.f32.negative.min], [1.0, 1.0, 1.0], 1], expected: [kAnyBounds, kAnyBounds, kAnyBounds] },
        { input: [[kValue.f32.positive.nearest_max, kValue.f32.negative.min, kValue.f32.positive.max], [1.0, 1.0, 1.0], 1], expected: [kAnyBounds, kAnyBounds, kAnyBounds] },
        { input: [[kValue.f32.positive.max, kValue.f32.positive.nearest_max, kValue.f32.negative.min], [1.0, 1.0, 1.0], 1], expected: [kAnyBounds, kAnyBounds, kAnyBounds] },
        { input: [[kValue.f32.negative.min, kValue.f32.positive.nearest_max, kValue.f32.positive.max], [1.0, 1.0, 1.0], 1], expected: [kAnyBounds, kAnyBounds, kAnyBounds] },
        { input: [[kValue.f32.positive.max, kValue.f32.negative.min, kValue.f32.positive.nearest_max], [1.0, 1.0, 1.0], 1], expected: [kAnyBounds, kAnyBounds, kAnyBounds] },
        { input: [[kValue.f32.negative.min, kValue.f32.positive.max, kValue.f32.positive.nearest_max], [1.0, 1.0, 1.0], 1], expected: [kAnyBounds, kAnyBounds, kAnyBounds] },
      ]
    )
    .fn(t => {
      const [i, s, r] = t.params.input;
      const expected = FP.f32.toVector(t.params.expected);
      const got = FP.f32.refractInterval(i, s, r);
      t.expect(
        objectEquals(expected, got),
        `refractIntervals([${i}], [${s}], ${r}) returned [${got}]. Expected [${expected}]`
      );
    });
}
