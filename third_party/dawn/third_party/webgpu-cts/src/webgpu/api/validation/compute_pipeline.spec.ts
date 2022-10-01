export const description = `
createComputePipeline and createComputePipelineAsync validation tests.

Note: entry point matching tests are in shader_module/entry_point.spec.ts
`;

import { makeTestGroup } from '../../../common/framework/test_group.js';
import { TShaderStage, getShaderWithEntryPoint } from '../../util/shader.js';

import { ValidationTest } from './validation_test.js';

class F extends ValidationTest {
  getShaderModule(
    shaderStage: TShaderStage = 'compute',
    entryPoint: string = 'main'
  ): GPUShaderModule {
    return this.device.createShaderModule({
      code: getShaderWithEntryPoint(shaderStage, entryPoint),
    });
  }
}

export const g = makeTestGroup(F);

g.test('basic')
  .desc(
    `
Control case for createComputePipeline and createComputePipelineAsync.
Call the API with valid compute shader and matching valid entryPoint, making sure that the test function working well.
`
  )
  .params(u => u.combine('isAsync', [true, false]))
  .fn(async t => {
    const { isAsync } = t.params;
    t.doCreateComputePipelineTest(isAsync, true, {
      layout: 'auto',
      compute: { module: t.getShaderModule('compute', 'main'), entryPoint: 'main' },
    });
  });

g.test('shader_module,invalid')
  .desc(
    `
Tests calling createComputePipeline(Async) with a invalid compute shader, and check that the APIs catch this error.
`
  )
  .params(u => u.combine('isAsync', [true, false]))
  .fn(async t => {
    const { isAsync } = t.params;
    t.doCreateComputePipelineTest(isAsync, false, {
      layout: 'auto',
      compute: {
        module: t.createInvalidShaderModule(),
        entryPoint: 'main',
      },
    });
  });

g.test('shader_module,compute')
  .desc(
    `
Tests calling createComputePipeline(Async) with valid but different stage shader and matching entryPoint,
and check that the APIs only accept compute shader.
`
  )
  .params(u =>
    u //
      .combine('isAsync', [true, false])
      .combine('shaderModuleStage', ['compute', 'vertex', 'fragment'] as TShaderStage[])
  )
  .fn(async t => {
    const { isAsync, shaderModuleStage } = t.params;
    const descriptor = {
      layout: 'auto' as const,
      compute: {
        module: t.getShaderModule(shaderModuleStage, 'main'),
        entryPoint: 'main',
      },
    };
    t.doCreateComputePipelineTest(isAsync, shaderModuleStage === 'compute', descriptor);
  });

g.test('shader_module,device_mismatch')
  .desc(
    'Tests createComputePipeline(Async) cannot be called with a shader module created from another device'
  )
  .paramsSubcasesOnly(u => u.combine('isAsync', [true, false]).combine('mismatched', [true, false]))
  .beforeAllSubcases(t => {
    t.selectMismatchedDeviceOrSkipTestCase(undefined);
  })
  .fn(async t => {
    const { isAsync, mismatched } = t.params;

    const device = mismatched ? t.mismatchedDevice : t.device;

    const module = device.createShaderModule({
      code: '@compute @workgroup_size(1) fn main() {}',
    });

    const descriptor = {
      layout: 'auto' as const,
      compute: {
        module,
        entryPoint: 'main',
      },
    };

    t.doCreateComputePipelineTest(isAsync, !mismatched, descriptor);
  });

g.test('pipeline_layout,device_mismatch')
  .desc(
    'Tests createComputePipeline(Async) cannot be called with a pipeline layout created from another device'
  )
  .paramsSubcasesOnly(u => u.combine('isAsync', [true, false]).combine('mismatched', [true, false]))
  .beforeAllSubcases(t => {
    t.selectMismatchedDeviceOrSkipTestCase(undefined);
  })
  .fn(async t => {
    const { isAsync, mismatched } = t.params;
    const device = mismatched ? t.mismatchedDevice : t.device;

    const layout = device.createPipelineLayout({ bindGroupLayouts: [] });

    const descriptor = {
      layout,
      compute: {
        module: t.getShaderModule('compute', 'main'),
        entryPoint: 'main',
      },
    };

    t.doCreateComputePipelineTest(isAsync, !mismatched, descriptor);
  });

g.test('limits,workgroup_storage_size')
  .desc(
    `
Tests calling createComputePipeline(Async) validation for compute using <= device.limits.maxComputeWorkgroupStorageSize bytes of workgroup storage.
`
  )
  .params(u =>
    u //
      .combine('isAsync', [true, false])
      .combineWithParams([
        { type: 'vec4<f32>', _typeSize: 16 },
        { type: 'mat4x4<f32>', _typeSize: 64 },
      ])
      .beginSubcases()
      .combine('countDeltaFromLimit', [0, 1])
  )
  .fn(async t => {
    const { isAsync, type, _typeSize, countDeltaFromLimit } = t.params;
    const countAtLimit = Math.floor(t.device.limits.maxComputeWorkgroupStorageSize / _typeSize);
    const count = countAtLimit + countDeltaFromLimit;

    const descriptor = {
      layout: 'auto' as const,
      compute: {
        module: t.device.createShaderModule({
          code: `
          var<workgroup> data: array<${type}, ${count}>;
          @compute @workgroup_size(64) fn main () {
            _ = data;
          }
          `,
        }),
        entryPoint: 'main',
      },
    };
    t.doCreateComputePipelineTest(isAsync, count <= countAtLimit, descriptor);
  });

g.test('limits,invocations_per_workgroup')
  .desc(
    `
Tests calling createComputePipeline(Async) validation for compute using <= device.limits.maxComputeInvocationsPerWorkgroup per workgroup.
`
  )
  .params(u =>
    u //
      .combine('isAsync', [true, false])
      .combine('size', [
        // Assume maxComputeWorkgroupSizeX/Y >= 129, maxComputeWorkgroupSizeZ >= 33
        [128, 1, 2],
        [129, 1, 2],
        [2, 128, 1],
        [2, 129, 1],
        [1, 8, 32],
        [1, 8, 33],
      ])
  )
  .fn(async t => {
    const { isAsync, size } = t.params;

    const descriptor = {
      layout: 'auto' as const,
      compute: {
        module: t.device.createShaderModule({
          code: `
          @compute @workgroup_size(${size.join(',')}) fn main () {
          }
          `,
        }),
        entryPoint: 'main',
      },
    };

    t.doCreateComputePipelineTest(
      isAsync,
      size[0] * size[1] * size[2] <= t.device.limits.maxComputeInvocationsPerWorkgroup,
      descriptor
    );
  });

g.test('limits,invocations_per_workgroup,each_component')
  .desc(
    `
Tests calling createComputePipeline(Async) validation for compute workgroup_size attribute has each component no more than their limits.
`
  )
  .params(u =>
    u //
      .combine('isAsync', [true, false])
      .combine('size', [
        // Assume maxComputeInvocationsPerWorkgroup >= 256
        [64],
        [256, 1, 1],
        [257, 1, 1],
        [1, 256, 1],
        [1, 257, 1],
        [1, 1, 63],
        [1, 1, 64],
        [1, 1, 65],
      ])
  )
  .fn(async t => {
    const { isAsync, size } = t.params;

    const descriptor = {
      layout: 'auto' as const,
      compute: {
        module: t.device.createShaderModule({
          code: `
          @compute @workgroup_size(${size.join(',')}) fn main () {
          }
          `,
        }),
        entryPoint: 'main',
      },
    };

    size[1] = size[1] ?? 1;
    size[2] = size[2] ?? 1;

    const _success =
      size[0] <= t.device.limits.maxComputeWorkgroupSizeX &&
      size[1] <= t.device.limits.maxComputeWorkgroupSizeY &&
      size[2] <= t.device.limits.maxComputeWorkgroupSizeZ;
    t.doCreateComputePipelineTest(isAsync, _success, descriptor);
  });
