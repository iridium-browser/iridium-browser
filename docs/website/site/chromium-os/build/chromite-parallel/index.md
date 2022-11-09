---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/build
  - Chromium OS Build
page_name: chromite-parallel
title: Writing multiprocess programs in Python
---

The
[chromite.lib.parallel](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/lib/parallel.py)
module is the preferred way to write multiprocess python programs in Chrome OS.

It offers the following advantages over the multiprocessing module:

*   The multiprocessing module can be buggy, so it is difficult to avoid
            hangs without substantial trial and error figuring out what features
            of multiprocessing actually work. We have already done this for you
            in the parallel library.
*   The interface for the parallel library is simpler than the
            multiprocessing interface and performs tasks that you would need to
            perform manually if you were using multiprocessing.
*   The parallel library automatically queues up output in temporary
            files and prints them out serially. This means that you never need
            to worry about interleaved output.
*   The parallel library handles cases where the subprocess hangs and
            prints out suitable errors.

Here's the
[chromite.lib.parallel](https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/lib/parallel.py)
function for running multiple steps in parallel in a very simple way. The
docstring should help explain how it works.

def RunParallelSteps(steps, max_parallel=None, halt_on_error=False,

return_values=False):

"""Run a list of functions in parallel.

This function blocks until all steps are completed.

The output from the functions is saved to a temporary file and printed as if

they were run in sequence.

If exceptions occur in the steps, we join together the tracebacks and print

them after all parallel tasks have finished running. Further, a

BackgroundFailure is raised with full stack traces of all exceptions.

Args:

steps: A list of functions to run.

max_parallel: The maximum number of simultaneous tasks to run in parallel.

By default, run all tasks in parallel.

halt_on_error: After the first exception occurs, halt any running steps,

and squelch any further output, including any exceptions that might occur.

return_values: If set to True, RunParallelSteps returns a list containing

the return values of the steps. Defaults to False.

Returns:

If |return_values| is True, the function will return a list containing the

return values of the steps.

Example:

# This snippet will execute in parallel:

# somefunc()

# anotherfunc()

# funcfunc()

steps = \[somefunc, anotherfunc, funcfunc\]

RunParallelSteps(steps)

# Blocks until all calls have completed.

"""

If you want to be able to use a worker-based model and push items to the
subprocess one by one, the BackgroundTaskRunner will do the job:

@contextlib.contextmanager

def BackgroundTaskRunner(task, \*args, \*\*kwargs):

"""Run the specified task on each queued input in a pool of processes.

This context manager starts a set of workers in the background, who each

wait for input on the specified queue. For each input on the queue, these

workers run task(\*args + \*input, \*\*kwargs). Note that certain kwargs will

not pass through to the task (see Args below for the list).

The output from these tasks is saved to a temporary file. When control

returns to the context manager, the background output is printed in order,

as if the tasks were run in sequence.

If exceptions occur in the steps, we join together the tracebacks and print

them after all parallel tasks have finished running. Further, a

BackgroundFailure is raised with full stack traces of all exceptions.

Example:

# This will run somefunc(1, 'small', 'cow', foo='bar') in the background

# as soon as data is added to the queue (i.e. queue.put() is called).

def somefunc(arg1, arg2, arg3, foo=None):

...

with BackgroundTaskRunner(somefunc, 1, foo='bar') as queue:

... do random stuff ...

queue.put(\['small', 'cow'\])

... do more random stuff while somefunc() runs ...

# Exiting the with statement will block until all calls have completed.

Args:

task: Function to run on each queued input.

queue: A queue of tasks to run. Add tasks to this queue, and they will

be run in the background. If None, one will be created on the fly.

processes: Number of processes to launch.

onexit: Function to run in each background process after all inputs are

processed.

"""
