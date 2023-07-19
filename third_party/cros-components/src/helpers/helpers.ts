/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @fileoverview A set of chromium safe helper functions. Many of these exist
 * in google3 already but we reimplement so they can also be imported in
 * chromium.
 */

/**
 * Asserts that `arg` is not null or undefined and informs typescript of this so
 * that code that runs after this check will know `arg` is not null. If `arg` is
 * null, throws an error with `msg`.
 *
 * NOTE: unlike the internal counterpart this ALWAYS throws an error when given
 * null or undefined. This is not compiled out for prod.
 */
export function assertExists<A>(
    arg: A, msg: string): asserts arg is NonNullable<A> {
  if (!arg) {
    throw new Error(`Assertion Error: ${msg}`);
  }
}

/**
 * Asserts that `arg` is not null and returns it. If `arg` is null, throws an
 * error with `msg`.
 *
 * NOTE: unlike the internal counterpart this ALWAYS throws an error when given
 * null or undefined. This is not compiled out for prod.
 */
export function castExists<A>(arg: A, msg: string): NonNullable<A> {
  if (!arg) {
    throw new Error(`Assertion Error: ${msg}`);
  }
  return arg;
}

/** Converts a given hex string to a [r, g, b] array. */
export function hexToRgb(hexString: string): [number, number, number] {
  const rHex = hexString.substring(1, 3);
  const gHex = hexString.substring(3, 5);
  const bHex = hexString.substring(5, 7);

  return [
    Number(`0x${rHex}`),
    Number(`0x${gHex}`),
    Number(`0x${bHex}`),
  ];
}
