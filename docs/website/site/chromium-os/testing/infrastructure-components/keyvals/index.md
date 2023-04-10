---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
- - /chromium-os/testing/infrastructure-components
  - Infrastructure Components
page_name: keyvals
title: Keyvals
---

Job keyvals are key/value pairs, used to cache/exchange data about jobs between
different services in autotest, via the database. There are two keyvals tables,
afe_job_keyvals and tko_job_keyvals. Both tables have a job_id column for
keyvals to be foreign keyed to a job. The job_id can be a suite job id if jobs
are queued by a suite run, or test job id if the job is queued directly from
atest/run_remote_test/autoserv.

The initial job keyvals get specified as part of the arguments to the create_job
RPC and entered into the database (into the afe_job_keyvals table). Then, before
we go to run a job, we pull the keyvals out of the database and write them to a
file. This is done by a class named TaskWithJobKeyvals that is a mixin that gets
included for the Agents representing job runs. The parsing of these keyvals back
into the database happens when the scheduler kicks off tko/parse.py to parse job
results during the PARSING phase. tko/parse.py calls tko/db.py:insert_job(),
which contains the call to update_job_keyvals() which does the actual job keyval
writing. These get inserted into tko_job_keyvals, so the initial keyvals are
always in afe_job_keyvals, and the final results in tko_job_keyvals.

afe_job_keyvals stores the a job's suite and build information.

tko_job_keyvals stores the test result information, including:

user that requested the job

job start/end time (job_started, job_queued, job_finished)

perf keyvals, which are up to individual tests to create as they see fit. The
use and lifecycle of perf keyvals is explained in detail on [a separate
webpage](/system/errors/NodeNotFound).

Importantly, the tko_job_keyvals entry for a suite job stores a dictionary
between hashed test names and job_id-owner. After a suite job is finished,
run_suite uses this dictionary to find out the job url for a particular test.
The reason for this is that when a suite is started, all keyvals are keyed by
suite job id. The test job id is set only after the test job is started. After a
test job has finished, autoserv creates logs a keyval (for the test’s job id)
with the hashed test name as the key and job_id-owner as the value. run_suite
pulls the keyval from tko_job_keyval table and maps the test job to its result
page, which is the test job page, rather than suite job page.
