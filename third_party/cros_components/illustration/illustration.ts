/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {css, CSSResultGroup, html, LitElement} from 'lit';
import {customElement} from 'lit/decorators';

/** Lottie renderer with correct hooks to handle dynamic color. */
@customElement('cros-illustration')
export class Illustration extends LitElement {
  static override styles: CSSResultGroup = css``;

  override render() {
    return html`<div></div>`;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-illustration': Illustration;
  }
}
