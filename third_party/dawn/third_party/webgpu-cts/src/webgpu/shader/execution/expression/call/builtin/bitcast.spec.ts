export const description = `
Execution tests for the 'bitcast' builtin function

@const @must_use fn bitcast<T>(e: T ) -> T
T is concrete numeric scalar or concerete numeric vector
Identity function.

@const @must_use fn bitcast<T>(e: S ) -> T
@const @must_use fn bitcast<vecN<T>>(e: vecN<S> ) -> vecN<T>
S is i32, u32, f32
T is i32, u32, f32, and T is not S
Reinterpretation of bits.  Beware non-normal f32 values.

@const @must_use fn bitcast<T>(e: vec2<f16> ) -> T
@const @must_use fn bitcast<vec2<T>>(e: vec4<f16> ) -> vec2<T>
@const @must_use fn bitcast<vec2<f16>>(e: T ) -> vec2<f16>
@const @must_use fn bitcast<vec4<f16>>(e: vec2<T> ) -> vec4<f16>
T is i32, u32, f32
`;

import { TestParams } from '../../../../../../common/framework/fixture.js';
import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../../gpu_test.js';
import { i32, u32, Type, TypeI32, TypeU32 } from '../../../../../util/conversion.js';
import { fullI32Range, fullU32Range } from '../../../../../util/math.js';
import { makeCaseCache } from '../../case_cache.js';
import { allInputSources, run, CaseList, InputSource, ShaderBuilder } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

const TODO_CASES: CaseList = [];
export const d = makeCaseCache('bitcast', {
  // Identity cases
  i32_to_i32: () => fullI32Range().map(e => ({ input: i32(e), expected: i32(e) })),
  u32_to_u32: () => fullU32Range().map(e => ({ input: u32(e), expected: u32(e) })),
  f32_to_f32: () => TODO_CASES,
  // i32,u32,f32 to different i32,u32,f32
  i32_to_u32: () => fullI32Range().map(e => ({ input: i32(e), expected: u32(e) })),
  i32_to_f32: () => TODO_CASES,
  u32_to_i32: () => fullU32Range().map(e => ({ input: u32(e), expected: i32(e) })),
  u32_to_f32: () => TODO_CASES,
  f32_to_i32: () => TODO_CASES,
  f32_to_u32: () => TODO_CASES,
});

/**
 * @returns a ShaderBuilder that generates a call to bitcast,
 * using appropriate destination type, which optionally can be
 * a WGSL type alias.
 */
function bitcastBuilder(canonicalDestType: string, params: TestParams): ShaderBuilder {
  const destType = params.vectorize
    ? `vec${params.vectorize}<${canonicalDestType}>`
    : canonicalDestType;

  if (params.alias) {
    return (
      parameterTypes: Array<Type>,
      resultType: Type,
      cases: CaseList,
      inputSource: InputSource
    ) =>
      `alias myalias = ${destType};\n` +
      builtin(`bitcast<myalias>`)(parameterTypes, resultType, cases, inputSource);
  }
  return builtin(`bitcast<${destType}>`);
}

// Identity cases
g.test('i32_to_i32')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast i32 to i32 tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .fn(async t => {
    const cases = await d.get('i32_to_i32');
    await run(t, bitcastBuilder('i32', t.params), [TypeI32], TypeI32, t.params, cases);
  });

g.test('u32_to_u32')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast u32 to u32 tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .fn(async t => {
    const cases = await d.get('u32_to_u32');
    await run(t, bitcastBuilder('u32', t.params), [TypeU32], TypeU32, t.params, cases);
  });

g.test('f32_to_f32')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast f32 to f32 tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

// To i32 from u32, f32
g.test('u32_to_i32')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast u32 to i32 tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .fn(async t => {
    const cases = await d.get('u32_to_i32');
    await run(t, bitcastBuilder('i32', t.params), [TypeU32], TypeI32, t.params, cases);
  });

g.test('f32_to_i32')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast f32 to i32 tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

// To u32 from i32, f32
g.test('i32_to_u32')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast i32 to u32 tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .fn(async t => {
    const cases = await d.get('i32_to_u32');
    await run(t, bitcastBuilder('u32', t.params), [TypeI32], TypeU32, t.params, cases);
  });

g.test('f32_to_u32')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast f32 to i32 tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

// To f32 from i32, u32
g.test('i32_to_f32')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast i32 to f32 tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

g.test('u32_to_f32')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast u32 to f32 tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

// 16 bit types

// f16 cases

// f16: Identity
g.test('f16_to_f16')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast f16 to f16 tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

// f16: 32-bit scalar numeric to vec2<f16>
g.test('i32_to_vec2h')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast i32 to vec2h tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

g.test('u32_to_vec2h')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast u32 to vec2h tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

g.test('f32_to_vec2h')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast u32 to vec2h tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

// f16: vec2<32-bit scalar numeric> to vec4<f16>
g.test('vec2i_to_vec4h')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast vec2i to vec4h tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

g.test('vec2u_to_vec4h')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast vec2u to vec4h tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

g.test('vec2f_to_vec4h')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast vec2f to vec2h tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

// f16: vec2<f16> to 32-bit scalar numeric
g.test('vec2h_to_i32')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast vec2h to i32 tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

g.test('vec2h_to_u32')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast vec2h to u32 tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

g.test('vec2h_to_f32')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast vec2h to f32 tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

// f16: vec4<f16> to vec2<32-bit scalar numeric>
g.test('vec4h_to_vec2i')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast vec4h to vec2i tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

g.test('vec4h_to_vec2u')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast vec4h to vec2u tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();

g.test('vec4h_to_vec2f')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast vec4h to vec2f tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .unimplemented();
