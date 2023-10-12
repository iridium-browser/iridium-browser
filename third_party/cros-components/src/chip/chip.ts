/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/chips/filter-chip.js';
import '@material/web/chips/input-chip.js';
import '@material/web/chips/suggestion-chip.js';

import {css, CSSResultGroup, html, LitElement} from 'lit';

const ICON_SIZE = css`20px`;
const FOCUS_RING_WIDTH = css`2px`;
/**
 * A cros-compliant chip component.
 * See spec:
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=3167%3A23000&t=ndVwslDMK3OwbNXW-0
 */
export class Chip extends LitElement {
  /** @export */
  disabled: boolean;
  /** @export */
  selected: boolean;
  /** @export */
  label: string;
  /** @export */
  type: 'filter'|'input';
  /**
   * Input chips can have a trailing icon (to close chip) or not.
   * @export
   */
  trailingIcon: boolean;
  /**
   * Input chips can have an avatar as leading icon which makes the icon
   * circular and changes border radius of component.
   * @export
   */
  avatar: boolean;

  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      display: inline-block;
    }

    md-filter-chip {
      --md-filter-chip-disabled-label-text-color: var(--cros-sys-on_surface);
      --md-filter-chip-disabled-label-text-opacity: var(--cros-sys-opacity-disabled);
      --md-filter-chip-disabled-leading-icon-color: var(--cros-sys-disabled);
      --md-filter-chip-disabled-leading-icon-opacity: var(--cros-sys-opacity-disabled);
      --md-filter-chip-disabled-outline-color: var(--cros-sys-separator);
      --md-filter-chip-disabled-outline-opacity: var(--cros-sys-opacity-disabled);
      --md-filter-chip-disabled-selected-container-color: var(--cros-sys-disabled_container);
      --md-filter-chip-disabled-selected-container-opacity: var(--cros-sys-opacity-disabled);
      --md-filter-chip-disabled-trailing-icon-color: var(--cros-sys-disabled);
      --md-filter-chip-disabled-trailing-icon-opacity: var(--cros-sys-opacity-disabled);
      --md-filter-chip-focus-label-text-color: var(--cros-sys-on_surface);
      --md-filter-chip-focus-leading-icon-color: var(--cros-sys-on_surface);
      --md-filter-chip-focus-trailing-icon-color: var(--cros-sys-on_surface);
      --md-filter-chip-hover-label-text-color: var(--cros-sys-on_surface);
      --md-filter-chip-hover-leading-icon-color: var(--cros-sys-on_surface);
      --md-filter-chip-hover-trailing-icon-color: var(--cros-sys-on_surface);
      --md-filter-chip-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-filter-chip-hover-state-layer-opacity: 1;
      --md-filter-chip-icon-size: ${ICON_SIZE};
      --md-filter-chip-label-text-type: var(--cros-button-1-font);
      --md-filter-chip-label-text-color: var(--cros-sys-on_surface);
      --md-filter-chip-leading-icon-color: var(--cros-sys-on_surface);
      --md-filter-chip-outline-color: var(--cros-sys-separator);
      --md-filter-chip-pressed-label-text-color: var(--cros-sys-on_surface);
      --md-filter-chip-pressed-leading-icon-color: var(--cros-sys-on_surface);
      --md-filter-chip-pressed-trailing-icon-color: var(--cros-sys-on_surface);
      --md-filter-chip-pressed-state-layer-color: var(--cros-sys-ripple_neutral_on_subtle);
      --md-filter-chip-pressed-state-layer-opacity: 1;
      --md-filter-chip-selected-container-color: var(--cros-sys-secondary_container);
      --md-filter-chip-selected-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-filter-chip-selected-hover-state-layer-opacity: 1;
      --md-filter-chip-selected-label-text-color: var(--cros-sys-on_secondary_container);
      --md-filter-chip-selected-leading-icon-color: var(--cros-sys-on_secondary_container);
      --md-filter-chip-selected-outline-width: 1px;
      --md-filter-chip-selected-pressed-state-layer-color: var(--cros-sys-ripple_neutral_on_subtle);
      --md-filter-chip-selected-pressed-state-layer-opacity: 1;
      --md-filter-chip-selected-trailing-icon-color: var(--cros-sys-on_secondary_container);
      --md-filter-chip-trailing-icon-color: var(--cros-sys-on_surface);
    }

    md-suggestion-chip {
      --md-suggestion-chip-disabled-label-text-color: var(--cros-sys-on_surface);
      --md-suggestion-chip-disabled-label-text-opacity: var(--cros-sys-opacity-disabled);
      --md-suggestion-chip-disabled-leading-icon-color: var(--cros-sys-disabled);
      --md-suggestion-chip-disabled-leading-icon-opacity: var(--cros-sys-opacity-disabled);
      --md-suggestion-chip-disabled-outline-color: var(--cros-sys-separator);
      --md-suggestion-chip-disabled-outline-opacity: var(--cros-sys-opacity-disabled);
      --md-suggestion-chip-focus-label-text-color: var(--cros-sys-on_surface);
      --md-suggestion-chip-focus-leading-icon-color: var(--cros-sys-on_surface);
      --md-suggestion-chip-hover-label-text-color: var(--cros-sys-on_surface);
      --md-suggestion-chip-hover-leading-icon-color: var(--cros-sys-on_surface);
      --md-suggestion-chip-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-suggestion-chip-hover-state-layer-opacity: 1;
      --md-suggestion-chip-icon-size: ${ICON_SIZE};
      --md-suggestion-chip-label-text-type: var(--cros-button-1-font);
      --md-suggestion-chip-label-text-color: var(--cros-sys-on_surface);
      --md-suggestion-chip-leading-icon-color: var(--cros-sys-on_surface);
      --md-suggestion-chip-outline-color: var(--cros-sys-separator);
      --md-suggestion-chip-pressed-label-text-color: var(--cros-sys-on_surface);
      --md-suggestion-chip-pressed-leading-icon-color: var(--cros-sys-on_surface);
      --md-suggestion-chip-pressed-state-layer-color: var(--cros-sys-ripple_neutral_on_subtle);
      --md-suggestion-chip-pressed-state-layer-opacity: 1;
    }

    md-input-chip {
      --md-input-chip-disabled-label-text-color: var(--cros-sys-on_surface);
      --md-input-chip-disabled-label-text-opacity: var(--cros-sys-opacity-disabled);
      --md-input-chip-disabled-leading-icon-color: var(--cros-sys-disabled);
      --md-input-chip-disabled-leading-icon-opacity: var(--cros-sys-opacity-disabled);
      --md-input-chip-disabled-outline-color: var(--cros-sys-separator);
      --md-input-chip-disabled-outline-opacity: var(--cros-sys-opacity-disabled);
      --md-input-chip-focus-label-text-color: var(--cros-sys-on_surface);
      --md-input-chip-focus-leading-icon-color: var(--cros-sys-on_surface);
      --md-input-chip-hover-label-text-color: var(--cros-sys-on_surface);
      --md-input-chip-hover-leading-icon-color: var(--cros-sys-on_surface);
      --md-input-chip-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-input-chip-hover-state-layer-opacity: 1;
      --md-input-chip-icon-size: ${ICON_SIZE};
      --md-input-chip-label-text-type: var(--cros-button-1-font);
      --md-input-chip-label-text-color: var(--cros-sys-on_surface);
      --md-input-chip-leading-icon-color: var(--cros-sys-on_surface);
      --md-input-chip-outline-color: var(--cros-sys-separator);
      --md-input-chip-pressed-label-text-color: var(--cros-sys-on_surface);
      --md-input-chip-pressed-leading-icon-color: var(--cros-sys-on_surface);
      --md-input-chip-pressed-state-layer-color: var(--cros-sys-ripple_neutral_on_subtle);
      --md-input-chip-pressed-state-layer-opacity: 1;
      }

      :host([avatar]) md-input-chip {
        --md-input-chip-container-shape: 9999px;
      }

      md-filter-chip::part(focus-ring),
      md-filter-chip::part(trailing-focus-ring),
      md-suggestion-chip::part(focus-ring),
      md-input-chip::part(focus-ring),
      md-input-chip::part(trailing-focus-ring) {
        --md-focus-ring-color: var(--cros-sys-focus_ring);
        --md-focus-ring-width: ${FOCUS_RING_WIDTH};
      }
  `;

  /** @nocollapse */
  static override properties = {
    disabled: {type: Boolean},
    selected: {type: Boolean},
    label: {type: String},
    type: {type: String},
    trailingIcon: {type: Boolean},
    avatar: {type: Boolean},
  };

  constructor() {
    super();
    this.disabled = false;
    this.selected = false;
    this.type = 'filter';
    this.trailingIcon = false;
    this.label = '';
    this.avatar = false;
  }

  override render() {
    if (this.type === 'filter') {
      return html`
        <md-filter-chip
            label=${this.label}
            ?disabled=${this.disabled}
            ?selected=${this.selected}>
          <slot slot='icon' name='icon'></slot>
        </md-filter-chip>
        `;
    }
    if (this.trailingIcon || this.avatar) {
      return html`
        <md-input-chip
          label=${this.label}
          ?disabled=${this.disabled}
          ?avatar=${this.avatar}>
            <slot slot='icon' name='icon'></slot>
          </md-input-chip>
        `;
    }
    return html`
      <md-suggestion-chip
        label=${this.label}
        ?disabled=${this.disabled}>
          <slot slot='icon' name='icon'></slot>
      </md-suggestion-chip>
      `;
  }
}

customElements.define('cros-chip', Chip);

declare global {
  interface HTMLElementTagNameMap {
    'cros-chip': Chip;
  }
}
