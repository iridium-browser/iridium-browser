export const description = `
Util math unit tests.
`;

import { makeTestGroup } from '../common/framework/test_group.js';
import { objectEquals } from '../common/util/util.js';
import { kBit, kValue } from '../webgpu/util/constants.js';
import {
  f16,
  f32,
  f64,
  float16ToUint16,
  float32ToUint32,
  uint16ToFloat16,
  uint32ToFloat32,
} from '../webgpu/util/conversion.js';
import {
  biasedRange,
  calculatePermutations,
  cartesianProduct,
  correctlyRoundedF16,
  correctlyRoundedF32,
  FlushMode,
  frexp,
  fullF16Range,
  fullF32Range,
  fullI32Range,
  hexToF16,
  hexToF32,
  hexToF64,
  lerp,
  linearRange,
  nextAfterF16,
  nextAfterF32,
  NextDirection,
  oneULPF32,
} from '../webgpu/util/math.js';

import { UnitTest } from './unit_test.js';

export const g = makeTestGroup(UnitTest);

/**
 * Utility wrapper around oneULP to test if a value is within 1 ULP(x)
 *
 * @param got number to test
 * @param expected number to be within 1 ULP of
 * @param mode should oneULP FTZ
 * @returns if got is within 1 ULP of expected
 */
function withinOneULPF32(got: number, expected: number, mode: FlushMode): boolean {
  const ulp = oneULPF32(expected, mode);
  return got >= expected - ulp && got <= expected + ulp;
}

/**
 * @returns true if arrays are equal within 1ULP, doing element-wise comparison
 *               as needed, and considering NaNs to be equal.
 *
 * Depends on the correctness of oneULP, which is tested in this file.
 **
 * @param got array of numbers to compare for equality
 * @param expect array of numbers to compare against
 * @param mode should different subnormals be considered the same, i.e. should
 *              FTZ occur during comparison
 **/
function compareArrayOfNumbersF32(
  got: Array<number>,
  expect: Array<number>,
  mode: FlushMode = 'flush'
): boolean {
  return (
    got.length === expect.length &&
    got.every((value, index) => {
      const expected = expect[index];
      return (
        (Number.isNaN(value) && Number.isNaN(expected)) || withinOneULPF32(value, expected, mode)
      );
    })
  );
}

interface nextAfterCase {
  val: number;
  dir: NextDirection;
  result: number;
}

g.test('nextAfterF32FlushToZero')
  .paramsSubcasesOnly<nextAfterCase>(
    // prettier-ignore
    [
    // Edge Cases
    { val: Number.NaN, dir: 'positive', result: Number.NaN },
    { val: Number.NaN, dir: 'negative', result: Number.NaN },
    { val: Number.POSITIVE_INFINITY, dir: 'positive', result: kValue.f32.infinity.positive },
    { val: Number.POSITIVE_INFINITY, dir: 'negative', result: kValue.f32.infinity.positive },
    { val: Number.NEGATIVE_INFINITY, dir: 'positive', result: kValue.f32.infinity.negative },
    { val: Number.NEGATIVE_INFINITY, dir: 'negative', result: kValue.f32.infinity.negative },

    // Zeroes
    { val: +0, dir: 'positive', result: kValue.f32.positive.min },
    { val: +0, dir: 'negative', result: kValue.f32.negative.max },
    { val: -0, dir: 'positive', result: kValue.f32.positive.min },
    { val: -0, dir: 'negative', result: kValue.f32.negative.max },

    // Subnormals
    { val: kValue.f32.subnormal.positive.min, dir: 'positive', result: kValue.f32.positive.min },
    { val: kValue.f32.subnormal.positive.min, dir: 'negative', result: kValue.f32.negative.max },
    { val: kValue.f32.subnormal.positive.max, dir: 'positive', result: kValue.f32.positive.min },
    { val: kValue.f32.subnormal.positive.max, dir: 'negative', result: kValue.f32.negative.max },
    { val: kValue.f32.subnormal.negative.min, dir: 'positive', result: kValue.f32.positive.min },
    { val: kValue.f32.subnormal.negative.min, dir: 'negative', result: kValue.f32.negative.max },
    { val: kValue.f32.subnormal.negative.max, dir: 'positive', result: kValue.f32.positive.min },
    { val: kValue.f32.subnormal.negative.max, dir: 'negative', result: kValue.f32.negative.max },

    // Normals
    { val: kValue.f32.positive.max, dir: 'positive', result: kValue.f32.infinity.positive },
    { val: kValue.f32.positive.max, dir: 'negative', result: hexToF32(0x7f7ffffe) },
    { val: kValue.f32.positive.min, dir: 'positive', result: hexToF32(0x00800001) },
    { val: kValue.f32.positive.min, dir: 'negative', result: 0 },
    { val: kValue.f32.negative.max, dir: 'positive', result: 0 },
    { val: kValue.f32.negative.max, dir: 'negative', result: hexToF32(0x80800001) },
    { val: kValue.f32.negative.min, dir: 'positive', result: hexToF32(0xff7ffffe) },
    { val: kValue.f32.negative.min, dir: 'negative', result: kValue.f32.infinity.negative },
    { val: hexToF32(0x03800000), dir: 'positive', result: hexToF32(0x03800001) },
    { val: hexToF32(0x03800000), dir: 'negative', result: hexToF32(0x037fffff) },
    { val: hexToF32(0x83800000), dir: 'positive', result: hexToF32(0x837fffff) },
    { val: hexToF32(0x83800000), dir: 'negative', result: hexToF32(0x83800001) },

    // Not precisely expressible as f32
    { val: 0.001, dir: 'positive', result: hexToF32(0x3a83126f) }, // positive normal
    { val: 0.001, dir: 'negative', result: hexToF32(0x3a83126e) }, // positive normal
    { val: -0.001, dir: 'positive', result: hexToF32(0xba83126e) }, // negative normal
    { val: -0.001, dir: 'negative', result: hexToF32(0xba83126f) }, // negative normal
    { val: 2.82E-40, dir: 'positive', result: kValue.f32.positive.min }, // positive subnormal
    { val: 2.82E-40, dir: 'negative', result: kValue.f32.negative.max }, // positive subnormal
    { val: -2.82E-40, dir: 'positive', result: kValue.f32.positive.min }, // negative subnormal
    { val: -2.82E-40, dir: 'negative', result: kValue.f32.negative.max }, // negative subnormal
    ]
  )
  .fn(t => {
    const val = t.params.val;
    const dir = t.params.dir;
    const expect = t.params.result;
    const got = nextAfterF32(val, dir, 'flush');
    t.expect(
      got === expect || (Number.isNaN(got) && Number.isNaN(expect)),
      `nextAfterF32(${f64(val)}, '${dir}', 'flush') returned ${f32(got)}. Expected ${f32(expect)}`
    );
  });

g.test('nextAfterF32NoFlush')
  .paramsSubcasesOnly<nextAfterCase>(
    // prettier-ignore
    [
    // Edge Cases
    { val: Number.NaN, dir: 'positive', result: Number.NaN },
    { val: Number.NaN, dir: 'negative', result: Number.NaN },
    { val: Number.POSITIVE_INFINITY, dir: 'positive', result: kValue.f32.infinity.positive },
    { val: Number.POSITIVE_INFINITY, dir: 'negative', result: kValue.f32.infinity.positive },
    { val: Number.NEGATIVE_INFINITY, dir: 'positive', result: kValue.f32.infinity.negative },
    { val: Number.NEGATIVE_INFINITY, dir: 'negative', result: kValue.f32.infinity.negative },

    // Zeroes
    { val: +0, dir: 'positive', result: kValue.f32.subnormal.positive.min },
    { val: +0, dir: 'negative', result: kValue.f32.subnormal.negative.max },
    { val: -0, dir: 'positive', result: kValue.f32.subnormal.positive.min },
    { val: -0, dir: 'negative', result: kValue.f32.subnormal.negative.max },

    // Subnormals
    { val: hexToF32(kBit.f32.subnormal.positive.min), dir: 'positive', result: hexToF32(0x00000002) },
    { val: hexToF32(kBit.f32.subnormal.positive.min), dir: 'negative', result: 0 },
    { val: hexToF32(kBit.f32.subnormal.positive.max), dir: 'positive', result: kValue.f32.positive.min },
    { val: hexToF32(kBit.f32.subnormal.positive.max), dir: 'negative', result: hexToF32(0x007ffffe) },
    { val: hexToF32(kBit.f32.subnormal.negative.min), dir: 'positive', result: hexToF32(0x807ffffe) },
    { val: hexToF32(kBit.f32.subnormal.negative.min), dir: 'negative', result: kValue.f32.negative.max },
    { val: hexToF32(kBit.f32.subnormal.negative.max), dir: 'positive', result: 0 },
    { val: hexToF32(kBit.f32.subnormal.negative.max), dir: 'negative', result: hexToF32(0x80000002) },

    // Normals
    { val: hexToF32(kBit.f32.positive.max), dir: 'positive', result: kValue.f32.infinity.positive },
    { val: hexToF32(kBit.f32.positive.max), dir: 'negative', result: hexToF32(0x7f7ffffe) },
    { val: hexToF32(kBit.f32.positive.min), dir: 'positive', result: hexToF32(0x00800001) },
    { val: hexToF32(kBit.f32.positive.min), dir: 'negative', result: kValue.f32.subnormal.positive.max },
    { val: hexToF32(kBit.f32.negative.max), dir: 'positive', result: kValue.f32.subnormal.negative.min },
    { val: hexToF32(kBit.f32.negative.max), dir: 'negative', result: hexToF32(0x80800001) },
    { val: hexToF32(kBit.f32.negative.min), dir: 'positive', result: hexToF32(0xff7ffffe) },
    { val: hexToF32(kBit.f32.negative.min), dir: 'negative', result: kValue.f32.infinity.negative },
    { val: hexToF32(0x03800000), dir: 'positive', result: hexToF32(0x03800001) },
    { val: hexToF32(0x03800000), dir: 'negative', result: hexToF32(0x037fffff) },
    { val: hexToF32(0x83800000), dir: 'positive', result: hexToF32(0x837fffff) },
    { val: hexToF32(0x83800000), dir: 'negative', result: hexToF32(0x83800001) },

    // Not precisely expressible as f32
    { val: 0.001, dir: 'positive', result: hexToF32(0x3a83126f) }, // positive normal
    { val: 0.001, dir: 'negative', result: hexToF32(0x3a83126e) }, // positive normal
    { val: -0.001, dir: 'positive', result: hexToF32(0xba83126e) }, // negative normal
    { val: -0.001, dir: 'negative', result: hexToF32(0xba83126f) }, // negative normal
    { val: 2.82E-40, dir: 'positive', result: hexToF32(0x0003121a) }, // positive subnormal
    { val: 2.82E-40, dir: 'negative', result: hexToF32(0x00031219) }, // positive subnormal
    { val: -2.82E-40, dir: 'positive', result: hexToF32(0x80031219) }, // negative subnormal
    { val: -2.82E-40, dir: 'negative', result: hexToF32(0x8003121a) }, // negative subnormal
  ]
  )
  .fn(t => {
    const val = t.params.val;
    const dir = t.params.dir;
    const expect = t.params.result;
    const got = nextAfterF32(val, dir, 'no-flush');
    t.expect(
      got === expect || (Number.isNaN(got) && Number.isNaN(expect)),
      `nextAfterF32(${f64(val)}, '${dir}', 'no-flush') returned ${f32(got)}. Expected ${f32(
        expect
      )}`
    );
  });

g.test('nextAfterF16FlushToZero')
  .paramsSubcasesOnly<nextAfterCase>(
    // prettier-ignore
    [
      // Edge Cases
      { val: Number.NaN, dir: 'positive', result: Number.NaN },
      { val: Number.NaN, dir: 'negative', result: Number.NaN },
      { val: Number.POSITIVE_INFINITY, dir: 'positive', result: kValue.f16.infinity.positive },
      { val: Number.POSITIVE_INFINITY, dir: 'negative', result: kValue.f16.infinity.positive },
      { val: Number.NEGATIVE_INFINITY, dir: 'positive', result: kValue.f16.infinity.negative },
      { val: Number.NEGATIVE_INFINITY, dir: 'negative', result: kValue.f16.infinity.negative },

      // Zeroes
      { val: +0, dir: 'positive', result: kValue.f16.positive.min },
      { val: +0, dir: 'negative', result: kValue.f16.negative.max },
      { val: -0, dir: 'positive', result: kValue.f16.positive.min },
      { val: -0, dir: 'negative', result: kValue.f16.negative.max },

      // Subnormals
      { val: kValue.f16.subnormal.positive.min, dir: 'positive', result: kValue.f16.positive.min },
      { val: kValue.f16.subnormal.positive.min, dir: 'negative', result: kValue.f16.negative.max },
      { val: kValue.f16.subnormal.positive.max, dir: 'positive', result: kValue.f16.positive.min },
      { val: kValue.f16.subnormal.positive.max, dir: 'negative', result: kValue.f16.negative.max },
      { val: kValue.f16.subnormal.negative.min, dir: 'positive', result: kValue.f16.positive.min },
      { val: kValue.f16.subnormal.negative.min, dir: 'negative', result: kValue.f16.negative.max },
      { val: kValue.f16.subnormal.negative.max, dir: 'positive', result: kValue.f16.positive.min },
      { val: kValue.f16.subnormal.negative.max, dir: 'negative', result: kValue.f16.negative.max },

      // Normals
      { val: kValue.f16.positive.max, dir: 'positive', result: kValue.f16.infinity.positive },
      { val: kValue.f16.positive.max, dir: 'negative', result: hexToF16(0x7bfe) },
      { val: kValue.f16.positive.min, dir: 'positive', result: hexToF16(0x0401) },
      { val: kValue.f16.positive.min, dir: 'negative', result: 0 },
      { val: kValue.f16.negative.max, dir: 'positive', result: 0 },
      { val: kValue.f16.negative.max, dir: 'negative', result: hexToF16(0x8401) },
      { val: kValue.f16.negative.min, dir: 'positive', result: hexToF16(0xfbfe) },
      { val: kValue.f16.negative.min, dir: 'negative', result: kValue.f16.infinity.negative },
      { val: hexToF16(0x1380), dir: 'positive', result: hexToF16(0x1381) },
      { val: hexToF16(0x1380), dir: 'negative', result: hexToF16(0x137f) },
      { val: hexToF16(0x9380), dir: 'positive', result: hexToF16(0x937f) },
      { val: hexToF16(0x9380), dir: 'negative', result: hexToF16(0x9381) },

      // Not precisely expressible as f16
      { val: 0.01, dir: 'positive', result: hexToF16(0x211f) }, // positive normal
      { val: 0.01, dir: 'negative', result: hexToF16(0x211e) }, // positive normal
      { val: -0.01, dir: 'positive', result: hexToF16(0xa11e) }, // negative normal
      { val: -0.01, dir: 'negative', result: hexToF16(0xa11f) }, // negative normal
      { val: 2.82E-40, dir: 'positive', result: kValue.f16.positive.min }, // positive subnormal
      { val: 2.82E-40, dir: 'negative', result: kValue.f16.negative.max }, // positive subnormal
      { val: -2.82E-40, dir: 'positive', result: kValue.f16.positive.min }, // negative subnormal
      { val: -2.82E-40, dir: 'negative', result: kValue.f16.negative.max }, // negative subnormal
    ]
  )
  .fn(t => {
    const val = t.params.val;
    const dir = t.params.dir;
    const expect = t.params.result;
    const got = nextAfterF16(val, dir, 'flush');
    t.expect(
      got === expect || (Number.isNaN(got) && Number.isNaN(expect)),
      `nextAfterF16(${f64(val)}, '${dir}', 'flush') returned ${f16(got)}. Expected ${f16(expect)}`
    );
  });

g.test('nextAfterF16NoFlush')
  .paramsSubcasesOnly<nextAfterCase>(
    // prettier-ignore
    [
      // Edge Cases
      { val: Number.NaN, dir: 'positive', result: Number.NaN },
      { val: Number.NaN, dir: 'negative', result: Number.NaN },
      { val: Number.POSITIVE_INFINITY, dir: 'positive', result: kValue.f16.infinity.positive },
      { val: Number.POSITIVE_INFINITY, dir: 'negative', result: kValue.f16.infinity.positive },
      { val: Number.NEGATIVE_INFINITY, dir: 'positive', result: kValue.f16.infinity.negative },
      { val: Number.NEGATIVE_INFINITY, dir: 'negative', result: kValue.f16.infinity.negative },

      // Zeroes
      { val: +0, dir: 'positive', result: kValue.f16.subnormal.positive.min },
      { val: +0, dir: 'negative', result: kValue.f16.subnormal.negative.max },
      { val: -0, dir: 'positive', result: kValue.f16.subnormal.positive.min },
      { val: -0, dir: 'negative', result: kValue.f16.subnormal.negative.max },

      // Subnormals
      { val: kValue.f16.subnormal.positive.min, dir: 'positive', result: hexToF16(0x0002) },
      { val: kValue.f16.subnormal.positive.min, dir: 'negative', result: 0 },
      { val: kValue.f16.subnormal.positive.max, dir: 'positive', result: kValue.f16.positive.min },
      { val: kValue.f16.subnormal.positive.max, dir: 'negative', result: hexToF16(0x03fe) },
      { val: kValue.f16.subnormal.negative.min, dir: 'positive', result: hexToF16(0x83fe) },
      { val: kValue.f16.subnormal.negative.min, dir: 'negative', result: kValue.f16.negative.max },
      { val: kValue.f16.subnormal.negative.max, dir: 'positive', result: 0 },
      { val: kValue.f16.subnormal.negative.max, dir: 'negative', result: hexToF16(0x8002) },

      // Normals
      { val: kValue.f16.positive.max, dir: 'positive', result: kValue.f16.infinity.positive },
      { val: kValue.f16.positive.max, dir: 'negative', result: hexToF16(0x7bfe) },
      { val: kValue.f16.positive.min, dir: 'positive', result: hexToF16(0x0401) },
      { val: kValue.f16.positive.min, dir: 'negative', result: kValue.f16.subnormal.positive.max },
      { val: kValue.f16.negative.max, dir: 'positive', result: kValue.f16.subnormal.negative.min },
      { val: kValue.f16.negative.max, dir: 'negative', result: hexToF16(0x8401) },
      { val: kValue.f16.negative.min, dir: 'positive', result: hexToF16(0xfbfe) },
      { val: kValue.f16.negative.min, dir: 'negative', result: kValue.f16.infinity.negative },
      { val: hexToF16(0x1380), dir: 'positive', result: hexToF16(0x1381) },
      { val: hexToF16(0x1380), dir: 'negative', result: hexToF16(0x137f) },
      { val: hexToF16(0x9380), dir: 'positive', result: hexToF16(0x937f) },
      { val: hexToF16(0x9380), dir: 'negative', result: hexToF16(0x9381) },

      // Not precisely expressible as f16
      { val: 0.01, dir: 'positive', result: hexToF16(0x211f) }, // positive normal
      { val: 0.01, dir: 'negative', result: hexToF16(0x211e) }, // positive normal
      { val: -0.01, dir: 'positive', result: hexToF16(0xa11e) }, // negative normal
      { val: -0.01, dir: 'negative', result: hexToF16(0xa11f) }, // negative normal
      { val: 2.82E-40, dir: 'positive', result: kValue.f16.subnormal.positive.min }, // positive subnormal
      { val: 2.82E-40, dir: 'negative', result: 0 }, // positive subnormal
      { val: -2.82E-40, dir: 'positive', result: 0 }, // negative subnormal
      { val: -2.82E-40, dir: 'negative', result: kValue.f16.subnormal.negative.max }, // negative subnormal
    ]
  )
  .fn(t => {
    const val = t.params.val;
    const dir = t.params.dir;
    const expect = t.params.result;
    const got = nextAfterF16(val, dir, 'no-flush');
    t.expect(
      got === expect || (Number.isNaN(got) && Number.isNaN(expect)),
      `nextAfterF16(${f64(val)}, '${dir}', 'no-flush') returned ${f16(got)}. Expected ${f16(
        expect
      )}`
    );
  });

interface OneULPCase {
  target: number;
  expect: number;
}

g.test('oneULPF32FlushToZero')
  .paramsSimple<OneULPCase>([
    // Edge Cases
    { target: Number.NaN, expect: Number.NaN },
    { target: Number.POSITIVE_INFINITY, expect: hexToF32(0x73800000) },
    { target: Number.NEGATIVE_INFINITY, expect: hexToF32(0x73800000) },

    // Zeroes
    { target: +0, expect: hexToF32(0x00800000) },
    { target: -0, expect: hexToF32(0x00800000) },

    // Subnormals
    { target: kValue.f32.subnormal.positive.min, expect: hexToF32(0x00800000) },
    { target: 2.82e-40, expect: hexToF32(0x00800000) }, // positive subnormal
    { target: kValue.f32.subnormal.positive.max, expect: hexToF32(0x00800000) },
    { target: kValue.f32.subnormal.negative.min, expect: hexToF32(0x00800000) },
    { target: -2.82e-40, expect: hexToF32(0x00800000) }, // negative subnormal
    { target: kValue.f32.subnormal.negative.max, expect: hexToF32(0x00800000) },

    // Normals
    { target: kValue.f32.positive.min, expect: hexToF32(0x00000001) },
    { target: 1, expect: hexToF32(0x33800000) },
    { target: 2, expect: hexToF32(0x34000000) },
    { target: 4, expect: hexToF32(0x34800000) },
    { target: 1000000, expect: hexToF32(0x3d800000) },
    { target: kValue.f32.positive.max, expect: hexToF32(0x73800000) },
    { target: kValue.f32.negative.max, expect: hexToF32(0x00000001) },
    { target: -1, expect: hexToF32(0x33800000) },
    { target: -2, expect: hexToF32(0x34000000) },
    { target: -4, expect: hexToF32(0x34800000) },
    { target: -1000000, expect: hexToF32(0x3d800000) },
    { target: kValue.f32.negative.min, expect: hexToF32(0x73800000) },

    // No precise f32 value
    { target: 0.001, expect: hexToF32(0x2f000000) }, // positive normal
    { target: -0.001, expect: hexToF32(0x2f000000) }, // negative normal
    { target: 1e40, expect: hexToF32(0x73800000) }, // positive out of range
    { target: -1e40, expect: hexToF32(0x73800000) }, // negative out of range
  ])
  .fn(t => {
    const target = t.params.target;
    const got = oneULPF32(target, 'flush');
    const expect = t.params.expect;
    t.expect(
      got === expect || (Number.isNaN(got) && Number.isNaN(expect)),
      `oneULPF32(${target}, 'flush') returned ${got}. Expected ${expect}`
    );
  });

g.test('oneULPF32NoFlush')
  .paramsSimple<OneULPCase>([
    // Edge Cases
    { target: Number.NaN, expect: Number.NaN },
    { target: Number.POSITIVE_INFINITY, expect: hexToF32(0x73800000) },
    { target: Number.NEGATIVE_INFINITY, expect: hexToF32(0x73800000) },

    // Zeroes
    { target: +0, expect: hexToF32(0x00000001) },
    { target: -0, expect: hexToF32(0x00000001) },

    // Subnormals
    { target: kValue.f32.subnormal.positive.min, expect: hexToF32(0x00000001) },
    { target: -2.82e-40, expect: hexToF32(0x00000001) }, // negative subnormal
    { target: kValue.f32.subnormal.positive.max, expect: hexToF32(0x00000001) },
    { target: kValue.f32.subnormal.negative.min, expect: hexToF32(0x00000001) },
    { target: 2.82e-40, expect: hexToF32(0x00000001) }, // positive subnormal
    { target: kValue.f32.subnormal.negative.max, expect: hexToF32(0x00000001) },

    // Normals
    { target: kValue.f32.positive.min, expect: hexToF32(0x00000001) },
    { target: 1, expect: hexToF32(0x33800000) },
    { target: 2, expect: hexToF32(0x34000000) },
    { target: 4, expect: hexToF32(0x34800000) },
    { target: 1000000, expect: hexToF32(0x3d800000) },
    { target: kValue.f32.positive.max, expect: hexToF32(0x73800000) },
    { target: kValue.f32.negative.max, expect: hexToF32(0x00000001) },
    { target: -1, expect: hexToF32(0x33800000) },
    { target: -2, expect: hexToF32(0x34000000) },
    { target: -4, expect: hexToF32(0x34800000) },
    { target: -1000000, expect: hexToF32(0x3d800000) },
    { target: kValue.f32.negative.min, expect: hexToF32(0x73800000) },

    // No precise f32 value
    { target: 0.001, expect: hexToF32(0x2f000000) }, // positive normal
    { target: -0.001, expect: hexToF32(0x2f000000) }, // negative normal
    { target: 1e40, expect: hexToF32(0x73800000) }, // positive out of range
    { target: -1e40, expect: hexToF32(0x73800000) }, // negative out of range
  ])
  .fn(t => {
    const target = t.params.target;
    const got = oneULPF32(target, 'no-flush');
    const expect = t.params.expect;
    t.expect(
      got === expect || (Number.isNaN(got) && Number.isNaN(expect)),
      `oneULPF32(${target}, no-flush) returned ${got}. Expected ${expect}`
    );
  });

g.test('oneULPF32')
  .paramsSimple<OneULPCase>([
    // Edge Cases
    { target: Number.NaN, expect: Number.NaN },
    { target: Number.NEGATIVE_INFINITY, expect: hexToF32(0x73800000) },
    { target: Number.POSITIVE_INFINITY, expect: hexToF32(0x73800000) },

    // Zeroes
    { target: +0, expect: hexToF32(0x00800000) },
    { target: -0, expect: hexToF32(0x00800000) },

    // Subnormals
    { target: kValue.f32.subnormal.negative.max, expect: hexToF32(0x00800000) },
    { target: -2.82e-40, expect: hexToF32(0x00800000) },
    { target: kValue.f32.subnormal.negative.min, expect: hexToF32(0x00800000) },
    { target: kValue.f32.subnormal.positive.max, expect: hexToF32(0x00800000) },
    { target: 2.82e-40, expect: hexToF32(0x00800000) },
    { target: kValue.f32.subnormal.positive.min, expect: hexToF32(0x00800000) },

    // Normals
    { target: kValue.f32.positive.min, expect: hexToF32(0x00000001) },
    { target: 1, expect: hexToF32(0x33800000) },
    { target: 2, expect: hexToF32(0x34000000) },
    { target: 4, expect: hexToF32(0x34800000) },
    { target: 1000000, expect: hexToF32(0x3d800000) },
    { target: kValue.f32.positive.max, expect: hexToF32(0x73800000) },
    { target: kValue.f32.negative.max, expect: hexToF32(0x000000001) },
    { target: -1, expect: hexToF32(0x33800000) },
    { target: -2, expect: hexToF32(0x34000000) },
    { target: -4, expect: hexToF32(0x34800000) },
    { target: -1000000, expect: hexToF32(0x3d800000) },
    { target: kValue.f32.negative.min, expect: hexToF32(0x73800000) },

    // No precise f32 value
    { target: -0.001, expect: hexToF32(0x2f000000) }, // negative normal
    { target: -1e40, expect: hexToF32(0x73800000) }, // negative out of range
    { target: 0.001, expect: hexToF32(0x2f000000) }, // positive normal
    { target: 1e40, expect: hexToF32(0x73800000) }, // positive out of range
  ])
  .fn(t => {
    const target = t.params.target;
    const got = oneULPF32(target);
    const expect = t.params.expect;
    t.expect(
      got === expect || (Number.isNaN(got) && Number.isNaN(expect)),
      `oneULPF32(${target}) returned ${got}. Expected ${expect}`
    );
  });

interface correctlyRoundedCase {
  value: number;
  expected: Array<number>;
}

g.test('correctlyRoundedF32')
  .paramsSubcasesOnly<correctlyRoundedCase>(
    // prettier-ignore
    [
      // Edge Cases
      { value: kValue.f32.infinity.positive, expected: [kValue.f32.positive.max, Number.POSITIVE_INFINITY] },
      { value: kValue.f32.infinity.negative, expected: [Number.NEGATIVE_INFINITY, kValue.f32.negative.min] },
      { value: kValue.f32.positive.max, expected: [kValue.f32.positive.max] },
      { value: kValue.f32.negative.min, expected: [kValue.f32.negative.min] },

      // 32-bit subnormals
      { value: kValue.f32.subnormal.positive.min, expected: [kValue.f32.subnormal.positive.min] },
      { value: kValue.f32.subnormal.positive.max, expected: [kValue.f32.subnormal.positive.max] },
      { value: kValue.f32.subnormal.negative.min, expected: [kValue.f32.subnormal.negative.min] },
      { value: kValue.f32.subnormal.negative.max, expected: [kValue.f32.subnormal.negative.max] },

      // 64-bit subnormals
      { value: hexToF64(0x0000_0000_0000_0001n), expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x0000_0000_0000_0002n), expected: [0, kValue.f32.subnormal.positive.min] },
      { value: hexToF64(0x800f_ffff_ffff_ffffn), expected: [kValue.f32.subnormal.negative.max, 0] },
      { value: hexToF64(0x800f_ffff_ffff_fffen), expected: [kValue.f32.subnormal.negative.max, 0] },

      // 32-bit normals
      { value: 0, expected: [0] },
      { value: kValue.f32.positive.min, expected: [kValue.f32.positive.min] },
      { value: kValue.f32.negative.max, expected: [kValue.f32.negative.max] },
      { value: hexToF32(0x03800000), expected: [hexToF32(0x03800000)] },
      { value: hexToF32(0x03800001), expected: [hexToF32(0x03800001)] },
      { value: hexToF32(0x83800000), expected: [hexToF32(0x83800000)] },
      { value: hexToF32(0x83800001), expected: [hexToF32(0x83800001)] },

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
    const value = t.params.value;
    const expected = t.params.expected;

    const got = correctlyRoundedF32(value);
    t.expect(
      objectEquals(expected, got),
      `correctlyRoundedF32(${f64(value)}) returned [${got.map(f32)}]. Expected [${expected.map(
        f32
      )}]`
    );
  });

g.test('correctlyRoundedF16')
  .paramsSubcasesOnly<correctlyRoundedCase>(
    // prettier-ignore
    [
      // Edge Cases
      { value: kValue.f16.infinity.positive, expected: [kValue.f16.positive.max, Number.POSITIVE_INFINITY] },
      { value: kValue.f16.infinity.negative, expected: [Number.NEGATIVE_INFINITY, kValue.f16.negative.min] },
      { value: kValue.f16.positive.max, expected: [kValue.f16.positive.max] },
      { value: kValue.f16.negative.min, expected: [kValue.f16.negative.min] },

      // 16-bit subnormals
      { value: kValue.f16.subnormal.positive.min, expected: [kValue.f16.subnormal.positive.min] },
      { value: kValue.f16.subnormal.positive.max, expected: [kValue.f16.subnormal.positive.max] },
      { value: kValue.f16.subnormal.negative.min, expected: [kValue.f16.subnormal.negative.min] },
      { value: kValue.f16.subnormal.negative.max, expected: [kValue.f16.subnormal.negative.max] },

      // 32-bit subnormals
      { value: kValue.f32.subnormal.positive.min, expected: [0, kValue.f16.subnormal.positive.min] },
      { value: kValue.f32.subnormal.positive.max, expected: [0, kValue.f16.subnormal.positive.min] },
      { value: kValue.f32.subnormal.negative.max, expected: [kValue.f16.subnormal.negative.max, 0] },
      { value: kValue.f32.subnormal.negative.min, expected: [kValue.f16.subnormal.negative.max, 0] },

      // 16-bit normals
      { value: 0, expected: [0] },
      { value: kValue.f16.positive.min, expected: [kValue.f16.positive.min] },
      { value: kValue.f16.negative.max, expected: [kValue.f16.negative.max] },
      { value: hexToF16(0x1380), expected: [hexToF16(0x1380)] },
      { value: hexToF16(0x1381), expected: [hexToF16(0x1381)] },
      { value: hexToF16(0x9380), expected: [hexToF16(0x9380)] },
      { value: hexToF16(0x9381), expected: [hexToF16(0x9381)] },

      // 32-bit normals
      { value: hexToF32(0x3a700001), expected: [hexToF16(0x1380), hexToF16(0x1381)] },
      { value: hexToF32(0x3a700002), expected: [hexToF16(0x1380), hexToF16(0x1381)] },
      { value: hexToF32(0xba700001), expected: [hexToF16(0x9381), hexToF16(0x9380)] },
      { value: hexToF32(0xba700002), expected: [hexToF16(0x9381), hexToF16(0x9380)] },
    ]
  )
  .fn(t => {
    const value = t.params.value;
    const expected = t.params.expected;

    const got = correctlyRoundedF16(value);
    t.expect(
      objectEquals(expected, got),
      `correctlyRoundedF16(${f64(value)}) returned [${got.map(f16)}]. Expected [${expected.map(
        f16
      )}]`
    );
  });

interface frexpCase {
  input: number;
  fract: number;
  exp: number;
}

g.test('frexp')
  .paramsSimple<frexpCase>([
    { input: 0, fract: 0, exp: 0 },
    { input: -0, fract: -0, exp: 0 },
    { input: Number.POSITIVE_INFINITY, fract: Number.POSITIVE_INFINITY, exp: 0 },
    { input: Number.NEGATIVE_INFINITY, fract: Number.NEGATIVE_INFINITY, exp: 0 },
    { input: 0.5, fract: 0.5, exp: 0 },
    { input: -0.5, fract: -0.5, exp: 0 },
    { input: 1, fract: 0.5, exp: 1 },
    { input: -1, fract: -0.5, exp: 1 },
    { input: 2, fract: 0.5, exp: 2 },
    { input: -2, fract: -0.5, exp: 2 },
    { input: 10000, fract: 0.6103515625, exp: 14 },
    { input: -10000, fract: -0.6103515625, exp: 14 },
    { input: kValue.f32.positive.max, fract: 0.9999999403953552, exp: 128 },
    { input: kValue.f32.positive.min, fract: 0.5, exp: -125 },
    { input: kValue.f32.negative.max, fract: -0.5, exp: -125 },
    { input: kValue.f32.negative.min, fract: -0.9999999403953552, exp: 128 },
    { input: kValue.f32.subnormal.positive.max, fract: 0.9999998807907104, exp: -126 },
    { input: kValue.f32.subnormal.positive.min, fract: 0.5, exp: -148 },
    { input: kValue.f32.subnormal.negative.max, fract: -0.5, exp: -148 },
    { input: kValue.f32.subnormal.negative.min, fract: -0.9999998807907104, exp: -126 },
  ])
  .fn(test => {
    const input = test.params.input;
    const got = frexp(input);
    const expect = { fract: test.params.fract, exp: test.params.exp };

    test.expect(
      objectEquals(got, expect),
      `frexp(${input}) returned { fract: ${got.fract}, exp: ${got.exp} }. Expected { fract: ${expect.fract}, exp: ${expect.exp} }`
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
      (Number.isNaN(got) && Number.isNaN(expect)) || withinOneULPF32(got, expect, 'flush'),
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
      compareArrayOfNumbersF32(got, expect, 'no-flush'),
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
      compareArrayOfNumbersF32(got, expect, 'no-flush'),
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

g.test('fullF32Range')
  .paramsSimple<fullF32RangeCase>(
    // prettier-ignore
    [
        { neg_norm: 0, neg_sub: 0, pos_sub: 0, pos_norm: 0, expect: [ 0.0 ] },
        { neg_norm: 1, neg_sub: 0, pos_sub: 0, pos_norm: 0, expect: [ kValue.f32.negative.min, 0.0] },
        { neg_norm: 2, neg_sub: 0, pos_sub: 0, pos_norm: 0, expect: [ kValue.f32.negative.min, kValue.f32.negative.max, 0.0 ] },
        { neg_norm: 3, neg_sub: 0, pos_sub: 0, pos_norm: 0, expect: [ kValue.f32.negative.min, -1.9999998807907104, kValue.f32.negative.max, 0.0 ] },
        { neg_norm: 0, neg_sub: 1, pos_sub: 0, pos_norm: 0, expect: [ kValue.f32.subnormal.negative.min, 0.0 ] },
        { neg_norm: 0, neg_sub: 2, pos_sub: 0, pos_norm: 0, expect: [ kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max, 0.0 ] },
        { neg_norm: 0, neg_sub: 0, pos_sub: 1, pos_norm: 0, expect: [ 0.0, kValue.f32.subnormal.positive.min ] },
        { neg_norm: 0, neg_sub: 0, pos_sub: 2, pos_norm: 0, expect: [ 0.0, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.positive.max ] },
        { neg_norm: 0, neg_sub: 0, pos_sub: 0, pos_norm: 1, expect: [ 0.0, kValue.f32.positive.min ] },
        { neg_norm: 0, neg_sub: 0, pos_sub: 0, pos_norm: 2, expect: [ 0.0, kValue.f32.positive.min, kValue.f32.positive.max ] },
        { neg_norm: 0, neg_sub: 0, pos_sub: 0, pos_norm: 3, expect: [ 0.0, kValue.f32.positive.min, 1.9999998807907104, kValue.f32.positive.max ] },
        { neg_norm: 1, neg_sub: 1, pos_sub: 1, pos_norm: 1, expect: [ kValue.f32.negative.min, kValue.f32.subnormal.negative.min, 0.0, kValue.f32.subnormal.positive.min, kValue.f32.positive.min ] },
        { neg_norm: 2, neg_sub: 2, pos_sub: 2, pos_norm: 2, expect: [ kValue.f32.negative.min, kValue.f32.negative.max, kValue.f32.subnormal.negative.min, kValue.f32.subnormal.negative.max, 0.0, kValue.f32.subnormal.positive.min, kValue.f32.subnormal.positive.max, kValue.f32.positive.min, kValue.f32.positive.max ] },
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
      compareArrayOfNumbersF32(got, expect, 'no-flush'),
      `fullF32Range(${neg_norm}, ${neg_sub}, ${pos_sub}, ${pos_norm}) returned [${got}]. Expected [${expect}]`
    );
  });

interface fullF16RangeCase {
  neg_norm: number;
  neg_sub: number;
  pos_sub: number;
  pos_norm: number;
  expect: Array<number>;
}

g.test('fullF16Range')
  .paramsSimple<fullF16RangeCase>(
    // prettier-ignore
    [
          { neg_norm: 0, neg_sub: 0, pos_sub: 0, pos_norm: 0, expect: [ 0.0 ] },
          { neg_norm: 1, neg_sub: 0, pos_sub: 0, pos_norm: 0, expect: [ kValue.f16.negative.min, 0.0] },
          { neg_norm: 2, neg_sub: 0, pos_sub: 0, pos_norm: 0, expect: [ kValue.f16.negative.min, kValue.f16.negative.max, 0.0 ] },
          { neg_norm: 3, neg_sub: 0, pos_sub: 0, pos_norm: 0, expect: [ kValue.f16.negative.min, -1.9990234375, kValue.f16.negative.max, 0.0 ] },
          { neg_norm: 0, neg_sub: 1, pos_sub: 0, pos_norm: 0, expect: [ kValue.f16.subnormal.negative.min, 0.0 ] },
          { neg_norm: 0, neg_sub: 2, pos_sub: 0, pos_norm: 0, expect: [ kValue.f16.subnormal.negative.min, kValue.f16.subnormal.negative.max, 0.0 ] },
          { neg_norm: 0, neg_sub: 0, pos_sub: 1, pos_norm: 0, expect: [ 0.0, kValue.f16.subnormal.positive.min ] },
          { neg_norm: 0, neg_sub: 0, pos_sub: 2, pos_norm: 0, expect: [ 0.0, kValue.f16.subnormal.positive.min, kValue.f16.subnormal.positive.max ] },
          { neg_norm: 0, neg_sub: 0, pos_sub: 0, pos_norm: 1, expect: [ 0.0, kValue.f16.positive.min ] },
          { neg_norm: 0, neg_sub: 0, pos_sub: 0, pos_norm: 2, expect: [ 0.0, kValue.f16.positive.min, kValue.f16.positive.max ] },
          { neg_norm: 0, neg_sub: 0, pos_sub: 0, pos_norm: 3, expect: [ 0.0, kValue.f16.positive.min, 1.9990234375, kValue.f16.positive.max ] },
          { neg_norm: 1, neg_sub: 1, pos_sub: 1, pos_norm: 1, expect: [ kValue.f16.negative.min, kValue.f16.subnormal.negative.min, 0.0, kValue.f16.subnormal.positive.min, kValue.f16.positive.min ] },
          { neg_norm: 2, neg_sub: 2, pos_sub: 2, pos_norm: 2, expect: [ kValue.f16.negative.min, kValue.f16.negative.max, kValue.f16.subnormal.negative.min, kValue.f16.subnormal.negative.max, 0.0, kValue.f16.subnormal.positive.min, kValue.f16.subnormal.positive.max, kValue.f16.positive.min, kValue.f16.positive.max ] },
      ]
  )
  .fn(test => {
    const neg_norm = test.params.neg_norm;
    const neg_sub = test.params.neg_sub;
    const pos_sub = test.params.pos_sub;
    const pos_norm = test.params.pos_norm;
    const got = fullF16Range({ neg_norm, neg_sub, pos_sub, pos_norm });
    const expect = test.params.expect;

    test.expect(
      compareArrayOfNumbersF32(got, expect),
      `fullF16Range(${neg_norm}, ${neg_sub}, ${pos_sub}, ${pos_norm}) returned [${got}]. Expected [${expect}]`
    );
  });

interface fullI32RangeCase {
  neg_count: number;
  pos_count: number;
  expect: Array<number>;
}

g.test('fullI32Range')
  .paramsSimple<fullI32RangeCase>(
    // prettier-ignore
    [
      { neg_count: 0, pos_count: 0, expect: [0] },
      { neg_count: 1, pos_count: 0, expect: [kValue.i32.negative.min, 0] },
      { neg_count: 2, pos_count: 0, expect: [kValue.i32.negative.min, -1, 0] },
      { neg_count: 3, pos_count: 0, expect: [kValue.i32.negative.min, -1610612736, -1, 0] },
      { neg_count: 0, pos_count: 1, expect: [0, 1] },
      { neg_count: 0, pos_count: 2, expect: [0, 1, kValue.i32.positive.max] },
      { neg_count: 0, pos_count: 3, expect: [0, 1, 536870912, kValue.i32.positive.max] },
      { neg_count: 1, pos_count: 1, expect: [kValue.i32.negative.min, 0, 1] },
      { neg_count: 2, pos_count: 2, expect: [kValue.i32.negative.min, -1, 0, 1, kValue.i32.positive.max ] },
    ]
  )
  .fn(test => {
    const neg_count = test.params.neg_count;
    const pos_count = test.params.pos_count;
    const got = fullI32Range({ negative: neg_count, positive: pos_count });
    const expect = test.params.expect;

    test.expect(
      compareArrayOfNumbersF32(got, expect),
      `fullI32Range(${neg_count}, ${pos_count}) returned [${got}]. Expected [${expect}]`
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
    { bits: kBit.f32.positive.less_than_one, value: kValue.f32.positive.less_than_one },
    { bits: kBit.f32.positive.pi.whole, value: kValue.f32.positive.pi.whole },
    { bits: kBit.f32.positive.pi.three_quarters, value: kValue.f32.positive.pi.three_quarters },
    { bits: kBit.f32.positive.pi.half, value: kValue.f32.positive.pi.half },
    { bits: kBit.f32.positive.pi.third, value: kValue.f32.positive.pi.third },
    { bits: kBit.f32.positive.pi.quarter, value: kValue.f32.positive.pi.quarter },
    { bits: kBit.f32.positive.pi.sixth, value: kValue.f32.positive.pi.sixth },
    { bits: kBit.f32.positive.e, value: kValue.f32.positive.e },
    { bits: kBit.f32.negative.max, value: kValue.f32.negative.max },
    { bits: kBit.f32.negative.min, value: kValue.f32.negative.min },
    { bits: kBit.f32.negative.nearest_min, value: kValue.f32.negative.nearest_min },
    { bits: kBit.f32.negative.pi.whole, value: kValue.f32.negative.pi.whole },
    { bits: kBit.f32.negative.pi.three_quarters, value: kValue.f32.negative.pi.three_quarters },
    { bits: kBit.f32.negative.pi.half, value: kValue.f32.negative.pi.half },
    { bits: kBit.f32.negative.pi.third, value: kValue.f32.negative.pi.third },
    { bits: kBit.f32.negative.pi.quarter, value: kValue.f32.negative.pi.quarter },
    { bits: kBit.f32.negative.pi.sixth, value: kValue.f32.negative.pi.sixth },
    { bits: kBit.f32.subnormal.positive.max, value: kValue.f32.subnormal.positive.max },
    { bits: kBit.f32.subnormal.positive.min, value: kValue.f32.subnormal.positive.min },
    { bits: kBit.f32.subnormal.negative.max, value: kValue.f32.subnormal.negative.max },
    { bits: kBit.f32.subnormal.negative.min, value: kValue.f32.subnormal.negative.min },
    { bits: kBit.f32.infinity.positive, value: kValue.f32.infinity.positive },
    { bits: kBit.f32.infinity.negative, value: kValue.f32.infinity.negative },
  ])
  .fn(test => {
    const bits = test.params.bits;
    const value = test.params.value;

    const val_to_bits = bits === float32ToUint32(value);
    const bits_to_val = value === uint32ToFloat32(bits);
    test.expect(
      val_to_bits && bits_to_val,
      `bits = ${bits}, value = ${value}, returned val_to_bits as ${val_to_bits}, and bits_to_val as ${bits_to_val}, they are expected to be equivalent`
    );
  });

// Test to confirm kBit and kValue constants are equivalent for f16
g.test('f16LimitsEquivalency')
  .paramsSimple<limitsCase>([
    { bits: kBit.f16.positive.max, value: kValue.f16.positive.max },
    { bits: kBit.f16.positive.min, value: kValue.f16.positive.min },
    { bits: kBit.f16.negative.max, value: kValue.f16.negative.max },
    { bits: kBit.f16.negative.min, value: kValue.f16.negative.min },
    { bits: kBit.f16.subnormal.positive.max, value: kValue.f16.subnormal.positive.max },
    { bits: kBit.f16.subnormal.positive.min, value: kValue.f16.subnormal.positive.min },
    { bits: kBit.f16.subnormal.negative.max, value: kValue.f16.subnormal.negative.max },
    { bits: kBit.f16.subnormal.negative.min, value: kValue.f16.subnormal.negative.min },
    { bits: kBit.f16.infinity.positive, value: kValue.f16.infinity.positive },
    { bits: kBit.f16.infinity.negative, value: kValue.f16.infinity.negative },
  ])
  .fn(test => {
    const bits = test.params.bits;
    const value = test.params.value;

    const val_to_bits = bits === float16ToUint16(value);
    const bits_to_val = value === uint16ToFloat16(bits);
    test.expect(
      val_to_bits && bits_to_val,
      `bits = ${bits}, value = ${value}, returned val_to_bits as ${val_to_bits}, and bits_to_val as ${bits_to_val}, they are expected to be equivalent`
    );
  });

interface cartesianProductCase<T> {
  inputs: T[][];
  result: T[][];
}

g.test('cartesianProductNumber')
  .paramsSimple<cartesianProductCase<number>>(
    // prettier-ignore
    [
      { inputs: [[0], [1]], result: [[0, 1]] },
      { inputs: [[0, 1], [2]], result: [[0, 2],
                                        [1, 2]] },
      { inputs: [[0], [1, 2]], result: [[0, 1],
                                        [0, 2]] },
      { inputs: [[0, 1], [2, 3]], result: [[0,2],
                                           [1, 2],
                                           [0, 3],
                                           [1, 3]] },
      { inputs: [[0, 1, 2], [3, 4, 5]], result: [[0, 3],
                                                 [1, 3],
                                                 [2, 3],
                                                 [0, 4],
                                                 [1, 4],
                                                 [2, 4],
                                                 [0, 5],
                                                 [1, 5],
                                                 [2, 5]] },
      { inputs: [[0, 1], [2, 3], [4, 5]], result: [[0, 2, 4],
                                                   [1, 2, 4],
                                                   [0, 3, 4],
                                                   [1, 3, 4],
                                                   [0, 2, 5],
                                                   [1, 2, 5],
                                                   [0, 3, 5],
                                                   [1, 3, 5]] },

  ]
  )
  .fn(test => {
    const inputs = test.params.inputs;
    const got = cartesianProduct(...inputs);
    const expect = test.params.result;

    test.expect(
      objectEquals(got, expect),
      `cartesianProduct(${JSON.stringify(inputs)}) returned ${JSON.stringify(
        got
      )}. Expected ${JSON.stringify(expect)} `
    );
  });

g.test('cartesianProductArray')
  .paramsSimple<cartesianProductCase<number[]>>(
    // prettier-ignore
    [
      { inputs: [[[0, 1], [2, 3]], [[4, 5], [6, 7]]], result: [[[0, 1], [4, 5]],
                                                               [[2, 3], [4, 5]],
                                                               [[0, 1], [6, 7]],
                                                               [[2, 3], [6, 7]]]},
      { inputs: [[[0, 1], [2, 3]], [[4, 5], [6, 7]], [[8, 9]]], result: [[[0, 1], [4, 5], [8, 9]],
                                                                         [[2, 3], [4, 5], [8, 9]],
                                                                         [[0, 1], [6, 7], [8, 9]],
                                                                         [[2, 3], [6, 7], [8, 9]]]},
      { inputs: [[[0, 1, 2], [3, 4, 5], [6, 7, 8]], [[2, 1, 0], [5, 4, 3], [8, 7, 6]]], result:  [[[0, 1, 2], [2, 1, 0]],
                                                                                                  [[3, 4, 5], [2, 1, 0]],
                                                                                                  [[6, 7, 8], [2, 1, 0]],
                                                                                                  [[0, 1, 2], [5, 4, 3]],
                                                                                                  [[3, 4, 5], [5, 4, 3]],
                                                                                                  [[6, 7, 8], [5, 4, 3]],
                                                                                                  [[0, 1, 2], [8, 7, 6]],
                                                                                                  [[3, 4, 5], [8, 7, 6]],
                                                                                                  [[6, 7, 8], [8, 7, 6]]]}

    ]
  )
  .fn(test => {
    const inputs = test.params.inputs;
    const got = cartesianProduct(...inputs);
    const expect = test.params.result;

    test.expect(
      objectEquals(got, expect),
      `cartesianProduct(${JSON.stringify(inputs)}) returned ${JSON.stringify(
        got
      )}. Expected ${JSON.stringify(expect)} `
    );
  });

interface calculatePermutationsCase<T> {
  input: T[];
  result: T[][];
}

g.test('calculatePermutations')
  .paramsSimple<calculatePermutationsCase<number>>(
    // prettier-ignore
    [
      { input: [0, 1], result: [[0, 1],
                                [1, 0]] },
      { input: [0, 1, 2], result: [[0, 1, 2],
                                   [0, 2, 1],
                                   [1, 0, 2],
                                   [1, 2, 0],
                                   [2, 0, 1],
                                   [2, 1, 0]] },
        { input: [0, 1, 2, 3], result: [[0, 1, 2, 3],
                                        [0, 1, 3, 2],
                                        [0, 2, 1, 3],
                                        [0, 2, 3, 1],
                                        [0, 3, 1, 2],
                                        [0, 3, 2, 1],
                                        [1, 0, 2, 3],
                                        [1, 0, 3, 2],
                                        [1, 2, 0, 3],
                                        [1, 2, 3, 0],
                                        [1, 3, 0, 2],
                                        [1, 3, 2, 0],
                                        [2, 0, 1, 3],
                                        [2, 0, 3, 1],
                                        [2, 1, 0, 3],
                                        [2, 1, 3, 0],
                                        [2, 3, 0, 1],
                                        [2, 3, 1, 0],
                                        [3, 0, 1, 2],
                                        [3, 0, 2, 1],
                                        [3, 1, 0, 2],
                                        [3, 1, 2, 0],
                                        [3, 2, 0, 1],
                                        [3, 2, 1, 0]] },
    ]
  )
  .fn(test => {
    const input = test.params.input;
    const got = calculatePermutations(input);
    const expect = test.params.result;

    test.expect(
      objectEquals(got, expect),
      `calculatePermutations(${JSON.stringify(input)}) returned ${JSON.stringify(
        got
      )}. Expected ${JSON.stringify(expect)} `
    );
  });
