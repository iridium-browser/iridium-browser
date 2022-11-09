---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/design-documents
  - Design Documents
- - /developers/design-documents/network-stack
  - Network Stack
page_name: http-authentication-throttling
title: HTTP Authentication Throttling
---

## Problem statement

Some users get locked out of proxies or servers after entering an invalid
username/password combination because Chrome will immediately reissue all
pending requests with the same invalid credentials.

To prevent this from happening, only one request should be reissued. If
authentication succeeds, all other pending requests can be reissued. If it
fails, the user will be presented with another login prompt. If the request is
cancelled, another pending request will be reissued.

The downside is that this will introduce an RTT penalty for the other pending
requests. However, that penalty will likely be dwarfed by the time to enter
username/password, and the penalty is severe enough that the cost is likely
worth it.

**Implementation Choices**

There are two general approaches for implementing this.
a) Do the throttling in the network stack, with no changes to the URLRequest
delegates. All pending requests are reissued from the perspective of
ResourceDispatcherHost and LoginHandler, but are queued someplace such as
HttpAuthController::MaybeGenerateAuthToken \[which can complete
asynchronously\], with the state residing in the HttpAuthCache::Entry. If the
authentication request succeeds, all pending requests continue down the network
stack. If it fails, pretend that all of the other requests also were rejected by
the server or proxy and send a 401 or 407 back up the network stack using the
same auth challenge as before.

b) Do the throttling in the LoginHandler, and only restart one URLRequest. To
make this happen, we'll need to change LoginHandler so there is one per (domain,
scheme, realm) tuple instead of one per URLRequest, and add a LoginHandlerTable
to do the discovery/creation. If the outstanding request succeeds, reissue all
other pending requests. If it fails, re-show all possible dialogs \[one per
tab\]. If it is cancelled, reissue another pending request or kill the
LoginHandler if none remain.

Initially a) seemed like a more attractive option. All users of the networking
stack would be able to take advantage of the behavior without having to
implement their own throttling mechanism. It's also less of a change: we already
do grouping of (domain, scheme, realm) tuples in the HttpAuthCache and have a
natural queuing location at HttpAuthController::MaybeGenerateAuthToken.

However, it has a number of issues which make it seem like the wrong approach:

*   If authentication fails, and the user cancels authentication, the
            pending requests will not contain the correct body of the 401 or 407
            response.
*   The NetLog and developer tools may show a number of requests which
            were not actually issued.
*   It's possible that not all consumers of the network stack want this
            behavior.
*   Only does throttling for HTTP, not FTP \[not sure if this is good or
            bad\].

As a result, doing the throttling at the LoginHandler level makes more sense.
It's also a more natural match for what's actually going on.

**LoginHandler throttling**

Instead of one LoginHandler per request, there will be one LoginHandler per
(domain, scheme, realm). A LoginHandlerDirectory will maintain a map of
LoginHandler's and manage their lifetime, and the LoginHandlerDirectory will be
owned by the ResourceDispatcherHost.

The LoginHandler interface will look like

class LoginHandler {

public:

// Adds a request to be handled

void AddRequest(URLRequest\* request);

// Removes a request from being handled, done on cancellation.

void RemoveRequest(URLRequest\* request);

// Called when the user provides auth credentials.

void OnCredentialsSupplied();

// Called when the user cancels entering auth credentials.

void OnUserCancel();

// Called when authentication succeeds on a request.

void OnAuthSucceeded(URLRequest\* request);

// Called when authentication fails on a request.

void OnAuthFailed(URLRequest\* request);

// Called by LoginHandlerDirectory to see if it should free.

bool IsEmpty() const;

};

The LoginHandler is a state machine, with four states.

WAITING_FOR_USER_INPUT

WAITING_FOR_USER_INPUT_COMPLETE TRYING_REQUEST

TRYING_REQUEST_COMPLETE

with perhaps a Nil or initialization state.

AddRequest() will always queue the request, but it should not already be in the
set of requests.

If RemoveRequest() is called while in the WAITING_FOR_USER_INPUT or
WAITING_FOR_USER_INPUT_COMPLETE state, it will remove from the set and remove a
dialog if it is the only request for a particular tab. It will also remove the
LoginHandler if it is the last request. If in the TRYING_REQUEST or
TRYING_REQUEST_COMPLETE state, the request is simply removed from the set if it
is not the currently attempted request. If it is the currently attempted
request, then TRYING_REQUEST is re-entered with a different request.

OnCredentialsSupplied() must be called during WAITING_FOR_USER_INPUT and will
transition to WAITING_FOR_USER_INPUT_COMPLETE. This will also choose a request
to try and enter TRYING_REQUEST.

OnUserCancel() must be called during WAITING_FOR_USER_INPUT and will transition
to WAITING_FOR_USER_INPUT_COMPLETE. This will cancel auth on all of the pending
requests and display the contents of the 401/407 body.

OnAuthSucceeded() must be called during the TRYING_REQUEST state, and will
transition to TRYING_REQUEST_COMPLETE. This will reissue all other pending
requests and close out the LoginHandler.

OnAuthFailed() must be called during the TRYING_REQUEST state, will enter
TRYING_REQUEST_COMPLETE, and will go back to WAITING_FOR_USER_INPUT. The pending
request is moved back to the main set of pending requests.

**Delaying credential entering into HttpAuthCache**

Although the LoginHandler changes described above will throttle most unconfirmed
authentication requests, there is still the chance that some will get through.

While the LoginHandler is in the TRYING_REQUEST state, the username/password are
entered into the HttpAuthCache::Entry before hearing back from the server or
proxy about whether the results are successful. Any other URLRequest's issued
during this period of time will use the unconfirmed username/password.

If the credentials are entered on a successful response, then this problem goes
away. ResourceDispatcherHost issued URLRequest's will likely fail, get a 401,
and get added to the queue of pending requests in the appropriate LoginHandler.
Other dispatched ones such as URLFetcher will simply fail.

There is a race I just thought about which might annoy owners, and I don't have
a good answer for. Assume that there is an outstanding request A which is
waiting to hear back from the proxy if the authentication succeeded. A second
request B comes in, notices no username/password in the HttpAuthCache and so
does not add preemptive authentication and issues a raw GET. Request A completes
successfully, fills in the HttpAuthCache with the credentials, goes back to the
LoginHandler, reissues all requests and closes the LoginHandler. Then, Request B
returns from the proxy with a 407 response code, since it provided no
Authorization header. Since the LoginHandler has been destroyed, a new one is
created and the user is presented with a login prompt again.

One way to fix that case is to add a PENDING state to the HttpAuthCache::Entry
when Request A is issued, and removed when it hears back from the proxy. Request
B will be stalled at the MaybeGenerateAuthToken state when the entry is pending,
and adds itself to a list in the entry. When it completes, all issues are
continued either using preemptive authentication with the credentials in the
cache entry \[if Request A succeeded\] or issued with no credentials if Request
A failed. That still may result in a race in the failure case, however.
