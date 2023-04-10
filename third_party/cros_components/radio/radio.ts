/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {MdRadio} from '@material/web/radio/radio.js';
import {css, CSSResultGroup, html, LitElement} from 'lit';
import {customElement, property, query} from 'lit/decorators';

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
    md-radio {
      --md-sys-color-primary: var(--cros-sys-primary);
      --md-sys-color-on-surface: var(--cros-sys-on_surface);
      --md-ripple-pressed-color: var(--cros-sys-ripple_primary);
      --md-ripple-pressed-opacity: 32%;
      --md-focus-ring-color: var(--cros-sys-primary);
    }
  `;

  @query('md-radio') mdRadio?: MdRadio;

  @property({type: Boolean}) disabled = false;

  @property({type: Boolean})
  get checked() {
    return this.mdRadio?.checked || false;
  }

  private checkedInternal = false;
  set checked(value: boolean) {
    // Store the value for `checked`, so that it is not lost if it is set before
    // the first render.
    this.checkedInternal = value;
    this.requestUpdate();
  }

  override render() {
    return html`
      <md-radio ?disabled=${this.disabled} ?checked=${
        this.checkedInternal}></md-radio>
    `;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-radio': Radio;
  }
}
