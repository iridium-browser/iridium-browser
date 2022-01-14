export const description = `
  texture related validation tests for B2T copy and T2B copy and writeTexture.

  Note: see api,validation,encoding,cmds,copyTextureToTexture:* for validation tests of T2T copy.

  TODO: expand the tests below to 1d texture.
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { assert } from '../../../../common/util/util.js';
import {
  kColorTextureFormats,
  kSizedTextureFormats,
  kTextureFormatInfo,
  textureDimensionAndFormatCompatible,
} from '../../../capability_info.js';
import { GPUConst } from '../../../constants.js';
import { align } from '../../../util/math.js';
import { virtualMipSize } from '../../../util/texture/base.js';
import { kImageCopyTypes } from '../../../util/texture/layout.js';

import {
  ImageCopyTest,
  texelBlockAlignmentTestExpanderForValueToCoordinate,
  formatCopyableWithMethod,
  getACopyableAspectWithMethod,
} from './image_copy.js';

export const g = makeTestGroup(ImageCopyTest);

g.test('valid')
  .desc(`The texture must be valid and not destroyed.`)
  .params(u =>
    u //
      .combine('method', kImageCopyTypes)
      .combine('textureState', ['valid', 'destroyed', 'error'])
      .combineWithParams([
        { depthOrArrayLayers: 1, dimension: '2d' },
        { depthOrArrayLayers: 3, dimension: '2d' },
        { depthOrArrayLayers: 3, dimension: '3d' },
      ] as const)
  )
  .fn(async t => {
    const { method, textureState, depthOrArrayLayers, dimension } = t.params;

    // A valid texture.
    let texture = t.device.createTexture({
      size: { width: 4, height: 4, depthOrArrayLayers },
      dimension,
      format: 'rgba8unorm',
      usage: GPUTextureUsage.COPY_SRC | GPUTextureUsage.COPY_DST,
    });

    switch (textureState) {
      case 'destroyed': {
        texture.destroy();
        break;
      }
      case 'error': {
        texture = t.getErrorTexture();
        break;
      }
    }

    const success = textureState === 'valid';
    const submit = textureState === 'destroyed';

    t.testRun(
      { texture },
      { bytesPerRow: 0 },
      { width: 0, height: 0, depthOrArrayLayers: 0 },
      { dataSize: 1, method, success, submit }
    );
  });

g.test('texture,device_mismatch')
  .desc('Tests the image copies cannot be called with a texture created from another device')
  .paramsSubcasesOnly(u =>
    u.combine('method', kImageCopyTypes).combine('mismatched', [true, false])
  )
  .unimplemented();

g.test('buffer,device_mismatch')
  .desc('Tests the image copies cannot be called with a buffer created from another device')
  .paramsSubcasesOnly(u =>
    u.combine('method', ['CopyB2T', 'CopyT2B'] as const).combine('mismatched', [true, false])
  )
  .unimplemented();

g.test('usage')
  .desc(`The texture must have the appropriate COPY_SRC/COPY_DST usage.`)
  .params(u =>
    u
      .combine('method', kImageCopyTypes)
      .combineWithParams([
        { depthOrArrayLayers: 1, dimension: '2d' },
        { depthOrArrayLayers: 3, dimension: '2d' },
        { depthOrArrayLayers: 3, dimension: '3d' },
      ] as const)
      .beginSubcases()
      .combine('usage', [
        GPUConst.TextureUsage.COPY_SRC | GPUConst.TextureUsage.TEXTURE_BINDING,
        GPUConst.TextureUsage.COPY_DST | GPUConst.TextureUsage.TEXTURE_BINDING,
        GPUConst.TextureUsage.COPY_SRC | GPUConst.TextureUsage.COPY_DST,
      ])
  )
  .fn(async t => {
    const { usage, method, depthOrArrayLayers, dimension } = t.params;

    const texture = t.device.createTexture({
      size: { width: 4, height: 4, depthOrArrayLayers },
      dimension,
      format: 'rgba8unorm',
      usage,
    });

    const success =
      method === 'CopyT2B'
        ? (usage & GPUTextureUsage.COPY_SRC) !== 0
        : (usage & GPUTextureUsage.COPY_DST) !== 0;

    t.testRun(
      { texture },
      { bytesPerRow: 0 },
      { width: 0, height: 0, depthOrArrayLayers: 0 },
      { dataSize: 1, method, success }
    );
  });

g.test('sample_count')
  .desc(
    `Multisampled textures cannot be copied. Note that we don't test 2D array and 3D textures because multisample is not supported for 2D array and 3D texture creation`
  )
  .params(u =>
    u //
      .combine('method', kImageCopyTypes)
      .beginSubcases()
      .combine('sampleCount', [1, 4])
  )
  .fn(async t => {
    const { sampleCount, method } = t.params;

    const texture = t.device.createTexture({
      size: { width: 4, height: 4, depthOrArrayLayers: 1 },
      sampleCount,
      format: 'rgba8unorm',
      usage: GPUTextureUsage.COPY_SRC | GPUTextureUsage.COPY_DST | GPUTextureUsage.TEXTURE_BINDING,
    });

    const success = sampleCount === 1;

    t.testRun(
      { texture },
      { bytesPerRow: 0 },
      { width: 0, height: 0, depthOrArrayLayers: 0 },
      { dataSize: 1, method, success }
    );
  });

g.test('mip_level')
  .desc(`The mipLevel of the copy must be in range of the texture.`)
  .params(u =>
    u
      .combine('method', kImageCopyTypes)
      .combineWithParams([
        { depthOrArrayLayers: 1, dimension: '2d' },
        { depthOrArrayLayers: 3, dimension: '2d' },
        { depthOrArrayLayers: 3, dimension: '3d' },
      ] as const)
      .beginSubcases()
      .combine('mipLevelCount', [3, 5])
      .combine('mipLevel', [3, 4])
  )
  .fn(async t => {
    const { mipLevelCount, mipLevel, method, depthOrArrayLayers, dimension } = t.params;

    const texture = t.device.createTexture({
      size: { width: 32, height: 32, depthOrArrayLayers },
      dimension,
      mipLevelCount,
      format: 'rgba8unorm',
      usage: GPUTextureUsage.COPY_SRC | GPUTextureUsage.COPY_DST,
    });

    const success = mipLevel < mipLevelCount;

    t.testRun(
      { texture, mipLevel },
      { bytesPerRow: 0 },
      { width: 0, height: 0, depthOrArrayLayers: 0 },
      { dataSize: 1, method, success }
    );
  });

g.test('format')
  .desc(`Test that it must be a full copy if the texture's format is depth/stencil format`)
  .params(u =>
    u //
      .combine('method', kImageCopyTypes)
      .combineWithParams([
        { depthOrArrayLayers: 1, dimension: '2d' },
        { depthOrArrayLayers: 3, dimension: '2d' },
        { depthOrArrayLayers: 32, dimension: '3d' },
      ] as const)
      .combine('format', kSizedTextureFormats)
      .filter(({ dimension, format }) => textureDimensionAndFormatCompatible(dimension, format))
      .filter(formatCopyableWithMethod)
      .beginSubcases()
      .combine('mipLevel', [0, 2])
      .combine('copyWidthModifier', [0, -1])
      .combine('copyHeightModifier', [0, -1])
      // If the texture has multiple depth/array slices and it is not a 3D texture, which means it is an array texture,
      // depthModifier is not needed upon the third dimension. Because different layers are different subresources in
      // an array texture. Whether it is a full copy or non-full copy doesn't make sense across different subresources.
      // However, different depth slices on the same mip level are within the same subresource for a 3d texture. So we
      // need to examine depth dimension via copyDepthModifier to determine whether it is a full copy for a 3D texture.
      .expand('copyDepthModifier', ({ dimension: d }) => (d === '3d' ? [0, -1] : [0]))
  )
  .fn(async t => {
    const {
      method,
      depthOrArrayLayers,
      dimension,
      format,
      mipLevel,
      copyWidthModifier,
      copyHeightModifier,
      copyDepthModifier,
    } = t.params;

    const info = kTextureFormatInfo[format];
    await t.selectDeviceOrSkipTestCase(info.feature);

    const size = { width: 32, height: 32, depthOrArrayLayers };
    const texture = t.device.createTexture({
      size,
      dimension,
      format,
      mipLevelCount: 5,
      usage: GPUTextureUsage.COPY_SRC | GPUTextureUsage.COPY_DST,
    });

    let success = true;
    if (
      (info.depth || info.stencil) &&
      (copyWidthModifier !== 0 || copyHeightModifier !== 0 || copyDepthModifier !== 0)
    ) {
      success = false;
    }

    const levelSize = virtualMipSize(
      dimension,
      [size.width, size.height, size.depthOrArrayLayers],
      mipLevel
    );
    const copySize = [
      levelSize[0] + copyWidthModifier * info.blockWidth,
      levelSize[1] + copyHeightModifier * info.blockHeight,
      // Note that compressed format is not supported for 3D textures yet, so there is no info.blockDepth.
      levelSize[2] + copyDepthModifier,
    ];

    t.testRun(
      { texture, mipLevel, aspect: getACopyableAspectWithMethod({ format, method }) },
      { bytesPerRow: 512, rowsPerImage: 32 },
      copySize,
      {
        dataSize: 512 * 32 * 32,
        method,
        success,
      }
    );
  });

g.test('origin_alignment')
  .desc(`Copy origin must be aligned to block size.`)
  .params(u =>
    u
      .combine('method', kImageCopyTypes)
      // No need to test depth/stencil formats because its copy origin must be [0, 0, 0], which is already aligned with block size.
      .combine('format', kColorTextureFormats)
      .filter(formatCopyableWithMethod)
      .combineWithParams([
        { depthOrArrayLayers: 1, dimension: '2d' },
        { depthOrArrayLayers: 3, dimension: '2d' },
        { depthOrArrayLayers: 3, dimension: '3d' },
      ] as const)
      .filter(({ dimension, format }) => textureDimensionAndFormatCompatible(dimension, format))
      .beginSubcases()
      .combine('coordinateToTest', ['x', 'y', 'z'] as const)
      .expand('valueToCoordinate', texelBlockAlignmentTestExpanderForValueToCoordinate)
  )
  .fn(async t => {
    const {
      valueToCoordinate,
      coordinateToTest,
      format,
      method,
      depthOrArrayLayers,
      dimension,
    } = t.params;
    const info = kTextureFormatInfo[format];
    await t.selectDeviceOrSkipTestCase(info.feature);

    const origin = { x: 0, y: 0, z: 0 };
    const size = { width: 0, height: 0, depthOrArrayLayers };
    let success = true;

    origin[coordinateToTest] = valueToCoordinate;
    switch (coordinateToTest) {
      case 'x': {
        success = origin.x % info.blockWidth === 0;
        break;
      }
      case 'y': {
        success = origin.y % info.blockHeight === 0;
        break;
      }
    }

    const texture = t.createAlignedTexture(format, size, origin, dimension);

    t.testRun({ texture, origin }, { bytesPerRow: 0, rowsPerImage: 0 }, size, {
      dataSize: 1,
      method,
      success,
    });
  });

g.test('1d')
  .desc(`1d texture copies must have height=depth=1.`)
  .params(u =>
    u
      .combine('method', kImageCopyTypes)
      .beginSubcases()
      .combine('width', [0, 1])
      .combineWithParams([
        { height: 1, depthOrArrayLayers: 1 },
        { height: 1, depthOrArrayLayers: 0 },
        { height: 1, depthOrArrayLayers: 2 },
        { height: 0, depthOrArrayLayers: 1 },
        { height: 2, depthOrArrayLayers: 1 },
      ])
  )
  .fn(async t => {
    const { method, width, height, depthOrArrayLayers } = t.params;
    const size = { width, height, depthOrArrayLayers };

    const texture = t.device.createTexture({
      size: { width: 2, height: 1, depthOrArrayLayers: 1 },
      dimension: '1d',
      format: 'rgba8unorm',
      usage: GPUTextureUsage.COPY_SRC | GPUTextureUsage.COPY_DST,
    });

    // For 1d textures we require copyHeight and copyDepth to be 1,
    // copyHeight or copyDepth being 0 should cause a validation error.
    const success = size.height === 1 && size.depthOrArrayLayers === 1;

    t.testRun({ texture }, { bytesPerRow: 256, rowsPerImage: 4 }, size, {
      dataSize: 16,
      method,
      success,
    });
  });

g.test('size_alignment')
  .desc(`Copy size must be aligned to block size.`)
  .params(u =>
    u
      .combine('method', kImageCopyTypes)
      // No need to test depth/stencil formats because its copy size must be subresource's size, which is already aligned with block size.
      .combine('format', kColorTextureFormats)
      .filter(formatCopyableWithMethod)
      .combine('dimension', ['2d', '3d'] as const)
      .filter(({ dimension, format }) => textureDimensionAndFormatCompatible(dimension, format))
      .beginSubcases()
      .combine('coordinateToTest', ['width', 'height', 'depthOrArrayLayers'] as const)
      .expand('valueToCoordinate', texelBlockAlignmentTestExpanderForValueToCoordinate)
  )
  .fn(async t => {
    const { valueToCoordinate, coordinateToTest, format, method } = t.params;
    const info = kTextureFormatInfo[format];
    await t.selectDeviceOrSkipTestCase(info.feature);

    const origin = { x: 0, y: 0, z: 0 };
    const size = { width: 0, height: 0, depthOrArrayLayers: 0 };
    let success = true;

    size[coordinateToTest] = valueToCoordinate;
    switch (coordinateToTest) {
      case 'width': {
        success = size.width % info.blockWidth === 0;
        break;
      }
      case 'height': {
        success = size.height % info.blockHeight === 0;
        break;
      }
    }

    const texture = t.createAlignedTexture(format, size, origin);

    const bytesPerRow = align(
      (align(size.width, info.blockWidth) / info.blockWidth) * info.bytesPerBlock,
      256
    );
    const rowsPerImage = align(size.height, info.blockHeight) / info.blockHeight;
    t.testRun({ texture, origin }, { bytesPerRow, rowsPerImage }, size, {
      dataSize: 1,
      method,
      success,
    });
  });

g.test('copy_rectangle')
  .desc(`The max corner of the copy rectangle (origin+copySize) must be inside the texture.`)
  .params(u =>
    u
      .combine('method', kImageCopyTypes)
      .combine('dimension', ['2d', '3d'] as const)
      .beginSubcases()
      .combine('originValue', [7, 8])
      .combine('copySizeValue', [7, 8])
      .combine('textureSizeValue', [14, 15])
      .combine('mipLevel', [0, 2])
      .combine('coordinateToTest', [0, 1, 2] as const)
  )
  .fn(async t => {
    const {
      originValue,
      copySizeValue,
      textureSizeValue,
      mipLevel,
      coordinateToTest,
      method,
      dimension,
    } = t.params;
    const format = 'rgba8unorm';
    const info = kTextureFormatInfo[format];

    const origin = [0, 0, 0];
    const copySize = [0, 0, 0];
    const textureSize = { width: 16 << mipLevel, height: 16 << mipLevel, depthOrArrayLayers: 16 };
    const success = originValue + copySizeValue <= textureSizeValue;

    origin[coordinateToTest] = originValue;
    copySize[coordinateToTest] = copySizeValue;
    switch (coordinateToTest) {
      case 0: {
        textureSize.width = textureSizeValue << mipLevel;
        break;
      }
      case 1: {
        textureSize.height = textureSizeValue << mipLevel;
        break;
      }
      case 2: {
        textureSize.depthOrArrayLayers =
          dimension === '3d' ? textureSizeValue << mipLevel : textureSizeValue;
        break;
      }
    }

    const texture = t.device.createTexture({
      size: textureSize,
      dimension,
      mipLevelCount: 3,
      format,
      usage: GPUTextureUsage.COPY_SRC | GPUTextureUsage.COPY_DST,
    });

    assert(copySize[0] % info.blockWidth === 0);
    const bytesPerRow = align(copySize[0] / info.blockWidth, 256);
    assert(copySize[1] % info.blockHeight === 0);
    const rowsPerImage = copySize[1] / info.blockHeight;
    t.testRun({ texture, origin, mipLevel }, { bytesPerRow, rowsPerImage }, copySize, {
      dataSize: 1,
      method,
      success,
    });
  });
