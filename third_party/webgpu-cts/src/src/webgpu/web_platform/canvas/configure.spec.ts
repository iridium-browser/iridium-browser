export const description = `
Tests for GPUCanvasContext.configure.

TODO:
- Test colorSpace
- Test viewFormats
`;

import { makeTestGroup } from '../../../common/framework/test_group.js';
import { assert } from '../../../common/util/util.js';
import {
  kAllTextureFormats,
  kCanvasTextureFormats,
  kTextureUsages,
} from '../../capability_info.js';
import { GPUConst } from '../../constants.js';
import { GPUTest } from '../../gpu_test.js';
import { kAllCanvasTypes, createCanvas } from '../../util/create_elements.js';

export const g = makeTestGroup(GPUTest);

g.test('defaults')
  .desc(
    `
    Ensure that the defaults for GPUCanvasConfiguration are correct.
    `
  )
  .params(u =>
    u //
      .combine('canvasType', kAllCanvasTypes)
  )
  .fn(async t => {
    const { canvasType } = t.params;
    const canvas = createCanvas(t, canvasType, 2, 2);
    const ctx = canvas.getContext('webgpu');
    assert(ctx instanceof GPUCanvasContext, 'Failed to get WebGPU context from canvas');

    ctx.configure({
      device: t.device,
      format: 'rgba8unorm',
    });

    const currentTexture = ctx.getCurrentTexture();
    t.expect(currentTexture.format === 'rgba8unorm');
    t.expect(currentTexture.usage === GPUTextureUsage.RENDER_ATTACHMENT);
    t.expect(currentTexture.dimension === '2d');
    t.expect(currentTexture.width === canvas.width);
    t.expect(currentTexture.height === canvas.height);
    t.expect(currentTexture.depthOrArrayLayers === 1);
    t.expect(currentTexture.mipLevelCount === 1);
    t.expect(currentTexture.sampleCount === 1);
  });

g.test('device')
  .desc(
    `
    Ensure that configure reacts appropriately to various device states.
    `
  )
  .params(u =>
    u //
      .combine('canvasType', kAllCanvasTypes)
  )
  .fn(async t => {
    const { canvasType } = t.params;
    const canvas = createCanvas(t, canvasType, 2, 2);
    const ctx = canvas.getContext('webgpu');
    assert(ctx instanceof GPUCanvasContext, 'Failed to get WebGPU context from canvas');

    // Calling configure without a device should throw.
    t.shouldThrow(true, () => {
      ctx.configure({
        format: 'rgba8unorm',
      } as GPUCanvasConfiguration);
    });

    // Device is not configured, so getCurrentTexture will throw.
    t.shouldThrow(true, () => {
      ctx.getCurrentTexture();
    });

    // Calling configure with a device should succeed.
    ctx.configure({
      device: t.device,
      format: 'rgba8unorm',
    });

    // getCurrentTexture will succeed with a valid device.
    ctx.getCurrentTexture();

    // Unconfiguring should cause the device to be cleared.
    ctx.unconfigure();
    t.shouldThrow(true, () => {
      ctx.getCurrentTexture();
    });

    // Should be able to successfully configure again after unconfiguring.
    ctx.configure({
      device: t.device,
      format: 'rgba8unorm',
    });
    ctx.getCurrentTexture();
  });

g.test('format')
  .desc(
    `
    Ensure that only valid texture formats are allowed when calling configure.
    `
  )
  .params(u =>
    u //
      .combine('canvasType', kAllCanvasTypes)
      .combine('format', kAllTextureFormats)
  )
  .beforeAllSubcases(t => {
    t.selectDeviceForTextureFormatOrSkipTestCase(t.params.format);
  })
  .fn(async t => {
    const { canvasType, format } = t.params;
    const canvas = createCanvas(t, canvasType, 2, 2);
    const ctx = canvas.getContext('webgpu');
    assert(ctx instanceof GPUCanvasContext, 'Failed to get WebGPU context from canvas');

    // Would prefer to use kCanvasTextureFormats.includes(format), but that's giving TS errors.
    let validFormat = false;
    for (const canvasFormat of kCanvasTextureFormats) {
      if (format === canvasFormat) {
        validFormat = true;
        break;
      }
    }

    t.expectValidationError(() => {
      ctx.configure({
        device: t.device,
        format,
      });
    }, !validFormat);

    t.expectValidationError(() => {
      // Should always return a texture, whether the configured format was valid or not.
      const currentTexture = ctx.getCurrentTexture();
      t.expect(currentTexture instanceof GPUTexture);
    }, !validFormat);
  });

g.test('usage')
  .desc(
    `
    Ensure that getCurrentTexture returns a texture with the configured usages.
    `
  )
  .params(u =>
    u //
      .combine('canvasType', kAllCanvasTypes)
      .beginSubcases()
      .expand('usage', p => {
        const usageSet = new Set<number>();
        for (const usage0 of kTextureUsages) {
          for (const usage1 of kTextureUsages) {
            usageSet.add(usage0 | usage1);
          }
        }
        return usageSet;
      })
  )
  .fn(async t => {
    const { canvasType, usage } = t.params;
    const canvas = createCanvas(t, canvasType, 2, 2);
    const ctx = canvas.getContext('webgpu');
    assert(ctx instanceof GPUCanvasContext, 'Failed to get WebGPU context from canvas');

    ctx.configure({
      device: t.device,
      format: 'rgba8unorm',
      usage,
    });

    const currentTexture = ctx.getCurrentTexture();
    t.expect(currentTexture instanceof GPUTexture);
    t.expect(currentTexture.usage === usage);

    // Try to use the texture with the given usage

    if (usage & GPUConst.TextureUsage.RENDER_ATTACHMENT) {
      const encoder = t.device.createCommandEncoder();
      const pass = encoder.beginRenderPass({
        colorAttachments: [
          {
            view: currentTexture.createView(),
            clearValue: [1.0, 0.0, 0.0, 1.0],
            loadOp: 'clear',
            storeOp: 'store',
          },
        ],
      });
      pass.end();
      t.device.queue.submit([encoder.finish()]);
    }

    if (usage & GPUConst.TextureUsage.TEXTURE_BINDING) {
      const bgl = t.device.createBindGroupLayout({
        entries: [
          {
            binding: 0,
            visibility: GPUShaderStage.FRAGMENT,
            texture: {},
          },
        ],
      });

      t.device.createBindGroup({
        layout: bgl,
        entries: [
          {
            binding: 0,
            resource: currentTexture.createView(),
          },
        ],
      });
    }

    if (usage & GPUConst.TextureUsage.STORAGE_BINDING) {
      const bgl = t.device.createBindGroupLayout({
        entries: [
          {
            binding: 0,
            visibility: GPUShaderStage.FRAGMENT,
            storageTexture: { access: 'write-only', format: currentTexture.format },
          },
        ],
      });

      t.device.createBindGroup({
        layout: bgl,
        entries: [
          {
            binding: 0,
            resource: currentTexture.createView(),
          },
        ],
      });
    }

    if (usage & GPUConst.TextureUsage.COPY_DST) {
      const rgbaData = new Uint8Array([255, 0, 0, 255]);

      t.device.queue.writeTexture({ texture: currentTexture }, rgbaData, {}, [1, 1, 1]);
    }

    if (usage & GPUConst.TextureUsage.COPY_SRC) {
      const size = [currentTexture.width, currentTexture.height, 1];
      const dstTexture = t.device.createTexture({
        format: currentTexture.format,
        usage: GPUTextureUsage.COPY_DST,
        size,
      });

      const encoder = t.device.createCommandEncoder();
      encoder.copyTextureToTexture({ texture: currentTexture }, { texture: dstTexture }, size);
      t.device.queue.submit([encoder.finish()]);
    }
  });

g.test('alpha_mode')
  .desc(
    `
    Ensure that all valid alphaMode values are allowed when calling configure.
    `
  )
  .params(u =>
    u //
      .combine('canvasType', kAllCanvasTypes)
      .beginSubcases()
      .combine('alphaMode', ['opaque', 'premultiplied'] as const)
  )
  .fn(async t => {
    const { canvasType, alphaMode } = t.params;
    const canvas = createCanvas(t, canvasType, 2, 2);
    const ctx = canvas.getContext('webgpu');
    assert(ctx instanceof GPUCanvasContext, 'Failed to get WebGPU context from canvas');

    ctx.configure({
      device: t.device,
      format: 'rgba8unorm',
      alphaMode,
    });

    const currentTexture = ctx.getCurrentTexture();
    t.expect(currentTexture instanceof GPUTexture);
  });
