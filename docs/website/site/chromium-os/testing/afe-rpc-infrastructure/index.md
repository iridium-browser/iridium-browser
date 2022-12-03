---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/testing
  - Testing Home
page_name: afe-rpc-infrastructure
title: AFE RPC Infrastructure
---

# Infrastructure

Our RPCs are served by apache running on cautotest using django's ability to map
python's functions to URLs. In frontend/afe/urls.py we end up mapping /rpc/ to
frontend/afe/views.py:handle_rpc() via the generate_patterns() call in
frontend/urls_common.py. The handle_rpc view very quickly bails out to the code
in frontend/afe/rpc_handler.py, which pulls all functions defined in
frontend/afe/rpc_interface.py or frontend/afe/site_rpc_interface.py, and then
exposes them as RPCs.

Any changes done to the RPC infrastructure will require a restart of apache,
since all of the python code is loaded only once when apache first starts. This
means that any updates to the code done after apache is started won’t be used or
seen. To solve this, one can gracefully restart apache by running sudo
/etc/init.d/apache2 reload. Any CLs that you write that touch RPC-related code
should include the line DEPLOY=apache to indicate to whoever is pushing that
this step needs to be done.

Data being sent and received by an RPC is serialized in json format, as can be
seen in frontend/afe/json_rpc/serviceHandler.py. The RPCs expect a JSON object
of the form {'id':int, 'method':string, 'params':\[JSON\]}. A response is sent
back in the form {'id':int, 'result':JSON, 'error':{'name':string,
'message':string, 'traceback':string}}.

As a client, to call an RPC it is preferable to use the wrapper declared in
autotest_lib.server.frontend.AFE. It automatically deduces the correct server
from your global/shadow config, and wraps around the more bare client-side RPC
interface in frontend/afe/json_rpc/proxy.py.

### RPCs

We use RPCs to provide a layer between database access and data querying. This
allows use to enforce ACLs, centralize logic to ensure that the operation that
is about to be done is sane, and to allow the underlying data representation to
change without having to change code everywhere.

The RPCs that we have allow for create/get/update/delete operations to be done
on most data that one would find important (hosts, labels, jobs, host queue
entries, etc.). Very straightforward examples of these can be found by looking
at the ./cli/atest source code.

If you take a look at the functions defined in rpc_interface.py, you'll see that
most of the get methods take an argument of filter_data. This argument is then
passed directly to the django models call, so everything that is allowed as a
parameter on a django filter() call (ie.__startswith, __in, etc.) is valid to
pass here. Slightly sadly though, this does mean that the RPCs do not completely
abstract one from the underlying schema of the database tables.

Two very notable RPCs are the create_job call in rpc_interface and
create_suite_job insite_rpc_interface. This is the only place in autotest where
jobs are created. Any special handling of job creation, such as parsing out
METAVARs, or ensuring control file sanity, can be done at this point to enforce
that only correctly formed control files will make it into the system.

The set of RPCs exposed doesn't have much functionality beyond just asking for a
piece of data to be immediately returned or modified. This means that we have
large parts of the system that sit and poll to see if an event has happened,
instead of being able to wait for the completion of an event. Attempting to
create a synchronous RPC, for example, one that creates a job, and returns once
the job is done would be much more difficult, as everything goes down to MySQL
in the end, and MySQL has no way to support notifying a piece of python code
that a value has changed. (Or, at least, I don't know how to use triggers to do
this.)

It should be noted that TKO has its own set of RPCs, generally used to access
data in tko_\* tables, uses of these don't come up quite as frequently in the
code, but a notable case is run_suite.py, which gets is information about the
suite that ran by querying the get_detailed_test_view TKO RPC.

To add a new RPC, one would simply just add a new normal python function to
site_rpc_interface.py. The only restriction is that the return value of the
function must be JSON serializable. It'd be a good idea to look at
frontend/afe/rpc_utils.py to see if there's any relevant functionality that'd be
useful. Specifically, if you're just wrapping around a django model query, you
can use the prepare_for_serialization function to massage the returned django
model objects into JSON.
