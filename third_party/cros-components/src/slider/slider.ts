/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/slider/slider.js';

import {css, CSSResultGroup, html, LitElement} from 'lit';

/**
 * A cros compliant slider component.
 * See spec:
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=2978%3A19626
 */
export class Slider extends LitElement {
  static override styles: CSSResultGroup = css`
    :host {
      display: inline-block;
    }
    md-slider {
      --md-slider-active-track-color: var(--cros-sys-primary);
      --md-slider-handle-color: var(--cros-sys-primary);
      --md-slider-inactive-track-color: var(--cros-sys-primary_container);
      --md-slider-handle-height: 12px;
      --md-slider-handle-width: 12px;
    }
  `;

  /** @nocollapse */
  static override properties = {
    value: {type: Number},
  };

  value: number;

  constructor() {
    super();
    this.value = 0;
  }

  override render() {
    return html`
      <md-slider value=${this.value}></md-slider>
    `;
  }
}

customElements.define('cros-slider', Slider);

declare global {
  interface HTMLElementTagNameMap {
    'cros-slider': Slider;
  }
}
