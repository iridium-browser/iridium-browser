module.exports = config => {
  config.addWatchTarget('./site/_stylesheets/');

  // `markdown-it` is Eleventy's default Markdown rendering engine.
  // We need a reference to it to customize its behavior, below.
  const md = require('markdown-it');

  // `markdown-it-anchor` is an Eleventy plugin that will add <a> tags to header elements.
  // (this improves the accessibility of linking to headers.)
  const anchor = require('markdown-it-anchor');

  // `uslug` is a Node package that convert text strings into "slugs" in a
  // Unicode-friendly way. (Slugs are the kebab-cased equivalents of text that
  // we use for page names, id's, etc. "Hello world" turns into 'hello-world".
  const uslug = require('uslug');

  // `markdown-it-attrs` is a markdown-it plugin that lets us customize the
  // `id` and `class` attributes of an element in the generated output;
  // we use this mostly for customizing the links in header tags.
  //
  // `markdown-it-toc-done-right` is a markdown-it plugin that adds support
  // for the `[TOC]` mechanism for generating the table of contents in a page.
  let mdlib = md({
    html: true,
  }).use(require('markdown-it-attrs'), {
    leftDelimiter: '{:',
    rightDelimiter: '}',
    allowedAttributes: ['id', 'class'],
  }).use(anchor, {
    slugify: s => uslug(s),
    level: 2,
    permalink: anchor.permalink.headerLink(),
  }).use(require('markdown-it-toc-done-right'), {
    slugify: uslug,
    tocClassName: 'toc',
    tocFirstLevel: 2,
    tocPattern: /\[TOC\]/,
  });

  config.setLibrary('md', mdlib);

  config.addCollection('allSortedByLowerCasedUrl', function(collectionApi) {
    return collectionApi.getAll().sort(function(a, b) {
      let x = a.url.toLowerCase();
      let y = b.url.toLowerCase()
      return (x > y) ? 1 : ((x < y) ? -1 : 0);
    });
  });

  // TODO(crbug.com/1271672): Figure out how to make this syntax and API
  // less clunky.
  const subpages = require('./scripts/subpages.js')
  function handleSubPages(collectionAll) {
    let pageUrl = this.page.url;
    return subpages.render(pageUrl, collectionAll);
  };
  config.addNunjucksShortcode("subpages", handleSubPages);

  // Copy binary assets over to the dist/ directory.

  // This list must be kept in sync with the lists in //.eleventy.js and
  // //scripts/upload_lobs.py.
  // TODO(crbug.com/1457683): Figure out how to share these lists to eliminate
  // the duplication and need to keep them in sync.
  let lob_extensions = [
    '.ai',
    '.bin',
    '.bmp',
    '.brd',
    '.bz2',
    '.crx',
    '.config',
    '.dia',
    '.gif',
    '.graffle',
    '.ico',
    '.jpg',
    'jpg', // Some files are missing the '.' :(.,
    '.jpeg',
    '.mp4',
    '.msi',
    '.pdf',
    'pdf',  // Some files are missing the '.' :(.
    '.png',
    'png',  // Some files are missing the '.' :(.
    '.PNG',
    '.swf',
    '.svg',
    '.tar.gz',
    '.tiff',
    '_trace',
    '.webp',
    '.xcf',
    '.xlsx',
    '.zip',
  ];

  // This should basically pick up everything that isn't a .md file
  // or a .sha1.
  // TODO(crbug.com/1457688): Figure out how to actually enforce this and get
  // rid of the "basically". There has to be a better approach. :).
  let extensions = lob_extensions.concat([
    '.cpp',
    '.css',
    '.csv',
    '.dot',
    '.ebuild',
    '.el',
    '.html',
    '.js',
    '.json',
    'patch',
    '.py',
    '.txt',
    '.xml'
  ]);

  for (let ext of extensions) {
    config.addPassthroughCopy('site/**/*' + ext);
  }

  // Set up the Content-Security-Policy (CSP) hash filter. Every <script>
  // tag must be run through this filter so that the hash of its contents
  // will be in the list of approved scripts in the CSP HTTP header.
  const crypto = require('crypto');
  const fs = require('fs');

  let script_hashes = new Set();
  function cspHash(raw) {
    const c = crypto.createHash('sha256');
    c.update(raw);
    let digest = c.digest('base64');
    script_hashes.add(`'sha256-${digest}'`);
    return raw;
  }

  config.addFilter('cspHash', cspHash);

  // Write out the firebase.json config file once we know which CSP
  // headers to set.
  config.on('afterBuild', () => {
    let script_src = " 'none'";
    if (script_hashes.size > 0) {
      script_src = '';
      for (const script_hash of script_hashes.values()) {
        script_src += ' ' + script_hash;
      }
      script_src += " 'unsafe-inline' 'strict-dynamic'";
    }

    fs.writeFileSync('firebase.json',
      JSON.stringify({
        'hosting': {
          'public': 'build',
          'ignore': [
            'firebase.json',
            '**/.*',
            '**/node_modules/**',
          ],
          'headers': [{
            'source': '**/*',
            'headers': [{
              'key': 'Content-Security-Policy',
              'value':
                "script-src" + script_src +
                "; object-src 'none'; base-uri 'none'; " +
                "report-uri https://csp.withgoogle.com/csp/chromium-website/",
              }],
          }],
        },
      }, null, 2) + '\n');
  });

  // Copy over Algolia files.
  config.addPassthroughCopy({
     'node_modules/@docsearch/js/dist/umd':
       '_scripts/@docsearch',
     'node_modules/@docsearch/css/dist':
       '_stylesheets/@docsearch',
  })

  return {
    dir: {
      input: 'site',
      output: 'build'
    },
    markdownTemplateEngine: 'njk',
    templateFormats: ['md', 'njk'],
    htmlTemplateEngine: 'njk',
    xmlTemplateEngine: 'njk',
  };
};
