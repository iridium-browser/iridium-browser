export const description = `
Execution Tests for the 'ceil' builtin function
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { assert } from '../../../../common/util/util.js';
import { GPUTest } from '../../../gpu_test.js';
import { NumberRepr } from '../../../util/conversion.js';
import { generateTypes } from '../../types.js';

import { kBit, kValue, runShaderTest } from './builtin.js';

export const g = makeTestGroup(GPUTest);

g.test('float_builtin_functions,ceil')
  .uniqueId('38d65728ea728bc5')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#float-builtin-functions')
  .desc(
    `
ceil:
T is f32 or vecN<f32> ceil(e: T ) -> T Returns the ceiling of e. Component-wise when T is a vector. (GLSLstd450Ceil)

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md
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
      'ceil',
      Float32Array,
      /* prettier-ignore */ [
        // Small positive numbers
        {input: NumberRepr.fromF32(0.1), expected: [NumberRepr.fromF32(1.0)] },
        {input: NumberRepr.fromF32(0.9), expected: [NumberRepr.fromF32(1.0)] },
        {input: NumberRepr.fromF32(1.1), expected: [NumberRepr.fromF32(2.0)] },
        {input: NumberRepr.fromF32(1.9), expected: [NumberRepr.fromF32(2.0)] },

        // Small negative numbers
        {input: NumberRepr.fromF32(-0.1), expected: [NumberRepr.fromF32(0.0)] },
        {input: NumberRepr.fromF32(-0.9), expected: [NumberRepr.fromF32(0.0)] },
        {input: NumberRepr.fromF32(-1.1), expected: [NumberRepr.fromF32(-1.0)] },
        {input: NumberRepr.fromF32(-1.9), expected: [NumberRepr.fromF32(-1.0)] },

        // Min and Max f32
        {input: NumberRepr.fromF32Bits(kBit.f32.negative.max), expected: [NumberRepr.fromF32(0.0)] },
        {input: NumberRepr.fromF32Bits(kBit.f32.negative.min), expected: [NumberRepr.fromF32Bits(kBit.f32.negative.min)] },
        {input: NumberRepr.fromF32Bits(kBit.f32.positive.min), expected: [NumberRepr.fromF32(1.0)] },
        {input: NumberRepr.fromF32Bits(kBit.f32.positive.max), expected: [NumberRepr.fromF32Bits(kBit.f32.positive.max)] },

        // Subnormal f32
        {input: NumberRepr.fromF32Bits(kBit.f32.subnormal.positive.max), expected: [NumberRepr.fromF32(1.0), NumberRepr.fromF32(0.0)] },
        {input: NumberRepr.fromF32Bits(kBit.f32.subnormal.positive.min), expected: [NumberRepr.fromF32(1.0), NumberRepr.fromF32(0.0)] },

        // Infinity f32
        {input: NumberRepr.fromF32Bits(kBit.f32.infinity.negative), expected: [NumberRepr.fromF32Bits(kBit.f32.infinity.negative)] },
        {input: NumberRepr.fromF32Bits(kBit.f32.infinity.positive), expected: [NumberRepr.fromF32Bits(kBit.f32.infinity.positive)] },

        // Powers of +2.0: 2.0^i: 1 <= i <= 31
        {input: NumberRepr.fromF32(kValue.powTwo.to1), expected: [NumberRepr.fromF32(kValue.powTwo.to1)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to2), expected: [NumberRepr.fromF32(kValue.powTwo.to2)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to3), expected: [NumberRepr.fromF32(kValue.powTwo.to3)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to4), expected: [NumberRepr.fromF32(kValue.powTwo.to4)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to5), expected: [NumberRepr.fromF32(kValue.powTwo.to5)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to6), expected: [NumberRepr.fromF32(kValue.powTwo.to6)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to7), expected: [NumberRepr.fromF32(kValue.powTwo.to7)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to8), expected: [NumberRepr.fromF32(kValue.powTwo.to8)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to9), expected: [NumberRepr.fromF32(kValue.powTwo.to9)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to10), expected: [NumberRepr.fromF32(kValue.powTwo.to10)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to11), expected: [NumberRepr.fromF32(kValue.powTwo.to11)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to12), expected: [NumberRepr.fromF32(kValue.powTwo.to12)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to13), expected: [NumberRepr.fromF32(kValue.powTwo.to13)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to14), expected: [NumberRepr.fromF32(kValue.powTwo.to14)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to15), expected: [NumberRepr.fromF32(kValue.powTwo.to15)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to16), expected: [NumberRepr.fromF32(kValue.powTwo.to16)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to17), expected: [NumberRepr.fromF32(kValue.powTwo.to17)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to18), expected: [NumberRepr.fromF32(kValue.powTwo.to18)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to19), expected: [NumberRepr.fromF32(kValue.powTwo.to19)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to20), expected: [NumberRepr.fromF32(kValue.powTwo.to20)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to21), expected: [NumberRepr.fromF32(kValue.powTwo.to21)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to22), expected: [NumberRepr.fromF32(kValue.powTwo.to22)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to23), expected: [NumberRepr.fromF32(kValue.powTwo.to23)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to24), expected: [NumberRepr.fromF32(kValue.powTwo.to24)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to25), expected: [NumberRepr.fromF32(kValue.powTwo.to25)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to26), expected: [NumberRepr.fromF32(kValue.powTwo.to26)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to27), expected: [NumberRepr.fromF32(kValue.powTwo.to27)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to28), expected: [NumberRepr.fromF32(kValue.powTwo.to28)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to29), expected: [NumberRepr.fromF32(kValue.powTwo.to29)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to30), expected: [NumberRepr.fromF32(kValue.powTwo.to30)] },
        {input: NumberRepr.fromF32(kValue.powTwo.to31), expected: [NumberRepr.fromF32(kValue.powTwo.to31)] },

        // Powers of -2.0: -2.0^i: 1 <= i <= 31
        {input: NumberRepr.fromF32(kValue.negPowTwo.to1), expected: [NumberRepr.fromF32(kValue.negPowTwo.to1)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to2), expected: [NumberRepr.fromF32(kValue.negPowTwo.to2)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to3), expected: [NumberRepr.fromF32(kValue.negPowTwo.to3)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to4), expected: [NumberRepr.fromF32(kValue.negPowTwo.to4)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to5), expected: [NumberRepr.fromF32(kValue.negPowTwo.to5)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to6), expected: [NumberRepr.fromF32(kValue.negPowTwo.to6)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to7), expected: [NumberRepr.fromF32(kValue.negPowTwo.to7)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to8), expected: [NumberRepr.fromF32(kValue.negPowTwo.to8)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to9), expected: [NumberRepr.fromF32(kValue.negPowTwo.to9)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to10), expected: [NumberRepr.fromF32(kValue.negPowTwo.to10)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to11), expected: [NumberRepr.fromF32(kValue.negPowTwo.to11)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to12), expected: [NumberRepr.fromF32(kValue.negPowTwo.to12)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to13), expected: [NumberRepr.fromF32(kValue.negPowTwo.to13)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to14), expected: [NumberRepr.fromF32(kValue.negPowTwo.to14)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to15), expected: [NumberRepr.fromF32(kValue.negPowTwo.to15)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to16), expected: [NumberRepr.fromF32(kValue.negPowTwo.to16)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to17), expected: [NumberRepr.fromF32(kValue.negPowTwo.to17)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to18), expected: [NumberRepr.fromF32(kValue.negPowTwo.to18)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to19), expected: [NumberRepr.fromF32(kValue.negPowTwo.to19)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to20), expected: [NumberRepr.fromF32(kValue.negPowTwo.to20)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to21), expected: [NumberRepr.fromF32(kValue.negPowTwo.to21)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to22), expected: [NumberRepr.fromF32(kValue.negPowTwo.to22)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to23), expected: [NumberRepr.fromF32(kValue.negPowTwo.to23)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to24), expected: [NumberRepr.fromF32(kValue.negPowTwo.to24)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to25), expected: [NumberRepr.fromF32(kValue.negPowTwo.to25)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to26), expected: [NumberRepr.fromF32(kValue.negPowTwo.to26)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to27), expected: [NumberRepr.fromF32(kValue.negPowTwo.to27)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to28), expected: [NumberRepr.fromF32(kValue.negPowTwo.to28)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to29), expected: [NumberRepr.fromF32(kValue.negPowTwo.to29)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to30), expected: [NumberRepr.fromF32(kValue.negPowTwo.to30)] },
        {input: NumberRepr.fromF32(kValue.negPowTwo.to31), expected: [NumberRepr.fromF32(kValue.negPowTwo.to31)] },
      ]
    );
  });
