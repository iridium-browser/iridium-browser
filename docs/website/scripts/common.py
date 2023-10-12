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
"""Generic collection of functions and objects used by different scripts."""
import dataclasses
import multiprocessing
import os
import queue
import sys
import time
import urllib.parse

# The absolute path to the top of the repo.
REPO_DIR = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

# The absolute path to the directory containing the content.
SITE_DIR = os.path.join(REPO_DIR, 'site')


def walk(top):
    """Returns a list of all the files found under the `top` directory

    This routine is a simplified wrapper around `os.walk`. It walks the
    top directory and all its subdirectories to gather the list of files.
    """
    paths = set()
    for dirpath, dnames, fnames in os.walk(top):
        for dname in dnames:
            rpath = os.path.relpath(os.path.join(dirpath, dname), top)
            if dname.startswith('.'):
                dnames.remove(dname)
        for fname in fnames:
            rpath = os.path.relpath(os.path.join(dirpath, fname), top)
            if fname.startswith('.'):
                continue
            paths.add(rpath)
    return sorted(paths)


class JobQueue:
    """Single-writer/multiple-reader job processor.

    The processor will spawn off multiple processes to
    handle the requests given it in parallel. The caller should
    create the JobQueue(), enqueue all of the requests to make with `request()`,
    and then repeatedly call `results` to retrieve the results of each job.

    Each subprocess will retrieve a job, process it using the provided
    `handler` routine, and return the result. If `jobs` > 1, the results
    may be returned out of order.
    """

    def __init__(self, handler, jobs):
        """Initialize the JobQueue.

        `handler` specifies the routine to be invoked for each job.
        The handler will be invoked with the task and obj that it was
        handed in request().

        `jobs` indicates the number of jobs to process in parallel.
        """

        self.handler = handler
        self.jobs = jobs
        self.pending = set()
        self.started = set()
        self.finished = set()
        self._request_q = multiprocessing.Queue()
        self._response_q = multiprocessing.Queue()
        self._start_time = None
        self._procs = []
        self._last_msg = None
        self._isatty = sys.stdout.isatty()

    def all_tasks(self):
        """Returns a list of all tasks requested so far.

        The list contains all of the `task` strings that were passed
        to request(). All of the tasks are returned regardless of the
        state they are in (not started, running, or completed)."""
        return self.pending | self.started | self.finished

    def request(self, task, obj):
        """Sends a job request to the job processor.

        `task` should consist of a human-readable string that will be
        printed out while the processor is running, and `obj` should
        be another arbitrary picklable object that can be passed to the
        handler along with `task`.
        """
        self.pending.add(task)
        self._request_q.put(('handle', task, obj))

    def results(self):
        """Returns the results of one job.

        `results` acts as a generator to return job results, returning
        the results one at a time. The results may be not be in the
        same order that the requests were.

        The job-processor will print its progress in a simplified Ninja-like
        format while the results are being returned. If the program is printing
        to a tty-like object, you will get a single line of output with
        task numbers and names. If the program is printing to a non-tty-like
        object, it will print the results one line at a time.
        """

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

        for _ in self._procs:
            self._request_q.put(('exit', None, None))
        for proc in self._procs:
            proc.join()
        if self._isatty:
            print()

    def _spawn(self):
        args = (self._request_q, self._response_q, self.handler)
        for i in range(self.jobs):
            proc = multiprocessing.Process(target=_worker,
                                           name='worker-%d' % i,
                                           args=args)
            self._procs.append(proc)
            proc.start()

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
    'Routine invoked repeatedly in the subprocesses to process the jobs.'
    while True:
        message, task, obj = request_q.get()

        assert message in ('exit', 'handle'), (
            "Unknown message type '%s'" % message)

        if message == 'exit':
            break

        # message == 'handle':
        response_q.put(('started', task, '', None))
        res, resp = handler(task, obj)
        response_q.put(('finished', task, res, resp))
