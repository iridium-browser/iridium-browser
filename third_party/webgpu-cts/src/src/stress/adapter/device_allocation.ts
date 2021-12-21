export const description = `
Stress tests for GPUAdapter.requestDevice.
`;

import { Fixture } from '../../common/framework/fixture.js';
import { makeTestGroup } from '../../common/framework/test_group.js';

export const g = makeTestGroup(Fixture);

g.test('coexisting')
  .desc(
    `Tests allocation of many coexisting GPUDevice objects.

TODO: These stress tests might not make sense. Allocating lots of GPUDevices is
currently crashy in Chrome, and there's not a great reason for applications to
do it. UAs should probably limit the number of simultaneously active devices,
but this is effectively blocked on implementing GPUDevice.destroy() to give
applications the necessary controls.`
  )
  .unimplemented();

g.test('continuous,with_destroy')
  .desc(
    `Tests allocation and destruction of many GPUDevice objects over time. Objects
are sequentially requested and destroyed over a very large number of
iterations.`
  )
  .unimplemented();

g.test('continuous,no_destroy')
  .desc(
    `Tests allocation and implicit GC of many GPUDevice objects over time. Objects
are sequentially requested and dropped for GC over a very large number of
iterations.`
  )
  .unimplemented();
