export const description = `Unit tests for conversion`;

import { makeTestGroup } from '../common/internal/test_group.js';
import { float16BitsToFloat32, float32ToFloat16Bits } from '../webgpu/util/conversion.js';

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

g.test('conversion,float16BitsToFloat32').fn(t => {
  cases.forEach(value => {
    // some loose check
    t.expect(Math.abs(float16BitsToFloat32(value[0]) - value[1]) <= 0.00001, value[0].toString(2));
  });
});

g.test('conversion,float32ToFloat16Bits').fn(t => {
  cases.forEach(value => {
    // some loose check
    // Does not handle clamping, underflow, overflow, or denormalized numbers.
    t.expect(Math.abs(float32ToFloat16Bits(value[1]) - value[0]) <= 1, value[1].toString());
  });
});
