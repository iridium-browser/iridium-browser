import { kUnitCaseParamsBuilder } from '../../../../../common/framework/params_builder.js';
import { makeTestGroup } from '../../../../../common/framework/test_group.js';
import { keysOf } from '../../../../../common/util/data_tables.js';
import { getGPU } from '../../../../../common/util/navigator_gpu.js';
import { assert } from '../../../../../common/util/util.js';
import { kLimitInfo } from '../../../../capability_info.js';
import { GPUTestBase } from '../../../../gpu_test.js';

type GPUSupportedLimit = keyof GPUSupportedLimits;

const CreatePipelineTypes = {
  createRenderPipeline: true,
  createComputePipeline: true,
};
export type CreatePipelineType = keyof typeof CreatePipelineTypes;

export const kCreatePipelineTypes = [
  'createRenderPipeline',
  'createComputePipeline',
] as CreatePipelineType[];

const CreatePipelineAsyncTypes = {
  createRenderPipelineAsync: true,
  createComputePipelineAsync: true,
};
export type CreatePipelineAsyncType = keyof typeof CreatePipelineAsyncTypes;

export const kCreatePipelineAsyncTypes = [
  'createRenderPipelineAsync',
  'createComputePipelineAsync',
] as CreatePipelineAsyncType[];

const RenderEncoderTypes = {
  render: true,
  renderBundle: true,
};
export type RenderEncoderType = keyof typeof RenderEncoderTypes;
export const kRenderEncoderTypes = keysOf(RenderEncoderTypes);

const EncoderTypes = {
  compute: true,
  render: true,
  renderBundle: true,
};
export type EncoderType = keyof typeof EncoderTypes;
export const kEncoderTypes = keysOf(EncoderTypes);

export const TestValues = {
  atLimit: true,
  overLimit: true,
};
export type TestValue = keyof typeof TestValues;
export const kTestValueKeys = keysOf(TestValues);

export function getTestValue(limit: number, testValue: TestValue) {
  switch (testValue) {
    case 'atLimit':
      return limit;
    case 'overLimit':
      return limit + 1;
  }
}

export const LimitValueTests = {
  atDefault: true,
  underDefault: true,
  betweenDefaultAndMaximum: true,
  atMaximum: true,
  overMaximum: true,
};
export type LimitValueTest = keyof typeof LimitValueTests;
export const kLimitValueTestKeys = keysOf(LimitValueTests);

function getLimitValue(defaultLimit: number, maximumLimit: number, limitValueTest: LimitValueTest) {
  switch (limitValueTest) {
    case 'atDefault':
      return defaultLimit;
    case 'underDefault':
      return defaultLimit - 1;
    case 'betweenDefaultAndMaximum':
      return ((defaultLimit + maximumLimit) / 2) | 0;
    case 'atMaximum':
      return maximumLimit;
    case 'overMaximum':
      return maximumLimit + 1;
  }
}

export type DeviceAndLimits = {
  device: GPUDevice;
  defaultLimit: number;
  maximumLimit: number;
  requestedLimit: number;
  actualLimit: number;
};

export type SpecificLimitTestInputs = DeviceAndLimits & {
  testValue: number;
  shouldError: boolean;
};

export type LimitTestInputs = SpecificLimitTestInputs & {
  testValueName: TestValue;
};

/**
 * Adds the default parameters to a limit test
 */
export const kLimitBaseParams = kUnitCaseParamsBuilder
  .combine('limitTest', kLimitValueTestKeys)
  .beginSubcases()
  .combine('testValueName', kTestValueKeys);

export class LimitTestsImpl extends GPUTestBase {
  _device: GPUDevice | undefined = undefined;
  limit: GPUSupportedLimit = '' as GPUSupportedLimit;

  get device(): GPUDevice {
    assert(this._device !== undefined, 'device is only valid in _testThenDestroyDevice callback');
    return this._device;
  }

  async requestDeviceWithLimits(
    adapter: GPUAdapter,
    requiredLimits: Record<string, number>,
    shouldReject: boolean
  ) {
    if (shouldReject) {
      this.shouldReject('OperationError', adapter.requestDevice({ requiredLimits }));
      return undefined;
    } else {
      return await adapter.requestDevice({ requiredLimits });
    }
  }

  async getAdapterAndLimits() {
    const limit = this.limit;
    const gpu = getGPU();
    const adapter = await gpu.requestAdapter();
    assert(!!adapter);

    const defaultLimit = (kLimitInfo as Record<string, { default: number }>)[limit].default;
    const maximumLimit = adapter.limits[limit] as number;
    assert(!Number.isNaN(defaultLimit));
    assert(!Number.isNaN(maximumLimit));

    return { adapter, defaultLimit, maximumLimit };
  }

  /**
   * Gets a device with the adapter a requested limit and checks that that limit
   * is correct or that the device failed to create if the requested limit is
   * beyond the maximum supported by the device.
   */
  async _getDeviceWithSpecificLimit(
    adapter: GPUAdapter,
    requestedLimit: number
  ): Promise<DeviceAndLimits | undefined> {
    const limit = this.limit;

    const defaultLimit = (kLimitInfo as Record<string, { default: number }>)[limit].default;
    const maximumLimit = adapter.limits[limit] as number;
    assert(!Number.isNaN(defaultLimit));
    assert(!Number.isNaN(maximumLimit));

    const requiredLimits: Record<string, number> = {};
    requiredLimits[limit] = requestedLimit;
    const shouldReject = requestedLimit > maximumLimit;

    const device = await this.requestDeviceWithLimits(adapter, requiredLimits, shouldReject);
    const actualLimit = (device ? device.limits[limit] : 0) as number;

    if (shouldReject) {
      this.expect(!device);
    } else {
      if (requestedLimit <= defaultLimit) {
        this.expect(actualLimit === defaultLimit);
      } else {
        this.expect(actualLimit === maximumLimit);
      }
    }

    return device ? { device, defaultLimit, maximumLimit, requestedLimit, actualLimit } : undefined;
  }

  /**
   * Gets a device with the adapter a requested limit and checks that that limit
   * is correct or that the device failed to create if the requested limit is
   * beyond the maximum supported by the device.
   */
  async _getDeviceWithRequestedLimit(
    limitValueTest: LimitValueTest
  ): Promise<DeviceAndLimits | undefined> {
    const { adapter, defaultLimit, maximumLimit } = await this.getAdapterAndLimits();

    const requestedLimit = getLimitValue(defaultLimit, maximumLimit, limitValueTest);
    return this._getDeviceWithSpecificLimit(adapter, requestedLimit);
  }

  /**
   * Call the given function and check no WebGPU errors are leaked
   */
  async _testThenDestroyDevice(
    deviceAndLimits: DeviceAndLimits,
    testValue: number,
    fn: (inputs: SpecificLimitTestInputs) => void | Promise<void>
  ) {
    assert(!this._device);

    const { device, actualLimit } = deviceAndLimits;
    this._device = device;
    const shouldError = testValue > actualLimit;

    device.pushErrorScope('internal');
    device.pushErrorScope('out-of-memory');
    device.pushErrorScope('validation');

    await fn({ ...deviceAndLimits, testValue, shouldError });

    const validationError = await device.popErrorScope();
    const outOfMemoryError = await device.popErrorScope();
    const internalError = await device.popErrorScope();

    this.expect(!validationError, validationError?.message || '');
    this.expect(!outOfMemoryError, outOfMemoryError?.message || '');
    this.expect(!internalError, internalError?.message || '');

    device.destroy();
    this._device = undefined;
  }

  /**
   * Creates a device with a specific limit.
   * If the limit of over the maximum we expect an exception
   * If the device is created then we call a test function, checking
   * that the function does not leak any GPU errors.
   */
  async testDeviceWithSpecificLimits(
    adapter: GPUAdapter,
    deviceLimitValue: number,
    testValue: number,
    fn: (inputs: SpecificLimitTestInputs) => void | Promise<void>
  ) {
    assert(!this._device);

    const deviceAndLimits = await this._getDeviceWithSpecificLimit(adapter, deviceLimitValue);
    // If we request over the limit requestDevice will throw
    if (!deviceAndLimits) {
      return;
    }

    await this._testThenDestroyDevice(deviceAndLimits, testValue, fn);
  }

  /**
   * Creates a device with the limit defined by LimitValueTest.
   * If the limit of over the maximum we expect an exception
   * If the device is created then we call a test function, checking
   * that the function does not leak any GPU errors.
   */
  async testDeviceWithRequestedLimits(
    limitTest: LimitValueTest,
    testValueName: TestValue,
    fn: (inputs: LimitTestInputs) => void | Promise<void>
  ) {
    assert(!this._device);

    const deviceAndLimits = await this._getDeviceWithRequestedLimit(limitTest);
    // If we request over the limit requestDevice will throw
    if (!deviceAndLimits) {
      return;
    }

    const { actualLimit } = deviceAndLimits;
    const testValue = getTestValue(actualLimit, testValueName);

    await this._testThenDestroyDevice(
      deviceAndLimits,
      testValue,
      async (inputs: SpecificLimitTestInputs) => {
        await fn({ ...inputs, testValueName });
      }
    );
  }

  /**
   * Calls a function that expects a GPU error if shouldError is true
   */
  // MAINTENANCE_TODO: Remove this duplicated code with GPUTest if possible
  async expectGPUErrorAsync<R>(
    filter: GPUErrorFilter,
    fn: () => R,
    shouldError: boolean = true,
    msg = ''
  ): Promise<R> {
    const { device } = this;

    device.pushErrorScope(filter);
    const returnValue = fn();
    if (returnValue instanceof Promise) {
      await returnValue;
    }

    const error = await device.popErrorScope();
    this.expect(
      !!error === shouldError,
      `${error?.message || 'no error when one was expected'}: ${msg}`
    );

    return returnValue;
  }

  /** Expect that the provided promise rejects, with the provided exception name. */
  async shouldRejectConditionally(
    expectedName: string,
    p: Promise<unknown>,
    shouldReject: boolean,
    msg?: string
  ): Promise<void> {
    if (shouldReject) {
      this.shouldReject(expectedName, p, msg);
    } else {
      this.shouldResolve(p, msg);
    }

    // We need to explicitly wait for the promise because the device may be
    // destroyed immediately after returning from this function.
    try {
      await p;
    } catch (e) {
      //
    }
  }

  /**
   * Calls a function that expects a validation error if shouldError is true
   */
  async expectValidationError<R>(fn: () => R, shouldError: boolean = true, msg = ''): Promise<R> {
    return this.expectGPUErrorAsync('validation', fn, shouldError, msg);
  }

  /**
   * Calls a function that expects to not generate a validation error
   */
  async expectNoValidationError<R>(fn: () => R, msg = ''): Promise<R> {
    return this.expectGPUErrorAsync('validation', fn, false, msg);
  }

  /**
   * Calls a function that might expect a validation error.
   * if shouldError is true then expect a validation error,
   * if shouldError is false then ignore out-of-memory errors.
   */
  async testForValidationErrorWithPossibleOutOfMemoryError<R>(
    fn: () => R,
    shouldError: boolean = true,
    msg = ''
  ): Promise<R> {
    const { device } = this;

    if (!shouldError) {
      device.pushErrorScope('out-of-memory');
      const result = fn();
      await device.popErrorScope();
      return result;
    }

    // Validation should fail before out-of-memory so there is no need to check
    // for out-of-memory here.
    device.pushErrorScope('validation');
    const returnValue = fn();
    const validationError = await device.popErrorScope();

    this.expect(
      !!validationError,
      `${validationError?.message || 'no error when one was expected'}: ${msg}`
    );

    return returnValue;
  }

  getGroupIndexWGSLForPipelineType(
    pipelineType: CreatePipelineType | CreatePipelineAsyncType,
    groupIndex: number
  ) {
    switch (pipelineType) {
      case 'createRenderPipeline':
      case 'createRenderPipelineAsync':
        return `
          @group(${groupIndex}) @binding(0) var<uniform> v: f32;
          @vertex fn main() -> @builtin(position) vec4f {
            return vec4f(v);
          }
        `;
      case 'createComputePipeline':
      case 'createComputePipelineAsync':
        return `
          @group(0) @binding(0) var<storage, read_write> d: f32;
          @group(${groupIndex}) @binding(0) var<uniform> v: f32;
          @compute @workgroup_size(1) fn main() {
            d = v;
          }
        `;
        break;
    }
  }

  getBindingIndexWGSLForPipelineType(
    pipelineType: CreatePipelineType | CreatePipelineAsyncType,
    bindingIndex: number
  ) {
    switch (pipelineType) {
      case 'createRenderPipeline':
      case 'createRenderPipelineAsync':
        return `
          @group(0) @binding(${bindingIndex}) var<uniform> v: f32;
          @vertex fn main() -> @builtin(position) vec4f {
            return vec4f(v);
          }
        `;
      case 'createComputePipeline':
      case 'createComputePipelineAsync':
        return `
          @group(0) @binding(0) var<storage, read_write> d: f32;
          @group(0) @binding(${bindingIndex}) var<uniform> v: f32;
          @compute @workgroup_size(1) fn main() {
            d = v;
          }
        `;
        break;
    }
  }

  createPipeline(createPipelineType: CreatePipelineType, module: GPUShaderModule) {
    const { device } = this;

    switch (createPipelineType) {
      case 'createRenderPipeline':
        return device.createRenderPipeline({
          layout: 'auto',
          vertex: {
            module,
            entryPoint: 'main',
          },
        });
        break;
      case 'createComputePipeline':
        return device.createComputePipeline({
          layout: 'auto',
          compute: {
            module,
            entryPoint: 'main',
          },
        });
        break;
    }
  }

  createPipelineAsync(createPipelineAsyncType: CreatePipelineAsyncType, module: GPUShaderModule) {
    const { device } = this;

    switch (createPipelineAsyncType) {
      case 'createRenderPipelineAsync':
        return device.createRenderPipelineAsync({
          layout: 'auto',
          vertex: {
            module,
            entryPoint: 'main',
          },
        });
      case 'createComputePipelineAsync':
        return device.createComputePipelineAsync({
          layout: 'auto',
          compute: {
            module,
            entryPoint: 'main',
          },
        });
    }
  }

  /**
   * Creates an GPURenderCommandsMixin setup with some initial state.
   */
  _getGPURenderCommandsMixin(encoderType: RenderEncoderType) {
    const { device } = this;

    switch (encoderType) {
      case 'render': {
        const buffer = device.createBuffer({
          size: 16,
          usage: GPUBufferUsage.UNIFORM,
        });

        const texture = device.createTexture({
          size: [1, 1],
          format: 'rgba8unorm',
          usage: GPUTextureUsage.RENDER_ATTACHMENT,
        });

        const layout = device.createBindGroupLayout({
          entries: [
            {
              binding: 0,
              visibility: GPUShaderStage.VERTEX,
              buffer: {},
            },
          ],
        });

        const bindGroup = device.createBindGroup({
          layout,
          entries: [
            {
              binding: 0,
              resource: { buffer },
            },
          ],
        });

        const encoder = device.createCommandEncoder();
        const mixin = encoder.beginRenderPass({
          colorAttachments: [
            {
              view: texture.createView(),
              loadOp: 'clear',
              storeOp: 'store',
            },
          ],
        });

        return {
          mixin,
          bindGroup,
          prep() {
            mixin.end();
          },
          test() {
            encoder.finish();
          },
          cleanup() {
            buffer.destroy();
            texture.destroy();
          },
        };
        break;
      }

      case 'renderBundle': {
        const buffer = device.createBuffer({
          size: 16,
          usage: GPUBufferUsage.UNIFORM,
        });

        const layout = device.createBindGroupLayout({
          entries: [
            {
              binding: 0,
              visibility: GPUShaderStage.VERTEX,
              buffer: {},
            },
          ],
        });

        const bindGroup = device.createBindGroup({
          layout,
          entries: [
            {
              binding: 0,
              resource: { buffer },
            },
          ],
        });

        const mixin = device.createRenderBundleEncoder({
          colorFormats: ['rgba8unorm'],
        });

        return {
          mixin,
          bindGroup,
          prep() {},
          test() {
            mixin.finish();
          },
          cleanup() {
            buffer.destroy();
          },
        };
        break;
      }
    }
  }

  /**
   * Tests a method on GPURenderCommandsMixin
   * The function will be called with the mixin.
   */
  async testGPURenderCommandsMixin(
    encoderType: RenderEncoderType,
    fn: ({ mixin }: { mixin: GPURenderCommandsMixin }) => void,
    shouldError: boolean,
    msg = ''
  ) {
    const { mixin, prep, test, cleanup } = this._getGPURenderCommandsMixin(encoderType);
    fn({ mixin });
    prep();

    await this.expectValidationError(test, shouldError, msg);

    cleanup();
  }

  /**
   * Creates GPUBindingCommandsMixin setup with some initial state.
   */
  _getGPUBindingCommandsMixin(encoderType: EncoderType) {
    const { device } = this;

    switch (encoderType) {
      case 'compute': {
        const buffer = device.createBuffer({
          size: 16,
          usage: GPUBufferUsage.UNIFORM,
        });

        const layout = device.createBindGroupLayout({
          entries: [
            {
              binding: 0,
              visibility: GPUShaderStage.COMPUTE,
              buffer: {},
            },
          ],
        });

        const bindGroup = device.createBindGroup({
          layout,
          entries: [
            {
              binding: 0,
              resource: { buffer },
            },
          ],
        });

        const encoder = device.createCommandEncoder();
        const mixin = encoder.beginComputePass();
        return {
          mixin,
          bindGroup,
          prep() {
            mixin.end();
          },
          test() {
            encoder.finish();
          },
          cleanup() {
            buffer.destroy();
          },
        };
        break;
      }
      case 'render':
        return this._getGPURenderCommandsMixin('render');
      case 'renderBundle':
        return this._getGPURenderCommandsMixin('renderBundle');
    }
  }

  /**
   * Tests a method on GPUBindingCommandsMixin
   * The function pass will be called with the mixin and a bindGroup
   */
  async testGPUBindingCommandsMixin(
    encoderType: EncoderType,
    fn: ({ bindGroup }: { mixin: GPUBindingCommandsMixin; bindGroup: GPUBindGroup }) => void,
    shouldError: boolean,
    msg = ''
  ) {
    const { mixin, bindGroup, prep, test, cleanup } = this._getGPUBindingCommandsMixin(encoderType);
    fn({ mixin, bindGroup });
    prep();

    await this.expectValidationError(test, shouldError, msg);

    cleanup();
  }
}

/**
 * Makes a new LimitTest class so that the tests have access to `limit`
 */
function makeLimitTestFixture(limit: GPUSupportedLimit): typeof LimitTestsImpl {
  class LimitTests extends LimitTestsImpl {
    limit = limit;
  }

  return LimitTests;
}

/**
 * This is to avoid repeating yourself (D.R.Y.) as I ran into that issue multiple times
 * writing these tests where I'd copy a test, need to rename a limit in 3-4 places,
 * forget one place, and then spend 20-30 minutes wondering why the test was failing.
 */
export function makeLimitTestGroup(limit: GPUSupportedLimit) {
  const description = `API Validation Tests for ${limit}.`;
  const g = makeTestGroup(makeLimitTestFixture(limit));
  return { g, description, limit };
}
