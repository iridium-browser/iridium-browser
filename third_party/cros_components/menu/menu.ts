/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/menu/menu.js';

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
  `;

  @property({attribute: false}) anchor: HTMLElement|null = null;
  @state() open = false;

  override render() {
    return html`
      <md-menu .anchor=${this.anchor || null} ?open=${this.open}> <slot></slot>
      </md-menu>
    `;
  }

  show() {
    this.open = true;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-menu': Menu;
  }
}
