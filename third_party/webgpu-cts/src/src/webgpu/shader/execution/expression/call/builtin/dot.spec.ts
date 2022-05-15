export const description = `
Execution tests for the 'dot' builtin function

T is AbstractInt, AbstractFloat, i32, u32, f32, or f16
@const fn dot(e1: vecN<T>,e2: vecN<T>) -> T
Returns the dot product of e1 and e2.

`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

g.test('abstract_int')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`abstract int tests`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [2, 3, 4] as const)
  )
  .unimplemented();

g.test('i32')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`i32 tests`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [2, 3, 4] as const)
  )
  .unimplemented();

g.test('u32')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`u32 tests`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [2, 3, 4] as const)
  )
  .unimplemented();

g.test('abstract_float')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`abstract float test`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [2, 3, 4] as const)
  )
  .unimplemented();

g.test('f32')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`f32 tests`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [2, 3, 4] as const)
  )
  .unimplemented();

g.test('f16')
  .specURL('https://www.w3.org/TR/WGSL/#vector-builtin-functions')
  .desc(`f16 tests`)
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [2, 3, 4] as const)
  )
  .unimplemented();
