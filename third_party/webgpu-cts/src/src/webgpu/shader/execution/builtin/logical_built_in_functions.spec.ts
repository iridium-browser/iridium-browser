export const description = `WGSL Logical built-in functions execution test`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUTest } from '../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);

g.test('builtin,all')
  .uniqueId(0x16815cc7b249ff78)
  .desc(
    `
vector all:
e: vecN<bool> all(e): bool Returns true if each component of e is true.
(OpAll)
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();

g.test('builtin,any')
  .uniqueId(0x55e14f481a7f3e66)
  .desc(
    `
vector any:
e: vecN<bool> any(e): bool Returns true if any component of e is true.
(OpAny)
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();

g.test('builtin,select_scalar')
  .uniqueId(0xb9bd4dd5c8b4b0d7)
  .desc(
    `
scalar select:
T is a scalar select(f:T,t:T,cond: bool): T Returns t when cond is true, and f otherwise.
(OpSelect)
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();

g.test('builtin,select_vector')
  .uniqueId(0xbf77b7217032259b)
  .desc(
    `
vector select:
T is a scalar select(f: vecN<T>,t: vecN<T,cond: vecN<bool>>) Component-wise selection.
Result component i is evaluated as select(f[i],t[i],cond[i]).
(OpSelect)
`
  )
  .params(u => u.combine('placeHolder1', ['placeHolder2', 'placeHolder3']))
  .unimplemented();
