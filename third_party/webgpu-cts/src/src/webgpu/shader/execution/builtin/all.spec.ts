export const description = `
Execution Tests for the 'all' builtin function
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { assert, TypedArrayBufferViewConstructor } from '../../../../common/util/util.js';
import { GPUTest } from '../../../gpu_test.js';
import { NumberRepr, NumberType } from '../../../util/conversion.js';
import { generateTypes } from '../../types.js';

import { Case, runShaderTestImpl } from './builtin.js';

export const g = makeTestGroup(GPUTest);
export function runShaderTest<F extends NumberType>(
  t: GPUTest,
  storageClass: string,
  storageMode: string,
  type: string,
  arrayLength: number,
  builtin: string,
  arrayType: TypedArrayBufferViewConstructor,
  cases: Array<Case<F>>
): void {
  const source = `
    [[block]]
    struct Data {
      values : [[stride(16)]] array<${type}, ${cases.length}>;
    };

    [[group(0), binding(0)]] var<${storageClass}, ${storageMode}> inputs : Data;
    [[group(0), binding(1)]] var<${storageClass}, write> outputs : Data;

    let dim_mask = 0x000Fu; // bits 0 through 3
    let pos0_field = 0x0010u; // bit 4
    let pos1_field = 0x0020u; // bit 5
    let pos2_field = 0x0040u; // bit 6
    let pos3_field = 0x0080u; // bit 7

    fn get_val(input: u32, field: u32) -> bool {
      return field == (input & field);
    }

    fn test_scalar(input: u32) -> u32 {
      let bool_input = get_val(input, pos0_field);
      if (${builtin}(bool_input)) {
        return 1u;
      }
      return 0u;
    }
    
     fn test_vec2(input: u32) -> u32 {
      let bool_input = vec2<bool>(get_val(input, pos0_field), 
                                  get_val(input, pos1_field));
      if (${builtin}(bool_input)) {
        return 1u;
      }
      return 0u;
    }

     fn test_vec3(input: u32) -> u32 {
      let bool_input = vec3<bool>(get_val(input, pos0_field), 
                                  get_val(input, pos1_field),
                                  get_val(input, pos2_field));
      if (${builtin}(bool_input)) {
        return 1u;
      }
      return 0u;
    }
    
    fn test_vec4(input: u32) -> u32 {
      let bool_input = vec4<bool>(get_val(input, pos0_field),
                                  get_val(input, pos1_field),
                                  get_val(input, pos2_field),
                                  get_val(input, pos3_field));
      if (${builtin}(bool_input)) {
        return 1u;
      }
      return 0u;
    }
    
    [[stage(compute), workgroup_size(1)]]
    fn main() {
      for(var i = 0; i < ${cases.length}; i = i + 1) {
        let input : ${type} = inputs.values[i];
        let dim : ${type} = input & dim_mask;
        if (dim == 0u || dim == 1u) {
          outputs.values[i] = test_scalar(inputs.values[i]);
        } 
        if (dim == 2u) {
          outputs.values[i] = test_vec2(inputs.values[i]);
        }
        if (dim == 3u) {
          outputs.values[i] = test_vec3(inputs.values[i]);
        }
        if (dim == 4u) {
          outputs.values[i] = test_vec4(inputs.values[i]);
        }
      }
    }
  `;
  runShaderTestImpl(
    t,
    storageClass,
    storageMode,
    type,
    arrayLength,
    builtin,
    arrayType,
    cases,
    source
  );
}

g.test('logical_builtin_functions,vector_all')
  .uniqueId('d140d173a2acf981')
  .specURL('https://www.w3.org/TR/2021/WD-WGSL-20210929/#logical-builtin-functions')
  .desc(
    `
vector all:
e: vecN<bool> all(e): bool Returns true if each component of e is true. (OpAll)
`
  )
  .params(u =>
    u
      .combineWithParams([{ storageClass: 'storage', storageMode: 'read_write' }] as const)
      .combine('containerType', ['scalar'] as const)
      .combine('isAtomic', [false])
      .combine('baseType', ['u32'] as const)
      .expandWithParams(generateTypes)
  )
  .fn(async t => {
    assert(t.params._kTypeInfo !== undefined, 'generated type is undefined');
    runShaderTest(
      t,
      t.params.storageClass,
      t.params.storageMode,
      t.params.type,
      t.params._kTypeInfo.arrayLength,
      'all',
      Uint32Array,
      /*
       * Since bools are not host shareable, bit packing to/from u32s is
       * required.
       * Input format:
       *   bits 0 to 3: number of dimensions of the vector to be passed in,
       *                1 indicates use a scalar. Not all of the space is
       *                needed, but makes it easier to read, since things are
       *                aligned on hex digits.
       *   bit 5: value for scalar or position 0 in vector
       *   bit 6: value for position 1 in vector
       *   bit 7: value for position 2 in vector
       *   bit 8: value for position 3 in vector
       * Output format:
       *   bit 0: return value of all
       */
      /* prettier-ignore */ [
        // Scalars
        { input: NumberRepr.fromU32(0x01), expected: [NumberRepr.fromU32(0)] }, // F -> F
        { input: NumberRepr.fromU32(0x11), expected: [NumberRepr.fromU32(1)] }, // T -> T
        // vec2
        { input: NumberRepr.fromU32(0x02), expected: [NumberRepr.fromU32(0)] }, // [ F, F ] -> F
        { input: NumberRepr.fromU32(0x12), expected: [NumberRepr.fromU32(0)] }, // [ T, F ] -> F
        { input: NumberRepr.fromU32(0x22), expected: [NumberRepr.fromU32(0)] }, // [ F, T ] -> F
        { input: NumberRepr.fromU32(0x32), expected: [NumberRepr.fromU32(1)] }, // [ T, T ] -> T
        // vec3
        { input: NumberRepr.fromU32(0x03), expected: [NumberRepr.fromU32(0)] }, // [ F, F, F ] -> F
        { input: NumberRepr.fromU32(0x13), expected: [NumberRepr.fromU32(0)] }, // [ T, F, F ] -> F
        { input: NumberRepr.fromU32(0x23), expected: [NumberRepr.fromU32(0)] }, // [ F, T, F ] -> F
        { input: NumberRepr.fromU32(0x33), expected: [NumberRepr.fromU32(0)] }, // [ T, T, F ] -> F
        { input: NumberRepr.fromU32(0x43), expected: [NumberRepr.fromU32(0)] }, // [ F, F, T ] -> F
        { input: NumberRepr.fromU32(0x53), expected: [NumberRepr.fromU32(0)] }, // [ T, F, T ] -> F
        { input: NumberRepr.fromU32(0x63), expected: [NumberRepr.fromU32(0)] }, // [ F, T, T ] -> F
        { input: NumberRepr.fromU32(0x73), expected: [NumberRepr.fromU32(1)] }, // [ T, T, T ] -> T
        // vec4
        { input: NumberRepr.fromU32(0x04), expected: [NumberRepr.fromU32(0)] }, // [ F, F, F, F ] -> F
        { input: NumberRepr.fromU32(0x14), expected: [NumberRepr.fromU32(0)] }, // [ F, T, F, F ] -> F
        { input: NumberRepr.fromU32(0x24), expected: [NumberRepr.fromU32(0)] }, // [ F, F, T, F ] -> F
        { input: NumberRepr.fromU32(0x34), expected: [NumberRepr.fromU32(0)] }, // [ F, T, T, F ] -> F
        { input: NumberRepr.fromU32(0x44), expected: [NumberRepr.fromU32(0)] }, // [ F, F, F, T ] -> F
        { input: NumberRepr.fromU32(0x54), expected: [NumberRepr.fromU32(0)] }, // [ F, T, F, T ] -> F
        { input: NumberRepr.fromU32(0x64), expected: [NumberRepr.fromU32(0)] }, // [ F, F, T, T ] -> F
        { input: NumberRepr.fromU32(0x74), expected: [NumberRepr.fromU32(0)] }, // [ F, T, T, T ] -> F
        { input: NumberRepr.fromU32(0x84), expected: [NumberRepr.fromU32(0)] }, // [ T, F, F, F ] -> F
        { input: NumberRepr.fromU32(0x94), expected: [NumberRepr.fromU32(0)] }, // [ T, F, F, T ] -> F
        { input: NumberRepr.fromU32(0xA4), expected: [NumberRepr.fromU32(0)] }, // [ T, F, T, F ] -> F
        { input: NumberRepr.fromU32(0xB4), expected: [NumberRepr.fromU32(0)] }, // [ T, F, T, T ] -> F
        { input: NumberRepr.fromU32(0xC4), expected: [NumberRepr.fromU32(0)] }, // [ T, T, F, F ] -> F
        { input: NumberRepr.fromU32(0xD4), expected: [NumberRepr.fromU32(0)] }, // [ T, T, F, T ] -> F
        { input: NumberRepr.fromU32(0xE4), expected: [NumberRepr.fromU32(0)] }, // [ T, T, T, F ] -> F
        { input: NumberRepr.fromU32(0xF4), expected: [NumberRepr.fromU32(1)] }, // [ T, T, T, T ] -> T
      ]
    );
  });
