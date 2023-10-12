/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/menu/menu-item';
import '../icon/icon';
import '../switch/switch';

import {MenuItem as MenuItemType} from '@material/web/menu/menu-item';
import {css, CSSResultGroup, html, LitElement, PropertyValues} from 'lit';

/**
 * A cros compliant menu-item component for use in cros-menu.
 * @fires trigger Fired when the user clicks on or otherwise activates the menu
 * item i.e. via a enter or space key.
 */
export class MenuItem extends LitElement implements MenuItemType {
  /** @nocollapse */
  // TODO(b/198759625): Remove negative margin-inline-end when mwc token
  // available (padding 16px)
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    md-menu-item {
      --md-list-item-disabled-label-text-opacity: var(--cros-sys-opacity-disabled);
      --md-list-item-disabled-leading-icon-opacity: var(--cros-sys-opacity-disabled);
      --md-list-item-disabled-trailing-icon-opacity: var(--cros-sys-opacity-disabled);
      --md-list-item-disabled-label-text-color: var(--cros-sys-on_surface);
      --md-list-item-focus-label-text-color: var(--cros-sys-on_surface);
      --md-list-item-hover-label-text-color: var(--cros-sys-on_surface);
      --md-list-item-hover-state-layer-color: var(--cros-sys-hover_on_subtle);
      --md-list-item-hover-state-layer-opacity: 1;
      --md-list-item-label-text-color: var(--cros-sys-on_surface);
      --md-list-item-label-text-type: var(--cros-button-2-font);
      --md-list-item-leading-icon-color: var(--cros-sys-on_surface);
      --md-list-item-pressed-label-text-color: var(--cros-sys-on_surface);
      --md-list-item-pressed-state-layer-color: var(--cros-sys-ripple_neutral_on_subtle);
      --md-list-item-pressed-state-layer-opacity: 1;
      --md-list-item-trailing-element-headline-trailing-element-space: 48px;
      --md-list-item-trailing-space: 16px;
    }
    md-menu-item([itemEnd=null]) {
      --md-list-item-trailing-space: 48px;
    }
    :host([itemEnd="icon"]) md-menu-item {
      --md-list-item-trailing-space: 12px;
    }
    :host([checked]) md-menu-item {
      --md-list-item-trailing-space: 12px;
    }
    md-menu-item::part(focus-ring) {
      --md-focus-ring-active-width: 2px;
      --md-focus-ring-color: var(--cros-sys-focus_ring);
      --md-focus-ring-width: 2px;
    }
    :host {
      display: inline block;
    }

    :host([checked]) cros-icon {
      color: var(--cros-sys-primary);
      height: 20px;
      width: 20px;
    }

    ::slotted([slot="start"]), ::slotted([slot="end"]) {
      color: var(--cros-sys-on_surface);
      height: 20px;
      width: 20px;
    }

    cros-switch {
      height: 18px;
      width: 32px;
    }

    .slot-end:not(cros-switch) {
      font: var(--cros-button-2-font);
      width: unset;
      height: unset;
    }

    #shortcut {
      color: var(--cros-sys-secondary);
    }
  `;

  static override shadowRootOptions = {
    mode: 'open' as const,
    delegatesFocus: true,
  };

  /** @nocollapse */
  static override properties = {
    headline: {type: String},
    itemStart: {type: String},
    itemEnd: {type: String},
    active: {type: Boolean, reflect: true},
    disabled: {type: Boolean},
    isMenuItem: {type: Boolean, reflect: true, attribute: 'md-menu-item'},
    isListItem: {type: Boolean, reflect: true, attribute: 'md-list-item'},
    checked: {type: Boolean, reflect: true},
    shortcutText: {type: String},
  };

  /**
   * Headline is the primary text of the list item, name follows from
   * md-menu-item.
   * @export
   */
  headline: string;
  /**
   * Item to be placed in the start slot if any. 'icon' exposes the start slot.
   * @export
   */
  itemStart: 'icon'|'';
  /**
   * Item to be placed in the end slot if any. Shortcut allows a text label (set
   * via `shortcutText`), switch uses cros-switch and 'icon' exposes the end
   * slot.
   * @export
   */
  itemEnd: 'shortcut'|'icon'|'switch'|null;

  // The following properties are necessary for wrapping and keyboard
  // navigation
  /**
   * Whether or not the menu-item is disabled.
   * @export
   */
  disabled: boolean;
  /** @export */
  readonly isMenuItem: boolean;
  /** @export */
  readonly isListItem: boolean;
  /**
   * Whether or not the element is actively being interacted with by md-list.
   * When active, tabindex is set to 0, and focuses the underlying item.
   * @export
   */
  active: boolean;
  /**
   * If true, this will result in a checkmark icon used in the end slot. This
   * overrides any other components in end slot if otherwise specified.
   * @export
   */
  checked: boolean;
  /**
   * The text that will appear in the end slot if menu-item is of `shortcut`
   * type. Defaults to 'Shortcut' if text is not given.
   * @export
   */
  shortcutText: string;

  constructor() {
    super();

    this.headline = '';
    this.itemStart = '';
    this.itemEnd = null;

    this.disabled = false;
    this.isMenuItem = true;
    this.isListItem = true;
    // NOTE: unlike md-sub-menu-item md-menu-item does not internally set
    // `.active`, so we do not require accessors to synchronize `this.active`
    // with `md-menu-item.active`
    this.active = false;
    this.checked = false;
    this.shortcutText = 'Shortcut';
  }

  // Start slot should exist when `itemStart` is  `icon`.
  private get startSlot(): HTMLSlotElement|null {
    return this.renderRoot.querySelector('slot[name="start"]');
  }

  // End slot should exist when `itemEnd` is `icon`.
  private get endSlot(): HTMLSlotElement|null {
    return this.renderRoot.querySelector('slot[name="end"]');
  }

  override updated(changedProperties: PropertyValues<this>) {
    super.updated(changedProperties);
    if (changedProperties.has('itemEnd') && this.itemEnd === 'switch') {
      this.addEventListener('keydown', this.switchKeyDownListener);
      this.setAttribute('role', 'menuitemcheckbox');
    }
    if (changedProperties.has('itemStart') && this.itemStart === 'icon' &&
        this.startSlot) {
      const startItems = this.startSlot.assignedElements();
      startItems.forEach((startItem) => {
        startItem.setAttribute('data-variant', 'icon');
      });
    }
    if (changedProperties.has('itemEnd') && this.itemEnd === 'icon' &&
        this.endSlot) {
      const endItems = this.endSlot.assignedElements();
      endItems.forEach((endItem) => {
        endItem.setAttribute('data-variant', 'icon');
      });
    }
  }

  override disconnectedCallback() {
    super.disconnectedCallback();
    if (this.itemEnd === 'switch') {
      this.removeEventListener('keydown', this.switchKeyDownListener);
    }
  }

  // Toggles switch item within the end slot on `enter` or `space`.
  private readonly switchKeyDownListener = (e: KeyboardEvent) => {
    if (e.key === 'Enter' || e.key === ' ') {
      const crosSwitch = this.renderRoot.querySelector('cros-switch')!;
      crosSwitch.selected = !crosSwitch.selected;
    }
  };

  // Set item to be placed in `end` slot of menu item (if any).
  // Checked property (check mark) will always take precedence over any icon
  // placed in end slot (if both are specified the checked icon will be used).
  // `switch` enforces cros-switch to be placed into the md-menu-item end slot,
  // `shortcut` uses a span that uses `shortcutText` as a label in the
  // md-menu-item end slot. `icon` exposes a slot so the user can pass in a
  // slotted element via cros-menu-item.
  private getEndSlot() {
    if (this.checked) {
      return html`
          <cros-icon
              slot="end"
              class="slot-end"
              data-variant="icon"
              aria-hidden="true">
          </cros-icon>`;
    }
    let endSlot;
    switch (this.itemEnd) {
      case 'shortcut':
        endSlot = html`
          <span
              slot="end"
              class="slot-end"
              id="shortcut"
              data-variant="icon">
            ${this.shortcutText}
          </span>`;
        break;
      case 'icon':
        endSlot = html`
          <slot
              name="end"
              slot="end"
              class="slot-end"
              aria-hidden="true">
          </slot>`;
        break;
      case 'switch':
        endSlot = html`
          <cros-switch
              slot="end"
              class="slot-end"
              data-variant="icon">
          </cros-switch>`;
        break;
      default:
        // md-list-item has a ::slotted(*) selector which will never select
        // the children of a <slot> or a <slot> itself which is why default
        // content is not set as children of slot.
        // i.e. <slot slot="end">${slotEnd}</slot>
        endSlot = html`<slot slot="end"></slot>`;
        break;
    }
    return endSlot;
  }

  private fireTriggerEvent() {
    this.dispatchEvent(
        new CustomEvent<void>('trigger', {bubbles: true, composed: true}));
  }

  override render() {
    const endSlot = this.getEndSlot();
    // `keep-open` property needs to be true in switch case so cros-switch can
    // be selected/unselected without menu closing.
    const keepOpen = this.itemEnd === 'switch';
    let startSlot = null;
    if (this.itemStart === 'icon') {
      startSlot =
          html`<slot name="start" slot="start" aria-hidden="true"></slot>`;
    }
    return html`
      <md-menu-item
          @close-menu=${() => void this.fireTriggerEvent()}
          headline=${this.headline}
          .keepOpen=${keepOpen}
          .disabled=${this.disabled}
          .active=${this.active}>
        ${startSlot}
        ${endSlot}
      </md-menu-item>
      `;
  }

  // This is just in case `delegatesFocus` doesn't do what is intended (which
  // is unfortunately often across browser updates).
  override focus() {
    this.renderRoot.querySelector('md-menu-item')?.focus();
  }
}

customElements.define('cros-menu-item', MenuItem);

declare global {
  interface HTMLElementTagNameMap {
    'cros-menu-item': MenuItem;
  }
}
