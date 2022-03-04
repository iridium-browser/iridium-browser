# Copyright © 2017-2018 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

xsltproc_args = [
  '--stringparam', 'man.authors.section.enabled', '0',
  '--stringparam', 'man.copyright.section.enabled', '0',
  '--stringparam', 'funcsynopsis.style', 'ansi',
  '--stringparam', 'man.output.quietly', '1',
  '--nonet', manpage_style,
]

xmls = [
  ['drm', '7'], ['drm-kms', '7'], ['drm-memory', '7'], ['drmAvailable', '3'],
  ['drmHandleEvent', '3'], ['drmModeGetResources', '3']
]
foreach x : xmls
  m = x[0]
  s = x[1]
  custom_target(
    m,
    input : files('@0@.xml'.format(m)),
    output : '@0@.@1@'.format(m, s),
    command : [prog_xslt, '-o', '@OUTPUT@', xsltproc_args, '@INPUT0@'],
    install : true,
    install_dir : join_paths(get_option('mandir'), 'man@0@'.format(s)),
    build_by_default : true,
  )
endforeach

foreach x : ['drm-mm', 'drm-gem', 'drm-ttm']
  gen = custom_target(
    'gen-@0@'.format(x),
    input : 'drm-memory.xml',
    output : '@0@.xml'.format(x),
    command : [
      prog_sed, '-e', 's@^\.so \([a-z_]\+\)\.\([0-9]\)$$@\.so man\2\/\1\.\2@',
      '@INPUT@',
    ],
    capture : true,
  )
  custom_target(
    '@0@.7'.format(x),
    input : gen,
    output : '@0@.7'.format(x, '7'),
    command : [prog_xslt, '-o', '@OUTPUT@', xsltproc_args, '@INPUT@'],
    install : true,
    install_dir : join_paths(get_option('mandir'), 'man7'),
    build_by_default : true,
  )
endforeach
