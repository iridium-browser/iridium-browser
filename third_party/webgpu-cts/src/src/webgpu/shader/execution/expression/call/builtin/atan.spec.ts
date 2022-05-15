export const description = `
Execution tests for the 'atan' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn atan(e: T ) -> T
Returns the arc tangent of e. Component-wise when T is a vector.

`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { ulpMatch } from '../../../../../util/compare.js';
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
  .desc(
    `
f32 tests

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
    const makeCase = (x: number): Case => {
      return makeUnaryF32Case(x, Math.atan);
    };
    const cases: Array<Case> = [
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
      ...fullF32Range({ neg_norm: 1000, neg_sub: 100, pos_sub: 100, pos_norm: 1000 }).map(x =>
        makeCase(x)
      ),
    ];

    const cfg: Config = t.params;
    cfg.cmpFloats = ulpMatch(4096);
    run(t, builtin('atan'), [TypeF32], TypeF32, cfg, cases);
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
