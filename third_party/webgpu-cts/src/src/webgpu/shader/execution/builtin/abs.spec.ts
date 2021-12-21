export const description = `
Execution Tests for the 'all' builtin function
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { assert } from '../../../../common/util/util.js';
import { GPUTest } from '../../../gpu_test.js';
import { generateTypes } from '../../types.js';

import { OperandType, runShaderTest } from './builtin.js';

export const g = makeTestGroup(GPUTest);

g.test('abs_float')
  .uniqueId(0x1f3fc889e2b1727f)
  .desc(
    `
https://www.w3.org/TR/2021/WD-WGSL-20210831#float-builtin-functions
float abs:
T is f32 or vecN<f32> abs(e: T ) -> T
Returns the absolute value of e (e.g. e with a positive sign bit).
Component-wise when T is a vector.
(GLSLstd450Fabs)
`
  )
  .params(u =>
    u
      .combineWithParams([
        { storageClass: 'storage', storageMode: 'read_write', access: 'read' },
      ] as const)
      .combine('containerType', ['scalar', 'vector'] as const)
      .combine('isAtomic', [false])
      .combine('baseType', ['f32'] as const)
      .beginSubcases()
      .expandWithParams(generateTypes)
  )
  .fn(async t => {
    assert(t.params._kTypeInfo !== undefined, 'generated type is undefined');
    runShaderTest(
      t,
      t.params.storageClass,
      t.params.storageMode,
      t.params.baseType,
      t.params.type,
      t.params._kTypeInfo.arrayLength,
      'abs',
      OperandType.Float,
      [
        { input: 0.0, expected: [0.0] },
        { input: 1.0, expected: [1.0] },
        { input: 2.0, expected: [2.0] },
        { input: 4.0, expected: [4.0] },
        { input: 8.0, expected: [8.0] },
        { input: 16.0, expected: [16.0] },
        { input: 32.0, expected: [32.0] },
        { input: 64.0, expected: [64.0] },
        { input: 128.0, expected: [128.0] },
        { input: 256.0, expected: [256.0] },
        { input: 512.0, expected: [512.0] },
        { input: 1024.0, expected: [1024.0] },
        { input: 2048.0, expected: [2048.0] },
        { input: 4096.0, expected: [4096.0] },
        { input: 8192.0, expected: [8192.0] },
        { input: 16384.0, expected: [16384.0] },
        { input: 32768.0, expected: [32768.0] },
        { input: 65536.0, expected: [65536.0] },
        { input: 131072.0, expected: [131072.0] },
        { input: 262144.0, expected: [262144.0] },
        { input: 524288.0, expected: [524288.0] },
        { input: 1048576.0, expected: [1048576.0] },
        { input: 8388607.0, expected: [8388607.0] }, //2^23 - 1
        { input: 16777215.0, expected: [16777215.0] }, //2^24 - 1
        { input: 16777216.0, expected: [16777216.0] }, //2^24
        { input: 0.0, expected: [0.0] },
        { input: -1.0, expected: [1.0] },
        { input: -2.0, expected: [2.0] },
        { input: -4.0, expected: [4.0] },
        { input: -8.0, expected: [8.0] },
        { input: -16.0, expected: [16.0] },
        { input: -32.0, expected: [32.0] },
        { input: -64.0, expected: [64.0] },
        { input: -128.0, expected: [128.0] },
        { input: -256.0, expected: [256.0] },
        { input: -512.0, expected: [512.0] },
        { input: -1024.0, expected: [1024.0] },
        { input: -2048.0, expected: [2048.0] },
        { input: -4096.0, expected: [4096.0] },
        { input: -8192.0, expected: [8192.0] },
        { input: -16384.0, expected: [16384.0] },
        { input: -32768.0, expected: [32768.0] },
        { input: -65536.0, expected: [65536.0] },
        { input: -131072.0, expected: [131072.0] },
        { input: -262144.0, expected: [262144.0] },
        { input: -524288.0, expected: [524288.0] },
        { input: -1048576.0, expected: [1048576.0] },
        { input: -8388607.0, expected: [8388607.0] }, //2^23 - 1
        { input: -16777215.0, expected: [16777215.0] }, //2^24 - 1
        { input: -16777216.0, expected: [16777216.0] }, //2^24
      ]
    );
  });

g.test('abs_int')
  .uniqueId(0xfab878e682c16d42)
  .desc(
    `
https://www.w3.org/TR/2021/WD-WGSL-20210831#integer-builtin-functions
signed abs:
T is i32 or vecN<i32> abs(e: T ) -> T The absolute value of e.
Component-wise when T is a vector.
If e evaluates to the largest negative value, then the result is e.
`
  )
  .params(u =>
    u
      .combineWithParams([
        { storageClass: 'storage', storageMode: 'read_write', access: 'read' },
      ] as const)
      .combine('containerType', ['scalar', 'vector'] as const)
      .combine('isAtomic', [false])
      .combine('baseType', ['i32'] as const)
      .beginSubcases()
      .expandWithParams(generateTypes)
  )
  .fn(async t => {
    assert(t.params._kTypeInfo !== undefined, 'generated type is undefined');
    runShaderTest(
      t,
      t.params.storageClass,
      t.params.storageMode,
      t.params.baseType,
      t.params.type,
      t.params._kTypeInfo.arrayLength,
      'abs',
      OperandType.Int,
      [
        { input: -1, expected: [1] },
        { input: -2, expected: [2] },
        { input: -4, expected: [4] },
        { input: -8, expected: [8] },
        { input: -16, expected: [16] },
        { input: -32, expected: [32] },
        { input: -64, expected: [64] },
        { input: -128, expected: [128] },
        { input: -256, expected: [256] },
        { input: -512, expected: [512] },
        { input: -1024, expected: [1024] },
        { input: -2048, expected: [2048] },
        { input: -4096, expected: [4096] },
        { input: -8192, expected: [8192] },
        { input: -16384, expected: [16384] },
        { input: -32768, expected: [32768] },
        { input: -65536, expected: [65536] },
        { input: -131072, expected: [131072] },
        { input: -262144, expected: [262144] },
        { input: -524288, expected: [524288] },
        { input: -1048576, expected: [1048576] },
        { input: -8388607, expected: [8388607] },
        { input: -16777215, expected: [16777215] },
        { input: -16777216, expected: [16777216] },
        { input: -134217727, expected: [134217727] },
        { input: -1073741823, expected: [1073741823] },
        { input: -2147483647, expected: [2147483647] },
        { input: 0, expected: [0] },
        { input: 1, expected: [1] },
        { input: 2, expected: [2] },
        { input: 4, expected: [4] },
        { input: 8, expected: [8] },
        { input: 16, expected: [16] },
        { input: 32, expected: [32] },
        { input: 64, expected: [64] },
        { input: 128, expected: [128] },
        { input: 256, expected: [256] },
        { input: 512, expected: [512] },
        { input: 1024, expected: [1024] },
        { input: 2048, expected: [2048] },
        { input: 4096, expected: [4096] },
        { input: 8192, expected: [8192] },
        { input: 16384, expected: [16384] },
        { input: 32768, expected: [32768] },
        { input: 65536, expected: [65536] },
        { input: 131072, expected: [131072] },
        { input: 262144, expected: [262144] },
        { input: 524288, expected: [524288] },
        { input: 1048576, expected: [1048576] },
        { input: 8388607, expected: [8388607] }, //2^23 - 1
        { input: 16777215, expected: [16777215] }, //2^24 - 1
        { input: 16777216, expected: [16777216] }, //2^24
        { input: 134217727, expected: [134217727] }, //2^27 - 1
        { input: 1073741823, expected: [1073741823] }, //2^30 - 1
      ]
    );
  });

g.test('abs_uint')
  .uniqueId(0x59ff84968a839124)
  .desc(
    `
https://www.w3.org/TR/2021/WD-WGSL-20210831#integer-builtin-function
scalar case, unsigned abs:
T is u32 or vecN<u32> abs(e: T ) -> T Result is e.
This is provided for symmetry with abs for signed integers.
Component-wise when T is a vector.
`
  )
  .params(u =>
    u
      .combineWithParams([
        { storageClass: 'storage', storageMode: 'read_write', access: 'read' },
      ] as const)
      .combine('containerType', ['scalar', 'vector'] as const)
      .combine('isAtomic', [false])
      .combine('baseType', ['u32'] as const)
      .beginSubcases()
      .expandWithParams(generateTypes)
  )
  .fn(async t => {
    assert(t.params._kTypeInfo !== undefined, 'generated type is undefined');
    runShaderTest(
      t,
      t.params.storageClass,
      t.params.storageMode,
      t.params.baseType,
      t.params.type,
      t.params._kTypeInfo.arrayLength,
      'abs',
      OperandType.Uint,
      [
        { input: 0, expected: [0] },
        { input: 1, expected: [1] },
        { input: 2, expected: [2] },
        { input: 4, expected: [4] },
        { input: 8, expected: [8] },
        { input: 16, expected: [16] },
        { input: 32, expected: [32] },
        { input: 64, expected: [64] },
        { input: 128, expected: [128] },
        { input: 256, expected: [256] },
        { input: 512, expected: [512] },
        { input: 1024, expected: [1024] },
        { input: 2048, expected: [2048] },
        { input: 4096, expected: [4096] },
        { input: 8192, expected: [8192] },
        { input: 16384, expected: [16384] },
        { input: 32768, expected: [32768] },
        { input: 65536, expected: [65536] },
        { input: 131072, expected: [131072] },
        { input: 262144, expected: [262144] },
        { input: 524288, expected: [524288] },
        { input: 1048576, expected: [1048576] },
        { input: 8388607, expected: [8388607] }, //2^23 - 1
        { input: 16777215, expected: [16777215] }, //2^24 - 1
        { input: 16777216, expected: [16777216] }, //2^24
        { input: 134217727, expected: [134217727] }, //2^27 - 1
        { input: 2147483647, expected: [2147483647] }, //2^31 - 1
      ]
    );
  });

g.test('abs_uint_hex')
  .uniqueId(0x59ff84968a839124)
  .desc(
    `
https://www.w3.org/TR/2021/WD-WGSL-20210831#integer-builtin-functions
scalar case, unsigned abs:
T is u32 or vecN<u32> abs(e: T ) -> T Result is e.
This is provided for symmetry with abs for signed integers.
Component-wise when T is a vector.
`
  )
  .params(u =>
    u
      .combineWithParams([
        { storageClass: 'storage', storageMode: 'read_write', access: 'read' },
      ] as const)
      .combine('containerType', ['scalar', 'vector'] as const)
      .combine('isAtomic', [false])
      .combine('baseType', ['u32'] as const)
      .beginSubcases()
      .expandWithParams(generateTypes)
  )
  .fn(async t => {
    assert(t.params._kTypeInfo !== undefined, 'generated type is undefined');
    runShaderTest(
      t,
      t.params.storageClass,
      t.params.storageMode,
      t.params.baseType,
      t.params.type,
      t.params._kTypeInfo.arrayLength,
      'abs',
      OperandType.Hex,
      [
        { input: 0xffffffff, expected: [0xffffffff] }, // -Inf f32
        { input: 0x477fe000, expected: [0x477fe000] }, // 65504 - largest positive f16
        { input: 0xc77fe000, expected: [0xc77fe000] }, // -65504 - largest negative f16
        { input: 0x3380346c, expected: [0x3380346c] }, // 0.0000000597 - smallest positive f16
      ]
    );
  });
