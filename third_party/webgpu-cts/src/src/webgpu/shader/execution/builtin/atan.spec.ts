export const description = `
Execution Tests for the 'atan' builtin function
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUTest } from '../../../gpu_test.js';
import { f32, f32Bits, TypeF32, u32 } from '../../../util/conversion.js';
import { biasedRange, linearRange } from '../../../util/math.js';

import { Case, Config, kBit, run, ulpThreshold } from './builtin.js';

export const g = makeTestGroup(GPUTest);

g.test('float_builtin_functions,atan')
  .uniqueId('b13828d6243d13dd')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#float-builtin-functions')
  .desc(
    `
atan:
T is f32 or vecN<f32> atan(e: T ) -> T Returns the arc tangent of e. Component-wise when T is a vector. (GLSLstd450Atan)

TODO(#792): Decide what the ground-truth is for these tests. [1]
`
  )
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    // [1]: Need to decide what the ground-truth is.
    const truthFunc = (x: number): Case => {
      return { input: f32(x), expected: f32(Math.atan(x)) };
    };

    // Well defined/border cases
    let cases: Array<Case> = [
      { input: f32Bits(kBit.f32.infinity.negative), expected: f32(-Math.PI / 2) },
      { input: f32(-Math.sqrt(3)), expected: f32(-Math.PI / 3) },
      { input: f32(-1), expected: f32(-Math.PI / 4) },
      { input: f32(-Math.sqrt(3) / 3), expected: f32(-Math.PI / 6) },
      { input: f32(Math.sqrt(3) / 3), expected: f32(Math.PI / 6) },
      { input: f32(1), expected: f32(Math.PI / 4) },
      { input: f32(Math.sqrt(3)), expected: f32(Math.PI / 3) },
      { input: f32Bits(kBit.f32.infinity.positive), expected: f32(Math.PI / 2) },

      // Zero-like cases
      { input: f32(0), expected: f32(0) },
      { input: f32Bits(kBit.f32.positive.min), expected: f32(0) },
      { input: f32Bits(kBit.f32.negative.max), expected: f32(0) },
      { input: f32Bits(kBit.f32.positive.zero), expected: f32(0) },
      { input: f32Bits(kBit.f32.negative.zero), expected: f32(0) },
      { input: f32Bits(kBit.f32.positive.min), expected: f32Bits(kBit.f32.positive.min) },
      { input: f32Bits(kBit.f32.negative.max), expected: f32Bits(kBit.f32.negative.max) },
      { input: f32Bits(kBit.f32.positive.min), expected: f32Bits(kBit.f32.negative.max) },
      { input: f32Bits(kBit.f32.negative.max), expected: f32Bits(kBit.f32.positive.min) },
      { input: f32Bits(kBit.f32.positive.zero), expected: f32Bits(kBit.f32.positive.zero) },
      { input: f32Bits(kBit.f32.negative.zero), expected: f32Bits(kBit.f32.negative.zero) },
      { input: f32Bits(kBit.f32.positive.zero), expected: f32Bits(kBit.f32.negative.zero) },
      { input: f32Bits(kBit.f32.negative.zero), expected: f32Bits(kBit.f32.positive.zero) },
    ];

    //  -2^32 < x <= -1, biased towards -1
    cases = cases.concat(biasedRange(f32(1), f32(2 ** 32), u32(1000)).map(x => truthFunc(-x)));

    // -1 <= x < 0, linearly spread
    cases = cases.concat(
      linearRange(f32Bits(kBit.f32.positive.min), f32(1), u32(100)).map(x => truthFunc(-x))
    );

    // 0 < x <= 1, linearly spread
    cases = cases.concat(
      linearRange(f32Bits(kBit.f32.positive.min), f32(1), u32(100)).map(x => truthFunc(x))
    );

    // 1 <= x < 2^32, biased towards 1
    cases = cases.concat(biasedRange(f32(1), f32(2 ** 32), u32(1000)).map(x => truthFunc(x)));

    const cfg: Config = t.params;
    cfg.cmpFloats = ulpThreshold(4096);
    run(t, 'atan', [TypeF32], TypeF32, cfg, cases);
  });
