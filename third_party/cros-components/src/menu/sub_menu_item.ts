/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/menu/sub-menu-item';
import '@material/web/menu/menu-item';
import './menu_item';

import {html, LitElement} from 'lit';
import {customElement, property} from 'lit/decorators';

/**
 * ChromeOS menu item which represents an openable submenu.
 * Hovering or clicking element will open a second submenu.
 */
@customElement('cros-sub-menu-item')
export class SubMenuItem extends LitElement {
  // Headline is the primary text of the list item, name follows from
  // md-menu-item.
  @property({type: String}) headline = '';

  override render() {
    return html`
      <md-sub-menu-item headline=${this.headline}>
        <slot name="submenu" slot="submenu"></slot>
      </md-sub-menu-item>
    `;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-sub-menu-item': SubMenuItem;
  }
}
