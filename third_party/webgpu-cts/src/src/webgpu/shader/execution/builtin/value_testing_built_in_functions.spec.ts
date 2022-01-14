export const description = `WGSL execution test. Section: Value-testing built-in functions`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUTest } from '../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

g.test('value_testing_builtin_functions,isNan')
  .uniqueId('fdd1e7105af70b74')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#value-testing-builtin-functions')
  .desc(
    `
isNan:
I is f32 or vecN<f32> T is bool if I is a scalar, or vecN<bool> if I is a vector isNan(e: I ) -> T Test for NaN according to IEEE-754. Component-wise when I is a vector. (OpIsNan)

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();

g.test('value_testing_builtin_functions,isInf')
  .uniqueId('3591ae3f3daa3871')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#value-testing-builtin-functions')
  .desc(
    `
isInf:
isInf(e: I ) -> T Test for infinity according to IEEE-754. Component-wise when I is a vector. (OpIsInf)

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();

g.test('value_testing_builtin_functions,isFinite')
  .uniqueId('bf8ee3764330ceb4')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#value-testing-builtin-functions')
  .desc(
    `
isFinite:
isFinite(e: I ) -> T Test a finite value according to IEEE-754. Component-wise when I is a vector.

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();

g.test('value_testing_builtin_functions,isNormal')
  .uniqueId('ea51009a88a27a15')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#value-testing-builtin-functions')
  .desc(
    `
isNormal:
isNormal(e: I ) -> T Test a normal value according to IEEE-754. Component-wise when I is a vector.

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();

g.test('value_testing_builtin_functions,runtime_sized_array_length')
  .uniqueId('8089b54fa4eeaa0b')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#value-testing-builtin-functions')
  .desc(
    `
runtime-sized array length:
e: ptr<storage,array<T>> arrayLength(e): u32 Returns the number of elements in the runtime-sized array. (OpArrayLength, but the implementation has to trace back to get the pointer to the enclosing struct.)

Please read the following guidelines before contributing:
https://github.com/gpuweb/cts/blob/main/docs/plan_autogen.md
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();
