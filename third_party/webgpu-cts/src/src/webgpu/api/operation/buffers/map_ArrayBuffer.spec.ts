export const description = `
Tests for the behavior of ArrayBuffers returned by getMappedRange.

TODO: Update these tests if we make this not an error, but instead "fake" the transfer:
  https://github.com/gpuweb/gpuweb/issues/747#issuecomment-623712878
TODO: Add tests that transfer to another thread instead of just using MessageChannel.
TODO: Add tests for any other Web APIs that can detach ArrayBuffers.
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { resolveOnTimeout } from '../../../../common/util/util.js';
import { GPUTest } from '../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

g.test('postMessage')
  .desc(
    `Using postMessage to send a getMappedRange-returned ArrayBuffer should throw an exception iff
    the ArrayBuffer is in the transfer list (x= map read, map write).

  TODO: Determine what the exception.name is expected to be. [1]`
  )
  .params(u =>
    u //
      .combine('transfer', [false, true])
      .combine('mapWrite', [false, true])
  )
  .fn(async t => {
    const { transfer, mapWrite } = t.params;
    const kSize = 1024;

    const buf = t.device.createBuffer({
      size: kSize,
      usage: mapWrite ? GPUBufferUsage.MAP_WRITE : GPUBufferUsage.MAP_READ,
    });
    await buf.mapAsync(mapWrite ? GPUMapMode.WRITE : GPUMapMode.READ);
    const ab1 = buf.getMappedRange();
    t.expect(ab1.byteLength === kSize, 'ab1 should have size of buffer');

    const mc = new MessageChannel();
    const ab2Promise = new Promise<ArrayBuffer>(resolve => {
      mc.port2.onmessage = ev => resolve(ev.data);
    });
    // [1]: Pass an exception name here instead of `true`, once we figure out what the exception
    // is supposed to be.
    t.shouldThrow(
      transfer ? true : false,
      () => {
        mc.port1.postMessage(ab1, transfer ? [ab1] : undefined);
      },
      'in postMessage'
    );
    t.expect(ab1.byteLength === kSize, 'after postMessage, ab1 should not be detached');

    buf.unmap();
    t.expect(ab1.byteLength === 0, 'after unmap, ab1 should be detached');

    const ab2 = await Promise.race([
      // If `transfer` is true, this resolveOnTimeout is _supposed_ to win the race, because the
      // postMessage should have errored and no message should have been received.
      resolveOnTimeout(100),
      // Either way, if postMessage succeeded, then we'll receive an ArrayBuffer on port2, and this
      // will win the race.
      ab2Promise,
    ]);

    if (ab2) {
      t.expect(!transfer, 'postMessage should have failed if it transferred ab1');
      // If `transfer` is false, this is a deep copy, and it shouldn't be affected by the unmap.
      t.expect(ab2.byteLength === kSize, 'ab2 should be the same size');
    }
  });
