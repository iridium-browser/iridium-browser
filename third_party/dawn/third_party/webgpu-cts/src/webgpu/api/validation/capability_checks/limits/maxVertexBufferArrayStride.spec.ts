import { roundDown } from '../../../../util/math.js';

import { kLimitBaseParams, makeLimitTestGroup, LimitValueTest, TestValue } from './limit_utils.js';

function getPipelineDescriptor(device: GPUDevice, testValue: number): GPURenderPipelineDescriptor {
  const code = `
  @vertex fn vs(@location(0) v: f32) -> @builtin(position) vec4f {
    return vec4f(v);
  }
  `;
  const module = device.createShaderModule({ code });
  return {
    layout: 'auto',
    vertex: {
      module,
      entryPoint: 'vs',
      buffers: [
        {
          arrayStride: testValue,
          attributes: [
            {
              shaderLocation: 0,
              offset: 0,
              format: 'float32',
            },
          ],
        },
      ],
    },
  };
}

const kMinAttributeStride = 4;

function getDeviceLimitToRequest(
  limitValueTest: LimitValueTest,
  defaultLimit: number,
  maximumLimit: number
) {
  switch (limitValueTest) {
    case 'atDefault':
      return defaultLimit;
    case 'underDefault':
      return defaultLimit - kMinAttributeStride;
    case 'betweenDefaultAndMaximum':
      return Math.min(
        defaultLimit,
        roundDown(((defaultLimit + maximumLimit) / 2) | 0, kMinAttributeStride)
      );
    case 'atMaximum':
      return maximumLimit;
    case 'overMaximum':
      return maximumLimit + kMinAttributeStride;
  }
}

function getTestValue(testValueName: TestValue, requestedLimit: number) {
  switch (testValueName) {
    case 'atLimit':
      return requestedLimit;
    case 'overLimit':
      return requestedLimit + kMinAttributeStride;
  }
}

function getDeviceLimitToRequestAndValueToTest(
  limitValueTest: LimitValueTest,
  testValueName: TestValue,
  defaultLimit: number,
  maximumLimit: number
) {
  const requestedLimit = getDeviceLimitToRequest(limitValueTest, defaultLimit, maximumLimit);
  return {
    requestedLimit,
    testValue: getTestValue(testValueName, requestedLimit),
  };
}

/*
Note: We need to request +4 (vs the default +1) because otherwise we may trigger the wrong validation
of the arrayStride not being a multiple of 4
*/
const limit = 'maxVertexBufferArrayStride';
export const { g, description } = makeLimitTestGroup(limit);

g.test('createRenderPipeline,at_over')
  .desc(`Test using createRenderPipeline at and over ${limit} limit`)
  .params(kLimitBaseParams)
  .fn(async t => {
    const { limitTest, testValueName } = t.params;
    const { adapter, defaultLimit, maximumLimit } = await t.getAdapterAndLimits();
    const { requestedLimit, testValue } = getDeviceLimitToRequestAndValueToTest(
      limitTest,
      testValueName,
      defaultLimit,
      maximumLimit
    );

    await t.testDeviceWithSpecificLimits(
      adapter,
      requestedLimit,
      testValue,
      async ({ device, testValue, shouldError }) => {
        const pipelineDescriptor = getPipelineDescriptor(device, testValue);

        await t.expectValidationError(() => {
          device.createRenderPipeline(pipelineDescriptor);
        }, shouldError);
      }
    );
  });

g.test('createRenderPipelineAsync,at_over')
  .desc(`Test using createRenderPipelineAsync at and over ${limit} limit`)
  .params(kLimitBaseParams)
  .fn(async t => {
    const { limitTest, testValueName } = t.params;
    const { adapter, defaultLimit, maximumLimit } = await t.getAdapterAndLimits();
    const { requestedLimit, testValue } = getDeviceLimitToRequestAndValueToTest(
      limitTest,
      testValueName,
      defaultLimit,
      maximumLimit
    );
    await t.testDeviceWithSpecificLimits(
      adapter,
      requestedLimit,
      testValue,
      async ({ device, testValue, shouldError }) => {
        const pipelineDescriptor = getPipelineDescriptor(device, testValue);
        await t.shouldRejectConditionally(
          'OperationError',
          device.createRenderPipelineAsync(pipelineDescriptor),
          shouldError
        );
      }
    );
  });
