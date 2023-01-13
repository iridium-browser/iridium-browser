export const description = `
Tests for readback from WebGPU Canvas.

TODO: implement all canvas types, see TODO on kCanvasTypes.
`;

import { makeTestGroup } from '../../../common/framework/test_group.js';
import { assert, raceWithRejectOnTimeout, unreachable } from '../../../common/util/util.js';
import { kCanvasAlphaModes, kCanvasTextureFormats } from '../../capability_info.js';
import { GPUTest } from '../../gpu_test.js';
import { checkElementsEqual } from '../../util/check_contents.js';
import {
  kAllCanvasTypes,
  CanvasType,
  createCanvas,
  createOnscreenCanvas,
} from '../../util/create_elements.js';

export const g = makeTestGroup(GPUTest);

// We choose 0x66 as the value for each color and alpha channel
// 0x66 / 0xff = 0.4
// Given a pixel value of RGBA = (0x66, 0, 0, 0x66) in the source WebGPU canvas,
// For alphaMode = opaque, the copy output should be RGBA = (0x66, 0, 0, 0xff)
// For alphaMode = premultiplied, the copy output should be RGBA = (0xff, 0, 0, 0x66)
const kPixelValue = 0x66;
const kPixelValueFloat = 0x66 / 0xff; // 0.4

// Use four pixels rectangle for the test:
// blue: top-left;
// green: top-right;
// red: bottom-left;
// yellow: bottom-right;
const expect = {
  /* prettier-ignore */
  'opaque': new Uint8ClampedArray([
    0, 0, kPixelValue, 0xff, // blue
    0, kPixelValue, 0, 0xff, // green
    kPixelValue, 0, 0, 0xff, // red
    kPixelValue, kPixelValue, 0, 0xff, // yellow
  ]),
  /* prettier-ignore */
  'premultiplied': new Uint8ClampedArray([
    0, 0, 0xff, kPixelValue, // blue
    0, 0xff, 0, kPixelValue, // green
    0xff, 0, 0, kPixelValue, // red
    0xff, 0xff, 0, kPixelValue, // yellow
  ]),
};

async function initCanvasContent<T extends CanvasType>(
  t: GPUTest,
  format: GPUTextureFormat,
  alphaMode: GPUCanvasAlphaMode,
  canvasType: T
) {
  const canvas = createCanvas(t, canvasType, 2, 2);
  const ctx = canvas.getContext('webgpu');
  assert(ctx instanceof GPUCanvasContext, 'Failed to get WebGPU context from canvas');

  ctx.configure({
    device: t.device,
    format,
    usage: GPUTextureUsage.COPY_SRC | GPUTextureUsage.COPY_DST,
    alphaMode,
  });

  const canvasTexture = ctx.getCurrentTexture();
  const tempTexture = t.device.createTexture({
    size: { width: 1, height: 1, depthOrArrayLayers: 1 },
    format,
    usage: GPUTextureUsage.COPY_SRC | GPUTextureUsage.RENDER_ATTACHMENT,
  });
  const tempTextureView = tempTexture.createView();
  const encoder = t.device.createCommandEncoder();

  const clearOnePixel = (origin: GPUOrigin3D, color: GPUColor) => {
    const pass = encoder.beginRenderPass({
      colorAttachments: [
        { view: tempTextureView, clearValue: color, loadOp: 'clear', storeOp: 'store' },
      ],
    });
    pass.end();
    encoder.copyTextureToTexture(
      { texture: tempTexture },
      { texture: canvasTexture, origin },
      { width: 1, height: 1 }
    );
  };

  clearOnePixel([0, 0], [0, 0, kPixelValueFloat, kPixelValueFloat]);
  clearOnePixel([1, 0], [0, kPixelValueFloat, 0, kPixelValueFloat]);
  clearOnePixel([0, 1], [kPixelValueFloat, 0, 0, kPixelValueFloat]);
  clearOnePixel([1, 1], [kPixelValueFloat, kPixelValueFloat, 0, kPixelValueFloat]);

  t.device.queue.submit([encoder.finish()]);
  tempTexture.destroy();

  await t.device.queue.onSubmittedWorkDone();

  return canvas;
}

function checkImageResult(t: GPUTest, image: CanvasImageSource, expect: Uint8ClampedArray) {
  const canvas: HTMLCanvasElement = createOnscreenCanvas(t, 2, 2);
  const ctx = canvas.getContext('2d');
  assert(ctx !== null);
  ctx.drawImage(image, 0, 0);
  readPixelsFrom2DCanvasAndCompare(t, ctx, expect);
}

function readPixelsFrom2DCanvasAndCompare(
  t: GPUTest,
  ctx: CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D,
  expect: Uint8ClampedArray
) {
  const actual = ctx.getImageData(0, 0, 2, 2).data;

  t.expectOK(checkElementsEqual(actual, expect));
}

g.test('onscreenCanvas,snapshot')
  .desc(
    `
    Ensure snapshot of canvas with WebGPU context is correct with
    - various WebGPU canvas texture formats
    - WebGPU canvas alpha mode = {"opaque", "premultiplied"}
    - snapshot methods = {convertToBlob, transferToImageBitmap, createImageBitmap}

    TODO: Snapshot canvas to jpeg, webp and other mime type and
          different quality. Maybe we should test them in reftest.
    `
  )
  .params(u =>
    u //
      .combine('format', kCanvasTextureFormats)
      .combine('alphaMode', kCanvasAlphaModes)
      .combine('snapshotType', ['toDataURL', 'toBlob', 'imageBitmap'])
  )
  .fn(async t => {
    const canvas = await initCanvasContent(t, t.params.format, t.params.alphaMode, 'onscreen');

    let snapshot: HTMLImageElement | ImageBitmap;
    switch (t.params.snapshotType) {
      case 'toDataURL': {
        const url = canvas.toDataURL();
        const img = new Image(canvas.width, canvas.height);
        img.src = url;
        await raceWithRejectOnTimeout(img.decode(), 5000, 'load image timeout');
        snapshot = img;
        break;
      }
      case 'toBlob': {
        const blobFromCanvas = new Promise(resolve => {
          canvas.toBlob(blob => resolve(blob));
        });
        const blob = (await blobFromCanvas) as Blob;
        const url = URL.createObjectURL(blob);
        const img = new Image(canvas.width, canvas.height);
        img.src = url;
        await raceWithRejectOnTimeout(img.decode(), 5000, 'load image timeout');
        snapshot = img;
        break;
      }
      case 'imageBitmap': {
        snapshot = await createImageBitmap(canvas);
        break;
      }
      default:
        unreachable();
    }

    checkImageResult(t, snapshot, expect[t.params.alphaMode]);
  });

g.test('offscreenCanvas,snapshot')
  .desc(
    `
    Ensure snapshot of offscreenCanvas with WebGPU context is correct with
    - various WebGPU canvas texture formats
    - WebGPU canvas alpha mode = {"opaque", "premultiplied"}
    - snapshot methods = {convertToBlob, transferToImageBitmap, createImageBitmap}

    TODO: Snapshot offscreenCanvas to jpeg, webp and other mime type and
          different quality. Maybe we should test them in reftest.
    `
  )
  .params(u =>
    u //
      .combine('format', kCanvasTextureFormats)
      .combine('alphaMode', kCanvasAlphaModes)
      .combine('snapshotType', ['convertToBlob', 'transferToImageBitmap', 'imageBitmap'])
  )
  .fn(async t => {
    const offscreenCanvas = await initCanvasContent(
      t,
      t.params.format,
      t.params.alphaMode,
      'offscreen'
    );

    let snapshot: HTMLImageElement | ImageBitmap;
    switch (t.params.snapshotType) {
      case 'convertToBlob': {
        if (typeof offscreenCanvas.convertToBlob === undefined) {
          t.skip("Browser doesn't support OffscreenCanvas.convertToBlob");
          return;
        }
        const blob = await offscreenCanvas.convertToBlob();
        const url = URL.createObjectURL(blob);
        const img = new Image(offscreenCanvas.width, offscreenCanvas.height);
        img.src = url;
        await raceWithRejectOnTimeout(img.decode(), 5000, 'load image timeout');
        snapshot = img;
        break;
      }
      case 'transferToImageBitmap': {
        if (typeof offscreenCanvas.transferToImageBitmap === undefined) {
          t.skip("Browser doesn't support OffscreenCanvas.transferToImageBitmap");
          return;
        }
        snapshot = offscreenCanvas.transferToImageBitmap();
        break;
      }
      case 'imageBitmap': {
        snapshot = await createImageBitmap(offscreenCanvas);
        break;
      }
      default:
        unreachable();
    }

    checkImageResult(t, snapshot, expect[t.params.alphaMode]);
  });

g.test('onscreenCanvas,uploadToWebGL')
  .desc(
    `
    Ensure upload WebGPU context canvas to webgl texture is correct with
    - various WebGPU canvas texture formats
    - WebGPU canvas alpha mode = {"opaque", "premultiplied"}
    - upload methods = {texImage2D, texSubImage2D}
    `
  )
  .params(u =>
    u //
      .combine('format', kCanvasTextureFormats)
      .combine('alphaMode', kCanvasAlphaModes)
      .combine('webgl', ['webgl', 'webgl2'])
      .combine('upload', ['texImage2D', 'texSubImage2D'])
  )
  .fn(async t => {
    const { format, webgl, upload } = t.params;
    const canvas = await initCanvasContent(t, format, t.params.alphaMode, 'onscreen');

    const expectCanvas: HTMLCanvasElement = createOnscreenCanvas(t, canvas.width, canvas.height);
    const gl = expectCanvas.getContext(webgl) as WebGLRenderingContext | WebGL2RenderingContext;
    if (gl === null) {
      return;
    }

    const texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
    switch (upload) {
      case 'texImage2D': {
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, canvas);
        break;
      }
      case 'texSubImage2D': {
        gl.texImage2D(
          gl.TEXTURE_2D,
          0,
          gl.RGBA,
          canvas.width,
          canvas.height,
          0,
          gl.RGBA,
          gl.UNSIGNED_BYTE,
          null
        );
        gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, gl.RGBA, gl.UNSIGNED_BYTE, canvas);
        break;
      }
      default:
        unreachable();
    }

    const fb = gl.createFramebuffer();

    gl.bindFramebuffer(gl.FRAMEBUFFER, fb);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, texture, 0);

    const pixels = new Uint8Array(canvas.width * canvas.height * 4);
    gl.readPixels(0, 0, 2, 2, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
    const actual = new Uint8ClampedArray(pixels);

    t.expectOK(checkElementsEqual(actual, expect[t.params.alphaMode]));
  });

g.test('drawTo2DCanvas')
  .desc(
    `
    Ensure draw WebGPU context canvas to 2d context canvas/offscreenCanvas is correct with
    - various WebGPU canvas texture formats
    - WebGPU canvas alpha mode = {"opaque", "premultiplied"}
    - WebGPU canvas type = {"onscreen", "offscreen"}
    - 2d canvas type = {"onscreen", "offscreen"}
    `
  )
  .params(u =>
    u //
      .combine('format', kCanvasTextureFormats)
      .combine('alphaMode', kCanvasAlphaModes)
      .combine('webgpuCanvasType', kAllCanvasTypes)
      .combine('canvas2DType', kAllCanvasTypes)
  )
  .fn(async t => {
    const { format, webgpuCanvasType, alphaMode, canvas2DType } = t.params;

    const canvas = await initCanvasContent(t, format, alphaMode, webgpuCanvasType);

    const expectCanvas = createCanvas(t, canvas2DType, canvas.width, canvas.height);
    const ctx = expectCanvas.getContext('2d');
    if (ctx === null) {
      t.skip(canvas2DType + ' canvas cannot get 2d context');
      return;
    }
    ctx.drawImage(canvas, 0, 0);

    readPixelsFrom2DCanvasAndCompare(t, ctx, expect[t.params.alphaMode]);
  });
