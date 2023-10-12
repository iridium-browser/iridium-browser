/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {css, CSSResultGroup, html, LitElement} from 'lit';

/**
 * A cros compliant separator component for use in cros-menu.
 */
export class MenuSeparator extends LitElement {
  static override styles: CSSResultGroup = css`
    hr {
      background-color: var(--cros-sys-separator);
      height: 1px;
      border: 0;
    }
  `;
  override render() {
    return html`
      <hr role="separator"></hr>
    `;
  }
}

customElements.define('cros-menu-separator', MenuSeparator);

declare global {
  interface HTMLElementTagNameMap {
    'cros-menu-separator': MenuSeparator;
  }
}
