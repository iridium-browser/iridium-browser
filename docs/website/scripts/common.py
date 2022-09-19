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

import dataclasses
import multiprocessing
import os
import queue
import sys
import threading
import time
import urllib.parse


site = 'https://www.chromium.org'

REPO_DIR = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
SITE_DIR = os.path.join(REPO_DIR, 'site')
BUILD_DIR = os.path.join(REPO_DIR, 'build')
DEFAULT_TEMPLATE = '/_includes/page.html'



alternates = [
    site,
    'http://dev.chromium.org',
    'https://dev.chromium.org',
    'https://sites.google.com/a/chromium.org/dev',
    'https://ssl.gstatic.com/sites/p/058338',
    'http://www.gstatic.com/sites/p/058338',
]


def cpu_count():
    return multiprocessing.cpu_count()


def read_text_file(path):
    return read_binary_file(path).decode('utf-8')


def read_binary_file(path):
    with open(path, 'rb') as fp:
        return fp.read()


def write_binary_file(path, content):
    with open(path, 'wb') as fp:
        return fp.write(content)

def write_text_file(path, content):
    with open(path, 'w') as fp:
        return fp.write(content)

def read_paths(path):
    paths = set()
    with open(path) as fp:
        for line in fp.readlines():
            idx = line.find('#')
            if idx != -1:
                line = line[:idx]
            line = line.strip()
            if line:
                paths.add(line)
    return paths


def to_path(page, top=SITE_DIR, ext='.md'):
    page = page.strip()
    if page == '/':
        page = ''
    if os.path.isdir(top + page):
        return page + '/index' + ext
    if os.path.exists(top + page):
        return page
    if ext and os.path.exists(top + page + ext):
        return page + ext
    return page


def walk(top, skip=None):
    skip = skip or set()
    paths = set()
    for dirpath, dnames, fnames in os.walk(top):
        for dname in dnames:
            rpath = os.path.relpath(os.path.join(dirpath, dname), top)
            if rpath in skip or dname.startswith('.'):
                dnames.remove(dname)
        for fname in fnames:
            rpath = os.path.relpath(os.path.join(dirpath, fname), top)
            if rpath in skip or fname.startswith('.'):
                continue
            paths.add(rpath)
    return sorted(paths)


def write_if_changed(path, content, mode='wb', encoding='utf-8'):
    if mode == 'w':
        content = content.encode(encoding)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if os.path.exists(path):
        with open(path, 'rb') as fp:
            old_content = fp.read()
            if content == old_content:
                return False
    write_binary_file(path, content)
    return True


def should_update(dest_page, source_pages):
    if not os.path.exists(dest_page):
        return True

    dest_pages = [dest_page]
    max_source_mtime = max(os.stat(p).st_mtime for p in source_pages)
    max_dest_mtime = max(os.stat(p).st_mtime for p in dest_pages)
    return max_source_mtime > max_dest_mtime 


class JobQueue:
    def __init__(self, handler, jobs, multiprocess=None):
        self.handler = handler
        self.jobs = jobs
        self.pending = set()
        self.started = set()
        self.finished = set()
        if multiprocess is None:
            self.multiprocess = (jobs > 1)
        else:
            self.multiprocess = multiprocess
        if self.multiprocess:
            self._request_q = multiprocessing.Queue()
            self._response_q = multiprocessing.Queue()
        else:
            self._request_q = queue.Queue()
            self._response_q = queue.Queue()
        self._start_time = None
        self._threads = []
        self._last_msg = None
        self._isatty = sys.stdout.isatty()

    def all_tasks(self):
        return self.pending | self.started | self.finished

    def request(self, task, obj):
        self.pending.add(task)
        self._request_q.put(('handle', task, obj))

    def results(self):
        self._start_time = time.time()
        self._spawn()

        while self.pending | self.started:
            msg, task, res, obj = self._response_q.get()

            if msg == 'started':
                self._mark_started(task)
            elif msg == 'finished':
                self._mark_finished(task, res)
                yield (task, res, obj)
            else:
                raise AssertionError

        for _ in self._threads:
            self._request_q.put(('exit', None, None))
        for thread in self._threads:
            thread.join()
        if self._isatty:
            print()

    def _spawn(self):
        args = (self._request_q, self._response_q, self.handler)
        for i in range(self.jobs):
            if self.multiprocess:
                thread = multiprocessing.Process(target=_worker,
                                                 name='worker-%d' % i,
                                                 args=args)
            else:
                thread = threading.Thread(target=_worker,
                                          name='worker-%d' % i,
                                          args=args)
            self._threads.append(thread)
            thread.start()

    def _mark_started(self, task):
        self.pending.remove(task)
        self.started.add(task)

    def _mark_finished(self, task, res):
        self.started.remove(task)
        self.finished.add(task)
        if res:
            self._print('%s failed:' % task, truncate=False)
            print()
            print(res)
        else:
            self._print('%s' % task)
        sys.stdout.flush()

    def _print(self, msg, truncate=True):
        if not self._isatty:
            print('[%d/%d] %s' % (len(self.finished), len(self.all_tasks()),
                                  msg))
            return

        if len(msg) > 76 and truncate:
            msg = msg[:76] + '...'
        if self._last_msg is not None:
            print('\r', end='')
        msg = '[%d/%d] %s' % (len(self.finished), len(self.all_tasks()), msg)
        print(msg, end='' if self._isatty else '\n')
        if self._last_msg is not None and len(self._last_msg) > len(msg):
            print(' ' * (len(self._last_msg) - len(msg)), end='')
            print('\r', end='')
        self._last_msg = msg


def _worker(request_q, response_q, handler):
    while True:
        message, task, obj = request_q.get()
        if message == 'exit':
            break
        elif message == 'handle':
            response_q.put(('started', task, '', None))
            res, resp = handler(task, obj)
            response_q.put(('finished', task, res, resp))
        else:
            raise AssertionError
