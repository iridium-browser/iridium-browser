import { assert } from '../../../common/util/util.js';

declare function takeScreenshotDelayed(ms: number): void;

interface GPURefTest {
  readonly device: GPUDevice;
  readonly queue: GPUQueue;
}

export function runRefTest(fn: (t: GPURefTest) => Promise<void>): void {
  void (async () => {
    assert(
      typeof navigator !== 'undefined' && navigator.gpu !== undefined,
      'No WebGPU implementation found'
    );

    const adapter = await navigator.gpu.requestAdapter();
    assert(adapter !== null);
    const device = await adapter.requestDevice();
    assert(device !== null);
    const queue = device.queue;

    await fn({ device, queue });

    takeScreenshotDelayed(50);
  })();
}
