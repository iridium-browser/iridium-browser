export const description = `
Execution tests for the 'inverseSqrt' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn inverseSqrt(e: T ) -> T
Returns the reciprocal of sqrt(e). Component-wise when T is a vector.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { ulpMatch } from '../../../../../util/compare.js';
import { kBit, kValue } from '../../../../../util/constants.js';
import { f32, f32Bits, TypeF32 } from '../../../../../util/conversion.js';
import { biasedRange, linearRange } from '../../../../../util/math.js';
import { Case, Config, run } from '../../expression.js';

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
    // [1]: Need to decide what the ground-truth is.
    const makeCase = (x: number): Case => {
      return { input: f32(x), expected: f32(1 / Math.sqrt(x)) };
    };

    // Well defined cases
    const cases: Array<Case> = [
      { input: f32Bits(kBit.f32.infinity.positive), expected: f32(0) },
      { input: f32(1), expected: f32(1) },
      // 0 < x <= 1 linearly spread
      ...linearRange(kValue.f32.positive.min, 1, 100).map(x => makeCase(x)),
      // 1 <= x < 2^32, biased towards 1
      ...biasedRange(1, 2 ** 32, 1000).map(x => makeCase(x)),
    ];

    const cfg: Config = t.params;
    cfg.cmpFloats = ulpMatch(2);
    run(t, builtin('inverseSqrt'), [TypeF32], TypeF32, cfg, cases);
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
