export const description = `Validation tests for @align`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { ShaderValidationTest } from '../shader_validation_test.js';

export const g = makeTestGroup(ShaderValidationTest);

const kValidAlign = new Set([
  '',
  '@align(1)',
  '@align(4)',
  '@align(4i)',
  '@align(0x4)',
  '@align(4,)',
  '@align(1073741824)',
  '@\talign\t(4)',
  '@/^comment^/align/^comment^/(4)',
]);
const kInvalidAlign = new Set([
  '@malign(4)',
  '@align()',
  '@align 4)',
  '@align(4',
  '@align(4, 2)',
  '@align(4,)',
  '@align(3)', // Not a power of 2
  '@align(val)',
  '@align(1.0)',
  '@align(4u)',
  '@align(4f)',
  '@align(4h)',
  '@align',
  '@align(0)',
  '@align(-4)',
  '@align(2147483646)', // Not a power of 2
  '@align(2147483648)', // Larger then max i32
]);

g.test('missing_attribute_on_param_struct')
  .desc(`Test that @align is parsed correctly.`)
  .params(u => u.combine('align', new Set([...kValidAlign, ...kInvalidAlign])))
  .fn(t => {
    const v = t.params.align.replace(/\^/g, '*');
    const code = `
const val: i32 = 4;
struct B {
  ${v} a: i32,
}

@group(0) @binding(0)
var<uniform> uniform_buffer: B;

@fragment
fn main() -> @location(0) vec4<f32> {
  return vec4<f32>(.4, .2, .3, .1);
}`;
    t.expectCompileResult(kValidAlign.has(t.params.align), code);
  });
