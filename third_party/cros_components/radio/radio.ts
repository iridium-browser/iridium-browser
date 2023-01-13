/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/radio/radio.js';

import {css, CSSResultGroup, html, LitElement} from 'lit';
import {customElement} from 'lit/decorators';

/**
 * A chromeOS compliant radio button.
 * See spec
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=2673%3A11119
 */
@customElement('cros-radio')
export class Radio extends LitElement {
  static override styles: CSSResultGroup = css`
    :host {
      display: inline-block;
    }
  `;

  override render() {
    return html`
      <md-radio></md-radio>
    `;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-radio': Radio;
  }
}
