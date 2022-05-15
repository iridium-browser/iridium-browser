#!/usr/bin/env vpython3
# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Export www.chromium.org to local files.

This script uses the Google GData and Google Sites APIs to extract the
content from http://www.chromium.org/ and write it into local files
that can be used to serve the same content.

The APIs are documented at

https://developers.google.com/sites/docs/1.0/developers_guide_protocol
https://developers.google.com/gdata/docs/json

Because www.chromium.org is a public site, this script requires no
authentication to work.

The exporting process attempts to convert the original content into
sane modern HTML as much as possible without changing the appearance
of any page significantly, with some minor exceptions.
"""

import argparse
import collections
import io
import json
import os
import pdb
import sys
import time
import traceback
import xml.etree.ElementTree as ET

from urllib.parse import urlparse
from urllib.request import urlopen
from urllib.error import HTTPError, URLError

import yaml

import common
import html2markdown


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--force', action='store_true',
                        help='ignore updated timestamps in local cache')
    parser.add_argument('-j', '--jobs', type=int, default=common.cpu_count())
    parser.add_argument('-t', '--test', action='store_true')
    parser.add_argument('-r', '--raw', action='store_true')
    parser.add_argument('-v', '--verbose', action='count')
    parser.add_argument('--max_results', type=int, default=5000)
    parser.add_argument('--start-index', type=int, default=1)
    parser.add_argument('--paths-to-skip')
    parser.add_argument('--path-list')
    parser.add_argument('path', nargs='*')
    args = parser.parse_args()

    entries = _entries(args)

    if args.path:
        paths_to_export = ['%s%s' % ('/' if not path.startswith('/') else '',
                                     path)
                           for path in args.path]
    elif args.path_list:
        paths_to_export = common.read_paths(args.path_list)
    else:
        paths_to_export = []

    if args.paths_to_skip:
        paths_to_skip = set(common.read_paths(args.paths_to_skip))
    else:
        paths_to_skip = set(
            common.read_paths(os.path.join(common.REPO_DIR,
                                           'scripts', 'paths_to_skip.txt')))

    max_input_mtime = max(os.stat(__file__).st_mtime,
                          os.stat(common.__file__).st_mtime,
                          os.stat(html2markdown.__file__).st_mtime)

    updated = 0
    paths = []

    if args.test:
        entry = _find_entry_by_path(paths_to_export[0], entries)
        if entry:
            metadata = _metadata(entry, entries)
            path = _path(entry, entries)
            _ = _handle_entry(path,
                              (entry, metadata, max_input_mtime, args.force,
                               args.raw))
            content = common.read_text_file('%s%s/index.md' %
                                            (common.SITE_DIR, path))
            print(content)
            return 0
        else:
            print('%s not found' % paths_to_export[0])
            return 1

    q = common.JobQueue(_handle_entry, args.jobs)

    paths_to_export = set(paths_to_export)
    exported_pages = set()
    for i, entry in enumerate(list(entries.values())[:args.max_results]):
        if entry['kind'] in ('webpage', 'listpage',
                             'announcementspage', 'filecabinet'):
            metadata = _metadata(entry, entries)
            path = _path(entry, entries)

            if path in paths_to_skip:
                continue
            exported_pages.add(path)
        elif entry['kind'] == 'attachment':
            metadata = {}
            path = entry['url'].replace(
                 'https://sites.google.com/a/chromium.org/dev/', '/').rstrip('/')
            if path in paths_to_skip:
                continue
        else:
            continue
        if not paths_to_export or (path in paths_to_export):
            q.request(path, (entry, metadata, max_input_mtime, args.force,
                             False))

    ret = 0
    for path, res, did_update in q.results():
        if res:
            ret = 1
        if did_update:
            updated += 1

    print('updated %d entries' % updated)
    return ret


def _find_entry_by_path(path, entries):
    for entry in entries.values():
        if entry['kind'] not in ('webpage', 'listpage',
                                 'announcmentspage', 'filecabinet'):
          continue
        entry_path = _path(entry, entries)
        if entry_path == path:
          return entry
    return None


def _handle_entry(task, obj):
    entry, metadata, max_input_mtime, force, raw = obj
    err = ''
    did_update = False

    if not task.startswith('/'):
        return 'malformed task', False

    yaml.SafeDumper.org_represent_str = yaml.SafeDumper.represent_str

    if task in (
        '/developers/jinja',
        '/developers/polymer-1-0',
        '/devtools/breakpoints-tutorial/index.html',
        '/devtools/breakpoints-tutorial/script.js',
        ):
        # TODO: Eleventy chokes on these files.
        return '', False

    def repr_str(dumper, data):
        if '\n' in data:
            return dumper.represent_scalar(u'tag:yaml.org,2002:str', data,
                                           style='|')
        return dumper.org_represent_str(data)

    yaml.add_representer(str, repr_str, Dumper=yaml.SafeDumper)


    mtime = _to_ts(entry['updated'])
    target_mtime = max(mtime, max_input_mtime)
    if entry['kind'] in ('webpage',
                         'listpage',
                         'announcementspage',
                         'filecabinet'):
        path = '%s%s/%s' % (common.SITE_DIR, task, 'index.md')
        if _needs_update(path, target_mtime, force):
            if raw:
                content = entry['content']
            else:
                content_sio = io.StringIO(entry['content'])
                md_sio = io.StringIO()
                md_sio.write('---\n')
                md_sio.write(yaml.safe_dump(metadata))
                md_sio.write('---\n\n')
                url_converter = _URLConverter()
                html2markdown.Convert(content_sio, md_sio, url_converter)
                if entry['kind'] == 'listpage':
                    md_sio.write('\n\n')
                    _write_listitems(md_sio, entry)
                content = md_sio.getvalue()
                content = content.replace(
                    'chromium.googlesource.com/chromium/src/+/master/',
                    'chromium.googlesource.com/chromium/src/+/HEAD/')
                content = content.replace('    \b\b\b\b', '')

            did_update = common.write_if_changed(path, content, mode='w')
        else:
            did_update = False
    elif entry['kind'] == 'listitem':
        # Handled as part of the corresponding 'listpage' entry.
        pass
    elif entry['kind'] == 'announcement':
        # TODO: implement me.
        pass
    elif entry['kind'] == 'attachment':
        path = '%s%s' % (common.SITE_DIR, task)
        path = path.replace(':', '_')
        path = path.replace('%20', ' ')
        path = path.replace('%2B', '+')
        if task in (
            '/developers/design-documents/network-stack/cookiemonster/CM-method-calls-new.png',
            '/developers/design-documents/cookie-split-loading/objects.png',
        ):
            # These are expected 404's that we ignore.
            did_update = False
        elif _needs_update(path, mtime, force):
            try:
                fp = urlopen(entry['url'])
                content = fp.read()
                did_update = common.write_if_changed(path, content)
            except (HTTPError, URLError, TimeoutError) as e:
                err = 'Error: %s' % e

    elif entry['kind'] == 'comment':
        # ignore comments in the migration
        pass
    elif entry['kind'] == 'tag':
        err = 'tag kind not implemented'
    else:
        err = 'unknown kind %s' % entry['kind']

    return err, did_update


def _write_listitems(content, entry):
    if not entry['listitems']:
        return

    headers = entry['listitems'][0].keys()
    rows = sorted(entry['listitems'],
                  key=lambda row: row.get('Release') or '')

    content.write('<table>\n')
    content.write('  <tr>\n')
    for header in headers:
        content.write('    <th>%s</th>\n' % header)
    content.write('  </tr>\n')
    for row in rows:
        content.write('  <tr>\n')
        for value in row.values():
            if value and value.startswith('<a xmlns='):
                value = value.replace(' xmlns="http://www.w3.org/1999/xhtml"', '')
            content.write('    <td>%s</td>\n' % (value or ''))
        content.write('  </tr>\n')
    content.write('</table>\n')


class _URLConverter:
    def Translate(self, href):
        if not href:
            return ''

        for path in common.alternates:
            if href.startswith(path):
                href = href.replace(path, '')

        if href.startswith('/_/rsrc'):
            href = '/' + '/'.join(href.split('/')[4:])

        url = urlparse(href)
        if '?' in href and url.netloc == '':
            href = href[0:href.index('?')]
        if 'Screenshot' in href:
            head, tail = href.split('Screenshot')
            tail = tail.replace(':', '%3A')
            href = head + 'Screenshot' + tail
        return href


def _path(entry, entries):
    path = entry['page_name']
    parent_id = entry.get('parent_id')
    while parent_id:
        path = entries[parent_id]['page_name'] + '/' + path
        parent_id = entries[parent_id].get('parent_id')

    path = ('/' + path).rstrip('/') or '/'
    return path


def _metadata(entry, entries):
    metadata = {}
    metadata['page_name'] = entry['page_name']
    metadata['title'] = entry['title']

    crumbs = []
    parent_id = entry.get('parent_id')
    while parent_id:
        parent = entries[parent_id]
        path = _path(parent, entries)
        title = parent['title']
        crumbs = [[path, title]] + crumbs
        parent_id = parent.get('parent_id')

    metadata['breadcrumbs'] = crumbs

    if metadata['page_name'] in (
        'chromium-projects',
        'chromium',
    ):
        metadata['use_title_as_h1'] = False

    return metadata


def _needs_update(path, mtime, force):
    if force:
        return True
    if os.path.exists(path):
        st = os.stat(path)
        return mtime > st.st_mtime
    return True


def _entries(args):
    entries = {}
    parents = {}

    # Looks like Sites probably caps results at 500 entries per request,
    # even if we request more than that.
    rownum = 0
    url = ('https://sites.google.com/feeds/content/chromium.org/dev'
           '?start-index=%d&max-results=%d&alt=json' %
               (args.start_index, 500 - rownum))
    doc, next_url = _fetch(url, args.force)

    for rownum, entry in enumerate(doc['feed']['entry'], start=1):
        row = _to_row(entry, rownum)
        entries[row['id']] = row
        if row.get('parent_id'):
            parents.setdefault(row['parent_id'], set()).add(row['id'])
    if args.verbose:
        print(' ... [%d]' % rownum)
    while next_url:
        doc, next_url = _fetch(next_url, args.force)
        for rownum, entry in enumerate(doc['feed']['entry'], start=rownum):
            row = _to_row(entry, rownum)
            entries[row['id']] = row
            if row.get('parent_id'):
              parents.setdefault(row['parent_id'], set()).add(row['id'])
        if args.verbose:
            print(' ... [%d]' % rownum)

    for entry_id, entry in entries.items():
        if entry['kind'] == 'listpage':
            entry['listitems'] = [entries[child_id]['fields'] for child_id
                                  in parents[entry_id]
                                  if entries[child_id]['kind'] == 'listitem']

    return entries


def _fetch(url, force):
    path = url.replace('https://sites.google.com/feeds/', 'scripts/feeds/')
    if _needs_update(path, 0, force):
        fp = urlopen(url)
        content = fp.read()
        doc = json.loads(content)
        updated = _to_ts(doc['feed']['updated']['$t'])
        common.write_if_changed(path, content)
    else:
        with open(path) as fp:
            doc = json.load(fp)
    next_url = _find_link(doc['feed'], 'next')
    return doc, next_url


def _find_link(doc, rel):
    for ent in doc['link']:
        if ent['rel'] == rel:
            return ent['href']
    return None


def _to_row(entry, rownum):
    row = {
        'rownum': rownum,
        'content': entry.get('content', {}).get('$t'),
        'id': _to_id(entry['id']['$t']),
        'kind': entry['category'][0]['label'],
        'published': entry['published']['$t'],
        'updated': entry['updated']['$t'],
    }

    row['page_name'] = entry.get('sites$pageName', {}).get('$t')
    row['title'] = entry.get('title', {}).get('$t')
    row['alt_url'] = _find_link(entry, 'alternate')

    if row['kind'] == 'attachment':
        row['url'] = _find_link(entry, 'alternate')
    else:
        row['url'] = _find_link(entry, 'self')

    if row['kind'] == 'listitem':
        path = row['url'].replace('https://sites.google.com',
                                  os.path.join(common.REPO_DIR, 'scripts'))
        if os.path.exists(path):
          xml_content = common.read_text_file(path)
        else:
          print('fetching %s' % row['url'])
          with urlopen(row['url']) as fp:
            xml_content = fp.read()
            common.write_if_changed(path, xml_content)

        root = ET.fromstring(xml_content)
        fields = root.findall('{http://schemas.google.com/spreadsheets/2006}field')
        row['fields'] = collections.OrderedDict((el.attrib['name'], el.text) for el in fields)

    parent_url = _find_link(entry,
                            'http://schemas.google.com/sites/2008#parent')
    if parent_url:
        row['parent_id'] = _to_id(parent_url)
    return row


def _to_id(url):
    return url[url.rfind('/') + 1:]


def _to_ts(iso_time):
    return time.mktime(time.strptime(iso_time, '%Y-%m-%dT%H:%M:%S.%fZ'))

if __name__ == '__main__':
    try:
        main()
    except Exception:
        extype, value, tb = sys.exc_info()
        traceback.print_exc()
        pdb.post_mortem(tb)
