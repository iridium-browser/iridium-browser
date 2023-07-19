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
import { Comparator, alwaysPass, anyOf } from '../../../../../util/compare.js';
import { kBit, kValue } from '../../../../../util/constants.js';
import {
  reinterpretI32AsF32,
  reinterpretF32AsI32,
  reinterpretF32AsU32,
  reinterpretU32AsF32,
  reinterpretU32AsI32,
  f32,
  i32,
  u32,
  Type,
  TypeF32,
  TypeI32,
  TypeU32,
} from '../../../../../util/conversion.js';
import {
  fullF32Range,
  fullI32Range,
  fullU32Range,
  linearRange,
  isSubnormalNumberF32,
} from '../../../../../util/math.js';
import { makeCaseCache } from '../../case_cache.js';
import { allInputSources, run, CaseList, InputSource, ShaderBuilder } from '../../expression.js';

import { builtin } from './builtin.js';

export const g = makeTestGroup(GPUTest);

const numNaNs = 11;
const f32InfAndNaNInU32: number[] = [
  // Cover NaNs evenly in integer space.
  // The positive NaN with the lowest integer representation is the integer
  // for infinity, plus one.
  // The positive NaN with the highest integer representation is i32.max (!)
  ...linearRange(kBit.f32.infinity.positive + 1, kBit.i32.positive.max, numNaNs),
  // The negative NaN with the lowest integer representation is the integer
  // for negative infinity, plus one.
  // The negative NaN with the highest integer representation is u32.max (!)
  ...linearRange(kBit.f32.infinity.negative + 1, kBit.u32.max, numNaNs),
  kBit.f32.infinity.positive,
  kBit.f32.infinity.negative,
];
const f32InfAndNaNInF32 = f32InfAndNaNInU32.map(u => reinterpretU32AsF32(u));
const f32InfAndNaNInI32 = f32InfAndNaNInU32.map(u => reinterpretU32AsI32(u));

const f32ZerosInU32 = [0, kBit.f32.negative.zero];
const f32ZerosInF32 = f32ZerosInU32.map(u => reinterpretU32AsF32(u));
const f32ZerosInI32 = f32ZerosInU32.map(u => reinterpretU32AsI32(u));

// f32FiniteRange is a list of finite f32s. fullF32Range() already
// has +0, we only need to add -0.
const f32FiniteRange: number[] = [...fullF32Range(), kValue.f32.negative.zero];
const f32RangeWithInfAndNaN: number[] = [...f32FiniteRange, ...f32InfAndNaNInF32];

const anyF32 = alwaysPass('any f32');
const anyI32 = alwaysPass('any i32');
const anyU32 = alwaysPass('any u32');

const i32RangeWithBitsForInfAndNaNAndZeros: number[] = [
  ...fullI32Range(),
  ...f32ZerosInI32,
  ...f32InfAndNaNInI32,
];
const i32RangeWithFiniteF32: number[] = i32RangeWithBitsForInfAndNaNAndZeros.filter(i =>
  isFinite(reinterpretI32AsF32(i))
);

const u32RangeWithBitsForInfAndNaNAndZeros: number[] = [
  ...fullU32Range(),
  ...f32ZerosInU32,
  ...f32InfAndNaNInU32,
];
const u32RangeWithFiniteF32: number[] = u32RangeWithBitsForInfAndNaNAndZeros.filter(u =>
  isFinite(reinterpretU32AsF32(u))
);

function isFinite(f: number): boolean {
  return !(Number.isNaN(f) || f === Number.POSITIVE_INFINITY || f === Number.NEGATIVE_INFINITY);
}

/**
 * @returns a Comparator for checking if a f32 value is a valid
 * bitcast conversion from f32.
 */
function bitcastF32ToF32Comparator(f: number): Comparator {
  if (!isFinite(f)) return anyF32;
  const acceptable: number[] = [f, ...(isSubnormalNumberF32(f) ? f32ZerosInF32 : [])];
  return anyOf(...acceptable.map(f32));
}

/**
 * @returns a Comparator for checking if a u32 value is a valid
 * bitcast conversion from f32.
 */
function bitcastF32ToU32Comparator(f: number): Comparator {
  if (!isFinite(f)) return anyU32;
  const acceptable: number[] = [
    reinterpretF32AsU32(f),
    ...(isSubnormalNumberF32(f) ? f32ZerosInU32 : []),
  ];
  return anyOf(...acceptable.map(u32));
}

/**
 * @returns a Comparator for checking if a i32 value is a valid
 * bitcast conversion from f32.
 */
function bitcastF32ToI32Comparator(f: number): Comparator {
  if (!isFinite(f)) return anyI32;
  const acceptable: number[] = [
    reinterpretF32AsI32(f),
    ...(isSubnormalNumberF32(f) ? f32ZerosInI32 : []),
  ];
  return anyOf(...acceptable.map(i32));
}

/**
 * @returns a Comparator for checking if a f32 value is a valid
 * bitcast conversion from i32.
 */
function bitcastI32ToF32Comparator(i: number): Comparator {
  const f: number = reinterpretI32AsF32(i);
  if (!isFinite(f)) return anyI32;
  // Positive or negative zero bit pattern map to any zero.
  if (f32ZerosInI32.includes(i)) return anyOf(...f32ZerosInF32.map(f32));
  const acceptable: number[] = [f, ...(isSubnormalNumberF32(f) ? f32ZerosInF32 : [])];
  return anyOf(...acceptable.map(f32));
}

/**
 * @returns a Comparator for checking if a f32 value is a valid
 * bitcast conversion from u32.
 */
function bitcastU32ToF32Comparator(u: number): Comparator {
  const f: number = reinterpretU32AsF32(u);
  if (!isFinite(f)) return anyU32;
  // Positive or negative zero bit pattern map to any zero.
  if (f32ZerosInU32.includes(u)) return anyOf(...f32ZerosInF32.map(f32));
  const acceptable: number[] = [f, ...(isSubnormalNumberF32(f) ? f32ZerosInF32 : [])];
  return anyOf(...acceptable.map(f32));
}

export const d = makeCaseCache('bitcast', {
  // Identity Cases
  i32_to_i32: () => fullI32Range().map(e => ({ input: i32(e), expected: i32(e) })),
  u32_to_u32: () => fullU32Range().map(e => ({ input: u32(e), expected: u32(e) })),
  f32_inf_nan_to_f32: () =>
    f32RangeWithInfAndNaN.map(e => ({
      input: f32(e),
      expected: bitcastF32ToF32Comparator(e),
    })),
  f32_to_f32: () =>
    f32FiniteRange.map(e => ({ input: f32(e), expected: bitcastF32ToF32Comparator(e) })),

  // i32,u32,f32 to different i32,u32,f32
  i32_to_u32: () => fullI32Range().map(e => ({ input: i32(e), expected: u32(e) })),
  i32_to_f32: () =>
    i32RangeWithFiniteF32.map(e => ({
      input: i32(e),
      expected: bitcastI32ToF32Comparator(e),
    })),
  i32_to_f32_inf_nan: () =>
    i32RangeWithBitsForInfAndNaNAndZeros.map(e => ({
      input: i32(e),
      expected: bitcastI32ToF32Comparator(e),
    })),
  u32_to_i32: () => fullU32Range().map(e => ({ input: u32(e), expected: i32(e) })),
  u32_to_f32: () =>
    u32RangeWithFiniteF32.map(e => ({
      input: u32(e),
      expected: bitcastU32ToF32Comparator(e),
    })),
  u32_to_f32_inf_nan: () =>
    u32RangeWithBitsForInfAndNaNAndZeros.map(e => ({
      input: u32(e),
      expected: bitcastU32ToF32Comparator(e),
    })),
  f32_inf_nan_to_i32: () =>
    f32RangeWithInfAndNaN.map(e => ({
      input: f32(e),
      expected: bitcastF32ToI32Comparator(e),
    })),
  f32_to_i32: () =>
    f32FiniteRange.map(e => ({ input: f32(e), expected: bitcastF32ToI32Comparator(e) })),

  f32_inf_nan_to_u32: () =>
    f32RangeWithInfAndNaN.map(e => ({
      input: f32(e),
      expected: bitcastF32ToU32Comparator(e),
    })),
  f32_to_u32: () =>
    f32FiniteRange.map(e => ({ input: f32(e), expected: bitcastF32ToU32Comparator(e) })),
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
  .fn(async t => {
    const cases = await d.get(
      // Infinities and NaNs are errors in const-eval.
      t.params.inputSource === 'const' ? 'f32_to_f32' : 'f32_inf_nan_to_f32'
    );
    await run(t, bitcastBuilder('f32', t.params), [TypeF32], TypeF32, t.params, cases);
  });

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
  .fn(async t => {
    const cases = await d.get(
      // Infinities and NaNs are errors in const-eval.
      t.params.inputSource === 'const' ? 'f32_to_i32' : 'f32_inf_nan_to_i32'
    );
    await run(t, bitcastBuilder('i32', t.params), [TypeF32], TypeI32, t.params, cases);
  });

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
  .fn(async t => {
    const cases = await d.get(
      // Infinities and NaNs are errors in const-eval.
      t.params.inputSource === 'const' ? 'f32_to_u32' : 'f32_inf_nan_to_u32'
    );
    await run(t, bitcastBuilder('u32', t.params), [TypeF32], TypeU32, t.params, cases);
  });

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
  .fn(async t => {
    const cases = await d.get(
      // Infinities and NaNs are errors in const-eval.
      t.params.inputSource === 'const' ? 'i32_to_f32' : 'i32_to_f32_inf_nan'
    );
    await run(t, bitcastBuilder('f32', t.params), [TypeI32], TypeF32, t.params, cases);
  });

g.test('u32_to_f32')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(`bitcast u32 to f32 tests`)
  .params(u =>
    u
      .combine('inputSource', allInputSources)
      .combine('vectorize', [undefined, 2, 3, 4] as const)
      .combine('alias', [false, true])
  )
  .fn(async t => {
    const cases = await d.get(
      // Infinities and NaNs are errors in const-eval.
      t.params.inputSource === 'const' ? 'u32_to_f32' : 'u32_to_f32_inf_nan'
    );
    await run(t, bitcastBuilder('f32', t.params), [TypeU32], TypeF32, t.params, cases);
  });

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
