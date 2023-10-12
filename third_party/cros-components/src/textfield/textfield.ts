/**
 * @license
 * Copyright 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/textfield/outlined-text-field';

import {MdOutlinedTextField, TextFieldType} from '@material/web/textfield/outlined-text-field';
import {css, CSSResultGroup, html, LitElement, nothing, PropertyValues} from 'lit';

/**
 * Textfields have two variants that differ only by the container background,
 * designed to improve contrast depending on the color of the surface behind it:
 * 1) textfield-on-app-base (lighter background, app base)
 * 2) textfield-on-shaded (darker background, shaded base)
 *
 * Consumers of the component can use the `shaded` property to ensure that it
 * has the correct container color for their use case.
 */
const TEXTFIELD_CONTAINER_ON_BASE = css`var(--cros-sys-input_field_on_base)`;
const TEXTFIELD_CONTAINER_ON_SHADED =
    css`var(--cros-sys-input_field_on_shaded)`;

/**
 * Textfield component. See the specs here:
 * https://www.figma.com/file/1XsFoZH868xLcLPfPZRxLh/CrOS-Next---Component-Library-%26-Spec?node-id=3227%3A25882&t=50tDpMdSJky6eT9O-0
 */
export class Textfield extends LitElement {
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      display: block;
    }

    :host(.focused) {
      outline: none;
    }

    :host([disabled]) {
      opacity: var(--cros-disabled-opacity);
    }

    md-outlined-text-field {
      /** Base styles */
      --md-outlined-field-leading-content-color: var(--cros-sys-secondary);
      --md-outlined-field-trailing-content-color: var(--cros-sys-secondary);
      --md-outlined-field-supporting-text-color: var(--cros-sys-on_surface);
      --md-outlined-field-supporting-text-leading-space: 0px;
      --md-outlined-field-supporting-text-trailing-space: 0px;
      --md-outlined-field-supporting-text-top-space: 8px;;
      --md-outlined-text-field-container-shape-end-end: 8px;
      --md-outlined-text-field-container-shape-end-start: 8px;
      --md-outlined-text-field-container-shape-start-end: 8px;
      --md-outlined-text-field-container-shape-start-start: 8px;
      --md-outlined-text-field-bottom-space: 8px;
      --md-outlined-text-field-input-text-color: var(--cros-sys-on_surface);
      --md-outlined-text-field-input-text-placeholder-color: var(--cros-sys-secondary);
      --md-outlined-text-field-input-text-prefix-color: var(--cros-sys-on_surface);
      --md-outlined-text-field-input-text-suffix-color: var(--cros-sys-on_surface);
      --md-outlined-text-field-input-text-type: var(--cros-body-2-font);
      --md-outlined-text-field-outline-width: 0px;
      --md-outlined-text-field-supporting-text-type: var(--cros-label-2-font);
      --md-outlined-text-field-top-space: 8px;
      /** Disabled */
      --md-outlined-field-disabled-leading-content-color: var(--cros-sys-secondary);
      --md-outlined-field-disabled-trailing-content-color: var(--cros-sys-secondary);
      --md-outlined-field-disabled-supporting-text-color: var(--cros-sys-on_surface);
      --md-outlined-text-field-disabled-input-text-color: var(--cros-sys-on_surface);
      --md-outlined-text-field-disabled-input-text-prefix-color: var(--cros-sys-on_surface);
      --md-outlined-text-field-disabled-input-text-suffix-color: var(--cros-sys-on_surface);
      /** Error */
      --md-outlined-field-error-focus-leading-content-color: var(--cros-sys-secondary);
      --md-outlined-field-error-focus-trailing-content-color: var(--cros-sys-secondary);
      --md-outlined-field-error-leading-content-color: var(--cros-sys-secondary);
      --md-outlined-field-error-trailing-content-color: var(--cros-sys-secondary);
      --md-outlined-text-field-error-focus-input-text-color: var(--cros-sys-on_surface);
      --md-outlined-text-field-error-hover-input-text-color: var(--cros-sys-on_surface);
      --md-outlined-text-field-error-hover-state-layer-opacity: 0;
      --md-outlined-text-field-error-hover-supporting-text-color: var(--cros-sys-error);
      --md-outlined-text-field-error-input-text-color: var(--cros-sys-on_surface);
      --md-outlined-text-field-error-supporting-text-color: var(--cros-sys-error);
      /** Focus */
      --md-outlined-field-focus-leading-content-color: var(--cros-sys-secondary);
      --md-outlined-field-focus-trailing-content-color: var(--cros-sys-secondary);
      --md-outlined-field-focus-outline-color: var(--cros-sys-primary);
      --md-outlined-text-field-focus-outline-color: var(--cros-sys-primary);
      --md-outlined-text-field-focus-caret-color: var(--cros-sys-primary);
      --md-outlined-text-field-focus-input-text-color: var(--cros-sys-on_surface);
      /** Hover */
      --md-outlined-field-hover-leading-content-color: var(--cros-sys-secondary);
      --md-outlined-field-hover-trailing-content-color: var(--cros-sys-secondary);
      --md-outlined-text-field-caret-color: var(--cros-sys-primary);
      --md-outlined-text-field-hover-outline-width: 0px;
      --md-outlined-text-field-hover-supporting-text-color: var(--cros-sys-on_surface);
      --md-outlined-text-field-hover-input-text-color: var(--cros-sys-on_surface);
      --md-outlined-text-field-hover-state-layer-opacity: 0;
      width: 100%;
    }

    .text-labels {
      color: var(--cros-sys-on_surface);
    }

    :host(.focused) .text-labels {
      color: var(--cros-sys-primary);
    }

    :host(.error) .text-labels {
      color: var(--cros-sys-error)
    }

    #visible-label {
      font: var(--cros-label-1-font);
      padding-bottom: 8px;
    }

    #main-container {
      position: relative;
    }

    #textfield-background {
      position:absolute;
      min-height: 36px;
      width: 100%;
      border-radius: 8px;
      background-color: ${TEXTFIELD_CONTAINER_ON_BASE};
    }

    :host([shaded]) #textfield-background {
      background-color: ${TEXTFIELD_CONTAINER_ON_SHADED};
    }
  `;

  /** @nocollapse */
  static override properties = {
    type: {type: String, attribute: true},
    shaded: {type: Boolean, reflect: true},
    value: {type: String, attribute: true},
    label: {type: String, attribute: true},
    suffix: {type: String, attribute: true},
    disabled: {type: Boolean, reflect: true},
    hint: {type: String, attribute: true},
    maxLength: {type: Number, attribute: true},
    placeholder: {type: String, attribute: true},
  };

  /** @nocollapse */
  static events = {
    /** The textfield value changed via user input. */
    CHANGE: 'change',
  } as const;

  private valueInternal = '';


  /** @export */
  type: TextFieldType = 'text';
  /**
   * When false, will use the darker container designed to sit on app-base. When
   * true, will use the lighter container colored designed for use with
   * app-base-shaded. Purely a cosmetic difference to improve contrast.
   * @export
   */
  shaded = false;

  /** @export */
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
   * @export
   */
  label: string;
  // Properties supported by the MD textfield that are surfaced to the
  // cros-textfield API. These are manually plumbed through as cros-textfield is
  // not a subclass. This is not an exhaustive list, and support should be
  // added by clients as needed.
  /** @export */
  suffix: string;
  /** @export */
  disabled: boolean;
  /** @export */
  hint: string;
  /**
   * Max length of the textfield value. If set to -1 or less, md-textfield will
   * ignore this value when restricting the length of the textfield, and also
   * not render the charCounter.
   * @export
   */
  maxLength = -1;
  /** @export */
  placeholder = '';

  get mdTextfield(): MdOutlinedTextField|undefined {
    return this.renderRoot?.querySelector('md-outlined-text-field') ||
        undefined;
  }

  constructor() {
    super();
    this.type = 'text';
    this.shaded = false;
    this.label = '';
    this.suffix = '';
    this.disabled = false;
    this.hint = '';
    this.maxLength = -1;
  }

  override async firstUpdated() {
    this.mdTextfield!.value = this.valueInternal;
    // Run the logic to forward any slotted icons to the md-text-field internal
    // icon slots.
    this.handleIconChange();
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
      ${this.maybeRenderLabel()}
      <div id="main-container">
        <div id="textfield-background"></div>
        <md-outlined-text-field
            ?disabled=${this.disabled}
            type=${this.type}
            data-aria-label=${this.label}
            value=${this.value}
            suffix-text=${this.suffix}
            maxLength=${this.maxLength}
            supporting-text=${this.hint}
            placeholder=${this.placeholder}
            @focus=${this.toggleFocusStyle}
            @blur=${this.toggleFocusStyle}
            @change=${this.onChange}
            @invalid=${() => void this.toggleErrorStyles(true)}>
          <slot name="leading" @slotchange=${this.handleIconChange}></slot>
          <slot
              name="trailing"
              @slotchange=${this.handleIconChange}>
          </slot>
        </md-outlined-text-field>
      </div>
    `;
  }

  private maybeRenderLabel() {
    return this.label ?
        html`<div id="visible-label" class="text-labels">${this.label}</div>` :
        nothing;
  }

  private handleIconChange() {
    const {leadingIcon, trailingIcon} = this.queryIconSlots();
    // When no icon in slot is given, we need to remove the slot attribute from
    // cros-icon-button, otherwise md-textfield will still render an empty
    // space.
    const leadingSlot = this.shadowRoot!.querySelector('slot[name="leading"]');
    const trailingSlot =
        this.shadowRoot!.querySelector('slot[name="trailing"]');

    if (leadingIcon) {
      leadingSlot!.setAttribute('slot', 'leadingicon');
    } else {
      leadingSlot!.removeAttribute('slot');
    }
    if (trailingIcon) {
      trailingSlot!.setAttribute('slot', 'trailingicon');
    } else {
      trailingSlot!.removeAttribute('slot');
    }
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

  private onChange() {
    this.checkAndUpdateValidity();
    this.dispatchEvent(new Event('change', {bubbles: true}));
  }

  private getSlottedIcon(slotName: 'leading'|'trailing') {
    return this.querySelector(`*[slot="${slotName}"]`) !== null;
  }

  private queryIconSlots() {
    return {
      leadingIcon: this.getSlottedIcon('leading'),
      trailingIcon: this.getSlottedIcon('trailing'),
    };
  }
}

customElements.define('cros-textfield', Textfield);

declare global {
  interface HTMLElementEventMap {
    [Textfield.events.CHANGE]: Event;
  }

  interface HTMLElementTagNameMap {
    'cros-textfield': Textfield;
  }
}
