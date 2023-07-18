// This is a shim file that performs testing via the old/deprecates f32 API
// calls.
// Those currently just pass-through to the new refactored FPContext
// implementation.
// As CTS migrates over to directly calling the new API these test will be
// replaced with direct call tests.

export const description = `
F32 unit tests.
`;

import { makeTestGroup } from '../common/framework/test_group.js';
import { objectEquals } from '../common/util/util.js';
import { kValue } from '../webgpu/util/constants.js';
import {
  absoluteErrorInterval,
  additionMatrixInterval,
  correctlyRoundedInterval,
  faceForwardIntervals,
  modfInterval,
  multiplicationMatrixMatrixInterval,
  multiplicationMatrixScalarInterval,
  multiplicationMatrixVectorInterval,
  multiplicationVectorMatrixInterval,
  refractInterval,
  subtractionMatrixInterval,
  toF32Interval,
  toF32Matrix,
  toF32Vector,
  transposeInterval,
  ulpInterval,
  determinantInterval,
  isF32Vector,
  isF32Matrix,
  spanF32Intervals,
} from '../webgpu/util/f32_interval.js';
import { FPInterval, IntervalBounds } from '../webgpu/util/floating_point.js';
import { hexToF32, hexToF64, map2DArray, oneULPF32 } from '../webgpu/util/math.js';

import { UnitTest } from './unit_test.js';

export const g = makeTestGroup(UnitTest);

/** Bounds indicating an expectation of an interval of all possible values */
const kAnyBounds: IntervalBounds = [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY];

/** Interval from kAnyBounds */
const kAnyInterval: FPInterval = toF32Interval(kAnyBounds);

/** @returns a number N * ULP greater than the provided number */
function plusNULP(x: number, n: number): number {
  return x + n * oneULPF32(x);
}

/** @returns a number one ULP greater than the provided number */
function plusOneULP(x: number): number {
  return plusNULP(x, 1);
}

/** @returns a number N * ULP less than the provided number */
function minusNULP(x: number, n: number): number {
  return x - n * oneULPF32(x);
}

/** @returns a number one ULP less than the provided number */
function minusOneULP(x: number): number {
  return minusNULP(x, 1);
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
      `FPInterval([${t.params.input}]) returned ${i}. Expected [${t.params.expected}]`
    );
  });

interface ContainsNumberCase {
  bounds: number | IntervalBounds;
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
    const i = toF32Interval(t.params.bounds);
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

g.test('contains_interval')
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
    const lhs = toF32Interval(t.params.lhs);
    const rhs = toF32Interval(t.params.rhs);
    const expected = t.params.expected;

    const got = lhs.contains(rhs);
    t.expect(expected === got, `${lhs}.contains(${rhs}) returned ${got}. Expected ${expected}`);
  });

interface SpanCase {
  intervals: (number | IntervalBounds)[];
  expected: number | IntervalBounds;
}

g.test('span')
  .paramsSubcasesOnly<SpanCase>(
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
    const intervals = t.params.intervals.map(i => toF32Interval(i));
    const expected = toF32Interval(t.params.expected);

    const got = spanF32Intervals(...intervals);
    t.expect(
      objectEquals(got, expected),
      `span({${intervals}}) returned ${got}. Expected ${expected}`
    );
  });

interface isF32VectorCase {
  input: (number | IntervalBounds | FPInterval)[];
  expected: boolean;
}

g.test('isF32Vector')
  .paramsSubcasesOnly<isF32VectorCase>([
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
    { input: [toF32Interval([1]), toF32Interval([2])], expected: true },
    { input: [toF32Interval([1, 2]), toF32Interval([2, 3])], expected: true },
    {
      input: [toF32Interval([1]), toF32Interval([2]), toF32Interval([3])],
      expected: true,
    },
    {
      input: [toF32Interval([1, 2]), toF32Interval([2, 3]), toF32Interval([3, 4])],
      expected: true,
    },
    {
      input: [toF32Interval([1]), toF32Interval([2]), toF32Interval([3]), toF32Interval([4])],
      expected: true,
    },
    {
      input: [
        toF32Interval([1, 2]),
        toF32Interval([2, 3]),
        toF32Interval([3, 4]),
        toF32Interval([4, 5]),
      ],
      expected: true,
    },

    // FPInterval, invalid dimensions
    { input: [toF32Interval([1])], expected: false },
    {
      input: [
        toF32Interval([1]),
        toF32Interval([2]),
        toF32Interval([3]),
        toF32Interval([4]),
        toF32Interval([5]),
      ],
      expected: false,
    },

    // Mixed
    { input: [1, [2]], expected: false },
    { input: [1, [2], toF32Interval([3])], expected: false },
    { input: [1, toF32Interval([2]), [3], 4], expected: false },
    { input: [toF32Interval(1), 2], expected: false },
    { input: [toF32Interval(1), [2]], expected: false },
  ])
  .fn(t => {
    const input = t.params.input;
    const expected = t.params.expected;

    const got = isF32Vector(input);
    t.expect(got === expected, `isF32Vector([${input}]) returned ${got}. Expected ${expected}`);
  });

interface toF32VectorCase {
  input: (number | IntervalBounds | FPInterval)[];
  expected: (number | IntervalBounds)[];
}

g.test('toF32Vector')
  .paramsSubcasesOnly<toF32VectorCase>([
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
    { input: [toF32Interval([1]), toF32Interval([2])], expected: [1, 2] },
    {
      input: [toF32Interval([1, 2]), toF32Interval([2, 3])],
      expected: [
        [1, 2],
        [2, 3],
      ],
    },
    {
      input: [toF32Interval([1]), toF32Interval([2]), toF32Interval([3])],
      expected: [1, 2, 3],
    },
    {
      input: [toF32Interval([1, 2]), toF32Interval([2, 3]), toF32Interval([3, 4])],
      expected: [
        [1, 2],
        [2, 3],
        [3, 4],
      ],
    },
    {
      input: [toF32Interval([1]), toF32Interval([2]), toF32Interval([3]), toF32Interval([4])],
      expected: [1, 2, 3, 4],
    },
    {
      input: [
        toF32Interval([1, 2]),
        toF32Interval([2, 3]),
        toF32Interval([3, 4]),
        toF32Interval([4, 5]),
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
    { input: [1, [2], toF32Interval([3])], expected: [1, 2, 3] },
    { input: [1, toF32Interval([2]), [3], 4], expected: [1, 2, 3, 4] },
    {
      input: [1, [2], [2, 3], kAnyInterval],
      expected: [1, 2, [2, 3], kAnyBounds],
    },
  ])
  .fn(t => {
    const input = t.params.input;
    const expected = t.params.expected.map(e => toF32Interval(e));

    const got = toF32Vector(input);
    t.expect(
      objectEquals(got, expected),
      `toF32Vector([${input}]) returned [${got}]. Expected [${expected}]`
    );
  });

interface isF32MatrixCase {
  input: (number | IntervalBounds | FPInterval)[][];
  expected: boolean;
}

g.test('isF32Matrix')
  .paramsSubcasesOnly<isF32MatrixCase>([
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
        [toF32Interval(1), toF32Interval(2)],
        [toF32Interval(3), toF32Interval(4)],
      ],
      expected: true,
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2)],
        [toF32Interval(3), toF32Interval(4)],
        [toF32Interval(5), toF32Interval(6)],
      ],
      expected: true,
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2)],
        [toF32Interval(3), toF32Interval(4)],
        [toF32Interval(5), toF32Interval(6)],
        [toF32Interval(7), toF32Interval(8)],
      ],
      expected: true,
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2), toF32Interval(3)],
        [toF32Interval(4), toF32Interval(5), toF32Interval(6)],
      ],
      expected: true,
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2), toF32Interval(3)],
        [toF32Interval(4), toF32Interval(5), toF32Interval(6)],
        [toF32Interval(7), toF32Interval(8), toF32Interval(9)],
      ],
      expected: true,
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2), toF32Interval(3)],
        [toF32Interval(4), toF32Interval(5), toF32Interval(6)],
        [toF32Interval(7), toF32Interval(8), toF32Interval(9)],
        [toF32Interval(10), toF32Interval(11), toF32Interval(12)],
      ],
      expected: true,
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2), toF32Interval(3), toF32Interval(4)],
        [toF32Interval(5), toF32Interval(6), toF32Interval(7), toF32Interval(8)],
      ],
      expected: true,
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2), toF32Interval(3), toF32Interval(4)],
        [toF32Interval(5), toF32Interval(6), toF32Interval(7), toF32Interval(8)],
        [toF32Interval(9), toF32Interval(10), toF32Interval(11), toF32Interval(12)],
      ],
      expected: true,
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2), toF32Interval(3), toF32Interval(4)],
        [toF32Interval(5), toF32Interval(6), toF32Interval(7), toF32Interval(8)],
        [toF32Interval(9), toF32Interval(10), toF32Interval(11), toF32Interval(12)],
        [toF32Interval(13), toF32Interval(14), toF32Interval(15), toF32Interval(16)],
      ],
      expected: true,
    },
    {
      input: [
        [toF32Interval([1, 2]), toF32Interval([2, 3])],
        [toF32Interval([3, 4]), toF32Interval([4, 5])],
      ],
      expected: true,
    },
    {
      input: [
        [toF32Interval([1, 2]), toF32Interval([2, 3])],
        [toF32Interval([3, 4]), toF32Interval([4, 5])],
        [toF32Interval([5, 6]), toF32Interval([6, 7])],
      ],
      expected: true,
    },
    {
      input: [
        [toF32Interval([1, 2]), toF32Interval([2, 3])],
        [toF32Interval([3, 4]), toF32Interval([4, 5])],
        [toF32Interval([5, 6]), toF32Interval([6, 7])],
        [toF32Interval([7, 8]), toF32Interval([8, 9])],
      ],
      expected: true,
    },
    {
      input: [
        [toF32Interval([1, 2]), toF32Interval([2, 3]), toF32Interval([3, 4])],
        [toF32Interval([4, 5]), toF32Interval([5, 6]), toF32Interval([6, 7])],
      ],
      expected: true,
    },
    {
      input: [
        [toF32Interval([1, 2]), toF32Interval([2, 3]), toF32Interval([3, 4])],
        [toF32Interval([4, 5]), toF32Interval([5, 6]), toF32Interval([6, 7])],
        [toF32Interval([7, 8]), toF32Interval([8, 9]), toF32Interval([9, 10])],
      ],
      expected: true,
    },
    {
      input: [
        [toF32Interval([1, 2]), toF32Interval([2, 3]), toF32Interval([3, 4])],
        [toF32Interval([4, 5]), toF32Interval([5, 6]), toF32Interval([6, 7])],
        [toF32Interval([7, 8]), toF32Interval([8, 9]), toF32Interval([9, 10])],
        [toF32Interval([10, 11]), toF32Interval([11, 12]), toF32Interval([12, 13])],
      ],
      expected: true,
    },
    {
      input: [
        [
          toF32Interval([1, 2]),
          toF32Interval([2, 3]),
          toF32Interval([3, 4]),
          toF32Interval([4, 5]),
        ],
        [
          toF32Interval([5, 6]),
          toF32Interval([6, 7]),
          toF32Interval([7, 8]),
          toF32Interval([8, 9]),
        ],
      ],
      expected: true,
    },
    {
      input: [
        [
          toF32Interval([1, 2]),
          toF32Interval([2, 3]),
          toF32Interval([3, 4]),
          toF32Interval([4, 5]),
        ],
        [
          toF32Interval([5, 6]),
          toF32Interval([6, 7]),
          toF32Interval([7, 8]),
          toF32Interval([8, 9]),
        ],
        [
          toF32Interval([9, 10]),
          toF32Interval([10, 11]),
          toF32Interval([11, 12]),
          toF32Interval([12, 13]),
        ],
      ],
      expected: true,
    },
    {
      input: [
        [
          toF32Interval([1, 2]),
          toF32Interval([2, 3]),
          toF32Interval([3, 4]),
          toF32Interval([4, 5]),
        ],
        [
          toF32Interval([5, 6]),
          toF32Interval([6, 7]),
          toF32Interval([7, 8]),
          toF32Interval([8, 9]),
        ],
        [
          toF32Interval([9, 10]),
          toF32Interval([10, 11]),
          toF32Interval([11, 12]),
          toF32Interval([12, 13]),
        ],
        [
          toF32Interval([13, 14]),
          toF32Interval([14, 15]),
          toF32Interval([15, 16]),
          toF32Interval([16, 17]),
        ],
      ],
      expected: true,
    },

    // FPInterval, invalid dimensions
    { input: [[toF32Interval(1)]], expected: false },
    {
      input: [[toF32Interval(1)], [toF32Interval(3), toF32Interval(4)]],
      expected: false,
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2)],
        [toF32Interval(3), toF32Interval(4), toF32Interval(5)],
      ],
      expected: false,
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2)],
        [toF32Interval(3), toF32Interval(4)],
        [toF32Interval(5)],
      ],
      expected: false,
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2)],
        [toF32Interval(3), toF32Interval(4)],
        [toF32Interval(5), toF32Interval(6)],
        [toF32Interval(7), toF32Interval(8)],
        [toF32Interval(9), toF32Interval(10)],
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
        [toF32Interval([3]), 4],
      ],
      expected: false,
    },
    {
      input: [
        [[1], toF32Interval([2])],
        [toF32Interval([3]), toF32Interval([4])],
      ],
      expected: false,
    },
    {
      input: [
        [toF32Interval(1), [2]],
        [3, 4],
      ],
      expected: false,
    },
  ])
  .fn(t => {
    const input = t.params.input;
    const expected = t.params.expected;

    const got = isF32Matrix(input);
    t.expect(got === expected, `isF32Matrix([${input}]) returned ${got}. Expected ${expected}`);
  });

interface toF32MatrixCase {
  input: (number | IntervalBounds | FPInterval)[][];
  expected: (number | IntervalBounds)[][];
}

g.test('toF32Matrix')
  .paramsSubcasesOnly<toF32MatrixCase>([
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
        [toF32Interval(1), toF32Interval(2)],
        [toF32Interval(3), toF32Interval(4)],
      ],
      expected: [
        [1, 2],
        [3, 4],
      ],
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2)],
        [toF32Interval(3), toF32Interval(4)],
        [toF32Interval(5), toF32Interval(6)],
      ],
      expected: [
        [1, 2],
        [3, 4],
        [5, 6],
      ],
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2)],
        [toF32Interval(3), toF32Interval(4)],
        [toF32Interval(5), toF32Interval(6)],
        [toF32Interval(7), toF32Interval(8)],
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
        [toF32Interval(1), toF32Interval(2), toF32Interval(3)],
        [toF32Interval(4), toF32Interval(5), toF32Interval(6)],
      ],
      expected: [
        [1, 2, 3],
        [4, 5, 6],
      ],
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2), toF32Interval(3)],
        [toF32Interval(4), toF32Interval(5), toF32Interval(6)],
        [toF32Interval(7), toF32Interval(8), toF32Interval(9)],
      ],
      expected: [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9],
      ],
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2), toF32Interval(3)],
        [toF32Interval(4), toF32Interval(5), toF32Interval(6)],
        [toF32Interval(7), toF32Interval(8), toF32Interval(9)],
        [toF32Interval(10), toF32Interval(11), toF32Interval(12)],
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
        [toF32Interval(1), toF32Interval(2), toF32Interval(3), toF32Interval(4)],
        [toF32Interval(5), toF32Interval(6), toF32Interval(7), toF32Interval(8)],
      ],
      expected: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
      ],
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2), toF32Interval(3), toF32Interval(4)],
        [toF32Interval(5), toF32Interval(6), toF32Interval(7), toF32Interval(8)],
        [toF32Interval(9), toF32Interval(10), toF32Interval(11), toF32Interval(12)],
      ],
      expected: [
        [1, 2, 3, 4],
        [5, 6, 7, 8],
        [9, 10, 11, 12],
      ],
    },
    {
      input: [
        [toF32Interval(1), toF32Interval(2), toF32Interval(3), toF32Interval(4)],
        [toF32Interval(5), toF32Interval(6), toF32Interval(7), toF32Interval(8)],
        [toF32Interval(9), toF32Interval(10), toF32Interval(11), toF32Interval(12)],
        [toF32Interval(13), toF32Interval(14), toF32Interval(15), toF32Interval(16)],
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
        [toF32Interval([1, 2]), toF32Interval([2, 3])],
        [toF32Interval([3, 4]), toF32Interval([4, 5])],
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
        [toF32Interval([1, 2]), toF32Interval([2, 3])],
        [toF32Interval([3, 4]), toF32Interval([4, 5])],
        [toF32Interval([5, 6]), toF32Interval([6, 7])],
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
        [toF32Interval([1, 2]), toF32Interval([2, 3])],
        [toF32Interval([3, 4]), toF32Interval([4, 5])],
        [toF32Interval([5, 6]), toF32Interval([6, 7])],
        [toF32Interval([7, 8]), toF32Interval([8, 9])],
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
        [toF32Interval([1, 2]), toF32Interval([2, 3]), toF32Interval([3, 4])],
        [toF32Interval([4, 5]), toF32Interval([5, 6]), toF32Interval([6, 7])],
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
        [toF32Interval([1, 2]), toF32Interval([2, 3]), toF32Interval([3, 4])],
        [toF32Interval([4, 5]), toF32Interval([5, 6]), toF32Interval([6, 7])],
        [toF32Interval([7, 8]), toF32Interval([8, 9]), toF32Interval([9, 10])],
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
        [toF32Interval([1, 2]), toF32Interval([2, 3]), toF32Interval([3, 4])],
        [toF32Interval([4, 5]), toF32Interval([5, 6]), toF32Interval([6, 7])],
        [toF32Interval([7, 8]), toF32Interval([8, 9]), toF32Interval([9, 10])],
        [toF32Interval([10, 11]), toF32Interval([11, 12]), toF32Interval([12, 13])],
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
          toF32Interval([1, 2]),
          toF32Interval([2, 3]),
          toF32Interval([3, 4]),
          toF32Interval([4, 5]),
        ],
        [
          toF32Interval([5, 6]),
          toF32Interval([6, 7]),
          toF32Interval([7, 8]),
          toF32Interval([8, 9]),
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
          toF32Interval([1, 2]),
          toF32Interval([2, 3]),
          toF32Interval([3, 4]),
          toF32Interval([4, 5]),
        ],
        [
          toF32Interval([5, 6]),
          toF32Interval([6, 7]),
          toF32Interval([7, 8]),
          toF32Interval([8, 9]),
        ],
        [
          toF32Interval([9, 10]),
          toF32Interval([10, 11]),
          toF32Interval([11, 12]),
          toF32Interval([12, 13]),
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
          toF32Interval([1, 2]),
          toF32Interval([2, 3]),
          toF32Interval([3, 4]),
          toF32Interval([4, 5]),
        ],
        [
          toF32Interval([5, 6]),
          toF32Interval([6, 7]),
          toF32Interval([7, 8]),
          toF32Interval([8, 9]),
        ],
        [
          toF32Interval([9, 10]),
          toF32Interval([10, 11]),
          toF32Interval([11, 12]),
          toF32Interval([12, 13]),
        ],
        [
          toF32Interval([13, 14]),
          toF32Interval([14, 15]),
          toF32Interval([15, 16]),
          toF32Interval([16, 17]),
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
        [toF32Interval([3]), 4],
      ],
      expected: [
        [1, 2],
        [3, 4],
      ],
    },
    {
      input: [
        [[1], toF32Interval([2])],
        [toF32Interval([3]), toF32Interval([4])],
      ],
      expected: [
        [1, 2],
        [3, 4],
      ],
    },
  ])
  .fn(t => {
    const input = t.params.input;
    const expected = map2DArray(t.params.expected, e => toF32Interval(e));

    const got = toF32Matrix(input);
    t.expect(
      objectEquals(got, expected),
      `toF32Matrix([${input}]) returned [${got}]. Expected [${expected}]`
    );
  });

interface CorrectlyRoundedCase {
  value: number;
  expected: number | IntervalBounds;
}

g.test('correctlyRoundedInterval')
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
    const expected = toF32Interval(t.params.expected);
    const got = correctlyRoundedInterval(t.params.value);
    t.expect(
      objectEquals(expected, got),
      `correctlyRoundedInterval(${t.params.value}) returned ${got}. Expected ${expected}`
    );
  });

interface AbsoluteErrorCase {
  value: number;
  error: number;
  expected: number | IntervalBounds;
}

g.test('absoluteErrorInterval')
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
    const expected = toF32Interval(t.params.expected);
    const got = absoluteErrorInterval(t.params.value, t.params.error);
    t.expect(
      objectEquals(expected, got),
      `absoluteErrorInterval(${t.params.value}, ${t.params.error}) returned ${got}. Expected ${expected}`
    );
  });

interface ULPCase {
  value: number;
  num_ulp: number;
  expected: number | IntervalBounds;
}

g.test('ulpInterval')
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
      { value: kValue.f32.positive.min, num_ulp: 1, expected: [0, plusOneULP(kValue.f32.positive.min)] },
      { value: kValue.f32.positive.min, num_ulp: 4096, expected: [0, plusNULP(kValue.f32.positive.min, 4096)] },
      { value: kValue.f32.negative.min, num_ulp: 0, expected: kValue.f32.negative.min },
      { value: kValue.f32.negative.min, num_ulp: 1, expected: kAnyBounds },
      { value: kValue.f32.negative.min, num_ulp: 4096, expected: kAnyBounds },
      { value: kValue.f32.negative.max, num_ulp: 0, expected: kValue.f32.negative.max },
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
      { value: hexToF64(0x0000_0000_0000_0001n), num_ulp: 0, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x0000_0000_0000_0001n), num_ulp: 1, expected: [minusOneULP(0), plusOneULP(kValue.f32.subnormal.positive.min)] },
      { value: hexToF64(0x0000_0000_0000_0001n), num_ulp: 4096, expected: [minusNULP(0, 4096), plusNULP(kValue.f32.subnormal.positive.min, 4096)] },
      { value: hexToF64(0x0000_0000_0000_0002n), num_ulp: 0, expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x0000_0000_0000_0002n), num_ulp: 1, expected: [minusOneULP(0), plusOneULP(kValue.f32.subnormal.positive.min)] },
      { value: hexToF64(0x0000_0000_0000_0002n), num_ulp: 4096, expected: [minusNULP(0, 4096), plusNULP(kValue.f32.subnormal.positive.min, 4096)] },
      { value: hexToF64(0x800f_ffff_ffff_ffffn), num_ulp: 0, expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: hexToF64(0x800f_ffff_ffff_ffffn), num_ulp: 1, expected: [minusOneULP(kValue.f32.subnormal.negative.max), plusOneULP(0)] },
      { value: hexToF64(0x800f_ffff_ffff_ffffn), num_ulp: 4096, expected: [minusNULP(kValue.f32.subnormal.negative.max, 4096), plusNULP(0, 4096)] },
      { value: hexToF64(0x800f_ffff_ffff_fffen), num_ulp: 0, expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: hexToF64(0x800f_ffff_ffff_fffen), num_ulp: 1, expected: [minusOneULP(kValue.f32.subnormal.negative.max), plusOneULP(0)] },
      { value: hexToF64(0x800f_ffff_ffff_fffen), num_ulp: 4096, expected: [minusNULP(kValue.f32.subnormal.negative.max, 4096), plusNULP(0, 4096)] },

      // Zero
      { value: 0, num_ulp: 0, expected: 0 },
      { value: 0, num_ulp: 1, expected: [minusOneULP(0), plusOneULP(0)] },
      { value: 0, num_ulp: 4096, expected: [minusNULP(0, 4096), plusNULP(0, 4096)] },
    ]
  )
  .fn(t => {
    const expected = toF32Interval(t.params.expected);
    const got = ulpInterval(t.params.value, t.params.num_ulp);
    t.expect(
      objectEquals(expected, got),
      `ulpInterval(${t.params.value}, ${t.params.num_ulp}) returned ${got}. Expected ${expected}`
    );
  });

interface FaceForwardCase {
  input: [number[], number[], number[]];
  expected: ((number | IntervalBounds)[] | undefined)[];
}

g.test('faceForwardIntervals')
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
    const expected = t.params.expected.map(e => (e !== undefined ? toF32Vector(e) : undefined));
    const got = faceForwardIntervals(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `faceForwardInterval([${x}], [${y}], [${z}]) returned [${got}]. Expected [${expected}]`
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

  g.test('refractInterval')
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
      const expected = toF32Vector(t.params.expected);
      const got = refractInterval(i, s, r);
      t.expect(
        objectEquals(expected, got),
        `refractIntervals([${i}], [${s}], ${r}) returned [${got}]. Expected [${expected}]`
      );
    });
}

interface ModfCase {
  input: number;
  fract: number | IntervalBounds;
  whole: number | IntervalBounds;
}

g.test('modfInterval')
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
      fract: toF32Interval(t.params.fract),
      whole: toF32Interval(t.params.whole),
    };

    const got = modfInterval(t.params.input);
    t.expect(
      objectEquals(expected, got),
      `modfInterval([${t.params.input}) returned { fract: [${got.fract}], whole: [${got.whole}] }. Expected { fract: [${expected.fract}], whole: [${expected.whole}] }`
    );
  });

interface MatrixToScalarCase {
  input: number[][];
  expected: number | IntervalBounds;
}

g.test('determinantInterval')
  .paramsSubcasesOnly<MatrixToScalarCase>([
    // Exterme values, i.e. subnormals, very large magnitudes, and those lead to
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
    const expected = toF32Interval(t.params.expected);
    const got = determinantInterval(input);
    t.expect(
      objectEquals(expected, got),
      `determinantInterval([${JSON.stringify(input)}]) returned '${got}. Expected '${expected}'`
    );
  });

interface MatrixToMatrixCase {
  input: number[][];
  expected: (number | IntervalBounds)[][];
}

g.test('transposeInterval')
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
    const expected = toF32Matrix(t.params.expected);
    const got = transposeInterval(input);
    t.expect(
      objectEquals(expected, got),
      `transposeInterval([${JSON.stringify(input)}]) returned '[${JSON.stringify(
        got
      )}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

interface MatrixPairToMatrixCase {
  input: [number[][], number[][]];
  expected: (number | IntervalBounds)[][];
}

g.test('additionMatrixInterval')
  .paramsSubcasesOnly<MatrixPairToMatrixCase>([
    // Only testing that different shapes of matrices are handled correctly
    // here, to reduce test duplication.
    // additionMatrixInterval uses AdditionIntervalOp for calculating intervals,
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
    const expected = toF32Matrix(t.params.expected);
    const got = additionMatrixInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `additionMatrixInterval([${JSON.stringify(x)}], [${JSON.stringify(
        y
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

g.test('subtractionMatrixInterval')
  .paramsSubcasesOnly<MatrixPairToMatrixCase>([
    // Only testing that different shapes of matrices are handled correctly
    // here, to reduce test duplication.
    // subtractionMatrixInterval uses SubtractionIntervalOp for calculating intervals,
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
    const expected = toF32Matrix(t.params.expected);
    const got = subtractionMatrixInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `subtractionMatrixInterval([${JSON.stringify(x)}], [${JSON.stringify(
        y
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

g.test('multiplicationMatrixMatrixInterval')
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
    const expected = toF32Matrix(t.params.expected);
    const got = multiplicationMatrixMatrixInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `multiplicationMatrixMatrixInterval([${JSON.stringify(x)}], [${JSON.stringify(
        y
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

interface MatrixScalarToMatrixCase {
  matrix: number[][];
  scalar: number;
  expected: (number | IntervalBounds)[][];
}

g.test('multiplicationMatrixScalarInterval')
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
    const expected = toF32Matrix(t.params.expected);
    const got = multiplicationMatrixScalarInterval(matrix, scalar);
    t.expect(
      objectEquals(expected, got),
      `multiplicationMatrixScalarInterval([${JSON.stringify(
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

g.test('multiplicationMatrixVectorInterval')
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
    const expected = toF32Vector(t.params.expected);
    const got = multiplicationMatrixVectorInterval(matrix, vector);
    t.expect(
      objectEquals(expected, got),
      `multiplicationMatrixVectorInterval([${JSON.stringify(matrix)}], [${JSON.stringify(
        vector
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });

interface VectorMatrixToVectorCase {
  vector: number[];
  matrix: number[][];
  expected: (number | IntervalBounds)[];
}

g.test('multiplicationVectorMatrixInterval')
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
    const expected = toF32Vector(t.params.expected);
    const got = multiplicationVectorMatrixInterval(vector, matrix);
    t.expect(
      objectEquals(expected, got),
      `multiplicationVectorMatrixInterval([${JSON.stringify(vector)}], [${JSON.stringify(
        matrix
      )}]) returned '[${JSON.stringify(got)}]'. Expected '[${JSON.stringify(expected)}]'`
    );
  });
