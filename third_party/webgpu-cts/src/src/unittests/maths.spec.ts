export const description = `
Util math unit tests.
`;

import { makeTestGroup } from '../common/framework/test_group.js';
import { diffULP } from '../webgpu/util/math.js';

import { UnitTest } from './unit_test.js';

export const g = makeTestGroup(UnitTest);

interface DiffULPCase {
  a: number;
  b: number;
  ulp: number;
}

function hexToF32(hex: number): number {
  return new Float32Array(new Uint32Array([hex]).buffer)[0];
}

g.test('test,math,diffULP')
  .paramsSimple<DiffULPCase>([
    { a: 0, b: 0, ulp: 0 },
    { a: 1, b: 2, ulp: 2 ** 23 }, // Single exponent step
    { a: 2, b: 1, ulp: 2 ** 23 }, // Single exponent step
    { a: 2, b: 4, ulp: 2 ** 23 }, // Single exponent step
    { a: 4, b: 2, ulp: 2 ** 23 }, // Single exponent step
    { a: -1, b: -2, ulp: 2 ** 23 }, // Single exponent step
    { a: -2, b: -1, ulp: 2 ** 23 }, // Single exponent step
    { a: -2, b: -4, ulp: 2 ** 23 }, // Single exponent step
    { a: -4, b: -2, ulp: 2 ** 23 }, // Single exponent step
    { a: hexToF32(0x00800000), b: hexToF32(0x00800001), ulp: 1 }, // Single mantissa step
    { a: hexToF32(0x00800001), b: hexToF32(0x00800000), ulp: 1 }, // Single mantissa step
    { a: hexToF32(0x03800000), b: hexToF32(0x03800001), ulp: 1 }, // Single mantissa step
    { a: hexToF32(0x03800001), b: hexToF32(0x03800000), ulp: 1 }, // Single mantissa step
    { a: -hexToF32(0x00800000), b: -hexToF32(0x00800001), ulp: 1 }, // Single mantissa step
    { a: -hexToF32(0x00800001), b: -hexToF32(0x00800000), ulp: 1 }, // Single mantissa step
    { a: -hexToF32(0x03800000), b: -hexToF32(0x03800001), ulp: 1 }, // Single mantissa step
    { a: -hexToF32(0x03800001), b: -hexToF32(0x03800000), ulp: 1 }, // Single mantissa step
    { a: hexToF32(0x00800000), b: -hexToF32(0x00800000), ulp: 2 ** 24 }, // Two exponent steps
    { a: -hexToF32(0x00800000), b: hexToF32(0x00800000), ulp: 2 ** 24 }, // Two exponent steps
  ])
  .fn(t => {
    const a = t.params.a;
    const b = t.params.b;
    const got = diffULP(a, b);
    const expect = t.params.ulp;
    t.expect(got === expect, `diffULP(${a}, ${b}) returned ${got}. Expected ${expect}`);
  });
