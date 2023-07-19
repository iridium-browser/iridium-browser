/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/** @fileoverview Helper methods for asynchronous code management. */

/**
 * Overload of waitForEvent() that automatically casts to an Event type
 * defined in HTMLElementEventMap.
 */
export function waitForEvent<T extends keyof HTMLElementEventMap>(
    target: EventTarget, event: T): Promise<HTMLElementEventMap[T]>;

/** Fallback overload of waitForEvent() with no downcasting. */
export function waitForEvent(
    target: EventTarget, event: string): Promise<Event>;

/**
 * Returns a Promise that resolves after the given `event` has been triggered
 * on the `target`.
 */
export async function waitForEvent(target: EventTarget, event: string) {
  return new Promise(
      resolve => void target.addEventListener(
          event, resolve, {once: true, passive: true}));
}
