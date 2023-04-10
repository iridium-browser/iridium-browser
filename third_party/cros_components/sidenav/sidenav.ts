/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {css, html, LitElement} from 'lit';
import {customElement, query, state} from 'lit/decorators';

import {SidenavItem, TreeItemCollapsedEvent, TreeItemEnabledChangedEvent, TreeItemExpandedEvent} from './sidenav_item';
import {isRTL, isSidenavItem} from './sidenav_util';

/**
 * Allow calling `scrollIntoViewIfNeeded`.
 * go/typescript-reopening
 */
declare interface ExtendedElement extends Element {
  // TODO(b/262453851): Investigate if we can use a standard approved method
  // instead of this. Perhaps a mix of intersection observer and scrollTo.
  scrollIntoViewIfNeeded: (centerIfNeeded: boolean) => void;
}

/**
 * <cros-sidenav> is the container of the <cros-sidenav-item> elements. An
 * example DOM structure is like this:
 *
 * <cros-sidenav>
 *   <cros-sidenav-item>
 *     <cros-sidenav-item></cros-sidenav-item>
 *   </cros-sidenav-item>
 *   <cros-sidenav-item></cros-sidenav-item>
 * </cros-sidenav>
 *
 * The enabling and focus of <cros-sidenav-item> is controlled in
 * <cros-sidenav>, this is because we need to make sure only one item is being
 * enabled or focused.
 */
@customElement('cros-sidenav')
export class Sidenav extends LitElement {
  static get events() {
    return {
      /** Triggers when a tree item has been enabled. */
      SIDENAV_ENABLED_CHANGED: 'sidenav_enabled_changed',
    } as const;
  }

  /** Return the enabled tree item, could be null. */
  get enabledItem(): SidenavItem|null {
    return this.enabledItemInternal;
  }
  set enabledItem(item: SidenavItem|null) {
    this.enableItem(item);
  }

  /** The child tree items. */
  get items(): SidenavItem[] {
    return this.childrenSlot.assignedElements().filter(isSidenavItem);
  }

  /** The child tree items which can be selected, e.g. using the arrow keys. */
  get selectableItems(): SidenavItem[] {
    return this.items.filter(item => !item.disabled);
  }

  /** The default unnamed slot to let consumer pass children tree items. */
  @query('slot') private readonly childrenSlot!: HTMLSlotElement;

  /**
   * Value to set aria-setsize, which is the number of the top level child tree
   * items.
   */
  @state() override ariaSetSize = '0';

  /**
   * Maintain these in the tree level so we can make sure at most one tree item
   * can be enabled/selected.
   */
  private enabledItemInternal: SidenavItem|null = null;

  /**
   * Keep track of which item is selected, so that we know
   *   1. where to move selection when the user navigates with the arrow keys.
   *   2. where to forward focus when the sidenav receives focus.
   */
  private lastSelectedItemInternal: SidenavItem|null = null;

  static override get styles() {
    return css`
      :host {
        display: block;
      }

      ul {
        list-style: none;
        margin: 0;
        padding: 0;
      }
    `;
  }

  override render() {
    return html`
      <ul
        class="tree"
        role="tree"
        aria-setsize=${this.ariaSetSize}
        @click=${this.onTreeClicked}
        @dblclick=${this.onTreeDblClicked}
        @keydown=${this.onTreeKeyDown}
        @sidenav_item_expanded=${this.onSidenavItemExpanded}
        @sidenav_item_collapsed=${this.onSidenavItemCollapsed}
        @sidenav_item_enabled_changed=${this.onSidenavItemEnabledChanged}
      >
        <slot @slotchange=${this.onSlotChanged}></slot>
      </ul>
    `;
  }

  /**
   * If the sidenav receives focus, proxy focus down to the selected sidenav
   * item.
   */
  override focus() {
    // If the user was navigating the sidenav before, focus should return to the
    // last item they selected. If it doesn't exist, fallback to the enabled
    // item, then to the first selectable item.
    if (this.lastSelectedItemInternal) {
      this.lastSelectedItemInternal.focus();
    } else if (this.enabledItem) {
      this.enabledItem.focus();
    } else if (this.selectableItems.length > 0) {
      this.selectableItems[0].focus();
    } else {
      super.focus();
    }
  }

  /**
   * When the children change, make sure the sidenav state is passed on to the
   * new tree structure.
   */
  private onSlotChanged() {
    this.ariaSetSize = this.selectableItems.length.toString();

    // If we had an enabled item but it's not in the children anymore, enable
    // another item.
    if (this.enabledItemInternal && !this.items.some(item => item.enabled)) {
      this.enabledItem = this.selectableItems[0];
    }
  }

  /** Handles the expanded event of the tree item. */
  private onSidenavItemExpanded(e: TreeItemExpandedEvent) {
    const treeItem = e.detail.item;
    (treeItem as unknown as ExtendedElement).scrollIntoViewIfNeeded(false);
  }

  /** Handles the collapse event of the tree item. */
  private onSidenavItemCollapsed(e: TreeItemCollapsedEvent) {
    const collapsedItem = e.detail.item;
    // If the currently selected tree item (`oldSelectedItem`) is a descent of
    // another tree item (`treeItem`) which is going to be collapsed, we need to
    // mark the ancestor tree item (`this`) as selected.
    if (this.lastSelectedItemInternal !== collapsedItem) {
      const oldSelectedItem = this.lastSelectedItemInternal;
      if (oldSelectedItem && collapsedItem.contains(oldSelectedItem)) {
        this.selectItem(collapsedItem);
      }
    }
  }

  /** Handles when that enabled status of an item has changed. */
  private onSidenavItemEnabledChanged(e: TreeItemEnabledChangedEvent) {
    const item = e.detail.item;

    if (item.enabled) {
      item.reveal();
      this.enabledItem = item;
    } else if (this.enabledItem === item) {
      this.enabledItem = null;
    }
  }

  /** Called when the user clicks on a tree item. */
  private async onTreeClicked(e: MouseEvent) {
    // Stop if the the click target is not a tree item.
    const treeItem = e.target as SidenavItem;
    if (treeItem && !isSidenavItem(treeItem)) {
      return;
    }

    if (treeItem.disabled) {
      e.stopImmediatePropagation();
      e.preventDefault();
      return;
    }

    // Use composed path to know which element inside the shadow root
    // has been clicked.
    const innerClickTarget = e.composedPath()[0] as HTMLElement;
    if (innerClickTarget.className === 'expand-icon') {
      treeItem.expanded = !treeItem.expanded;
    } else {
      treeItem.enabled = true;
    }
  }

  /** Called when the user double clicks on a tree item. */
  private async onTreeDblClicked(e: MouseEvent) {
    // Stop if the the click target is not a tree item.
    const treeItem = e.target as SidenavItem;
    if (treeItem && !isSidenavItem(treeItem)) {
      return;
    }

    if (treeItem.disabled) {
      e.stopImmediatePropagation();
      e.preventDefault();
      return;
    }

    // Use composed path to know which element inside the shadow root
    // has been clicked.
    const innerClickTarget = e.composedPath()[0] as HTMLElement;
    if (innerClickTarget.className !== 'expand-icon' &&
        treeItem.hasChildren()) {
      treeItem.expanded = !treeItem.expanded;
    }
  }

  /**
   * Handle the keydown within the tree, this mainly handles the navigation
   * and the selection with the keyboard.
   * TODO(b/262453851): Update this to just do a querySelectorAll for all items
   * instead of using tabbleItems and then maybe just subtract / add index's
   * instead of using this getPrevious/getNext linked list we have.
   */
  private onTreeKeyDown(e: KeyboardEvent) {
    if (e.ctrlKey || e.repeat) {
      return;
    }

    if (!this.lastSelectedItemInternal) {
      return;
    }

    if (this.selectableItems.length === 0) {
      return;
    }

    let itemToSelect: SidenavItem|null|undefined = null;
    switch (e.key) {
      case 'Enter':
      case 'Space':
        this.enableItem(this.lastSelectedItemInternal);
        break;
      case 'ArrowUp':
        itemToSelect = this.getPreviousItem(this.lastSelectedItemInternal);
        break;
      case 'ArrowDown':
        itemToSelect = this.getNextItem(this.lastSelectedItemInternal);
        break;
      case 'ArrowLeft':
      case 'ArrowRight':
        // Don't let back/forward keyboard shortcuts be used.
        if (e.altKey) {
          break;
        }

        const expandKey = isRTL() ? 'ArrowLeft' : 'ArrowRight';
        if (e.key === expandKey) {
          if (this.lastSelectedItemInternal.hasChildren() &&
              !this.lastSelectedItemInternal.expanded) {
            this.lastSelectedItemInternal.expanded = true;
          } else {
            itemToSelect = this.lastSelectedItemInternal.selectableItems[0];
          }
        } else {
          if (this.lastSelectedItemInternal.expanded) {
            this.lastSelectedItemInternal.expanded = false;
          } else {
            itemToSelect = this.lastSelectedItemInternal.parentItem;
          }
        }
        break;
      case 'Home':
        itemToSelect = this.selectableItems[0];
        break;
      case 'End':
        itemToSelect = this.selectableItems[this.selectableItems.length - 1];
        break;
      default:
        break;
    }

    if (itemToSelect) {
      this.selectItem(itemToSelect);
      e.preventDefault();
    }
  }

  /** Helper function that returns the next selectable tree item. */
  private getNextItem(item: SidenavItem): SidenavItem|null {
    if (item.expanded && item.selectableItems.length > 0) {
      return item.selectableItems[0];
    }

    return this.getNextHelper(item);
  }

  /** Another helper function that returns the next selectable tree item. */
  private getNextHelper(item: SidenavItem|null): SidenavItem|null {
    if (!item) {
      return null;
    }

    const nextSibling = item.nextElementSibling as SidenavItem | null;
    if (nextSibling) {
      if (nextSibling.disabled) {
        return this.getNextHelper(nextSibling);
      }
      return nextSibling;
    }
    return this.getNextHelper(item.parentItem);
  }

  /** Helper function that returns the previous selectable tree item. */
  private getPreviousItem(item: SidenavItem): SidenavItem|null {
    let previousSibling = item.previousElementSibling as SidenavItem | null;
    while (previousSibling && previousSibling.disabled) {
      previousSibling =
          previousSibling.previousElementSibling as SidenavItem | null;
    }
    if (previousSibling) {
      return this.getLastHelper(previousSibling);
    }
    return item.parentItem;
  }

  /**
   * Helper function that returns the last selectable tree item in the subtree.
   */
  private getLastHelper(item: SidenavItem|null): SidenavItem|null {
    if (!item) {
      return null;
    }
    if (item.expanded && item.selectableItems.length > 0) {
      const lastChild = item.selectableItems[item.selectableItems.length - 1];
      return this.getLastHelper(lastChild);
    }
    return item;
  }

  /**
   * Make `itemToEnable` become the enabled item in the tree, this will
   * also un-enable the previously enabled tree item to make sure at most
   * one tree item is enabled in the tree.
   */
  private enableItem(itemToEnable: SidenavItem|null) {
    if (itemToEnable === this.enabledItemInternal) {
      return;
    }
    // TODO(b/262453851): We can just "unselect" all items to avoid having to
    // deal with selectedItemInternal here. Then maybe we can remove
    // selectedItemInternal.
    const previousEnabledItem = this.enabledItemInternal;
    if (previousEnabledItem) {
      previousEnabledItem.enabled = false;
    }
    this.enabledItemInternal = itemToEnable;
    if (this.enabledItemInternal) {
      this.enabledItemInternal.enabled = true;
      this.selectItem(this.enabledItemInternal);
      (this.enabledItemInternal as unknown as ExtendedElement)
          .scrollIntoViewIfNeeded(false);
    }
    const enabledChangeEvent: SidenavEnabledChangedEvent =
        new CustomEvent(Sidenav.events.SIDENAV_ENABLED_CHANGED, {
          bubbles: true,
          composed: true,
          detail: {
            previousEnabledItem,
            enabledItem: this.enabledItem,
          },
        });
    this.dispatchEvent(enabledChangeEvent);
  }

  /**
   * Make `itemToSelect` become the selected item in the tree. The previously
   * selected item is unselected. The selected item is the only tabbable item,
   * so that if the user tabs away and returns, they return to the last selected
   * item.
   */
  private selectItem(itemToSelect: SidenavItem) {
    const oldSelectedItem = this.lastSelectedItemInternal;
    // Store what is currently selected so we can restore focus  next time the
    // user navigates to the sidenav.
    this.lastSelectedItemInternal = itemToSelect;

    if (oldSelectedItem) oldSelectedItem.tabIndex = -1;
    this.lastSelectedItemInternal.tabIndex = 0;

    // If the sidenav has focus, move focus to the newly selected item.
    if (this.contains(document.activeElement)) {
      // If `itemToSelect` is a parent of `oldSelectedItem`, we can't move focus
      // without blurring `oldSelectedItem` first.
      if (oldSelectedItem) oldSelectedItem.blur();
      this.lastSelectedItemInternal.focus();
    }
  }
}

/** Type of the tree item enabling custom event. */
export type SidenavEnabledChangedEvent = CustomEvent<{
  /**
   * The tree item which has been enabled previously.
   * TODO(b/262453851): Figure out if we need this event at all, and if we do if
   * we need the prev selected field. In files I don't think this is ever used:
   * https://source.chromium.org/search?q=previousSelectedItem%20f:file_manager&sq=
   */
  previousEnabledItem: SidenavItem | null,
  /** The tree item which has been enabled now. */
  enabledItem: SidenavItem | null,
}>;

declare global {
  interface HTMLElementEventMap {
    [Sidenav.events.SIDENAV_ENABLED_CHANGED]: SidenavEnabledChangedEvent;
  }

  interface HTMLElementTagNameMap {
    'cros-sidenav': Sidenav;
  }
}
