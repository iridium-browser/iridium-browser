export const description = `
Tests for capabilities added by rg11b10ufloat-renderable flag.
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUConst } from '../../../constants.js';
import { ValidationTest } from '../validation_test.js';

export const g = makeTestGroup(ValidationTest);

g.test('create_texture')
  .desc(
    `
Test that it is valid to create rg11b10ufloat texture with RENDER_ATTACHMENT usage and/or
sampleCount > 1, iff rg11b10ufloat-renderable feature is enabled.
Note, the createTexture tests cover these validation cases where this feature is not enabled.
`
  )
  .params(u =>
    u.combine('usage', [0, GPUConst.TextureUsage.RENDER_ATTACHMENT]).combine('sampleCount', [1, 4])
  )
  .unimplemented();

g.test('begin_render_pass')
  .desc(
    `
Test that it is valid to begin render pass with rg11b10ufloat texture format
iff rg11b10ufloat-renderable feature is enabled.
`
  )
  .unimplemented();

g.test('begin_render_bundle_encoder')
  .desc(
    `
Test that it is valid to begin render bundle encoder with rg11b10ufloat texture
format iff rg11b10ufloat-renderable feature is enabled.
`
  )
  .unimplemented();

g.test('create_render_pipeline')
  .desc(
    `
Test that it is valid to create render pipeline with rg11b10ufloat texture format
in descriptor.fragment.targets iff rg11b10ufloat-renderable feature is enabled.
`
  )
  .unimplemented();
