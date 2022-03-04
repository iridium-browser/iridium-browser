export const description = `WGSL execution test. Section: Integer built-in functions`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUTest } from '../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

g.test('integer_builtin_functions,unsigned_clamp')
  .uniqueId('386458e12e52645b')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#integer-builtin-functions')
  .desc(
    `
unsigned clamp:
T is u32 or vecN<u32> clamp(e1: T ,e2: T,e3: T) -> T Returns min(max(e1,e2),e3). Component-wise when T is a vector. (GLSLstd450UClamp)

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();

g.test('integer_builtin_functions,signed_clamp')
  .uniqueId('da51d3c8cc902ab2')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#integer-builtin-functions')
  .desc(
    `
signed clamp:
T is i32 or vecN<i32> clamp(e1: T ,e2: T,e3: T) -> T Returns min(max(e1,e2),e3). Component-wise when T is a vector. (GLSLstd450SClamp)

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();

g.test('integer_builtin_functions,count_1_bits')
  .uniqueId('259605bdcc180a4b')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#integer-builtin-functions')
  .desc(
    `
count 1 bits:
T is i32, u32, vecN<i32>, or vecN<u32> countOneBits(e: T ) -> T The number of 1 bits in the representation of e. Also known as "population count". Component-wise when T is a vector. (SPIR-V OpBitCount)

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();

g.test('integer_builtin_functions,unsigned_max')
  .uniqueId('2cce54f65e71b3a3')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#integer-builtin-functions')
  .desc(
    `
unsigned max:
T is u32 or vecN<u32> max(e1: T ,e2: T) -> T Returns e2 if e1 is less than e2, and e1 otherwise. Component-wise when T is a vector. (GLSLstd450UMax)

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();

g.test('integer_builtin_functions,signed_max')
  .uniqueId('ef8c37107946a69e')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#integer-builtin-functions')
  .desc(
    `
signed max:
T is i32 or vecN<i32> max(e1: T ,e2: T) -> T Returns e2 if e1 is less than e2, and e1 otherwise. Component-wise when T is a vector. (GLSLstd450SMax)

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();

g.test('integer_builtin_functions,bit_reversal')
  .uniqueId('8a7550f1097993f8')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#integer-builtin-functions')
  .desc(
    `
bit reversal:
T is i32, u32, vecN<i32>, or vecN<u32> reverseBits(e: T ) -> T Reverses the bits in e: The bit at position k of the result equals the bit at position 31-k of e. Component-wise when T is a vector. (SPIR-V OpBitReverse)

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();
