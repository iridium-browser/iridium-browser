/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '../../material/web/iconbutton/filled-icon-button';
import '../icon/icon';
import '@material/web/iconbutton/filled-tonal-icon-button.js';
import '@material/web/iconbutton/standard-icon-button.js';

import {css, CSSResultGroup, html, LitElement} from 'lit';

/**
 * Floating icon buttons have two variants that differ only by container
 * background, designed to improve contrast depending on the color of the
 * surface behind it:
 * 1) on-selectable-prominent (darker background, light icon color)
 * 2) on-selectable-subtle (lighter background, dark icon color)
 */
const ICON_ON_SELECTABLE_PROMINENT = css`var(--cros-sys-on_primary)`;
const ICON_ON_SELECTABLE_SUBTLE = css`var(--cros-sys-on_primary_container)`;

/**
 * A cros compliant icon-button component.
 * See spec:
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=2447%3A5928
 */
export class IconButton extends LitElement {
  /**
   * When false, uses lighter container and darker icon color. When true, uses
   * darker container and lighter icon color.
   * @export
   */
  prominent: boolean;
  /** @export */
  iconButtonStyle: 'filled'|'floating'|'onImage'|'toggle';
  /** @export */
  iconButtonSize: 'small'|'default'|'large';
  /** @export */
  disabled: boolean;
  /** @export */
  selected: boolean;

  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      display: inline-block;
    }
    md-filled-tonal-icon-button {
      --md-filled-tonal-icon-button-container-color: var(--cros-sys-surface_variant);
      --md-filled-tonal-icon-button-container-shape: 12px;
      --md-filled-tonal-icon-button-container-size: 40px;
      --md-filled-tonal-icon-button-disabled-container-color: var(--cros-sys-disabled_container);
      --md-filled-tonal-icon-button-disabled-container-opacity: 1;
      --md-filled-tonal-icon-button-disabled-icon-color: var(--cros-sys-disabled);
      --md-filled-tonal-icon-button-disabled-icon-opacity: 1;
      --md-filled-tonal-icon-button-focus-icon-color: var(--cros-sys-on_surface);
      --md-filled-tonal-icon-button-hover-icon-color: var(--cros-sys-on_surface);
      --md-filled-tonal-icon-button-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-filled-tonal-icon-button-hover-state-layer-opacity: 1;
      --md-filled-tonal-icon-button-icon-color: var(--cros-sys-on_surface);
      --md-filled-tonal-icon-button-icon-size: 20px;
      --md-filled-tonal-icon-button-pressed-icon-color: var(--cros-sys-on_surface);
      --md-filled-tonal-icon-button-pressed-state-layer-color: var(--cros-sys-ripple_neutral_on_subtle);
      --md-filled-tonal-icon-button-pressed-state-layer-opacity: 1;
      --md-filled-tonal-icon-button-selected-focus-icon-color: var(--cros-sys-on_surface);
      --md-filled-tonal-icon-button-selected-pressed-icon-color: var(--cros-sys-on_surface);
      --md-focus-ring-color: var(--cros-sys-focus_ring);
    }
    md-standard-icon-button {
      --md-focus-ring-color: var(--cros-sys-focus_ring);
      --md-icon-button-disabled-icon-color: var(--cros-sys-disabled);
      --md-icon-button-disabled-icon-opacity: 1;
      --md-icon-button-icon-size: 20px;
      --md-icon-button-selected-focus-icon-color: ${ICON_ON_SELECTABLE_SUBTLE};
      --md-icon-button-selected-hover-icon-color: ${ICON_ON_SELECTABLE_SUBTLE};
      --md-icon-button-selected-icon-color: ${ICON_ON_SELECTABLE_SUBTLE};
      --md-icon-button-selected-pressed-icon-color: ${
      ICON_ON_SELECTABLE_SUBTLE};
      --md-icon-button-state-layer-shape: 12px;
      --md-icon-button-state-layer-size: 40px;
      --md-icon-button-unselected-focus-icon-color: ${
      ICON_ON_SELECTABLE_SUBTLE};
      --md-icon-button-unselected-hover-icon-color: ${
      ICON_ON_SELECTABLE_SUBTLE};
      --md-icon-button-unselected-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-icon-button-unselected-hover-state-layer-opacity: 1;
      --md-icon-button-unselected-icon-color: ${ICON_ON_SELECTABLE_SUBTLE};
      --md-icon-button-unselected-pressed-icon-color: ${
      ICON_ON_SELECTABLE_SUBTLE};
      --md-icon-button-unselected-pressed-state-layer-color: var(--cros-sys-ripple_neutral_on_subtle);
      --md-icon-button-unselected-pressed-state-layer-opacity: 1;
    }
    md-standard-icon-button.small {
      --md-icon-button-icon-size: 16px;
      --md-icon-button-state-layer-shape: 100%;
      --md-icon-button-state-layer-size: 32px;
    }
    md-standard-icon-button.large {
      --md-icon-button-icon-size: 24px;
      --md-icon-button-state-layer-shape: 16px;
      height: 64px;
      margin: 0;
      width: 80px;
    }
    :host([prominent]) md-standard-icon-button {
      --md-icon-button-selected-focus-icon-color: ${
      ICON_ON_SELECTABLE_PROMINENT};
      --md-icon-button-selected-hover-icon-color: ${
      ICON_ON_SELECTABLE_PROMINENT};
      --md-icon-button-selected-icon-color: ${ICON_ON_SELECTABLE_PROMINENT};
      --md-icon-button-selected-pressed-icon-color: ${
      ICON_ON_SELECTABLE_PROMINENT};
      --md-icon-button-unselected-focus-icon-color: ${
      ICON_ON_SELECTABLE_PROMINENT};
      --md-icon-button-unselected-hover-icon-color: ${
      ICON_ON_SELECTABLE_PROMINENT};
      --md-icon-button-unselected-icon-color: ${ICON_ON_SELECTABLE_PROMINENT};
      --md-icon-button-unselected-pressed-icon-color: ${
      ICON_ON_SELECTABLE_PROMINENT};
    }
    md-filled-tonal-icon-button.small {
      --md-filled-tonal-icon-button-container-shape: 100%;
      --md-filled-tonal-icon-button-container-size: 32px;
      --md-filled-tonal-icon-button-icon-size: 16px;
    }
    md-filled-tonal-icon-button.large {
      --md-filled-tonal-icon-button-container-shape: 16px;
      --md-filled-tonal-icon-button-container-size: 80px;
      --md-filled-tonal-icon-button-icon-size: 24px;
      height: 64px;
      margin: 0;
      width: 80px;
    }
    md-filled-icon-button {
      --md-filled-icon-button-container-shape: 12px;
      --md-filled-icon-button-container-size: 40px;
      --md-filled-icon-button-disabled-container-color: #00000000;
      --md-filled-icon-button-disabled-container-opacity: 1;
      --md-filled-icon-button-disabled-icon-color: var(--cros-sys-disabled);
      --md-filled-icon-button-disabled-icon-opacity: 1;
      --md-filled-icon-button-hover-state-layer-opacity: 1;
      --md-filled-icon-button-icon-size: 20px;
      --md-filled-icon-button-selected-container-color: var(--cros-sys-primary_container);
      --md-filled-icon-button-toggle-selected-focus-icon-color: var(--cros-sys-on_primary_container);
      --md-filled-icon-button-toggle-selected-hover-icon-color: var(--cros-sys-on_primary_container);
      --md-filled-icon-button-toggle-selected-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-filled-icon-button-toggle-selected-icon-color: var(--cros-sys-on_primary_container);
      --md-filled-icon-button-toggle-selected-pressed-icon-color: var(--cros-sys-on_primary_container);
      --md-filled-icon-button-toggle-selected-pressed-state-layer-color: var(--cros-sys-ripple_primary);
      --md-filled-icon-button-toggle-unselected-focus-icon-color: var(--cros-sys-on_surface);
      --md-filled-icon-button-toggle-unselected-hover-icon-color: var(--cros-sys-on_surface);
      --md-filled-icon-button-toggle-unselected-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-filled-icon-button-toggle-unselected-icon-color: var(--cros-sys-on_surface);
      --md-filled-icon-button-toggle-unselected-pressed-icon-color: var(--cros-sys-on_surface);
      --md-filled-icon-button-toggle-unselected-pressed-state-layer-color: var(--cros-sys-ripple_primary);
      --md-filled-icon-button-unselected-container-color: #00000000;
      --md-focus-ring-color: var(--cros-sys-focus_ring);
    }
    md-filled-icon-button.large {
      --md-filled-icon-button-container-shape: 16px;
      --md-filled-icon-button-container-size: 80px;
      --md-filled-icon-button-icon-size: 24px;
      height: 64px;
      margin: 0;
      width: 80px;
    }
    md-filled-icon-button[selected] {
      --md-filled-icon-button-disabled-container-color: var(--cros-sys-disabled_container);
      --md-filled-icon-button-disabled-icon-color: var(--cros-sys-disabled);
    }
  `;

  /** @nocollapse */
  static override properties = {
    prominent: {type: Boolean, reflect: true},
    iconButtonStyle: {type: String},
    iconButtonSize: {type: String},
    disabled: {type: Boolean},
    selected: {type: Boolean},
  };

  constructor() {
    super();
    this.prominent = false;
    this.iconButtonStyle = 'filled';
    this.iconButtonSize = 'default';
    this.disabled = false;
    this.selected = false;
  }


  override render() {
    if (this.iconButtonStyle === 'toggle') {
      return html`<md-filled-icon-button toggle ?selected=${
          this.selected} class=${this.iconButtonSize} ?disabled=${
          this.disabled}>
        <slot name="icon"></slot>
        <slot name="selectedIcon" slot="selectedIcon"></slot>
      </md-filled-icon-button>`;
    } else if (this.iconButtonStyle === 'floating') {
      return html`<md-standard-icon-button class=${
          this.iconButtonSize} ?disabled=${this.disabled}>
        <slot name="icon"></slot>
      </md-standard-icon-button>`;
    } else {
      // Both filled & onImage use the md-filled-tonal-icon-button.
      return html`<md-filled-tonal-icon-button class=${
          this.iconButtonSize} ?disabled=${this.disabled}>
        <slot name="icon"></slot>
      </md-filled-tonal-icon-button>`;
    }
  }
}

customElements.define('cros-icon-button', IconButton);

declare global {
  interface HTMLElementTagNameMap {
    'cros-icon-button': IconButton;
  }
}
