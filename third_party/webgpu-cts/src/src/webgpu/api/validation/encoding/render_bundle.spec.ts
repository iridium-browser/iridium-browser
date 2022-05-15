export const description = `
TODO:
- test creating a render bundle, and if it's valid, test that executing it is not an error
    - color formats {all possible formats} {zero, one, multiple}
    - depth/stencil format {unset, all possible formats}
- ?
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { ValidationTest } from '../validation_test.js';

export const g = makeTestGroup(ValidationTest);

g.test('render_bundles,device_mismatch')
  .desc(
    `
    Tests executeBundles cannot be called with render bundles created from another device
    Test with two bundles to make sure all bundles can be validated:
    - bundle0 and bundle1 from same device
    - bundle0 and bundle1 from different device
    `
  )
  .paramsSubcasesOnly([
    { bundle0Mismatched: false, bundle1Mismatched: false }, // control case
    { bundle0Mismatched: true, bundle1Mismatched: false },
    { bundle0Mismatched: false, bundle1Mismatched: true },
  ])
  .fn(async t => {
    const { bundle0Mismatched, bundle1Mismatched } = t.params;
    const mismatched = bundle0Mismatched || bundle1Mismatched;

    if (mismatched) {
      await t.selectMismatchedDeviceOrSkipTestCase(undefined);
    }

    const descriptor: GPURenderBundleEncoderDescriptor = {
      colorFormats: ['rgba8unorm'],
    };

    const bundle0Encoder = mismatched
      ? t.mismatchedDevice.createRenderBundleEncoder(descriptor)
      : t.device.createRenderBundleEncoder(descriptor);
    const bundle0 = bundle0Encoder.finish();
    const bundle1Encoder = mismatched
      ? t.mismatchedDevice.createRenderBundleEncoder(descriptor)
      : t.device.createRenderBundleEncoder(descriptor);
    const bundle1 = bundle1Encoder.finish();

    const encoder = t.createEncoder('render pass');
    encoder.encoder.executeBundles([bundle0, bundle1]);

    encoder.validateFinish(!mismatched);
  });
