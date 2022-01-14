export const description = `
Tests for the indirect-specific aspects of drawIndirect/drawIndexedIndirect.
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUTest } from '../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

const filled = new Uint8Array([0, 255, 0, 255]);
const notFilled = new Uint8Array([0, 0, 0, 0]);

const kDrawIndirectParametersSize = 4 * Uint32Array.BYTES_PER_ELEMENT;

g.test('basics,drawIndirect')
  .desc(
    `Test that the indirect draw parameters are tightly packed for drawIndirect.
An indirectBuffer is created based on indirectOffset. The actual draw args being used indicated by the
indirectOffset is going to draw a left bottom triangle.
While the remaining indirectBuffer is populated with random numbers or draw args
that draw right top triangle, both, or nothing which will fail the color check.
The test will check render target to see if only the left bottom area is filled,
meaning the expected draw args is uploaded correctly by the indirectBuffer and indirectOffset.

Params:
    - indirectOffset= {0, 4, k * sizeof(args struct), k * sizeof(args struct) + 4}
    `
  )
  .paramsSubcasesOnly(u =>
    u //
      .combine('indirectOffset', [
        0,
        Uint32Array.BYTES_PER_ELEMENT,
        1 * kDrawIndirectParametersSize,
        1 * kDrawIndirectParametersSize + Uint32Array.BYTES_PER_ELEMENT,
        3 * kDrawIndirectParametersSize,
        3 * kDrawIndirectParametersSize + Uint32Array.BYTES_PER_ELEMENT,
        99 * kDrawIndirectParametersSize,
        99 * kDrawIndirectParametersSize + Uint32Array.BYTES_PER_ELEMENT,
      ] as const)
  )
  .fn(t => {
    const { indirectOffset } = t.params;

    const o = indirectOffset / Uint32Array.BYTES_PER_ELEMENT;
    const arraySize = o + 8;
    const indirectBuffer = [...Array(arraySize)].map(() => Math.floor(Math.random() * 100));

    // draw args that will draw the left bottom triangle (expected call)
    indirectBuffer[o] = 3; // vertexCount
    indirectBuffer[o + 1] = 1; // instanceCount
    indirectBuffer[o + 2] = 0; // firstVertex
    indirectBuffer[o + 3] = 0; // firstInstance

    // draw args that will draw both triangles
    indirectBuffer[o + 4] = 6; // vertexCount
    indirectBuffer[o + 5] = 1; // instanceCount
    indirectBuffer[o + 6] = 0; // firstVertex
    indirectBuffer[o + 7] = 0; // firstInstance

    if (o >= 4) {
      // draw args that will draw the right top triangle
      indirectBuffer[o - 4] = 3; // vertexCount
      indirectBuffer[o - 3] = 1; // instanceCount
      indirectBuffer[o - 2] = 3; // firstVertex
      indirectBuffer[o - 1] = 0; // firstInstance
    }

    if (o >= 8) {
      // draw args that will draw nothing
      indirectBuffer[0] = 0; // vertexCount
      indirectBuffer[1] = 0; // instanceCount
      indirectBuffer[2] = 0; // firstVertex
      indirectBuffer[3] = 0; // firstInstance
    }

    const kRenderTargetFormat = 'rgba8unorm';
    const pipeline = t.device.createRenderPipeline({
      vertex: {
        module: t.device.createShaderModule({
          code: `[[stage(vertex)]] fn main([[location(0)]] pos : vec2<f32>) -> [[builtin(position)]] vec4<f32> {
              return vec4<f32>(pos, 0.0, 1.0);
          }`,
        }),
        entryPoint: 'main',
        buffers: [
          {
            attributes: [
              {
                shaderLocation: 0,
                format: 'float32x2',
                offset: 0,
              },
            ],
            arrayStride: 2 * Float32Array.BYTES_PER_ELEMENT,
          },
        ],
      },
      fragment: {
        module: t.device.createShaderModule({
          code: `[[stage(fragment)]] fn main() -> [[location(0)]] vec4<f32> {
            return vec4<f32>(0.0, 1.0, 0.0, 1.0);
        }`,
        }),
        entryPoint: 'main',
        targets: [
          {
            format: kRenderTargetFormat,
          },
        ],
      },
    });

    const renderTarget = t.device.createTexture({
      size: [4, 4],
      usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.COPY_SRC,
      format: kRenderTargetFormat,
    });

    const commandEncoder = t.device.createCommandEncoder();
    const renderPass = commandEncoder.beginRenderPass({
      colorAttachments: [
        {
          view: renderTarget.createView(),
          loadValue: [0, 0, 0, 0],
          storeOp: 'store',
        },
      ],
    });
    renderPass.setPipeline(pipeline);
    renderPass.setVertexBuffer(
      0,
      t.makeBufferWithContents(
        /* prettier-ignore */
        new Float32Array([
          // The bottom left triangle
          -1.0, 1.0,
           1.0, -1.0,
          -1.0, -1.0,

          // The top right triangle
          -1.0, 1.0,
           1.0, -1.0,
           1.0, 1.0,
        ]),
        GPUBufferUsage.VERTEX
      ),
      0
    );
    renderPass.drawIndirect(
      t.makeBufferWithContents(new Uint32Array(indirectBuffer), GPUBufferUsage.INDIRECT),
      indirectOffset
    );
    renderPass.endPass();
    t.queue.submit([commandEncoder.finish()]);

    // The bottom left area is filled
    t.expectSinglePixelIn2DTexture(
      renderTarget,
      kRenderTargetFormat,
      { x: 0, y: 1 },
      { exp: filled }
    );
    // The top right area is not filled
    t.expectSinglePixelIn2DTexture(
      renderTarget,
      kRenderTargetFormat,
      { x: 1, y: 0 },
      { exp: notFilled }
    );
  });

g.test('basics,drawIndexedIndirect')
  .desc(
    `Test that the indirect draw parameters are tightly packed for drawIndexedIndirect.
    `
  )
  .unimplemented();
