---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: gs-offloader
title: GS Offloader
---

The gs_offloader is a crucial part of the chrome os lab infrastructure as it is
in charge of ensuring that the drones don't run out of disk space. When we run
jobs on the drone, we store their job results on the drone and each one
generates at least some amount of data. However due to the number of tests we
run and the fact that crash logs can cause the amount of data to increase
greatly, disk space on our drones can be eaten up rather quickly. But we can't
simply delete the job results as we may need it in the future when bugs occur or
we discover infrastructure problems.

gs_offloader solves this problem by running on its own on the drones and
offloads the test result data to google storage for permanent storage. In a
brief overview: gs_offloader monitors the results directory of tests and special
tasks and for each folder in the results directory it checks if the
corresponding job is completed. If so the gs_offloader will make system level
calls to send this data to google storage. In general it will be using gsutil
but rsyncing to another host is an option as well.

Originally, gs_offloader offloaded both the job results and the special task
results within the same loop. It was found that we would frequently spend a
while offloading special task results, which are quite small, while job results,
which are much larger, would pile up. Thus, we could either try to prioritize
directories by size, or just run two instances of gs_offloader, one for special
tasks and one for job results. The latter is far simpler, so it's what we
currently have. Thus, we run two copies of gs_offloader, and the code path for
each is slightly different but follows the same type of logic.

The gs_offloader's main thread loops over the results directory and for each
folder it uses the is_job_complete module to determine if it should offload the
directory. is_job_complete makes an RPC call to check if that job has been
marked as completed. If so the folder is queued up to be offloaded by the
gs_offloader's offloading threads.

The actual offloading takes place as a system level call to gsutil. This means
that the system runninggs_offloader needs its ~/.boto file setup correctly in
order for it to work, as that's where the credentials to access google storage
are held. The call follows the format of gsutil -m cp -eR -a project-private
\[folder to offloader\] gs://chromeos-autotest-results/\[folder to offloader
relative path to results\].

If offloading is going too slowly, (ie. nagios alerts are being sent out about
the drives filling up, and the problem has been tracked down to there being too
many job results stored locally) you can take advantage of the multi-threading
by restarting gs_offloader to run with more offloading threads until most of the
results are gone. By default, it only runs with one gsutil command at a time.
This can be done by running sudo stop gs_offloader; sudo start gs_offloader
PARALLELISM=5 to have gs_offloader have up to 5 parallel invocations of gsutil
running at once.

Due to the fact that gsutil calls can hang for hours we have a built-in timeout
of 3 hours. This timeout is implemented by surrounding the subprocess call we
make to kick off gsutil with a SIGALRM timer. This works by setting the SIGALRM
timer to 3 hours, kicking off gsutil with subprocess.Popen and then waiting on
the kicked off process. If the timeout occurs before Popen returns then we know
we hit a timeout and log the error and send out an email warning the lab team.

Since we always have the chance of the gsutil call failing we wanted to ensure
we properly capture stdout and stderr for the process call. Therefore we
redirect both to temp files in /tmp so if there is a failure we have the data
and email it out. And if not we can simply delete it. The emailer code is the
email_manager used by the scheduler in order to reduce the number of email
clients we use for our team.

It's worthwhile to note that gs_offloader is designed to be an extremely
long-running process. This means that it'll likely be subjected to, and should
properly handle, unusual events such as the results database being truncated,
cautotest or cautotest-mysql moving to a different host, or other services being
down/unavailable. The term 'thread' has also been used here lightly, the current
implementation actually uses a wrapper around multiprocessing in chromite to do
the parallelism, as the SIGALRM timer doesn't play well with python threading.

In conclusion, the gs_offloader monitors our result repositories and sends the
data to google storage for permanent storage. Without it our machines would fill
up and the lab would go down making it a crucial piece of the lab
infrastructure.
