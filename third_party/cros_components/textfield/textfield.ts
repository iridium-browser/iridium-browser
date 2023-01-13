/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/textfield/filled-text-field';

import {MdFilledTextField} from '@material/web/textfield/filled-text-field';
import {css, CSSResultGroup, html, LitElement} from 'lit';
import {customElement, property, query} from 'lit/decorators';

/**
 * Textfield component. See the specs here:
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=3227%3A25882&t=50tDpMdSJky6eT9O-0
 */
@customElement('cros-textfield')
export class Textfield extends LitElement {
  @query('md-filled-text-field') mdTextfield?: MdFilledTextField;
  static override styles: CSSResultGroup = css`

    /* TODO(b:241483751): Use Jelly unified typography system. */
    :host {
      font-family: 'Roboto', 'Noto', sans-serif;
      font-size: 10px;
    }

    md-filled-text-field {
      --md-filled-text-field-container-shape-start-start: 8px;
      --md-filled-text-field-container-shape-start-end: 8px;
      --md-filled-text-field-container-shape-end-start: 8px;
      --md-filled-text-field-container-shape-end-end: 8px;
      --md-filled-text-field-container-height: 36px;
      --md-filled-text-field-active-indicator-height: 0;
      --md-filled-text-field-disabled-active-indicator-height: 0;
      --md-filled-text-field-hover-active-indicator-height: 0;
      --md-filled-text-field-focus-active-indicator-height: 2px;
      --md-filled-text-field-pressed-active-indicator-height: 2px;
      --md-filled-text-field-active-indicator-color: var(--cros-sys-primary);
      --md-filled-text-field-pressed-active-indicator-color: var(--cros-sys-primary);
      --md-filled-text-field-input-text-prefix-color: var(--cros-sys-secondary);
      --md-filled-text-field-disabled-input-text-color: var(--cros-sys-disabled);
      --md-filled-text-field-hover-state-layer-opacity: 0;
      --md-filled-text-field-error-hover-state-layer-opacity: 0;
      width: 100%;
    }

    #visibleLabel {
      color: var(--cros-sys-on-surface);
      padding-bottom: 8px;
    }
  `;

  /**
   * Properties supported by the MD textfield that are surfaced to the
   * cros-textfield API. These are manually plumbed through as cros-textfield is
   * not a subclass. This is not an exhaustive list, and support should be
   * added by clients as needed.
   */
  @property({attribute: true, type: String}) value = '';
  @property({attribute: true, type: String}) label = '';
  @property({attribute: true, type: String}) suffix = '';
  @property({attribute: true, type: Boolean}) disabled = false;
  @property({attribute: true, type: String}) hint = '';

  /**
   * Max length of the textfield value. If set to -1 or less, md-textfield will
   * ignore this value when restricting the length of the textfield, and also
   * not render the charCounter.
   */
  @property({attribute: true, type: Number}) maxLength = -1;
  override render() {
    return html`
      <div id="visibleLabel">${this.label}</div>
      <md-filled-text-field
        ?disabled=${this.disabled}
        data-aria-label=${this.label}
        value=${this.value}
        suffixtext=${this.suffix}
        maxLength=${this.maxLength}
        supportingText=${this.hint}>
      </md-filled-text-field>
    `;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-textfield': Textfield;
  }
}
