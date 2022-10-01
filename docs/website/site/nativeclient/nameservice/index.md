---
breadcrumbs:
- - /nativeclient
  - Native Client
page_name: nameservice
title: nameservice
---

**NAME**

> NaClNameServiceGetRoot, NaClNameServiceChannel, NaClNameServiceResolve -- look
> up names in namespaces

**SYNOPSIS**

> #include &lt;stdint.h&gt;

> #include &lt;fcntl.h&gt;

> #include &lt;nacl/nameservice.h&gt;

> int32_t NaClNameServiceGetRoot(void);

> void NaClNameServiceClose(int32_t service);

> int32_t NaClNameServiceChannel(int32_t service);

> void NaClNameServiceChannelClose(int32_t channel);

> int32_t NaClNameServiceResolve(int32_t channel, char const \*name, int32_t
> flags, int32_t mode);

> size_t NaClNameServiceEnumerate(int32_t channel, char \*buffer, size_t
> nbytes);

> void NaClNameServiceChannelAsyncThread(int32_t channel);

> int32_t NaClNameServiceResolveAsync(int32_t channel, char const \*name,
> int32_t flags, int32_t mode, void (\*callback)(void \*state, int32_t desc,
> int32_t errno), void \*state);

> int32_t NaClNameServiceEnumerateAsync(int32_t channel, char \*buffer, size_t
> nbytes, void (\*callback)(void \*state, size_t written), void \*state);

**DESCRIPTION**

> The name service interface provides a general mechanism to resolve names,
> mapping a NUL-terminated character string to an I/O descriptor. Resolution may
> result in descriptors that represent files, name resolvers for lower-level
> namespaces, and, in the future, device interfaces such as video cameras or
> joysticks. Initial namespaces that can be reached from the "root" namespaces
> are manifest files, WebFS persistent files, WebFS temporary files, and files
> included with the standard Native Client runtime.

> NaClNameServiceGetRoot() returns a "root" name service which every Native
> Client module is created with. This name service identity is used to create
> name service channels using which name resolution can occur, using
> NaClNameServiceChannel(). Each Native Client module may create multiple name
> service channels, e.g., one per thread or to form a channel pool that grows as
> needed. Each channel may be used for only one synchronous resolution operation
> at a time, and it is up to the Native Client module to ensure that this holds.

> As an alternative to using the synchronous API, NaClNameServiceResolveAsync
> may be used instead. Before NaClNameServiceResolveAsync is used, the caller
> must spawn a thread and invoke NaClNameServiceChannelAsyncThread, which will
> only return when the channel is closed. Here, the name service will invoke
> callback with the result desc -- which, on an error, will be -1, so the
> callback is guaranteed to be called exactly once -- once a result is
> available. The callback/state functor will be invoked on a separate thread,
> and it is the responsibility of the NaCl module provided callback functor to
> process the result in a thread-safe manner. Multiple
> NaClNameServiceResolveAsync may be invoked on the same channel. However, the
> maximum number of NaClNameServiceResolveAsync operations actually allowed in
> flight is not specified, and the order of callbacks may occur in any order. If
> the callback is invoked with the desc argument equal to -1, the errno argument
> will have a meaningful value. If the maximum number of in-flight asynchronous
> name resolutions is exceeded, NaClNameServiceResolveAsync returns -1, and the
> caller may retry later, e.g., when a callback is invoked; otherwise
> NaClNameServiceResolveAsync returns 0 to indicate that the callback will fire,
> or may have fired.

> The name, flags, and mode arguments for NaClNameServiceResolve and
> NaClNameServiceResolveAsync are essentially that of the familiar POSIX open(2)
> call, with the following differences: flags may contain, instead of the usual
> O_RDONLY, O_RDWR, etc, values defined in fcntl.h, O_NAMESERVICE. When
> resolving a name with O_NAMESERVICE, the mode argument is ignored. If the
> resolution succeeds, the return value is a name service, which may in turn be
> used with NaClNameServiceChannel. If the resolution fails,
> NaClNameServiceResolve returns -1 and the thread-local variable errno is set
> to indicate the reason for failure.

> To enumerate all names at a name service, use NaClNameServiceEnumerate or
> NaClNameServiceEnumerateAsync. The supplied buffer is overwritten with at most
> nbytes of NUL-terminate string names, and the actual number of bytes written
> is returned, in the case of NaClNameServiceEnumerate; or is supplied as the
> written input argument of the completion callback functor callback/state, in
> the case of NaClNameServiceEnumerateAsync. NB: If written &lt; nbytes, then
> all names in the namespace were returned. Some name services, e.g., those
> associated with file systems, may permit the opening of directories as name
> services.

> All I/O operations using I/O descriptors returned by name services will cause
> the caller to block until the operation succeeds, unless the NaCl thread
> synchronous I/O restriction is in force; in this case, the I/O restricted
> thread(s) will not be able to perform I/O operations unless explicitly
> enabled: such operations will result in EWOULDBLOCK.

> Do not use both the asynchronous and synchronous resolution interfaces, even
> in a temporally disjoint manner.

**EXAMPLES**

> int32_t root_nameservice = NaClNameServiceGetRoot();

> int32_t root_ns_channel = NaClNameServiceChannel(root_nameservice);

> int32_t web_fs_nameservice = NaClNameServiceResolve(root_ns_channel,
> "WebFsPersistentStore", O_NAMESERVICE, 0);

> int32_t manifest_nameservice = NaClNameServiceResolve(root_ns_channel,
> "ManifestFiles", O_NAMESERVICE, 0);

> int32_t fd = NaClNameServiceResolve(manifest_nameservice, "libsdl.so",
> O_RDONLY, 0);

> void \*ResolverThread(void \*state) {

> NaClNameServiceAsyncThread((int32_t) (intptr_t) state);

> return NULL;

> }

> void ResolverCallback(void \*state, int32_t desc, int32_t errno) {

> /\* grab a lock, save desc somewhere, and signal a condvar \*/

> struct ResolverCallbackState \*info = (struct ResolverCallbackState \*) state;

> pthread_mutex_lock(&info-&gt;mu);

> info-&gt;desc = desc; info-&gt;errno = errno;

> pthread_cond_broadcast(&info-&gt;cv);

> pthread_mutex_unlock(&info-&gt;mu);

> }

> pthread_t tid;

> struct ResolverCallbackState \*info = ...;

> pthread_create(&tid, (pthread_attr_t) NULL, ResolverThread,
> web_fs_nameservice);

> if (0 != NaClNameServiceResolveAsync(web_fs_nameservice, "/Nexuiz/SavedGame",
> O_RDWR, 0, ResolverCallback, info)) {

> /\* failed \*/

> }

**SEE ALSO**

> open, close, read, write, fstat, iorestrict

**BUGS**

> stat-like call is missing; TBD.
