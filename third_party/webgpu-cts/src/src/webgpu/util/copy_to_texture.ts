import { GPUTest } from '../gpu_test.js';

import { checkElementsEqual, checkElementsBetween } from './check_contents.js';
import { align } from './math.js';
import { kBytesPerRowAlignment } from './texture/layout.js';

export function isFp16Format(format: GPUTextureFormat): boolean {
  switch (format) {
    case 'r16float':
    case 'rg16float':
    case 'rgba16float':
      return true;
    default:
      return false;
  }
}

export class CopyToTextureUtils extends GPUTest {
  // TODO(crbug.com/dawn/868): Should be possible to consolidate this along with texture checking
  checkCopyExternalImageResult(
    src: GPUBuffer,
    expected: ArrayBufferView,
    width: number,
    height: number,
    bytesPerPixel: number,
    isFp16: boolean
  ): void {
    const exp = new Uint8Array(expected.buffer, expected.byteOffset, expected.byteLength);
    const rowPitch = align(width * bytesPerPixel, kBytesPerRowAlignment);

    const readbackPromise = this.readGPUBufferRangeTyped(src, {
      type: Uint8Array,
      typedLength: rowPitch * height,
    });

    this.eventualAsyncExpectation(async niceStack => {
      const readback = await readbackPromise;
      const check = this.checkBufferWithRowPitch(
        readback.data,
        exp,
        width,
        height,
        rowPitch,
        bytesPerPixel,
        isFp16
      );
      if (check !== undefined) {
        niceStack.message = check;
        this.rec.expectationFailed(niceStack);
      }
      readback.cleanup();
    });
  }

  // TODO(crbug.com/dawn/868): Should be possible to consolidate this along with texture checking
  checkBufferWithRowPitch(
    actual: Uint8Array,
    exp: Uint8Array,
    width: number,
    height: number,
    rowPitch: number,
    bytesPerPixel: number,
    isFp16: boolean
  ): string | undefined {
    const bytesPerRow = width * bytesPerPixel;
    // When dst format is fp16 formats, the expectation and real result always has 1 bit difference in the ending
    // (e.g. CC vs CD) if there needs some alpha ops (if alpha channel is not 0.0 or 1.0). Suspect it is errors when
    // doing encoding. We check fp16 dst texture format with 1-bit ULP tolerance.
    if (isFp16) {
      for (let y = 0; y < height; ++y) {
        const expRow = exp.subarray(y * bytesPerRow, bytesPerRow);
        const checkResult = checkElementsBetween(actual.subarray(y * rowPitch, bytesPerRow), [
          i => (expRow[i] > 0 ? expRow[i] - 1 : expRow[i]),
          i => expRow[i] + 1,
        ]);
        if (checkResult !== undefined) return `on row ${y}: ${checkResult}`;
      }
    } else {
      for (let y = 0; y < height; ++y) {
        const checkResult = checkElementsEqual(
          actual.subarray(y * rowPitch, bytesPerRow),
          exp.subarray(y * bytesPerRow, bytesPerRow)
        );
        if (checkResult !== undefined) return `on row ${y}: ${checkResult}`;
      }
    }
    return undefined;
  }

  doTestAndCheckResult(
    imageCopyExternalImage: GPUImageCopyExternalImage,
    dstTextureCopyView: GPUImageCopyTextureTagged,
    copySize: GPUExtent3DDict,
    bytesPerPixel: number,
    expectedData: Uint8ClampedArray,
    isFp16: boolean
  ): void {
    this.device.queue.copyExternalImageToTexture(
      imageCopyExternalImage,
      dstTextureCopyView,
      copySize
    );

    const externalImage = imageCopyExternalImage.source;
    const dstTexture = dstTextureCopyView.texture;

    const bytesPerRow = align(externalImage.width * bytesPerPixel, kBytesPerRowAlignment);
    const testBuffer = this.device.createBuffer({
      size: bytesPerRow * externalImage.height,
      usage: GPUBufferUsage.COPY_SRC | GPUBufferUsage.COPY_DST,
    });
    this.trackForCleanup(testBuffer);

    const encoder = this.device.createCommandEncoder();

    encoder.copyTextureToBuffer(
      { texture: dstTexture, mipLevel: 0, origin: { x: 0, y: 0, z: 0 } },
      { buffer: testBuffer, bytesPerRow },
      { width: externalImage.width, height: externalImage.height, depthOrArrayLayers: 1 }
    );
    this.device.queue.submit([encoder.finish()]);

    this.checkCopyExternalImageResult(
      testBuffer,
      expectedData,
      externalImage.width,
      externalImage.height,
      bytesPerPixel,
      isFp16
    );
  }
}
