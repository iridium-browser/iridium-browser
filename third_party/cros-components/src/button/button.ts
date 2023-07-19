/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/button/filled-button.js';
import '@material/web/button/text-button.js';

import {css, CSSResultGroup, html, LitElement} from 'lit';

const PADDING = css`16px`;
const PADDING_ICON_SIDE = css`12px`;
const CONTAINER_HEIGHT = css`36px`;
const ICON_SIZE = css`20px`;
const MAX_WIDTH = css`200px`;
const MIN_WIDTH = css`64px`;

/**
 * A chromeOS compliant button.
 * See spec
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=2116%3A4082&t=kbaCFk5KdayGTyuL-0
 */
export class Button extends LitElement {
  // TODO(b/258982831): Centre icon. See spec.
  // Note that theme colours have opacity defined in the colour, but default
  // colours have opacities set separately. As a consequence, styles are broken
  // unless a cros theme is present.
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    md-filled-button {
      max-width: ${MAX_WIDTH};
      min-width: ${MIN_WIDTH};
      --md-filled-button-container-height: ${CONTAINER_HEIGHT};
      --md-filled-button-disabled-container-color: var(--cros-sys-disabled_container);
      --md-filled-button-disabled-container-opacity: 100%;
      --md-filled-button-disabled-label-text-color: var(--cros-sys-disabled);
      --md-filled-button-disabled-label-text-opacity: 100%;
      --md-filled-button-focus-state-layer-opacity: 100%;
      --md-filled-button-hover-container-elevation: 0;
      --md-filled-button-hover-state-layer-opacity: 100%;
      --md-filled-button-pressed-state-layer-opacity: 100%;
      --md-filled-button-spacing-leading: ${PADDING};
      --md-filled-button-spacing-trailing: ${PADDING};
      --md-filled-button-with-icon-icon-size: ${ICON_SIZE};
      --md-filled-button-with-icon-disabled-icon-color: var(--cros-sys-disabled);
      --md-filled-button-with-icon-disabled-icon-opacity: 100%;
      --md-filled-button-with-icon-spacing-leading: ${PADDING_ICON_SIDE};
      --md-filled-button-with-icon-spacing-trailing: ${PADDING};
      --md-filled-button-with-trailing-icon-spacing-leading: ${PADDING};
      --md-filled-button-with-trailing-icon-spacing-trailing: ${
      PADDING_ICON_SIDE};
      --md-sys-color-secondary: var(--cros-sys-focus_ring);
      --md-filled-button-label-text-type: var(--cros-typography-button2);
    }

    md-filled-button.primary {
      --md-sys-color-primary: var(--cros-sys-primary);
      --md-sys-color-on-primary: var(--cros-sys-on_primary);
      --md-filled-button-with-icon-icon-color: var(--cros-sys-on_primary);
      --md-filled-button-hover-state-layer-color: var(--cros-sys-hover_on_prominent);
      --md-filled-button-pressed-state-layer-color: var(--cros-sys-ripple_primary);
    }

    md-filled-button.secondary {
      --md-sys-color-primary: var(--cros-sys-primary_container);
      --md-sys-color-on-primary: var(--cros-sys-on_primary_container);
      --md-filled-button-with-icon-icon-color: var(--cros-sys-on_primary_container);
      --md-filled-button-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-filled-button-pressed-state-layer-color: var(--cros-sys-ripple_primary);
    }

    md-text-button {
      max-width: ${MAX_WIDTH};
      min-width: ${MIN_WIDTH};
      --md-text-button-container-height: ${CONTAINER_HEIGHT};
      --md-text-button-disabled-label-text-color: var(--cros-sys-disabled);
      --md-text-button-disabled-label-text-opacity: 100%;
      --md-text-button-focus-state-layer-opacity: 100%;
      --md-text-button-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-text-button-hover-state-layer-opacity: 100%;
      --md-text-button-pressed-state-layer-color: var(--cros-sys-ripple_neutral_on_subtle);
      --md-text-button-pressed-state-layer-opacity: 100%;
      --md-text-button-spacing-leading: ${PADDING};
      --md-text-button-spacing-trailing: ${PADDING};
      --md-text-button-with-icon-disabled-icon-color: var(--cros-sys-disabled);
      --md-text-button-with-icon-disabled-icon-opacity: 100%;
      --md-text-button-with-icon-icon-size: ${ICON_SIZE};
      --md-text-button-with-icon-icon-color: var(--cros-sys-primary);
      --md-text-button-with-icon-spacing-leading: ${PADDING_ICON_SIDE};
      --md-text-button-with-icon-spacing-trailing: ${PADDING};
      --md-text-button-with-trailing-icon-spacing-leading: ${PADDING};
      --md-text-button-with-trailing-icon-spacing-trailing: ${
      PADDING_ICON_SIDE};
      --md-sys-color-primary: var(--cros-sys-primary);
      --md-sys-color-secondary: var(--cros-sys-focus_ring);
      --md-text-button-label-text-type: var(--cros-typography-button2);
    }
  `;

  /** @export */
  label: string;
  /** @export */
  disabled: boolean;
  /** @export */
  trailingIcon: boolean;
  /**
   * How the button should be styled. One of {primary, secondary, floating}.
   * @export
   */
  buttonStyle: 'primary'|'secondary'|'floating' = 'primary';

  /** @nocollapse */
  static override properties = {
    label: {type: String},
    disabled: {type: Boolean},
    trailingIcon: {type: Boolean},
    buttonStyle: {type: String},
  };

  constructor() {
    super();
    this.label = '';
    this.disabled = false;
    this.trailingIcon = false;
  }

  override render() {
    if (this.buttonStyle === 'floating') {
      return html`
        <md-text-button class=${this.buttonStyle}
            ?disabled=${this.disabled}
            ?trailingIcon=${this.trailingIcon}>
          <slot slot="icon" name="icon"></slot>
          ${this.label}
        </md-text-button>
        `;
    }

    return html`
        <md-filled-button class=${this.buttonStyle}
            ?disabled=${this.disabled}
            ?trailingIcon=${this.trailingIcon}>
          <slot slot="icon" name="icon"></slot>
          ${this.label}
        </md-filled-button>
        `;
  }
}

customElements.define('cros-button', Button);

declare global {
  interface HTMLElementTagNameMap {
    'cros-button': Button;
  }
}
