export const description = `
Execution Tests for the 'abs' builtin function
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { assert } from '../../../../common/util/util.js';
import { GPUTest } from '../../../gpu_test.js';
import { NumberRepr } from '../../../util/conversion.js';
import { generateTypes } from '../../types.js';

import { kBit, kValue, runShaderTest } from './builtin.js';

export const g = makeTestGroup(GPUTest);

g.test('integer_builtin_functions,abs_unsigned')
  .uniqueId('59ff84968a839124')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#integer-builtin-functions')
  .desc(
    `
scalar case, unsigned abs:
abs(e: T ) -> T
T is u32 or vecN<u32>. Result is e.
This is provided for symmetry with abs for signed integers.
Component-wise when T is a vector.
`
  )
  .params(u =>
    u
      .combineWithParams([{ storageClass: 'storage', storageMode: 'read_write' }] as const)
      .combine('containerType', ['scalar', 'vector'] as const)
      .combine('isAtomic', [false])
      .combine('baseType', ['u32'] as const)
      .expandWithParams(generateTypes)
  )
  .fn(async t => {
    assert(t.params._kTypeInfo !== undefined, 'generated type is undefined');
    runShaderTest(
      t,
      t.params.storageClass,
      t.params.storageMode,
      t.params.type,
      t.params._kTypeInfo.arrayLength,
      'abs',
      Uint32Array,
      /* prettier-ignore */ [
        // Min and Max u32
        {input: NumberRepr.fromU32Bits(kBit.u32.min), expected: [NumberRepr.fromU32Bits(kBit.u32.min)] },
        {input: NumberRepr.fromU32Bits(kBit.u32.max), expected: [NumberRepr.fromU32Bits(kBit.u32.max)] },
        // Powers of 2: -2^i: 0 =< i =< 31
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to0), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to0)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to1), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to1)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to2), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to2)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to3), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to3)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to4), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to4)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to5), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to5)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to6), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to6)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to7), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to7)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to8), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to8)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to9), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to9)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to10), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to10)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to11), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to11)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to12), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to12)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to13), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to13)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to14), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to14)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to15), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to15)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to16), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to16)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to17), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to17)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to18), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to18)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to19), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to19)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to20), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to20)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to21), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to21)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to22), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to22)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to23), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to23)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to24), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to24)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to25), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to25)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to26), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to26)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to27), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to27)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to28), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to28)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to29), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to29)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to30), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to30)] },
        {input: NumberRepr.fromU32Bits(kBit.powTwo.to31), expected: [NumberRepr.fromU32Bits(kBit.powTwo.to31)] },
      ]
    );
  });

g.test('integer_builtin_functions,abs_signed')
  .uniqueId('d8fc581d17db6ae8')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#integer-builtin-functions')
  .desc(
    `
signed abs:
abs(e: T ) -> T
T is i32 or vecN<i32>. The result is the absolute value of e.
Component-wise when T is a vector.
If e evaluates to the largest negative value, then the result is e.
(GLSLstd450SAbs)
`
  )
  .params(u =>
    u
      .combineWithParams([{ storageClass: 'storage', storageMode: 'read_write' }] as const)
      .combine('containerType', ['scalar', 'vector'] as const)
      .combine('isAtomic', [false])
      .combine('baseType', ['i32'] as const)
      .expandWithParams(generateTypes)
  )
  .fn(async t => {
    assert(t.params._kTypeInfo !== undefined, 'generated type is undefined');
    runShaderTest(
      t,
      t.params.storageClass,
      t.params.storageMode,
      t.params.type,
      t.params._kTypeInfo.arrayLength,
      'abs',
      Int32Array,
      /* prettier-ignore */ [
        // Min and max i32
        // If e evaluates to the largest negative value, then the result is e.
        {input: NumberRepr.fromI32Bits(kBit.i32.negative.min), expected: [NumberRepr.fromI32Bits(kBit.i32.negative.min)] },
        {input: NumberRepr.fromI32Bits(kBit.i32.negative.max), expected: [NumberRepr.fromI32Bits(kBit.i32.positive.min)] },
        {input: NumberRepr.fromI32Bits(kBit.i32.positive.max), expected: [NumberRepr.fromI32Bits(kBit.i32.positive.max)] },
        {input: NumberRepr.fromI32Bits(kBit.i32.positive.min), expected: [NumberRepr.fromI32Bits(kBit.i32.positive.min)] },
        // input: -1 * pow(2, n), n = {-31, ..., 0 }, expected: [pow(2, n), n = {-31, ..., 0}]
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to0), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to0)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to1), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to1)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to2), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to2)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to3), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to3)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to4), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to4)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to5), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to5)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to6), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to6)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to7), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to7)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to8), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to8)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to9), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to9)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to10), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to10)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to11), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to11)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to12), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to12)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to13), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to13)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to14), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to14)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to15), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to15)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to16), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to16)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to17), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to17)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to18), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to18)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to19), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to19)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to20), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to20)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to21), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to21)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to22), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to22)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to23), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to23)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to24), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to24)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to25), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to25)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to26), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to26)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to27), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to27)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to28), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to28)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to29), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to29)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to30), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to30)] },
        {input: NumberRepr.fromI32Bits(kBit.negPowTwo.to31), expected: [NumberRepr.fromI32Bits(kBit.powTwo.to31)] },
      ]
    );
  });

g.test('float_builtin_functions,abs_float')
  .uniqueId('2c1782b6a8dec8cb')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#float-builtin-functions')
  .desc(
    `
float abs:
abs(e: T ) -> T
T is f32 or vecN<f32>
Returns the absolute value of e (e.g. e with a positive sign bit).
Component-wise when T is a vector. (GLSLstd450Fabs)
`
  )
  .params(u =>
    u
      .combineWithParams([{ storageClass: 'storage', storageMode: 'read_write' }] as const)
      .combine('containerType', ['scalar', 'vector'] as const)
      .combine('isAtomic', [false])
      .combine('baseType', ['f32'] as const)
      .expandWithParams(generateTypes)
  )
  .fn(async t => {
    assert(t.params._kTypeInfo !== undefined, 'generated type is undefined');
    runShaderTest(
      t,
      t.params.storageClass,
      t.params.storageMode,
      t.params.type,
      t.params._kTypeInfo.arrayLength,
      'abs',
      Float32Array,
      /* prettier-ignore */ [
        // Min and Max f32
        {input: NumberRepr.fromF32Bits(kBit.f32.negative.max), expected: [NumberRepr.fromF32Bits(0x0080_0000)] },
        {input: NumberRepr.fromF32Bits(kBit.f32.negative.min), expected: [NumberRepr.fromF32Bits(0x7f7f_ffff)] },
        {input: NumberRepr.fromF32Bits(kBit.f32.positive.min), expected: [NumberRepr.fromF32Bits(kBit.f32.positive.min)] },
        {input: NumberRepr.fromF32Bits(kBit.f32.positive.max), expected: [NumberRepr.fromF32Bits(kBit.f32.positive.max)] },

        // Subnormal f32
        // TODO(sarahM0): Check if this is needed (or if it has to fail). If yes add other values.
        {input: NumberRepr.fromF32Bits(kBit.f32.subnormal.positive.max), expected: [NumberRepr.fromF32Bits(kBit.f32.subnormal.positive.max), NumberRepr.fromF32(0)] },
        {input: NumberRepr.fromF32Bits(kBit.f32.subnormal.positive.min), expected: [NumberRepr.fromF32Bits(kBit.f32.subnormal.positive.min), NumberRepr.fromF32(0)] },

        // Infinity f32
        {input: NumberRepr.fromF32Bits(kBit.f32.infinity.negative), expected: [NumberRepr.fromF32Bits(kBit.f32.infinity.positive)] },
        {input: NumberRepr.fromF32Bits(kBit.f32.infinity.positive), expected: [NumberRepr.fromF32Bits(kBit.f32.infinity.positive)] },

        // Powers of 2.0: -2.0^i: -1 >= i >= -31
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus1), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus1)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus2), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus2)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus3), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus3)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus4), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus4)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus5), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus5)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus6), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus6)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus7), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus7)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus8), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus8)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus9), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus9)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus10), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus10)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus11), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus11)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus12), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus12)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus13), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus13)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus14), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus14)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus15), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus15)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus16), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus16)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus17), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus17)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus18), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus18)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus19), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus19)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus20), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus20)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus21), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus21)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus22), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus22)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus23), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus23)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus24), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus24)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus25), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus25)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus26), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus26)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus27), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus27)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus28), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus28)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus29), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus29)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus30), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus30)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.toMinus31), expected: [NumberRepr.fromF32(kValue.powTwo.toMinus31)] },

        // Powers of 2.0: -2.0^i: 1 <= i <= 31
        {input: NumberRepr.fromF32(kValue.negPowTwo.to1), expected: [NumberRepr.fromF32(kValue.powTwo.to1)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to2), expected: [NumberRepr.fromF32(kValue.powTwo.to2)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to3), expected: [NumberRepr.fromF32(kValue.powTwo.to3)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to4), expected: [NumberRepr.fromF32(kValue.powTwo.to4)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to5), expected: [NumberRepr.fromF32(kValue.powTwo.to5)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to6), expected: [NumberRepr.fromF32(kValue.powTwo.to6)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to7), expected: [NumberRepr.fromF32(kValue.powTwo.to7)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to8), expected: [NumberRepr.fromF32(kValue.powTwo.to8)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to9), expected: [NumberRepr.fromF32(kValue.powTwo.to9)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to10), expected: [NumberRepr.fromF32(kValue.powTwo.to10)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to11), expected: [NumberRepr.fromF32(kValue.powTwo.to11)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to12), expected: [NumberRepr.fromF32(kValue.powTwo.to12)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to13), expected: [NumberRepr.fromF32(kValue.powTwo.to13)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to14), expected: [NumberRepr.fromF32(kValue.powTwo.to14)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to15), expected: [NumberRepr.fromF32(kValue.powTwo.to15)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to16), expected: [NumberRepr.fromF32(kValue.powTwo.to16)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to17), expected: [NumberRepr.fromF32(kValue.powTwo.to17)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to18), expected: [NumberRepr.fromF32(kValue.powTwo.to18)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to19), expected: [NumberRepr.fromF32(kValue.powTwo.to19)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to20), expected: [NumberRepr.fromF32(kValue.powTwo.to20)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to21), expected: [NumberRepr.fromF32(kValue.powTwo.to21)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to22), expected: [NumberRepr.fromF32(kValue.powTwo.to22)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to23), expected: [NumberRepr.fromF32(kValue.powTwo.to23)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to24), expected: [NumberRepr.fromF32(kValue.powTwo.to24)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to25), expected: [NumberRepr.fromF32(kValue.powTwo.to25)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to26), expected: [NumberRepr.fromF32(kValue.powTwo.to26)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to27), expected: [NumberRepr.fromF32(kValue.powTwo.to27)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to28), expected: [NumberRepr.fromF32(kValue.powTwo.to28)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to29), expected: [NumberRepr.fromF32(kValue.powTwo.to29)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to30), expected: [NumberRepr.fromF32(kValue.powTwo.to30)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to31), expected: [NumberRepr.fromF32(kValue.powTwo.to31)] },
      ]
    );
  });
