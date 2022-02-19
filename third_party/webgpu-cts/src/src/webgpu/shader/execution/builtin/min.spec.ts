export const description = `
Execution Tests for the 'min' builtin function
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUTest } from '../../../gpu_test.js';
import { i32, i32Bits, TypeI32, TypeU32, u32 } from '../../../util/conversion.js';

import { run } from './builtin.js';

export const g = makeTestGroup(GPUTest);

g.test('integer_builtin_functions,unsigned_min')
  .uniqueId('29aba7ede5b93cdd')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#integer-builtin-functions')
  .desc(
    `
unsigned min:
T is u32 or vecN<u32> min(e1: T ,e2: T) -> T Returns e1 if e1 is less than e2, and e2 otherwise. Component-wise when T is a vector. (GLSLstd450UMin)

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md
`
  )
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    run(t, 'min', [TypeU32, TypeU32], TypeU32, t.params, [
      { input: [u32(1), u32(1)], expected: u32(1) },
      { input: [u32(0), u32(0)], expected: u32(0) },
      { input: [u32(0xffffffff), u32(0xffffffff)], expected: u32(0xffffffff) },
      { input: [u32(1), u32(2)], expected: u32(1) },
      { input: [u32(2), u32(1)], expected: u32(1) },
      { input: [u32(0x70000000), u32(0x80000000)], expected: u32(0x70000000) },
      { input: [u32(0x80000000), u32(0x70000000)], expected: u32(0x70000000) },
      { input: [u32(0), u32(0xffffffff)], expected: u32(0) },
      { input: [u32(0xffffffff), u32(0)], expected: u32(0) },
      { input: [u32(0), u32(0xffffffff)], expected: u32(0) },
    ]);
  });

g.test('integer_builtin_functions,signed_min')
  .uniqueId('60c8ecdf409b45fc')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#integer-builtin-functions')
  .desc(
    `
signed min:
T is i32 or vecN<i32> min(e1: T ,e2: T) -> T Returns e1 if e1 is less than e2, and e2 otherwise. Component-wise when T is a vector. (GLSLstd45SUMin)

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md
`
  )
  .params(u =>
    u
      .combine('storageClass', ['uniform', 'storage_r', 'storage_rw'] as const)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
  )
  .fn(async t => {
    run(t, 'min', [TypeI32, TypeI32], TypeI32, t.params, [
      { input: [i32(1), i32(1)], expected: i32(1) },
      { input: [i32(0), i32(0)], expected: i32(0) },
      { input: [i32(-1), i32(-1)], expected: i32(-1) },
      { input: [i32(1), i32(2)], expected: i32(1) },
      { input: [i32(2), i32(1)], expected: i32(1) },
      { input: [i32(-1), i32(-2)], expected: i32(-2) },
      { input: [i32(-2), i32(-1)], expected: i32(-2) },
      { input: [i32(1), i32(-1)], expected: i32(-1) },
      { input: [i32(-1), i32(1)], expected: i32(-1) },
      { input: [i32Bits(0x70000000), i32Bits(0x80000000)], expected: i32Bits(0x80000000) },
      { input: [i32Bits(0x80000000), i32Bits(0x70000000)], expected: i32Bits(0x80000000) },
      { input: [i32Bits(0xffffffff), i32(0)], expected: i32Bits(0xffffffff) },
      { input: [i32(0), i32Bits(0xffffffff)], expected: i32Bits(0xffffffff) },
    ]);
  });
