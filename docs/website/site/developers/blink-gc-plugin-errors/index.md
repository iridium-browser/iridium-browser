---
breadcrumbs:
- - /developers
  - For Developers
page_name: blink-gc-plugin-errors
title: Blink GC Plugin Errors
---

## Introduction

With the addition of the new Blink garbage collection infrastructure we have
created a clang compiler plugin to statically check various conditions are met.
This page documents what conditions are checked, which error a failed condition
will raise and how the error might be fixed.

All diagnostics produced by the checker are prefixed with \[blink-gc\].

## Tracing Errors

All members of a class must be traced in order to ensure that the GC system does
not reclaim their storage space. Thus, if a class contains any fields that need
to be traced (ie, the field is either a GC managed object or it also contains
fields that need to be traced), then the class must implement a trace method to
do so.

If you get the error:

*   Class 'Foo' requires a trace method because it contains fields that
            require tracing.

then your class/struct contains fields that need to be traced and you must
define a trace method doing so. The plugin will provide a note describing which
fields that need to be traced. A problematic class might look like:

class Foo : public GarbageCollected&lt;Foo&gt; {

public:

Baz\* getTheBaz() { return m_baz; }

private:

Member&lt;Baz&gt; m_baz;

};

This is fixed by defining a trace method and tracing the field m_baz:

class Foo : public GarbageCollected&lt;Foo&gt; {

public:

Baz\* getTheBaz() { return m_baz; }

virtual void trace(Visitor\* visitor) { visitor-&gt;trace(m_baz); }

private:

Member&lt;Baz&gt; m_baz;

};

If you get the error:

*   Class 'Foo' overrides non-virtual trace of base class 'Bar'.

then your class/struct has overridden a non-virtual trace method of a base class
and the base class should either be virtual or the hierarchy needs to implement
manual trace dispatching (see the section on Dispatch Errors below).

If you get the errors:

*   Base class 'Foo' of derived class 'Bar' requires tracing.
*   Class 'Bar' has untraced fields that require tracing.

then your class/struct has a trace method, but it does not trace all of the
required parts. The base class tracing ensures that fields declared in base
classes are traced. The plugin will provide a note describing which fields that
need to be traced. A problematic class might look like:

class Bar : public Foo {

public:

Baz\* getTheOtherBaz() { return m_otherBaz; }

void trace(Visitor\* visitor) { }

private:

Member&lt;Baz&gt; m_otherBaz;

};

This is fixed by tracing Foo and m_otherBaz:

class Bar : public Foo {

public:

Baz\* getTheOtherBaz() { return m_otherBaz; }

void trace(Visitor\* visitor)

{

visitor-&gt;trace(m_otherBaz);

Foo::trace(visitor);

}

private:

Member&lt;Baz&gt; m_otherBaz;

};

## Field Errors

In order to ensure proper tracing, each field of a class is checked against some
correctness requirements described below.

If you get the errors:

*   Class 'Foo' contains invalid fields.
*   Class 'Foo' contains GC root in field 'm_bar'.

then your class/struct contains problematic fields. If a field points to a GC
allocated object then it should be in a Member, so if the field is currently a
raw pointer, RefPtr or OwnPtr, it should be changed to a Member or WeakMember
and properly traced. A problematic class might look like:

class Foo : public GarbageCollected&lt;Foo&gt; {

public:

Baz\* getTheBaz() { return m_baz; }

private:

OwnPtr&lt;Baz&gt; m_baz;

};

Here Baz is a GC allocated type and the issue is fixed by replacing the OwnPtr
by Member and tracing the field:

class Foo : public GarbageCollected&lt;Foo&gt; {

public:

Baz\* getTheBaz() { return m_baz; }

virtual void trace(Visitor\* visitor) { visitor-&gt;trace(m_baz); }

private:

Member&lt;Baz&gt; m_baz;

};

Another type of invalid field is the embedding of or a pointer to a
stack-allocated object from within A GC allocated object. This is simply not
permitted and the containing class must also be stack allocated or the
stack-allocated object must become GC allocated.

If a field inside a GC allocated object either defines a GC root (ie, Persistent
or PersistentHeapXXX collection) or embeds a GC root via a part object, then we
likely have a memory leak. In this case the Persistent should be replaced by a
properly traced Member. A problematic case might look like:

class Part {

DISALLOW_ALLOCATION();

public:

Baz\* getTheBaz() { return m_baz; }

private:

Persistent&lt;Baz&gt; m_baz;

};

class Foo : public GarbageCollected&lt;Foo&gt; {

public:

Baz\* getTheBaz() { return m_part.getTheBaz(); }

private:

Part m_part;

};

Here Part is used as a part object of Foo and it contains a Persistent pointer
to Baz. To fix the issue the Persistent should be replaced by Member and Part
should be traced:

class Part {

DISALLOW_ALLOCATION();

public:

Baz\* getTheBaz() { return m_baz; }

void trace(Visitor\* visitor) { visitor-&gt;trace(m_baz); }

private:

Member&lt;Baz&gt; m_baz;

};

class Foo : public GarbageCollected&lt;Foo&gt; {

public:

Baz\* getTheBaz() { return m_part.getTheBaz(); }

virtual void trace(Visitor\* visitor) { visitor-&gt;trace(m_part); }

private:

Part m_part;

};

## Finalization Errors

When an object is allocated on the GC managed heap it might need finalization
support. Objects that derive a GC base with finalization support will be
finalized on the first GC where they have become unreachable. Thus the
finalization time (ie, time of destruction) is not known in advance. By default
finalization of an object will call the objects destructor. If a class does not
have finalization support its destructor will never be called. The existence of
destructors that are never called can hide subtle bugs and so we check agains
this with the GC plugin. The basic rule is that a class with a "non-trivial
destructor" must have finalization support. A class has a trivial destructor if
and only if the destructor is the default generated destructor and all of the
class bases and members have trivial destructors.

If you get the error:

*   Class 'Foo' requires finalization.

then your class/struct has a "non-trivial finalizer" and therefore needs to
inherit from a finalized GC base class, such as, GarbageCollectedFinalized. The
plugin will try to emit notes about which bases, members and destructors are
causing the class to have a non-trivial finalizer. For example:

class Foo : public GarbageCollected&lt;Foo&gt; {

public:

~Foo();

};

will issue a note about a "user-declared destructor". The destructor can then
either be removed or finalization support can be added to the class:

class Foo : public GarbageCollectedFinalized&lt;Foo&gt; {

public:

~Foo();

};

If you get the error:

*   Finalizer '~Foo' accesses potentially finalized field 'm_baz'.

then your finalizer is accessing a field of Foo that might be finalized in the
same GC round. In other words, the object pointed to by m_baz might already be
reclaimed by the GC. For example:

class Foo : public GarbageCollected&lt;Foo&gt; {

public:

~Foo { m_baz-&gt;doSomeCleanup(); }

void trace(Visitor\* visitor) { visitor-&gt;trace(m_baz); }

private:

Member&lt;Baz&gt; m_baz;

};

needs to be changed to perform the cleanup of baz in some other way dependent on
the concrete situation. Maybe the cleanup code can be moved into the
finalization of Baz.

## Dispatch Errors

Some places in the Blink code base avoid introducing a vtable for a class
hierarchy, such as the CSSValue class hierarchy. In such cases the programmer
needs to define the dispatch methods manually.

If you get the errors:

*   Class 'Foo' is missing manual trace dispatch.
*   Class 'Foo' is missing manual finalization dispatch.

your class needs to define a trace method and/or a
finalizeGarbageCollectedObject method to perform the manual dispatch.

If you get the errors:

*   Class 'Foo' contains or inherits virtual methods but implements
            manual dispatching.

your class has implemented manual dispatch but might be able to simply have a
virtual trace and destructor.

If you get the errors:

*   Missing dispatch to class 'Bar' in manual trace dispatch.
*   Missing dispatch to class 'Bar' in manual finalize dispatch.

then the derived class 'Bar' is not dispatched to from the manually implemented
dispatch.

## Derived Class Errors

If you get the error:

*   Class 'Foo' must derive its GC base in the left-most position.

then the class Foo inherits from a GC base, but does so incorrectly. To ensure
that the vtable entry and GC meta data of a class is mapped to a reliable
location in memory, we require that the GC base is always inherited in the
left-most position in the super-class declarations. For example:

class Foo : public Bar, public GarbageCollected&lt;Foo&gt; {

public:

void trace(Visitor\* visitor) { visitor-&gt;trace(m_baz); }

private:

Member&lt;Baz&gt; m_baz;

};

needs to swap the position of Bar and the GC base:

class Foo : public GarbageCollected&lt;Foo&gt;, public Bar {

public:

void trace(Visitor\* visitor) { visitor-&gt;trace(m_baz); }

private:

Member&lt;Baz&gt; m_baz;

};

The same is true if the GC base is inherited from some other derived class, say
Baz, then the super-class declaration of Baz must be in the left-most position.

If you get the error:

*   Left-most base class 'Bar' of derived class 'Foo' must be
            polymorphic.
*   Left-most base class 'Bar' of derived class 'Foo' must define a
            virtual trace method.

then the base class 'Bar' needs to be polymorphic and if 'Foo' has a virtual
trace method then it must be declared virtual in 'Bar'. The requirement is to
ensure that the GC have a consistent view of a vtable/virtual-trace even if a GC
happens while an object is only partially constructed. In both cases, the
typical fix will simply be to define a virtual trace in 'Bar', override it in
'Foo' and trace the super class there. For example:

class Bar : public GarbageCollected&lt;Bar&gt; {

public:

Bar(); // could potentially trigger a GC.

};

class Foo : public Bar {

public:

virtual void trace(Visitor\* visitor) { visitor-&gt;trace(m_baz); }

private:

Member&lt;Baz&gt; m_baz;

};

needs to define trace in 'Bar' and trace it in 'Foo':

class Bar : public GarbageCollected&lt;Bar&gt; {

public:

Bar();

virtual void trace(Visitor\*) { }

};

class Foo : public Bar {

public:

void trace(Visitor\* visitor)

{

visitor-&gt;trace(m_baz);

Bar::trace(visitor);

}

private:

Member&lt;Baz&gt; m_baz;

};

If you get the error:

*   Stack-allocated class 'Foo' derives class 'Bar' which is not stack
            allocated

then either class Foo can not be annotated as "stack allocated", or class Bar
must be annotated "stack allocated" and used as such. A stack allocated class
cannot be new'ed, ie, allocated on the managed heap or using any other kind of
allocator. For example:

class Bar {

public:

void someMethod();

};

class Foo : public Bar {

STACK_ALLOCATED();

private:

Member&lt;Baz&gt; m_baz;

};

needs to either make Bar stack allocated:

class Bar {

STACK_ALLOCATED();

public:

void someMethod();

};

class Foo : public Bar {

private:

Member&lt;Baz&gt; m_baz;

};

Notice that the stack allocated annotation is inherited and that a stack
allocated object does not need to define a trace method: its pointers are found
conservatively on the stack.

Alternatively, Foo can be made GC allocated:

class Bar {

public:

void someMethod();

};

class Foo : public GarbageCollected&lt;Foo&gt;, public Bar {

public:

void trace(Visitor\* visitor) { visitor-&gt;trace(m_baz); }

private:

Member&lt;Baz&gt; m_baz;

};
