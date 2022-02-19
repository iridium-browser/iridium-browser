export const description = `
copyToTexture with HTMLCanvasElement and OffscreenCanvas sources.

TODO: consider whether external_texture and copyToTexture video tests should be in the same file
`;

import { makeTestGroup } from '../../../common/framework/test_group.js';
import { unreachable, assert, memcpy } from '../../../common/util/util.js';
import {
  RegularTextureFormat,
  kTextureFormatInfo,
  kValidTextureFormatsForCopyE2T,
} from '../../capability_info.js';
import { CopyToTextureUtils, isFp16Format } from '../../util/copy_to_texture.js';
import { canvasTypes, allCanvasTypes, createCanvas } from '../../util/create_elements.js';
import { kTexelRepresentationInfo } from '../../util/texture/texel_data.js';

/**
 * If the destination format specifies a transfer function,
 * copyExternalImageToTexture (like B2T/T2T copies) should ignore it.
 */
function formatForExpectedPixels(format: RegularTextureFormat): RegularTextureFormat {
  return format === 'rgba8unorm-srgb'
    ? 'rgba8unorm'
    : format === 'bgra8unorm-srgb'
    ? 'bgra8unorm'
    : format;
}

class F extends CopyToTextureUtils {
  // TODO: Cache the generated canvas to avoid duplicated initialization.
  init2DCanvasContent({
    canvasType,
    width,
    height,
    paintOpaqueRects,
  }: {
    canvasType: canvasTypes;
    width: number;
    height: number;
    paintOpaqueRects: boolean;
  }): {
    canvas: HTMLCanvasElement | OffscreenCanvas;
    canvasContext: CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D;
  } {
    const canvas = createCanvas(this, canvasType, width, height);

    let canvasContext = null;
    canvasContext = canvas.getContext('2d') as
      | CanvasRenderingContext2D
      | OffscreenCanvasRenderingContext2D
      | null;

    if (canvasContext === null) {
      this.skip(canvasType + ' canvas 2d context not available');
    }

    const rectWidth = Math.floor(width / 2);
    const rectHeight = Math.floor(height / 2);

    // The rgb10a2unorm dst texture will have tiny errors when we compare actual and expectation.
    // This is due to the convert from 8-bit to 10-bit combined with alpha value ops. So for
    // rgb10a2unorm dst textures, we'll set alphaValue to 1.0 to test.
    const alphaValue = paintOpaqueRects ? 1.0 : 0.6;
    const ctx = canvasContext as CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D;
    // Red
    ctx.fillStyle = `rgba(255, 0, 0, ${alphaValue})`;
    ctx.fillRect(0, 0, rectWidth, rectHeight);
    // Lime
    ctx.fillStyle = `rgba(0, 255, 0, ${alphaValue})`;
    ctx.fillRect(rectWidth, 0, width - rectWidth, rectHeight);
    // Blue
    ctx.fillStyle = `rgba(0, 0, 255, ${alphaValue})`;
    ctx.fillRect(0, rectHeight, rectWidth, height - rectHeight);
    // White
    ctx.fillStyle = `rgba(255, 255, 255, ${alphaValue})`;
    ctx.fillRect(rectWidth, rectHeight, width - rectWidth, height - rectHeight);

    return { canvas, canvasContext };
  }

  // TODO: Cache the generated canvas to avoid duplicated initialization.
  initGLCanvasContent({
    canvasType,
    contextName,
    width,
    height,
    premultiplied,
    paintOpaqueRects,
  }: {
    canvasType: canvasTypes;
    contextName: 'webgl' | 'webgl2';
    width: number;
    height: number;
    premultiplied: boolean;
    paintOpaqueRects: boolean;
  }): {
    canvas: HTMLCanvasElement | OffscreenCanvas;
    canvasContext: WebGLRenderingContext | WebGL2RenderingContext;
  } {
    const canvas = createCanvas(this, canvasType, width, height);

    const gl = canvas.getContext(contextName, { premultipliedAlpha: premultiplied }) as
      | WebGLRenderingContext
      | WebGL2RenderingContext
      | null;

    if (gl === null) {
      this.skip(canvasType + ' canvas ' + contextName + ' context not available');
    }
    this.trackForCleanup(gl);

    const rectWidth = Math.floor(width / 2);
    const rectHeight = Math.floor(height / 2);

    const alphaValue = paintOpaqueRects ? 1.0 : 0.6;
    const colorValue = premultiplied ? alphaValue : 1.0;

    // For webgl/webgl2 context canvas, if the context created with premultipliedAlpha attributes,
    // it means that the value in drawing buffer is premultiplied or not. So we should set
    // premultipliedAlpha value for premultipliedAlpha true gl context and unpremultipliedAlpha value
    // for the premulitpliedAlpha false gl context.
    gl.enable(gl.SCISSOR_TEST);
    gl.scissor(0, 0, rectWidth, rectHeight);
    gl.clearColor(colorValue, 0.0, 0.0, alphaValue);
    gl.clear(gl.COLOR_BUFFER_BIT);

    gl.scissor(rectWidth, 0, width - rectWidth, rectHeight);
    gl.clearColor(0.0, colorValue, 0.0, alphaValue);
    gl.clear(gl.COLOR_BUFFER_BIT);

    gl.scissor(0, rectHeight, rectWidth, height - rectHeight);
    gl.clearColor(0.0, 0.0, colorValue, alphaValue);
    gl.clear(gl.COLOR_BUFFER_BIT);

    gl.scissor(rectWidth, rectHeight, width - rectWidth, height - rectHeight);
    gl.clearColor(colorValue, colorValue, colorValue, alphaValue);
    gl.clear(gl.COLOR_BUFFER_BIT);

    return { canvas, canvasContext: gl };
  }

  getExpectedPixels({
    context,
    width,
    height,
    format,
    contextType,
    srcPremultiplied,
    dstPremultiplied,
  }: {
    context:
      | CanvasRenderingContext2D
      | OffscreenCanvasRenderingContext2D
      | WebGLRenderingContext
      | WebGL2RenderingContext;
    width: number;
    height: number;
    format: RegularTextureFormat;
    contextType: '2d' | 'gl';
    srcPremultiplied: boolean;
    dstPremultiplied: boolean;
  }): Uint8ClampedArray {
    const bytesPerPixel = kTextureFormatInfo[format].bytesPerBlock;

    const expectedPixels = new Uint8ClampedArray(bytesPerPixel * width * height);
    let sourcePixels;
    if (contextType === '2d') {
      const ctx = context as CanvasRenderingContext2D | OffscreenCanvasRenderingContext2D;
      sourcePixels = ctx.getImageData(0, 0, width, height).data;
    } else if (contextType === 'gl') {
      sourcePixels = new Uint8ClampedArray(width * height * 4);
      const gl = context as WebGLRenderingContext | WebGL2RenderingContext;
      gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, sourcePixels);
    } else {
      unreachable();
    }

    // Generate expectedPixels
    // Use getImageData and readPixels to get canvas contents.
    const rep = kTexelRepresentationInfo[format];
    const divide = 255.0;
    let rgba: { R: number; G: number; B: number; A: number };
    for (let i = 0; i < height; ++i) {
      for (let j = 0; j < width; ++j) {
        const pixelPos = i * width + j;

        rgba = {
          R: sourcePixels[pixelPos * 4] / divide,
          G: sourcePixels[pixelPos * 4 + 1] / divide,
          B: sourcePixels[pixelPos * 4 + 2] / divide,
          A: sourcePixels[pixelPos * 4 + 3] / divide,
        };

        if (!srcPremultiplied && dstPremultiplied) {
          rgba.R *= rgba.A;
          rgba.G *= rgba.A;
          rgba.B *= rgba.A;
        }

        if (srcPremultiplied && !dstPremultiplied) {
          assert(rgba.A !== 0.0);
          rgba.R /= rgba.A;
          rgba.G /= rgba.A;
          rgba.B /= rgba.A;
        }

        // WebGL readPixel returns pixels from bottom-left origin. Using CopyExternalImageToTexture
        // to copy from WebGL Canvas keeps top-left origin. So the expectation from webgl.readPixel should
        // be flipped.
        const dstPixelPos = contextType === 'gl' ? (height - i - 1) * width + j : pixelPos;

        memcpy(
          { src: rep.pack(rep.encode(rgba)) },
          { dst: expectedPixels, start: dstPixelPos * bytesPerPixel }
        );
      }
    }

    return expectedPixels;
  }
}

export const g = makeTestGroup(F);

g.test('copy_contents_from_2d_context_canvas')
  .desc(
    `
  Test HTMLCanvasElement and OffscreenCanvas with 2d context
  can be copied to WebGPU texture correctly.

  It creates HTMLCanvasElement/OffscreenCanvas with '2d'.
  Use fillRect(2d context) to render red rect for top-left,
  green rect for top-right, blue rect for bottom-left and white for bottom-right.

  Then call copyExternalImageToTexture() to do a full copy to the 0 mipLevel
  of dst texture, and read the contents out to compare with the canvas contents.

  The tests covers:
  - Valid canvas type
  - Valid 2d context type
  - TODO: color space tests need to be added
  - TODO: Add error tolerance for rgb10a2unorm dst texture format

  And the expected results are all passed.
  `
  )
  .params(u =>
    u
      .combine('canvasType', allCanvasTypes)
      .combine('dstColorFormat', kValidTextureFormatsForCopyE2T)
      .combine('dstPremultiplied', [true, false])
      .beginSubcases()
      .combine('width', [1, 2, 4, 15, 255, 256])
      .combine('height', [1, 2, 4, 15, 255, 256])
  )
  .fn(async t => {
    const { width, height, canvasType, dstColorFormat, dstPremultiplied } = t.params;

    // When dst texture format is rgb10a2unorm, the generated expected value of the result
    // may have tiny errors compared to the actual result when the channel value is
    // not 1.0 or 0.0.
    // For example, we init the pixel with r channel to 0.6. And the denormalized value for
    // 10-bit channel is 613.8, which needs to call "round" or other function to get an integer.
    // It is possible that gpu adopt different "round" as our cpu implementation(we use Math.round())
    // and it will generate tiny errors.
    // So the cases with rgb10a2unorm dst texture format are handled specially by painting opaque rects
    // to ensure they will have stable result after alphaOps(should keep the same value).
    const { canvas, canvasContext } = t.init2DCanvasContent({
      canvasType,
      width,
      height,
      paintOpaqueRects: dstColorFormat === 'rgb10a2unorm',
    });

    const dst = t.device.createTexture({
      size: {
        width,
        height,
        depthOrArrayLayers: 1,
      },
      format: dstColorFormat,
      usage:
        GPUTextureUsage.COPY_DST | GPUTextureUsage.COPY_SRC | GPUTextureUsage.RENDER_ATTACHMENT,
    });

    // Construct expected value for different dst color format
    const dstBytesPerPixel = kTextureFormatInfo[dstColorFormat].bytesPerBlock;
    const format: RegularTextureFormat = formatForExpectedPixels(dstColorFormat);

    // For 2d canvas, get expected pixels with getImageData(), which returns unpremultiplied
    // values.
    const expectedPixels = t.getExpectedPixels({
      context: canvasContext,
      width,
      height,
      format,
      contextType: '2d',
      srcPremultiplied: false,
      dstPremultiplied,
    });

    t.doTestAndCheckResult(
      { source: canvas, origin: { x: 0, y: 0 } },
      {
        texture: dst,
        origin: { x: 0, y: 0 },
        colorSpace: 'srgb',
        premultipliedAlpha: dstPremultiplied,
      },
      { width: canvas.width, height: canvas.height, depthOrArrayLayers: 1 },
      dstBytesPerPixel,
      expectedPixels,
      isFp16Format(dstColorFormat)
    );
  });

g.test('copy_contents_from_gl_context_canvas')
  .desc(
    `
  Test HTMLCanvasElement and OffscreenCanvas with webgl/webgl2 context
  can be copied to WebGPU texture correctly.

  It creates HTMLCanvasElement/OffscreenCanvas with webgl'/'webgl2'.
  Use stencil + clear to render red rect for top-left, green rect
  for top-right, blue rect for bottom-left and white for bottom-right.
  And do premultiply alpha in advance if the webgl/webgl2 context is created
  with premultipliedAlpha : true.

  Then call copyExternalImageToTexture() to do a full copy to the 0 mipLevel
  of dst texture, and read the contents out to compare with the canvas contents.

  The tests covers:
  - Valid canvas type
  - Valid webgl/webgl2 context type
  - TODO: color space tests need to be added
  - TODO: Add error tolerance for rgb10a2unorm dst texture format

  And the expected results are all passed.
  `
  )
  .params(u =>
    u
      .combine('canvasType', allCanvasTypes)
      .combine('contextName', ['webgl', 'webgl2'] as const)
      .combine('dstColorFormat', kValidTextureFormatsForCopyE2T)
      .combine('srcPremultiplied', [true, false])
      .combine('dstPremultiplied', [true, false])
      .beginSubcases()
      .combine('width', [1, 2, 4, 15, 255, 256])
      .combine('height', [1, 2, 4, 15, 255, 256])
  )
  .fn(async t => {
    const {
      width,
      height,
      canvasType,
      contextName,
      dstColorFormat,
      srcPremultiplied,
      dstPremultiplied,
    } = t.params;

    // When dst texture format is rgb10a2unorm, the generated expected value of the result
    // may have tiny errors compared to the actual result when the channel value is
    // not 1.0 or 0.0.
    // For example, we init the pixel with r channel to 0.6. And the denormalized value for
    // 10-bit channel is 613.8, which needs to call "round" or other function to get an integer.
    // It is possible that gpu adopt different "round" as our cpu implementation(we use Math.round())
    // and it will generate tiny errors.
    // So the cases with rgb10a2unorm dst texture format are handled specially by by painting opaque rects
    // to ensure they will have stable result after alphaOps(should keep the same value).
    const { canvas, canvasContext } = t.initGLCanvasContent({
      canvasType,
      contextName,
      width,
      height,
      premultiplied: srcPremultiplied,
      paintOpaqueRects: dstColorFormat === 'rgb10a2unorm',
    });

    const dst = t.device.createTexture({
      size: {
        width,
        height,
        depthOrArrayLayers: 1,
      },
      format: dstColorFormat,
      usage:
        GPUTextureUsage.COPY_DST | GPUTextureUsage.COPY_SRC | GPUTextureUsage.RENDER_ATTACHMENT,
    });

    // Construct expected value for different dst color format
    const dstBytesPerPixel = kTextureFormatInfo[dstColorFormat].bytesPerBlock;
    const format: RegularTextureFormat = formatForExpectedPixels(dstColorFormat);
    const expectedPixels = t.getExpectedPixels({
      context: canvasContext,
      width,
      height,
      format,
      contextType: 'gl',
      srcPremultiplied,
      dstPremultiplied,
    });

    t.doTestAndCheckResult(
      { source: canvas, origin: { x: 0, y: 0 } },
      {
        texture: dst,
        origin: { x: 0, y: 0 },
        colorSpace: 'srgb',
        premultipliedAlpha: dstPremultiplied,
      },
      { width: canvas.width, height: canvas.height, depthOrArrayLayers: 1 },
      dstBytesPerPixel,
      expectedPixels,
      isFp16Format(dstColorFormat)
    );
  });
