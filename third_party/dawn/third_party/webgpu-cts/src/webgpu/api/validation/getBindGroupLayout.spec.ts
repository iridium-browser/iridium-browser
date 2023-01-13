export const description = `
  getBindGroupLayout validation tests.
`;

import { makeTestGroup } from '../../../common/framework/test_group.js';

import { ValidationTest } from './validation_test.js';

export const g = makeTestGroup(ValidationTest);

g.test('index_range,explicit_layout')
  .desc(
    `
  Test that a validation error is generated if the index exceeds the size of the bind group layouts
  using a pipeline with an explicit layout.
  `
  )
  .params(u => u.combine('index', [0, 1, 2, 3, 4, 5]))
  .fn(async t => {
    const { index } = t.params;

    const pipelineBindGroupLayouts = t.device.createBindGroupLayout({
      entries: [],
    });

    const kBindGroupLayoutsSizeInPipelineLayout = 1;
    const pipelineLayout = t.device.createPipelineLayout({
      bindGroupLayouts: [pipelineBindGroupLayouts],
    });

    const pipeline = t.device.createRenderPipeline({
      layout: pipelineLayout,
      vertex: {
        module: t.device.createShaderModule({
          code: `
            @vertex
            fn main()-> @builtin(position) vec4<f32> {
              return vec4<f32>(0.0, 0.0, 0.0, 1.0);
            }`,
        }),
        entryPoint: 'main',
      },
      fragment: {
        module: t.device.createShaderModule({
          code: `
            @fragment
            fn main() -> @location(0) vec4<f32> {
              return vec4<f32>(0.0, 1.0, 0.0, 1.0);
            }`,
        }),
        entryPoint: 'main',
        targets: [{ format: 'rgba8unorm' }],
      },
    });

    const shouldError = index >= kBindGroupLayoutsSizeInPipelineLayout;

    t.expectValidationError(() => {
      pipeline.getBindGroupLayout(index);
    }, shouldError);
  });

g.test('index_range,auto_layout')
  .desc(
    `
  Test that a validation error is generated if the index exceeds the size of the bind group layouts
  using a pipeline with an auto layout.
  `
  )
  .params(u => u.combine('index', [0, 1, 2, 3, 4, 5]))
  .fn(async t => {
    const { index } = t.params;

    const kBindGroupLayoutsSizeInPipelineLayout = 1;

    const pipeline = t.device.createRenderPipeline({
      layout: 'auto',
      vertex: {
        module: t.device.createShaderModule({
          code: `
            @vertex
            fn main()-> @builtin(position) vec4<f32> {
              return vec4<f32>(0.0, 0.0, 0.0, 1.0);
            }`,
        }),
        entryPoint: 'main',
      },
      fragment: {
        module: t.device.createShaderModule({
          code: `
            @group(0) @binding(0) var<uniform> binding: f32;
            @fragment
            fn main() -> @location(0) vec4<f32> {
              _ = binding;
              return vec4<f32>(0.0, 1.0, 0.0, 1.0);
            }`,
        }),
        entryPoint: 'main',
        targets: [{ format: 'rgba8unorm' }],
      },
    });

    const shouldError = index >= kBindGroupLayoutsSizeInPipelineLayout;

    t.expectValidationError(() => {
      pipeline.getBindGroupLayout(index);
    }, shouldError);
  });
