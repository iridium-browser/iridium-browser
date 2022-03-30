import { assert, unreachable } from '../../../common/util/util.js';
import { gammaDecompress } from '../../util/conversion.js';

import { runRefTest } from './gpu_ref_test.js';

type WriteCanvasMethod =
  | 'copyBufferToTexture'
  | 'copyTextureToTexture'
  | 'copyExternalImageToTexture'
  | 'DrawTextureSample'
  | 'DrawVertexColor'
  | 'DrawFragcoord';

export function run(
  format: GPUTextureFormat,
  targets: { cvs: HTMLCanvasElement; writeCanvasMethod: WriteCanvasMethod }[]
) {
  runRefTest(async t => {
    let shaderValue: number = 0x7f / 0xff;
    let isOutputSrgb = false;
    switch (format) {
      case 'bgra8unorm':
      case 'rgba8unorm':
        break;
      case 'bgra8unorm-srgb':
      case 'rgba8unorm-srgb':
        // NOTE: "-srgb" cases haven't been tested (there aren't any .html files that use them).

        // Reverse gammaCompress to get same value shader output as non-srgb formats:
        shaderValue = gammaDecompress(shaderValue);
        isOutputSrgb = true;
        break;
      default:
        unreachable();
    }
    const shaderValueStr = shaderValue.toFixed(5);

    function copyBufferToTexture(ctx: GPUCanvasContext) {
      const rows = 2;
      const bytesPerRow = 256;
      const buffer = t.device.createBuffer({
        mappedAtCreation: true,
        size: rows * bytesPerRow,
        usage: GPUBufferUsage.COPY_SRC,
      });
      const mapping = buffer.getMappedRange();
      switch (format) {
        case 'bgra8unorm':
        case 'bgra8unorm-srgb':
          {
            const data = new Uint8Array(mapping);
            data.set(new Uint8Array([0x00, 0x00, 0x7f, 0xff]), 0); // red
            data.set(new Uint8Array([0x00, 0x7f, 0x00, 0xff]), 4); // green
            data.set(new Uint8Array([0x7f, 0x00, 0x00, 0xff]), 256 + 0); // blue
            data.set(new Uint8Array([0x00, 0x7f, 0x7f, 0xff]), 256 + 4); // yellow
          }
          break;
        case 'rgba8unorm':
        case 'rgba8unorm-srgb':
          {
            const data = new Uint8Array(mapping);
            data.set(new Uint8Array([0x7f, 0x00, 0x00, 0xff]), 0); // red
            data.set(new Uint8Array([0x00, 0x7f, 0x00, 0xff]), 4); // green
            data.set(new Uint8Array([0x00, 0x00, 0x7f, 0xff]), 256 + 0); // blue
            data.set(new Uint8Array([0x7f, 0x7f, 0x00, 0xff]), 256 + 4); // yellow
          }
          break;
      }
      buffer.unmap();

      const encoder = t.device.createCommandEncoder();
      encoder.copyBufferToTexture({ buffer, bytesPerRow }, { texture: ctx.getCurrentTexture() }, [
        2,
        2,
        1,
      ]);
      t.device.queue.submit([encoder.finish()]);
    }

    function getImageBitmap(): Promise<ImageBitmap> {
      const imageData = new ImageData(
        /* prettier-ignore */ new Uint8ClampedArray([
          0x7f, 0x00, 0x00, 0xff,
          0x00, 0x7f, 0x00, 0xff,
          0x00, 0x00, 0x7f, 0xff,
          0x7f, 0x7f, 0x00, 0xff,
        ]),
        2,
        2
      );
      return createImageBitmap(imageData);
    }

    function setupSrcTexture(imageBitmap: ImageBitmap): GPUTexture {
      const [srcWidth, srcHeight] = [imageBitmap.width, imageBitmap.height];
      const srcTexture = t.device.createTexture({
        size: [srcWidth, srcHeight, 1],
        format,
        usage:
          GPUTextureUsage.TEXTURE_BINDING |
          GPUTextureUsage.RENDER_ATTACHMENT |
          GPUTextureUsage.COPY_DST |
          GPUTextureUsage.COPY_SRC,
      });
      t.device.queue.copyExternalImageToTexture({ source: imageBitmap }, { texture: srcTexture }, [
        imageBitmap.width,
        imageBitmap.height,
      ]);
      return srcTexture;
    }

    async function copyExternalImageToTexture(ctx: GPUCanvasContext) {
      const imageBitmap = await getImageBitmap();
      t.device.queue.copyExternalImageToTexture(
        { source: imageBitmap },
        { texture: ctx.getCurrentTexture() },
        [imageBitmap.width, imageBitmap.height]
      );
    }

    async function copyTextureToTexture(ctx: GPUCanvasContext) {
      const imageBitmap = await getImageBitmap();
      const srcTexture = setupSrcTexture(imageBitmap);

      const encoder = t.device.createCommandEncoder();
      encoder.copyTextureToTexture(
        { texture: srcTexture, mipLevel: 0, origin: { x: 0, y: 0, z: 0 } },
        { texture: ctx.getCurrentTexture(), mipLevel: 0, origin: { x: 0, y: 0, z: 0 } },
        [imageBitmap.width, imageBitmap.height, 1]
      );
      t.device.queue.submit([encoder.finish()]);
    }

    async function DrawTextureSample(ctx: GPUCanvasContext) {
      const imageBitmap = await getImageBitmap();
      const srcTexture = setupSrcTexture(imageBitmap);

      const pipeline = t.device.createRenderPipeline({
        vertex: {
          module: t.device.createShaderModule({
            code: `
struct VertexOutput {
  @builtin(position) Position : vec4<f32>;
  @location(0) fragUV : vec2<f32>;
};

@stage(vertex)
fn main(@builtin(vertex_index) VertexIndex : u32) -> VertexOutput {
  var pos = array<vec2<f32>, 6>(
      vec2<f32>( 1.0,  1.0),
      vec2<f32>( 1.0, -1.0),
      vec2<f32>(-1.0, -1.0),
      vec2<f32>( 1.0,  1.0),
      vec2<f32>(-1.0, -1.0),
      vec2<f32>(-1.0,  1.0));

  var uv = array<vec2<f32>, 6>(
      vec2<f32>(1.0, 0.0),
      vec2<f32>(1.0, 1.0),
      vec2<f32>(0.0, 1.0),
      vec2<f32>(1.0, 0.0),
      vec2<f32>(0.0, 1.0),
      vec2<f32>(0.0, 0.0));

  var output : VertexOutput;
  output.Position = vec4<f32>(pos[VertexIndex], 0.0, 1.0);
  output.fragUV = uv[VertexIndex];
  return output;
}
            `,
          }),
          entryPoint: 'main',
        },
        fragment: {
          module: t.device.createShaderModule({
            // NOTE: "-srgb" cases haven't been tested (there aren't any .html files that use them).
            code: `
@group(0) @binding(0) var mySampler: sampler;
@group(0) @binding(1) var myTexture: texture_2d<f32>;

fn gammaDecompress(n: f32) -> f32 {
  var r = n;
  if (r <= 0.04045) {
    r = r * 25.0 / 323.0;
  } else {
    r = pow((200.0 * r + 11.0) / 121.0, 12.0 / 5.0);
  }
  r = clamp(r, 0.0, 1.0);
  return r;
}

@stage(fragment)
fn srgbMain(@location(0) fragUV: vec2<f32>) -> @location(0) vec4<f32> {
  var result = textureSample(myTexture, mySampler, fragUV);
  result.r = gammaDecompress(result.r);
  result.g = gammaDecompress(result.g);
  result.b = gammaDecompress(result.b);
  return result;
}

@stage(fragment)
fn linearMain(@location(0) fragUV: vec2<f32>) -> @location(0) vec4<f32> {
  return textureSample(myTexture, mySampler, fragUV);
}
            `,
          }),
          entryPoint: isOutputSrgb ? 'srgbMain' : 'linearMain',
          targets: [{ format }],
        },
        primitive: {
          topology: 'triangle-list',
        },
      });

      const sampler = t.device.createSampler({
        magFilter: 'nearest',
        minFilter: 'nearest',
      });

      const uniformBindGroup = t.device.createBindGroup({
        layout: pipeline.getBindGroupLayout(0),
        entries: [
          {
            binding: 0,
            resource: sampler,
          },
          {
            binding: 1,
            resource: srcTexture.createView(),
          },
        ],
      });

      const renderPassDescriptor: GPURenderPassDescriptor = {
        colorAttachments: [
          {
            view: ctx.getCurrentTexture().createView(),

            loadValue: { r: 0.5, g: 0.5, b: 0.5, a: 1.0 },
            storeOp: 'store',
          },
        ],
      };

      const commandEncoder = t.device.createCommandEncoder();
      const passEncoder = commandEncoder.beginRenderPass(renderPassDescriptor);
      passEncoder.setPipeline(pipeline);
      passEncoder.setBindGroup(0, uniformBindGroup);
      passEncoder.draw(6, 1, 0, 0);
      passEncoder.endPass();
      t.device.queue.submit([commandEncoder.finish()]);
    }

    function DrawVertexColor(ctx: GPUCanvasContext) {
      const pipeline = t.device.createRenderPipeline({
        vertex: {
          module: t.device.createShaderModule({
            code: `
struct VertexOutput {
  @builtin(position) Position : vec4<f32>;
  @location(0) fragColor : vec4<f32>;
};

@stage(vertex)
fn main(@builtin(vertex_index) VertexIndex : u32) -> VertexOutput {
  var pos = array<vec2<f32>, 6>(
      vec2<f32>( 0.5,  0.5),
      vec2<f32>( 0.5, -0.5),
      vec2<f32>(-0.5, -0.5),
      vec2<f32>( 0.5,  0.5),
      vec2<f32>(-0.5, -0.5),
      vec2<f32>(-0.5,  0.5));

  var offset = array<vec2<f32>, 4>(
    vec2<f32>( -0.5,  0.5),
    vec2<f32>( 0.5, 0.5),
    vec2<f32>(-0.5, -0.5),
    vec2<f32>( 0.5,  -0.5));

  var color = array<vec4<f32>, 4>(
      vec4<f32>(${shaderValueStr}, 0.0, 0.0, 1.0),
      vec4<f32>(0.0, ${shaderValueStr}, 0.0, 1.0),
      vec4<f32>(0.0, 0.0, ${shaderValueStr}, 1.0),
      vec4<f32>(${shaderValueStr}, ${shaderValueStr}, 0.0, 1.0));

  var output : VertexOutput;
  output.Position = vec4<f32>(pos[VertexIndex % 6u] + offset[VertexIndex / 6u], 0.0, 1.0);
  output.fragColor = color[VertexIndex / 6u];
  return output;
}
            `,
          }),
          entryPoint: 'main',
        },
        fragment: {
          module: t.device.createShaderModule({
            code: `
@stage(fragment)
fn main(@location(0) fragColor: vec4<f32>) -> @location(0) vec4<f32> {
  return fragColor;
}
            `,
          }),
          entryPoint: 'main',
          targets: [{ format }],
        },
        primitive: {
          topology: 'triangle-list',
        },
      });

      const renderPassDescriptor: GPURenderPassDescriptor = {
        colorAttachments: [
          {
            view: ctx.getCurrentTexture().createView(),

            loadValue: { r: 0.5, g: 0.5, b: 0.5, a: 1.0 },
            storeOp: 'store',
          },
        ],
      };

      const commandEncoder = t.device.createCommandEncoder();
      const passEncoder = commandEncoder.beginRenderPass(renderPassDescriptor);
      passEncoder.setPipeline(pipeline);
      passEncoder.draw(24, 1, 0, 0);
      passEncoder.endPass();
      t.device.queue.submit([commandEncoder.finish()]);
    }

    function DrawFragcoord(ctx: GPUCanvasContext) {
      const pipeline = t.device.createRenderPipeline({
        vertex: {
          module: t.device.createShaderModule({
            code: `
struct VertexOutput {
  @builtin(position) Position : vec4<f32>;
};

@stage(vertex)
fn main(@builtin(vertex_index) VertexIndex : u32) -> VertexOutput {
  var pos = array<vec2<f32>, 6>(
      vec2<f32>( 1.0,  1.0),
      vec2<f32>( 1.0, -1.0),
      vec2<f32>(-1.0, -1.0),
      vec2<f32>( 1.0,  1.0),
      vec2<f32>(-1.0, -1.0),
      vec2<f32>(-1.0,  1.0));

  var output : VertexOutput;
  output.Position = vec4<f32>(pos[VertexIndex], 0.0, 1.0);
  return output;
}
            `,
          }),
          entryPoint: 'main',
        },
        fragment: {
          module: t.device.createShaderModule({
            code: `
@group(0) @binding(0) var mySampler: sampler;
@group(0) @binding(1) var myTexture: texture_2d<f32>;

@stage(fragment)
fn main(@builtin(position) fragcoord: vec4<f32>) -> @location(0) vec4<f32> {
  var coord = vec2<u32>(floor(fragcoord.xy));
  var color = vec4<f32>(0.0, 0.0, 0.0, 1.0);
  if (coord.x == 0u) {
    if (coord.y == 0u) {
      color.r = ${shaderValueStr};
    } else {
      color.b = ${shaderValueStr};
    }
  } else {
    if (coord.y == 0u) {
      color.g = ${shaderValueStr};
    } else {
      color.r = ${shaderValueStr};
      color.g = ${shaderValueStr};
    }
  }
  return color;
}
            `,
          }),
          entryPoint: 'main',
          targets: [{ format }],
        },
        primitive: {
          topology: 'triangle-list',
        },
      });

      const renderPassDescriptor: GPURenderPassDescriptor = {
        colorAttachments: [
          {
            view: ctx.getCurrentTexture().createView(),

            loadValue: { r: 0.5, g: 0.5, b: 0.5, a: 1.0 },
            storeOp: 'store',
          },
        ],
      };

      const commandEncoder = t.device.createCommandEncoder();
      const passEncoder = commandEncoder.beginRenderPass(renderPassDescriptor);
      passEncoder.setPipeline(pipeline);
      passEncoder.draw(6, 1, 0, 0);
      passEncoder.endPass();
      t.device.queue.submit([commandEncoder.finish()]);
    }

    for (const { cvs, writeCanvasMethod } of targets) {
      const ctx = cvs.getContext('webgpu');
      assert(ctx !== null, 'Failed to get WebGPU context from canvas');

      ctx.configure({
        device: t.device,
        format,
        usage: GPUTextureUsage.COPY_DST | GPUTextureUsage.RENDER_ATTACHMENT,
      });

      switch (writeCanvasMethod) {
        case 'copyBufferToTexture':
          copyBufferToTexture(ctx);
          break;
        case 'copyExternalImageToTexture':
          await copyExternalImageToTexture(ctx);
          break;
        case 'copyTextureToTexture':
          await copyTextureToTexture(ctx);
          break;
        case 'DrawTextureSample':
          await DrawTextureSample(ctx);
          break;
        case 'DrawVertexColor':
          DrawVertexColor(ctx);
          break;
        case 'DrawFragcoord':
          DrawFragcoord(ctx);
          break;
        default:
          unreachable();
      }
    }
  });
}
