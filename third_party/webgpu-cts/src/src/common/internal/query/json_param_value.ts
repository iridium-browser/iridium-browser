import { assert, sortObjectByKey } from '../../util/util.js';
import { JSONWithUndefined } from '../params_utils.js';

// JSON can't represent `undefined` and by default stores it as `null`.
// Instead, store `undefined` as this magic string value in JSON.
const jsUndefinedMagicValue = '_undef_';

function stringifyFilter(k: string, v: unknown): unknown {
  // Make sure no one actually uses the magic value as a parameter.
  assert(v !== jsUndefinedMagicValue);

  return v === undefined ? jsUndefinedMagicValue : v;
}

export function stringifyParamValue(value: JSONWithUndefined): string {
  return JSON.stringify(value, stringifyFilter);
}

/**
 * Like stringifyParamValue but sorts dictionaries by key, for hashing.
 */
export function stringifyParamValueUniquely(value: JSONWithUndefined): string {
  return JSON.stringify(value, (k, v) => {
    if (typeof v === 'object' && v !== null) {
      return sortObjectByKey(v);
    }

    return stringifyFilter(k, v);
  });
}

export function parseParamValue(s: string): JSONWithUndefined {
  return JSON.parse(s, (k, v) => (v === jsUndefinedMagicValue ? undefined : v));
}
