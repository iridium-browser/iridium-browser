export const description = `
TODO:
- test compatibility between bind groups and pipelines
    - the binding resource in bindGroups[i].layout is "group-equivalent" (value-equal) to pipelineLayout.bgls[i].
    - in the test fn, test once without the dispatch/draw (should always be valid) and once with
      the dispatch/draw, to make sure the validation happens in dispatch/draw.
    - x= {dispatch, all draws} (dispatch/draw should be size 0 to make sure validation still happens if no-op)
    - x= all relevant stages

TODO: subsume existing test, rewrite fixture as needed.
`;

import { makeTestGroup } from '../../../../../common/framework/test_group.js';
import {
  kSamplerBindingTypes,
  kShaderStageCombinations,
  kBufferBindingTypes,
} from '../../../../capability_info.js';
import { GPUConst } from '../../../../constants.js';
import {
  ProgrammableEncoderType,
  kProgrammableEncoderTypes,
} from '../../../../util/command_buffer_maker.js';
import { ValidationTest } from '../../validation_test.js';

function getTestCmds(encoderType: ProgrammableEncoderType): readonly string[] {
  if (encoderType === 'compute pass') {
    return ['dispatch', 'dispatchIndirect'] as const;
  } else {
    return ['draw', 'drawIndexed', 'drawIndirect', 'drawIndexedIndirect'] as const;
  }
}

const kResourceTypes = ['buffer', 'sampler', 'texture', 'storageTexture', 'externalTexture'];

class F extends ValidationTest {
  getUniformBuffer(): GPUBuffer {
    return this.device.createBuffer({
      size: 8 * Float32Array.BYTES_PER_ELEMENT,
      usage: GPUBufferUsage.UNIFORM,
    });
  }

  createRenderPipelineWithLayout(
    device: GPUDevice,
    bindGroups: Array<Array<GPUBindGroupLayoutEntry>>
  ): GPURenderPipeline {
    const shader = `
      [[stage(vertex)]] fn vs_main() -> [[builtin(position)]] vec4<f32> {
        return vec4<f32>(1.0, 1.0, 0.0, 1.0);
      }

      [[stage(fragment)]] fn fs_main() -> [[location(0)]] vec4<f32> {
        return vec4<f32>(0.0, 1.0, 0.0, 1.0);
      }
    `;
    const module = device.createShaderModule({ code: shader });
    const pipeline = this.device.createRenderPipeline({
      layout: device.createPipelineLayout({
        bindGroupLayouts: bindGroups.map(entries => device.createBindGroupLayout({ entries })),
      }),
      vertex: {
        module,
        entryPoint: 'vs_main',
      },
      fragment: {
        module,
        entryPoint: 'fs_main',
        targets: [{ format: 'rgba8unorm' }],
      },
      primitive: { topology: 'triangle-list' },
    });
    return pipeline;
  }

  createComputePipelineWithLayout(
    device: GPUDevice,
    bindGroups: Array<Array<GPUBindGroupLayoutEntry>>
  ): GPUComputePipeline {
    const shader = `
      [[stage(compute), workgroup_size(1, 1, 1)]]
        fn main([[builtin(global_invocation_id)]] GlobalInvocationID : vec3<u32>) {
      }
    `;

    const module = device.createShaderModule({ code: shader });
    const pipeline = this.device.createComputePipeline({
      layout: device.createPipelineLayout({
        bindGroupLayouts: bindGroups.map(entries => device.createBindGroupLayout({ entries })),
      }),
      compute: {
        module,
        entryPoint: 'main',
      },
    });
    return pipeline;
  }

  beginRenderPass(commandEncoder: GPUCommandEncoder): GPURenderPassEncoder {
    const attachmentTexture = this.device.createTexture({
      format: 'rgba8unorm',
      size: { width: 16, height: 16, depthOrArrayLayers: 1 },
      usage: GPUTextureUsage.RENDER_ATTACHMENT,
    });

    return commandEncoder.beginRenderPass({
      colorAttachments: [
        {
          view: attachmentTexture.createView(),
          loadValue: { r: 1.0, g: 0.0, b: 0.0, a: 1.0 },
          storeOp: 'store',
        },
      ],
    });
  }
}

export const g = makeTestGroup(F);

g.test('bind_groups_and_pipeline_layout_mismatch')
  .desc(
    `
    Tests the bind groups must match the requirements of the pipeline layout.
    - bind groups required by the pipeline layout are required.
    - bind groups unused by the pipeline layout can be set or not.

    TODO: merge existing tests to this test
    `
  )
  .params(u =>
    u
      .combine('encoderType', kProgrammableEncoderTypes)
      .expand('call', p => getTestCmds(p.encoderType))
      .beginSubcases()
      .combineWithParams([
        { setBindGroup0: true, setUnusedBindGroup1: true, setBindGroup2: true, _success: true },
        { setBindGroup0: true, setUnusedBindGroup1: false, setBindGroup2: true, _success: true },
        { setBindGroup0: true, setUnusedBindGroup1: true, setBindGroup2: false, _success: false },
        { setBindGroup0: false, setUnusedBindGroup1: true, setBindGroup2: true, _success: false },
        { setBindGroup0: false, setUnusedBindGroup1: false, setBindGroup2: false, _success: false },
      ])
      .combine('useU32Array', [false, true])
  )
  .unimplemented();

g.test('it_is_invalid_to_draw_in_a_render_pass_with_missing_bind_groups')
  .paramsSubcasesOnly([
    { setBindGroup1: true, setBindGroup2: true, _success: true },
    { setBindGroup1: true, setBindGroup2: false, _success: false },
    { setBindGroup1: false, setBindGroup2: true, _success: false },
    { setBindGroup1: false, setBindGroup2: false, _success: false },
  ])
  .fn(async t => {
    const { setBindGroup1, setBindGroup2, _success } = t.params;

    const bindGroupLayouts: GPUBindGroupLayoutEntry[][] = [
      // bind group layout 0
      [
        {
          binding: 0,
          visibility: GPUShaderStage.VERTEX,
          buffer: { type: 'uniform' },
        },
      ],

      // bind group layout 1
      [
        {
          binding: 0,
          visibility: GPUShaderStage.VERTEX,
          buffer: { type: 'uniform' },
        },
      ],
    ];
    const pipeline = t.createRenderPipelineWithLayout(t.device, bindGroupLayouts);

    const uniformBuffer = t.getUniformBuffer();

    const bindGroup0 = t.device.createBindGroup({
      entries: [
        {
          binding: 0,
          resource: {
            buffer: uniformBuffer,
          },
        },
      ],
      layout: t.device.createBindGroupLayout({
        entries: bindGroupLayouts[0],
      }),
    });

    const bindGroup1 = t.device.createBindGroup({
      entries: [
        {
          binding: 0,
          resource: {
            buffer: uniformBuffer,
          },
        },
      ],
      layout: t.device.createBindGroupLayout({
        entries: bindGroupLayouts[1],
      }),
    });

    const commandEncoder = t.device.createCommandEncoder();
    const renderPass = t.beginRenderPass(commandEncoder);
    renderPass.setPipeline(pipeline);
    if (setBindGroup1) {
      renderPass.setBindGroup(0, bindGroup0);
    }
    if (setBindGroup2) {
      renderPass.setBindGroup(1, bindGroup1);
    }
    renderPass.draw(3);
    renderPass.endPass();
    t.expectValidationError(() => {
      commandEncoder.finish();
    }, !_success);
  });

g.test('buffer_binding,render_pipeline')
  .desc(
    `
  The GPUBufferBindingLayout bindings configure should be exactly
  same in PipelineLayout and bindgroup.
  - TODO: test more draw functions, e.g. indirect
  - TODO: test more visibilities, e.g. vetex
  - TODO: bind group should be created with different layout
  `
  )
  .params(u => u.combine('type', kBufferBindingTypes))
  .fn(async t => {
    const { type } = t.params;

    // Create fixed bindGroup
    const uniformBuffer = t.getUniformBuffer();

    const bindGroup = t.device.createBindGroup({
      entries: [
        {
          binding: 0,
          resource: {
            buffer: uniformBuffer,
          },
        },
      ],
      layout: t.device.createBindGroupLayout({
        entries: [
          {
            binding: 0,
            visibility: GPUShaderStage.FRAGMENT,
            buffer: {}, // default type: uniform
          },
        ],
      }),
    });

    // Create pipeline with different layouts
    const pipeline = t.createRenderPipelineWithLayout(t.device, [
      [
        {
          binding: 0,
          visibility: GPUShaderStage.FRAGMENT,
          buffer: {
            type,
          },
        },
      ],
    ]);

    const success = type === undefined || type === 'uniform';

    const commandEncoder = t.device.createCommandEncoder();
    const renderPass = t.beginRenderPass(commandEncoder);
    renderPass.setPipeline(pipeline);
    renderPass.setBindGroup(0, bindGroup);
    renderPass.draw(3);
    renderPass.endPass();
    t.expectValidationError(() => {
      commandEncoder.finish();
    }, !success);
  });

g.test('sampler_binding,render_pipeline')
  .desc(
    `
  The GPUSamplerBindingLayout bindings configure should be exactly
  same in PipelineLayout and bindgroup.
  - TODO: test more draw functions, e.g. indirect
  - TODO: test more visibilities, e.g. vetex
  `
  )
  .params(u =>
    u //
      .combine('bglType', kSamplerBindingTypes)
      .combine('bgType', kSamplerBindingTypes)
  )
  .fn(async t => {
    const { bglType, bgType } = t.params;
    const bindGroup = t.device.createBindGroup({
      entries: [
        {
          binding: 0,
          resource:
            bgType === 'comparison'
              ? t.device.createSampler({ compare: 'always' })
              : t.device.createSampler(),
        },
      ],
      layout: t.device.createBindGroupLayout({
        entries: [
          {
            binding: 0,
            visibility: GPUShaderStage.FRAGMENT,
            sampler: { type: bgType },
          },
        ],
      }),
    });

    // Create pipeline with different layouts
    const pipeline = t.createRenderPipelineWithLayout(t.device, [
      [
        {
          binding: 0,
          visibility: GPUShaderStage.FRAGMENT,
          sampler: {
            type: bglType,
          },
        },
      ],
    ]);

    const success = bglType === bgType;

    const commandEncoder = t.device.createCommandEncoder();
    const renderPass = t.beginRenderPass(commandEncoder);
    renderPass.setPipeline(pipeline);
    renderPass.setBindGroup(0, bindGroup);
    renderPass.draw(3);
    renderPass.endPass();
    t.expectValidationError(() => {
      commandEncoder.finish();
    }, !success);
  });

g.test('bgl_binding_mismatch')
  .desc(
    'Tests the binding number must exist or not exist in both bindGroups[i].layout and pipelineLayout.bgls[i]'
  )
  .params(u =>
    u
      .combine('encoderType', kProgrammableEncoderTypes)
      .expand('call', p => getTestCmds(p.encoderType))
      .beginSubcases()
      .combineWithParams([
        { bgBindings: [0, 1, 2], plBindings: [0, 1, 2], _success: true },
        { bgBindings: [0, 1, 2], plBindings: [0, 1, 3], _success: false },
        { bgBindings: [0, 2], plBindings: [0, 2], _success: true },
        { bgBindings: [0, 2], plBindings: [2, 0], _success: true },
        { bgBindings: [0, 1, 2], plBindings: [0, 1], _success: false },
        { bgBindings: [0, 1], plBindings: [0, 1, 2], _success: false },
      ])
      .combine('useU32Array', [false, true])
  )
  .unimplemented();

g.test('bgl_visibility_mismatch')
  .desc('Tests the visibility in bindGroups[i].layout and pipelineLayout.bgls[i] must be matched')
  .params(u =>
    u
      .combine('encoderType', kProgrammableEncoderTypes)
      .expand('call', p => getTestCmds(p.encoderType))
      .beginSubcases()
      .combine('bgVisibility', kShaderStageCombinations)
      .expand('plVisibility', p =>
        p.encoderType === 'compute pass'
          ? ([GPUConst.ShaderStage.COMPUTE] as const)
          : ([
              GPUConst.ShaderStage.VERTEX,
              GPUConst.ShaderStage.FRAGMENT,
              GPUConst.ShaderStage.VERTEX | GPUConst.ShaderStage.FRAGMENT,
            ] as const)
      )
      .combine('useU32Array', [false, true])
  )
  .unimplemented();

g.test('bgl_resource_type_mismatch')
  .desc(
    'Tests the binding resource type in bindGroups[i].layout and pipelineLayout.bgls[i] must be matched'
  )
  .params(u =>
    u
      .combine('encoderType', kProgrammableEncoderTypes)
      .expand('call', p => getTestCmds(p.encoderType))
      .beginSubcases()
      .combine('bgResourceType', kResourceTypes)
      .combine('plResourceType', kResourceTypes)
      .combine('useU32Array', [false, true])
  )
  .unimplemented();
