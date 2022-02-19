#!/usr/bin/env python3
# Copyright 2021 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Script for crawling a website.

This script will crawl a website and download all of the pages and
related assets, rewriting the links to point to the local copies.
"""

import argparse
import html
import os
import sys
import time
import urllib.parse
import urllib.request

import common


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('-j', '--jobs', type=int,
                        default=common.cpu_count(),
                        help='Number of URLs to fetch in parallel '
                             '(default %(default)s)')
    parser.add_argument('-l', '--limit', type=int, metavar='N',
                        help='Only fetch the first N pages')
    parser.add_argument('-o', '--out-dir', default='pages')
    parser.add_argument('--path-list', default='scripts/paths_to_crawl.txt',
                        help='initial list of URLs to seed the crawl')
    parser.add_argument('--paths-to-skip', default='scripts/paths_to_skip.txt',
                        help='list of URLs to skip (expected 404s)')
    parser.add_argument('--prefix', default='',
                        help='path to prepend to all the URLs')
    parser.add_argument('paths', nargs='*')
    args = parser.parse_args(argv)

    if args.paths:
        urls = [common.site + path for path in args.paths]
    elif args.path_list:
        urls = [common.site + path for path in
                common.read_paths(args.path_list)]
    else:
        urls = [common.site + '/']

    args.alternates = common.alternates

    Crawler(args).crawl(urls)


class Crawler:
    def __init__(self, args):
        self.site = None
        self.host = None
        self.args = args
        self.queue = common.JobQueue(self._handle_url, self.args.jobs,
                                     multiprocess=False)
        self.paths_to_skip = set()

    def crawl(self, urls):
        if self.args.paths_to_skip:
            self.paths_to_skip = common.read_paths(self.args.paths_to_skip)

        self.site = urllib.parse.urlparse(urls[0])
        self.host = urllib.parse.urlunparse(
            urllib.parse.ParseResult(self.site.scheme,
                                     self.site.netloc,
                                     path='', params='', query='', fragment=''))

        self._fetch(urls)
        num_errors = 0
        num_urls = 0
        for task, res, links in self.queue.results():
            if res:
                num_errors += 1
            num_urls += 1
            self._fetch(urllib.parse.urljoin(self.host + task, link)
                        for link in links)

        print('Crawled %d URLs%s.' % (num_urls,
                (' (%d errors)' % num_errors) if num_errors else ''))

        return 0

    def _fetch(self, urls):
        for url in urls:
            should_fetch, task, new_url = self._filter(url)
            if should_fetch:
                self.queue.request(task, new_url)

    def _filter(self, url):
        comps = urllib.parse.urlparse(url)
        if (not any(url.startswith(x) for x in common.alternates) or
            comps.path.startswith('/system/app/pages')):
            return False, comps.path, url

        task = _rewrite_link(url, '')
        idx = task.find('#')
        if idx != -1:
            task = task[:idx]
        task = task.rstrip('/') or '/'
        if task in self.paths_to_skip:
            return False, task, url
        if task in self.queue.all_tasks():
            return False, task, url

        new_url_comps = urllib.parse.ParseResult(
            comps.scheme,
            comps.netloc,
            comps.path.rstrip('/') or '/',
            params='',
            query='',
            fragment='')
        new_url = urllib.parse.urlunparse(new_url_comps)

        all_tasks = self.queue.all_tasks()
        if not self.args.limit or len(all_tasks) < self.args.limit:
            if task not in all_tasks:
                return True, task, new_url
        return False, task, new_url

    def _handle_url(self, task, url):
        del task
        prefix = self.args.prefix
        out_dir = self.args.out_dir
        comps = urllib.parse.urlparse(url)
        path = _rewrite_link(url, prefix)

        res = ''
        links = []

        for i in range(4):
            try:
                with urllib.request.urlopen(url) as resp:
                    content_type = resp.getheader('Content-Type')
                    content = resp.read()
            except Exception as e:
                if i < 3:
                    time.sleep(1.0)
                    continue

                res = '%s: %s' % (type(e), str(e))
                return res, links

        if content_type.startswith('text/html'):
            page, links = _rewrite_html(content.decode('utf-8'), prefix)
            new_content = page.encode('utf-8')
            path = path.rstrip('/') + '/index.html'
        elif content_type == 'text/css':
            page, links = _rewrite_css(content.decode('utf-8'), prefix)
            new_content = page.encode('utf-8')
        else:
            new_content = content
        common.write_if_changed(out_dir + path, new_content)
        return res, links


def _rewrite_html(page, prefix):
    links = set()
    page = _rewrite_tag(page, prefix, links, 'a', 'href')
    page = _rewrite_tag(page, prefix, links, 'img', 'src')
    page = _rewrite_tag(page, prefix, links, 'script', 'src')
    for val in ('stylesheet', 'shortcut icon', 'apple-touch-icon'):
        page = _rewrite_tag(page, prefix, links, 'link', 'href', val)

    return page, links


def _rewrite_tag(page, prefix, links, tag, attr, rel=None, follow=True):
    new_page = ''
    if rel:
        tag_str = '<%s rel="%s"' % (tag, rel)
    else:
        tag_str = '<%s' % (tag,)
    attr_str = '%s="' % (attr,)

    pos = 0
    while True:
        idx = page.find(tag_str, pos)
        if idx == -1:
            new_page += page[pos:]
            break

        tag_close_idx = page.find('>', idx)
        attr_idx = page.find(attr_str, idx)
        if attr_idx == -1 or attr_idx > tag_close_idx:
            new_page += page[pos:tag_close_idx]
            pos = tag_close_idx
            continue

        link_start = attr_idx + len(attr_str)
        link_end = page.find('"', link_start)

        link = html.unescape(page[link_start:link_end])
        new_link = _rewrite_link(link, prefix)

        if follow or tag != 'a':
            links.add(link)

        new_page += page[pos:link_start]
        new_page += new_link
        pos = link_end

    return new_page


def _rewrite_css(content, prefix):
    content, links = _rewrite_rule(content, prefix, '@import "', '"')
    content, more_links = _rewrite_rule(content, prefix, 'url(', ')')
    links.update(more_links)

    return content, links


def _rewrite_rule(content, prefix, start, end):
    new_content = ''
    links = set()

    pos = 0
    while True:
        start_idx = content.find(start, pos)
        if start_idx == -1:
            new_content += content[pos:]
            break

        end_idx = content.find(end, start_idx)

        link_start = start_idx + len(start)

        if ((content[link_start] == '"' and content[end_idx-1] == '"') or
            (content[link_start] == "'" and content[end_idx-1] == "'")):
            link_start += 1
            end_idx -= 1
        link = content[link_start:end_idx]
        new_link = _rewrite_link(link, prefix)

        new_content += content[pos:link_start]
        new_content += new_link
        pos = end_idx

    return new_content, links


def _rewrite_link(link, prefix):
    new_link = link
    if '?' in new_link:
        new_link = link[0:new_link.index('?')]
    for alt in common.alternates:
        new_link = new_link.replace(alt, '')
    for site_prefix in ('/sites/p/058338/','/sites/p/d955fc'):
        if new_link.startswith(site_prefix):
            new_link = new_link[len(site_prefix):]
    if new_link.startswith('/_/rsrc'):
        new_link = '/' + '/'.join(new_link.split('/')[4:])
    new_link = new_link.rstrip('/') or '/'
    if prefix and new_link.startswith('/'):
        new_link = '/%s%s' % (prefix, new_link)
    return new_link


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
