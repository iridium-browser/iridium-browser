export const description = `
Execution tests for the 'atan2' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn atan2(e1: T ,e2: T ) -> T
Returns the arc tangent of e1 over e2. Component-wise when T is a vector.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { anyOf, ulpMatch } from '../../../../../util/compare.js';
import { f64, TypeF32 } from '../../../../../util/conversion.js';
import { fullF32Range, isSubnormalNumber } from '../../../../../util/math.js';
import { Case, Config, makeBinaryF32Case, run } from '../../expression.js';

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
    const cfg: Config = t.params;
    cfg.cmpFloats = ulpMatch(4096);

    // [1]: Need to decide what the ground-truth is.
    const makeCase = (y: number, x: number): Case => {
      const c = makeBinaryF32Case(y, x, Math.atan2, true);
      if (isSubnormalNumber(y)) {
        // If y is subnormal, also expect possible results of atan2(0, x)
        c.expected = anyOf(c.expected, f64(0), f64(Math.PI), f64(-Math.PI));
      }
      return c;
    };

    const numeric_range = fullF32Range({
      neg_norm: 100,
      neg_sub: 10,
      pos_sub: 10,
      pos_norm: 100,
    });

    const cases: Array<Case> = [];
    numeric_range.forEach((y, y_idx) => {
      numeric_range.forEach((x, x_idx) => {
        // atan2(y, 0) is not well defined, so skipping those cases
        if (!isSubnormalNumber(x)) {
          if (x_idx >= y_idx) {
            cases.push(makeCase(y, x));
          }
        }
      });
    });
    run(t, builtin('atan2'), [TypeF32, TypeF32], TypeF32, cfg, cases);
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
