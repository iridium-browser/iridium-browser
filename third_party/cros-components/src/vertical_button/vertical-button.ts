/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/button/filled-button.js';
import '@material/web/button/text-button.js';

import {css, CSSResultGroup, html, LitElement} from 'lit';
import {styleMap} from 'lit/directives/style-map';

/**
 * A ChromeOS compliant vertical-button.
 * See spec
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?type=design&node-id=9644-165677&mode=design&t=0xBcgechLxIMCLG0-0
 */
export class VerticalButton extends LitElement {
  static BORDER_RADIUS = 20;
  static MIN_HEIGHT = 62;
  static ICON_SIZE = 20;
  static ICON_TEXT_PADDING = 4;
  static HORIZONTAL_PADDING = 16;
  static PADDING_BOTTOM = 8;
  static PADDING_TOP = 10;
  static ROW_HEIGHT = 20;
  static WIDTH = 112;

  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      display: inline-block;
    }
    md-filled-button {
      width: 100%;
      min-height: ${VerticalButton.MIN_HEIGHT}px;
      min-width: ${VerticalButton.WIDTH}px;
      --md-filled-button-leading-space: ${VerticalButton.HORIZONTAL_PADDING}px;
      --md-filled-button-trailing-space: ${VerticalButton.HORIZONTAL_PADDING}px;
      --md-filled-button-with-leading-icon-leading-space: 0px;
      --md-filled-button-with-leading-icon-trailing-space: 0px;
      --md-filled-button-with-trailing-icon-leading-space: 0px;
      --md-filled-button-with-trailing-icon-trailing-space: 0px;
      --md-filled-button-container-height: ${VerticalButton.MIN_HEIGHT}px;
      --md-filled-button-container-shape-start-start: ${
      VerticalButton.BORDER_RADIUS}px;
      --md-filled-button-container-shape-start-end: ${
      VerticalButton.BORDER_RADIUS}px;
      --md-filled-button-container-shape-end-start: ${
      VerticalButton.BORDER_RADIUS}px;
      --md-filled-button-container-shape-end-end: ${
      VerticalButton.BORDER_RADIUS}px;
      --md-filled-button-icon-color: var(--cros-sys-on_primary_container);
      --md-filled-button-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-filled-button-hover-container-elevation: 0;
      --md-filled-button-hover-state-layer-opacity: 100%;
      --md-filled-button-pressed-state-layer-color: var(--cros-sys-ripple_primary);
      --md-filled-button-pressed-state-layer-opacity: 100%;
      --md-filled-button-label-text-type: var(--cros-button-2-font);
      --md-sys-color-on-primary: var(--cros-sys-on_primary_container);
      --md-sys-color-primary: var(--cros-sys-secondary_container);
      --md-sys-color-secondary: var(--cros-sys-focus_ring);
    }
    :host([selected]) md-filled-button  {
      --md-filled-button-icon-color: var(--cros-sys-on_primary);
      --md-filled-button-hover-state-layer-color: var(--cros-sys-hover_on_prominent);
      --md-sys-color-on-primary: var(--cros-sys-on_primary);
      --md-sys-color-primary: var(--cros-sys-primary);
    }
    :host([disabled]) md-filled-button {
      --md-filled-button-icon-color: var(--cros-sys-disabled);
      --md-filled-button-disabled-label-text-color: var(--cros-sys-disabled);
      --md-filled-button-disabled-container-color: var(--cros-sys-disabled);
    }
    :host([disabled]:not([selected])) md-filled-button {
      --md-filled-button-disabled-container-color: var(--cros-sys-disabled_container);
    }
    :host div {
      padding-top: ${VerticalButton.PADDING_TOP}px;
      padding-bottom: ${VerticalButton.PADDING_BOTTOM}px;
      display: flex;
      align-items: center;
      flex-direction: column;
      justify-content: center;
      width: 100%;
    }
    :host div span {
      white-space: pre-wrap;
    }
    ::slotted([slot="icon"]) {
      margin-bottom: ${VerticalButton.ICON_TEXT_PADDING}px;
      height: ${VerticalButton.ICON_SIZE}px;
      width: ${VerticalButton.ICON_SIZE}px;
      display: block;
      vertical-align: middle;
    }
    `;

  /** @export */
  selected: boolean;
  /** @export */
  disabled: boolean;
  /** @export */
  label: string;
  /** @export */
  /*
   * Variation in height is driven by number of rows to display for the label.
   * Height will always be a multiple of row height (after all other
   * considerations - padding and icon size).
   *
   * Note: This does not autosize content as it may be desirable to size a
   * vertical-button as a number of rows equal to some other button in a group,
   * rather than due to content in this particular button.
   */
  labelRows: number;

  /** @nocollapse */
  static override properties = {
    selected: {type: Boolean, reflect: true},
    disabled: {type: Boolean, reflect: true},
    label: {type: String, reflect: true},
    labelRows: {type: Number, reflect: true, attribute: 'label-rows'},
    role: {type: String, reflect: true},
  };

  constructor() {
    super();
    this.selected = false;
    this.disabled = false;
    this.label = '';
    this.labelRows = 1;
    this.role = 'button';
  }

  override render() {
    const styles = {
      '--md-filled-button-container-height':
          ((this.labelRows - 1) * VerticalButton.ROW_HEIGHT +
           VerticalButton.MIN_HEIGHT)
              .toString() +
          'px',
    };
    return html`
        <md-filled-button
          style=${styleMap(styles)}
          has-icon="false"
          ?selected=${this.selected}
          ?disabled=${this.disabled}
          aria-pressed=${this.selected ? 'true' : 'false'}>
          <div>
            <slot slot="icon" name="icon" aria-hidden="true"></slot>
            <span>${this.label}</span>
          </div>
        </md-filled-button>
        `;
  }
}

customElements.define('cros-vertical-button', VerticalButton);

declare global {
  interface HTMLElementTagNameMap {
    'cros-vertical-button': VerticalButton;
  }
}
