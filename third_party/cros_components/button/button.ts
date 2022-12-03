/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {css, CSSResultGroup, html, LitElement} from 'lit';
import {customElement} from 'lit/decorators';

/** A chromeOS compliant button. */
@customElement('cros-button')
export class Button extends LitElement {
  static override styles: CSSResultGroup = css``;

  override render() {
    return html`<button></button>`;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-button': Button;
  }
}
