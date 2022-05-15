export const description = `Unit tests for conversion`;

import { makeTestGroup } from '../common/internal/test_group.js';
import {
  float16BitsToFloat32,
  float32ToFloat16Bits,
  float32ToFloatBits,
  floatBitsToNormalULPFromZero,
  floatBitsToNumber,
  kFloat16Format,
  kFloat32Format,
} from '../webgpu/util/conversion.js';

import { UnitTest } from './unit_test.js';

export const g = makeTestGroup(UnitTest);

const cases = [
  [0b0_01111_0000000000, 1],
  [0b0_00001_0000000000, 0.00006103515625],
  [0b0_01101_0101010101, 0.33325195],
  [0b0_11110_1111111111, 65504],
  [0b0_00000_0000000000, 0],
  [0b0_01110_0000000000, 0.5],
  [0b0_01100_1001100110, 0.1999512],
  [0b0_01111_0000000001, 1.00097656],
  [0b0_10101_1001000000, 100],
  [0b1_01100_1001100110, -0.1999512],
  [0b1_10101_1001000000, -100],
];

g.test('float16BitsToFloat32').fn(t => {
  for (const [bits, number] of [
    ...cases,
    [0b1_00000_0000000000, -0], // (resulting sign is not actually tested)
    [0b0_00000_1111111111, 0.00006104], // subnormal f16 input
    [0b1_00000_1111111111, -0.00006104],
  ]) {
    const actual = float16BitsToFloat32(bits);
    t.expect(
      // some loose check
      Math.abs(actual - number) <= 0.00001,
      `for ${bits.toString(2)}, expected ${number}, got ${actual}`
    );
  }
});

g.test('float32ToFloat16Bits').fn(t => {
  for (const [bits, number] of [
    ...cases,
    [0b0_00000_0000000000, 0.00001], // input that becomes subnormal in f16 is rounded to 0
    [0b1_00000_0000000000, -0.00001], // and sign is preserved
  ]) {
    // some loose check
    const actual = float32ToFloat16Bits(number);
    t.expect(
      Math.abs(actual - bits) <= 1,
      `for ${number}, expected ${bits.toString(2)}, got ${actual.toString(2)}`
    );
  }
});

g.test('float32ToFloatBits_floatBitsToNumber')
  .paramsSubcasesOnly(u =>
    u
      .combine('signed', [0, 1] as const)
      .combine('exponentBits', [5, 8])
      .combine('mantissaBits', [10, 23])
  )
  .fn(t => {
    const { signed, exponentBits, mantissaBits } = t.params;
    const bias = (1 << (exponentBits - 1)) - 1;

    for (const [, value] of cases) {
      if (value < 0 && signed === 0) continue;
      const bits = float32ToFloatBits(value, signed, exponentBits, mantissaBits, bias);
      const reconstituted = floatBitsToNumber(bits, { signed, exponentBits, mantissaBits, bias });
      t.expect(Math.abs(reconstituted - value) <= 0.0000001, `${reconstituted} vs ${value}`);
    }
  });

g.test('floatBitsToULPFromZero,16').fn(t => {
  const test = (bits: number, ulpFromZero: number) =>
    t.expect(floatBitsToNormalULPFromZero(bits, kFloat16Format) === ulpFromZero, bits.toString(2));
  // Zero
  test(0b0_00000_0000000000, 0);
  // Subnormal
  test(0b0_00000_0000000001, 0);
  test(0b1_00000_0000000001, 0);
  test(0b0_00000_1111111111, 0);
  test(0b1_00000_1111111111, 0);
  // Normal
  test(0b0_00001_0000000000, 1); // 0 + 1ULP
  test(0b1_00001_0000000000, -1); // 0 - 1ULP
  test(0b0_00001_0000000001, 2); // 0 + 2ULP
  test(0b1_00001_0000000001, -2); // 0 - 2ULP
  test(0b0_01110_0000000000, 0b01101_0000000001); // 0.5
  test(0b1_01110_0000000000, -0b01101_0000000001); // -0.5
  test(0b0_01110_1111111110, 0b01101_1111111111); // 1.0 - 2ULP
  test(0b1_01110_1111111110, -0b01101_1111111111); // -(1.0 - 2ULP)
  test(0b0_01110_1111111111, 0b01110_0000000000); // 1.0 - 1ULP
  test(0b1_01110_1111111111, -0b01110_0000000000); // -(1.0 - 1ULP)
  test(0b0_01111_0000000000, 0b01110_0000000001); // 1.0
  test(0b1_01111_0000000000, -0b01110_0000000001); // -1.0
  test(0b0_01111_0000000001, 0b01110_0000000010); // 1.0 + 1ULP
  test(0b1_01111_0000000001, -0b01110_0000000010); // -(1.0 + 1ULP)
  test(0b0_10000_0000000000, 0b01111_0000000001); // 2.0
  test(0b1_10000_0000000000, -0b01111_0000000001); // -2.0

  const testThrows = (b: number) =>
    t.shouldThrow('Error', () => floatBitsToNormalULPFromZero(b, kFloat16Format));
  // Infinity
  testThrows(0b0_11111_0000000000);
  testThrows(0b1_11111_0000000000);
  // NaN
  testThrows(0b0_11111_1111111111);
  testThrows(0b1_11111_1111111111);
});

g.test('floatBitsToULPFromZero,32').fn(t => {
  const test = (bits: number, ulpFromZero: number) =>
    t.expect(floatBitsToNormalULPFromZero(bits, kFloat32Format) === ulpFromZero, bits.toString(2));
  // Zero
  test(0b0_00000000_00000000000000000000000, 0);
  // Subnormal
  test(0b0_00000000_00000000000000000000001, 0);
  test(0b1_00000000_00000000000000000000001, 0);
  test(0b0_00000000_11111111111111111111111, 0);
  test(0b1_00000000_11111111111111111111111, 0);
  // Normal
  test(0b0_00000001_00000000000000000000000, 1); // 0 + 1ULP
  test(0b1_00000001_00000000000000000000000, -1); // 0 - 1ULP
  test(0b0_00000001_00000000000000000000001, 2); // 0 + 2ULP
  test(0b1_00000001_00000000000000000000001, -2); // 0 - 2ULP
  test(0b0_01111110_00000000000000000000000, 0b01111101_00000000000000000000001); // 0.5
  test(0b1_01111110_00000000000000000000000, -0b01111101_00000000000000000000001); // -0.5
  test(0b0_01111110_11111111111111111111110, 0b01111101_11111111111111111111111); // 1.0 - 2ULP
  test(0b1_01111110_11111111111111111111110, -0b01111101_11111111111111111111111); // -(1.0 - 2ULP)
  test(0b0_01111110_11111111111111111111111, 0b01111110_00000000000000000000000); // 1.0 - 1ULP
  test(0b1_01111110_11111111111111111111111, -0b01111110_00000000000000000000000); // -(1.0 - 1ULP)
  test(0b0_01111111_00000000000000000000000, 0b01111110_00000000000000000000001); // 1.0
  test(0b1_01111111_00000000000000000000000, -0b01111110_00000000000000000000001); // -1.0
  test(0b0_01111111_00000000000000000000001, 0b01111110_00000000000000000000010); // 1.0 + 1ULP
  test(0b1_01111111_00000000000000000000001, -0b01111110_00000000000000000000010); // -(1.0 + 1ULP)
  test(0b0_11110000_00000000000000000000000, 0b11101111_00000000000000000000001); // 2.0
  test(0b1_11110000_00000000000000000000000, -0b11101111_00000000000000000000001); // -2.0

  const testThrows = (b: number) =>
    t.shouldThrow('Error', () => floatBitsToNormalULPFromZero(b, kFloat32Format));
  // Infinity
  testThrows(0b0_11111111_00000000000000000000000);
  testThrows(0b1_11111111_00000000000000000000000);
  // NaN
  testThrows(0b0_11111111_11111111111111111111111);
  testThrows(0b0_11111111_00000000000000000000001);
  testThrows(0b1_11111111_11111111111111111111111);
  testThrows(0b1_11111111_00000000000000000000001);
});
