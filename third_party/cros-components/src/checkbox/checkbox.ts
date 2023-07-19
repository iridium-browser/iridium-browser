/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/checkbox/checkbox.js';

import {MdCheckbox} from '@material/web/checkbox/checkbox.js';
import {css, CSSResultGroup, html, LitElement} from 'lit';

import {shouldProcessClick} from '../helpers/helpers';

/**
 * A ChromeOS compliant checkbox.
 * See spec
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=2796%3A12821&t=yiSJIVSrbtOP4ZCo-0
 */
export class Checkbox extends LitElement {
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    md-checkbox {
      --md-checkbox-container-size: 16px;
      --md-checkbox-icon-size: 16px;
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
  /** @nocollapse */
  static override properties = {
    checked: {type: Boolean, reflect: true},
    disabled: {type: Boolean, reflect: true},
  };

  /** @nocollapse */
  static events = {
    /** The checkbox value changed via user input. */
    CHANGE: 'change',
  } as const;

  /** @export */
  checked: boolean;
  /** @export */
  disabled: boolean;

  get mdCheckbox(): MdCheckbox {
    return this.renderRoot.querySelector('md-checkbox') as MdCheckbox;
  }

  constructor() {
    super();
    this.disabled = false;
    this.checked = false;

    this.addEventListener('click', (event: MouseEvent) => {
      if (shouldProcessClick(event)) {
        this.click();
      }
    });
  }

  override render() {
    return html`
      <md-checkbox
          ?disabled=${this.disabled}
          ?checked=${this.checked}
          @change=${this.onChange}>
      </md-checkbox>
    `;
  }

  private onChange() {
    this.checked = this.mdCheckbox.checked;
    this.dispatchEvent(new Event('change', {bubbles: true}));
  }

  override click() {
    this.mdCheckbox.click();
  }
}

customElements.define('cros-checkbox', Checkbox);

declare global {
  interface HTMLElementEventMap {
    [Checkbox.events.CHANGE]: Event;
  }

  interface HTMLElementTagNameMap {
    'cros-checkbox': Checkbox;
  }
}
