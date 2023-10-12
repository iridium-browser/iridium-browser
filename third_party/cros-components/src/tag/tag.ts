/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {css, CSSResultGroup, html, LitElement} from 'lit';

/**
 * A ChromeOS compliant tag.
 * See spec
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?type=design&node-id=9644-165677&mode=design&t=0xBcgechLxIMCLG0-0
 */
export class Tag extends LitElement {
  static HEIGHT = 32;
  static PADDING = 12;
  static RADIUS = 8;

  onBase: boolean;

  // Required '@nocollapse' defends properties from being removed by Closure.
  /** @nocollapse */
  static override properties = {
    // Component attribute, kept in sync, and known as 'on-base' in the DOM.
    onBase: {type: Boolean, reflect: true, attribute: 'on-base'},
  };

  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      align-items: center;
      // Background customized by component CSS '--cros-tag-surface-color'
      background: var(--cros-tag-surface-color, var(--cros-sys-app_base));
      border-radius: ${Tag.RADIUS}px;
      color: var(--cros-sys-on_surface);
      display: flex;
      font: var(--cros-button-2-font);
      height: ${Tag.HEIGHT}px;
      justify-items: center;
      margin: 0;
      padding: 0 ${Tag.PADDING}px;
    }
    :host([on-base]) {
      background: var(--cros-tag-surface-color, var(--cros-sys-hover_on_subtle));
    }
    `;

  constructor() {
    super();
    this.onBase = false;
  }

  override render() {
    return html`
      <div><slot></slot></div>
    `;
  }
}

// Required because Lit @customElement decorators are not approved for use.
customElements.define('cros-tag', Tag);

declare global {
  interface HTMLElementTagNameMap {
    'cros-tag': Tag;
  }
}
