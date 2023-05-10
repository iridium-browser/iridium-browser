/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {MdSwitch} from '@material/web/switch/switch.js';
import {css, CSSResultGroup, html, LitElement} from 'lit';
import {customElement, property, query} from 'lit/decorators';

/**
 * A chromeOS switch component.
 */
@customElement('cros-switch')
export class Switch extends LitElement {
  static override styles: CSSResultGroup = css`
    :host {
      display: inline-block;
    }
    md-switch {
      --md-switch-unselected-handle-height: 12px;
      --md-switch-unselected-handle-width: 12px;

      --md-switch-selected-handle-height: 12px;
      --md-switch-selected-handle-width: 12px;

      --md-switch-pressed-handle-height: 12px;
      --md-switch-pressed-handle-width: 12px;

      --md-switch-track-height: 16px;
      --md-switch-track-outline-width: 0;
      --md-switch-track-width: 32px;

      --md-switch-state-layer-size: 32px;

      --md-focus-ring-width: 2px;

      /* selected */
      --md-switch-selected-track-color: var(--cros-sys-primary);
      --md-switch-selected-hover-track-color: var(--cros-sys-primary);
      --md-switch-selected-focus-track-color: var(--cros-sys-primary);
      --md-switch-selected-pressed-track-color: var(--cros-sys-primary);

      --md-switch-selected-handle-color: var(--cros-sys-on_primary);
      --md-switch-selected-hover-handle-color: var(--cros-sys-on_primary);
      --md-switch-selected-focus-handle-color: var(--cros-sys-on_primary);
      --md-switch-selected-pressed-handle-color: var(--cros-sys-on_primary);

      /* unselected */
      --md-switch-unselected-track-color: var(--cros-sys-secondary);
      --md-switch-unselected-hover-track-color: var(--cros-sys-secondary);
      --md-switch-unselected-focus-track-color: var(--cros-sys-secondary);
      --md-switch-unselected-pressed-track-color: var(--cros-sys-secondary);

      --md-switch-unselected-handle-color: var(--cros-sys-on_secondary);
      --md-switch-unselected-hover-handle-color: var(--cros-sys-on_secondary);
      --md-switch-unselected-focus-handle-color: var(--cros-sys-on_secondary);
      --md-switch-unselected-pressed-handle-color: var(--cros-sys-on_secondary);

      --md-focus-ring-color: var(--cros-sys-primary);
    }

    /* disabled */
    md-switch[disabled] {
      opacity: 0.38;
      --md-switch-disabled-unselected-handle-color: var(--cros-sys-on_secondary);
      --md-switch-disabled-unselected-track-color: var(--cros-sys-secondary);

      --md-switch-disabled-selected-handle-color: var(--cros-sys-on_primary);
      --md-switch-disabled-selected-track-color: var(--cros-sys-primary);
    }
  `;

  @query('md-switch') mdSwitch?: MdSwitch;

  @property({type: Boolean}) disabled = false;
  @property({type: Boolean}) selected = false;

  override render() {
    return html`
    <md-switch ?disabled=${this.disabled} ?selected=${
        this.selected}></md-switch>
    `;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-switch': Switch;
  }
}
