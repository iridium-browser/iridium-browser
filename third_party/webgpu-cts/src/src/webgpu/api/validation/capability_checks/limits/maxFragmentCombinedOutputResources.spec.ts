import { range } from '../../../../../common/util/util.js';

import {
  computeBytesPerSample,
  kMaximumLimitBaseParams,
  makeLimitTestGroup,
  LimitsRequest,
} from './limit_utils.js';

function select<T>(array: T[], i: number) {
  return array[i % array.length];
}

const kTextureStorageTypes = [
  { type: 'texture_storage_1d', coords: '0' },
  { type: 'texture_storage_2d', coords: 'vec2u(0)' },
  { type: 'texture_storage_2d_array', coords: 'vec2u(0), 0' },
  { type: 'texture_storage_3d', coords: 'vec3u(0)' },
];

function getPipelineDescriptor(device: GPUDevice, testValue: number) {
  const numStorageResourcesTargetPerType = (testValue / 3) | 0;
  const numStorageBuffers = Math.min(
    numStorageResourcesTargetPerType,
    device.limits.maxStorageBuffersPerShaderStage
  );
  const numStorageTextures = Math.min(
    numStorageResourcesTargetPerType,
    device.limits.maxStorageTexturesPerShaderStage
  );

  const numTargets = testValue - numStorageBuffers - numStorageTextures;

  const bindingAndGroup = (i: number, bindingOffset: number) =>
    `@group(${i % 3}) @binding(${((i / 3) | 0) * 2 + bindingOffset})`;

  const code = `
    @vertex fn vs() -> @builtin(position) vec4f {
      return vec4f(0);
    }

    // these are unused and so should not affect the test.
    @group(3) @binding(0) var unusedTexture1d: texture_storage_1d<rgba32float, write>;
    @group(3) @binding(1) var unusedTexture2d: texture_storage_2d<rgba32float, write>;
    @group(3) @binding(2) var unusedTexture2dArray: texture_storage_2d_array<rgba32float, write>;
    @group(3) @binding(3) var unusedTexture3d: texture_storage_3d<rgba32float, write>;

    // testValue: ${testValue}
    // numStorageBuffers: ${numStorageBuffers}
    // numStorageTextures: ${numStorageTextures}
    // numTargets: ${numTargets}

    // We use rgba32float format storage textures as they take the most size and so are most likely
    // to trigger any invalid validation.

    ${range(
      numStorageBuffers,
      i => `${bindingAndGroup(i, 0)} var<storage, read_write> usedBuffer${i}: array<f32>;`
    ).join('\n    ')}
    ${range(
      numStorageTextures,
      i =>
        `${bindingAndGroup(i, 1)} var usedTexture${i}: ${
          select(kTextureStorageTypes, i).type
        }<rgba32float, write>;`
    ).join('\n    ')}

    @fragment fn fs() -> @location(0) vec4f {
      ${range(numStorageBuffers, i => `usedBuffer${i}[0] = 0.0;`).join('\n      ')}
      ${range(
        numStorageTextures,
        i => `textureStore(usedTexture${i}, ${select(kTextureStorageTypes, i).coords}, vec4f(0));`
      ).join('\n      ')}
      return vec4f(0);
    }
  `;
  const module = device.createShaderModule({ code });
  const pipelineDescriptor: GPURenderPipelineDescriptor = {
    layout: 'auto',
    vertex: {
      module,
      entryPoint: 'vs',
    },
    fragment: {
      module,
      entryPoint: 'fs',
      targets: new Array(numTargets).fill({ format: 'r8unorm', writeMask: 0 }),
    },
  };
  return { pipelineDescriptor, code };
}

const kExtraLimits: LimitsRequest = {
  maxColorAttachments: 'adapterLimit',
  maxColorAttachmentBytesPerSample: 'adapterLimit',
  maxStorageBuffersPerShaderStage: 'adapterLimit',
  maxStorageTexturesPerShaderStage: 'adapterLimit',
};

const limit = 'maxFragmentCombinedOutputResources';
export const { g, description } = makeLimitTestGroup(limit);

g.test('createRenderPipeline,at_over')
  .desc(`Test using at and over ${limit} limit in createRenderPipeline(Async)`)
  .params(kMaximumLimitBaseParams.combine('async', [false, true] as const))
  .fn(async t => {
    const { limitTest, testValueName, async } = t.params;
    await t.testDeviceWithRequestedMaximumLimits(
      limitTest,
      testValueName,
      async ({ device, testValue, shouldError }) => {
        const { pipelineDescriptor, code } = getPipelineDescriptor(device, testValue);
        const targets = pipelineDescriptor.fragment?.targets as GPUColorTargetState[];
        const bytesPerSample = computeBytesPerSample(targets);
        if (
          targets.length > device.limits.maxColorAttachments ||
          bytesPerSample > device.limits.maxColorAttachmentBytesPerSample
        ) {
          return;
        }

        await t.testCreateRenderPipeline(pipelineDescriptor, async, shouldError, code);
      },
      kExtraLimits
    );
  });
