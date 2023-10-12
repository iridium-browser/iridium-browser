/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/ripple/ripple.js';
import '@material/web/focus/md-focus-ring.js';

import {css, CSSResultGroup, html, LitElement} from 'lit';

function renderTick() {
  return html`
      <svg
          class="selected-tick"
          width="18"
          height="18"
          viewBox="0 0 18 18"
          xmlns="http://www.w3.org/2000/svg">
        <path
            fill-rule="evenodd"
            clip-rule="evenodd"
            d="M18 9C18 13.9706 13.9706 18 9 18C4.02944 18 0 13.9706 0 9C0
                4.02944 4.02944 0 9 0C13.9706 0 18 4.02944 18 9ZM13.1705
                5.9545C12.7312 5.51516 12.0188 5.51516 11.5795 5.9545L7.875
                9.65901L6.4205 8.2045C5.98116 7.76517 5.26884 7.76517 4.8295
                8.2045C4.39016 8.64384 4.39016 9.35616 4.8295 9.7955L7.0795
                12.0455C7.51884 12.4848 8.23116 12.4848 8.6705 12.0455L13.1705
                7.5455C13.6098 7.10616 13.6098 6.39384 13.1705 5.9545Z"/>
      </svg>`;
}

/** A chromeOS compliant card. */
export class Card extends LitElement {
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      color: var(--cros-sys-on_surface);
      display: inline-block;
      font: var(--cros-body-0-font);
    }

    #container {
      align-items: center;
      background-color: var(--cros-card-background-color, var(--cros-sys-app_base));
      border: 1px solid var(--cros-card-border-color, var(--cros-sys-separator));
      border-radius: 12px;
      box-sizing: border-box;
      --md-focus-ring-color: var(--cros-sys-focus_ring);
      display: grid;
      outline: none;
      padding: var(--cros-card-padding, 16px);
      position: relative;
      --md-focus-ring-shape: 12px;
    }

    :host([cardStyle="filled"]) #container {
      border: none;
    }

    :host([cardStyle="elevated"]) #container {
      background-color: var(--cros-card-background-color, var(--cros-sys-base_elevated));
      box-shadow: var(--cros-card-elevation-shadow, var(--cros-elevation-1-shadow));
    }

    :host([selected]) #container {
      background-color: var(--cros-card-selected-background-color, var(--cros-sys-primary_container));
    }

    md-ripple {
      color: var(--cros-sys-ripple_primary);
      --md-ripple-hover-color: var(--cros-card-hover-color, var(--cros-sys-hover_on_subtle));
      --md-ripple-hover-opacity: 1;
      --md-ripple-pressed-color: var(--cros-card-pressed-color, var(--cros-sys-ripple_neutral_on_subtle));
      --md-ripple-pressed-opacity: 1;
    }

    :host([disabled]) {
      opacity: var(--cros-disabled-opacity);
    }

    .selected-tick {
      display: none;
    }

    /* TODO(b/288189513): Make icon position and color customisable. */
    :host([selected]) .selected-tick {
      display: unset;
      width: 20px;
      height: 20px;
      fill: var(--cros-sys-on_primary_container);
      position: absolute;
      inset-inline-end: 20px;
      bottom: 20px;
    }
  `;

  /** @nocollapse */
  static override shadowRootOptions = {
    ...LitElement.shadowRootOptions,
    delegatesFocus: true
  };

  /** @export */
  disabled = false;

  /** @export */
  selected = false;

  /** @export */
  cardStyle: 'outline'|'filled'|'elevated' = 'outline';

  /** @nocollapse */
  static override properties = {
    cardStyle: {type: String, reflect: true},
    disabled: {type: Boolean, reflect: true},
    selected: {type: Boolean, reflect: true},
    tabIndex: {type: Number},
  };

  override render() {
    return html`
        <div id="container" tabindex=${this.tabIndex}>
          <md-ripple
              for="container"
              ?disabled=${this.disabled}>
          </md-ripple>
          <md-focus-ring
              for="container"
              ?disabled=${this.disabled}>
          </md-focus-ring>
          ${renderTick()}
          <slot></slot>
        </div>
    `;
  }
}

customElements.define('cros-card', Card);

declare global {
  interface HTMLElementTagNameMap {
    'cros-card': Card;
  }
}
