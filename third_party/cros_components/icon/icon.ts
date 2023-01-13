/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {css, CSSResultGroup, html, LitElement} from 'lit';
import {customElement} from 'lit/decorators';

/** A ChromeOS compliant icon. */
@customElement('cros-icon')
export class Icon extends LitElement {
  static override styles: CSSResultGroup = css`
      :host {
        background-color: currentcolor;
      }
    `;

  override render() {
    return html`<span></span>`;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-icon': Icon;
  }
}
