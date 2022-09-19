export const description = `Tests writeTexture validation.`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUConst } from '../../../constants.js';
import { ValidationTest } from '../validation_test.js';

export const g = makeTestGroup(ValidationTest);

g.test('usages')
  .desc(
    `
  Tests calling writeTexture with the texture missed COPY_DST usage.
    - texture {with, without} COPY DST usage
  `
  )
  .paramsSubcasesOnly([
    { usage: GPUConst.TextureUsage.COPY_DST }, // control case
    { usage: GPUConst.TextureUsage.STORAGE_BINDING },
    { usage: GPUConst.TextureUsage.STORAGE_BINDING | GPUConst.TextureUsage.COPY_SRC },
    { usage: GPUConst.TextureUsage.STORAGE_BINDING | GPUConst.TextureUsage.COPY_DST },
  ])
  .fn(async t => {
    const { usage } = t.params;
    const texture = t.device.createTexture({
      size: { width: 16, height: 16 },
      usage,
      format: 'rgba8unorm' as const,
    });
    const data = new Uint8Array(16);
    const size = [1, 1];

    const isValid = usage & GPUConst.TextureUsage.COPY_DST ? true : false;
    t.expectValidationError(() => {
      t.device.queue.writeTexture({ texture }, data, {}, size);
    }, !isValid);
  });
