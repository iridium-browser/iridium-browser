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
export function castExists<A>(arg: A, msg: string|null = null): NonNullable<A> {
  if (arg === null || arg === undefined) {
    throw new Error(
        msg ? `Assertion Error: ${msg}` : `The reference is ${arg}`);
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

/**
 * Whether `event.target` should forward a click event to its children and local
 * state. False means a child should already be aware of the click.
 */
export function shouldProcessClick(event: MouseEvent) {
  // Event must start at the event target.
  if (event.currentTarget !== event.target) {
    return false;
  }
  // Event must not be retargeted from shadowRoot.
  if (event.composedPath()[0] !== event.target) {
    return false;
  }
  // Target must not be disabled; this should only occur for a synthetically
  // dispatched click.
  if ((event.target as EventTarget & {disabled: boolean}).disabled) {
    return false;
  }
  return true;
}
