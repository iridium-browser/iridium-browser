export const description = `
F32Interval unit tests.
`;

import { makeTestGroup } from '../common/framework/test_group.js';
import { assert, objectEquals } from '../common/util/util.js';
import { kValue } from '../webgpu/util/constants.js';
import {
  absInterval,
  absoluteErrorInterval,
  additionInterval,
  atanInterval,
  atan2Interval,
  ceilInterval,
  clampMedianInterval,
  clampMinMaxInterval,
  correctlyRoundedInterval,
  cosInterval,
  divisionInterval,
  expInterval,
  exp2Interval,
  F32Interval,
  floorInterval,
  fractInterval,
  inverseSqrtInterval,
  logInterval,
  log2Interval,
  maxInterval,
  minInterval,
  multiplicationInterval,
  negationInterval,
  tanInterval,
  sinInterval,
  subtractionInterval,
  ulpInterval,
} from '../webgpu/util/f32_interval.js';
import { hexToF32, hexToF64, oneULP } from '../webgpu/util/math.js';

import { UnitTest } from './unit_test.js';

export const g = makeTestGroup(UnitTest);

/** Convert a pair of numbers in an array to a F32Interval
 *
 * Used for fluently specifying test params as `[a, b]` instead of
 * `new F32Interval(a, b)`
 */
function arrayToInterval(bounds: [number, number]): F32Interval {
  const [begin, end] = bounds;
  return new F32Interval(begin, end);
}

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

interface ConstructorCase {
  input: [number, number];
  expected: [number, number];
}

g.test('constructor')
  .paramsSubcasesOnly<ConstructorCase>(
    // prettier-ignore
    [
      // Common cases
      { input: [0, 10], expected: [0, 10]},
      { input: [-5, 0], expected: [-5, 0]},
      { input: [-5, 10], expected: [-5, 10]},
      { input: [0, 0], expected: [0, 0]},
      { input: [10, 10], expected: [10, 10]},
      { input: [-5, -5], expected: [-5, -5]},

      // Edges
      { input: [0, kValue.f32.positive.max], expected: [0, kValue.f32.positive.max]},
      { input: [kValue.f32.negative.min, 0], expected: [kValue.f32.negative.min, 0]},
      { input: [kValue.f32.negative.min, kValue.f32.positive.max], expected: [kValue.f32.negative.min, kValue.f32.positive.max]},

      // Out of range
      { input: [0, 2 * kValue.f32.positive.max], expected: [0, Number.POSITIVE_INFINITY]},
      { input: [2 * kValue.f32.negative.min, 0], expected: [Number.NEGATIVE_INFINITY, 0]},
      { input: [2 * kValue.f32.negative.min, 2 * kValue.f32.positive.max], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]},

      // Infinities
      { input: [0, kValue.f32.infinity.positive], expected: [0, Number.POSITIVE_INFINITY]},
      { input: [kValue.f32.infinity.negative, 0], expected: [Number.NEGATIVE_INFINITY, 0]},
      { input: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]},
    ]
  )
  .fn(t => {
    const [input_begin, input_end] = t.params.input;
    const [expected_begin, expected_end] = t.params.expected;

    const i = new F32Interval(input_begin, input_end);
    t.expect(
      i.begin === expected_begin && i.end === expected_end,
      `F32Interval(${input_begin}, ${input_end}) returned ${i}. Expected [${expected_begin}, ${expected_end}`
    );
  });

interface ContainsNumberCase {
  bounds: [number, number];
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
    { bounds: [0, 0], value: 0, expected: true },
    { bounds: [0, 0], value: 10, expected: false },
    { bounds: [0, 0], value: -1000, expected: false },
    { bounds: [10, 10], value: 10, expected: true },
    { bounds: [10, 10], value: 0, expected: false },
    { bounds: [10, 10], value: -10, expected: false },
    { bounds: [10, 10], value: 11, expected: false },

    // Upper infinity
    { bounds: [0, kValue.f32.infinity.positive], value: kValue.f32.positive.min, expected: true },
    { bounds: [0, kValue.f32.infinity.positive], value: kValue.f32.positive.max, expected: true },
    { bounds: [0, kValue.f32.infinity.positive], value: Number.POSITIVE_INFINITY, expected: true },
    { bounds: [0, kValue.f32.infinity.positive], value: kValue.f32.negative.min, expected: false },
    { bounds: [0, kValue.f32.infinity.positive], value: kValue.f32.negative.max, expected: false },
    { bounds: [0, kValue.f32.infinity.positive], value: Number.NEGATIVE_INFINITY, expected: false },

    // Lower infinity
    { bounds: [kValue.f32.infinity.negative, 0], value: kValue.f32.positive.min, expected: false },
    { bounds: [kValue.f32.infinity.negative, 0], value: kValue.f32.positive.max, expected: false },
    { bounds: [kValue.f32.infinity.negative, 0], value: Number.POSITIVE_INFINITY, expected: false },
    { bounds: [kValue.f32.infinity.negative, 0], value: kValue.f32.negative.min, expected: true },
    { bounds: [kValue.f32.infinity.negative, 0], value: kValue.f32.negative.max, expected: true },
    { bounds: [kValue.f32.infinity.negative, 0], value: Number.NEGATIVE_INFINITY, expected: true },

    // Full infinity
    { bounds: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], value: kValue.f32.positive.min, expected: true },
    { bounds: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], value: kValue.f32.positive.max, expected: true },
    { bounds: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], value: Number.POSITIVE_INFINITY, expected: true },
    { bounds: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], value: kValue.f32.negative.min, expected: true },
    { bounds: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], value: kValue.f32.negative.max, expected: true },
    { bounds: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], value: Number.NEGATIVE_INFINITY, expected: true },
    { bounds: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], value: Number.NaN, expected: true },

    // Maximum f32 boundary
    { bounds: [0, kValue.f32.positive.max], value: kValue.f32.positive.min, expected: true },
    { bounds: [0, kValue.f32.positive.max], value: kValue.f32.positive.max, expected: true },
    { bounds: [0, kValue.f32.positive.max], value: Number.POSITIVE_INFINITY, expected: false },
    { bounds: [0, kValue.f32.positive.max], value: kValue.f32.negative.min, expected: false },
    { bounds: [0, kValue.f32.positive.max], value: kValue.f32.negative.max, expected: false },
    { bounds: [0, kValue.f32.positive.max], value: Number.NEGATIVE_INFINITY, expected: false },

    // Minimum f32 boundary
    { bounds: [kValue.f32.negative.min, 0], value: kValue.f32.positive.min, expected: false },
    { bounds: [kValue.f32.negative.min, 0], value: kValue.f32.positive.max, expected: false },
    { bounds: [kValue.f32.negative.min, 0], value: Number.POSITIVE_INFINITY, expected: false },
    { bounds: [kValue.f32.negative.min, 0], value: kValue.f32.negative.min, expected: true },
    { bounds: [kValue.f32.negative.min, 0], value: kValue.f32.negative.max, expected: true },
    { bounds: [kValue.f32.negative.min, 0], value: Number.NEGATIVE_INFINITY, expected: false },

    // Out of range high
    { bounds: [0, 2 * kValue.f32.positive.max], value: kValue.f32.positive.min, expected: true },
    { bounds: [0, 2 * kValue.f32.positive.max], value: kValue.f32.positive.max, expected: true },
    { bounds: [0, 2 * kValue.f32.positive.max], value: Number.POSITIVE_INFINITY, expected: true },
    { bounds: [0, 2 * kValue.f32.positive.max], value: kValue.f32.negative.min, expected: false },
    { bounds: [0, 2 * kValue.f32.positive.max], value: kValue.f32.negative.max, expected: false },
    { bounds: [0, 2 * kValue.f32.positive.max], value: Number.NEGATIVE_INFINITY, expected: false },

    // Out of range low
    { bounds: [2 * kValue.f32.negative.min, 0], value: kValue.f32.positive.min, expected: false },
    { bounds: [2 * kValue.f32.negative.min, 0], value: kValue.f32.positive.max, expected: false },
    { bounds: [2 * kValue.f32.negative.min, 0], value: Number.POSITIVE_INFINITY, expected: false },
    { bounds: [2 * kValue.f32.negative.min, 0], value: kValue.f32.negative.min, expected: true },
    { bounds: [2 * kValue.f32.negative.min, 0], value: kValue.f32.negative.max, expected: true },
    { bounds: [2 * kValue.f32.negative.min, 0], value: Number.NEGATIVE_INFINITY, expected: true },

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
    const i = arrayToInterval(t.params.bounds);
    const value = t.params.value;
    const expected = t.params.expected;

    const got = i.contains(value);
    t.expect(expected === got, `${i}.contains(${value}) returned ${got}. Expected ${expected}`);
  });

interface ContainsIntervalCase {
  lhs: [number, number];
  rhs: [number, number];
  expected: boolean;
}

g.test('contains_interval')
  .paramsSubcasesOnly<ContainsIntervalCase>(
    // prettier-ignore
    [
      // Common usage
      { lhs: [-10, 10], rhs: [0, 0], expected: true},
      { lhs: [-10, 10], rhs: [-1, 0], expected: true},
      { lhs: [-10, 10], rhs: [0, 2], expected: true},
      { lhs: [-10, 10], rhs: [-1, 2], expected: true},
      { lhs: [-10, 10], rhs: [0, 10], expected: true},
      { lhs: [-10, 10], rhs: [-10, 2], expected: true},
      { lhs: [-10, 10], rhs: [-10, 10], expected: true},
      { lhs: [-10, 10], rhs: [-100, 10], expected: false},

      // Upper infinity
      { lhs: [0, kValue.f32.infinity.positive], rhs: [0, 0], expected: true},
      { lhs: [0, kValue.f32.infinity.positive], rhs: [-1, 0], expected: false},
      { lhs: [0, kValue.f32.infinity.positive], rhs: [0, 1], expected: true},
      { lhs: [0, kValue.f32.infinity.positive], rhs: [0, kValue.f32.positive.max], expected: true},
      { lhs: [0, kValue.f32.infinity.positive], rhs: [0, Number.POSITIVE_INFINITY], expected: true},
      { lhs: [0, kValue.f32.infinity.positive], rhs: [100, Number.POSITIVE_INFINITY], expected: true},
      { lhs: [0, kValue.f32.infinity.positive], rhs: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY], expected: false},

      // Lower infinity
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [0, 0], expected: true},
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [-1, 0], expected: true},
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [kValue.f32.negative.min, 0], expected: true},
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [0, 1], expected: false},
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [Number.NEGATIVE_INFINITY, 0], expected: true},
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [Number.NEGATIVE_INFINITY, -100 ], expected: true},
      { lhs: [kValue.f32.infinity.negative, 0], rhs: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY], expected: false},

      // Full infinity
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [0, 0], expected: true},
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [-1, 0], expected: true},
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [0, 1], expected: true},
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [0, Number.POSITIVE_INFINITY], expected: true},
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [100, Number.POSITIVE_INFINITY], expected: true},
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [Number.NEGATIVE_INFINITY, 0], expected: true},
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [Number.NEGATIVE_INFINITY, -100 ], expected: true},
      { lhs: [kValue.f32.infinity.negative, kValue.f32.infinity.positive], rhs: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY], expected: true},

      // Maximum f32 boundary
      { lhs: [0, kValue.f32.positive.max], rhs: [0, 0], expected: true},
      { lhs: [0, kValue.f32.positive.max], rhs: [-1, 0], expected: false},
      { lhs: [0, kValue.f32.positive.max], rhs: [0, 1], expected: true},
      { lhs: [0, kValue.f32.positive.max], rhs: [0, kValue.f32.positive.max], expected: true},
      { lhs: [0, kValue.f32.positive.max], rhs: [0, Number.POSITIVE_INFINITY], expected: false},
      { lhs: [0, kValue.f32.positive.max], rhs: [100, Number.POSITIVE_INFINITY], expected: false},
      { lhs: [0, kValue.f32.positive.max], rhs: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY], expected: false},

      // Minimum f32 boundary
      { lhs: [kValue.f32.negative.min, 0], rhs: [0, 0], expected: true},
      { lhs: [kValue.f32.negative.min, 0], rhs: [-1, 0], expected: true},
      { lhs: [kValue.f32.negative.min, 0], rhs: [kValue.f32.negative.min, 0], expected: true},
      { lhs: [kValue.f32.negative.min, 0], rhs: [0, 1], expected: false},
      { lhs: [kValue.f32.negative.min, 0], rhs: [Number.NEGATIVE_INFINITY, 0], expected: false},
      { lhs: [kValue.f32.negative.min, 0], rhs: [Number.NEGATIVE_INFINITY, -100 ], expected: false},
      { lhs: [kValue.f32.negative.min, 0], rhs: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY], expected: false},

      // Out of range high
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [0, 0], expected: true},
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [-1, 0], expected: false},
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [0, 1], expected: true},
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [0, kValue.f32.positive.max], expected: true},
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [0, Number.POSITIVE_INFINITY], expected: true},
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [100, Number.POSITIVE_INFINITY], expected: true},
      { lhs: [0, 2 * kValue.f32.positive.max], rhs: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY], expected: false},

      // Out of range low
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [0, 0], expected: true},
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [-1, 0], expected: true},
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [kValue.f32.negative.min, 0], expected: true},
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [0, 1], expected: false},
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [Number.NEGATIVE_INFINITY, 0], expected: true},
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [Number.NEGATIVE_INFINITY, -100 ], expected: true},
      { lhs: [2 * kValue.f32.negative.min, 0], rhs: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY], expected: false},
    ]
  )
  .fn(t => {
    const lhs = arrayToInterval(t.params.lhs);
    const rhs = arrayToInterval(t.params.rhs);
    const expected = t.params.expected;

    const got = lhs.contains(rhs);
    t.expect(expected === got, `${lhs}.contains(${rhs}) returned ${got}. Expected ${expected}`);
  });

interface SpanCase {
  intervals: Array<[number, number]>;
  expected: [number, number];
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
      { intervals: [[kValue.f32.infinity.negative, 0], [0, kValue.f32.infinity.positive]], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]},

      // Multiple Intervals
      { intervals: [[0, 1], [2, 3], [4, 5]], expected: [0, 5]},
      { intervals: [[0, 1], [4, 5], [2, 3]], expected: [0, 5]},
      { intervals: [[0, 1], [0, 1], [0, 1]], expected: [0, 1]},
    ]
  )
  .fn(t => {
    const intervals = t.params.intervals.map(arrayToInterval);
    const expected = arrayToInterval(t.params.expected);

    const got = F32Interval.span(...intervals);
    t.expect(
      objectEquals(got, expected),
      `span({${intervals}}) returned ${got}. Expected ${expected}`
    );
  });

interface CorrectlyRoundedCase {
  value: number;
  expected: [number, number];
}

g.test('correctlyRoundedInterval')
  .paramsSubcasesOnly<CorrectlyRoundedCase>(
    // prettier-ignore
    [
      // Edge Cases
      { value: kValue.f32.infinity.positive, expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { value: kValue.f32.infinity.negative, expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { value: kValue.f32.positive.max, expected: [kValue.f32.positive.max, kValue.f32.positive.max] },
      { value: kValue.f32.negative.min, expected: [kValue.f32.negative.min, kValue.f32.negative.min] },
      { value: kValue.f32.positive.min, expected: [kValue.f32.positive.min, kValue.f32.positive.min] },
      { value: kValue.f32.negative.max, expected: [kValue.f32.negative.max, kValue.f32.negative.max] },

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
      { value: hexToF32(0x03800000), expected: [hexToF32(0x03800000), hexToF32(0x03800000)] },
      { value: hexToF32(0x03800001), expected: [hexToF32(0x03800001), hexToF32(0x03800001)] },
      { value: hexToF32(0x83800000), expected: [hexToF32(0x83800000), hexToF32(0x83800000)] },
      { value: hexToF32(0x83800001), expected: [hexToF32(0x83800001), hexToF32(0x83800001)] },

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
    const value = t.params.value;
    const expected = arrayToInterval(t.params.expected);

    const got = correctlyRoundedInterval(value);
    t.expect(
      objectEquals(expected, got),
      `correctlyRoundedInterval(${value}) returned ${got}. Expected ${expected}`
    );
  });

interface AbsoluteErrorCase {
  value: number;
  error: number;
  expected: [number, number];
}

g.test('absoluteErrorInterval')
  .paramsSubcasesOnly<AbsoluteErrorCase>(
    // prettier-ignore
    [
      // Edge Cases
      { value: kValue.f32.infinity.positive, error: 0, expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { value: kValue.f32.infinity.positive, error: 2 ** -11, expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { value: kValue.f32.infinity.positive, error: 1, expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { value: kValue.f32.infinity.negative, error: 0, expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { value: kValue.f32.infinity.negative, error: 2 ** -11, expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { value: kValue.f32.infinity.negative, error: 1, expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { value: kValue.f32.positive.max, error: 0, expected: [kValue.f32.positive.max, kValue.f32.positive.max] },
      { value: kValue.f32.positive.max, error: 2 ** -11, expected: [kValue.f32.positive.max, kValue.f32.positive.max] },
      { value: kValue.f32.positive.max, error: kValue.f32.positive.max, expected: [0, Number.POSITIVE_INFINITY] },
      { value: kValue.f32.positive.min, error: 0, expected: [kValue.f32.positive.min,  kValue.f32.positive.min] },
      { value: kValue.f32.positive.min, error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: kValue.f32.positive.min, error: 1, expected: [-1, 1] },
      { value: kValue.f32.negative.min, error: 0, expected: [kValue.f32.negative.min, kValue.f32.negative.min] },
      { value: kValue.f32.negative.min, error: 2 ** -11, expected: [kValue.f32.negative.min, kValue.f32.negative.min] },
      { value: kValue.f32.negative.min, error: kValue.f32.positive.max, expected: [Number.NEGATIVE_INFINITY, 0] },
      { value: kValue.f32.negative.max, error: 0, expected: [kValue.f32.negative.max, kValue.f32.negative.max] },
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
      { value: 0, error: 0, expected: [0, 0] },
      { value: 0, error: 2 ** -11, expected: [-(2 ** -11), 2 ** -11] },
      { value: 0, error: 1, expected: [-1, 1] },
    ]
  )
  .fn(t => {
    const value = t.params.value;
    const error = t.params.error;
    const expected = arrayToInterval(t.params.expected);

    const got = absoluteErrorInterval(value, error);
    t.expect(
      objectEquals(expected, got),
      `absoluteErrorInterval(${value}, ${error}) returned ${got}. Expected ${expected}`
    );
  });

interface ULPCase {
  value: number;
  num_ulp: number;
  expected: [number, number];
}

g.test('ulpInterval')
  .paramsSubcasesOnly<ULPCase>(
    // prettier-ignore
    [
      // Edge Cases
      { value: kValue.f32.infinity.positive, num_ulp: 0, expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { value: kValue.f32.infinity.positive, num_ulp: 1, expected: [minusOneULP(kValue.f32.positive.max), Number.POSITIVE_INFINITY] },
      { value: kValue.f32.infinity.positive, num_ulp: 4096, expected: [minusNULP(kValue.f32.positive.max, 4096), Number.POSITIVE_INFINITY] },
      { value: kValue.f32.infinity.negative, num_ulp: 0, expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { value: kValue.f32.infinity.negative, num_ulp: 1, expected: [Number.NEGATIVE_INFINITY, plusOneULP(kValue.f32.negative.min)] },
      { value: kValue.f32.infinity.negative, num_ulp: 4096, expected: [Number.NEGATIVE_INFINITY, plusNULP(kValue.f32.negative.min, 4096)] },
      { value: kValue.f32.positive.max, num_ulp: 0, expected: [kValue.f32.positive.max, kValue.f32.positive.max] },
      { value: kValue.f32.positive.max, num_ulp: 1, expected: [minusOneULP(kValue.f32.positive.max), Number.POSITIVE_INFINITY] },
      { value: kValue.f32.positive.max, num_ulp: 4096, expected: [minusNULP(kValue.f32.positive.max, 4096), Number.POSITIVE_INFINITY] },
      { value: kValue.f32.positive.min, num_ulp: 0, expected: [kValue.f32.positive.min, kValue.f32.positive.min] },
      { value: kValue.f32.positive.min, num_ulp: 1, expected: [0, plusOneULP(kValue.f32.positive.min)] },
      { value: kValue.f32.positive.min, num_ulp: 4096, expected: [0, plusNULP(kValue.f32.positive.min, 4096)] },
      { value: kValue.f32.negative.min, num_ulp: 0, expected: [kValue.f32.negative.min, kValue.f32.negative.min] },
      { value: kValue.f32.negative.min, num_ulp: 1, expected: [Number.NEGATIVE_INFINITY, plusOneULP(kValue.f32.negative.min)] },
      { value: kValue.f32.negative.min, num_ulp: 4096, expected: [Number.NEGATIVE_INFINITY, plusNULP(kValue.f32.negative.min, 4096)] },
      { value: kValue.f32.negative.max, num_ulp: 0, expected: [kValue.f32.negative.max, kValue.f32.negative.max] },
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
      { value: 0, num_ulp: 0, expected: [0, 0] },
      { value: 0, num_ulp: 1, expected: [minusOneULP(0), plusOneULP(0)] },
      { value: 0, num_ulp: 4096, expected: [minusNULP(0, 4096), plusNULP(0, 4096)] },
    ]
  )
  .fn(t => {
    const value = t.params.value;
    const num_ulp = t.params.num_ulp;
    const expected = arrayToInterval(t.params.expected);

    const got = ulpInterval(value, num_ulp);
    t.expect(
      objectEquals(expected, got),
      `ulpInterval(${value}, ${num_ulp}) returned ${got}. Expected ${expected}`
    );
  });

interface PointToIntervalCase {
  input: number;
  // If expected is an Array of two values, the test should interpret them as
  // human readable begin/end values that need to be adjusted for the error
  // calculation specific to the function being tested.
  // This is to facilitate writing tests in a fluent manner, i.e. being able to
  // express a expectation as `plusOneULP(π/2)` instead of
  // `π/2 + ULP(π/2) + 4096 * ULP(π/2 + ULP(π/2))`
  //
  // If expected is an F32Interval, it is to be interpreted as an exact value,
  // with no additional processing. This is to allow for expressing specific
  // cases with human readable values that would require effectively
  // implementing the entire interval system under test to accommodate them in
  // the test error calculation.
  expected: [number, number] | F32Interval;
}

g.test('absInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      // Common usages
      { input: 1, expected: [1, 1] },
      { input: -1, expected: [1, 1] },
      { input: 0.1, expected: [hexToF32(0x3dcccccc), hexToF32(0x3dcccccd)] },
      { input: -0.1, expected: [hexToF32(0x3dcccccc), hexToF32(0x3dcccccd)] },

      // Edge cases
      { input: kValue.f32.infinity.positive, expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: kValue.f32.infinity.negative, expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: kValue.f32.positive.max, expected: [kValue.f32.positive.max, kValue.f32.positive.max] },
      { input: kValue.f32.positive.min, expected: [kValue.f32.positive.min, kValue.f32.positive.min] },
      { input: kValue.f32.negative.min, expected: [kValue.f32.positive.max, kValue.f32.positive.max] },
      { input: kValue.f32.negative.max, expected: [kValue.f32.positive.min, kValue.f32.positive.min] },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [0, kValue.f32.subnormal.positive.max] },
      { input: kValue.f32.subnormal.positive.min, expected: [0, kValue.f32.subnormal.positive.min] },
      { input: kValue.f32.subnormal.negative.min, expected: [0, kValue.f32.subnormal.positive.max] },
      { input: kValue.f32.subnormal.negative.max, expected: [0, kValue.f32.subnormal.positive.min] },

      // 64-bit subnormals
      { input: hexToF64(0x00000000, 0x00000001), expected: [0, kValue.f32.subnormal.positive.min] },
      { input: hexToF64(0x800fffff, 0xffffffff), expected: [0, kValue.f32.subnormal.positive.min] },

      // Zero
      { input: 0, expected: [0, 0]},
    ]
  )
  .fn(t => {
    const input = t.params.input;
    const expected =
      t.params.expected instanceof Array ? arrayToInterval(t.params.expected) : t.params.expected;

    const got = absInterval(input);
    t.expect(
      objectEquals(expected, got),
      `absInterval(${input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('atanInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: Number.NEGATIVE_INFINITY, expected: [kValue.f32.negative.pi.half, plusOneULP(kValue.f32.negative.pi.half)] },
      { input: hexToF32(0xbfddb3d7), expected: [kValue.f32.negative.pi.third, plusOneULP(kValue.f32.negative.pi.third)] }, // x = -√3
      { input: -1, expected: [kValue.f32.negative.pi.quarter, plusOneULP(kValue.f32.negative.pi.quarter)] },
      { input: hexToF32(0xbf13cd3a), expected: [kValue.f32.negative.pi.sixth, plusOneULP(kValue.f32.negative.pi.sixth)] },  // x = -1/√3
      { input: 0, expected: [0, 0] },
      { input: hexToF32(0x3f13cd3a), expected: [minusOneULP(kValue.f32.positive.pi.sixth), kValue.f32.positive.pi.sixth] },  // x = 1/√3
      { input: 1, expected: [minusOneULP(kValue.f32.positive.pi.quarter), kValue.f32.positive.pi.quarter] },
      { input: hexToF32(0x3fddb3d7), expected: [minusOneULP(kValue.f32.positive.pi.third), kValue.f32.positive.pi.third] }, // x = √3
      { input: Number.POSITIVE_INFINITY, expected: [minusOneULP(kValue.f32.positive.pi.half), kValue.f32.positive.pi.half] },
    ]
  )
  .fn(t => {
    const error = (x: number): number => {
      return 4096 * oneULP(x);
    };

    const input = t.params.input;
    let expected: F32Interval;
    if (t.params.expected instanceof Array) {
      const [begin, end] = t.params.expected;
      expected = arrayToInterval([begin - error(begin), end + error(end)]);
    } else {
      expected = t.params.expected;
    }

    const got = atanInterval(input);
    t.expect(
      objectEquals(expected, got),
      `atanInterval(${input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('ceilInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: [0, 0] },
      { input: 0.1, expected: [1, 1] },
      { input: 0.9, expected: [1, 1] },
      { input: 1.0, expected: [1, 1] },
      { input: 1.1, expected: [2, 2] },
      { input: 1.9, expected: [2, 2] },
      { input: -0.1, expected: [0, 0] },
      { input: -0.9, expected: [0, 0] },
      { input: -1.0, expected: [-1, -1] },
      { input: -1.1, expected: [-1, -1] },
      { input: -1.9, expected: [-1, -1] },

      // Edge cases
      { input: Number.POSITIVE_INFINITY, expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: Number.NEGATIVE_INFINITY, expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: kValue.f32.positive.max, expected: [kValue.f32.positive.max, kValue.f32.positive.max] },
      { input: kValue.f32.positive.min, expected: [1, 1] },
      { input: kValue.f32.negative.min, expected: [kValue.f32.negative.min, kValue.f32.negative.min] },
      { input: kValue.f32.negative.max, expected: [0, 0] },
      { input: kValue.powTwo.to30, expected: [kValue.powTwo.to30, kValue.powTwo.to30] },
      { input: -kValue.powTwo.to30, expected: [-kValue.powTwo.to30, -kValue.powTwo.to30] },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [0, 1] },
      { input: kValue.f32.subnormal.positive.min, expected: [0, 1] },
      { input: kValue.f32.subnormal.negative.min, expected: [0, 0] },
      { input: kValue.f32.subnormal.negative.max, expected: [0, 0] },
    ]
  )
  .fn(t => {
    const input = t.params.input;
    const expected =
      t.params.expected instanceof Array ? arrayToInterval(t.params.expected) : t.params.expected;

    const got = ceilInterval(input);
    t.expect(
      objectEquals(expected, got),
      `ceilInterval(${input}) returned ${got}. Expected ${expected}`
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
      { input: Number.NEGATIVE_INFINITY, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: kValue.f32.negative.min, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: kValue.f32.negative.pi.whole, expected: [-1, plusOneULP(-1)] },
      { input: kValue.f32.negative.pi.third, expected: [minusOneULP(1/2), 1/2] },
      { input: 0, expected: [1, 1] },
      { input: kValue.f32.positive.pi.third, expected: [minusOneULP(1/2), 1/2] },
      { input: kValue.f32.positive.pi.whole, expected: [-1, plusOneULP(-1)] },
      { input: kValue.f32.positive.max, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: Number.POSITIVE_INFINITY, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
    ]
  )
  .fn(t => {
    const error = 2 ** -11;

    const input = t.params.input;
    let expected: F32Interval;
    if (t.params.expected instanceof Array) {
      const [begin, end] = t.params.expected;
      expected = arrayToInterval([begin - error, end + error]);
    } else {
      expected = t.params.expected;
    }

    const got = cosInterval(input);
    t.expect(
      objectEquals(expected, got),
      `cosInterval(${input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('expInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: Number.NEGATIVE_INFINITY, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: 0, expected: [1,1] },
      { input: 1, expected: [kValue.f32.positive.e, plusOneULP(kValue.f32.positive.e)] },
      { input: 89, expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
    ]
  )
  .fn(t => {
    const error = (input: number, result: number): number => {
      const n = 3 + 2 * Math.abs(input);
      return n * oneULP(result);
    };

    const input = t.params.input;
    let expected: F32Interval;
    if (t.params.expected instanceof Array) {
      const [begin, end] = t.params.expected;
      expected = arrayToInterval([begin - error(input, begin), end + error(input, end)]);
    } else {
      expected = t.params.expected;
    }

    const got = expInterval(input);
    t.expect(
      objectEquals(expected, got),
      `expInterval(${input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('exp2Interval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: Number.NEGATIVE_INFINITY, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: 0, expected: [1,1] },
      { input: 1, expected: [2, 2] },
      { input: 128, expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
    ]
  )
  .fn(t => {
    const error = (input: number, result: number): number => {
      const n = 3 + 2 * Math.abs(input);
      return n * oneULP(result);
    };

    const input = t.params.input;
    let expected: F32Interval;
    if (t.params.expected instanceof Array) {
      const [begin, end] = t.params.expected;
      expected = arrayToInterval([begin - error(input, begin), end + error(input, end)]);
    } else {
      expected = t.params.expected;
    }

    const got = exp2Interval(input);
    t.expect(
      objectEquals(expected, got),
      `exp2Interval(${input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('floorInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: [0, 0] },
      { input: 0.1, expected: [0, 0] },
      { input: 0.9, expected: [0, 0] },
      { input: 1.0, expected: [1, 1] },
      { input: 1.1, expected: [1, 1] },
      { input: 1.9, expected: [1, 1] },
      { input: -0.1, expected: [-1, -1] },
      { input: -0.9, expected: [-1, -1] },
      { input: -1.0, expected: [-1, -1] },
      { input: -1.1, expected: [-2, -2] },
      { input: -1.9, expected: [-2, -2] },

      // Edge cases
      { input: Number.POSITIVE_INFINITY, expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: Number.NEGATIVE_INFINITY, expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: kValue.f32.positive.max, expected: [kValue.f32.positive.max, kValue.f32.positive.max] },
      { input: kValue.f32.positive.min, expected: [0, 0] },
      { input: kValue.f32.negative.min, expected: [kValue.f32.negative.min, kValue.f32.negative.min] },
      { input: kValue.f32.negative.max, expected: [-1, -1] },
      { input: kValue.powTwo.to30, expected: [kValue.powTwo.to30, kValue.powTwo.to30] },
      { input: -kValue.powTwo.to30, expected: [-kValue.powTwo.to30, -kValue.powTwo.to30] },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [0, 0] },
      { input: kValue.f32.subnormal.positive.min, expected: [0, 0] },
      { input: kValue.f32.subnormal.negative.min, expected: [-1, 0] },
      { input: kValue.f32.subnormal.negative.max, expected: [-1, 0] },
    ]
  )
  .fn(t => {
    const input = t.params.input;
    const expected =
      t.params.expected instanceof Array ? arrayToInterval(t.params.expected) : t.params.expected;

    const got = floorInterval(input);
    t.expect(
      objectEquals(expected, got),
      `floorInterval(${input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('fractInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: [0, 0] },
      { input: 0.1, expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] }, // ~0.1
      { input: 0.9, expected: [hexToF32(0x3f666666), plusOneULP(hexToF32(0x3f666666))] },  // ~0.9
      { input: 1.0, expected: [0.0, 0.0] },
      { input: 1.1, expected: [hexToF64(0x3fb99998, 0x00000000), hexToF64(0x3fb9999a, 0x00000000)] }, // ~0.1
      { input: -0.1, expected: [hexToF32(0x3f666666), plusOneULP(hexToF32(0x3f666666))] },  // ~0.9
      { input: -0.9, expected: [hexToF64(0x3fb99999, 0x00000000), hexToF64(0x3fb9999a, 0x00000000)] }, // ~0.1
      { input: -1.0, expected: [0.0, 0.0] },
      { input: -1.1, expected: [hexToF64(0x3feccccc, 0xc0000000), hexToF64(0x3feccccd, 0x00000000), ] }, // ~0.9

      // Edge cases
      { input: Number.POSITIVE_INFINITY, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: Number.NEGATIVE_INFINITY, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]  },
      { input: kValue.f32.positive.max, expected: [0, 0] },
      { input: kValue.f32.positive.min, expected: [kValue.f32.positive.min, kValue.f32.positive.min] },
      { input: kValue.f32.negative.min, expected: [0, 0] },
      { input: kValue.f32.negative.max, expected: [kValue.f32.positive.less_than_one, 1.0] },
    ]
  )
  .fn(t => {
    const input = t.params.input;
    const expected =
      t.params.expected instanceof Array ? arrayToInterval(t.params.expected) : t.params.expected;

    const got = fractInterval(input);
    t.expect(
      objectEquals(expected, got),
      `fractInterval(${input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('inverseSqrtInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: -1, expected: arrayToInterval([Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]) },
      { input: 0, expected: arrayToInterval([Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]) },
      { input: 0.04, expected: [minusOneULP(5), plusOneULP(5)] },
      { input: 1, expected: [1, 1] },
      { input: 100, expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: kValue.f32.positive.max, expected: [hexToF32(0x1f800000), plusNULP(hexToF32(0x1f800000), 2)] },  // ~5.421...e-20, i.e. 1/√max f32
      { input: Number.POSITIVE_INFINITY, expected: [0, plusNULP(hexToF32(0x1f800000), 2)] },
    ]
  )
  .fn(t => {
    const error = (input: number, result: number): number => {
      return 2 * oneULP(result);
    };

    const input = t.params.input;
    let expected: F32Interval;
    if (t.params.expected instanceof Array) {
      const [begin, end] = t.params.expected;
      expected = arrayToInterval([begin - error(input, begin), end + error(input, end)]);
    } else {
      expected = t.params.expected;
    }

    const got = inverseSqrtInterval(input);
    t.expect(
      objectEquals(expected, got),
      `inverseSqrtInterval(${input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('logInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: -1, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: 0, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: 1, expected: [0, 0] },
      { input: kValue.f32.positive.e, expected: [minusOneULP(1), 1] },
      { input: kValue.f32.positive.max, expected: [minusOneULP(hexToF32(0x42b17218)), hexToF32(0x42b17218)] },  // ~88.72...
    ]
  )
  .fn(t => {
    const error = (input: number, result: number): number => {
      if (input >= 0.5 && input <= 2.0) {
        return 2 ** -21;
      }
      return 3 * oneULP(result);
    };

    const input = t.params.input;
    let expected: F32Interval;
    if (t.params.expected instanceof Array) {
      const [begin, end] = t.params.expected;
      expected = arrayToInterval([begin - error(input, begin), end + error(input, end)]);
    } else {
      expected = t.params.expected;
    }

    const got = logInterval(input);
    t.expect(
      objectEquals(expected, got),
      `logInterval(${input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('log2Interval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: -1, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: 0, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: 1, expected: [0, 0] },
      { input: 2, expected: [1, 1] },
      { input: kValue.f32.positive.max, expected: [minusOneULP(128), 128] },
    ]
  )
  .fn(t => {
    const error = (input: number, result: number): number => {
      if (input >= 0.5 && input <= 2.0) {
        return 2 ** -21;
      }
      return 3 * oneULP(result);
    };

    const input = t.params.input;
    let expected: F32Interval;
    if (t.params.expected instanceof Array) {
      const [begin, end] = t.params.expected;
      expected = arrayToInterval([begin - error(input, begin), end + error(input, end)]);
    } else {
      expected = t.params.expected;
    }

    const got = log2Interval(input);
    t.expect(
      objectEquals(expected, got),
      `log2Interval(${input}) returned ${got}. Expected ${expected}`
    );
  });

g.test('negationInterval')
  .paramsSubcasesOnly<PointToIntervalCase>(
    // prettier-ignore
    [
      { input: 0, expected: [0, 0] },
      { input: 0.1, expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] }, // ~-0.1
      { input: 1.0, expected: [-1.0, -1.0] },
      { input: 1.9, expected: [hexToF32(0xbff33334), plusOneULP(hexToF32(0xbff33334))] },  // ~-1.9
      { input: -0.1, expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] }, // ~0.1
      { input: -1.0, expected: [1, 1] },
      { input: -1.9, expected: [minusOneULP(hexToF32(0x3ff33334)), hexToF32(0x3ff33334)] },  // ~1.9

      // Edge cases
      { input: Number.POSITIVE_INFINITY, expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: Number.NEGATIVE_INFINITY, expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: kValue.f32.positive.max, expected: [kValue.f32.negative.min, kValue.f32.negative.min] },
      { input: kValue.f32.positive.min, expected: [kValue.f32.negative.max, kValue.f32.negative.max] },
      { input: kValue.f32.negative.min, expected: [kValue.f32.positive.max, kValue.f32.positive.max] },
      { input: kValue.f32.negative.max, expected: [kValue.f32.positive.min, kValue.f32.positive.min] },

      // 32-bit subnormals
      { input: kValue.f32.subnormal.positive.max, expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: kValue.f32.subnormal.positive.min, expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: kValue.f32.subnormal.negative.min, expected: [0, kValue.f32.subnormal.positive.max] },
      { input: kValue.f32.subnormal.negative.max, expected: [0, kValue.f32.subnormal.positive.min] },
    ]
  )
  .fn(t => {
    const input = t.params.input;
    const expected =
      t.params.expected instanceof Array ? arrayToInterval(t.params.expected) : t.params.expected;

    const got = negationInterval(input);
    t.expect(
      objectEquals(expected, got),
      `negationInterval(${input}) returned ${got}. Expected ${expected}`
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
      { input: Number.NEGATIVE_INFINITY, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: kValue.f32.negative.min, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: kValue.f32.negative.pi.half, expected: [-1, plusOneULP(-1)] },
      { input: 0, expected: [0, 0] },
      { input: kValue.f32.positive.pi.half, expected: [minusOneULP(1), 1] },
      { input: kValue.f32.positive.max, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: Number.POSITIVE_INFINITY, expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
    ]
  )
  .fn(t => {
    const error = 2 ** -11;

    const input = t.params.input;
    let expected: F32Interval;
    if (t.params.expected instanceof Array) {
      const [begin, end] = t.params.expected;
      expected = arrayToInterval([begin - error, end + error]);
    } else {
      expected = t.params.expected;
    }

    const got = sinInterval(input);
    t.expect(
      objectEquals(expected, got),
      `sinInterval(${input}) returned ${got}. Expected ${expected}`
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
      { input: Number.NEGATIVE_INFINITY, expected: arrayToInterval([Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]) },
      { input: kValue.f32.negative.min, expected: arrayToInterval([Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]) },
      { input: kValue.f32.negative.pi.whole, expected: arrayToInterval([hexToF64(0xbf4002bc, 0x90000000), hexToF64(0x3f400144, 0xf0000000)]) },  // ~0.0
      { input: kValue.f32.negative.pi.half, expected: arrayToInterval([Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]) },
      { input: 0, expected: arrayToInterval([hexToF64(0xbf400200, 0xb0000000), hexToF64(0x3f400200, 0xb0000000)]) },  // ~0.0
      { input: kValue.f32.positive.pi.half, expected: arrayToInterval([Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]) },
      { input: kValue.f32.positive.pi.whole, expected: arrayToInterval([hexToF64(0xbf400144, 0xf0000000), hexToF64(0x3f4002bc, 0x90000000)]) },  // ~0.0
      { input: kValue.f32.positive.max, expected: arrayToInterval([Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]) },
      { input: Number.POSITIVE_INFINITY, expected: arrayToInterval([Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]) },
    ]
  )
  .fn(t => {
    const input = t.params.input;
    assert(
      !(t.params.expected instanceof Array),
      'Expectations for tanInterval must be expressed precisely as F32Intervals'
    );
    const expected: F32Interval = t.params.expected;
    const got = tanInterval(input);
    t.expect(
      objectEquals(expected, got),
      `tanInterval(${input}) returned ${got}. Expected ${expected}`
    );
  });

interface BinaryToIntervalCase {
  // input is a pair of independent values, not an range, so should not be
  // converted to a F32Interval.
  input: [number, number];

  // If expected is an Array of two values, the test should interpret them as
  // human readable begin/end values that need to be adjusted for the error
  // calculation specific to the function being tested.
  // This is to facilitate writing tests in a fluent manner, i.e. being able to
  // express a expectation as `plusOneULP(π/2)` instead of
  // `π/2 + ULP(π/2) + 4096 * ULP(π/2 + ULP(π/2))`
  //
  // If expected is an F32Interval, it is to be interpreted as an exact value,
  // with no additional processing. This is to allow for expressing specific
  // cases with human readable values that would require effectively
  // implementing the entire interval system under test to accommodate them in
  // the test error calculation.
  expected: [number, number] | F32Interval;
}

g.test('additionInterval')
  .paramsSubcasesOnly<BinaryToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: [0, 0] },
      { input: [1, 0], expected: [1, 1] },
      { input: [0, 1], expected: [1, 1] },
      { input: [-1, 0], expected: [-1, -1] },
      { input: [0, -1], expected: [-1, -1] },
      { input: [1, 1], expected: [2, 2] },
      { input: [1, -1], expected: [0, 0] },
      { input: [-1, 1], expected: [0, 0] },
      { input: [-1, -1], expected: [-2, -2] },

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
      { input: [0, Number.POSITIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [Number.POSITIVE_INFINITY, 0], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [0, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [Number.NEGATIVE_INFINITY, 0], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [Number.NEGATIVE_INFINITY, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;

    const expected =
      t.params.expected instanceof Array ? arrayToInterval(t.params.expected) : t.params.expected;

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
      { input: [Number.POSITIVE_INFINITY, 1], expected: [minusOneULP(kValue.f32.positive.pi.half), kValue.f32.positive.pi.half] },

      // positive y, negative x
      { input: [1, -1], expected: [minusOneULP(kValue.f32.positive.pi.three_quarters), kValue.f32.positive.pi.three_quarters] },
      { input: [Number.POSITIVE_INFINITY, -1], expected: [minusOneULP(kValue.f32.positive.pi.half), kValue.f32.positive.pi.half] },

      // negative y, negative x
      { input: [-1, -1], expected: [kValue.f32.negative.pi.three_quarters, plusOneULP(kValue.f32.negative.pi.three_quarters)] },
      { input: [Number.NEGATIVE_INFINITY, -1], expected: [kValue.f32.negative.pi.half, plusOneULP(kValue.f32.negative.pi.half)] },

      // negative y, positive x
      { input: [-1, 1], expected: [kValue.f32.negative.pi.quarter, plusOneULP(kValue.f32.negative.pi.quarter)] },
      { input: [Number.NEGATIVE_INFINITY, 1], expected: [kValue.f32.negative.pi.half, plusOneULP(kValue.f32.negative.pi.half)] },

      // Discontinuity @ y = 0
      { input: [0, 0], expected: arrayToInterval([Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]) },
      { input: [0, kValue.f32.subnormal.positive.max], expected: arrayToInterval([Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]) },
      { input: [0, kValue.f32.subnormal.negative.min], expected: arrayToInterval([Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY]) },
      { input: [0, kValue.f32.positive.min], expected: [kValue.f32.negative.pi.whole, kValue.f32.positive.pi.whole] },
      { input: [0, kValue.f32.negative.max], expected: [kValue.f32.negative.pi.whole, kValue.f32.positive.pi.whole] },
      { input: [0, kValue.f32.positive.max], expected: [kValue.f32.negative.pi.whole, kValue.f32.positive.pi.whole] },
      { input: [0, kValue.f32.negative.min], expected: [kValue.f32.negative.pi.whole, kValue.f32.positive.pi.whole] },
      { input: [0, Number.POSITIVE_INFINITY], expected: [kValue.f32.negative.pi.whole, kValue.f32.positive.pi.whole] },
      { input: [0, Number.NEGATIVE_INFINITY], expected: [kValue.f32.negative.pi.whole, kValue.f32.positive.pi.whole] },
    ]
  )
  .fn(t => {
    const error = (x: number): number => {
      return 4096 * oneULP(x);
    };

    const [y, x] = t.params.input;
    let expected: F32Interval;
    if (t.params.expected instanceof Array) {
      const [begin, end] = t.params.expected;
      expected = arrayToInterval([begin - error(begin), end + error(end)]);
    } else {
      expected = t.params.expected;
    }

    const got = atan2Interval(y, x);
    t.expect(
      objectEquals(expected, got),
      `atan2Interval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('divisionInterval')
  .paramsSubcasesOnly<BinaryToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 1], expected: [0, 0] },
      { input: [0, -1], expected: [0, 0] },
      { input: [1, 1], expected: [1, 1] },
      { input: [1, -1], expected: [-1, -1] },
      { input: [-1, 1], expected: [-1, -1] },
      { input: [-1, -1], expected: [1, 1] },
      { input: [4, 2], expected: [2, 2] },
      { input: [-4, 2], expected: [-2, -2] },
      { input: [4, -2], expected: [-2, -2] },
      { input: [-4, -2], expected: [2, 2] },

      // 64-bit normals
      { input: [0, 0.1], expected: [0, 0] },
      { input: [0, -0.1], expected: [0, 0] },
      { input: [1, 0.1], expected: [minusOneULP(10), plusOneULP(10)] },
      { input: [-1, 0.1], expected: [minusOneULP(-10), plusOneULP(-10)] },
      { input: [1, -0.1], expected: [minusOneULP(-10), plusOneULP(-10)] },
      { input: [-1, -0.1], expected: [minusOneULP(10), plusOneULP(10)] },

      // Denominator out of range
      { input: [1, Number.POSITIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: [1, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: [Number.NEGATIVE_INFINITY, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: [1, kValue.f32.positive.max], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: [1, kValue.f32.negative.min], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: [1, 0], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: [1, kValue.f32.subnormal.positive.max], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
    ]
  )
  .fn(t => {
    const error = (x: number): number => {
      return 2.5 * oneULP(x);
    };

    const [x, y] = t.params.input;
    let expected: F32Interval;
    if (t.params.expected instanceof Array) {
      const [begin, end] = t.params.expected;
      expected = arrayToInterval([begin - error(begin), end + error(end)]);
    } else {
      expected = t.params.expected;
    }

    const got = divisionInterval(x, y);
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
      { input: [0, 0], expected: [0, 0] },
      { input: [1, 0], expected: [1, 1] },
      { input: [0, 1], expected: [1, 1] },
      { input: [-1, 0], expected: [0, 0] },
      { input: [0, -1], expected: [0, 0] },
      { input: [1, 1], expected: [1, 1] },
      { input: [1, -1], expected: [1, 1] },
      { input: [-1, 1], expected: [1, 1] },
      { input: [-1, -1], expected: [-1, -1] },

      // 64-bit normals
      { input: [0.1, 0], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0, 0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [-0.1, 0], expected: [0, 0] },
      { input: [0, -0.1], expected: [0, 0] },
      { input: [0.1, 0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0.1, -0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [-0.1, 0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [-0.1, -0.1], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1

      // 32-bit normals
      { input: [kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.min, 0], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [0, kValue.f32.subnormal.positive.min], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [kValue.f32.subnormal.negative.max, 0], expected: [0, 0] },
      { input: [0, kValue.f32.subnormal.negative.max], expected: [0, 0] },
      { input: [kValue.f32.subnormal.negative.min, 0], expected: [0, 0] },
      { input: [0, kValue.f32.subnormal.negative.min], expected: [0, 0] },

      // Infinities
      { input: [0, Number.POSITIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [Number.POSITIVE_INFINITY, 0], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [0, Number.NEGATIVE_INFINITY], expected: [0, 0] },
      { input: [Number.NEGATIVE_INFINITY, 0], expected: [0, 0] },
      { input: [Number.NEGATIVE_INFINITY, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;

    const expected =
      t.params.expected instanceof Array ? arrayToInterval(t.params.expected) : t.params.expected;

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
      { input: [0, 0], expected: [0, 0] },
      { input: [1, 0], expected: [0, 0] },
      { input: [0, 1], expected: [0, 0] },
      { input: [-1, 0], expected: [-1, -1] },
      { input: [0, -1], expected: [-1, -1] },
      { input: [1, 1], expected: [1, 1] },
      { input: [1, -1], expected: [-1, -1] },
      { input: [-1, 1], expected: [-1, -1] },
      { input: [-1, -1], expected: [-1, -1] },

      // 64-bit normals
      { input: [0.1, 0], expected: [0, 0] },
      { input: [0, 0.1], expected: [0, 0] },
      { input: [-0.1, 0], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [0, -0.1], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [0.1, 0.1], expected: [minusOneULP(hexToF32(0x3dcccccd)), hexToF32(0x3dcccccd)] },  // ~0.1
      { input: [0.1, -0.1], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [-0.1, 0.1], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1
      { input: [-0.1, -0.1], expected: [hexToF32(0xbdcccccd), plusOneULP(hexToF32(0xbdcccccd))] },  // ~-0.1

      // 32-bit normals
      { input: [kValue.f32.subnormal.positive.max, 0], expected: [0, 0] },
      { input: [0, kValue.f32.subnormal.positive.max], expected: [0, 0] },
      { input: [kValue.f32.subnormal.positive.min, 0], expected: [0, 0] },
      { input: [0, kValue.f32.subnormal.positive.min], expected: [0, 0] },
      { input: [kValue.f32.subnormal.negative.max, 0], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [0, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [kValue.f32.subnormal.negative.min, 0], expected: [kValue.f32.subnormal.negative.min, 0] },
      { input: [0, kValue.f32.subnormal.negative.min], expected: [kValue.f32.subnormal.negative.min, 0] },

      // Infinities
      { input: [0, Number.POSITIVE_INFINITY], expected: [0, 0] },
      { input: [Number.POSITIVE_INFINITY, 0], expected: [0, 0] },
      { input: [Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [0, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [Number.NEGATIVE_INFINITY, 0], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [Number.NEGATIVE_INFINITY, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;

    const expected =
      t.params.expected instanceof Array ? arrayToInterval(t.params.expected) : t.params.expected;

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
      { input: [0, 0], expected: [0, 0] },
      { input: [1, 0], expected: [0, 0] },
      { input: [0, 1], expected: [0, 0] },
      { input: [-1, 0], expected: [0, 0] },
      { input: [0, -1], expected: [0, 0] },
      { input: [1, 1], expected: [1, 1] },
      { input: [1, -1], expected: [-1, -1] },
      { input: [-1, 1], expected: [-1, -1] },
      { input: [-1, -1], expected: [1, 1] },
      { input: [2, 1], expected: [2, 2] },
      { input: [1, -2], expected: [-2, -2] },
      { input: [-2, 1], expected: [-2, -2] },
      { input: [-2, -1], expected: [2, 2] },
      { input: [2, 2], expected: [4, 4] },
      { input: [2, -2], expected: [-4, -4] },
      { input: [-2, 2], expected: [-4, -4] },
      { input: [-2, -2], expected: [4, 4] },

      // 64-bit normals
      { input: [0.1, 0], expected: [0, 0] },
      { input: [0, 0.1], expected: [0, 0] },
      { input: [-0.1, 0], expected: [0, 0] },
      { input: [0, -0.1], expected: [0, 0] },
      { input: [0.1, 0.1], expected: [minusNULP(hexToF32(0x3c23d70a), 2), plusOneULP(hexToF32(0x3c23d70a))] },  // ~0.01
      { input: [0.1, -0.1], expected: [minusOneULP(hexToF32(0xbc23d70a)), plusNULP(hexToF32(0xbc23d70a), 2)] },  // ~-0.01
      { input: [-0.1, 0.1], expected: [minusOneULP(hexToF32(0xbc23d70a)), plusNULP(hexToF32(0xbc23d70a), 2)] },  // ~-0.01
      { input: [-0.1, -0.1], expected: [minusNULP(hexToF32(0x3c23d70a), 2), plusOneULP(hexToF32(0x3c23d70a))] },  // ~0.01

      // Infinities
      { input: [0, Number.POSITIVE_INFINITY], expected: [0, 0] },
      { input: [1, Number.POSITIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [-1, Number.POSITIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [0, Number.NEGATIVE_INFINITY], expected: [0, 0] },
      { input: [1, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [-1, Number.NEGATIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [Number.NEGATIVE_INFINITY, Number.NEGATIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },

      // Edge of f32
      { input: [kValue.f32.positive.max, kValue.f32.positive.max], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [kValue.f32.negative.min, kValue.f32.negative.min], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [kValue.f32.positive.max, kValue.f32.negative.min], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [kValue.f32.negative.min, kValue.f32.positive.max], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;

    const expected =
      t.params.expected instanceof Array ? arrayToInterval(t.params.expected) : t.params.expected;

    const got = multiplicationInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `multiplicationInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

g.test('subtractionInterval')
  .paramsSubcasesOnly<BinaryToIntervalCase>(
    // prettier-ignore
    [
      // 32-bit normals
      { input: [0, 0], expected: [0, 0] },
      { input: [1, 0], expected: [1, 1] },
      { input: [0, 1], expected: [-1, -1] },
      { input: [-1, 0], expected: [-1, -1] },
      { input: [0, -1], expected: [1, 1] },
      { input: [1, 1], expected: [0, 0] },
      { input: [1, -1], expected: [2, 2] },
      { input: [-1, 1], expected: [-2, -2] },
      { input: [-1, -1], expected: [0, 0] },

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
      { input: [0, Number.POSITIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [Number.POSITIVE_INFINITY, 0], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: [0, Number.NEGATIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [Number.NEGATIVE_INFINITY, 0], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [Number.NEGATIVE_INFINITY, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY] },
      { input: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { input: [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
    ]
  )
  .fn(t => {
    const [x, y] = t.params.input;

    const expected =
      t.params.expected instanceof Array ? arrayToInterval(t.params.expected) : t.params.expected;

    const got = subtractionInterval(x, y);
    t.expect(
      objectEquals(expected, got),
      `subtractionInterval(${x}, ${y}) returned ${got}. Expected ${expected}`
    );
  });

interface TernaryToIntervalCase {
  input: [number, number, number];
  expected: [number, number];
}

g.test('clampMedianInterval')
  .paramsSubcasesOnly<TernaryToIntervalCase>(
    // prettier-ignore
    [
      // Normals
      { input: [0, 0, 0], expected: [0, 0] },
      { input: [1, 0, 0], expected: [0, 0] },
      { input: [0, 1, 0], expected: [0, 0] },
      { input: [0, 0, 1], expected: [0, 0] },
      { input: [1, 0, 1], expected: [1, 1] },
      { input: [1, 1, 0], expected: [1, 1] },
      { input: [0, 1, 1], expected: [1, 1] },
      { input: [1, 1, 1], expected: [1, 1] },
      { input: [1, 10, 100], expected: [10, 10] },
      { input: [10, 1, 100], expected: [10, 10] },
      { input: [100, 1, 10], expected: [10, 10] },
      { input: [-10, 1, 100], expected: [1, 1] },
      { input: [10, 1, -100], expected: [1, 1] },
      { input: [-10, 1, -100], expected: [-10, -10] },
      { input: [-10, -10, -10], expected: [-10, -10] },

      // Subnormals
      { input: [kValue.f32.subnormal.positive.max, 0, 0], expected: [0, 0] },
      { input: [0, kValue.f32.subnormal.positive.max, 0], expected: [0, 0] },
      { input: [0, 0, kValue.f32.subnormal.positive.max], expected: [0, 0] },
      { input: [kValue.f32.subnormal.positive.max, 0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, 0], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [0, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.negative.max], expected: [0, kValue.f32.subnormal.positive.min] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [kValue.f32.positive.max, kValue.f32.positive.max, kValue.f32.subnormal.positive.min], expected: [kValue.f32.positive.max, kValue.f32.positive.max] },

      // Infinities
      { input: [0, 1, Number.POSITIVE_INFINITY], expected: [1, 1] },
      { input: [0, Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;

    const expected = arrayToInterval(t.params.expected);

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
      { input: [0, 0, 0], expected: [0, 0] },
      { input: [1, 0, 0], expected: [0, 0] },
      { input: [0, 1, 0], expected: [0, 0] },
      { input: [0, 0, 1], expected: [0, 0] },
      { input: [1, 0, 1], expected: [1, 1] },
      { input: [1, 1, 0], expected: [0, 0] },
      { input: [0, 1, 1], expected: [1, 1] },
      { input: [1, 1, 1], expected: [1, 1] },
      { input: [1, 10, 100], expected: [10, 10] },
      { input: [10, 1, 100], expected: [10, 10] },
      { input: [100, 1, 10], expected: [10, 10] },
      { input: [-10, 1, 100], expected: [1, 1] },
      { input: [10, 1, -100], expected: [-100, -100] },
      { input: [-10, 1, -100], expected: [-100, -100] },
      { input: [-10, -10, -10], expected: [-10, -10] },

      // Subnormals
      { input: [kValue.f32.subnormal.positive.max, 0, 0], expected: [0, 0] },
      { input: [0, kValue.f32.subnormal.positive.max, 0], expected: [0, 0] },
      { input: [0, 0, kValue.f32.subnormal.positive.max], expected: [0, 0] },
      { input: [kValue.f32.subnormal.positive.max, 0, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, 0], expected: [0, 0] },
      { input: [0, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.max], expected: [0, kValue.f32.subnormal.positive.max] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [kValue.f32.subnormal.positive.max, kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max], expected: [kValue.f32.subnormal.negative.max, 0] },
      { input: [kValue.f32.positive.max, kValue.f32.positive.max, kValue.f32.subnormal.positive.min], expected: [0, kValue.f32.subnormal.positive.min] },

      // Infinities
      { input: [0, 1, Number.POSITIVE_INFINITY], expected: [1, 1] },
      { input: [0, Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY], expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { input: [Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY], expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
    ]
  )
  .fn(t => {
    const [x, y, z] = t.params.input;

    const expected = arrayToInterval(t.params.expected);

    const got = clampMinMaxInterval(x, y, z);
    t.expect(
      objectEquals(expected, got),
      `clampMinMaxInterval(${x}, ${y}, ${z}) returned ${got}. Expected ${expected}`
    );
  });
