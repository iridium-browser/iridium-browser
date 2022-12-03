export const description = `
This test dedicatedly tests validation of pipeline overridable constants of createRenderPipeline.
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';

import { CreateRenderPipelineValidationTest } from './common.js';

export const g = makeTestGroup(CreateRenderPipelineValidationTest);

g.test('identifier,vertex')
  .desc(
    `
Tests calling createComputePipeline(Async) validation for overridable constants identifiers in vertex state.
`
  )
  .params(u =>
    u //
      .combine('isAsync', [true, false])
      .combineWithParams([
        { vertexConstants: {}, _success: true },
        { vertexConstants: { x: 1, y: 1 }, _success: true },
        { vertexConstants: { x: 1, y: 1, 1: 1, 1000: 1 }, _success: true },
        { vertexConstants: { xxx: 1 }, _success: false },
        { vertexConstants: { 1: 1 }, _success: true },
        { vertexConstants: { 2: 1 }, _success: false },
        { vertexConstants: { z: 1 }, _success: false }, // pipeline constant id is specified for z
        { vertexConstants: { w: 1 }, _success: false }, // pipeline constant id is specified for w
        { vertexConstants: { 1: 1, z: 1 }, _success: false }, // pipeline constant id is specified for z
      ] as { vertexConstants: Record<string, GPUPipelineConstantValue>; _success: boolean }[])
  )
  .fn(async t => {
    const { isAsync, vertexConstants, _success } = t.params;

    t.doCreateRenderPipelineTest(isAsync, _success, {
      layout: 'auto',
      vertex: {
        module: t.device.createShaderModule({
          code: `
            override x: f32 = 0.0;
            override y: f32 = 0.0;
            @id(1) override z: f32 = 0.0;
            @id(1000) override w: f32 = 1.0;
            @vertex fn main() -> @builtin(position) vec4<f32> {
              return vec4<f32>(x, y, z, w);
            }`,
        }),
        entryPoint: 'main',
        constants: vertexConstants,
      },
      fragment: {
        module: t.device.createShaderModule({
          code: `@fragment fn main() -> @location(0) vec4<f32> {
              return vec4<f32>(0.0, 1.0, 0.0, 1.0);
            }`,
        }),
        entryPoint: 'main',
        targets: [{ format: 'rgba8unorm' }],
      },
    });
  });

g.test('identifier,fragment')
  .desc(
    `
Tests calling createComputePipeline(Async) validation for overridable constants identifiers in fragment state.
`
  )
  .params(u =>
    u //
      .combine('isAsync', [true, false])
      .combineWithParams([
        { fragmentConstants: {}, _success: true },
        { fragmentConstants: { r: 1, g: 1 }, _success: true },
        { fragmentConstants: { r: 1, g: 1, 1: 1, 1000: 1 }, _success: true },
        { fragmentConstants: { xxx: 1 }, _success: false },
        { fragmentConstants: { 1: 1 }, _success: true },
        { fragmentConstants: { 2: 1 }, _success: false },
        { fragmentConstants: { b: 1 }, _success: false }, // pipeline constant id is specified for b
        { fragmentConstants: { a: 1 }, _success: false }, // pipeline constant id is specified for a
        { fragmentConstants: { 1: 1, b: 1 }, _success: false }, // pipeline constant id is specified for b
      ] as { fragmentConstants: Record<string, GPUPipelineConstantValue>; _success: boolean }[])
  )
  .fn(async t => {
    const { isAsync, fragmentConstants, _success } = t.params;

    const descriptor = t.getDescriptor({
      fragmentShaderCode: `
        override r: f32 = 0.0;
        override g: f32 = 0.0;
        @id(1) override b: f32 = 0.0;
        @id(1000) override a: f32 = 0.0;
        @fragment fn main()
            -> @location(0) vec4<f32> {
            return vec4<f32>(r, g, b, a);
        }`,
      fragmentConstants,
    });

    t.doCreateRenderPipelineTest(isAsync, _success, descriptor);
  });

g.test('uninitialized,vertex')
  .desc(
    `
Tests calling createComputePipeline(Async) validation for uninitialized overridable constants in vertex state.
`
  )
  .params(u =>
    u //
      .combine('isAsync', [true, false])
      .combineWithParams([
        { vertexConstants: {}, _success: false },
        { vertexConstants: { x: 1, y: 1 }, _success: false }, // z is missing
        { vertexConstants: { x: 1, z: 1 }, _success: true },
        { vertexConstants: { x: 1, y: 1, z: 1, w: 1 }, _success: true },
      ] as { vertexConstants: Record<string, GPUPipelineConstantValue>; _success: boolean }[])
  )
  .fn(async t => {
    const { isAsync, vertexConstants, _success } = t.params;

    t.doCreateRenderPipelineTest(isAsync, _success, {
      layout: 'auto',
      vertex: {
        module: t.device.createShaderModule({
          code: `
            override x: f32;
            override y: f32 = 0.0;
            override z: f32;
            override w: f32 = 1.0;
            @vertex fn main() -> @builtin(position) vec4<f32> {
              return vec4<f32>(x, y, z, w);
            }`,
        }),
        entryPoint: 'main',
        constants: vertexConstants,
      },
      fragment: {
        module: t.device.createShaderModule({
          code: `@fragment fn main() -> @location(0) vec4<f32> {
              return vec4<f32>(0.0, 1.0, 0.0, 1.0);
            }`,
        }),
        entryPoint: 'main',
        targets: [{ format: 'rgba8unorm' }],
      },
    });
  });

g.test('uninitialized,fragment')
  .desc(
    `
Tests calling createComputePipeline(Async) validation for uninitialized overridable constants in fragment state.
`
  )
  .params(u =>
    u //
      .combine('isAsync', [true, false])
      .combineWithParams([
        { fragmentConstants: {}, _success: false },
        { fragmentConstants: { r: 1, g: 1 }, _success: false }, // b is missing
        { fragmentConstants: { r: 1, b: 1 }, _success: true },
        { fragmentConstants: { r: 1, g: 1, b: 1, a: 1 }, _success: true },
      ] as { fragmentConstants: Record<string, GPUPipelineConstantValue>; _success: boolean }[])
  )
  .fn(async t => {
    const { isAsync, fragmentConstants, _success } = t.params;

    const descriptor = t.getDescriptor({
      fragmentShaderCode: `
        override r: f32;
        override g: f32 = 0.0;
        override b: f32;
        override a: f32 = 0.0;
        @fragment fn main()
            -> @location(0) vec4<f32> {
            return vec4<f32>(r, g, b, a);
        }
          `,
      fragmentConstants,
    });

    t.doCreateRenderPipelineTest(isAsync, _success, descriptor);
  });
