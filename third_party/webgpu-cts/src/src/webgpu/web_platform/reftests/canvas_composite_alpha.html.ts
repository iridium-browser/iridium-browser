import { assert, unreachable } from '../../../common/util/util.js';

import { runRefTest } from './gpu_ref_test.js';

// <canvas> element from html page
declare const cvs: HTMLCanvasElement;

type WriteCanvasMethod = 'draw' | 'copy';

export function run(
  format: GPUTextureFormat,
  alphaMode: GPUCanvasAlphaMode,
  writeCanvasMethod: WriteCanvasMethod
) {
  runRefTest(async t => {
    const ctx = cvs.getContext('webgpu');
    assert(ctx instanceof GPUCanvasContext, 'Failed to get WebGPU context from canvas');

    switch (format) {
      case 'bgra8unorm':
      case 'bgra8unorm-srgb':
      case 'rgba8unorm':
      case 'rgba8unorm-srgb':
      case 'rgba16float':
        break;
      default:
        unreachable();
    }

    // This is mimic globalAlpha in 2d context blending behavior
    const alphaFromShader = { premultiplied: '0.5', opaque: '1.0' }[alphaMode];

    let usage = 0;
    switch (writeCanvasMethod) {
      case 'draw':
        usage = GPUTextureUsage.RENDER_ATTACHMENT;
        break;
      case 'copy':
        usage = GPUTextureUsage.COPY_DST;
        break;
    }
    ctx.configure({
      device: t.device,
      format,
      usage,
      alphaMode,
    });

    // The blending behavior here is to mimic 2d context blending behavior
    // of drawing rects in order
    // https://drafts.fxtf.org/compositing/#porterduffcompositingoperators_srcover
    const kBlendStateSourceOver = {
      color: {
        srcFactor: 'src-alpha',
        dstFactor: 'one-minus-src-alpha',
        operation: 'add',
      },
      alpha: {
        srcFactor: 'one',
        dstFactor: 'one-minus-src-alpha',
        operation: 'add',
      },
    } as const;

    const pipeline = t.device.createRenderPipeline({
      layout: 'auto',
      vertex: {
        module: t.device.createShaderModule({
          code: `
struct VertexOutput {
@builtin(position) Position : vec4<f32>,
@location(0) fragColor : vec4<f32>,
}

@vertex
fn main(@builtin(vertex_index) VertexIndex : u32) -> VertexOutput {
var pos = array<vec2<f32>, 6>(
    vec2<f32>( 0.75,  0.75),
    vec2<f32>( 0.75, -0.75),
    vec2<f32>(-0.75, -0.75),
    vec2<f32>( 0.75,  0.75),
    vec2<f32>(-0.75, -0.75),
    vec2<f32>(-0.75,  0.75));

var offset = array<vec2<f32>, 4>(
vec2<f32>( -0.25,  0.25),
vec2<f32>( 0.25, 0.25),
vec2<f32>(-0.25, -0.25),
vec2<f32>( 0.25,  -0.25));

var color = array<vec4<f32>, 4>(
    vec4<f32>(0.4, 0.0, 0.0, ${alphaFromShader}),
    vec4<f32>(0.0, 0.4, 0.0, ${alphaFromShader}),
    vec4<f32>(0.0, 0.0, 0.4, ${alphaFromShader}),
    vec4<f32>(0.4, 0.4, 0.0, ${alphaFromShader})); // 0.4 -> 0x66

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
@fragment
fn main(@location(0) fragColor: vec4<f32>) -> @location(0) vec4<f32> {
return fragColor;
}
        `,
        }),
        entryPoint: 'main',
        targets: [
          {
            format,
            blend: { premultiplied: kBlendStateSourceOver, opaque: undefined }[alphaMode],
          },
        ],
      },
      primitive: {
        topology: 'triangle-list',
      },
    });

    let renderTarget: GPUTexture;
    switch (writeCanvasMethod) {
      case 'draw':
        renderTarget = ctx.getCurrentTexture();
        break;
      case 'copy':
        renderTarget = t.device.createTexture({
          size: [ctx.canvas.width, ctx.canvas.height],
          format,
          usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.COPY_SRC,
        });
        break;
    }
    const renderPassDescriptor: GPURenderPassDescriptor = {
      colorAttachments: [
        {
          view: renderTarget.createView(),
          clearValue: { r: 0.0, g: 0.0, b: 0.0, a: 0.0 },
          loadOp: 'clear',
          storeOp: 'store',
        },
      ],
    };

    const commandEncoder = t.device.createCommandEncoder();
    const passEncoder = commandEncoder.beginRenderPass(renderPassDescriptor);
    passEncoder.setPipeline(pipeline);
    passEncoder.draw(6, 1, 0, 0);
    passEncoder.draw(6, 1, 6, 0);
    passEncoder.draw(6, 1, 12, 0);
    passEncoder.draw(6, 1, 18, 0);
    passEncoder.end();

    switch (writeCanvasMethod) {
      case 'draw':
        break;
      case 'copy':
        commandEncoder.copyTextureToTexture(
          {
            texture: renderTarget,
          },
          {
            texture: ctx.getCurrentTexture(),
          },
          [ctx.canvas.width, ctx.canvas.height]
        );
        break;
    }

    t.device.queue.submit([commandEncoder.finish()]);
  });
}
