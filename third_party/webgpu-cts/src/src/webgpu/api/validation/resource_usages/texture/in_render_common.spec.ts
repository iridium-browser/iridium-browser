export const description = `
TODO:
- 2 views:
    - x= {upon the same subresource, or different subresources {mip level, array layer, aspect} of
         the same texture}
    - x= possible resource usages on each view:
         - both as render attachments
         - both in bind group {texture_binding, storage_binding}
         - one in bind group, and another as render attachment
    - x= different shader stages: {0, ..., 7}
        - maybe first view vis = {1, 2, 4}, second view vis = {0, ..., 7}
    - x= bindings are in {
        - same draw call
        - same pass, different draw call
        - different pass
        - }
(It's probably not necessary to test EVERY possible combination of options in this whole
block, so we could break it down into a few smaller ones (one for different types of
subresources, one for same draw/same pass/different pass, one for visibilities).)
`;

import { makeTestGroup } from '../../../../../common/framework/test_group.js';
import { ValidationTest } from '../../validation_test.js';

class F extends ValidationTest {
  getColorAttachment(
    texture: GPUTexture,
    textureViewDescriptor?: GPUTextureViewDescriptor
  ): GPURenderPassColorAttachment {
    const view = texture.createView(textureViewDescriptor);

    return {
      view,
      clearValue: { r: 1.0, g: 0.0, b: 0.0, a: 1.0 },
      loadOp: 'clear',
      storeOp: 'store',
    };
  }
}

export const g = makeTestGroup(F);

// Test that the different subresource of the same texture are allowed to be used as color
// attachments in same / different render passes, while the same subresource is only allowed to be
// used as different color attachments in different render passes.
g.test('subresources_from_same_texture_as_color_attachments')
  .params(u =>
    u
      .combine('baseLayer0', [0, 1])
      .combine('baseLevel0', [0, 1])
      .combine('baseLayer1', [0, 1])
      .combine('baseLevel1', [0, 1])
      .combine('inSamePass', [true, false])
      .unless(t => t.inSamePass && t.baseLevel0 !== t.baseLevel1)
  )
  .fn(async t => {
    const { baseLayer0, baseLevel0, baseLayer1, baseLevel1, inSamePass } = t.params;

    const texture = t.device.createTexture({
      format: 'rgba8unorm',
      usage: GPUTextureUsage.RENDER_ATTACHMENT,
      size: [16, 16, 2],
      mipLevelCount: 2,
    });

    const colorAttachment1 = t.getColorAttachment(texture, {
      baseArrayLayer: baseLayer0,
      baseMipLevel: baseLevel0,
      mipLevelCount: 1,
    });
    const colorAttachment2 = t.getColorAttachment(texture, {
      baseArrayLayer: baseLayer1,
      baseMipLevel: baseLevel1,
      mipLevelCount: 1,
    });
    const encoder = t.device.createCommandEncoder();
    if (inSamePass) {
      const renderPass = encoder.beginRenderPass({
        colorAttachments: [colorAttachment1, colorAttachment2],
      });
      renderPass.end();
    } else {
      const renderPass1 = encoder.beginRenderPass({
        colorAttachments: [colorAttachment1],
      });
      renderPass1.end();
      const renderPass2 = encoder.beginRenderPass({
        colorAttachments: [colorAttachment2],
      });
      renderPass2.end();
    }

    const success = inSamePass ? baseLayer0 !== baseLayer1 : true;
    t.expectValidationError(() => {
      encoder.finish();
    }, !success);
  });
