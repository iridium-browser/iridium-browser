export const description = `
Execution tests for the 'modf' builtin function

T is f32 or f16
@const fn modf(e:T) -> result_struct
Splits |e| into fractional and whole number parts.
The whole part is (|e| % 1.0), and the fractional part is |e| minus the whole part.
Returns the result_struct for the given type.

S is f32 or f16
T is vecN<S>
@const fn modf(e:T) -> result_struct
Splits the components of |e| into fractional and whole number parts.
The |i|'th component of the whole and fractional parts equal the whole and fractional parts of modf(e[i]).
Returns the result_struct for the given type.

`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { allInputSources } from '../../expression.js';

export const g = makeTestGroup(GPUTest);

g.test('scalar_f32')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(
    `
T is f32

struct __modf_result {
  fract : f32, // fractional part
  whole : f32  // whole part
}
`
  )
  .params(u => u.combine('inputSource', allInputSources))
  .unimplemented();

g.test('scalar_f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(
    `
T is f16

struct __modf_result_f16 {
  fract : f16, // fractional part
  whole : f16  // whole part
}
`
  )
  .params(u => u.combine('inputSource', allInputSources))
  .unimplemented();

g.test('vector_f32')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(
    `
T is vecN<f32>

struct __modf_result_vecN {
  fract : vecN<f32>, // fractional part
  whole : vecN<f32>  // whole part
}
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('vectorize', [2, 3, 4] as const))
  .unimplemented();

g.test('vector_f16')
  .specURL('https://www.w3.org/TR/WGSL/#float-builtin-functions')
  .desc(
    `
T is vecN<f16>

struct __modf_result_vecN_f16 {
  fract : vecN<f16>, // fractional part
  whole : vecN<f16>  // whole part
}
`
  )
  .params(u => u.combine('inputSource', allInputSources).combine('vectorize', [2, 3, 4] as const))
  .unimplemented();
