/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/textfield/filled-text-field';

import {MdFilledTextField, TextFieldType as MdTextfieldType} from '@material/web/textfield/filled-text-field';
import {css, CSSResultGroup, html, LitElement, PropertyValues} from 'lit';
import {customElement, property, query} from 'lit/decorators';

/**
 * Textfields have two variants that differ only by the container background,
 * designed to improve contrast depending on the color of the surface behind it:
 * 1) textfield-on-app-base (lighter background, app base)
 * 2) textfield-on-shaded (darker background, shaded base)
 *
 * Consumers of the component can use the `shaded` property to ensure that it
 * has the correct container color for their use case.
 */
const TEXTFIELD_CONTAINER_ON_BASE = css`var(--cros-sys-input_field_dark)`;
const TEXTFIELD_CONTAINER_ON_SHADED = css`var(--cros-sys-input_field_light)`;

/**
 * Textfield component. See the specs here:
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=3227%3A25882&t=50tDpMdSJky6eT9O-0
 */
@customElement('cros-textfield')
export class Textfield extends LitElement {
  @query('md-filled-text-field') mdTextfield?: MdFilledTextField;
  @property({attribute: true, type: String}) type: MdTextfieldType = 'text';

  /**
   * When false, will use the darker container designed to sit on app-base. When
   * true, will use the lighter container colored designed for use with
   * app-base-shaded. Purely a cosmetic difference to improve contrast.
   */
  @property({type: Boolean, reflect: true}) shaded = false;

  @property({attribute: true, type: String})
  get value() {
    return this.mdTextfield?.value || '';
  }

  set value(value: string) {
    // On first render the initial value is lost because mwcTextfield is not yet
    // rendered. Store it in `valueInternal` and set it in firstUpdated().
    this.valueInternal = value;
    if (this.mdTextfield) {
      this.mdTextfield.value = this.valueInternal;
    }
  }

  /**
   * The visible label above the textfield. This is also used as the aria-label
   * for the internal md-textfield.
   */
  @property({attribute: true, type: String}) label = '';

  /**
   * Properties supported by the MD textfield that are surfaced to the
   * cros-textfield API. These are manually plumbed through as cros-textfield is
   * not a subclass. This is not an exhaustive list, and support should be
   * added by clients as needed.
   */
  @property({attribute: true, type: String}) suffix = '';
  @property({attribute: true, type: Boolean}) disabled = false;
  @property({attribute: true, type: String}) hint = '';

  /**
   * Max length of the textfield value. If set to -1 or less, md-textfield will
   * ignore this value when restricting the length of the textfield, and also
   * not render the charCounter.
   */
  @property({attribute: true, type: Number}) maxLength = -1;

  private valueInternal = '';

  static override styles: CSSResultGroup = css`

    :host {
      display: block;
      /* TODO(b/241483751): Use Jelly unified typography system. */
      font-family: 'Roboto', 'Noto', sans-serif;
      font-size: 10px;
    }

    :host(.focused) {
      outline: none;
    }

    :host(.focused) #visibleLabel {
      color: var(--cros-sys-primary);
    }

    :host(.error) #visibleLabel {
      color: var(--cros-sys-error)
    }

    :host([disabled]) #visibleLabel {
      opacity: var(--cros-sys-opacity-disabled);
    }

    md-filled-text-field {
      --md-filled-text-field-active-indicator-height: 0;
      --md-filled-text-field-active-indicator-color: var(--cros-sys-primary);
      --md-filled-text-field-container-color: ${TEXTFIELD_CONTAINER_ON_BASE};
      --md-filled-text-field-container-padding-vertical: 6px;
      --md-filled-text-field-container-shape-start-start: 8px;
      --md-filled-text-field-container-shape-start-end: 8px;
      --md-filled-text-field-container-shape-end-start: 8px;
      --md-filled-text-field-container-shape-end-end: 8px;
      --md-filled-text-field-disabled-active-indicator-height: 0;
      --md-filled-text-field-disabled-input-text-color: var(--cros-sys-disabled);
      --md-filled-text-field-error-supporting-text-color: var(--cros-sys-error);
      --md-filled-text-field-error-hover-state-layer-opacity: 0;
      --md-filled-text-field-error-input-text-color: var(--cros-sys-on_surface);
      --md-filled-text-field-error-focus-input-text-color: var(--cros-sys-on_surface);
      --md-filled-text-field-error-focus-active-indicator-color: var(--cros-sys-error);
      --md-filled-text-field-error-hover-input-text-color: var(--cros-sys-on_surface);
      --md-filled-text-field-error-hover-supporting-text-color: var(--cros-sys-error);
      --md-filled-text-field-focus-active-indicator-color: var(--cros-sys-primary);
      --md-filled-text-field-focus-active-indicator-height: 2px;
      --md-filled-text-field-hover-state-layer-opacity: 0;
      --md-filled-text-field-hover-active-indicator-height: 0;
      --md-filled-text-field-input-text-color: var(--cros-sys-on_surface);
      --md-filled-text-field-input-text-prefix-color: var(--cros-sys-on_surface);
      --md-filled-text-field-hover-input-text-color: var(--cros-sys-on_surface);
      --md-filled-field-hover-supporting-text-color: var(--cros-sys-on_surface);
      --md-filled-text-field-focus-input-text-color: var(--cros-sys-on_surface);
      --md-filled-text-field-pressed-active-indicator-height: 2px;
      --md-filled-text-field-pressed-active-indicator-color: var(--cros-sys-primary);
      --md-filled-text-field-supporting-text-color: var(--cros-sys-on_surface);
      width: 100%;
    }

    :host([shaded]) md-filled-text-field {
      --md-filled-text-field-container-color: ${TEXTFIELD_CONTAINER_ON_SHADED};
    }

    #visibleLabel {
      color: var(--cros-sys-on_surface);
      padding-bottom: 8px;
    }
  `;

  override async firstUpdated(changedProperties: PropertyValues<Textfield>) {
    this.mdTextfield!.value = this.valueInternal;
  }

  override update(changedProperties: PropertyValues<Textfield>) {
    // There is no corresponding 'valid' event to match md-textfield's
    // 'invalid' event, so just check on every update to see if it's still
    // invalid and remove the class if not. It's possible to also just
    // toggleErrorStyle() on update exclusively, but listening for the invalid
    // event allows us to sync the style update with MD's style update.
    if (this.mdTextfield && this.classList.contains('error')) {
      this.toggleErrorStyles(this.mdTextfield.error);
    }
    super.update(changedProperties);
  }

  override render() {
    return html`
      <div id="visibleLabel">${this.label}</div>
      <md-filled-text-field
        ?disabled=${this.disabled}
        type=${this.type}
        data-aria-label=${this.label}
        value=${this.value}
        suffixtext=${this.suffix}
        maxLength=${this.maxLength}
        supportingText=${this.hint}
        @focus=${this.toggleFocusStyle}
        @blur=${this.toggleFocusStyle}
        @change=${this.checkAndUpdateValidity}
        @invalid=${() => void this.toggleErrorStyles(true)}>
      </md-filled-text-field>
    `;
  }

  private checkAndUpdateValidity() {
    const valid = this.mdTextfield!.reportValidity();
    this.toggleErrorStyles(!valid);
  }

  private toggleErrorStyles(error: boolean) {
    this.classList.toggle('error', error);
  }
  private toggleFocusStyle() {
    this.classList.toggle('focused');
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cros-textfield': Textfield;
  }
}
