export const description = `
Memory Synchronization Tests for Texture: read before write, read after write, and write after write to the same subresource.

- TODO: Test synchronization between multiple queues.
`;

import { makeTestGroup } from '../../../../../common/framework/test_group.js';
import { GPUTest } from '../../../../gpu_test.js';

import {
  kOperationBoundaries,
  kBoundaryInfo,
  kAllReadOps,
  kAllWriteOps,
  checkOpsValidForContext,
} from './texture_sync_test.js';

export const g = makeTestGroup(GPUTest);

g.test('rw')
  .desc(
    `
    Perform a 'read' operations on a texture subresource, followed by a 'write' operation.
    Operations are separated by a 'boundary' (pass, encoder, queue-op, etc.).
    Test that the results are synchronized.
    The read should not see the contents written by the subsequent write.`
  )
  .params(u =>
    u
      .combine('boundary', kOperationBoundaries)
      .expand('_context', p => kBoundaryInfo[p.boundary].contexts)
      .expandWithParams(function* ({ _context }) {
        for (const read of kAllReadOps) {
          for (const write of kAllWriteOps) {
            if (checkOpsValidForContext([read, write], _context)) {
              yield {
                read: { op: read, in: _context[0] },
                write: { op: write, in: _context[1] },
              };
            }
          }
        }
      })
  )
  .unimplemented();

g.test('wr')
  .desc(
    `
    Perform a 'write' operation on a texture subresource, followed by a 'read' operation.
    Operations are separated by a 'boundary' (pass, encoder, queue-op, etc.).
    Test that the results are synchronized.
    The read should see exactly the contents written by the previous write.`
  )
  .params(u =>
    u
      .combine('boundary', kOperationBoundaries)
      .expand('_context', p => kBoundaryInfo[p.boundary].contexts)
      .expandWithParams(function* ({ _context }) {
        for (const read of kAllReadOps) {
          for (const write of kAllWriteOps) {
            if (checkOpsValidForContext([write, read], _context)) {
              yield {
                write: { op: write, in: _context[0] },
                read: { op: read, in: _context[1] },
              };
            }
          }
        }
      })
  )
  .unimplemented();

g.test('ww')
  .desc(
    `
    Perform a 'first' write operation on a texture subresource, followed by a 'second' write operation.
    Operations are separated by a 'boundary' (pass, encoder, queue-op, etc.).
    Test that the results are synchronized.
    The second write should overwrite the contents of the first.`
  )
  .params(u =>
    u
      .combine('boundary', kOperationBoundaries)
      .expand('_context', p => kBoundaryInfo[p.boundary].contexts)
      .expandWithParams(function* ({ _context }) {
        for (const first of kAllWriteOps) {
          for (const second of kAllWriteOps) {
            if (checkOpsValidForContext([first, second], _context)) {
              yield {
                first: { op: first, in: _context[0] },
                second: { op: second, in: _context[1] },
              };
            }
          }
        }
      })
  )
  .unimplemented();
