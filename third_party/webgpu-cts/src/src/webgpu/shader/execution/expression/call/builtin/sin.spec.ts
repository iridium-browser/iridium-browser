export const description = `
Execution tests for the 'sin' builtin function

S is AbstractFloat, f32, f16
T is S or vecN<S>
@const fn sin(e: T ) -> T
Returns the sine of e. Component-wise when T is a vector.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { absMatch } from '../../../../../util/compare.js';
import { TypeF32 } from '../../../../../util/conversion.js';
import { linearRange } from '../../../../../util/math.js';
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
      return makeUnaryF32Case(x, Math.sin);
    };

    // Spec defines accuracy on [-π, π]
    const cases = linearRange(-Math.PI, Math.PI, 1000).map(x => makeCase(x));

    const cfg: Config = t.params;
    cfg.cmpFloats = absMatch(2 ** -11);
    run(t, builtin('sin'), [TypeF32], TypeF32, cfg, cases);
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
