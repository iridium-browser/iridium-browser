export const description = `
Execution Tests for the 'cos' builtin function
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUTest } from '../../../gpu_test.js';
import { f32, TypeF32 } from '../../../util/conversion.js';

import { absThreshold, Case, Config, run } from './builtin.js';

export const g = makeTestGroup(GPUTest);

g.test('float_builtin_functions,cos')
  .uniqueId('650973d690dcd841')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#float-builtin-functions')
  .desc(
    `
cos:
T is f32 or vecN<f32> cos(e: T ) -> T Returns the cosine of e. Component-wise when T is a vector. (GLSLstd450Cos)

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md

TODO(#792): Decide what the ground-truth is for these tests. [1]
`
  )
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    const cases = new Array<Case>(1000);
    for (let i = 0; i < cases.length; i++) {
      // [1]: Need to decide what the ground-truth is.
      const angle = -Math.PI + (2.0 * Math.PI * i) / (cases.length - 1);
      cases[i] = { input: f32(angle), expected: f32(Math.cos(angle)) };
    }

    const cfg: Config = t.params;
    cfg.cmpFloats = absThreshold(2 ** -11);
    run(t, 'cos', [TypeF32], TypeF32, cfg, cases);
  });
