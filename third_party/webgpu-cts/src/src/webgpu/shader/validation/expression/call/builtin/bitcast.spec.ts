export const description = `
Validation negative tests for bitcast builtins.
`;

import { makeTestGroup } from '../../../../../../common/framework/test_group.js';
import { kBit } from '../../../../../util/constants.js';
import { linearRange } from '../../../../../util/math.js';
import { ShaderValidationTest } from '../../../shader_validation_test.js';

export const g = makeTestGroup(ShaderValidationTest);

// A VectorCase specfies the number of components a vector type has,
// and which component will have a bad value.
// Use width = 1 to indicate a scalar.
type VectorCase = { width: number; badIndex: number };
const vectorCases: VectorCase[] = [
  { width: 1, badIndex: 0 },
  { width: 2, badIndex: 0 },
  { width: 2, badIndex: 1 },
  { width: 3, badIndex: 0 },
  { width: 3, badIndex: 1 },
  { width: 3, badIndex: 2 },
  { width: 4, badIndex: 0 },
  { width: 4, badIndex: 1 },
  { width: 4, badIndex: 2 },
  { width: 4, badIndex: 3 },
];

const numNaNs = 4;
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

g.test('bad_const_to_f32')
  .specURL('https://www.w3.org/TR/WGSL/#floating-point-evaluation')
  .desc(
    `
It is a shader-creation error if any const-expression of floating-point type evaluates to NaN or infinity.
`
  )
  .params(u =>
    u
      .combine('fromScalarType', ['i32', 'u32'] as const)
      .combine('vectorize', [...vectorCases] as const)
      .combine('bitBadValue', [...f32InfAndNaNInU32] as const)
  )
  .fn(t => {
    // For scalar cases, generate code like:
    //  const f = bitcast<f32>(i32(u32(0x7f800000)));
    // For vector cases, generate code where one component is bad. In this case
    // width=4 and badIndex=2
    //  const f = bitcast<vec4f>(vec4<32>(0,0,i32(u32(0x7f800000)),0));
    const width = t.params.vectorize.width;
    const badIndex = t.params.vectorize.badIndex;
    const badScalar = `${t.params.fromScalarType}(u32(${t.params.bitBadValue}))`;
    const destType = width === 1 ? 'f32' : `vec${width}f`;
    const srcType =
      width === 1 ? t.params.fromScalarType : `vec${width}<${t.params.fromScalarType}>`;
    const components = [...Array(width).keys()]
      .map(i => (i === badIndex ? badScalar : '0'))
      .join(',');
    const code = `const f = bitcast<${destType}>(${srcType}(${components}));`;
    t.expectCompileResult(false, code);
  });

const f32_matrix_types = [2, 3, 4]
  .map(i => [2, 3, 4].map(j => `mat${i}x${j}f`))
  .reduce((a, c) => a.concat(c), []);
const bool_types = ['bool', ...[2, 3, 4].map(i => `vec${i}<bool>`)];

g.test('bad_type_constructible')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(
    `
Bitcast only applies to concrete numeric scalar or concrete numeric vector.
Test constructible types.
`
  )
  .params(u =>
    u
      .combine('type', [...f32_matrix_types, ...bool_types, 'array<i32,2>', 'S'])
      .combine('direction', ['to', 'from'])
  )
  .fn(t => {
    const T = t.params.type;
    const preamble = T === 'S' ? 'struct S { a:i32 } ' : '';
    // Create a value of type T using zero-construction: T().
    const srcVal = t.params.direction === 'to' ? '0' : `${T}()`;
    const destType = t.params.direction === 'to' ? T : 'i32';
    const code = preamble + `const x = bitcast<${destType}>(${srcVal});`;
    t.expectCompileResult(false, code);
  });

g.test('bad_type_nonconstructible')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(
    `
Bitcast only applies to concrete numeric scalar or concrete numeric vector.
Test non-constructible types.
`
  )
  .params(u => u.combine('var', ['s', 't', 'b', 'p']).combine('direction', ['to', 'from']))
  .fn(t => {
    const typeOf: Record<string, string> = {
      s: 'sampler',
      t: 'texture_depth_2d',
      b: 'array<i32>',
      p: 'ptr<private,i32>',
    };
    const srcVal = t.params.direction === 'to' ? '0' : t.params.var;
    const destType = t.params.direction === 'to' ? typeOf[t.params.var] : 'i32';
    const code = `
    @group(0) @binding(0) var s: sampler;
    @group(0) @binding(1) var t: texture_depth_2d;
    @group(0) @binding(2) var<storage> b: array<i32>;
    var<private> v: i32;
    @compute @workgroup_size(1)
    fn main() {
      let p = &v;
      let x = bitcast<${destType}>(${srcVal});
    }
    `;
    t.expectCompileResult(false, code);
  });

g.test('bad_to_vec3h')
  .specURL('https://www.w3.org/TR/WGSL/#bitcast-builtin')
  .desc(
    `
Can't cast numeric type to vec3<f16> because it is 48 bits wide
and no other type is that size.
`
  )
  .unimplemented();
