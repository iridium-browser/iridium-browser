/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/radio/radio.js';

import {MdRadio} from '@material/web/radio/radio.js';
import {css, CSSResultGroup, html, LitElement} from 'lit';

/**
 * A chromeOS compliant radio button.
 * See spec
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=2673%3A11119
 */
export class Radio extends LitElement {
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      display: inline-block;
    }
    md-radio {
      --md-radio-icon-size: 16px;
      --md-radio-icon-color: var(--cros-sys-on_surface);
      --md-radio-pressed-state-layer-color: var(--cros-sys-ripple_primary);
      --md-radio-pressed-state-layer-opacity: 32%;
      --md-radio-selected-pressed-state-layer-color: var(--cros-sys-ripple_primary);
      --md-radio-selected-pressed-state-layer-opacity: 32%;
      --md-sys-color-on-surface: var(--cros-sys-on_surface);
      --md-sys-color-primary: var(--cros-sys-primary);
    }
    md-radio::part(focus-ring) {
      --md-focus-ring-color: var(--cros-sys-primary);
      --md-focus-ring-width: 2px;
    }
  `;
  /** @nocollapse */
  static override properties = {
    checked: {type: Boolean},
    disabled: {type: Boolean},
  };

  private checkedInternal = false;

  /** @export */
  disabled: boolean;

  get mdRadio(): MdRadio|undefined {
    return this.renderRoot?.querySelector('md-radio') || undefined;
  }

  /** @export */
  get checked() {
    return this.mdRadio?.checked || false;
  }
  set checked(value: boolean) {
    // Store the value for `checked`, so that it is not lost if it is set before
    // the first render.
    this.checkedInternal = value;
    this.requestUpdate();
  }

  constructor() {
    super();
    this.disabled = false;
  }

  override render() {
    return html`
      <md-radio ?disabled=${this.disabled} ?checked=${
        this.checkedInternal} touch-target="wrapper"></md-radio>
    `;
  }
}

customElements.define('cros-radio', Radio);

declare global {
  interface HTMLElementTagNameMap {
    'cros-radio': Radio;
  }
}
