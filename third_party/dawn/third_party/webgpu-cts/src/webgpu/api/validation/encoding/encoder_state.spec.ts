export const description = `
TODO:
- createCommandEncoder
- non-pass command, or beginPass, during {render, compute} pass
- {before (control case), after} finish()
    - x= {finish(), ... all non-pass commands}
- {before (control case), after} end()
    - x= {render, compute} pass
    - x= {finish(), ... all relevant pass commands}
    - x= {
        - before endPass (control case)
        - after endPass (no pass open)
        - after endPass+beginPass (a new pass of the same type is open)
        - }
    - should make whole encoder invalid
- ?
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { objectEquals } from '../../../../common/util/util.js';
import { ValidationTest } from '../validation_test.js';

class F extends ValidationTest {
  beginRenderPass(commandEncoder: GPUCommandEncoder): GPURenderPassEncoder {
    const attachmentTexture = this.device.createTexture({
      format: 'rgba8unorm',
      size: { width: 16, height: 16, depthOrArrayLayers: 1 },
      usage: GPUTextureUsage.RENDER_ATTACHMENT,
    });
    this.trackForCleanup(attachmentTexture);
    return commandEncoder.beginRenderPass({
      colorAttachments: [
        {
          view: attachmentTexture.createView(),
          clearValue: { r: 1.0, g: 0.0, b: 0.0, a: 1.0 },
          loadOp: 'clear',
          storeOp: 'store',
        },
      ],
    });
  }
}

export const g = makeTestGroup(F);

g.test('pass_end_invalid_order')
  .desc(
    `
  Test that beginning a {compute,render} pass before ending the previous {compute,render} pass
  causes an error.

  TODO: Update this test according to https://github.com/gpuweb/gpuweb/issues/2464
  `
  )
  .params(u =>
    u
      .combine('pass0Type', ['compute', 'render'])
      .combine('pass1Type', ['compute', 'render'])
      .beginSubcases()
      .combine('firstPassEnd', [true, false])
      .combine('endPasses', [[], [0], [1], [0, 1], [1, 0]])
  )
  .fn(async t => {
    const { pass0Type, pass1Type, firstPassEnd, endPasses } = t.params;

    const encoder = t.device.createCommandEncoder();

    const firstPass =
      pass0Type === 'compute' ? encoder.beginComputePass() : t.beginRenderPass(encoder);

    if (firstPassEnd) firstPass.end();

    // Begin a second pass before ending the previous pass.
    const secondPass =
      pass1Type === 'compute' ? encoder.beginComputePass() : t.beginRenderPass(encoder);

    const passes = [firstPass, secondPass];
    for (const index of endPasses) {
      passes[index].end();
    }

    // If {endPasses} is '[1]' and {firstPass} ends, it's a control case.
    const valid = firstPassEnd && objectEquals(endPasses, [1]);

    t.expectValidationError(() => {
      encoder.finish();
    }, !valid);
  });

g.test('call_after_successful_finish')
  .desc(`Test that encoding command after a successful finish generates a validation error.`)
  .paramsSubcasesOnly(u =>
    u.combine('passType', ['compute', 'render']).combine('IsEncoderFinished', [false, true])
  )
  .fn(async t => {
    const { passType, IsEncoderFinished } = t.params;

    const encoder = t.device.createCommandEncoder();

    const pass = passType === 'compute' ? encoder.beginComputePass() : t.beginRenderPass(encoder);
    pass.end();

    const srcBuffer = t.device.createBuffer({
      size: 1024,
      usage: GPUBufferUsage.COPY_SRC,
    });

    const dstBuffer = t.device.createBuffer({
      size: 1024,
      usage: GPUBufferUsage.COPY_DST,
    });

    if (IsEncoderFinished) {
      encoder.finish();
      t.expectValidationError(() => {
        encoder.copyBufferToBuffer(srcBuffer, 0, dstBuffer, 0, 0);
      }, IsEncoderFinished);
    } else {
      t.expectValidationError(() => {
        encoder.copyBufferToBuffer(srcBuffer, 0, dstBuffer, 0, 0);
      }, IsEncoderFinished);
      encoder.finish();
    }
  });

g.test('pass_end_none')
  .desc(
    `
  Test that ending a {compute,render} pass without ending the passes generates a validation error.
  `
  )
  .paramsSubcasesOnly(u => u.combine('passType', ['compute', 'render']).combine('endCount', [0, 1]))
  .fn(async t => {
    const { passType, endCount } = t.params;

    const encoder = t.device.createCommandEncoder();

    const pass = passType === 'compute' ? encoder.beginComputePass() : t.beginRenderPass(encoder);

    for (let i = 0; i < endCount; ++i) {
      pass.end();
    }

    t.expectValidationError(() => {
      encoder.finish();
    }, endCount === 0);
  });

g.test('pass_end_twice')
  .desc('Test that ending a {compute,render} pass twice generates a validation error.')
  .paramsSubcasesOnly(u =>
    u //
      .combine('passType', ['compute', 'render'])
      .combine('endTwice', [false, true])
  )
  .fn(async t => {
    const { passType, endTwice } = t.params;

    const encoder = t.device.createCommandEncoder();

    const pass = passType === 'compute' ? encoder.beginComputePass() : t.beginRenderPass(encoder);

    pass.end();
    if (endTwice) {
      t.expectValidationError(() => {
        pass.end();
      });
    }

    encoder.finish();
  });
