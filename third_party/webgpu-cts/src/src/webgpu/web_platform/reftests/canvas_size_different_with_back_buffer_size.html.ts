import { assert } from '../../../common/util/util.js';

import { runRefTest } from './gpu_ref_test.js';

// <canvas> element from html page
declare const cvs_larger_than_back_buffer: HTMLCanvasElement;
declare const cvs_same_as_back_buffer: HTMLCanvasElement;
declare const cvs_smaller_than_back_buffer: HTMLCanvasElement;
declare const cvs_change_size_after_configure: HTMLCanvasElement;
declare const cvs_change_size_and_reconfigure: HTMLCanvasElement;
declare const back_buffer_smaller_than_cvs_and_css: HTMLCanvasElement;
declare const cvs_smaller_than_back_buffer_and_css: HTMLCanvasElement;

export function run() {
  runRefTest(async t => {
    const red = new Uint8Array([0x00, 0x00, 0xff, 0xff]);
    const green = new Uint8Array([0x00, 0xff, 0x00, 0xff]);
    const blue = new Uint8Array([0xff, 0x00, 0x00, 0xff]);
    const yellow = new Uint8Array([0x00, 0xff, 0xff, 0xff]);
    const pixelBytes = 4;

    function setRowColors(data: Uint8Array, offset: number, colors: Array<Uint8Array>) {
      for (let i = 0; i < colors.length; ++i) {
        data.set(colors[i], offset + i * pixelBytes);
      }
    }

    function updateWebGPUBackBuffer(
      canvas: HTMLCanvasElement,
      ctx: GPUCanvasContext,
      configureSize: [number, number] | undefined,
      pixels: Array<Array<Uint8Array>>
    ) {
      const backBufferSize = configureSize ?? [canvas.width, canvas.height];
      ctx.configure({
        device: t.device,
        format: 'bgra8unorm',
        usage: GPUTextureUsage.COPY_DST,
        size: configureSize,
      });

      const rows = pixels.length;
      const bytesPerRow = 256;
      const buffer = t.device.createBuffer({
        mappedAtCreation: true,
        size: rows * bytesPerRow,
        usage: GPUBufferUsage.COPY_SRC,
      });
      const mapping = buffer.getMappedRange();
      const data = new Uint8Array(mapping);

      for (let i = 0; i < pixels.length; ++i) {
        setRowColors(data, bytesPerRow * i, pixels[i]);
      }

      buffer.unmap();
      const texture = ctx.getCurrentTexture();

      const encoder = t.device.createCommandEncoder();
      encoder.copyBufferToTexture({ buffer, bytesPerRow }, { texture }, backBufferSize);
      t.device.queue.submit([encoder.finish()]);
      buffer.destroy();
    }

    // Test back buffer smaller than canvas size
    {
      const back_buffer_width = cvs_larger_than_back_buffer.width / 2;
      const back_buffer_height = cvs_larger_than_back_buffer.height / 2;
      const ctx = cvs_larger_than_back_buffer.getContext('webgpu');
      assert(ctx !== null);

      updateWebGPUBackBuffer(
        cvs_larger_than_back_buffer,
        ctx,
        [back_buffer_width, back_buffer_height],
        [
          [red, red, green],
          [red, red, green],
          [blue, blue, yellow],
          [blue, blue, yellow],
        ]
      );
    }

    // Test back buffer is same as canvas size
    {
      const ctx = cvs_same_as_back_buffer.getContext('webgpu');
      assert(ctx !== null);

      updateWebGPUBackBuffer(cvs_same_as_back_buffer, ctx, undefined, [
        [red, red, green],
        [red, red, green],
        [blue, blue, yellow],
        [blue, blue, yellow],
      ]);
    }

    // Test back buffer is larger than canvas size.
    {
      const back_buffer_width = cvs_smaller_than_back_buffer.width * 2;
      const back_buffer_height = cvs_smaller_than_back_buffer.height * 2;
      const ctx = cvs_smaller_than_back_buffer.getContext('webgpu');
      assert(ctx !== null);

      updateWebGPUBackBuffer(
        cvs_smaller_than_back_buffer,
        ctx,
        [back_buffer_width, back_buffer_height],
        [
          [red, red, red, red, green, green],
          [red, red, red, red, green, green],
          [red, red, red, red, green, green],
          [red, red, red, red, green, green],
          [blue, blue, blue, blue, yellow, yellow],
          [blue, blue, blue, blue, yellow, yellow],
          [blue, blue, blue, blue, yellow, yellow],
          [blue, blue, blue, blue, yellow, yellow],
        ]
      );
    }

    // Test js change canvas size after back buffer has been configured
    {
      const back_buffer_width = cvs_change_size_after_configure.width;
      const back_buffer_height = cvs_change_size_after_configure.height;
      const ctx = cvs_change_size_after_configure.getContext('webgpu');
      assert(ctx !== null);

      updateWebGPUBackBuffer(
        cvs_change_size_after_configure,
        ctx,
        [back_buffer_width, back_buffer_height],
        [
          [red, red, green],
          [red, red, green],
          [blue, blue, yellow],
          [blue, blue, yellow],
        ]
      );

      cvs_change_size_after_configure.width = 6;
      cvs_change_size_after_configure.height = 8;
    }

    // Test js change canvas size after back buffer has been configured
    // and back buffer configure again.
    {
      const ctx = cvs_change_size_and_reconfigure.getContext('webgpu');
      assert(ctx !== null);

      updateWebGPUBackBuffer(cvs_change_size_and_reconfigure, ctx, undefined, [
        [red, red, green],
        [red, red, green],
        [blue, blue, yellow],
        [blue, blue, yellow],
      ]);

      cvs_change_size_and_reconfigure.width = 6;
      cvs_change_size_and_reconfigure.height = 8;

      updateWebGPUBackBuffer(cvs_change_size_and_reconfigure, ctx, undefined, [
        [red, red, red, red, green, green],
        [red, red, red, red, green, green],
        [red, red, red, red, green, green],
        [red, red, red, red, green, green],
        [blue, blue, blue, blue, yellow, yellow],
        [blue, blue, blue, blue, yellow, yellow],
        [blue, blue, blue, blue, yellow, yellow],
        [blue, blue, blue, blue, yellow, yellow],
      ]);
    }

    // Test back buffer size < canvas size < CSS size
    {
      const back_buffer_width = back_buffer_smaller_than_cvs_and_css.width / 2;
      const back_buffer_height = back_buffer_smaller_than_cvs_and_css.height / 2;
      const ctx = back_buffer_smaller_than_cvs_and_css.getContext('webgpu');
      assert(ctx !== null);

      updateWebGPUBackBuffer(
        back_buffer_smaller_than_cvs_and_css,
        ctx,
        [back_buffer_width, back_buffer_height],
        [
          [red, red, green],
          [red, red, green],
          [blue, blue, yellow],
          [blue, blue, yellow],
        ]
      );
    }

    // Test canvas size < back buffer size < CSS size
    {
      const back_buffer_width = cvs_smaller_than_back_buffer_and_css.width * 2;
      const back_buffer_height = cvs_smaller_than_back_buffer_and_css.height * 2;
      const ctx = cvs_smaller_than_back_buffer_and_css.getContext('webgpu');
      assert(ctx !== null);

      updateWebGPUBackBuffer(
        cvs_smaller_than_back_buffer_and_css,
        ctx,
        [back_buffer_width, back_buffer_height],
        [
          [red, red, red, red, green, green],
          [red, red, green, red, green, green],
          [red, red, red, red, green, green],
          [red, red, red, red, green, green],
          [blue, blue, blue, blue, yellow, yellow],
          [blue, blue, blue, blue, yellow, yellow],
          [blue, blue, blue, blue, yellow, yellow],
          [blue, blue, blue, blue, yellow, yellow],
        ]
      );
    }
  });
}
