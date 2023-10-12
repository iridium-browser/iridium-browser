/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/button/filled-button.js';
import '@material/web/button/text-button.js';

import {css, CSSResultGroup, html, LitElement} from 'lit';

// The padding on the label start/end when there is no icons.
const LABEL_PADDING_START_END = css`16px`;
// The padding on the icon start/end when present.
const ICON_PADDING_START_END = css`12px`;
// The inline gap between the label and the optional icons.
const ICON_GAP = css`8px`;
const CONTAINER_HEIGHT = css`36px`;
const ICON_SIZE = css`20px`;
const MIN_WIDTH = css`64px`;

/**
 * A chromeOS compliant button.
 * See spec
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=2116%3A4082&t=kbaCFk5KdayGTyuL-0
 */
export class Button extends LitElement {
  static override shadowRootOptions:
      ShadowRootInit = {mode: 'open', delegatesFocus: true};

  // TODO(b/258982831): Centre icon. See spec.
  // Note that theme colours have opacity defined in the colour, but default
  // colours have opacities set separately. As a consequence, styles are broken
  // unless a cros theme is present.
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      display: inline-block;
      --cros-button-max-width_ : var(--cros-button-max-width,200px);
    }

    ::slotted(*) {
      display: inline-flex;
      block-size: ${ICON_SIZE};
      inline-size: ${ICON_SIZE};
    }

    .content-container {
      align-items: center;
      display: flex;
      gap: ${ICON_GAP};
    }

    md-filled-button:has(.content-container.has-leading-icon)  {
      --md-filled-button-leading-space: ${ICON_PADDING_START_END};
    }

    md-filled-button:has(.content-container.has-trailing-icon)  {
      --md-filled-button-trailing-space: ${ICON_PADDING_START_END};
    }

    md-text-button:has(.content-container.has-leading-icon)  {
      --md-text-button-leading-space: ${ICON_PADDING_START_END};
    }

    md-text-button:has(.content-container.has-trailing-icon)  {
      --md-text-button-trailing-space: ${ICON_PADDING_START_END};
    }

    md-filled-button {
      max-width: var(--cros-button-max-width_);
      min-width: ${MIN_WIDTH};
      --md-filled-button-container-height: ${CONTAINER_HEIGHT};
      --md-filled-button-disabled-container-color: var(--cros-sys-disabled_container);
      --md-filled-button-disabled-container-opacity: 100%;
      --md-filled-button-disabled-label-text-color: var(--cros-sys-disabled);
      --md-filled-button-disabled-label-text-opacity: 100%;
      --md-filled-button-focus-state-layer-opacity: 100%;
      --md-filled-button-hover-container-elevation: 0;
      --md-filled-button-hover-state-layer-opacity: 100%;
      --md-filled-button-label-text-type: var(--cros-button-2-font);
      --md-filled-button-leading-space: ${LABEL_PADDING_START_END};
      --md-filled-button-pressed-state-layer-opacity: 100%;
      --md-filled-button-trailing-space: ${LABEL_PADDING_START_END};
      --md-sys-color-secondary: var(--cros-sys-focus_ring);
    }

    :host([button-style="primary"]) md-filled-button {
      --md-sys-color-primary: var(--cros-sys-primary);
      --md-sys-color-on-primary: var(--cros-sys-on_primary);
      --md-filled-button-hover-state-layer-color: var(--cros-sys-hover_on_prominent);
      --md-filled-button-pressed-state-layer-color: var(--cros-sys-ripple_primary);
    }

    :host([button-style="secondary"]) md-filled-button {
      --md-filled-button-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-filled-button-pressed-state-layer-color: var(--cros-sys-ripple_primary);
      --md-sys-color-primary: var(--cros-sys-primary_container);
      --md-sys-color-on-primary: var(--cros-sys-on_primary_container);
    }

    md-text-button {
      max-width: var(--cros-button-max-width_);
      min-width: ${MIN_WIDTH};
      --md-sys-color-primary: var(--cros-sys-primary);
      --md-sys-color-secondary: var(--cros-sys-focus_ring);
      --md-text-button-container-height: ${CONTAINER_HEIGHT};
      --md-text-button-disabled-label-text-color: var(--cros-sys-disabled);
      --md-text-button-disabled-label-text-opacity: 100%;
      --md-text-button-focus-state-layer-opacity: 100%;
      --md-text-button-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-text-button-hover-state-layer-opacity: 100%;
      --md-text-button-label-text-type: var(--cros-button-2-font);
      --md-text-button-leading-space: ${LABEL_PADDING_START_END};
      --md-text-button-pressed-state-layer-color: var(--cros-sys-ripple_neutral_on_subtle);
      --md-text-button-pressed-state-layer-opacity: 100%;
      --md-text-button-trailing-space: ${LABEL_PADDING_START_END};
    }
  `;

  /** @export */
  label: string;
  /** @export */
  disabled: boolean;
  /**
   * How the button should be styled. One of {primary, secondary, floating}.
   * @export
   */
  buttonStyle: 'primary'|'secondary'|'floating' = 'primary';

  /** @nocollapse */
  static override properties = {
    ariaLabel: {type: String, reflect: true, attribute: 'aria-label'},
    label: {type: String, reflect: true},
    disabled: {type: Boolean, reflect: true},
    buttonStyle: {type: String, reflect: true, attribute: 'button-style'},
  };

  constructor() {
    super();
    this.ariaLabel = '';
    this.ariaHasPopup = 'false';
    this.label = '';
    this.disabled = false;
  }

  override render() {
    const ariaHasPopup = (this.ariaHasPopup ?? 'false') as 'false' | 'true' |
        'menu' | 'listbox' | 'tree' | 'grid' | 'dialog';

    if (this.buttonStyle === 'floating') {
      return html`
        <md-text-button
            aria-label=${this.ariaLabel || ''}
            aria-haspopup=${ariaHasPopup}
            ?disabled=${this.disabled}>
          ${this.renderButtonContent()}
        </md-text-button>
        `;
    }

    return html`
        <md-filled-button
            aria-label=${this.ariaLabel || ''}
            aria-haspopup=${ariaHasPopup}
            ?disabled=${this.disabled}>
          ${this.renderButtonContent()}
        </md-filled-button>
        `;
  }

  private renderButtonContent() {
    return html`
      <div class="content-container">
        <slot name="leading-icon" @slotchange=${this.onSlotChange}></slot>
        ${this.label}
        <slot name="trailing-icon" @slotchange=${this.onSlotChange}></slot>
      </div>
    `;
  }

  private hasSlottedIcon(icon: 'leading'|'trailing'): boolean {
    return !!this.querySelector(`*[slot='${icon}-icon']`);
  }

  // The padding before/after the label changes based on whether there is an
  // icon present. We use the slot change event to toggle the different padding
  // styles on the container, as it's easier to achieve here compared with pure
  // CSS selectors. The actual padding is applied to the material button which
  // is difficult to define a pure CSS relationship with due to the nature of
  // how the slots are arranged.
  private onSlotChange() {
    const container = this.shadowRoot!.querySelector('.content-container');
    container!.classList.toggle(
        'has-leading-icon', this.hasSlottedIcon('leading'));
    container!.classList.toggle(
        'has-trailing-icon', this.hasSlottedIcon('trailing'));
  }
}

customElements.define('cros-button', Button);

declare global {
  interface HTMLElementTagNameMap {
    'cros-button': Button;
  }
}
