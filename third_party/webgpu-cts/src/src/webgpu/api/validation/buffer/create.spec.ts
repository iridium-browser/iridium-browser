export const description = `
Tests for validation in createBuffer.
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { assert } from '../../../../common/util/util.js';
import { kBufferSizeAlignment } from '../../../capability_info.js';
import { GPUConst } from '../../../constants.js';
import { kMaxSafeMultipleOf8 } from '../../../util/math.js';
import { ValidationTest } from '../validation_test.js';

export const g = makeTestGroup(ValidationTest);

assert(kBufferSizeAlignment === 4);
g.test('size')
  .desc('Test buffer size alignment.')
  .params(u =>
    u
      .combine('mappedAtCreation', [false, true])
      .beginSubcases()
      .combine('size', [
        0,
        kBufferSizeAlignment * 0.5,
        kBufferSizeAlignment,
        kBufferSizeAlignment * 1.5,
        kBufferSizeAlignment * 2,
      ])
  )
  .unimplemented();

g.test('usage')
  .desc('Test combinations of (one to two?) usage flags.')
  .params(u =>
    u //
      .beginSubcases()
      .combine('mappedAtCreation', [false, true])
      .combine('usage', [
        // Not implemented
      ])
  )
  .unimplemented();

const BufferUsage = GPUConst.BufferUsage;
g.test('createBuffer_invalid_and_oom')
  .desc(
    `When creating a mappable buffer, it's expected that shmem may be immediately allocated
(in the content process, before validation occurs in the GPU process). If the buffer is really
large, though, it could fail shmem allocation before validation fails. Ensure that OOM error is
hidden behind the "more severe" validation error.`
  )
  .paramsSubcasesOnly(u =>
    u.combineWithParams([
      { _valid: true, usage: BufferUsage.UNIFORM, size: 16 },
      { _valid: true, usage: BufferUsage.STORAGE, size: 16 },
      // Invalid because UNIFORM is not allowed with map usages.
      { usage: BufferUsage.MAP_WRITE | BufferUsage.UNIFORM, size: 16 },
      { usage: BufferUsage.MAP_WRITE | BufferUsage.UNIFORM, size: kMaxSafeMultipleOf8 },
      { usage: BufferUsage.MAP_WRITE | BufferUsage.UNIFORM, size: 0x20_0000_0000 }, // 128 GiB
      { usage: BufferUsage.MAP_READ | BufferUsage.UNIFORM, size: 16 },
      { usage: BufferUsage.MAP_READ | BufferUsage.UNIFORM, size: kMaxSafeMultipleOf8 },
      { usage: BufferUsage.MAP_READ | BufferUsage.UNIFORM, size: 0x20_0000_0000 }, // 128 GiB
      // Invalid because size is not aligned to 4 bytes.
      { usage: BufferUsage.STORAGE, size: 15 },
      { usage: BufferUsage.STORAGE, size: kMaxSafeMultipleOf8 - 1 },
      { usage: BufferUsage.STORAGE, size: 0x20_0000_0000 - 1 }, // 128 GiB - 1
    ] as const)
  )
  .fn(t => {
    const { _valid, usage, size } = t.params;

    t.expectGPUError('validation', () => t.device.createBuffer({ size, usage }), !_valid);
  });
