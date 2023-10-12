/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {css, CSSResultGroup, html, LitElement} from 'lit';
import {customElement} from 'lit/decorators';

/**
 * Example checked icon.
 * TODO(b/231672472): Delete once icon system is in place.
 */
export const CHECKED_ICON = html`
  <svg width="20" height="20" viewBox="0 0 20 20" xmlns="http://www.w3.org/2000/svg">
    <path d="M13.7071 7.29289C13.3166 6.90237 12.6834 6.90237 12.2929 7.29289L9 10.5858L7.70711 9.29289C7.31658 8.90237 6.68342 8.90237 6.29289 9.29289C5.90237 9.68342 5.90237 10.3166 6.29289 10.7071L8.29289 12.7071C8.68342 13.0976 9.31658 13.0976 9.70711 12.7071L13.7071 8.70711C14.0976 8.31658 14.0976 7.68342 13.7071 7.29289Z"/>
    <path fill-rule="evenodd" clip-rule="evenodd" d="M10 18C14.4183 18 18 14.4183 18 10C18 5.58172 14.4183 2 10 2C5.58172 2 2 5.58172 2 10C2 14.4183 5.58172 18 10 18ZM10 16C13.3137 16 16 13.3137 16 10C16 6.68629 13.3137 4 10 4C6.68629 4 4 6.68629 4 10C4 13.3137 6.68629 16 10 16Z"/>
  </svg>
`;

/** A ChromeOS compliant icon. */
@customElement('cros-icon')
export class Icon extends LitElement {
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
      :host {
        --icon-color: currentcolor;
        display: inline-block;
        height: 20px;
        width: 20px;
      }

      :host([dir="rtl"][rtlflip]) {
        transform: scaleX(-1);
      }

      ::slotted(*) {
        fill: var(--icon-color);
      }

      svg {
        fill: var(--icon-color);
      }

    `;

  override render() {
    return html`<slot name="icon">${CHECKED_ICON}</slot>`;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-icon': Icon;
  }
}
