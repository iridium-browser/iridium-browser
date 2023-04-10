/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {MdCheckbox} from '@material/web/checkbox/checkbox.js';
import {css, CSSResultGroup, html, LitElement} from 'lit';
import {customElement, property, query} from 'lit/decorators';

/**
 * A ChromeOS compliant checkbox.
 * See spec
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=2796%3A12821&t=yiSJIVSrbtOP4ZCo-0
 */
@customElement('cros-checkbox')
export class Checkbox extends LitElement {
  static override styles: CSSResultGroup = css`
    md-checkbox {
      --md-checkbox-container-size: 20px;
      --md-checkbox-icon-size: 20px;
      --md-checkbox-state-layer-size: 40px;

      --md-sys-color-primary: var(--cros-sys-primary);
      --md-sys-color-on-surface: var(--cros-sys-on_surface);
      --md-sys-color-secondary: var(--cros-sys-focus_ring);
      --md-checkbox-selected-icon-opacity: 0%;
      --md-checkbox-unselected-focused-outline-color: var(--cros-sys-on_surface);
      --md-checkbox-unselected-outline-color: var(--cros-sys-on_surface);
      --md-checkbox-unselected-hover-outline-color: var(--cros-sys-on_surface);
      --md-checkbox-unselected-pressed-outline-color: var(--cros-sys-on_surface);
      --md-checkbox-unselected-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-checkbox-unselected-hover-state-layer-opacity: 100%;
      --md-checkbox-selected-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-checkbox-selected-hover-state-layer-opacity: 100%;
      --md-checkbox-unselected-pressed-state-layer-color: var(--cros-sys-ripple_primary);
      --md-checkbox-unselected-pressed-state-layer-opacity: 100%;
      --md-checkbox-selected-pressed-state-layer-color: var(--cros-sys-ripple_primary);
      --md-checkbox-selected-pressed-state-layer-opacity: 100%;
      --md_checkbox-unselected-disabled-container-opacity: 38%;
      --md-checkbox-selected-disabled-container-opacity: 38%;
    }
  `;

  @query('md-checkbox') mdCheckbox?: MdCheckbox;

  checkedInternal: boolean = false;
  @property({type: Boolean, reflect: true})
  get checked() {
    return this.mdCheckbox?.checked || false;
  }
  set checked(value: boolean) {
    // Store the value for `checked`, so that it is not lost if it is set before
    // the first render.
    this.checkedInternal = value;
    this.requestUpdate();
  }

  @property({type: Boolean}) disabled = false;

  override render() {
    return html`
      <md-checkbox ?disabled=${this.disabled} ?checked=${this.checkedInternal}>
      </md-checkbox>
    `;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-checkbox': Checkbox;
  }
}
