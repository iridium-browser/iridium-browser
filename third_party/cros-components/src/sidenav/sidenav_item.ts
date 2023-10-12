/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import '@material/web/ripple/ripple.js';

import {MdRipple} from '@material/web/ripple/ripple.js';
import {css, CSSResultGroup, html, LitElement, PropertyValues} from 'lit';
import {classMap} from 'lit/directives/class-map';
import {ifDefined} from 'lit/directives/if-defined';
import {styleMap} from 'lit/directives/style-map';

import {castExists} from '../helpers/helpers';

import {isSidenav, isSidenavItem} from './sidenav_util';

/** Type of the tree item expanded custom event. */
export type SidenavItemExpandedEvent = CustomEvent<{
  /** The tree item which has been expanded. */
  item: SidenavItem,
}>;
/** Type of the tree item collapsed custom event. */
export type SidenavItemCollapsedEvent = CustomEvent<{
  /** The tree item which has been collapsed. */
  item: SidenavItem,
}>;
/** Type of the tree item collapsed custom event. */
export type SidenavItemRenamedEvent = CustomEvent<{
  /** The tree item which has been renamed. */
  item: SidenavItem,
  /** The label before rename. */
  oldLabel: string,
  /** The label after rename. */
  newLabel: string,
}>;
/** Type of the tree item enabled changed custom event. */
export type SidenavItemEnabledChangedEvent = CustomEvent<{
  /** The tree item which has been (un)enabled. */
  item: SidenavItem,
  /** The new enabled value. */
  enabled: boolean,
}>;

/** The number of pixels to indent per level. */
export const TREE_ITEM_INDENT_PX = 20;

/** The width and height of the expand and label icons. */
const ICON_SIZE = css`20px`;

/**
 * Spacing between items in the a row (in LTR):
 *
 * In flat Sidenavs:
 *
 *   1. [ICON_LEADING_MARGIN]
 *      <icon>
 *      [ICON_LABEL_GAP]
 *      <label>
 *      [LABEL_TRAILING_MARGIN]
 *
 *   2. [LABEL_ONLY_MARGIN]
 *      <label>
 *      [LABEL_TRAILING_MARGIN]
 *
 * In layered sidenavs:
 * (the expand icon takes space even when a layered item has no children)
 *
 *   3. [EXPAND_LEADING_MARGIN]
 *      <expand icon>
 *      <icon>
 *      [ICON_LABEL_GAP]
 *      <label>
 *      [LABEL_TRAILING_MARGIN]
 *
 *   4. [EXPAND_LEADING_MARGIN]
 *      <expand icon>
 *      [EXPAND_LABEL_GAP]
 *      <label>
 *      [LABEL_TRAILING_MARGIN]
 */
const ICON_LABEL_GAP = css`8px`;
const EXPAND_LABEL_GAP = css`4px`;
const ICON_LEADING_MARGIN = css`28px`;
const EXPAND_LEADING_MARGIN = css`8px`;
const LABEL_ONLY_MARGIN = css`24px`;
const LABEL_TRAILING_MARGIN = css`12px`;
const ITEM_GAP = css`8px`;

/** Selects a host that is in a layered Sidenav. */
const LAYERED_SELECTOR = css`:host(:is([inLayered], [may-have-children]))`;

/** An item for a ChromeOS compliant sidenav element. */
export class SidenavItem extends LitElement {
  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      display: block;
    }

    li {
      display: block;
      font: var(--cros-button-2-font);
    }

    li:focus-visible {
      outline: none;
    }

    :host([separator])::before {
      border-bottom: 1px solid var(--cros-separator-color);
      content: '';
      display: block;
      margin: ${ITEM_GAP} 0;
      width: 100%;
    }

    .tree-row,
    .md-ripple {
      align-items: center;
      border-inline-start-width: 0 !important;
      border-radius: 20px;
      box-sizing: border-box;
      color: var(--cros-sys-on_surface);
      cursor: pointer;
      display: flex;
      height: 40px;
      margin-top: ${ITEM_GAP};
      position: relative;
      user-select: none;
      white-space: nowrap;
    }

    li:focus-visible .tree-row {
      outline: 2px solid var(--cros-sys-focus_ring);
      outline-offset: 2px;
      z-index: 2;
    }

    :host([disabled]) .tree-row {
      pointer-events: none;
      color: var(--cros-sys-disabled);
    }

    :host([enabled]) .tree-row {
      background-color: var(--cros-sys-primary);
      color: var(--cros-sys-on_primary);
    }

    :host([error]) .tree-row {
      background-color: var(--cros-sys-error_container);
      color: var(--cros-sys-on_error_container);
    }

    :host-context(.pointer-active) .tree-row:not(:active) {
      cursor: default;
    }

    .expand-icon {
      display: none;
      fill: currentcolor;
      flex: none;
      height: ${ICON_SIZE};
      -webkit-mask-position: center;
      -webkit-mask-repeat: no-repeat;
      position: relative;
      transform: rotate(-90deg);
      transition: all 150ms;
      visibility: hidden;
      width: ${ICON_SIZE};
    }

    ${LAYERED_SELECTOR} .expand-icon {
      display: unset;
    }

    li[aria-expanded] .expand-icon {
      visibility: visible;
    }

    .expand-icon,
    slot[name="icon"]::slotted(*) {
      width: 20px;
      height: 20px;
      align-items: center;
    }

    :host-context([dir=rtl]) .expand-icon {
      transform: rotate(90deg);
    }

    :host([expanded]) .expand-icon {
      transform: rotate(0);
    }

    slot[name="icon"]::slotted(*) {
      color: var(--cros-sys-on_surface);
      flex: none;
      height: ${ICON_SIZE};
      margin-inline-start: ${ICON_LEADING_MARGIN};
      width: ${ICON_SIZE};
    }

    :host([enabled]) slot[name="icon"]::slotted(*) {
      color: var(--cros-sys-on_primary)
    }

    :host([disabled]) slot[name="icon"]::slotted(*) {
      color: var(--cros-sys-disabled);
    }

    :host([error]) slot[name="icon"]::slotted(*) {
      color: var(--cros-sys-on_error_container);
    }

    .tree-label {
      display: block;
      flex: auto;
      margin-inline-end: ${LABEL_TRAILING_MARGIN};
      margin-inline-start: ${LABEL_ONLY_MARGIN};
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: pre;
    }

    ${LAYERED_SELECTOR} .expand-icon {
      margin-inline-start: ${EXPAND_LEADING_MARGIN}
    }

    ${LAYERED_SELECTOR} slot[name="icon"]::slotted(*) {
      margin-inline-start: 0px;
      margin-inline-end: 0px;
    }

    ${LAYERED_SELECTOR} .tree-label {
      margin-inline-start: ${EXPAND_LABEL_GAP};
    }

    ${LAYERED_SELECTOR} .has-icon .tree-label,
    li[aria-expanded] .tree-label,
    .has-icon + .tree-label, {
      margin-inline-start: ${ICON_LABEL_GAP};
    }

    .rename {
      background-color: var(--cros-sys-app_base);
      border: none;
      border-radius: 4px;
      color: var(--cros-sys-on_surface);
      height: 20px;
      margin-inline-end: ${LABEL_TRAILING_MARGIN};
      margin-inline-start: ${LABEL_ONLY_MARGIN};
      outline: 2px solid var(--cros-sys-focus_ring);
      overflow: hidden;
      font: var(--cros-button-2-font);
    }

    .has-icon :is(.tree-label, .rename) {
      margin-inline-start: ${ICON_LABEL_GAP};
    }

    ${LAYERED_SELECTOR} .rename {
      margin-inline-start: ${EXPAND_LABEL_GAP};
    }

    ${LAYERED_SELECTOR} .has-icon :is(.tree-label, .rename) {
      margin-inline-start: ${ICON_LABEL_GAP};
    }

    /* We need to ensure that even empty labels take up space */
    .tree-label:empty::after {
      content: ' ';
      white-space: pre;
    }

    ul.tree-children {
      height: 0px;
      list-style: none;
      margin: 0 0 ${ITEM_GAP} 0;
      outline: none;
      overflow: hidden;
      padding: 0;
    }

    :host([expanded]) ul.tree-children {
      display: block;
      height: auto;
      margin: 0;
      overflow: visible;
    }

    :host([enabled]) .rename {
      outline: 2px solid var(--cros-sys-inverse_primary);
    }

    .rename::selection {
      background-color: var(--cros-sys-highlight_text)
    }

    md-ripple {
      border-radius: 20px;
    }

    :host(:not([renaming])) md-ripple {
      color: var(--cros-sys-ripple_primary);
      --md-ripple-hover-color: var(--cros-sys-hover_on_subtle);
      --md-ripple-hover-opacity: 100%;
      --md-ripple-pressed-color: var(--cros-sys-ripple_primary);
      --md-ripple-pressed-opacity: 100%;
    }
  `;
  /** @nocollapse */
  static override shadowRootOptions = {
    ...LitElement.shadowRootOptions,
    delegatesFocus: true
  };
  /** @nocollapse */
  static override properties = {
    separator: {type: Boolean, reflect: true},
    disabled: {type: Boolean, reflect: true},
    enabled: {type: Boolean, reflect: true},
    expanded: {type: Boolean, reflect: true},
    renaming: {type: Boolean, reflect: true},
    error: {type: Boolean, reflect: true},
    mayHaveChildren:
        {type: Boolean, reflect: true, attribute: 'may-have-children'},
    label: {type: String, reflect: true},
    tabIndex: {attribute: false},
    layer: {type: Number, reflect: true},
    inLayered: {type: Boolean, reflect: true},
    hasIcon: {type: Boolean, reflect: true},
  };

  /** @nocollapse */
  static get events() {
    return {
      /** Triggers when a sidenav item has been expanded. */
      SIDENAV_ITEM_EXPANDED: 'sidenav_item_expanded',
      /** Triggers when a sidenav item has been collapsed. */
      SIDENAV_ITEM_COLLAPSED: 'sidenav_item_collapsed',
      /** Triggers when a sidenav item's label has been renamed. */
      SIDENAV_ITEM_RENAMED: 'sidenav_item_renamed',
      /** Triggers when a sidenav item's changed status changed. */
      SIDENAV_ITEM_ENABLED_CHANGED: 'sidenav_item_enabled_changed',
    } as const;
  }

  /**
   * The `separator` attribute will show a top border for the tree item. It's
   * mainly used to identify this tree item is a start of the new section.
   * @export
   */
  separator: boolean;
  /**
   * Indicate if a tree item is disabled or not. Disabled tree item will have
   * a greyed out color, can't be enabled, can't get focus. It can still have
   * children, but it can't be expanded, and the expand icon will be hidden.
   * @export
   */
  disabled: boolean;
  /**
   * Indicate if a tree item has been enabled or not.
   * @export
   */
  enabled: boolean;
  /**
   * Indicate if a tree item has been expanded or not.
   * @export
   */
  expanded: boolean;
  /**
   * Indicate if a tree item is in renaming mode or not.
   * @export
   */
  renaming: boolean;
  /**
   * Indicate the item contains an error.
   * @export
   */
  error: boolean;
  /**
   * A tree item will have children if the child tree items have been inserted
   * into its default slot. Only use `mayHaveChildren` if we want the tree item
   * to appear as having children even without the actual child tree items
   * (e.g. no DOM children). This is mainly used when we asynchronously load
   * child tree items.
   * @export
   */
  mayHaveChildren: boolean;
  /**
   * The label text of the tree item.
   * @export
   */
  label: string;
  /**
   * If the user navigates to this item, tabs away from the sidenav and then
   * tabs back into the sidenav, the item that receives focus should be the one
   * they had navigated to. `tabIndex` is forwarded to the ` < li > ` element.
   * @export
   */
  override tabIndex: number;
  /**
   * Indicate the depth of this tree item, we use it to calculate the padding
   * indentation. Note: "aria-level" can be calculated by DOM structure so
   * no need to provide it explicitly.
   * @export
   */
  layer: number;

  /**
   * Whether an icon has been slotted. CSS cannot distinguish slots that have
   * slotted items, so we check manually and set a class.
   * @export
   */
  hasIcon: boolean;

  /**
   * Whether this item is in a Sidenav with nested SidenavItems. If this
   * SidenavItem has a Sidenav parent, the parent will control this attribute.
   * @export
   */
  inLayered: boolean;

  private get treeItemElement(): HTMLLIElement {
    return castExists(this.renderRoot.querySelector('li'));
  }

  private get renameInputElement(): HTMLInputElement {
    return castExists(
        this.renderRoot.querySelector('.rename') as HTMLInputElement);
  }

  private get childrenSlotElement(): HTMLSlotElement {
    return castExists(
        this.renderRoot.querySelector('slot:not([name])') as HTMLSlotElement);
  }

  private get iconSlotElement(): HTMLSlotElement {
    return castExists(
        this.renderRoot.querySelector('slot[name="icon"]') as HTMLSlotElement);
  }

  protected get ripple(): Promise<MdRipple|null> {
    // We can't use async / await here as js does not support async getters
    // and setters.
    return this.updateComplete.then(() => {
      return this.renderRoot.querySelector('md-ripple');
    });
  }

  constructor() {
    super();
    this.separator = false;
    this.disabled = false;
    this.enabled = false;
    this.expanded = false;
    this.renaming = false;
    this.error = false;
    this.mayHaveChildren = false;
    this.label = '';
    this.tabIndex = -1;
    this.layer = 0;
    this.hasIcon = false;
    this.inLayered = false;
  }

  /** The child tree items which can be tabbed to. */
  get selectableItems(): SidenavItem[] {
    return this.items.filter(item => !item.disabled);
  }

  hasChildren(): boolean {
    return this.mayHaveChildren || this.items.length > 0;
  }

  /**
   * Return the parent SidenavItem, if there is one. For top level SidenavItems
   * with no parent, return null.
   */
  get parentItem(): SidenavItem|null {
    let p = this.parentElement;
    while (p) {
      if (isSidenavItem(p)) {
        return p;
      }
      if (isSidenav(p)) {
        return null;
      }
      p = p.parentElement;
    }
    return p;
  }

  /** Expands all parent items. */
  reveal() {
    let pi = this.parentItem;
    while (pi) {
      pi.expanded = true;
      pi = pi.parentItem;
    }
  }

  /**
   * The child tree items.
   * TODO(b/262453851): Should be private, and accessible to tests.
   */
  items: SidenavItem[] = [];

  /** Indicate if we should commit the rename on input blur or not. */
  private shouldRenameOnBlur = true;

  override render() {
    const showExpandIcon = this.hasChildren();
    const treeRowStyles = {
      /** @export */
      paddingInlineStart:
          `max(0px, calc(${TREE_ITEM_INDENT_PX} * ${this.layer}px))`,
    };

    const treeRowClasses = {'tree-row': true, 'has-icon': this.hasIcon};

    return html`
      <li
          class="tree-item"
          role="treeitem"
          tabindex=${this.tabIndex}
          aria-labelledby="tree-label"
          aria-selected=${this.enabled}
          aria-expanded=${ifDefined(showExpandIcon ? this.expanded : undefined)}
          aria-disabled=${this.disabled}>
        <div
            class=${classMap(treeRowClasses)}
            style=${styleMap(treeRowStyles)}>
          <md-ripple></md-ripple>
          <!-- TODO(b/262453851): Implement icon from spec -->
          <span
              class="expand-icon"
              @click=${this.onExpandIconClicked}>
            <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 20 20">
              <polyline points="6 8 10 12 14 8 6 8"/>
            </svg>
          </span>
          <slot
              name="icon"
              class=${this.hasIcon ? 'has-icon' : ''}
              @slotchange=${this.onIconSlotChanged}>
          </slot>
          ${this.renderTreeLabel()}
        </div>
        <ul
            class="tree-children"
            role="group">
          <slot @slotchange=${this.onSlotChanged}></slot>
        </ul>
       </li>
     `;
  }

  override firstUpdated() {
    this.addEventListener('click', this.onRowClicked);
  }

  override updated(changedProperties: PropertyValues<this>) {
    super.updated(changedProperties);
    if (changedProperties.has('expanded')) {
      this.onExpandChanged();
    }
    if (changedProperties.has('enabled')) {
      this.onEnabledChanged();
    }
    if (changedProperties.has('renaming')) {
      this.onRenamingChanged();
    }
  }

  private renderTreeLabel() {
    if (this.renaming) {
      // Stop propagation of some events to prevent them being captured by
      // tree when the tree item is in renaming mode.
      const interceptPropagation = (e: MouseEvent) => {
        e.stopPropagation();
      };
      return html`
        <input
          class="rename"
          type="text"
          spellcheck="false"
          .value=${this.label}
          @click=${interceptPropagation}
          @dblclick=${interceptPropagation}
          @mouseup=${interceptPropagation}
          @mousedown=${interceptPropagation}
          @blur=${this.onRenameInputBlur}
          @keydown=${this.onRenameInputKeydown}
        />
      `;
    }
    return html`
    <span
      class="tree-label"
      id="tree-label"
    >${this.label || ''}</span>`;
  }

  private onIconSlotChanged() {
    this.hasIcon = this.iconSlotElement.assignedElements().length > 0;
    this.iconSlotElement.assignedElements().forEach(icon => {
      icon.setAttribute('tabIndex', '-1');
    });
  }

  // TODO(b/262453851): There's probably a nicer way to implement this.
  private onSlotChanged() {
    // Whether the old set of children contained the enabled item.
    const oldItemsContainEnabledItem =
        this.items.some((item: SidenavItem) => item.enabled);
    // Update `items` every time when the children slot changes (e.g.
    // add/remove).
    this.items =
        this.childrenSlotElement.assignedElements().filter(isSidenavItem);

    let updateScheduled = false;

    // If an expanded item's last children is deleted, update expanded property.
    if (this.items.length === 0 && this.expanded) {
      this.expanded = false;
      updateScheduled = true;
    }

    if (oldItemsContainEnabledItem &&
        !this.items.some((item: SidenavItem) => item.enabled)) {
      // If the currently enabled item was in the old set of children but not in
      // the new set, it means it's being removed from the children slot. The
      // parent becomes the enabled item.
      this.enabled = true;
      updateScheduled = true;
    }

    for (const item of this.items) {
      item.layer = this.layer + 1;
    }

    // Explicitly trigger an update because render() relies on hasChildren().
    if (!updateScheduled) {
      this.requestUpdate();
    }
  }

  private onExpandChanged() {
    if (this.expanded) {
      const expandedEvent: SidenavItemExpandedEvent =
          new CustomEvent(SidenavItem.events.SIDENAV_ITEM_EXPANDED, {
            bubbles: true,
            composed: true,
            detail: {item: this},
          });
      this.dispatchEvent(expandedEvent);
    } else {
      const collapseEvent: SidenavItemCollapsedEvent =
          new CustomEvent(SidenavItem.events.SIDENAV_ITEM_COLLAPSED, {
            bubbles: true,
            composed: true,
            detail: {item: this},
          });
      this.dispatchEvent(collapseEvent);
    }
  }

  private onEnabledChanged() {
    const event: SidenavItemEnabledChangedEvent =
        new CustomEvent(SidenavItem.events.SIDENAV_ITEM_ENABLED_CHANGED, {
          bubbles: true,
          composed: true,
          detail: {item: this, enabled: this.enabled},
        });
    this.dispatchEvent(event);
  }

  private onExpandIconClicked(e: MouseEvent) {
    e.stopPropagation();
    if (this.disabled) return;
    this.expanded = !this.expanded;
  }

  private onRowClicked(e: MouseEvent) {
    // Prevent the parent `SidenavItem` from acting on this event.
    e.stopPropagation();
    if (this.disabled) return;
    this.enabled = true;
  }

  private onRenamingChanged() {
    if (this.renaming) {
      this.renameInputElement?.focus();
      this.renameInputElement?.select();
    }
  }

  private onRenameInputKeydown(e: KeyboardEvent) {
    // Make sure that the tree does not handle the key.
    e.stopPropagation();

    if (e.repeat) {
      return;
    }

    // Calling this.focus() blurs the input which will make the tree item
    // non editable.
    switch (e.key) {
      case 'Escape':
        // By default blur() will trigger the rename, but when ESC is pressed
        // we don't want the blur() (triggered by treeItemElement.focus() below)
        // to commit the rename.
        this.shouldRenameOnBlur = false;
        this.treeItemElement.focus();
        e.preventDefault();
        break;
      case 'Enter':
        // treeItemElement.focus() will trigger blur() for the rename input
        // which will commit the rename.
        this.treeItemElement.focus();
        e.preventDefault();
        break;
      default:
        break;
    }
  }

  private onRenameInputBlur() {
    this.renaming = false;
    if (this.shouldRenameOnBlur) {
      this.commitRename(this.renameInputElement?.value || '');
    } else {
      this.shouldRenameOnBlur = true;
    }
  }

  private commitRename(newName: string) {
    const isEmpty = newName.trim() === '';
    const isChanged = newName !== this.label;
    if (isEmpty || !isChanged) {
      return;
    }
    const oldLabel = this.label;
    this.label = newName;
    const renameEvent: SidenavItemRenamedEvent =
        new CustomEvent(SidenavItem.events.SIDENAV_ITEM_RENAMED, {
          bubbles: true,
          composed: true,
          detail: {item: this, oldLabel, newLabel: newName},
        });
    this.dispatchEvent(renameEvent);
  }
}

customElements.define('cros-sidenav-item', SidenavItem);

declare global {
  interface HTMLElementEventMap {
    [SidenavItem.events.SIDENAV_ITEM_EXPANDED]: SidenavItemExpandedEvent;
    [SidenavItem.events.SIDENAV_ITEM_COLLAPSED]: SidenavItemCollapsedEvent;
    [SidenavItem.events.SIDENAV_ITEM_RENAMED]: SidenavItemRenamedEvent;
    [SidenavItem.events.SIDENAV_ITEM_ENABLED_CHANGED]:
        SidenavItemEnabledChangedEvent;
  }

  interface HTMLElementTagNameMap {
    'cros-sidenav-item': SidenavItem;
  }
}
