export const description = `
TODO:
- Test that pipeline overridable constant values given at pipeline creation time are used correctly inside shader.
- Test that float number precision is preserved for overrides.
- Test that an override with different constant values assigned at both vertex and fragment state.
- Test that an override with different constant values assigned at different pipeline creations that share the same shader module.
`;

import { makeTestGroup } from '../../../../common/framework/test_group.js';
import { GPUTest } from '../../../gpu_test.js';

export const g = makeTestGroup(GPUTest);
