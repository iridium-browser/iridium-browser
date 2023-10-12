/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/checkbox/checkbox.js';

import {MdCheckbox} from '@material/web/checkbox/checkbox.js';
import {css, CSSResultGroup, html, LitElement} from 'lit';

import {shouldProcessClick} from '../helpers/helpers';

// The spec says icon size is 20px, but that doesn't render to the same thing.
const ICON_SIZE = css`16px`;
const TOUCH_TARGET_SIZE = css`48px`;
const RIPPLE_SIZE = css`40px`;

/**
 * A ChromeOS compliant checkbox.
 * See spec
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=2796%3A12821&t=yiSJIVSrbtOP4ZCo-0
 */
export class Checkbox extends LitElement {
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      display: inline-block;
      height: ${TOUCH_TARGET_SIZE};
      vertical-align: calc((${TOUCH_TARGET_SIZE} - ${ICON_SIZE}) / 2);
      width: ${TOUCH_TARGET_SIZE};

      margin-inline-start: calc(var(--cros-checkbox-reserve-inline-start, ${
      ICON_SIZE}) - ${ICON_SIZE});
      margin-inline-end: calc(var(--cros-checkbox-reserve-inline-end, ${
      ICON_SIZE}) - ${ICON_SIZE});
      margin-top: calc(var(--cros-checkbox-reserve-top, ${ICON_SIZE}) - ${
      ICON_SIZE});
      margin-bottom: calc(var(--cros-checkbox-reserve-bottom, ${ICON_SIZE}) - ${
      ICON_SIZE});
    }

    md-checkbox {
      --md-checkbox-container-size: ${ICON_SIZE};
      --md-checkbox-icon-size: ${ICON_SIZE};
      --md-checkbox-state-layer-size: ${RIPPLE_SIZE};

      --md-checkbox-selected-container-color: var(--cros-sys-primary);
      --md-checkbox-selected-disabled-container-color: var(--cros-sys-primary);
      --md-checkbox-selected-disabled-container-opacity: 1;
      --md-checkbox-selected-disabled-icon-color: var(--cros-sys-on_primary);
      --md-checkbox-selected-focus-container-color: var(--cros-sys-primary);
      --md-checkbox-selected-focus-icon-color: var(--cros-sys-on_primary);
      --md-checkbox-selected-hover-container-color: var(--cros-sys-primary);
      --md-checkbox-selected-hover-icon-color: var(--cros-sys-on_primary);
      --md-checkbox-selected-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-checkbox-selected-hover-state-layer-opacity: 100%;
      --md-checkbox-selected-icon-color: var(--cros-sys-on_primary);
      --md-checkbox-selected-pressed-container-color: var(--cros-sys-primary);
      --md-checkbox-selected-pressed-icon-color: var(--cros-sys-on_primary);
      --md-checkbox-selected-pressed-state-layer-color: var(--cros-sys-ripple_primary);
      --md-checkbox-selected-pressed-state-layer-opacity: 100%;

      --md-checkbox-disabled-container-opacity: 1;
      --md-checkbox-disabled-outline-color: var(--cros-sys-on_surface);
      --md-checkbox-outline-color: var(--cros-sys-on_surface);
      --md-checkbox-focus-outline-color: var(--cros-sys-on_surface);
      --md-checkbox-hover-outline-color: var(--cros-sys-on_surface);
      --md-checkbox-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-checkbox-hover-state-layer-opacity: 100%;
      --md-checkbox-pressed-outline-color: var(--cros-sys-on_surface);
      --md-checkbox-pressed-state-layer-color: var(--cros-sys-ripple_primary);
      --md-checkbox-pressed-state-layer-opacity: 100%;

      --md-sys-color-secondary: var(--cros-sys-focus_ring);
    }

    :host([disabled]) md-checkbox {
      opacity: var(--cros-disabled-opacity);
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
          @change=${this.onChange}
          touch-target="wrapper">
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
