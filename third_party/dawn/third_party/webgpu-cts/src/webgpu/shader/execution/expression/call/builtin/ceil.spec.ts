export const description = `
Execution tests for the 'ceil' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn ceil(e: T ) -> T
Returns the ceiling of e. Component-wise when T is a vector.

`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { correctlyRoundedMatch } from '../../../../../util/compare.js';
import { kBit } from '../../../../../util/constants.js';
import { f32, f32Bits, TypeF32 } from '../../../../../util/conversion.js';
import { fullF32Range } from '../../../../../util/math.js';
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
      return makeUnaryF32Case(x, Math.ceil);
    };

    const cases: Array<Case> = [
      // Small positive numbers
      { input: f32(0.1), expected: f32(1.0) },
      { input: f32(0.9), expected: f32(1.0) },
      { input: f32(1.1), expected: f32(2.0) },
      { input: f32(1.9), expected: f32(2.0) },

      // Small negative numbers
      { input: f32(-0.1), expected: f32(0.0) },
      { input: f32(-0.9), expected: f32(0.0) },
      { input: f32(-1.1), expected: f32(-1.0) },
      { input: f32(-1.9), expected: f32(-1.0) },

      // Min and Max f32
      { input: f32Bits(kBit.f32.negative.max), expected: f32(0.0) },
      { input: f32Bits(kBit.f32.negative.min), expected: f32Bits(kBit.f32.negative.min) },
      { input: f32Bits(kBit.f32.positive.min), expected: f32(1.0) },
      { input: f32Bits(kBit.f32.positive.max), expected: f32Bits(kBit.f32.positive.max) },

      // Infinity f32
      { input: f32Bits(kBit.f32.infinity.negative), expected: f32Bits(kBit.f32.infinity.negative) },
      { input: f32Bits(kBit.f32.infinity.positive), expected: f32Bits(kBit.f32.infinity.positive) },
      ...fullF32Range().map(x => makeCase(x)),
    ];

    run(t, builtin('ceil'), [TypeF32], TypeF32, cfg, cases);
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
