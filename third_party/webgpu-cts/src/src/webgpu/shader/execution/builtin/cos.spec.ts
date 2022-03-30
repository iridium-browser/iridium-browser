export const description = `
Execution Tests for the 'cos' builtin function
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUTest } from '../../../gpu_test.js';
import { f32, TypeF32, u32 } from '../../../util/conversion.js';
import { linearRange } from '../../../util/math.js';

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
    // [1]: Need to decide what the ground-truth is.
    const truthFunc = (x: number): Case => {
      return { input: f32(x), expected: f32(Math.cos(x)) };
    };

    // Spec defines accuracy on [-π, π]
    const cases = linearRange(f32(-Math.PI), f32(Math.PI), u32(1000)).map(x => truthFunc(x));

    const cfg: Config = t.params;
    cfg.cmpFloats = absThreshold(2 ** -11);
    run(t, 'cos', [TypeF32], TypeF32, cfg, cases);
  });
