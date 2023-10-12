/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/button/text-button.js';
import '../icon/icon';

import {css, CSSResultGroup, html, LitElement} from 'lit';

const CHEVRON_ICON =
    html`<svg viewBox="0 0 20 20" xmlns="http://www.w3.org/2000/svg" id="icon">
      <path fill-rule="evenodd" clip-rule="evenodd" d="M6.66669 13.825L10.7872 10L6.66669 6.175L7.93524 5L13.3334 10L7.93524 15L6.66669 13.825Z"/>
    </svg>`;

/**
 * A cros compliant breadcrumb-item component for use in cros-breadcrumb.
 */
export class BreadcrumbItem extends LitElement {
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      --breadcrumb-size: 36px;
      --icon-size: 36px;
      --chevron-size: 20px;
      --font-type: var(--cros-title-1-font);
      --padding-between: 2px;
    }
    md-text-button {
      --md-sys-color-primary: var(--cros-sys-on_surface_variant);
      --md-text-button-container-height: var(--breadcrumb-size);
      --md-text-button-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-text-button-hover-state-layer-opacity: 1;
      --md-text-button-label-text-type: var(--font-type);
      --md-text-button-pressed-state-layer-color: var(--cros-sys-ripple_neutral_on_subtle);
      --md-text-button-pressed-state-layer-opacity: 1;
    }
    md-text-button::part(focus-ring) {
      --md-focus-ring-color: var(--cros-sys-focus_ring);
    }
    ::slotted(*) {
      color: var(--cros-sys-on_surface_variant);
      height: var(--icon-size);
      width: var(--icon-size);
    }
    :host {
      align-items: center;
      display: flex;
    }
    #icon {
      padding-left: var(--padding-between);
      padding-right: var(--padding-between);
      width: var(--chevron-size);
      height: var(--chevron-size);
    }
    :host(:last-of-type) #icon {
      display: none;
    }
    :host-context([dir=rtl]) #icon {
      transform: scaleX(-1);
    }
  `;
  /** @nocollapse */
  static override properties = {
    label: {type: String},
    variant: {type: String},
  };

  /**
   * @export
   */
  label: string;
  /**
   * @export
   */
  variant: 'text'|'icon';

  constructor() {
    super();
    this.label = '';
    this.variant = 'text';
  }

  override render() {
    const itemVariant = (this.variant === 'icon') ?
        html`<slot></slot>` :
        html`<md-text-button>${this.label}</md-text-button>`;
    return html`${itemVariant} ${CHEVRON_ICON}`;
  }
}

customElements.define('cros-breadcrumb-item', BreadcrumbItem);

declare global {
  interface HTMLElementTagNameMap {
    'cros-breadcrumb-item': BreadcrumbItem;
  }
}
