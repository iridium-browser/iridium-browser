/**
 * @license
 * Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

import {css, CSSResultGroup, html, LitElement, PropertyValues} from 'lit';

const SVG_CONTENT_TYPE = 'image/svg+xml';

function isAbortError(error: unknown): boolean {
  return (error as Error).name === 'AbortError';
}

/** A static illustration for SVG illustrations. */
export class StaticIllustration extends LitElement {
  /** A url to a external svg asset with embedded cros var references. */
  url: string|null;

  /**
   * After we fetch a external svg file from `url` we cache it so we can recolor
   * it without having to repeat the network request.
   */
  private cachedExternalSVG: string|null;
  /**
   * A data url containing our external SVG data after having css vars replaced
   * with real hex codes.
   */
  private coloredExternalSVGDataURL: string|null;
  /**
   * Everytime we make a fetch we increment this token. This allows us to
   * gracefully handle the user changing the url multiple times before the first
   * request has had a chance to complete.
   */
  private currentFetchToken = 0;

  /** AbortController associated with a currently pending fetch request. */
  private abortController: AbortController|null = null;

  /** @nocollapse */
  static override properties = {
    url: {type: String},
    cachedExternalSVG: {type: String},
    coloredExternalSVGDataURL: {type: String}
  };

  /** @nocollapse */
  static override styles: CSSResultGroup = css`
    :host {
      width: 100px;
      height: 100px;
      display: block;
    }
    img {
      width: 100%;
      height: 100%;
    }
  `;

  constructor() {
    super();
    this.url = null;
    this.cachedExternalSVG = null;
    this.coloredExternalSVGDataURL = null;
  }

  // TODO(b/264329425): Bind this to the color scheme change event.
  private recolorExternalAsset() {
    if (!this.cachedExternalSVG) {
      // If the user changed their color theme and we don't have an asset to
      // recolor we can silently exit. This isn't an error state however because
      // we expect this function to be called at any time the OS color theme
      // changes which can be before the element is initialized or when we
      // aren't rendering a external assert.
      return;
    }

    // Replaces references to css variables in the svg source i.e
    //     fill="var(--cros-sys-illo1)"
    //     <style> * { fill: var(--cros-sys-illo1); } </style>
    // With their actual hex values in the current color scheme.
    //     fill="#ff00ff"
    //     <style> * { fill: #ff00ff } </style>
    // This is because when rendering svg in a <img> tag the svg is isolated
    // from the page context and does not have access to any css variables.
    // Note that we also can't render external svg in page since it's untrusted
    // and would pose a XSS risk.
    const resolvedSVG = this.cachedExternalSVG.replaceAll(
        /var\((--[\w\-_\.]+)\)/gi,
        (match, variable) => getComputedStyle(this).getPropertyValue(variable));
    const b64 = btoa(resolvedSVG);
    this.coloredExternalSVGDataURL = `data:${SVG_CONTENT_TYPE};base64,${b64}`;
  }

  private async fetchExternalSVG(url: string) {
    this.currentFetchToken++;
    const currentSession = this.currentFetchToken;

    // Abort a previous pending request.
    if (this.abortController) {
      this.abortController.abort();
      this.abortController = null;
    }
    let response: Response;
    try {
      this.abortController = new AbortController();
      const {signal} = this.abortController;
      response = await fetch(url, {signal});
    } catch (e: unknown) {
      if (isAbortError(e)) {
        // Request must have been aborted. Gracefully exit.
        return;
      }
      // Some other error happened. Rethrow.
      throw e;
    }

    // Check if while we were waiting for our response we sent out another
    // request, if so we ignore this response. Since we abort our previous
    // request this shouldn't ever happen but this catches the situation where
    // a response arrives in the period between us calling abort() and the
    // request actually be aborted.
    // See the dom spec (https://dom.spec.whatwg.org/#interface-abortcontroller)
    // which states that the abort() call will "store reason ... and signal to
    // any observers that the associated acitivity is to be aborted".
    if (this.currentFetchToken > currentSession) {
      return;
    }
    const code = response.status;
    const contentType = response.headers.get('Content-Type');
    if (code !== 200) {
      throw new Error(`cros-static-illustration could not fetch '${url}',
        got response (${code})`);
    } else if (contentType !== SVG_CONTENT_TYPE) {
      throw new Error(
          `cros-static-illustration could not process ${url} as it has a non
        svg content-type (${contentType})`);
    }

    this.cachedExternalSVG = await response.text();
    this.recolorExternalAsset();
    this.dispatchEvent(new CustomEvent('illustration-updated'));
  }

  override updated(changedProperties: PropertyValues<this>) {
    if (changedProperties.has('url') && this.url) {
      this.fetchExternalSVG(this.url);
    }
  }

  override render() {
    if (this.coloredExternalSVGDataURL) {
      return html`
        <img src="${this.coloredExternalSVGDataURL}" />
      `;
    } else {
      // TODO(b/274718543): Allow users to specify static illustrations as
      // templates.
      return html``;
    }
  }
}

customElements.define('cros-static-illustration', StaticIllustration);

declare global {
  interface HTMLElementTagNameMap {
    'cros-static-illustration': StaticIllustration;
  }
}
