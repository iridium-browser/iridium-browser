export const description = `Validation tests for entry point user-defined IO`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { ShaderValidationTest } from '../shader_validation_test.js';

export const g = makeTestGroup(ShaderValidationTest);

// List of types to test against.
const kTestTypes = [
  { type: 'bool', _valid: false },
  { type: 'u32', _valid: true },
  { type: 'i32', _valid: true },
  { type: 'f32', _valid: true },
  { type: 'vec2<bool>', _valid: false },
  { type: 'vec2<u32>', _valid: true },
  { type: 'vec2<i32>', _valid: true },
  { type: 'vec2<f32>', _valid: true },
  { type: 'vec3<bool>', _valid: false },
  { type: 'vec3<u32>', _valid: true },
  { type: 'vec3<i32>', _valid: true },
  { type: 'vec3<f32>', _valid: true },
  { type: 'vec4<bool>', _valid: false },
  { type: 'vec4<u32>', _valid: true },
  { type: 'vec4<i32>', _valid: true },
  { type: 'vec4<f32>', _valid: true },
  { type: 'mat2x2<f32>', _valid: false },
  { type: 'mat2x3<f32>', _valid: false },
  { type: 'mat2x4<f32>', _valid: false },
  { type: 'mat3x2<f32>', _valid: false },
  { type: 'mat3x3<f32>', _valid: false },
  { type: 'mat3x4<f32>', _valid: false },
  { type: 'mat4x2<f32>', _valid: false },
  { type: 'mat4x3<f32>', _valid: false },
  { type: 'mat4x4<f32>', _valid: false },
  { type: 'atomic<u32>', _valid: false },
  { type: 'atomic<i32>', _valid: false },
  { type: 'array<bool,4>', _valid: false },
  { type: 'array<u32,4>', _valid: false },
  { type: 'array<i32,4>', _valid: false },
  { type: 'array<f32,4>', _valid: false },
  { type: 'MyStruct', _valid: false },
] as const;

/**
 * Generate an entry point that uses a user-defined IO variable.
 *
 * @param attribute The attribute to use for the user-defined IO.
 * @param type The type to use for the user-defined IO.
 * @param stage The shader stage.
 * @param io An "in|out" string specifying whether the user-defined IO is an input or an output.
 * @param use_struct True to wrap the user-defined IO in a struct.
 * @returns The generated shader code.
 */
function generateShader({
  attribute,
  type,
  stage,
  io,
  use_struct,
}: {
  attribute: string;
  type: string;
  stage: string;
  io: string;
  use_struct: boolean;
}) {
  let code = '';

  if (use_struct) {
    // Generate a struct that wraps the location attribute variable.
    code += 'struct S {\n';
    code += `  ${attribute} value : ${type};\n`;
    if (stage === 'vertex' && io === 'out') {
      // Add position builtin for vertex outputs.
      code += `  [[builtin(position)]] position : vec4<f32>;\n`;
    }
    code += '};\n\n';
  }

  if (stage !== '') {
    // Generate the entry point attributes.
    code += `[[stage(${stage})]]`;
    if (stage === 'compute') {
      code += ' [[workgroup_size(1)]]';
    }
  }

  // Generate the entry point parameter and return type.
  let param = '';
  let retType = '';
  let retVal = '';
  if (io === 'in') {
    if (use_struct) {
      param = `in : S`;
    } else {
      param = `${attribute} value : ${type}`;
    }

    // Vertex shaders must always return `builtin(position)`.
    if (stage === 'vertex') {
      retType = `-> [[builtin(position)]] vec4<f32>`;
      retVal = `return vec4<f32>();`;
    }
  } else if (io === 'out') {
    if (use_struct) {
      retType = '-> S';
      retVal = `return S();`;
    } else {
      retType = `-> ${attribute} ${type}`;
      retVal = `return ${type}();`;
    }
  }

  code += `
    fn main(${param}) ${retType} {
      ${retVal}
    }
  `;

  return code;
}

g.test('stage_inout')
  .desc(`Test validation of user-defined IO stage and in/out usage`)
  .params(u =>
    u
      .combine('use_struct', [true, false] as const)
      .combine('target_stage', ['vertex', 'fragment', 'compute'] as const)
      .combine('target_io', ['in', 'out'] as const)
      .beginSubcases()
  )
  .fn(t => {
    const code = generateShader({
      attribute: '[[location(0)]]',
      type: 'f32',
      stage: t.params.target_stage,
      io: t.params.target_io,
      use_struct: t.params.use_struct,
    });

    // Expect to fail for compute shaders or when used as a non-struct vertex output (since the
    // position built-in must also be specified).
    const expectation =
      t.params.target_stage === 'fragment' ||
      (t.params.target_stage === 'vertex' && (t.params.target_io === 'in' || t.params.use_struct));
    t.expectCompileResult(expectation, code);
  });

g.test('type')
  .desc(`Test validation of user-defined IO types`)
  .params(u =>
    u
      .combine('use_struct', [true, false] as const)
      .combineWithParams(kTestTypes)
      .beginSubcases()
  )
  .fn(t => {
    let code = '';

    if (t.params.type === 'MyStruct') {
      // Generate a struct that contains a valid type.
      code += 'struct MyStruct {\n';
      code += `  value : f32;\n`;
      code += '};\n\n';
    }

    code += generateShader({
      attribute: '[[location(0)]]',
      type: t.params.type,
      stage: 'fragment',
      io: 'in',
      use_struct: t.params.use_struct,
    });

    // Expect to pass iff a valid type is used.
    t.expectCompileResult(t.params._valid, code);
  });

g.test('nesting')
  .desc(`Test validation of nested user-defined IO`)
  .params(u =>
    u
      .combine('target_stage', ['vertex', 'fragment', ''] as const)
      .combine('target_io', ['in', 'out'] as const)
      .beginSubcases()
  )
  .fn(t => {
    let code = '';

    // Generate a struct that contains a valid type.
    code += 'struct Inner {\n';
    code += `  [[location(0)]] value : f32;\n`;
    code += '};\n\n';
    code += 'struct Outer {\n';
    code += `  inner : Inner;\n`;
    code += '};\n\n';

    code += generateShader({
      attribute: '',
      type: 'Outer',
      stage: t.params.target_stage,
      io: t.params.target_io,
      use_struct: false,
    });

    // Expect to fail pass only if the struct is not used for entry point IO.
    t.expectCompileResult(t.params.target_stage === '', code);
  });

g.test('duplicates')
  .desc(`Test validation of duplicate user-defined IO attributes`)
  .unimplemented();
