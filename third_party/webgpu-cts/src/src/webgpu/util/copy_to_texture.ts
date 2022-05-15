import { assert, memcpy } from '../../common/util/util.js';
import { RegularTextureFormat } from '../capability_info.js';
import { GPUTest } from '../gpu_test.js';

import { makeInPlaceColorConversion } from './color_space_conversion.js';
import { TexelView } from './texture/texel_view.js';
import { TexelCompareOptions, textureContentIsOKByT2B } from './texture/texture_ok.js';

export class CopyToTextureUtils extends GPUTest {
  doFlipY(
    sourcePixels: Uint8ClampedArray,
    width: number,
    height: number,
    bytesPerPixel: number
  ): Uint8ClampedArray {
    const dstPixels = new Uint8ClampedArray(width * height * bytesPerPixel);
    for (let i = 0; i < height; ++i) {
      for (let j = 0; j < width; ++j) {
        const srcPixelPos = i * width + j;
        // WebGL readPixel returns pixels from bottom-left origin. Using CopyExternalImageToTexture
        // to copy from WebGL Canvas keeps top-left origin. So the expectation from webgl.readPixel should
        // be flipped.
        const dstPixelPos = (height - i - 1) * width + j;

        memcpy(
          { src: sourcePixels, start: srcPixelPos * bytesPerPixel, length: bytesPerPixel },
          { dst: dstPixels, start: dstPixelPos * bytesPerPixel }
        );
      }
    }

    return dstPixels;
  }

  getExpectedPixels(
    sourcePixels: Uint8ClampedArray,
    width: number,
    height: number,
    format: RegularTextureFormat,
    isFlipY: boolean,
    conversion: {
      srcPremultiplied: boolean;
      dstPremultiplied: boolean;
      srcColorSpace?: PredefinedColorSpace;
      dstColorSpace?: GPUPredefinedColorSpace;
    }
  ): TexelView {
    const applyConversion = makeInPlaceColorConversion(conversion);

    const divide = 255.0;
    return TexelView.fromTexelsAsColors(
      format,
      coords => {
        assert(coords.x < width && coords.y < height && coords.z === 0, 'out of bounds');
        const y = isFlipY ? height - coords.y - 1 : coords.y;
        const pixelPos = y * width + coords.x;

        const rgba = {
          R: sourcePixels[pixelPos * 4] / divide,
          G: sourcePixels[pixelPos * 4 + 1] / divide,
          B: sourcePixels[pixelPos * 4 + 2] / divide,
          A: sourcePixels[pixelPos * 4 + 3] / divide,
        };
        applyConversion(rgba);
        return rgba;
      },
      { clampToFormatRange: true }
    );
  }

  doTestAndCheckResult(
    imageCopyExternalImage: GPUImageCopyExternalImage,
    dstTextureCopyView: GPUImageCopyTextureTagged,
    expTexelView: TexelView,
    copySize: Required<GPUExtent3DDict>,
    texelCompareOptions: TexelCompareOptions
  ): void {
    this.device.queue.copyExternalImageToTexture(
      imageCopyExternalImage,
      dstTextureCopyView,
      copySize
    );

    const resultPromise = textureContentIsOKByT2B(
      this,
      { texture: dstTextureCopyView.texture },
      copySize,
      { expTexelView },
      texelCompareOptions
    );
    this.eventualExpectOK(resultPromise);
  }
}
