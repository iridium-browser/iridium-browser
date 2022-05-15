export const description = `
Execution tests for the 'ldexp' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>

K is AbstractInt, i32
I is K or vecN<K>, where
  I is a scalar if T is a scalar, or a vector when T is a vector

@const fn ldexp(e1: T ,e2: I ) -> T
Returns e1 * 2^e2. Component-wise when T is a vector.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { correctlyRoundedMatch } from '../../../../../util/compare.js';
import { kValue } from '../../../../../util/constants.js';
import { f32, i32, TypeF32, TypeI32 } from '../../../../../util/conversion.js';
import { biasedRange, linearRange, quantizeToI32 } from '../../../../../util/math.js';
import { Case, Config, run } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

g.test('abstract_float')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(
    `
`
  )
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
    const truthFunc = (e1: number, e2: number): Case | undefined => {
      const i32_e2 = quantizeToI32(e2);
      const result = e1 * Math.pow(2, i32_e2);
      // Unclear what the expected behaviour for values that overflow f32 bounds, see https://github.com/gpuweb/gpuweb/issues/2631
      if (!Number.isFinite(result)) {
        return undefined;
      } else if (result > kValue.f32.positive.max) {
        return undefined;
      } else if (result < kValue.f32.positive.min) {
        return undefined;
      }
      return { input: [f32(e1), i32(e2)], expected: f32(result) };
    };

    const e1_range: Array<number> = [
      //  -2^32 < x <= -1, biased towards -1
      ...biasedRange(-1, -(2 ** 32), 50),
      // -1 <= x <= 0, linearly spread
      ...linearRange(-1, 0, 20),
      // 0 <= x <= -1, linearly spread
      ...linearRange(0, 1, 20),
      // 1 <= x < 2^32, biased towards 1
      ...biasedRange(1, 2 ** 32, 50),
    ];

    const e2_range: Array<number> = [
      // -127 < x <= 0, biased towards 0
      ...biasedRange(0, -127, 20).map(x => Math.round(x)),
      // 0 <= x < 128, biased towards 0
      ...biasedRange(0, 128, 20).map(x => Math.round(x)),
    ];

    const cases: Array<Case> = [];
    e1_range.forEach(e1 => {
      e2_range.forEach(e2 => {
        const c = truthFunc(e1, e2);
        if (c !== undefined) {
          cases.push(c);
        }
      });
    });
    const cfg: Config = t.params;
    cfg.cmpFloats = correctlyRoundedMatch();
    run(t, builtin('ldexp'), [TypeF32, TypeI32], TypeF32, cfg, cases);
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
