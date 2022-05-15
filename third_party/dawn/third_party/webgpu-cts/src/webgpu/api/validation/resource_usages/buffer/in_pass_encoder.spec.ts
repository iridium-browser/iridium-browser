export const description = `
Buffer Usages Validation Tests in Render Pass and Compute Pass.
`;

import { makeTestGroup } from '../../../../../common/framework/test_group.js';
import { assert, unreachable } from '../../../../../common/util/util.js';
import { ValidationTest } from '../../validation_test.js';

const kBoundBufferSize = 256;

class F extends ValidationTest {
  createBindGroupLayoutForTest(
    type: 'uniform' | 'storage' | 'read-only-storage',
    resourceVisibility: 'compute' | 'fragment'
  ): GPUBindGroupLayout {
    const bindGroupLayoutEntry: GPUBindGroupLayoutEntry = {
      binding: 0,
      visibility:
        resourceVisibility === 'compute' ? GPUShaderStage.COMPUTE : GPUShaderStage.FRAGMENT,
      buffer: {
        type,
      },
    };
    return this.device.createBindGroupLayout({
      entries: [bindGroupLayoutEntry],
    });
  }

  createBindGroupForTest(
    buffer: GPUBuffer,
    offset: number,
    type: 'uniform' | 'storage' | 'read-only-storage',
    resourceVisibility: 'compute' | 'fragment'
  ): GPUBindGroup {
    return this.device.createBindGroup({
      layout: this.createBindGroupLayoutForTest(type, resourceVisibility),
      entries: [
        {
          binding: 0,
          resource: { buffer, offset, size: kBoundBufferSize },
        },
      ],
    });
  }

  beginSimpleRenderPass(encoder: GPUCommandEncoder) {
    const colorTexture = this.device.createTexture({
      format: 'rgba8unorm',
      usage: GPUTextureUsage.RENDER_ATTACHMENT,
      size: [16, 16, 1],
    });
    return encoder.beginRenderPass({
      colorAttachments: [
        {
          view: colorTexture.createView(),
          loadOp: 'load',
          storeOp: 'store',
        },
      ],
    });
  }
}

function IsBufferUsageInBindGroup(
  bufferUsage:
    | 'uniform'
    | 'storage'
    | 'read-only-storage'
    | 'vertex'
    | 'index'
    | 'indirect'
    | 'indexedIndirect'
): boolean {
  switch (bufferUsage) {
    case 'uniform':
    case 'storage':
    case 'read-only-storage':
      return true;
    case 'vertex':
    case 'index':
    case 'indirect':
    case 'indexedIndirect':
      return false;
    default:
      unreachable();
  }
}

export const g = makeTestGroup(F);

g.test('subresources,buffer_usage_in_render_pass')
  .desc(
    `
Test that when one buffer is used in one render pass encoder, its list of internal usages within one
usage scope (all the commands in the whole render pass) can only be a compatible usage list; while
there is no such restriction when it is used in different render pass encoders. The usage scope
rules are not related to the buffer offset or the bind group layout visibilities.`
  )
  .params(u =>
    u
      .combine('inSamePass', [true, false])
      .combine('hasOverlap', [true, false])
      .beginSubcases()
      .combine('usage0', [
        'uniform',
        'storage',
        'read-only-storage',
        'vertex',
        'index',
        'indirect',
        'indexedIndirect',
      ] as const)
      .combine('visibility0', ['compute', 'fragment'] as const)
      .unless(t => t.visibility0 === 'compute' && !IsBufferUsageInBindGroup(t.usage0))
      .combine('usage1', [
        'uniform',
        'storage',
        'read-only-storage',
        'vertex',
        'index',
        'indirect',
        'indexedIndirect',
      ] as const)
      .combine('visibility1', ['compute', 'fragment'] as const)
      // The situation that the index buffer is reset by another setIndexBuffer call will be tested
      // in another test case.
      .unless(
        t =>
          (t.visibility1 === 'compute' && !IsBufferUsageInBindGroup(t.usage1)) ||
          (t.usage0 === 'index' && t.usage1 === 'index')
      )
  )
  .fn(async t => {
    const { inSamePass, hasOverlap, usage0, visibility0, usage1, visibility1 } = t.params;

    const UseBufferOnRenderPassEncoder = (
      buffer: GPUBuffer,
      index: number,
      offset: number,
      type:
        | 'uniform'
        | 'storage'
        | 'read-only-storage'
        | 'vertex'
        | 'index'
        | 'indirect'
        | 'indexedIndirect',
      bindGroupVisibility: 'compute' | 'fragment',
      renderPassEncoder: GPURenderPassEncoder
    ) => {
      switch (type) {
        case 'uniform':
        case 'storage':
        case 'read-only-storage': {
          const bindGroup = t.createBindGroupForTest(buffer, offset, type, bindGroupVisibility);
          renderPassEncoder.setBindGroup(index, bindGroup);
          break;
        }
        case 'vertex': {
          renderPassEncoder.setVertexBuffer(index, buffer, offset, kBoundBufferSize);
          break;
        }
        case 'index': {
          renderPassEncoder.setIndexBuffer(buffer, 'uint16', offset, kBoundBufferSize);
          break;
        }
        case 'indirect': {
          const renderPipeline = t.createNoOpRenderPipeline();
          renderPassEncoder.setPipeline(renderPipeline);
          renderPassEncoder.drawIndirect(buffer, offset);
          break;
        }
        case 'indexedIndirect': {
          const renderPipeline = t.createNoOpRenderPipeline();
          renderPassEncoder.setPipeline(renderPipeline);
          const indexBuffer = t.device.createBuffer({
            size: 4,
            usage: GPUBufferUsage.INDEX,
          });
          renderPassEncoder.setIndexBuffer(indexBuffer, 'uint16');
          renderPassEncoder.drawIndexedIndirect(buffer, offset);
          break;
        }
      }
    };

    const buffer = t.device.createBuffer({
      size: kBoundBufferSize * 2,
      usage:
        GPUBufferUsage.UNIFORM |
        GPUBufferUsage.STORAGE |
        GPUBufferUsage.VERTEX |
        GPUBufferUsage.INDEX |
        GPUBufferUsage.INDIRECT,
    });

    const encoder = t.device.createCommandEncoder();
    const renderPassEncoder = t.beginSimpleRenderPass(encoder);
    const offset0 = 0;
    const index0 = 0;
    UseBufferOnRenderPassEncoder(buffer, index0, offset0, usage0, visibility0, renderPassEncoder);
    const offset1 = hasOverlap ? offset0 : kBoundBufferSize;
    const index1 = 1;
    if (inSamePass) {
      UseBufferOnRenderPassEncoder(buffer, index1, offset1, usage1, visibility1, renderPassEncoder);
      renderPassEncoder.end();
    } else {
      renderPassEncoder.end();
      const anotherRenderPassEncoder = t.beginSimpleRenderPass(encoder);
      UseBufferOnRenderPassEncoder(
        buffer,
        index1,
        offset1,
        usage1,
        visibility1,
        anotherRenderPassEncoder
      );
      anotherRenderPassEncoder.end();
    }

    const fail =
      inSamePass &&
      ((usage0 === 'storage' && usage1 !== 'storage') ||
        (usage0 !== 'storage' && usage1 === 'storage'));
    t.expectValidationError(() => {
      encoder.finish();
    }, fail);
  });

g.test('subresources,buffer_usage_in_one_compute_pass_with_no_dispatch')
  .desc(
    `
Test that it is always allowed to set multiple bind groups with same buffer in a compute pass
encoder without any dispatch calls as state-setting compute pass commands, like setBindGroup(index,
bindGroup, dynamicOffsets), do not contribute directly to a usage scope.`
  )
  .params(u =>
    u
      .combine('usage0', ['uniform', 'storage', 'read-only-storage'] as const)
      .combine('usage1', ['uniform', 'storage', 'read-only-storage'] as const)
      .beginSubcases()
      .combine('visibility0', ['compute', 'fragment'] as const)
      .combine('visibility1', ['compute', 'fragment'] as const)
      .combine('hasOverlap', [true, false])
  )
  .fn(async t => {
    const { usage0, usage1, visibility0, visibility1, hasOverlap } = t.params;

    const buffer = t.device.createBuffer({
      size: kBoundBufferSize * 2,
      usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.STORAGE,
    });

    const encoder = t.device.createCommandEncoder();
    const computePassEncoder = encoder.beginComputePass();

    const offset0 = 0;
    const bindGroup0 = t.createBindGroupForTest(buffer, offset0, usage0, visibility0);
    computePassEncoder.setBindGroup(0, bindGroup0);

    const offset1 = hasOverlap ? offset0 : kBoundBufferSize;
    const bindGroup1 = t.createBindGroupForTest(buffer, offset1, usage1, visibility1);
    computePassEncoder.setBindGroup(1, bindGroup1);

    computePassEncoder.end();

    t.expectValidationError(() => {
      encoder.finish();
    }, false);
  });

g.test('subresources,buffer_usage_in_one_compute_pass_with_one_dispatch')
  .desc(
    `
Test that when one buffer is used in one compute pass encoder, its list of internal usages within
one usage scope can only be a compatible usage list. According to WebGPU SPEC, within one dispatch,
for each bind group slot that is used by the current GPUComputePipeline's layout, every subresource
referenced by that bind group is "used" in the usage scope. `
  )
  .params(u =>
    u
      .combine('usage0AccessibleInDispatch', [true, false])
      .combine('usage1AccessibleInDispatch', [true, false])
      .combine('dispatchBeforeUsage1', [true, false])
      .beginSubcases()
      .combine('usage0', ['uniform', 'storage', 'read-only-storage', 'indirect'] as const)
      .combine('visibility0', ['compute', 'fragment'] as const)
      .filter(t => {
        // The buffer with `indirect` usage is always accessible in the dispatch call.
        if (
          t.usage0 === 'indirect' &&
          (!t.usage0AccessibleInDispatch || t.visibility0 !== 'compute' || !t.dispatchBeforeUsage1)
        ) {
          return false;
        }
        if (t.usage0AccessibleInDispatch && t.visibility0 !== 'compute') {
          return false;
        }
        if (t.dispatchBeforeUsage1 && t.usage1AccessibleInDispatch) {
          return false;
        }
        return true;
      })
      .combine('usage1', ['uniform', 'storage', 'read-only-storage', 'indirect'] as const)
      .combine('visibility1', ['compute', 'fragment'] as const)
      .filter(t => {
        if (
          t.usage1 === 'indirect' &&
          (!t.usage1AccessibleInDispatch || t.visibility1 !== 'compute' || t.dispatchBeforeUsage1)
        ) {
          return false;
        }
        // When the first buffer usage is `indirect`, there has already been one dispatch call, so
        // in this test we always make the second usage inaccessible in the dispatch call.
        if (
          t.usage1AccessibleInDispatch &&
          (t.visibility1 !== 'compute' || t.usage0 === 'indirect')
        ) {
          return false;
        }
        return true;
      })
      .combine('hasOverlap', [true, false])
  )
  .fn(async t => {
    const {
      usage0AccessibleInDispatch,
      usage1AccessibleInDispatch,
      dispatchBeforeUsage1,
      usage0,
      visibility0,
      usage1,
      visibility1,
      hasOverlap,
    } = t.params;

    const buffer = t.device.createBuffer({
      size: kBoundBufferSize * 2,
      usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.STORAGE | GPUBufferUsage.INDIRECT,
    });

    const encoder = t.device.createCommandEncoder();
    const computePassEncoder = encoder.beginComputePass();

    const offset0 = 0;
    switch (usage0) {
      case 'uniform':
      case 'storage':
      case 'read-only-storage': {
        const bindGroup0 = t.createBindGroupForTest(buffer, offset0, usage0, visibility0);
        computePassEncoder.setBindGroup(0, bindGroup0);

        /*
         * setBindGroup(bindGroup0);
         * dispatchWorkgroups();
         * setBindGroup(bindGroup1);
         */
        if (dispatchBeforeUsage1) {
          let pipelineLayout: GPUPipelineLayout | undefined = undefined;
          if (usage0AccessibleInDispatch) {
            const bindGroupLayout0 = t.createBindGroupLayoutForTest(usage0, visibility0);
            pipelineLayout = t.device.createPipelineLayout({
              bindGroupLayouts: [bindGroupLayout0],
            });
          }
          const computePipeline = t.createNoOpComputePipeline(pipelineLayout);
          computePassEncoder.setPipeline(computePipeline);
          computePassEncoder.dispatchWorkgroups(1);
        }
        break;
      }
      case 'indirect': {
        /*
         * dispatchWorkgroupsIndirect(buffer);
         * setBindGroup(bindGroup1);
         */
        assert(dispatchBeforeUsage1);
        const computePipeline = t.createNoOpComputePipeline();
        computePassEncoder.setPipeline(computePipeline);
        computePassEncoder.dispatchWorkgroupsIndirect(buffer, offset0);
        break;
      }
    }

    const offset1 = hasOverlap ? offset0 : kBoundBufferSize;
    switch (usage1) {
      case 'uniform':
      case 'storage':
      case 'read-only-storage': {
        const bindGroup1 = t.createBindGroupForTest(buffer, offset1, usage1, visibility1);
        const bindGroupIndex = usage0AccessibleInDispatch ? 1 : 0;
        computePassEncoder.setBindGroup(bindGroupIndex, bindGroup1);

        /*
         * setBindGroup(bindGroup0);
         * setBindGroup(bindGroup1);
         * dispatchWorkgroups();
         */
        if (!dispatchBeforeUsage1) {
          const bindGroupLayouts: GPUBindGroupLayout[] = [];
          if (usage0AccessibleInDispatch && usage0 !== 'indirect') {
            const bindGroupLayout0 = t.createBindGroupLayoutForTest(usage0, visibility0);
            bindGroupLayouts.push(bindGroupLayout0);
          }
          if (usage1AccessibleInDispatch) {
            const bindGroupLayout1 = t.createBindGroupLayoutForTest(usage1, visibility1);
            bindGroupLayouts.push(bindGroupLayout1);
          }
          const pipelineLayout: GPUPipelineLayout | undefined = bindGroupLayouts
            ? t.device.createPipelineLayout({
                bindGroupLayouts,
              })
            : undefined;
          const computePipeline = t.createNoOpComputePipeline(pipelineLayout);
          computePassEncoder.setPipeline(computePipeline);
          computePassEncoder.dispatchWorkgroups(1);
        }
        break;
      }
      case 'indirect': {
        /*
         * setBindGroup(bindGroup0);
         * dispatchWorkgroupsIndirect(buffer);
         */
        assert(!dispatchBeforeUsage1);
        let pipelineLayout: GPUPipelineLayout | undefined = undefined;
        if (usage0AccessibleInDispatch) {
          assert(usage0 !== 'indirect');
          pipelineLayout = t.device.createPipelineLayout({
            bindGroupLayouts: [t.createBindGroupLayoutForTest(usage0, visibility0)],
          });
        }
        const computePipeline = t.createNoOpComputePipeline(pipelineLayout);
        computePassEncoder.setPipeline(computePipeline);
        computePassEncoder.dispatchWorkgroupsIndirect(buffer, offset1);
        break;
      }
    }
    computePassEncoder.end();

    const usageHasConflict =
      (usage0 === 'storage' && usage1 !== 'storage') ||
      (usage0 !== 'storage' && usage1 === 'storage');
    const fail =
      usageHasConflict &&
      visibility0 === 'compute' &&
      visibility1 === 'compute' &&
      usage0AccessibleInDispatch &&
      usage1AccessibleInDispatch;
    t.expectValidationError(() => {
      encoder.finish();
    }, fail);
  });

g.test('subresources,buffer_usage_in_compute_pass_with_two_dispatches')
  .desc(
    `
Test that it is always allowed to use one buffer in different dispatch calls as in WebGPU SPEC,
within one dispatch, for each bind group slot that is used by the current GPUComputePipeline's
layout, every subresource referenced by that bind group is "used" in the usage scope, and different
dispatch calls refer to different usage scopes.`
  )
  .params(u =>
    u
      .combine('usage0', ['uniform', 'storage', 'read-only-storage', 'indirect'] as const)
      .combine('usage1', ['uniform', 'storage', 'read-only-storage', 'indirect'] as const)
      .beginSubcases()
      .combine('inSamePass', [true, false])
      .combine('hasOverlap', [true, false])
  )
  .fn(async t => {
    const { usage0, usage1, inSamePass, hasOverlap } = t.params;

    const UseBufferOnComputePassEncoder = (
      computePassEncoder: GPUComputePassEncoder,
      buffer: GPUBuffer,
      usage: 'uniform' | 'storage' | 'read-only-storage' | 'indirect',
      offset: number
    ) => {
      switch (usage) {
        case 'uniform':
        case 'storage':
        case 'read-only-storage': {
          const bindGroup = t.createBindGroupForTest(buffer, offset, usage, 'compute');
          computePassEncoder.setBindGroup(0, bindGroup);

          const bindGroupLayout = t.createBindGroupLayoutForTest(usage, 'compute');
          const pipelineLayout = t.device.createPipelineLayout({
            bindGroupLayouts: [bindGroupLayout],
          });
          const computePipeline = t.createNoOpComputePipeline(pipelineLayout);
          computePassEncoder.setPipeline(computePipeline);
          computePassEncoder.dispatchWorkgroups(1);
          break;
        }
        case 'indirect': {
          const computePipeline = t.createNoOpComputePipeline();
          computePassEncoder.setPipeline(computePipeline);
          computePassEncoder.dispatchWorkgroupsIndirect(buffer, offset);
          break;
        }
        default:
          unreachable();
          break;
      }
    };

    const buffer = t.device.createBuffer({
      size: kBoundBufferSize * 2,
      usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.STORAGE | GPUBufferUsage.INDIRECT,
    });

    const encoder = t.device.createCommandEncoder();
    const computePassEncoder = encoder.beginComputePass();

    const offset0 = 0;
    const offset1 = hasOverlap ? offset0 : kBoundBufferSize;
    UseBufferOnComputePassEncoder(computePassEncoder, buffer, usage0, offset0);

    if (inSamePass) {
      UseBufferOnComputePassEncoder(computePassEncoder, buffer, usage1, offset1);
      computePassEncoder.end();
    } else {
      computePassEncoder.end();
      const anotherComputePassEncoder = encoder.beginComputePass();
      UseBufferOnComputePassEncoder(anotherComputePassEncoder, buffer, usage1, offset1);
      anotherComputePassEncoder.end();
    }

    t.expectValidationError(() => {
      encoder.finish();
    }, false);
  });
