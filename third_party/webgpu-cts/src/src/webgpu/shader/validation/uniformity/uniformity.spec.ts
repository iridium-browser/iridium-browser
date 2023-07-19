export const description = `Validation tests for uniformity analysis`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { keysOf } from '../../../../common/util/data_tables.js';
import { unreachable } from '../../../../common/util/util.js';
import { ShaderValidationTest } from '../shader_validation_test.js';

export const g = makeTestGroup(ShaderValidationTest);

const kCollectiveOps = [
  { op: 'textureSample', stage: 'fragment' },
  { op: 'textureSampleBias', stage: 'fragment' },
  { op: 'textureSampleCompare', stage: 'fragment' },
  { op: 'dpdx', stage: 'fragment' },
  { op: 'dpdxCoarse', stage: 'fragment' },
  { op: 'dpdxFine', stage: 'fragment' },
  { op: 'dpdy', stage: 'fragment' },
  { op: 'dpdyCoarse', stage: 'fragment' },
  { op: 'dpdyFine', stage: 'fragment' },
  { op: 'fwidth', stage: 'fragment' },
  { op: 'fwidthCoarse', stage: 'fragment' },
  { op: 'fwidthFine', stage: 'fragment' },
  { op: 'storageBarrier', stage: 'compute' },
  { op: 'workgroupBarrier', stage: 'compute' },
  { op: 'workgroupUniformLoad', stage: 'compute' },
];

const kConditions = [
  { cond: 'uniform_storage_ro', expectation: true },
  { cond: 'nonuniform_storage_ro', expectation: false },
  { cond: 'nonuniform_storage_rw', expectation: false },
  { cond: 'nonuniform_builtin', expectation: false },
  { cond: 'uniform_literal', expectation: true },
  { cond: 'uniform_const', expectation: true },
  { cond: 'uniform_override', expectation: true },
  { cond: 'uniform_let', expectation: true },
  { cond: 'nonuniform_let', expectation: false },
  { cond: 'uniform_or', expectation: true },
  { cond: 'nonuniform_or1', expectation: false },
  { cond: 'nonuniform_or2', expectation: false },
  { cond: 'uniform_and', expectation: true },
  { cond: 'nonuniform_and1', expectation: false },
  { cond: 'nonuniform_and2', expectation: false },
  { cond: 'uniform_func_var', expectation: true },
  { cond: 'nonuniform_func_var', expectation: false },
];

function generateCondition(condition: string): string {
  switch (condition) {
    case 'uniform_storage_ro': {
      return `ro_buffer[0] == 0`;
    }
    case 'nonuniform_storage_ro': {
      return `ro_buffer[priv_var[0]] == 0`;
    }
    case 'nonuniform_storage_rw': {
      return `rw_buffer[0] == 0`;
    }
    case 'nonuniform_builtin': {
      return `p.x == 0`;
    }
    case 'uniform_literal': {
      return `false`;
    }
    case 'uniform_const': {
      return `c`;
    }
    case 'uniform_override': {
      return `o == 0`;
    }
    case 'uniform_let': {
      return `u_let == 0`;
    }
    case 'nonuniform_let': {
      return `n_let == 0`;
    }
    case 'uniform_or': {
      return `u_let == 0 || uniform_buffer.y > 1`;
    }
    case 'nonuniform_or1': {
      return `u_let == 0 || n_let == 0`;
    }
    case 'nonuniform_or2': {
      return `n_let == 0 || u_let == 0`;
    }
    case 'uniform_and': {
      return `u_let == 0 && uniform_buffer.y > 1`;
    }
    case 'nonuniform_and1': {
      return `u_let == 0 && n_let == 0`;
    }
    case 'nonuniform_and2': {
      return `n_let == 0 && u_let == 0`;
    }
    case 'uniform_func_var': {
      return `u_f == 0`;
    }
    case 'nonuniform_func_var': {
      return `n_f == 0`;
    }
    default: {
      unreachable(`Unhandled condition`);
    }
  }
}

function generateOp(op: string): string {
  switch (op) {
    case 'textureSample': {
      return `let x = ${op}(tex, s, vec2(0,0));\n`;
    }
    case 'textureSampleBias': {
      return `let x = ${op}(tex, s, vec2(0,0), 0);\n`;
    }
    case 'textureSampleCompare': {
      return `let x = ${op}(tex_depth, s_comp, vec2(0,0), 0);\n`;
    }
    case 'storageBarrier':
    case 'workgroupBarrier': {
      return `${op}();\n`;
    }
    case 'workgroupUniformLoad': {
      return `let x = ${op}(&wg);`;
    }
    case 'dpdx':
    case 'dpdxCoarse':
    case 'dpdxFine':
    case 'dpdy':
    case 'dpdyCoarse':
    case 'dpdyFine':
    case 'fwidth':
    case 'fwidthCoarse':
    case 'fwidthFine': {
      return `let x = ${op}(0);\n`;
    }
    default: {
      unreachable(`Unhandled op`);
    }
  }
}

function generateConditionalStatement(statement: string, condition: string, op: string): string {
  const code = ``;
  switch (statement) {
    case 'if': {
      return `if ${generateCondition(condition)} {
        ${generateOp(op)};
      }
      `;
    }
    case 'for': {
      return `for (; ${generateCondition(condition)};) {
        ${generateOp(op)};
      }
      `;
    }
    case 'while': {
      return `while ${generateCondition(condition)} {
        ${generateOp(op)};
      }
      `;
    }
    case 'switch': {
      return `switch u32(${generateCondition(condition)}) {
        case 0: {
          ${generateOp(op)};
        }
        default: { }
      }
      `;
    }
    default: {
      unreachable(`Unhandled statement`);
    }
  }

  return code;
}

g.test('basics')
  .desc(`Test collective operations in simple uniform or non-uniform control flow.`)
  .params(u =>
    u
      .combineWithParams(kCollectiveOps)
      .combineWithParams(kConditions)
      .combine('statement', ['if', 'for', 'while', 'switch'] as const)
      .beginSubcases()
  )
  .fn(t => {
    let code = `
 @group(0) @binding(0) var s : sampler;
 @group(0) @binding(1) var s_comp : sampler_comparison;
 @group(0) @binding(2) var tex : texture_2d<f32>;
 @group(0) @binding(3) var tex_depth : texture_depth_2d;

 @group(1) @binding(0) var<storage, read> ro_buffer : array<f32, 4>;
 @group(1) @binding(1) var<storage, read_write> rw_buffer : array<f32, 4>;
 @group(1) @binding(2) var<uniform> uniform_buffer : vec4<f32>;

 var<private> priv_var : array<f32, 4> = array(0,0,0,0);

 const c = false;
 override o : f32;
`;

    if (t.params.stage === 'compute') {
      code += `var<workgroup> wg : f32;\n`;
      code += ` @workgroup_size(16, 1, 1)`;
    }
    code += `@${t.params.stage}`;
    code += `\nfn main(`;
    if (t.params.stage === 'compute') {
      code += `@builtin(global_invocation_id) p : vec3<u32>`;
    } else {
      code += `@builtin(position) p : vec4<f32>`;
    }
    code += `) {
      let u_let = uniform_buffer.x;
      let n_let = rw_buffer[0];
      var u_f = uniform_buffer.z;
      var n_f = rw_buffer[1];
    `;

    // Simple control statement containing the op.
    code += generateConditionalStatement(t.params.statement, t.params.cond, t.params.op);

    code += `\n}\n`;

    t.expectCompileResult(t.params.expectation, code);
  });

const kFragmentBuiltinValues = [
  {
    builtin: `position`,
    type: `vec4<f32>`,
  },
  {
    builtin: `front_facing`,
    type: `bool`,
  },
  {
    builtin: `sample_index`,
    type: `u32`,
  },
  {
    builtin: `sample_mask`,
    type: `u32`,
  },
];

g.test('fragment_builtin_values')
  .desc(`Test uniformity of fragment built-in values`)
  .params(u => u.combineWithParams(kFragmentBuiltinValues).beginSubcases())
  .fn(t => {
    let cond = ``;
    switch (t.params.type) {
      case `u32`:
      case `i32`:
      case `f32`: {
        cond = `p > 0`;
        break;
      }
      case `vec4<u32>`:
      case `vec4<i32>`:
      case `vec4<f32>`: {
        cond = `p.x > 0`;
        break;
      }
      case `bool`: {
        cond = `p`;
        break;
      }
      default: {
        unreachable(`Unhandled type`);
      }
    }
    const code = `
@group(0) @binding(0) var s : sampler;
@group(0) @binding(1) var tex : texture_2d<f32>;

@fragment
fn main(@builtin(${t.params.builtin}) p : ${t.params.type}) {
  if ${cond} {
    let texel = textureSample(tex, s, vec2<f32>(0,0));
  }
}
`;

    t.expectCompileResult(true, `diagnostic(off, derivative_uniformity);\n` + code);
    t.expectCompileResult(false, code);
  });

const kComputeBuiltinValues = [
  {
    builtin: `local_invocation_id`,
    type: `vec3<f32>`,
    uniform: false,
  },
  {
    builtin: `local_invocation_index`,
    type: `u32`,
    uniform: false,
  },
  {
    builtin: `global_invocation_id`,
    type: `vec3<u32>`,
    uniform: false,
  },
  {
    builtin: `workgroup_id`,
    type: `vec3<u32>`,
    uniform: true,
  },
  {
    builtin: `num_workgroups`,
    type: `vec3<u32>`,
    uniform: true,
  },
];

g.test('compute_builtin_values')
  .desc(`Test uniformity of compute built-in values`)
  .params(u => u.combineWithParams(kComputeBuiltinValues).beginSubcases())
  .fn(t => {
    let cond = ``;
    switch (t.params.type) {
      case `u32`:
      case `i32`:
      case `f32`: {
        cond = `p > 0`;
        break;
      }
      case `vec3<u32>`:
      case `vec3<i32>`:
      case `vec3<f32>`: {
        cond = `p.x > 0`;
        break;
      }
      case `bool`: {
        cond = `p`;
        break;
      }
      default: {
        unreachable(`Unhandled type`);
      }
    }
    const code = `
@compute @workgroup_size(16,1,1)
fn main(@builtin(${t.params.builtin}) p : ${t.params.type}) {
  if ${cond} {
    workgroupBarrier();
  }
}
`;

    t.expectCompileResult(t.params.uniform, code);
  });

function expectedUniformity(uniform: string, init: string): boolean {
  if (uniform === `always`) {
    return true;
  } else if (uniform === `init`) {
    return init === `no_init` || init === `uniform`;
  }

  // uniform == `never` (or unknown values)
  return false;
}

const kFuncVarCases = {
  no_assign: {
    typename: `u32`,
    typedecl: ``,
    assignment: ``,
    cond: `x > 0`,
    uniform: `init`,
  },
  simple_uniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `x = uniform_value[0];`,
    cond: `x > 0`,
    uniform: `always`,
  },
  simple_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `x = nonuniform_value[0];`,
    cond: `x > 0`,
    uniform: `never`,
  },
  compound_assign_uniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `x += uniform_value[0];`,
    cond: `x > 0`,
    uniform: `init`,
  },
  compound_assign_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `x -= nonuniform_value[0];`,
    cond: `x > 0`,
    uniform: `never`,
  },
  unreachable_uniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      break;
      x = uniform_value[0];
    }`,
    cond: `x > 0`,
    uniform: `init`,
  },
  unreachable_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      break;
      x = nonuniform_value[0];
    }`,
    cond: `x > 0`,
    uniform: `init`,
  },
  if_no_else_uniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `if uniform_cond {
      x = uniform_value[0];
    }`,
    cond: `x > 0`,
    uniform: `init`,
  },
  if_no_else_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `if uniform_cond {
      x = nonuniform_value[0];
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  if_no_then_uniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `if uniform_cond {
    } else {
      x = uniform_value[0];
    }`,
    cond: `x > 0`,
    uniform: `init`,
  },
  if_no_then_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `if uniform_cond {
    } else {
      x = nonuniform_value[0];
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  if_else_uniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `if uniform_cond {
      x = uniform_value[0];
    } else {
      x = uniform_value[1];
    }`,
    cond: `x > 0`,
    uniform: `always`,
  },
  if_else_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `if uniform_cond {
      x = nonuniform_value[0];
    } else {
      x = nonuniform_value[1];
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  if_else_split: {
    typename: `u32`,
    typedecl: ``,
    assignment: `if uniform_cond {
      x = uniform_value[0];
    } else {
      x = nonuniform_value[0];
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  if_unreachable_else_none: {
    typename: `u32`,
    typedecl: ``,
    assignment: `if uniform_cond {
    } else {
      return;
    }`,
    cond: `x > 0`,
    uniform: `init`,
  },
  if_unreachable_else_uniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `if uniform_cond {
      x = uniform_value[0];
    } else {
      return;
    }`,
    cond: `x > 0`,
    uniform: `always`,
  },
  if_unreachable_else_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `if uniform_cond {
      x = nonuniform_value[0];
    } else {
      return;
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  if_unreachable_then_none: {
    typename: `u32`,
    typedecl: ``,
    assignment: `if uniform_cond {
      return;
    }`,
    cond: `x > 0`,
    uniform: `init`,
  },
  if_unreachable_then_uniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `if uniform_cond {
      return;
    } else {
      x = uniform_value[0];
    }`,
    cond: `x > 0`,
    uniform: `always`,
  },
  if_unreachable_then_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `if uniform_cond {
      return;
    } else {
      x = nonuniform_value[0];
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  if_nonescaping_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `if uniform_cond {
      x = nonuniform_value[0];
      return;
    }`,
    cond: `x > 0`,
    uniform: `init`,
  },
  loop_body_depends_on_continuing_uniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      if x > 0 {
        let tmp = textureSample(t, s, vec2f(0,0));
      }
      continuing {
        x = uniform_value[0];
        break if uniform_cond;
      }
    }`,
    cond: `true`, // override the standard check
    uniform: `init`,
  },
  loop_body_depends_on_continuing_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      if x > 0 {
        let tmp = textureSample(t, s, vec2f(0,0));
      }
      continuing {
        x = nonuniform_value[0];
        break if uniform_cond;
      }
    }`,
    cond: `true`, // override the standard check
    uniform: `never`,
  },
  loop_body_uniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      x = uniform_value[0];
      continuing {
        break if uniform_cond;
      }
    }`,
    cond: `x > 0`,
    uniform: `always`,
  },
  loop_body_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      x = nonuniform_value[0];
      continuing {
        break if uniform_cond;
      }
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  loop_body_nonuniform_cond: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      // The analysis doesn't recognize the content of the value.
      x = uniform_value[0];
      continuing {
        break if nonuniform_cond;
      }
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  loop_unreachable_continuing: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      break;
      continuing {
        break if uniform_cond;
      }
    }`,
    cond: `x > 0`,
    uniform: `init`,
  },
  loop_continuing_from_body_uniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      x = uniform_value[0];
      continuing  {
        if x > 0 {
          let tmp = textureSample(t, s, vec2f(0,0));
        }
        break if uniform_cond;
      }
    }`,
    cond: `true`, // override the standard check
    uniform: `always`,
  },
  loop_continuing_from_body_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      x = nonuniform_value[0];
      continuing  {
        if x > 0 {
          let tmp = textureSample(t, s, vec2f(0,0));
        }
        break if uniform_cond;
      }
    }`,
    cond: `true`, // override the standard check
    uniform: `never`,
  },
  loop_continuing_from_body_split1: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      if uniform_cond {
        x = uniform_value[0];
      }
      continuing {
        if x > 0 {
          let tmp = textureSample(t, s, vec2f(0,0));
        }
        break if uniform_cond;
      }
    }`,
    cond: `true`, // override the standard check
    uniform: `init`,
  },
  loop_continuing_from_body_split2: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      if uniform_cond {
        x = nonuniform_value[0];
      }
      continuing {
        if x > 0 {
          let tmp = textureSample(t, s, vec2f(0,0));
        }
        break if uniform_cond;
      }
    }`,
    cond: `true`, // override the standard check
    uniform: `never`,
  },
  loop_continuing_from_body_split3: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      if uniform_cond {
        x = uniform_value[0];
      } else {
        x = uniform_value[1];
      }
      continuing {
        if x > 0 {
          let tmp = textureSample(t, s, vec2f(0,0));
        }
        break if uniform_cond;
      }
    }`,
    cond: `true`, // override the standard check
    uniform: `always`,
  },
  loop_continuing_from_body_split4: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      if nonuniform_cond {
        x = uniform_value[0];
      } else {
        x = uniform_value[1];
      }
      continuing {
        if x > 0 {
          let tmp = textureSample(t, s, vec2f(0,0));
        }
        break if uniform_cond;
      }
    }`,
    cond: `true`, // override the standard check
    uniform: `never`,
  },
  loop_continuing_from_body_split5: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      if nonuniform_cond {
        x = uniform_value[0];
      } else {
        x = uniform_value[0];
      }
      continuing {
        if x > 0 {
          let tmp = textureSample(t, s, vec2f(0,0));
        }
        break if uniform_cond;
      }
    }`,
    cond: `true`, // override the standard check
    // The analysis doesn't recognize that uniform_value[0] is assignment on all paths.
    uniform: `never`,
  },
  loop_in_loop_with_continue_uniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      loop {
        x = nonuniform_value[0];
        if nonuniform_cond {
          break;
        }
        continue;
      }
      x = uniform_value[0];
      continuing {
        if x > 0 {
          let tmp = textureSample(t, s, vec2f(0,0));
        }
        break if uniform_cond;
      }
    }`,
    cond: `true`, // override the standard check
    uniform: `always`,
  },
  loop_in_loop_with_continue_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      loop {
        x = uniform_value[0];
        if uniform_cond {
          break;
        }
        continue;
      }
      x = nonuniform_value[0];
      continuing {
        if x > 0 {
          let tmp = textureSample(t, s, vec2f(0,0));
        }
        break if uniform_cond;
      }
    }`,
    cond: `true`, // override the standard check
    uniform: `never`,
  },
  after_loop_with_uniform_break_uniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      if uniform_cond {
        x = uniform_value[0];
        break;
      }
    }`,
    cond: `x > 0`,
    uniform: `always`,
  },
  after_loop_with_uniform_break_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      if uniform_cond {
        x = nonuniform_value[0];
        break;
      }
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  after_loop_with_nonuniform_break: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      if nonuniform_cond {
        x = uniform_value[0];
        break;
      }
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  after_loop_with_uniform_breaks: {
    typename: `u32`,
    typedecl: ``,
    assignment: `loop {
      if uniform_cond {
        x = uniform_value[0];
        break;
      } else {
        break;
      }
    }`,
    cond: `x > 0`,
    uniform: `init`,
  },
  switch_uniform_case: {
    typename: `u32`,
    typedecl: ``,
    assignment: `switch uniform_val {
      case 0 {
        if x > 0 {
          let tmp = textureSample(t, s, vec2f(0,0));
        }
      }
      default {
      }
    }`,
    cond: `true`, // override default check
    uniform: `init`,
  },
  switch_nonuniform_case: {
    typename: `u32`,
    typedecl: ``,
    assignment: `switch nonuniform_val {
      case 0 {
        if x > 0 {
          let tmp = textureSample(t, s, vec2f(0,0));
        }
      }
      default {
      }
    }`,
    cond: `true`, // override default check
    uniform: `never`,
  },
  after_switch_all_uniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `switch uniform_val {
      case 0 {
        x = uniform_value[0];
      }
      case 1,2 {
        x = uniform_value[1];
      }
      default {
        x = uniform_value[2];
      }
    }`,
    cond: `x > 0`,
    uniform: `always`,
  },
  after_switch_some_assign: {
    typename: `u32`,
    typedecl: ``,
    assignment: `switch uniform_val {
      case 0 {
        x = uniform_value[0];
      }
      case 1,2 {
        x = uniform_value[1];
      }
      default {
      }
    }`,
    cond: `x > 0`,
    uniform: `init`,
  },
  after_switch_nonuniform: {
    typename: `u32`,
    typedecl: ``,
    assignment: `switch uniform_val {
      case 0 {
        x = uniform_value[0];
      }
      case 1,2 {
        x = uniform_value[1];
      }
      default {
        x = nonuniform_value[0];
      }
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  after_switch_with_break_nonuniform1: {
    typename: `u32`,
    typedecl: ``,
    assignment: `switch uniform_val {
      default {
        if uniform_cond {
          x = uniform_value[0];
          break;
        }
        x = nonuniform_value[0];
      }
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  after_switch_with_break_nonuniform2: {
    typename: `u32`,
    typedecl: ``,
    assignment: `switch uniform_val {
      default {
        x = uniform_value[0];
        if uniform_cond {
          x = nonuniform_value[0];
          break;
        }
      }
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  for_loop_uniform_body: {
    typename: `u32`,
    typedecl: ``,
    assignment: `for (var i = 0; i < 10; i += 1) {
      x = uniform_value[0];
    }`,
    cond: `x > 0`,
    uniform: `init`,
  },
  for_loop_nonuniform_body: {
    typename: `u32`,
    typedecl: ``,
    assignment: `for (var i = 0; i < 10; i += 1) {
      x = nonuniform_value[0];
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  for_loop_uniform_body_no_condition: {
    typename: `u32`,
    typedecl: ``,
    assignment: `for (var i = 0; ; i += 1) {
      x = uniform_value[0];
      if uniform_cond {
        break;
      }
    }`,
    cond: `x > 0`,
    uniform: `always`,
  },
  for_loop_nonuniform_body_no_condition: {
    typename: `u32`,
    typedecl: ``,
    assignment: `for (var i = 0; ; i += 1) {
      x = nonuniform_value[0];
      if uniform_cond {
        break;
      }
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  for_loop_uniform_increment: {
    typename: `u32`,
    typedecl: ``,
    assignment: `for (; uniform_cond; x += uniform_value[0]) {
    }`,
    cond: `x > 0`,
    uniform: `init`,
  },
  for_loop_nonuniform_increment: {
    typename: `u32`,
    typedecl: ``,
    assignment: `for (; uniform_cond; x += nonuniform_value[0]) {
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  for_loop_uniform_init: {
    typename: `u32`,
    typedecl: ``,
    assignment: `for (x = uniform_value[0]; uniform_cond; ) {
    }`,
    cond: `x > 0`,
    uniform: `always`,
  },
  for_loop_nonuniform_init: {
    typename: `u32`,
    typedecl: ``,
    assignment: `for (x = nonuniform_value[0]; uniform_cond;) {
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  while_loop_uniform_body: {
    typename: `u32`,
    typedecl: ``,
    assignment: `while uniform_cond {
      x = uniform_value[0];
    }`,
    cond: `x > 0`,
    uniform: `init`,
  },
  while_loop_nonuniform_body: {
    typename: `u32`,
    typedecl: ``,
    assignment: `while uniform_cond {
      x = nonuniform_value[0];
    }`,
    cond: `x > 0`,
    uniform: `never`,
  },
  partial_assignment_uniform: {
    typename: `block`,
    typedecl: `struct block {
      x : u32,
      y : u32
    }`,
    assignment: `x.x = uniform_value[0].x;`,
    cond: `x.x > 0`,
    uniform: `init`,
  },
  partial_assignment_nonuniform: {
    typename: `block`,
    typedecl: `struct block {
      x : u32,
      y : u32
    }`,
    assignment: `x.x = nonuniform_value[0].x;`,
    cond: `x.x > 0`,
    uniform: `never`,
  },
  partial_assignment_all_members_uniform: {
    typename: `block`,
    typedecl: `struct block {
      x : u32,
      y : u32
    }`,
    assignment: `x.x = uniform_value[0].x;
    x.y = uniform_value[1].y;`,
    cond: `x.x > 0`,
    uniform: `init`,
  },
  partial_assignment_all_members_nonuniform: {
    typename: `block`,
    typedecl: `struct block {
      x : u32,
      y : u32
    }`,
    assignment: `x.x = nonuniform_value[0].x;
    x.y = uniform_value[0].x;`,
    cond: `x.x > 0`,
    uniform: `never`,
  },
  partial_assignment_single_element_struct_uniform: {
    typename: `block`,
    typedecl: `struct block {
      x : u32
    }`,
    assignment: `x.x = uniform_value[0].x;`,
    cond: `x.x > 0`,
    uniform: `init`,
  },
  partial_assignment_single_element_struct_nonuniform: {
    typename: `block`,
    typedecl: `struct block {
      x : u32
    }`,
    assignment: `x.x = nonuniform_value[0].x;`,
    cond: `x.x > 0`,
    uniform: `never`,
  },
  partial_assignment_single_element_array_uniform: {
    typename: `array<u32, 1>`,
    typedecl: ``,
    assignment: `x[0] = uniform_value[0][0];`,
    cond: `x[0] > 0`,
    uniform: `init`,
  },
  partial_assignment_single_element_array_nonuniform: {
    typename: `array<u32, 1>`,
    typedecl: ``,
    assignment: `x[0] = nonuniform_value[0][0];`,
    cond: `x[0] > 0`,
    uniform: `never`,
  },
  nested1: {
    typename: `block`,
    typedecl: `struct block {
      x : u32,
      y : u32
    }`,
    assignment: `for (; uniform_cond; ) {
      if uniform_cond {
        x = uniform_value[0];
        break;
        x.y = nonuniform_value[0].y;
      } else {
        if uniform_cond {
          continue;
        }
        x = uniform_value[1];
      }
    }`,
    cond: `x.x > 0`,
    uniform: `init`,
  },
  nested2: {
    typename: `block`,
    typedecl: `struct block {
      x : u32,
      y : u32
    }`,
    assignment: `for (; uniform_cond; ) {
      if uniform_cond {
        x = uniform_value[0];
        break;
        x.y = nonuniform_value[0].y;
      } else {
        if nonuniform_cond {
          continue;
        }
        x = uniform_value[1];
      }
    }`,
    cond: `x.x > 0`,
    uniform: `never`,
  },
};

const kVarInit = {
  no_init: ``,
  uniform: `= uniform_value[3];`,
  nonuniform: `= nonuniform_value[3];`,
};

g.test('function_variables')
  .desc(`Test uniformity of function variables`)
  .params(u => u.combine('case', keysOf(kFuncVarCases)).combine('init', keysOf(kVarInit)))
  .fn(t => {
    const func_case = kFuncVarCases[t.params.case];
    const code = `
${func_case.typedecl}

@group(0) @binding(0)
var<storage> uniform_value : array<${func_case.typename}, 4>;
@group(0) @binding(1)
var<storage, read_write> nonuniform_value : array<${func_case.typename}, 4>;

@group(1) @binding(0)
var t : texture_2d<f32>;
@group(1) @binding(1)
var s : sampler;

var<private> nonuniform_cond : bool = true;
const uniform_cond : bool = true;
var<private> nonuniform_val : u32 = 0;
const uniform_val : u32 = 0;

@fragment
fn main() {
  var x : ${func_case.typename} ${kVarInit[t.params.init]};

  ${func_case.assignment}

  if ${func_case.cond} {
    let tmp = textureSample(t, s, vec2f(0,0));
  }
}
`;

    const result = expectedUniformity(func_case.uniform, t.params.init);
    if (!result) {
      t.expectCompileResult(true, `diagnostic(off, derivative_uniformity);\n` + code);
    }
    t.expectCompileResult(result, code);
  });
