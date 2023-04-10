import { range } from '../../../../../common/util/util.js';

import {
  kCreatePipelineTypes,
  kCreatePipelineAsyncTypes,
  kEncoderTypes,
  kLimitBaseParams,
  makeLimitTestGroup,
} from './limit_utils.js';

const limit = 'maxBindGroups';
export const { g, description } = makeLimitTestGroup(limit);

g.test('createPipelineLayout,at_over')
  .desc(`Test using createPipelineLayout at and over ${limit} limit`)
  .params(kLimitBaseParams)
  .fn(async t => {
    const { limitTest, testValueName } = t.params;
    await t.testDeviceWithRequestedLimits(
      limitTest,
      testValueName,
      async ({ device, testValue, shouldError }) => {
        const bindGroupLayouts = range(testValue, (i: number) =>
          device.createBindGroupLayout({
            entries: [
              {
                binding: 0,
                visibility: GPUShaderStage.VERTEX,
                buffer: {},
              },
            ],
          })
        );

        await t.expectValidationError(() => {
          device.createPipelineLayout({ bindGroupLayouts });
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

        const code = t.getGroupIndexWGSLForPipelineType(createPipelineType, lastIndex);
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

        const code = t.getGroupIndexWGSLForPipelineType(createPipelineAsyncType, lastIndex);
        const module = device.createShaderModule({ code });

        const promise = t.createPipelineAsync(createPipelineAsyncType, module);
        await t.shouldRejectConditionally('OperationError', promise, shouldError);
      }
    );
  });

g.test('setBindGroup,at_over')
  .desc(`Test using setBindGroup at and over ${limit} limit`)
  .params(kLimitBaseParams.combine('encoderType', kEncoderTypes))
  .fn(async t => {
    const { limitTest, testValueName, encoderType } = t.params;
    await t.testDeviceWithRequestedLimits(
      limitTest,
      testValueName,
      async ({ testValue, actualLimit, shouldError }) => {
        const lastIndex = testValue - 1;
        await t.testGPUBindingCommandsMixin(
          encoderType,
          ({ mixin, bindGroup }) => {
            mixin.setBindGroup(lastIndex, bindGroup);
          },
          shouldError,
          `shouldError: ${shouldError}, actualLimit: ${actualLimit}, testValue: ${lastIndex}`
        );
      }
    );
  });
