/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/menu/menu-item';

import {html, LitElement} from 'lit';
import {customElement, property} from 'lit/decorators';

/**
 * A cros compliant menu-item component for use in cros-menu.
 */
@customElement('cros-menu-item')
export class MenuItem extends LitElement {
  // Headline is the primary text of the list item, name follows from
  // md-menu-item.
  @property({type: String}) headline = '';

  override render() {
    return html`
      <md-menu-item headline=${this.headline}></md-menu-item>
    `;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-menu-item': MenuItem;
  }
}
