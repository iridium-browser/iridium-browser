---
breadcrumbs:
- - /developers
  - For Developers
page_name: lock-and-condition-variable
title: Chrome C++ Lock and ConditionVariable
---

# First a warning: do you really need Locking or CondVars?

Are you sure you need to use explicit locking and condition variables? In Chrome
code, message passing is far more common (via `TaskRunner` and `PostTask`) and
low-level primitives like locks and condition variables should be used only when
necessary.

Some additional motivation:

*   Condition variables are nearly impossible to implement correctly on
            Windows XP or earlier. Our implementation is correct, but _very_
            slow. Whenever you use a CV you are disproportionately harming our
            performance on Windows.
*   A lot of times people just want to wait on a boolean. In such cases,
            if message passing cannot work, please use WaitableEvent instead.

But for the times when you *do* need to use locks and condition variables, this
document will explain the best practices and pitfalls in using them.

(Much of the below was originally written by Mike Burrows.) (TODO: Figure out
how to get anchor links to work)

## Using `Lock` and `ConditionVariable`

### Terminology and basics

The `Lock` class implements a *mut*ual *ex*clusion lock, or *mutex* for short. A
mutex is used to permit only one thread at a time to have exclusive access to
some resource, which is typically some variable or data structure. Mutexes are
so common that many words have been coined to describe their operation.

Each `Lock` `mu` has two basic operations: `mu.Acquire()` and `mu.Release()`.
Conceptually, it has just a single bit of abstract state: the boolean `mu.held`.
When `mu` is created, `mu.held` is `false` and `mu` is said to be *free* or
*unlocked*. `mu.Acquire()` blocks the caller until some moment when `mu` is
free, and then atomically changes `mu.held` from `false` to `true`; `mu` is then
said to be *held* or *locked* by the caller. `mu.Release()` sets `mu.held` to
`false` once more.

Calling `mu.Acquire()` is often referred to as *locking* or *acquiring* `mu`,
while calling `mu.Release()` is referred to as *unlocking* or *releasing* `mu`.
An action performed by a thread while holding `mu` is said to be performed
*under* `mu`. Data structures manipulated under `mu`, and their invariants, are
said to be *protected by* `mu`.

Clients of `Lock` must obey these rules:

1.  Each time a thread acquires `mu` it must later release `mu`.
2.  A thread may not attempt to release `mu` unless it holds `mu`.

Because `mu.Acquire()` acts atomically to change the state of `mu.held` we are
guaranteed that, if these rules are followed, only one thread may hold `mu` at
any given time.

These rules are best followed by bracketing regions of code with matching calls
to `mu.Acquire()` and `mu.Release()` in the same procedure. Such sections of
code are called *critical regions* or *critical sections*, or occasionally
[*monitors*](#monitors) after Hoare monitors, from which mutexes were derived.
(A Hoare monitor is an abstraction that automatically acquires a lock on entry
and releases it on exit.) In Chrome C++ code, many use the idiom `AutoLockl(mu)`,
which acquires `mu` and releases it when `l` goes out of scope. (Less
commonly, `AutoUnlock l(mu)` can also be used.)

Mutexes perform two tasks in concurrent programming. Their primary role is to
protect the invariants of variables and data structures manipulated by multiple
threads (these invariants are often called *monitor invariants*, again recalling
the work of Hoare). The programmer is required to establish the invariant before
releasing the mutex; he can then assume the invariant holds whenever he acquires
the mutex, even if his updates *temporarily* invalidate the invariant during the
critical section. One cannot guarantee the invariant is true in a thread that
does not hold the mutex, because the mutex holder may be changing the monitored
state at that moment. For example, suppose `Lock mu` protects the invariant that
`a + b == 0`. This code is then legal:

    mu.Acquire();
    assert(a + b == 0);
    // invariant assumed to hold
    a++;
    // invariant temporarily invalidated
    b--;
    // invariant restored before mu is released
    mu.Release();

while this code is erroneous:

    mu.Acquire();
    assert(a + b == 0);
    // invariant assumed to hold
    a++;
    // invariant invalidated
    mu.Release();
    // BUG: mu released while invariant invalid
    mu.Acquire();
    b--;
    // attempt to restore the invariant, but the damage is already done
    mu.Release();

The following does not invalidate the invariant, but incorrectly assumes it is true when the lock is not held:

    mu.Acquire();
    assert(a + b == 0);
    // correct: invariant assumed to hold
    mu.Release();
    assert(a + b == 0);
    // BUG: can't assume invariant without lock

The invariant holds only when evaluated on state observed in a single
critical section:

    mu.Acquire();
    assert(a + b == 0);
    // correct: invariant assumed to hold
    temp = a;
    // takes a temporary copy of "a"
    mu.Release();

    mu.Acquire();
    assert(temp + b == 0);
    // BUG: can't assume invariant on state
    // from two distinct critical sections
    mu.Release();

A less obvious role of mutexes is to ensure a sequentially-consistent view of
such data structures on machines with memory systems that are not sequentially
consistent. Mutexes also prevent compiler reordering which could otherwise cause
race conditions.

### Other properties of `Lock`

The call `mu.Try()` either returns `false` or acquires `mu` and returns
`true`. It does not block. If `mu` is free, it is unlikely to return
`false`.

The call `mu.AssertAcquired()` aborts the process in debug mode if `mu` is
not held by the calling thread..

`Lock` is able to synchronize its own deletion. For example, if an object
`*o` contains a `Lock` `mu` and a correctly-maintained reference count `c`,
this code is safe:

    o->mu.Acquire();
    bool del = (--(o->c) == 0);
    o->mu.Release();
    if (del) { delete o; }

Not all lock implementations have this property, so it
should not be taken for granted when porting code. (To provide this
property, we guarantee that `mu.Release()` does not touch `mu` after the
atomic moment at which `mu` becomes free.)

`Lock` is not re-entrant (also known as not recursive). See
[below](#reentrant).

### Condition variables

Condition variables are a means for blocking a thread until some condition has
been satisfied. Viewed in isolation, a condition variable allows threads to
block and to be woken by other threads. However, condition variables are
designed to be used in a specific way; a condition variable interacts with a
mutex to make it easy to wait for an [arbitrary condition on state protected by
the mutex](#condprotect). Chrome's C++ condition variables have type
`ConditionVariable`.

Suppose that a thread is to wait for some boolean expression *cond_expr* to
become true, where the state associated with *cond_expr* is protected by `Lock
mu`. The programmer would write:

// Waiter mu.Acquire(); while (!cond_expr) { cv.Wait(); // mu was passed to cv's
constructor } // cond_expr now holds ... mu.Release(); The `Wait()` call
atomically unlocks `mu` (which the thread must hold), and blocks on the
condition variable `cv`. When another thread signals the condition variable, the
thread will reacquire the `mu`, and go around the [mandatory
while-loop](#whileloop) to recheck *cond_expr*.

Another thread that makes *cond_expr* true might execute:

    // Waker mu.Acquire();
    Make_cond_expr_True();
    // cond_expr now holds
    cv.Signal();
    mu.Release();

The call to `Signal()` wakes at least one of the
threads waiting on `cv`. Many threads may be blocked on a condition variable at
any given time; if it makes sense to wake more than one such thread
`Broadcast()` can be used. (However, this may lead to contention and poor
performance if all waiting threads use the same lock; a possibly better approach
to getting a lot of threads out of `Wait()` is to have each thread (upon exiting
`Wait()`) call `Signal()` to free up another `Wait()`ing thread.)

A single condition variable can be used by threads waiting for different
conditions. However, in this case, `Broadcast()` *must* be used when any of the
conditions becomes true, because the `ConditionVariable` implementation cannot
otherwise guarantee to wake the correct thread. It can be more efficient to use
one condition variable for each different condition; any number of condition
variables can be used with a single mutex.

Both `Signal()` and `Broadcast()` are efficient if there are no threads to wake.
(TODO: verify this) Clients should call `Signal()` or `Broadcast()` [inside the
critical
section](http://www.chromium.org/developers/lock-and-condition-variable#TOC-Why-put-Signal-inside-the-critical-section-)
that makes the condition true.

The call `TimedWait()` allows a thread to wait until either a condition is true,
or until some time has elapsed. Like `Wait()`, `TimedWait()` *always* reacquires
the mutex before returning. Example use:

    static const int64 kMSToWait = 1000;
    // we'll wait at most 1000ms TimeDelta
    left_to_wait = base::Milliseconds(kMSToWait);

    // ms to wait at any given time
    Time deadline = Time::Now() + left_to_wait;
    mu.Acquire();
    while(!cond_expr && left_to_wait.InMilliseconds() > 0) {
      cv.TimedWait(left_to_wait);
      left_to_wait = deadline - Time::Now();
    }
    if (cond_expr) {
      // cond_expr true
    } else {
      // cond_expr false; 1000ms timeout expired
    }
    mu.Release();

### Recommended usage

Variables accessed by more than one thread and written by at least one thread
should be protected consistently by a `Lock`.

An exception to this rule is that fields may be initialized in constructors
without a mutex, since no other thread should be able to reference the memory at
that time.

#### Recommended commenting convention

There are two main dangers when using threads and mutexes: deadlocks and data
races. These can be avoided using some simple rules and adding small comments to
variable and procedure declarations. You are strongly advised to use such
comments; they may seem tedious to write, but they help tremendously in avoiding
errors. The particular commenting conventions shown below are derived from the
work of Nelson and Manasse.

Critical sections should almost always start and end in the same routine.
That is, if a routine acquires a lock, it should release it before it
returns, and it should release no locks that it does not itself acquire.
This is normally achieved by writing `Acquire()` and `Release()` calls in
pairs, or by using `AutoLock l(mu)`, which automatically releases `mu` when
`l` goes out of scope.

Every shared variable/field should have a comment indicating which mutex
protects it:

    int accesses_;
    // account of accesses (guarded by mu_)
    // or a comment explaining why no mutex is needed:
    int table_size_; // no. of elements in table (readonly after init)

Every mutex should have a comment indicating which variables and also any
non-obvious invariants it protects:

    // protects accesses_, list_, count_
    // invariant: count_ == number of elements in linked-list list_
    Lock mu_;

Think of the matching comments on
variables and mutexes as analogous to matching types on procedure arguments
and parameters; the redundancy can be very helpful to later maintainers of
the code.

Whenever a thread can hold two mutexes concurrently, either one (or both) of
the mutexes should be commented with `acquired before` or `acquired after`
to indicate which mutex must be acquired first:

    Lock mu0_;
    // protects foo_ (acquired before mu1_)

If the mutex acquisition order is not consistent, [deadlock may result](#deadlock).

Each routine should have a comment indicating which mutexes must and must
not be held on entry. This allows implementors to edit routines without
examining their call sites, and allows clients to use routines without
reading their bodies.

Never hold locks when invoking a callback, as the callback may call into the
module once more, leading to deadlock. (Violations of this rule should be
extremely rare and conspicuously commented in the module's interface.)
Comments should indicate what threads can or cannot be used for callbacks:

    // Call cb.Run() in "ms" milliseconds.
    // cb.Run() is called in a private thread; it will not be called
    // from the thread calling RunAfter(), even if ms==0.
    void RunAfter(Closure cb, int ms);

In rare cases, it may be useful for a routine to acquire a lock and return
without releasing it, or to release a lock (perhaps temporarily using
`ConditionVariable::Wait`) that it did not acquire. Such routines may
surprise clients, and should be commented clearly in interfaces. Note that a
routine that acquires a lock and returns without releasing it is practically
a locking primitive and should be commented as such.

Every condition variable should have a comment indicating when it is
signalled:

    ConditionVariable non_empty_; // signalled when the queue becomes non-empty

In some cases, exclusive access to data is ensured by referencing it only
from one thread. (See the section on [message passing](#message).) The
thread can be thought of as playing the part of a mutex; you should name the
thread and use the name in comments as if it were a lock.

    int queue_length_; // length of receive queue (under net_thread) ...
    // Process one packet from the queue.
    // L >= net_thread void
    ProcessPacket() { ... }

In very rare cases, a variable may be protected by more than one mutex. This
means that the variable may be read while holding any of the mutexes, but
may be written only when all the mutexes are held. You should document it
clearly in the comments.

    int bytes_left_; // bytes remaining in queue (under mu_ and net_thread)

If these conventions are followed it is straightforward to tell what locks are
held at any point in a routine by reading the routine and its comments. By
reading the comments on shared variables and mutexes, it is then possible to
tell that all variable accesses are correctly protected by a mutex, and that
mutexes are acquired in the correct order.

Without such comments, working with mutexes is significantly harder. We
recommend their use.

#### Performance hints

##### Critical sections should not be too long

Normally, you should hold mutexes for short periods (nanoseconds to
microseconds) at a time, and the mutexes should be free most of the time, often
approaching 99%. To achieve this, it's best to avoid doing slow operations such
as I/O inside critical sections---assuming it is not the purpose of the mutex to
serialize the I/O, of course.

Another, more complex technique is to make the locking more *fine-grained* by
employing more locks, each protecting a subset of the data.

Other transformations may help, such as breaking a critical section in two, or
arranging to perform long-running operations on state that is local to a thread.

##### Lock acquisitions are cheap but not free

A lock acquisition is generally more expensive than a cached variable access,
but less expensive than a cache miss. If a mutex is acquired and released too
often (say, more than a hundred thousand times per second) the overhead of these
operations themselves may start to be significant in CPU profiles.

Frequent acquisition can be avoided by combining critical sections, or by
delaying operations on shared state by buffering them in memory local to a
single thread. For example,
[tcmalloc](http://goog-perftools.sourceforge.net/doc/tcmalloc.html) uses a
per-thread cache of memory to avoid locking overhead on every allocation.

### Pitfalls

#### Deadlocks

A deadlock (sometimes called a *deadly embrace*) occurs when an *activity*
attempts to acquire a limited *resource* that has been exhausted and cannot be
replenished unless that activity makes progress.

When considering deadlocks involving only mutexes, each activity is typically
represented by a thread, and the resources are mutexes that are exhausted when
held, and replenished when released. However, other possibilities exist: In a
[message passing](#message) environment, an activity may be represented by a
message, and a resource may be represented by a bounded-length message queue or
a bounded-size thread pool. When both mutexes and message passing are used,
deadlocks may involve combinations of these activities and resources.

The simplest mutex-related deadlock is the *self-deadlock*:

    mu.Acquire();
    mu.Acquire();
    // BUG: deadlock: thread already holds mu Deadlocks

involving two resources, such as a mutex and a bounded-sized thread pool, are
easily generated too, but deadlocks involving three or more resources are less
common. A two-mutex deadlock results when thread T0 attempts to acquire M1 while
holding M0 at the same time that thread T1 attempts to acquire M0 while holding
M1; each thread will wait indefinitely for the other.

##### Debugging deadlocks

Fortunately, deadlocks are among the easiest bugs to debug and avoid. Debugging
is typically easy because the address space stops exactly where the bug occurs.
A stack trace of the threads is usually all that is is required to see what the
threads are waiting for and what resources they hold. (Deadlocks involving
messages in queues can be harder to spot.)

##### Avoiding deadlocks

Deadlocks can be avoided by disallowing cycles in the resources' exhaust-before
graph; this can be done by imposing a partial order on the graph. If an activity
can exhaust resource R0 and then attempt to use a resource R1 that may be
exhausted, then we say that R0 *precedes* R1 (and R1 *follows* R0) in the
exhaust-before graph. To guarantee no deadlocks, it is sufficient to guarantee
that if R0 *precedes* R1, then R1 never *precedes* R0. That is, for all pairs of
resources R0 and R1, either R0 and R1 are unordered (neither *precedes* the
other), or their ordering is unambiguous (one *precedes* the other, but not vice
versa).

Considering only mutexes, we can avoid deadlocks by ensuring that the
acquired-before graph is a partial order and is therefore free of cycles. In
practice, we simply pick an order for any two mutexes that can be held
simultaneously by the same thread, and comment the code with this choice.

As described [above](#comments), if a thread does `mu0.Acquire(); mu1.Acquire();
` then we should comment the declarations of `mu0` and `mu1` with either
`acquired before` or `acquired after` (or both). Because we wish our code to be
modular, our comments should also indicate what locks a caller must or must not
hold on entry to a routine. Combined, these comments allow the programmer to
know whether he is about to violate the locking order by acquiring a mutex or
calling a routine. Experience shows that if this convention is followed,
deadlocks are usually both rare and easy to correct.

A particularly important rule of thumb for deadlock avoidance is [never hold a
lock while invoking a callback](#callback). More generally, try to avoid holding
locks for long periods, and across calls to other levels of abstraction that you
do not fully understand. (That is, you might hold a lock access an access to a
hash table, but you should not hold a lock across a call to a complex
subsystem.)

#### Races

Races occur in three main ways:

A shared variable is accessed without being protected consistently by a
mutex. The reasons for the problems are discussed at length [below](#why).
This error can be avoided with the conventions [already
described](#comments); simply ensure that each shared variable is accessed
only when its protecting mutex is known to be held. Such races can be
detected automatically by
[ThreadSanitizer](http://code.google.com/p/data-race-test/wiki/ThreadSanitizer)
as described [below](#racedebug).

A critical section modifies protected state but does not [preserve the
monitor invariant](#invariant_bug). Such bugs are rare if invariants are
commented correctly.

A critical section reads protected state, which is then encoded in a
temporary variable or the program counter. Then the lock is released, then
reacquired and the state from the previous critical section is used as
though still valid:

    string name_; // guarded by mu_
    size_t NameLen() {
      AutoLock l(this->mu_);
      return this->name_.size();
    }

    // Requires 0 <= i < this->name_.size()
    int CharFromName(size_t i) {
      AutoLock l(this->mu_);
      return this->name_[i];
    }
    ...
    size_t len = this->NameLen();
    int x = 0;
    if (len > 0) {
      // BUG: temporary len encodes protected state from a previous
      // critical section that is used inside another.
      // The length of name_ may have changed since len was assigned.
      x = this->CharFromName(len - 1);
    }
    ...

This is the most insidious form of race, and the best known way
to avoid them is vigilance. Fortunately, they are quite rare. There are
algorithms that can detect such races using data flow analysis, but as yet
none has been applied to C++.

### Debugging

`Lock` and `ConditionVariable` have various features to aid debugging.

#### Assertions

`mu.AssertAcquired()`: abort in debug mode if `mu` is not held by the
calling thread.

#### Race detection

Race detection requires an external tool. One such tool is
[ThreadSanitizer](http://code.google.com/p/data-race-test/wiki/ThreadSanitizer),
which is a dynamic race detector based on the [Valgrind](http://go/valgrind)
binary translation framework. See [this
page](http://code.google.com/p/data-race-test/wiki/ThreadSanitizer) for more
details on how to use it with Chrome.

### Examples

The following show simple implementations of reader-writer locks and
producer-consumer queues using condition variables.

#### Reader-writer lock

The example below could be improved in various ways at the cost of clarity. In
particular, they allow readers to starve writers.

    class RWLock {
      public:
        RWAcquire() : lockers_(0), lock_free_(&mu_) {}
        ~RWAcquire() {}
        void WriteAcquire() {
          // acquire a write lock
          this->mu_.Acquire();
          while (this->lockers_ != 0) {
            this->lock_free_.Wait();
          }
          this->lockers_ = -1;
          this->mu_.Release();
        }
        void ReadAcquire() {
          // acquire a read lock
          this->mu_.Acquire();
          while (this->lockers_ < 0) {
            this->lock_free_.Wait();
          }
          this->lockers_++; this->mu_.Release();
        }

        void Release() {
          // release lock (either mode)
          this->mu_.Acquire();
          this->lockers_ = (this->lockers_ == -1? 0 : this->lockers_ - 1);
          if (this->lockers_ == 0) {
            // if lock became free, wake all waiters
            this->lock_free_.Broadcast();
          }
          this->mu_.Release();
        }
        private: Lock mu_;
        // protects lockers_ int lockers_;
        // 0 => free, -1 => writer, +ve => reader count
        ConditionVariable lock_free_; // signalled when lockers_ becomes 0
    };

#### Producer-consumer queue

    class PCQ {
      // a bounded, blocking FIFO producer-consumer queue
      public:
      PCQ(int n) : max_count_(n), non_full_(&mu_), non_empty_(&mu_) {}
      ~PCQ() {
        CHECK_EQ(this->queue_.size(), 0);
      }
      // error if queue is not empty
      // waits until queue is not full, then adds value to its end
      void Add(void *value) {
        this->mu_.Acquire();
        while (this->queue_.size() == this->max_count_) {
          this->non_full_.Wait();
        }
        this->non_empty_.Signal(); // no need to Broadcast.
        // (only one remover can consume this item)
        // Could use:
        // if (this->queue_.size() == 0) { this->non_empty_.Broadcast(); }
        this->queue_.push_back(value);
        this->mu_.Release();
      }
      // waits until this queue is non-empty, then removes and returns first value
      void *Remove() {
        this->mu_.Acquire();
        while (this->queue_.size() == 0) {
          this->non_empty_.Wait();
        }
        this->non_full_.Signal();
        // no need to Broadcast. (only one adder can consume this gap)
        // Could use:
        // if (this->queue_.size() == this->max_count_) {
        //  this->non_full_.Broadcast();
        // }
        void *value = this->queue_.front();
        this->queue_.pop_front();
        this->mu_.Release();
        return value;
      }
      private:
        Lock mu_; // protects *queue_
        // protects invariant 0 <= queue_.size() <= max_count_
        deque<void *> queue_; // list of queued items const int
        max_count_; // max number of items in *queue_ (readonly after init)
        ConditionVariable non_full_; // signalled when queue becomes non-full
        ConditionVariable non_empty_; // signalled when queue becomes non-empty
    };

## Why are mutexes the way they are?

### Why use a mutex when accessing shared data?

It is perilous to access data that another thread may be modifying concurrently.
Consider accesses to a C++ `string`. A well-formed `string` may have invariants,
such as its length field indicating the true length of the string it represents.
Such invariants may be temporarily broken by the `string` implementation when an
update occurs. Clearly, one thread may not be allowed to read a `string` that is
being written by another, as it may not observe a length consistent with the
stored bytes. If the string is accessed only under a mutex `mu`, the `string`'s
invariants become `mu`'s monitor invariants, and each thread will see a
well-formed `string`.

It is tempting to assume that mutexes are unnecessary when there is no obvious
monitor invariant to protect. Consider a variable or field with type `double`.
One might expect to be able to read and write this variable from multiple
threads without the protection of a mutex, but this is not safe:

Many machines, including the x86, do not guarantee to access a `double`
atomically. (A stack-allocated double need not be naturally-aligned by the
compiler, potentially leading to two memory operations for a single access.)
Thus, there *is* an invariant we need to protect: that the `double` is
well-formed.

On some machines seemingly obvious data-dependency properties do not hold
without the cross-thread synchronization provided by a mutex; a thread might
read a well-formed `double` but get a value from an apparently earlier time.
This comment applies to all types, including integers, pointers, and even
["atomic" types](#atomic) provided by the language or runtime.

A variable's concrete type may change as a program is maintained, and this
change may be hidden by a `typedef`.

In short, data accessed by multiple threads should be protected with a mutex. To
do otherwise is to court disaster.

Despite this advice, some programmers enjoy the intellectual challenge of using
lower-level primitives (["atomic" types](#atomic), compare-and-swap, memory
barrier) instead of mutexes. There are many problems with such code:

*   it usually does not work.
*   it is often no faster overall than using mutexes.
*   it may rely on assumptions about the hardware or compiler, or both.

However, the most important reason not to use such code is that it is
complicated. Even if the author understands it, the next maintainer of the code
may not. Worse, he may *think* he understands it.

The best way to avoid locking is to avoid shared mutable state. When shared
mutable state is needed, use a mutex. If you experience [lock
contention](#perf), consider using more mutexes, each protecting less data (that
is, make the locking finer-grained). If you feel you must access a shared
mutable variable without a mutex, and have data that shows it is worth the
maintenance expense of doing so, ask an expert how to do it.

### Why can the holder of a `Lock` not reacquire it?

Some mutex implementations, notably that of Java, and the Windows
CRITICAL_SECTION are called *reentrant* or *recursive*. They allow the holder of
a mutex to reacquire the mutex without deadlocking by maintaining an internal
*acquisition count* and *holder* identity instead of the `held` boolean. The
mutex is free when the count is zero. The acquisition count keeps track of the
number of acquisitions performed by the holder; the holder is required to
release the mutex the same number of times it acquired it to make the mutex
free. This bookkeeping allows a method of an object to call other methods of the
same object while holding the lock, even if those other methods acquire the
lock. We do not allow this apparently convenient usage in `Lock` not because of
the small additional cost of maintaining the counter, but because of two
problems.

Recall that a mutex's primary purpose is to allow the programmer to maintain
monitor invariants, and that the programmer may assume a monitor invariant just
after acquiring the appropriate mutex. Consider a Java method M0 that acquires a
mutex `mu` protecting invariant *Inv*. The author of M0 is entitled to assume
*Inv* at the moment he acquires `mu`. Now consider another method M1 that
acquires `mu`, invalidates *Inv* during an update, calls M0, restores *Inv*, and
releases `mu`. M0 assumes *Inv*, but *Inv* is untrue when M0 is called, so M0
fails. Remember that both M0 and M1 may have multiple implementations written by
multiple authors over years, and perhaps multiple implementations in the same
program, due to inheritance. The source code of M0 may not be available to the
author of M1, or vice versa. Without remarkably good discipline and
documentation standards, the programmers may not understand why M0 is not
functioning correctly.

If a programmer attempted to do the same thing with a non-reentrant mutex such
as `Lock`, his code would instantly deadlock and a stack trace would show that a
thread is attempting to reacquire a lock it already holds. Not only is the error
discovered immediately, but the fix is usually trivial: write a small method M0
that acquires `mu` and calls a private method M0', which does the real work,
assuming *Inv* but without acquiring `mu`. The specifications of M0 and M0'
differ only in their locking behaviour, so the programmer almost always
documents this difference, often in the names of M0 and M0'. The presence of the
additional method and the corresponding name or comment provides an additional
reminder to the author of M1. He realizes that by calling M0' rather than M0, he
has the burden of establishing *Inv* before the call---it is not guaranteed
automatically by the monitor invariant. This solution is not a panacea, but
disallowing reentrancy at least makes the error apparent, rather than hiding it.

The second problem with reentrancy is associated with condition variables. In
the example above, imagine that M0 waits on a condition variable and thus
effectively contains more than one critical section. Normally M0 will work, but
if called from M1 with `mu` held, it is unclear what happens, and neither
outcome is satisfactory. If the wait() primitive decrements `mu`'s acquisition
count by one, `mu` does not become free, the condition never becomes true, and
the thread deadlocks. If instead the acquisition count is set to zero by
`Wait()`, `mu` becomes free during a critical section initiated by M1. This is
likely to cause M1 to malfunction silently. In the non-reentrant case, M0 must
be split into M0 and M0' as before. Since M0' waits on a condition variable, it
now has an interesting specification: it temporarily releases a mutex that it
did not acquire. This is unusual, and usually very dangerous, so one might
expect a comment to that effect. This comment then tells the author of M1 that
he must be especially careful if he calls M0'.

### Why not use monitored modules? (or automatically locked objects, locking pointers, lock-free data structures, ...)

It seems attractive to automate the acquisition and release of mutexes by
declaring somehow that a mutex will be acquired on entry to a module and
released on exit, as in a Hoare monitor. This can be used for trivial cases, but
even quite common examples require more complex locking.

Consider a table `*t` mapping strings to integer counts. The table might have
methods `insert()`, `lookup()`, `remove()`. If the table provides its own
synchronization, perhaps inserted automatically in each method by some
mechanism, we eliminate data races within the table itself, but this does not
help the client. Consider this code, which increments the count for "foo" in the
table `*t`:

    int *v = t->lookup("foo");
    // safe because *t is a monitor

    if (v != 0) {
      (*v)++;
      // BUG: data race: unlocked increment
    } else {
      t->insert("foo", 1);
      // safe because *t is a monitor
    }

If the client does not use his own mutex,
counts may be missed. If he does, the synchronization inside `*t` is redundant.
Thus, monitored modules are rarely helpful.

The implementors of SGI STL made the [same
observation](http://www.sgi.com/tech/stl/thread_safety.html).

A further problem is that the designers of automatic locking mechanisms often
desire to reduce the amount of typing needed to implement a monitor, rather than
to improve the readability and maintainability of the code. All too often, these
two desires are in conflict; some code is [more readable](#autolock) if one can
tell when a lock is released.

### Alternatives to mutexes

There are a number of ways to handle concurrency, and ways to avoid it
altogether. Of the various possible models, only two permit high-performance
implementations that can use multiple CPUs and sharing of resources, and still
allow large programs to be built from smaller components while maintaining
abstraction boundaries. These models are "threads + mutexes +
condition-variables", and "threads + message-passing". These two can be used
together, and often are.

#### Message passing

One can associate data with threads, so that each thread *owns* some variables
and data structures; a variable is accessed only by its owning thread. Other
threads wishing to access the data then communicate with the owning thread via
*message passing*, such as Chrome's `TaskRunner`.

This style is a dual of the mutex-based style (see Lauer and Needham's oft-cited
paper on the subject): A message-send corresponds to acquiring a mutex; running
in a critical section corresponds to executing code within the owning thread;
and receiving a reply corresponds to releasing the mutex. Thus, the most obvious
difference between the approaches is that in message-passing all the code that
manipulates a particular data item is brought together into one thread, while
with mutexes the data accesses can be interleaved with other code.

Message passing and mutexes can be intermixed; often one is preferred either
because the author is comfortable with the style, or because one leads to a
clearer module than the other. The message-passing model tends to work well when
there is a natural resource that already serializes accesses (such as an I/O
device), a linear state machine best expressed as a single routine, or when
critical sections are long. Mutexes work well when critical sections are short
and may be invoked in many places, or when reader-writer locks can be used
effectively. In Chrome code, message passing is much more common (via
`TaskRunner` and `PostTask`) and low-level primitives like locks and condition
variables are used only when necessary.

Both models allow high-throughput implementations, and both can suffer from both
races and deadlocks. Deadlocks can often be eliminated in the message-passing
model by using unbounded queues and/or threadpools.

#### Atomic types and atomic operations

Many runtimes and languages (including C++11) provide atomic operations, such as
compare-and-swap, and "atomic" types that can be read and written atomically.
Atomic operations and types are much harder to use than one might first think,
and they should not be used in normal code. Unfortunately, programmers are
attracted to them for various reasons:

Programmers enjoy the intellectual puzzle of using these operations. This
leads to clever, but ill-advised, and often broken code.

Algorithms involving atomic operations are extremely subtle. For example, a
general-purpose, efficient, lock-free, singly-linked list algorithm took
significant research to discover, and requires care to implement. Almost all
programmers make mistakes when they attempt direct use of atomic operations.
Even when they don't make mistakes, the resulting code is hard for others to
maintain. Both CPUs and compilers can rearrange reads and writes in ways
that lead to subtle race conditions. The simple-sounding pattern of
[double-checked
locking](http://en.wikipedia.org/wiki/Double-checked_locking) is actually
extremely subtle and is usually implemented incorrectly.

Programmers assume that locking is expensive, and that using atomic
operations will be more efficient. But in reality, acquiring and releasing a
lock is cheaper than a cache miss; attention to cache behaviour is usually a
more fruitful way to improve performance. Furthermore, lock-free data
structures are often more expensive than using locks. This is because a lock
allows arbitrary changes to be made to a complex data structure; if the same
changes must be made without a lock, the result is likely to take more
atomic read-modify-write instructions, not fewer.

People wish to avoid lock contention when concurrency is high. This is best
solved by partitioning locked data structures to avoid lock contention. For
example, it is easier, more efficient, and [more useful](#monitors) to build
a high-concurrency hash table from many normal hash tables, each with its
own lock, than to build one lock-free hash table using atomic operations.

Atomic operations should be used in only a handful of low-level data structures,
written by a few experts, and then reviewed and tested thoroughly.
Unfortunately, many attempt to write lock-free code, and almost always this is a
mistake. Please do not fall into this trap: do *not* use atomic operations or
types---if you do, you will make mistakes, and you will cost the company time
and money.

#### A single thread

A process that uses only a single thread requires no mutexes, and this is often
the best approach for simple programs that do not require high performance or
that are inherently sequential. However, it is usually not the best choice for
large programs, or when high performance is required.

A single-threaded application can use only one CPU, which typically makes it
far slower than other options, even when the overheads of locking are taken
into account. If the application is simple enough, one may be able to run
multiple copies of the same program on each machine, but this introduces two
inefficiencies: cross-address-space context switches are more expensive than
thread context switches because threads share TLB entries while address
spaces do not; and the address space separation precludes sharing some
resources (caches, ports, etc.).

Some programs, such as network servers, exhibit natural concurrency: they
must deal with many concurrent client requests, so some mechanism is needed
to allow this.

##### The fallacy of thread context-switch cost

Some try to argue that it is significantly cheaper to multiplex a single thread
than to use multiple threads because a single thread requires no thread context
switches. Such an argument stems from confusion about what constitutes a
"context switch" and what contributes to its cost. A context switch is simply
the act of multiplexing the processor between multiple activities; its dominant
costs are similar regardless of whether this is done in kernel-mode or in
user-mode:

When a program switches to a new activity, it incurs cache and TLB misses by
touching the data and instructions associated with a new activity. This cost
is the most important, and is essentially the same regardless of whether the
new activity takes place in a different thread or in the same thread. The
cost occurs not only when a multithreaded program performs a thread context
switch, but also when an [event driven](#event) program processes a new
event, or when a [co-operative multithreaded](#coop) program switches
context in user-space. Multithreaded programs rarely context switch due to
time-slicing because time-slices are large.

The cost of user-kernel mode switches is sometimes counted as part of the
context-switch cost between threads. However, multithreaded programs usually
context switch when they have *already* entered the kernel for other
reasons---typically, via some system call or to service an interrupt. A
single-threaded program incurs these same mode switches, and thus they are
common to all models. One might expect mutex and condition variable calls to
add to the number of system calls, but this effect is modest because
uncontended mutexes induce no system calls, while contended mutexes and
condition-variable operations should be relatively rare.

Context switches between address spaces are more expensive because TLB
entries must be replaced.

To summarize, if a single address space is used, the costs of switching between
activities are nearly independent of the number of threads used within that
address space; the technique that leads to slowest execution is to run multiple
copies of a single-threaded program.

##### The event-driven style

To handle concurrent activities in a single thread, some programmers adopt a
style variously known as *event-driven*, *state-machine* or
*continuation-passing*: One can decompose the actions for a given request into a
graph of *handler* routines that never block for I/O internally, but rather
specify which handlers should be invoked when pending I/O requests complete.
This approach can be made to work and may even be straightforward in simple
programs, but it has bad effects on readability, modularity, and abstraction, as
well as using only one CPU. To see the problems with the event-driven style,
consider the code ... for (int i = 0; i != 10; i++) { foo(i); } ... Now imagine
that the third-party library routine `foo()` is modified at some future time to
improve its functionality or average performance. Imagine that a side-effect of
this improvement is that occasionally `foo()` must perform some blocking I/O
that should not be performed within a handler. Neither the author of the
for-loop nor the author of the new implementation of `foo()` has done anything
unusual, and yet the program may show poor throughput or even deadlock in an
event-driven environment. Even subtler changes can have undesirable effects.
Imagine that `foo()` includes a call to `fprintf()`; if one day the output
stream is redirected to a device with high-throughput but high-latency, the
program's throughput will drop precipitously because `foo()`'s latency cannot be
hidden in the event-driven model without rewriting both `fprintf()` and `foo()`.

We can fix the performance problem if we change `foo()`'s signature to include a
continuation to be called when `foo()` completes. However, this is not a local
change: the loop must be restructured so that the loop variable is encoded in
the continuation state. Worse, *every* use of `foo()` must be similarly modified
not only to handle the new signature, but to break the calling procedure into
two---the prefix before the call to `foo()`, and the continuation. Thus, a
*local* change in `foo()` has affected an arbitrary number of modules in a
significant way: the event-driven style does not preserve modularity.

Notice how this differs from the multi-threaded case. Just as the event-driven
style style requires that `foo()` be non-blocking, the multithreaded style
requires that `foo()` be thread-safe. However, this constraint is easier to live
with. First, most libraries are thread-safe either naturally or by design, while
few are designed for use in event-driven systems. (For example, `fprintf()` is
thread safe, but provides no callback mechanism.) Second, if `foo()` were not
thread safe, calls to it can be made safe *either* by a change to `foo()` *or*
by wrapping `foo()` with a new routine `foo()` that acquires a lock before
calling the old `foo()`. Either change is local, and does not affect other
aspects of the interface, so modularity is preserved.

The problem with the event-driven style is worse for routines like `printf()`
whose signatures cannot be changed lightly. Even more worryingly, some I/O
methods cannot be made efficient in a single-threaded event-driven system *even
with arbitrary restructuring of the entire program*. For example, while one can
with effort write a continuation-passing equivalent of `printf()`, memory-mapped
I/O and programmed I/O have no such equivalent.

A further problem with the event-driven style is that the resulting code becomes
quite difficult to understand, maintain, and debug. This is primarily because it
is harder to tell from reading the code which routine caused the current one to
be called, and which routine will be called next. Standard debugging and
performance tools become less effective too, as they rarely have support for
tracing through continuations, and continuation mechanisms rarely maintain call
history. In contrast, non-event-driven programs maintain much of their state and
history conveniently on the thread stacks; debugging tools can reveal a great
deal simply by displaying stack traces.

##### Co-operative multithreading

An alternative style called *co-operative multithreading* allows the programmer
to use multiple threads on a single CPU. The scheduler guarantees that no two
threads can run concurrently, and guarantees never to pre-empt a thread that has
not blocked. In theory, this allows mutexes to be omitted: the code `a++; b--;`
will always execute atomically. Unfortunately, reliance on this property makes
the code more fragile. For example, because any I/O may block, `a++;
printf("bar\n"); b--;` probably does not execute atomically, and `a++; foo();
b--;` may or may not execute atomically, depending on the implementation of
`foo()`. Thus, co-operative multithreading without explicit synchronization can
lead to code in which a bug may be introduced to one module by adding a debug
printf-statement in another module. If explicit synchronization is used, the
technique becomes equivalent to the straightforward use of threads.

For these reasons, unless a program is quite simple it usually pays in both
performance and maintainability to use multiple threads, and to protect shared
variables explicitly with mutexes or to communicate between threads with
messages.

### Why is `cv.Wait()` always in a while-loop?

Hoare's original condition variables did not require the while-loop, but modern
versions require it for somewhat subtle reasons:

The presence of the while-loop allows one to tell by *local* inspection that
the condition is true when the loop exits. Hoare's original precise
semantics required inspection of all code that could potentially call
`Signal()`, which made some errors rather harder to debug.

The while-loop allows clients to do spurious wakeups, which gives the
programmer the opportunity to trade performance for ease of programming.
Suppose he arranges for threads *always* to signal the condition variable
when they modify protected state, rather than only when they make a specific
condition true. This allows modularity between waiters and wakers: the
wakers don't need to know what conditions wakers are waiting for, and each
waiter can wait for a different condition without affecting the code of the
wakers.

The while-loop allows the condition variable implementation more freedom to
schedule woken threads in any order. Consider a thread T0 that wakes thread
T1 that was waiting for condition C. If the runtime semantics guarantee that
T1 will enter the critical section next, T1 can assume C. But context
switches have overhead, so it is usually more efficient merely to add T1 to
the run queue while continuing to run T0 and perhaps other threads, which
may then enter the critical section before T1. If any of those threads
falsifies C, T1 must not assume C on entering the critical section;
scheduling has made it appear that it has received a spurious wakeup. The
while-loop ensures that T1 tests C, and continues only if C is really true.
Thus, the while-loop effectively allows more freedom in choosing an
efficient scheduling discipline.

Timed waits become less error-prone. A timed wait may cause a thread to wake
before its condition C is true. Suppose the programmer forgets to test for a
timeout. If he is forced to use a while-loop, his thread will go to sleep
again and his program will probably deadlock, allowing easy detection of the
bug. Without the while-loop, the thread would falsely assume C, and cause
arbitrarily bad behavior.

### Why must the condition used with `cv.Wait()` be a function of state protected by the mutex?

Consider a thread W waiting for a condition `cond_expr` to become true:
mu.Acquire(); while (!cond_expr) { cv.Wait(); // mu was passed to cv's
constructor } // cond_expr now holds ... mu.Release(); If `cond_expr` is not a
function of state protected by `mu`, two bad things can happen:

Suppose that thread W finds `cond_expr` false, and is about to call
`cv.Wait()`. If the state associated with `cond_expr` is not protected by
`mu`, another thread can make `cond_expr` true and call `cv.Signal()` before
W calls `cv.Wait()`. This means that W may block indefinitely in `Wait()`,
even though `cond_expr` is true (only a thread currently in `Wait()` is
woken by a call to `Signal()`).

Suppose that thread W finds `cond_expr` true, and is about to execute the
code labelled "cond_expr now holds". If the state associated with
`cond_expr` is not protected by `mu`, another thread can make `cond_expr`
false before W runs the rest of the code, so W cannot assume `cond_expr`.
This negates the purpose of the condition variable, which was to give W a
guarantee about `cond_expr`.

### Why put `Signal()` inside the critical section?

In most cases, it is correct to put `Signal()` after the critical section, but
in Chrome code it is *always* both safe and efficient to put it inside the
critical section. (TODO: verify this)

Some texts recommend putting `Signal()` after the critical section because this
makes it more likely that the mutex is free when a thread attempts to reacquire
it on return from `Wait()`. If the `Signal()` were inside the critical section,
a naive implementation might wake the thread which could then block once more on
the mutex held by the very thread that woke it.

Chrome's condition variables (and most reasonable implementations) detect this
case, and delay waking the waiting thread until the mutex is free. (TODO: verify
this) Hence, there is no performance penalty for calling `Signal()` inside the
critical section.

In rare cases, it is incorrect to call `Signal()` after the critical section, so
we recommend always using it inside the critical section. The following code can
attempt to access the condition variable after it has been deleted, but could be
safe if `Signal()` were called inside the critical section.

    struct Foo {
      Lock mu;
      ConditionVariable cv;
      bool del;
      ...
    };
    ...
    void Thread1(Foo *foo) {
      foo->mu.Acquire();
      while (!foo->del) {
        foo->cv.Wait();
      }
      foo->mu.Release();
      delete foo;
    }
    ...
    void Thread2(Foo *foo) {
      foo->mu.Acquire();
      foo->del = true;
      // Signal() should be called here
      foo->mu.Release();
      foo->cv.Signal(); // BUG: foo may have been deleted
    }

### Why should implementors of mutexes pay attention to mutex performance under contention?

Clients should avoid lock contention, because contention necessarily implies
less parallelism; some threads are blocked while another executes the critical
section. Because clients must avoid contention, some implementors of mutexes pay
less attention to the performance of mutexes under contention. However,
contention is sometimes encountered despite clients' best efforts. For example:

A network server may become overloaded or see a changed pattern of use,
causing a mutex to be used more than it normally would.

A program may be run on an upgraded machine with more CPUs, causing
contention on a mutex that was previously lightly loaded.

Software developers encourage abstraction between parts of our programs, so
the authors and clients of modules may have different expectations of how
the module will be used. In particular, a client may cause contention on a
mutex that he is unaware of.

While it is important for clients to fix contention to avoid loss of
parallelism, that loss of parallelism should be their main consideration. The
performance of the mutex itself should not degrade precipitously, even when
heavily contended. That is: an overloaded server should recover from overload if
the load drops once more; a machine with more CPUs should run no slower than a
machine with fewer CPUs; and calling a module more often should not reduce the
amount of work that gets done, even if it doesn't increase it.

Ideally, a critical section should provide approximately the same rate of
progress to many contending threads as it can to a single thread. Mutex
implementations can approximate this ideal by not providing fairness, and by
preventing multiple threads that have already blocked from simultaneously
competing for the lock.

### Does every Lock operation imply a memory barrier?

Programmers should not use `Lock` operations as a means for inserting arbitrary
memory barriers into their code. (Or for exerting control over when threads
run.) `Lock` operations imply only ordering necessary for the protection of
monitor invariants. In particular, the intent is:

*If* threads T0 and T1 execute the following code, where some location is
modified by one of `T0_Inside()` and `T1_Inside()` and read or written by the
other:

    // thread T0                             // thread T1
    T0_Before();
                                              T1_Before();
    mu.Acquire();
                                              mu.Acquire();
    T0_Inside();
                                              T1_Inside();
    mu.Release();
                                              mu.Release();
    T0_After();
                                              T1_After();


*then* the memory operations in `Tx_Before()` and `Tx_Inside()` all precede the
memory operations in `Ty_Inside()` and `Ty_After()` either for `x=0, y=1` or for
`x=1, y=0`.

If the predicate does not hold, no memory ordering should be assumed from the
`Lock` operations. This surprises programmers who expect the simplest possible
implementation, with no optimizations. Unfortunately, this expectation is
reinforced by some API standards.

We discourage such assumptions because they make various transformations more
difficult. Examples include:

*   Removal of critical sections that are redundant with others.
*   Removal of locks used by only one thread, or that protect no data.
*   Coalescing and splitting of locks and critical sections.
*   Conversion of exclusive locks to shared locks.
*   Replacing locks with transactional memory.

Some lock implementations already apply some of these transformations, and are
more efficient as a result. Therefore, `Lock` reserves the right to use such
transformations when safe, even if that means removing memory barriers.
