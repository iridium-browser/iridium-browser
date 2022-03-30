export const description = `WGSL execution test. Section: Integer built-in functions`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUTest } from '../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

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
