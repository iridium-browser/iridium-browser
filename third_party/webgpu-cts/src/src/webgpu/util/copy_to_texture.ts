import { GPUTest } from '../gpu_test.js';

import { checkElementsEqual } from './check_contents.js';
import { align } from './math.js';
import { kBytesPerRowAlignment } from './texture/layout.js';

export class CopyToTextureUtils extends GPUTest {
  // TODO(crbug.com/dawn/868): Should be possible to consolidate this along with texture checking
  checkCopyExternalImageResult(
    src: GPUBuffer,
    expected: ArrayBufferView,
    width: number,
    height: number,
    bytesPerPixel: number
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
        bytesPerPixel
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
    bytesPerPixel: number
  ): string | undefined {
    const bytesPerRow = width * bytesPerPixel;
    for (let y = 0; y < height; ++y) {
      const checkResult = checkElementsEqual(
        actual.subarray(y * rowPitch, bytesPerRow),
        exp.subarray(y * bytesPerRow, bytesPerRow)
      );
      if (checkResult !== undefined) return `on row ${y}: ${checkResult}`;
    }
    return undefined;
  }

  doTestAndCheckResult(
    imageCopyExternalImage: GPUImageCopyExternalImage,
    dstTextureCopyView: GPUImageCopyTextureTagged,
    copySize: GPUExtent3DDict,
    bytesPerPixel: number,
    expectedData: Uint8ClampedArray
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
      bytesPerPixel
    );
  }
}
