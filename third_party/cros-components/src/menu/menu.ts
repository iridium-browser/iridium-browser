/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {Corner} from '@material/web/menu/menu.js';
import {css, CSSResultGroup, html, LitElement} from 'lit';
import {customElement, property, state} from 'lit/decorators';

/**
 * A chromeOS menu component.
 * See spec
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=2650%3A7994&t=01NOG3FTGuaigSvB-0
 */
@customElement('cros-menu')
export class Menu extends LitElement {
  static override styles: CSSResultGroup = css`
    :host {
      display: inline-block;
    }
    md-menu {
      --md-list-container-color: var(--cros-sys-on_primary);
      --md-menu-container-surface-tint-layer-color: white;
      --md-menu-container-shape: 8px;
      --md-list-list-item-one-line-container-height: 36px;
      --md-list-list-item-label-text-type: var(--cros-typography-body2);
    }
  `;

  static override shadowRootOptions = {
    ...LitElement.shadowRootOptions,
    delegatesFocus: true
  };

  @property({attribute: false}) anchor: HTMLElement|null = null;
  @property({attribute: 'anchor-corner'}) anchorCorner: Corner = 'END_START';
  @state() open = false;
  @property({type: Boolean, attribute: 'has-overflow'}) hasOverflow = false;

  override render() {
    return html`
      <md-menu .anchor=${this.anchor} ?open=${this.open} @closed=${
        this.close} ?has-overflow=${this.hasOverflow} anchor-corner=${
        this.anchorCorner} delegatesFocus> <slot></slot>
      </md-menu>
    `;
  }

  show() {
    this.open = true;
  }

  close() {
    this.open = false;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-menu': Menu;
  }
}
