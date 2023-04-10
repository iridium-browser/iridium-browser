import {
  kCreatePipelineTypes,
  kCreatePipelineAsyncTypes,
  kLimitBaseParams,
  makeLimitTestGroup,
} from './limit_utils.js';

const limit = 'maxBindingsPerBindGroup';
export const { g, description } = makeLimitTestGroup(limit);

g.test('createBindGroupLayout,at_over')
  .desc(`Test using createBindGroupLayout at and over ${limit} limit`)
  .params(kLimitBaseParams)
  .fn(async t => {
    const { limitTest, testValueName } = t.params;
    await t.testDeviceWithRequestedLimits(
      limitTest,
      testValueName,
      async ({ device, testValue, shouldError }) => {
        await t.expectValidationError(() => {
          device.createBindGroupLayout({
            entries: [
              {
                binding: testValue - 1,
                visibility: GPUShaderStage.VERTEX,
                buffer: {},
              },
            ],
          });
        }, shouldError);
      }
    );
  });

g.test('createPipeline,at_over')
  .desc(`Test using createRenderPipeline and createComputePipeline at and over ${limit} limit`)
  .params(kLimitBaseParams.combine('createPipelineType', kCreatePipelineTypes))
  .fn(async t => {
    const { limitTest, testValueName, createPipelineType } = t.params;
    await t.testDeviceWithRequestedLimits(
      limitTest,
      testValueName,
      async ({ device, testValue, shouldError }) => {
        const lastIndex = testValue - 1;

        const code = t.getBindingIndexWGSLForPipelineType(createPipelineType, lastIndex);
        const module = device.createShaderModule({ code });

        await t.expectValidationError(() => {
          t.createPipeline(createPipelineType, module);
        }, shouldError);
      }
    );
  });

g.test('createPipelineAsync,at_over')
  .desc(
    `Test using createRenderPipelineAsync and createComputePipelineAsync at and over ${limit} limit`
  )
  .params(kLimitBaseParams.combine('createPipelineAsyncType', kCreatePipelineAsyncTypes))
  .fn(async t => {
    const { limitTest, testValueName, createPipelineAsyncType } = t.params;

    await t.testDeviceWithRequestedLimits(
      limitTest,
      testValueName,
      async ({ device, testValue, shouldError }) => {
        const lastIndex = testValue - 1;

        const code = t.getBindingIndexWGSLForPipelineType(createPipelineAsyncType, lastIndex);
        const module = device.createShaderModule({ code });

        const promise = t.createPipelineAsync(createPipelineAsyncType, module);
        await t.shouldRejectConditionally('OperationError', promise, shouldError);
      }
    );
  });
