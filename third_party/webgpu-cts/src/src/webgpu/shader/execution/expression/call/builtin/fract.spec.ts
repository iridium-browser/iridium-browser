export const description = `
Execution tests for the 'fract' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn fract(e: T ) -> T
Returns the fractional part of e, computed as e - floor(e).
Component-wise when T is a vector.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { correctlyRoundedMatch, anyOf } from '../../../../../util/compare.js';
import { kBit, kValue } from '../../../../../util/constants.js';
import { f32, f32Bits, TypeF32 } from '../../../../../util/conversion.js';
import { isSubnormalNumber } from '../../../../../util/math.js';
import { Case, Config, makeUnaryF32Case, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

g.test('abstract_float')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`abstract float tests`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();

g.test('f32')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f32 tests`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const cfg: Config = t.params;
    cfg.cmpFloats = correctlyRoundedMatch();

    const makeCase = (x: number): Case => {
      const result = x - Math.floor(x);
      if (result < 1.0 || isSubnormalNumber(x)) {
        return makeUnaryF32Case(x, x => x - Math.floor(x));
      }
      // Very small negative numbers can lead to catastrophic cancellation, thus calculating a fract of 1.0, which is
      // technically not a fractional part.
      // Some platforms return the nearest number less than 1, and others 1.0.
      // https://github.com/gpuweb/gpuweb/issues/2822 has been filed to clarify.
      return { input: f32(x), expected: anyOf(f32(1.0), f32Bits(0x3f7f_ffff)) };
    };

    const cases: Array<Case> = [
      // Zeroes
      { input: f32Bits(kBit.f32.positive.zero), expected: f32(0) },
      { input: f32Bits(kBit.f32.negative.zero), expected: f32(0) },

      // Positive numbers
      makeCase(0.1), // ~0.1 -> ~0.1
      makeCase(0.5), // 0.5 -> 0.5
      makeCase(0.9), // ~0.9 -> ~0.9
      makeCase(1), // 1 -> 0
      makeCase(2), // 2 -> 0
      makeCase(1.11), // ~1.11 -> ~0.11
      makeCase(10.0001), // ~10.0001 -> ~0.0001

      // Negative numbers
      makeCase(-0.1), // ~-0.1 -> ~0.9
      makeCase(-0.5), // -0.5 -> 0.5
      makeCase(-0.9), // ~-0.9 -> ~0.1
      makeCase(-1), // -1 -> 0
      makeCase(-2), // -2 -> 0
      makeCase(-1.11), // ~-1.11 -> ~0.89
      makeCase(-10.0001), // -10.0001 -> ~0.9999

      // Min and Max f32
      makeCase(kValue.f32.positive.min),
      makeCase(kValue.f32.positive.max),
      makeCase(kValue.f32.negative.max),
      makeCase(kValue.f32.negative.min),

      // Subnormal f32
      makeCase(kValue.f32.subnormal.positive.min),
      makeCase(kValue.f32.subnormal.positive.max),
      makeCase(kValue.f32.subnormal.negative.max),
      makeCase(kValue.f32.subnormal.negative.min),
    ];

    // prettier-ignore
    run(t, builtin('fract'), [TypeF32], TypeF32, cfg, cases);
  });

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(`f16 tests`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .unimplemented();
