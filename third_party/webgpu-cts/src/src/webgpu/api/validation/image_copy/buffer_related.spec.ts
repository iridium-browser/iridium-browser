export const description = `Validation tests for buffer related parameters for copies`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import {
  kSizedTextureFormats,
  kTextureFormatInfo,
  textureDimensionAndFormatCompatible,
} from '../../../capability_info.js';
import { GPUConst } from '../../../constants.js';
import { kResourceStates } from '../../../gpu_test.js';
import { kImageCopyTypes } from '../../../util/texture/layout.js';

import { ImageCopyTest, formatCopyableWithMethod } from './image_copy.js';

export const g = makeTestGroup(ImageCopyTest);

g.test('valid')
  .desc(`The buffer must be valid and not destroyed.`)
  .params(u =>
    u //
      // B2B copy validations are at api,validation,encoding,cmds,copyBufferToBuffer.spec.ts
      .combine('method', ['CopyB2T', 'CopyT2B'] as const)
      .combine('state', kResourceStates)
  )
  .fn(async t => {
    const { method, state } = t.params;

    // A valid buffer.
    const buffer = t.createBufferWithState(state, {
      size: 16,
      usage: GPUBufferUsage.COPY_SRC | GPUBufferUsage.COPY_DST,
    });

    const success = state !== 'invalid';
    const submit = state !== 'destroyed';

    const texture = t.device.createTexture({
      size: { width: 2, height: 2, depthOrArrayLayers: 1 },
      format: 'rgba8unorm',
      usage: GPUTextureUsage.COPY_SRC | GPUTextureUsage.COPY_DST,
    });

    t.testBuffer(
      buffer,
      texture,
      { bytesPerRow: 0 },
      { width: 0, height: 0, depthOrArrayLayers: 0 },
      { dataSize: 16, method, success, submit }
    );
  });

g.test('usage')
  .desc(`The buffer must have the appropriate COPY_SRC/COPY_DST usage.`)
  .params(u =>
    u
      // B2B copy validations are at api,validation,encoding,cmds,copyBufferToBuffer.spec.ts
      .combine('method', ['CopyB2T', 'CopyT2B'] as const)
      .beginSubcases()
      .combine('usage', [
        GPUConst.BufferUsage.COPY_SRC | GPUConst.BufferUsage.UNIFORM,
        GPUConst.BufferUsage.COPY_DST | GPUConst.BufferUsage.UNIFORM,
        GPUConst.BufferUsage.COPY_SRC | GPUConst.BufferUsage.COPY_DST,
      ])
  )
  .fn(async t => {
    const { method, usage } = t.params;

    const buffer = t.device.createBuffer({
      size: 16,
      usage,
    });

    const success =
      method === 'CopyB2T'
        ? (usage & GPUBufferUsage.COPY_SRC) !== 0
        : (usage & GPUBufferUsage.COPY_DST) !== 0;

    const texture = t.device.createTexture({
      size: { width: 2, height: 2, depthOrArrayLayers: 1 },
      format: 'rgba8unorm',
      usage: GPUTextureUsage.COPY_SRC | GPUTextureUsage.COPY_DST,
    });

    t.testBuffer(
      buffer,
      texture,
      { bytesPerRow: 0 },
      { width: 0, height: 0, depthOrArrayLayers: 0 },
      { dataSize: 16, method, success }
    );
  });

g.test('bytes_per_row_alignment')
  .desc(
    `Test that bytesPerRow must be a multiple of 256 for CopyB2T and CopyT2B if it is required.`
  )
  .params(u =>
    u //
      .combine('method', kImageCopyTypes)
      .combine('dimension', ['2d', '3d'] as const)
      .combine('format', kSizedTextureFormats)
      .filter(({ dimension, format }) => textureDimensionAndFormatCompatible(dimension, format))
      .filter(formatCopyableWithMethod)
      .beginSubcases()
      .combine('bytesPerRow', [undefined, 0, 1, 255, 256, 257, 512])
      .combine('copyHeightInBlocks', [0, 1, 2, 3])
      .expand('_textureHeightInBlocks', p => [
        p.copyHeightInBlocks === 0 ? 1 : p.copyHeightInBlocks,
      ])
      // Depth/stencil format copies must copy the whole subresource.
      .unless(p => {
        const info = kTextureFormatInfo[p.format];
        return (info.depth || info.stencil) && p.copyHeightInBlocks !== p._textureHeightInBlocks;
      })
      // bytesPerRow must be specified and it must be equal or greater than the bytes size of each row if we are copying multiple rows.
      // Note that we are copying one single block on each row in this test.
      .filter(
        ({ format, bytesPerRow, copyHeightInBlocks }) =>
          (bytesPerRow === undefined && copyHeightInBlocks <= 1) ||
          (bytesPerRow !== undefined && bytesPerRow >= kTextureFormatInfo[format].bytesPerBlock)
      )
  )
  .fn(async t => {
    const {
      method,
      dimension,
      format,
      bytesPerRow,
      copyHeightInBlocks,
      _textureHeightInBlocks,
    } = t.params;

    const info = kTextureFormatInfo[format];
    await t.selectDeviceOrSkipTestCase(info.feature);

    const buffer = t.device.createBuffer({
      size: 512 * 8 * 16,
      usage: GPUBufferUsage.COPY_SRC | GPUBufferUsage.COPY_DST,
    });

    let success = false;
    // writeTexture doesn't require bytesPerRow to be 256-byte aligned.
    if (method === 'WriteTexture') success = true;
    // If the copy height <= 1, bytesPerRow is not required.
    if (copyHeightInBlocks <= 1 && bytesPerRow === undefined) success = true;
    // If bytesPerRow > 0 and it is a multiple of 256, it will succeeed if other parameters are valid.
    if (bytesPerRow !== undefined && bytesPerRow > 0 && bytesPerRow % 256 === 0) success = true;

    const size = [info.blockWidth, _textureHeightInBlocks * info.blockHeight, 1];
    const texture = t.device.createTexture({
      size,
      dimension,
      format,
      usage: GPUTextureUsage.COPY_SRC | GPUTextureUsage.COPY_DST,
    });

    const copySize = [info.blockWidth, copyHeightInBlocks * info.blockHeight, 1];

    t.testBuffer(buffer, texture, { bytesPerRow }, copySize, {
      dataSize: 512 * 8 * 16,
      method,
      success,
    });
  });
