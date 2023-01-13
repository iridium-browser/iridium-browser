/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/button/filled-button.js';
import '@material/web/button/text-button.js';

import {css, CSSResultGroup, LitElement} from 'lit';
import {customElement, property} from 'lit/decorators';
import {html, literal} from 'lit/static-html.js';

function getButtonImpl(style: string) {
  switch (style) {
    case 'text':
      return literal`md-text-button`;
    case 'primary':
    case 'secondary':
    default:
      return literal`md-filled-button`;
  }
}

/**
 * A chromeOS compliant button.
 * See spec
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=2116%3A4082&t=kbaCFk5KdayGTyuL-0
 */
@customElement('cros-button')
export class Button extends LitElement {
  // TODO(b/258982831): Centre icon. See spec.
  // Note that theme colours have opacity defined in the colour, but default
  // colours have opacities set separately. As a consequence, styles are broken
  // unless a cros theme is present.
  static override styles: CSSResultGroup = css`
    md-filled-button {
      max-width: 200px;
      min-width: 64px;
      --md-sys-color-secondary: var(--cros-sys-focus_ring);
      --md-filled-button-container-height: 36px;
      --md-filled-button-with-icon-icon-size: 20px;
      --md-filled-button-disabled-container-color: var(--cros-sys-disabled_container);
      --md-filled-button-disabled-label-text-color: var(--cros-sys-disabled);
      --md-filled-button-with-icon-disabled-icon-color: var(--cros-sys-disabled);
      --md-filled-button-focus-state-layer-opacity: 100%;
      --md-filled-button-hover-state-layer-opacity: 100%;
      --md-filled-button-pressed-state-layer-opacity: 100%;
    }

    md-filled-button.primary {
      --md-sys-color-primary: var(--cros-sys-primary);
      --md-sys-color-on-primary: var(--cros-sys-on_primary);
      --md-filled-button-with-icon-icon-color: var(--cros-sys-on_primary);
      --md-filled-button-hover-state-layer-color: var(--cros-sys-hover_on_prominent);
      --md-filled-button-pressed-state-layer-color: var(--cros-sys-hover_on_prominent);
    }

    md-filled-button.secondary {
      --md-sys-color-primary: var(--cros-sys-primary_container);
      --md-sys-color-on-primary: var(--cros-sys-on_primary_container);
      --md-filled-button-with-icon-icon-color: var(--cros-sys-on_primary_container);
      --md-filled-button-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-filled-button-pressed-state-layer-color: var(--cros-sys-ripple_primary);
    }

    md-text-button {
      max-width: 200px;
      min-width: 64px;
      --md-text-button-container-height: 36px;
      --md-text-button-with-icon-icon-size: 20px;
      --md-sys-color-primary: var(--cros-sys-primary);
      --md-sys-color-secondary: var(--cros-sys-focus_ring);
      --md-text-button-with-icon-icon-color: var(--cros-sys-primary);
      --md-text-button-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-text-button-pressed-state-layer-color: var(--cros-sys-ripple_neutral_on_subtle);
      --md-text-button-disabled-label-text-color: var(--cros-sys-disabled);
      --md-text-button-with-icon-disabled-icon-color: var(--cros-sys-disabled);
      --md-text-button-focus-state-layer-opacity: 100%;
      --md-text-button-hover-state-layer-opacity: 100%;
      --md-text-button-pressed-state-layer-opacity: 100%;
    }
  `;

  @property({type: String}) label = '';
  @property({type: Boolean}) disabled = false;
  @property({type: Boolean}) trailingIcon = false;

  /** How the button should be styled. One of {primary, secondary, text} */
  @property({type: String})
  buttonStyle: 'primary'|'secondary'|'text' = 'primary';

  override render() {
    const mdButtonTag = getButtonImpl(this.buttonStyle);
    return html`
        <${mdButtonTag} class=${this.buttonStyle}
            label=${this.label}
            ?disabled=${this.disabled}
            ?trailingIcon=${this.trailingIcon}>
          <slot slot="icon" name="icon"></slot>
        </${mdButtonTag}>
        `;
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-button': Button;
  }
}
