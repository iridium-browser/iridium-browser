import {
  kTextureFormatInfo,
  SizedTextureFormat,
  DepthStencilFormat,
  depthStencilFormatCopyableAspects,
} from '../../../capability_info.js';
import { ImageCopyType } from '../../../util/texture/layout.js';
import { ValidationTest } from '../validation_test.js';

export class ImageCopyTest extends ValidationTest {
  testRun(
    textureCopyView: GPUImageCopyTexture,
    textureDataLayout: GPUImageDataLayout,
    size: GPUExtent3D,
    {
      method,
      dataSize,
      success,
      submit = false,
    }: {
      method: ImageCopyType;
      dataSize: number;
      success: boolean;
      /** If submit is true, the validaton error is expected to come from the submit and encoding
       * should succeed. */
      submit?: boolean;
    }
  ): void {
    switch (method) {
      case 'WriteTexture': {
        const data = new Uint8Array(dataSize);

        this.expectValidationError(() => {
          this.device.queue.writeTexture(textureCopyView, data, textureDataLayout, size);
        }, !success);

        break;
      }
      case 'CopyB2T': {
        const buffer = this.device.createBuffer({
          size: dataSize,
          usage: GPUBufferUsage.COPY_SRC,
        });
        this.trackForCleanup(buffer);

        const encoder = this.device.createCommandEncoder();
        encoder.copyBufferToTexture({ buffer, ...textureDataLayout }, textureCopyView, size);

        if (submit) {
          const cmd = encoder.finish();
          this.expectValidationError(() => {
            this.device.queue.submit([cmd]);
          }, !success);
        } else {
          this.expectValidationError(() => {
            encoder.finish();
          }, !success);
        }

        break;
      }
      case 'CopyT2B': {
        const buffer = this.device.createBuffer({
          size: dataSize,
          usage: GPUBufferUsage.COPY_DST,
        });
        this.trackForCleanup(buffer);

        const encoder = this.device.createCommandEncoder();
        encoder.copyTextureToBuffer(textureCopyView, { buffer, ...textureDataLayout }, size);

        if (submit) {
          const cmd = encoder.finish();
          this.expectValidationError(() => {
            this.device.queue.submit([cmd]);
          }, !success);
        } else {
          this.expectValidationError(() => {
            encoder.finish();
          }, !success);
        }

        break;
      }
    }
  }

  // This is a helper function used for creating a texture when we don't have to be very
  // precise about its size as long as it's big enough and properly aligned.
  createAlignedTexture(
    format: SizedTextureFormat,
    copySize: Required<GPUExtent3DDict> = { width: 1, height: 1, depthOrArrayLayers: 1 },
    origin: Required<GPUOrigin3DDict> = { x: 0, y: 0, z: 0 },
    dimension: Required<GPUTextureDimension> = '2d'
  ): GPUTexture {
    const info = kTextureFormatInfo[format];
    return this.device.createTexture({
      size: {
        width: Math.max(1, copySize.width + origin.x) * info.blockWidth,
        height: Math.max(1, copySize.height + origin.y) * info.blockHeight,
        depthOrArrayLayers: Math.max(1, copySize.depthOrArrayLayers + origin.z),
      },
      dimension,
      format,
      usage: GPUTextureUsage.COPY_SRC | GPUTextureUsage.COPY_DST,
    });
  }

  testBuffer(
    buffer: GPUBuffer,
    texture: GPUTexture,
    textureDataLayout: GPUImageDataLayout,
    size: GPUExtent3D,
    {
      method,
      dataSize,
      success,
      submit = true,
    }: {
      method: ImageCopyType;
      dataSize: number;
      success: boolean;
      /** If submit is true, the validaton error is expected to come from the submit and encoding
       * should succeed. */
      submit?: boolean;
    }
  ): void {
    switch (method) {
      case 'WriteTexture': {
        const data = new Uint8Array(dataSize);

        this.expectValidationError(() => {
          this.device.queue.writeTexture({ texture }, data, textureDataLayout, size);
        }, !success);

        break;
      }
      case 'CopyB2T': {
        const { encoder, validateFinishAndSubmit } = this.createEncoder('non-pass');
        encoder.copyBufferToTexture({ buffer, ...textureDataLayout }, { texture }, size);
        validateFinishAndSubmit(success, submit);

        break;
      }
      case 'CopyT2B': {
        const { encoder, validateFinishAndSubmit } = this.createEncoder('non-pass');
        encoder.copyTextureToBuffer({ texture }, { buffer, ...textureDataLayout }, size);
        validateFinishAndSubmit(success, submit);

        break;
      }
    }
  }
}

// For testing divisibility by a number we test all the values returned by this function:
function valuesToTestDivisibilityBy(number: number): Iterable<number> {
  const values = [];
  for (let i = 0; i <= 2 * number; ++i) {
    values.push(i);
  }
  values.push(3 * number);
  return values;
}

interface WithFormat {
  format: SizedTextureFormat;
}

interface WithFormatAndCoordinate extends WithFormat {
  coordinateToTest: keyof GPUOrigin3DDict | keyof GPUExtent3DDict;
}

interface WithFormatAndMethod extends WithFormat {
  method: ImageCopyType;
}

// This is a helper function used for expanding test parameters for offset alignment, by spec
export function texelBlockAlignmentTestExpanderForOffset({ format }: WithFormat) {
  const info = kTextureFormatInfo[format];
  if (info.depth || info.stencil) {
    return valuesToTestDivisibilityBy(4);
  }

  return valuesToTestDivisibilityBy(kTextureFormatInfo[format].bytesPerBlock);
}

// This is a helper function used for expanding test parameters for texel block alignment tests on rowsPerImage
export function texelBlockAlignmentTestExpanderForRowsPerImage({ format }: WithFormat) {
  return valuesToTestDivisibilityBy(kTextureFormatInfo[format].blockHeight);
}

// This is a helper function used for expanding test parameters for texel block alignment tests on origin and size
export function texelBlockAlignmentTestExpanderForValueToCoordinate({
  format,
  coordinateToTest,
}: WithFormatAndCoordinate) {
  switch (coordinateToTest) {
    case 'x':
    case 'width':
      return valuesToTestDivisibilityBy(kTextureFormatInfo[format].blockWidth!);

    case 'y':
    case 'height':
      return valuesToTestDivisibilityBy(kTextureFormatInfo[format].blockHeight!);

    case 'z':
    case 'depthOrArrayLayers':
      return valuesToTestDivisibilityBy(1);
  }
}

// This is a helper function used for filtering test parameters
export function formatCopyableWithMethod({ format, method }: WithFormatAndMethod): boolean {
  const info = kTextureFormatInfo[format];
  if (info.depth || info.stencil) {
    const supportedAspects: readonly GPUTextureAspect[] = depthStencilFormatCopyableAspects(
      method,
      format as DepthStencilFormat
    );
    return supportedAspects.length > 0;
  }
  if (method === 'CopyT2B') {
    return info.copySrc;
  } else {
    return info.copyDst;
  }
}

// This is a helper function used for filtering test parameters
export function getACopyableAspectWithMethod({
  format,
  method,
}: WithFormatAndMethod): GPUTextureAspect {
  const info = kTextureFormatInfo[format];
  if (info.depth || info.stencil) {
    const supportedAspects: readonly GPUTextureAspect[] = depthStencilFormatCopyableAspects(
      method,
      format as DepthStencilFormat
    );
    return supportedAspects[0];
  }
  return 'all' as GPUTextureAspect;
}
