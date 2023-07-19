/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/menu/menu.js';
import '@material/web/menu/sub-menu-item';
import '@material/web/menu/menu-item';
import './menu_item';

import {css, CSSResultGroup, html, LitElement} from 'lit';

// From google3/chrome/apps/common/client/icons/chromeos/ic_chevron-right.svg
const chevronIcon =
    html`<svg width="20" height="20" viewBox="0 0 20 20" xmlns="http://www.w3.org/2000/svg" slot="end" id="icon">
      <path fill-rule="evenodd" clip-rule="evenodd" d="M6.66669 13.825L10.7872 10L6.66669 6.175L7.93524 5L13.3334 10L7.93524 15L6.66669 13.825Z"/>
    </svg>`;
/**
 * ChromeOS menu item which represents an openable submenu.
 * Hovering or clicking element will open a second submenu.
 */
export class SubMenuItem extends LitElement {
  // TODO(b/276045973): Currently elevation layer is on top of selected state
  // color making it darker than hover. Per chrome spec we want hover/selected
  // to be the same. Once opacities are flattened & colors updated, update
  // screenshot tests for components. For now we remove color altogether for
  // selected state.
  static override styles: CSSResultGroup = css`
    md-sub-menu-item {
      --md-menu-item-list-item-selected-container-color: #00000000;
    }
    :host-context(html[dir=rtl]) #icon {
      transform: scaleX(-1);
    }
  `;

  static override shadowRootOptions = {
    mode: 'open' as const,
    delegatesFocus: true,
  };

  /** @nocollapse */
  static override properties = {
    headline: {type: String},
    active: {type: Boolean, reflect: true},
    disabled: {type: Boolean},
    isMenuItem: {type: Boolean, reflect: true, attribute: 'md-menu-item'},
    isListItem: {type: Boolean, reflect: true, attribute: 'md-list-item'},
  };

  /**
   * Headline is the primary text of the list item, name follows
   * from md-menu-item.
   */
  headline: string;

  // The following properties are necessary for wrapping and keyboard navigation
  /**
   * Whether or not the menu-item is disabled.
   */
  disabled: boolean;
  readonly isMenuItem: boolean;
  readonly isListItem: boolean;

  /**
   * Whether or not the element is actively being interacted with by md-list.
   * When active, tabindex is set to 0, and focuses the underlying item.
   */
  get active() {
    // NOTE: md-sub-menu-item changes .active inside its implementation, so the
    // easiest way to keep this.active in sync with md-sub-menu.active is just
    // to create accessors.
    return this.renderRoot.querySelector('md-sub-menu-item')?.active ?? false;
  }

  set active(value: boolean) {
    const item = this.renderRoot.querySelector('md-sub-menu-item');
    if (item) {
      item.active = value;
    }
  }

  /**
   * Sets the item in the selected visual state when a submenu is opened.
   */
  get selected() {
    // NOTE: md-sub-menu-item changes .selected inside its implementation, so
    // the easiest way to keep this.selected in sync with md-sub-menu.selected
    // is just to create accessors.
    return this.renderRoot.querySelector('md-sub-menu-item')?.selected ?? false;
  }

  set selected(value: boolean) {
    const item = this.renderRoot.querySelector('md-sub-menu-item');
    if (item) {
      item.selected = value;
    }
  }

  constructor() {
    super();

    this.headline = '';
    this.disabled = false;
    this.isMenuItem = true;
    this.isListItem = true;
  }

  override render() {
    return html`
      <md-sub-menu-item
          .headline=${this.headline}
          .disabled=${this.disabled}>
      ${chevronIcon}<slot name='submenu' slot='submenu'></slot>
      </md-sub-menu-item>`;
  }

  /**
   * Shows the submenu.
   */
  show() {
    if (this.renderRoot.querySelector('md-sub-menu-item') === null) {
      throw new Error(
          'Called show before menu has rendered, maybe try await updateComplete.');
    }
    this.renderRoot.querySelector('md-sub-menu-item')!.show();
  }

  /**
   * Closes the submenu.
   */
  close() {
    // This close method is required for properly wrapping md-sub-menu-item
    // because md-menu uses it to close nested submenus.
    if (this.renderRoot.querySelector('md-sub-menu-item') === null) {
      throw new Error(
          'Called show before menu has rendered, maybe try await updateComplete.');
    }
    this.renderRoot.querySelector('md-sub-menu-item')!.close();
  }

  // This is just in case `delegatesFocus` doesn't do what is intended (which is
  // unfortunately often across browser updates).
  override focus() {
    this.renderRoot.querySelector('md-sub-menu-item')?.focus();
  }
}

customElements.define('cros-sub-menu-item', SubMenuItem);

declare global {
  interface HTMLElementTagNameMap {
    'cros-sub-menu-item': SubMenuItem;
  }
}
