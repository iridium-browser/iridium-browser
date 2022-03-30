# Authoring guidelines

*This page talks about how to write individual pages for the site. See
[CONTRIBUTING.md](CONTRIBUTING.md) for how to actually make the changes.*

www.chromium.org is a relatively simple website.

Pages are written in Markdown and translated using a single extremely simple
[Nunjucks](https://mozilla.github.io/nunjucks/)
[template](site/_includes/page.html) into HTML during the build process.

The site uses a single basic [Sass/SCSS](sass-lang.com)
[stylesheet](site/_stylesheets/default.scss)
(using the Node/NPM library version of Sass).

Binary objects (PDFs, images, etc.) are stored in a
[Google Cloud Storage](cloud.google.com/storage) bucket, indexed by
SHA-1 checksums that are committed into this repo. Run
[//scripts/upload_lobs.py](../scripts/upload_lobs.py) to upload things
(you must be a contributor be able to run this script).

## Front matter

The Markdown pages must contain a "front matter" section that can set a few
variables to control aspects of the page appearance. The front matter
must be in the form of a YAML document, and the following variables are
supported:

*   `breadcrumbs`: An optional list of (page_title, link) link pairs to
    parent pages for a given page. If set, they will show up as a
    "breadcrumbs" trail at the top of the page, above the title.
*   `page_name`: The name of the page (usually the name of the enclosing
    directory).
*   `title`: The title of this page. By default, the title will
    be included as an `H1` tag on the page, and is the value that should
    be used in other breadcrumbs lists.
*   `redirect`: To automatically redirect this page/URL to somewhere else,
    set this to a URL.
*   `use_title_as_h1`: If this is set to `true` (the default), the title
    will be included as an H1.

## Naming

Each page must live in its own directory in a file named `index.md`, in
order to match the link structure used by the old Google Sites layout.
Use `words-with-dashes` to form the name of your page.

*Once enough time has passed after the launch, we'll probably relax this
requirement.*

## Style

Please follow the
[Google Markdown style guide](https://github.com/google/styleguide/blob/gh-pages/docguide/style.md).

Pages can embed HTML, but please be careful when doing
so, because we want the site to maintain a consistent look and feel
(which the old site didn't do so much).

You must not use any inline CSS or inline JavaScript. We can support
custom styling and scripts, but doing so requires the approval of the
[//OWNERS](../OWNERS) at this time.

HTML tags may embed Markdown content, but doing so is somewhat finicky.
You should have reliable success if you separate all HTML and Markdown
blocks by blank lines, e.g.:

```md
# Some markdown

<table>
<tr>
<td>

Some *more* markdown

</td>
</tr>
</table>

Yet more markdown.
```

See [/chromium-os/chrome-os-systems-supporting-android-apps](https://new.chromium.org/chromium-os/chrome-os-systems-supporting-android-apps)
for an example.

## Markdown extensions

### Two-column pages

You can display text in two columns using divs with particular classes:

```md
<div class="two-column-container">
<div class="column">

# Some markdown

</div>
<div class="column">

# Some more markdown
</div>
</div>
```

See [/chromium-projects](https://new.chromium.org/chromium-projects)
for an example.

### Tables of contents

If you write `[TOC]` on a line by itself a table of contents will be
generated from the headers in the file and inserted in its place.
The table of contents will include H2-H6 headers, but not the
H1 header (if present).

### Subpages

If you write `{% subpages collections.all %} you will get a hierarchical
tree of links to all of the subpages of your page inserted into the doc.

*Note: the syntax for this is clunky and we hope to replace this with a
proper shortcode like `[TOC]`. Star [crbug.com/1271672](crbug.com/1271672)
to get updates on this.

### Custom IDs and classes

You may customize the `ID` and `class` attributes of block-level elements
as follows:

```
## Header 2 {:#my-header .my-class}
```

will generate:

```
<h2 class="my-class" id="my-header"><a href="#my-header">Header 2</a></h2>
```

This should mostly be used to override the automatically-generated id
for headers, and only sparingly to trigger custom CSS rules on other
elements.

You must use `{:` and `}` as the delimiters around the custom attributes.

## Known issues

*   [crbug.com/1269867](crbug.com/1269867): We should have an auto-formatter
    for the Markdown pages.
*   [crbug.com/1269868](crbug.com/1269868): We should consider using a linter.
*   [crbug.com/1260460](crbug.com/1260460): We should be automatically
    generating the `page_name` and `breadcrumbs` fields, rather than relying
    on authors to set them.
*   [crbug.com/1269860](crbug.com/1269860): We need to document the flavor
    of Markdown that is supported along with any extensions that are enabled.
*   [crbug.com/1267094](crbug.com/1267094): We want a better, more WYSIWYG
    authoring environment.
