/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {css, html, LitElement} from 'lit';
import {customElement, query, state} from 'lit/decorators';

import {SidenavItem, TreeItemCollapsedEvent, TreeItemExpandedEvent} from './sidenav_item';
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
    return this.itemsInternal;
  }

  /** The child tree items which can be tabbed/focused into. */
  get tabbableItems(): SidenavItem[] {
    return this.itemsInternal.filter(item => !item.disabled);
  }

  /** The default unnamed slot to let consumer pass children tree items. */
  @query('slot') private readonly childrenSlot!: HTMLSlotElement;

  /**
   * Value to set aria-setsize, which is the number of the top level child tree
   * items.
   */
  @state() override ariaSetSize = '0';

  /** The child tree items. */
  private itemsInternal: SidenavItem[] = [];
  /**
   * Maintain these in the tree level so we can make sure at most one tree item
   * can be enabled/focused.
   */
  private enabledItemInternal: SidenavItem|null = null;
  private focusedItemInternal: SidenavItem|null = null;

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
      >
        <slot @slotchange=${this.onSlotChanged}></slot>
      </ul>
    `;
  }

  // TODO(b/262453851): See if we can remove the internal copy of the dom we
  // have (tabbleItems, itemInternals) and instead lazy query the DOM when we
  // need items. This setup is more prone to weird lit bugs and the cost of
  // querying the dom is low.
  private onSlotChanged() {
    const oldItems = new Set(this.itemsInternal);
    // Update `itemsInternal` every time when the children slot changes (e.g.
    // add/remove).
    this.itemsInternal =
        this.childrenSlot.assignedElements().filter(isSidenavItem);
    this.ariaSetSize = this.tabbableItems.length.toString();

    if (this.enabledItemInternal) {
      const newItems = new Set(this.itemsInternal);
      if (oldItems.has(this.enabledItemInternal) &&
          !newItems.has(this.enabledItemInternal)) {
        // If the currently enabled item exists in `oldItems` but not in
        // `newItems`, it means it's being removed from the children slot,
        // we need to enable the first .
        this.enabledItem = this.tabbableItems[0];
      }
    }
  }

  /** Handles the expanded event of the tree item. */
  private onSidenavItemExpanded(e: TreeItemExpandedEvent) {
    const treeItem = e.detail.item;
    (treeItem as unknown as ExtendedElement).scrollIntoViewIfNeeded(false);
  }

  /** Handles the collapse event of the tree item. */
  private onSidenavItemCollapsed(e: TreeItemCollapsedEvent) {
    const treeItem = e.detail.item;
    // If the currently focused tree item (`oldFocusedItem`) is a descent of
    // another tree item (`treeItem`) which is going to be collapsed, we need to
    // mark the ancestor tree item (`this`) as focused.
    if (this.focusedItemInternal !== treeItem) {
      const oldFocusedItem = this.focusedItemInternal;
      if (oldFocusedItem && treeItem.contains(oldFocusedItem)) {
        this.focusItem(treeItem);
      }
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

    if (!this.focusedItemInternal) {
      return;
    }

    if (this.tabbableItems.length === 0) {
      return;
    }

    let itemToFocus: SidenavItem|null|undefined = null;
    switch (e.key) {
      case 'Enter':
      case 'Space':
        this.enableItem(this.focusedItemInternal);
        break;
      case 'ArrowUp':
        itemToFocus = this.getPreviousItem(this.focusedItemInternal);
        break;
      case 'ArrowDown':
        itemToFocus = this.getNextItem(this.focusedItemInternal);
        break;
      case 'ArrowLeft':
      case 'ArrowRight':
        // Don't let back/forward keyboard shortcuts be used.
        if (e.altKey) {
          break;
        }

        const expandKey = isRTL() ? 'ArrowLeft' : 'ArrowRight';
        if (e.key === expandKey) {
          if (this.focusedItemInternal.hasChildren() &&
              !this.focusedItemInternal.expanded) {
            this.focusedItemInternal.expanded = true;
          } else {
            itemToFocus = this.focusedItemInternal.tabbableItems[0];
          }
        } else {
          if (this.focusedItemInternal.expanded) {
            this.focusedItemInternal.expanded = false;
          } else {
            itemToFocus = this.focusedItemInternal.parentItem;
          }
        }
        break;
      case 'Home':
        itemToFocus = this.tabbableItems[0];
        break;
      case 'End':
        itemToFocus = this.tabbableItems[this.tabbableItems.length - 1];
        break;
      default:
        break;
    }

    if (itemToFocus) {
      this.focusItem(itemToFocus);
      e.preventDefault();
    }
  }

  /** Helper function that returns the next tabbable tree item. */
  private getNextItem(item: SidenavItem): SidenavItem|null {
    if (item.expanded && item.tabbableItems.length > 0) {
      return item.tabbableItems[0];
    }

    return this.getNextHelper(item);
  }

  /** Another helper function that returns the next tabbable tree item. */
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

  /** Helper function that returns the previous tabbable tree item. */
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
   * Helper function that returns the last tabbable tree item in the subtree.
   */
  private getLastHelper(item: SidenavItem|null): SidenavItem|null {
    if (!item) {
      return null;
    }
    if (item.expanded && item.tabbableItems.length > 0) {
      const lastChild = item.tabbableItems[item.tabbableItems.length - 1];
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
      this.focusItem(this.enabledItemInternal);
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
   * Make `itemToEnable` become the focused item in the tree, this will
   * also unfocus the previously focused tree item to make sure at most
   * one tree item is selected in the tree. The focused item is the only
   * tabbable item, so that if the user tabs away and returns, they return to
   * the last focused item.
   */
  private focusItem(itemToFocus: SidenavItem) {
    const previousFocusedItem = this.focusedItemInternal;
    if (previousFocusedItem) {
      // TODO(b/262453851): confirm this is needed. It should be implicit when
      // you focus a new thing that the old thing gets blurred.
      previousFocusedItem.blur();
      previousFocusedItem.tabIndex = -1;
    }
    this.focusedItemInternal = itemToFocus;
    this.focusedItemInternal.focus();
    this.focusedItemInternal.tabIndex = 0;
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
