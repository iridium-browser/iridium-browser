---
breadcrumbs: []
page_name: rvalue-references
title: Rvalue references in Chromium
---

Now that C++11 support is available in our standard library (libc++), we should
be able to make use of standard container types in more places, and to use
containers and standard types more efficiently. In order to do that we need to
understand and make use of std::move() and rvalue references.

We hope that the information here will be enough for you to start using these
C++11 features safely in Chromium. (Some of the information here may
purposefully gloss over details or edge cases.)

[TOC]

## Rvalue references and std::move()

The [google style
guide](https://google.github.io/styleguide/cppguide.html#Rvalue_references)
allows the following: Use rvalue references only to define move constructors and
move assignment operators, or for perfect forwarding. Here we talk about the
first type of use, and follow up about the second later.

Many of the ideas here come from a talk given by Scott Meyers that is [available
on
MSDN](https://channel9.msdn.com/Events/GoingNative/2013/An-Effective-Cpp11-14-Sampler).
The video is really excellent and worth watching.

There are links to full example code with each of the examples. You can try them
out locally, or drop them into an [online
compiler](http://coliru.stacked-crooked.com/).

### 1. What makes rvalues.

You know that you have an rvalue when the object has no name. Typically rvalues
are temporary objects that exist on the stack as the result of a function call
or other operation.

- Returning a value from a function will turn that value into an rvalue. Once
you call return on an object, the name of the object does not exist anymore (it
goes out of scope), so it becomes an rvalue.

- Similarly, calling a function will give back an rvalue. The return value of a
function has no name.

```cpp
MyType MakeMyType() {
  MyType type;
  return type;
}

int main(int argc, char **argv) {
  // MakeMyType returns an rvalue MyType.
  MyType type = MakeMyType();
  ProcessMyType(MakeMyType());
}
```

<http://pastebin.com/raw.php?i=ki3q2Gbi>

### 2. Move constructor is used with rvalues.

Move constructors are similar to copy constructors. They construct an object out
of another object of the same type. The signature of the method changes in order
for it to accept rvalues only.

Here’s an example of a class with a move constructor. It will cause the compiler
to not generate an implicit copy constructor, making this class move-only.

```cpp
class MyType {
 public:
  MyType() {}
  MyType(MyType&& other) { fprintf(stderr, "move ctor\n"); }
  // Copy constructor is implicitly deleted.
  // MyType(const MyType& other) = delete;
};

MyType MakeMyType(int a) {
  // This is here to circumvent some compiler optimizations,
  // to ensure that we will actually call a move constructor.
  if (a % 2) {
    MyType type;
    return type;
  }
  MyType type;
  return type;
}

int main(int argc, char **argv) {
  // MakeMyType returns an rvalue MyType.
  // Both lines below call our move constructor.
  MyType type = MakeMyType(2);
  ProcessMyType(MakeMyType(2));
}
```

<http://pastebin.com/raw.php?i=tjxdwQuL>

The argument of the move constructor is a MyType&& which is a reference (like a
MyType&) except that it will only bind to an rvalue. That means the following
won’t compile, since it tries to construct a new MyType without an rvalue.

```cpp
class MyType {
 public:
  MyType() {}
  MyType(MyType&& other) { fprintf(stderr, "move ctor\n"); }
  // Copy constructor is implicitly deleted.
  // MyType(const MyType& other) = delete;
};

int main(int argc, char** argv) {
  MyType type;
  // |type| is an lvalue, so this will not compile because move constructor's
  // argument will only bind to rvalues.
  MyType other_type = type;
}
```

<http://pastebin.com/raw.php?i=9bjPNiWY>

### 3. Move constructor does a shallow copy.

A move constructor (like a copy constructor) works by copying the members of
another object into itself. The differences of a move constructor are the
following:

- The move constructor can be destructive. Since the argument is a non-const
reference, it may modify the object passed to it. This is okay since the
argument is an rvalue, so the caller does not hold onto a name for the original
object, and won’t make use of it (note that its destructor will happen though).

- The move constructor only needs to shallow copy. For example if the object
holds a pointer, it doesn’t need to deref the pointer and copy its contents. The
move constructor can just copy the pointer’s address. This is where the
performance benefits of move constructors happen.

```cpp
class MyType {
 public:
  MyType() {
    pointer_ = new int;
    *pointer_ = 1;
    memset(array_, 0, sizeof(array_));
    vector_.push_back(3.14);
  }

  MyType(MyType&& other) {
    fprintf(stderr, "move ctor\n");

    // Steal the memory, null out |other|.
    pointer_ = other.pointer_;
    other.pointer_ = nullptr;

    // Copy the contents of the array.
    memcpy(array_, other.array_, sizeof(array_));

    // Swap with our (empty) vector.
    vector_.swap(other.vector_);
  }

  ~MyType() {
    delete pointer_;
  }

 private:
  int* pointer_;
  char array_[42];
  std::vector<float> vector_;
};

int main(int argc, char **argv) {
  // MakeMyType returns an rvalue MyType.
  // Both lines below call our move constructor.
  MyType type = MakeMyType(2);
  ProcessMyType(MakeMyType(2));
}
```

<http://pastebin.com/raw.php?i=TXZB61Sg>

### 4. std::move() will cast a variable to an rvalue.

If you want to construct a new object from a variable, you can still use the
move constructor to do so.
[std::move()](http://en.cppreference.com/w/cpp/utility/move) will cast your
variable to an rvalue, allowing it to bind to a move constructor.

```cpp
int main(int argc, char** argv) {
  MyType type;
  // MyType other_type = type; won't work, since there is no copy ctor.
  // std::move casts |type| to an rvalue, so it uses the move constructor.
  MyType other_type = std::move(type);
}
```

<http://pastebin.com/raw.php?i=DzNMxt5j>

This also means that you can pass an rvalue to a function that uses
pass-by-value.

```cpp
int main(int argc, char** argv) {
  MyType type;

  // The return value of MakeMyType is an rvalue, so the move constructor
  // is used.
  ProcessMyType(MakeMyType(2));

  // Error: |type| is an lvalue, and is not copyable.
  ProcessMyType(type);

  // But std::move() turns lvalues into rvalues, so the move constructor
  // is used again.
  ProcessMyType(std::move(type));
}
```

<http://pastebin.com/raw.php?i=P7prZEur>

However..

### 5. You must not use a variable after calling std::move() on it.

Since you have casted your variable to an rvalue, functions that receive rvalues
may act destructively on your variable, so using the variable’s contents
afterward may result in undefined behaviour. The only valid things you may do
after calling std::move() on a variable are:

- Destroy it

- Assign to it (ie. replace its contents)

- Call a `reset()` method or similar if it is annotated with
`REINITIALIZES_AFTER_MOVE` to put the object back into a well-defined state

This applies to member variables too, don't leave your class in an undefined
state when leaving a method.

### 6. std::move() does not move, it casts.

You may think of std::move&lt;T&gt;() as instead rvalue_cast&lt;T&gt;(). It does
not perform any action of its own on the value passed to it, only changing what
methods you can pass it to.

So the following does nothing:

```cpp
MyType t;
std::move(t); // Casts it but it’s unused.
```

Just the same as:

```cpp
char t;
reinterpret_cast&lt;unsigned char&gt;(t); // Cast it but it’s unused.
```

In fact, the actual implementation of std::move() is done with static_cast. It
is simply a cast from MyType to MyType&& (a reference to an rvalue of MyType):

```cpp
return static_cast<typename std::remove_reference<T>::type&&>(t);
```

### 7. Copy constructors can also bind to rvalues (with lower priority).

Never read std::move() on an unfamiliar type and assume anything about what is
going to happen!

If you see code that calls std::move() it looks like it should be calling a move
constructor, but you could be wrong.

When a move constructor on the type being cast to an rvalue does not exist, the
compiler will fall back to binding the rvalue as a const-reference. This means
the copy-constructor would be used instead.

```cpp
class MyType {
 public:
  MyType() {
    pointer_ = new int;
    *pointer_ = 1;
    memset(array_, 0, sizeof(array_));
    vector_.push_back(3.14);
  }

  MyType(const MyType& other) {
    fprintf(stderr, "copy ctor\n");

    // Copy memory.
    pointer_ = new int;
    *pointer_ = *other.pointer_;

    // Copy the contents of the array.
    memcpy(array_, other.array_, sizeof(array_));

    // More copies.
    vector_ = other.vector_;
  }

  ~MyType() {
    delete pointer_;
  }

 private:
  int* pointer_;
  char array_[42];
  std::vector<float> vector_;
};

int main(int argc, char** argv) {
  {
    MyType type;
    // MyType other_type = type; works the same way here.
    // std::move casts |type| to an rvalue, but since there is no move
    // constructor, the copy constructor is used.
    MyType other_type = std::move(type);
  }

  {
    MyType type;
    // MakeMyType returns an rvalue, but since there is no move constructor, the
    // copy constructor is used.
    ProcessMyType(MakeMyType(2));
    // ProcessMyType(type); works the same way here.
    // std::move casts |type| to an rvalue, but since there is no move
    // constructor, the copy constructor is used.
    ProcessMyType(std::move(type));
  }
}
```
<http://pastebin.com/raw.php?i=p5ciyVVk>

So do not assume anything from reading std::move(). You need to understand the
class being moved to know what will happen.

### 8. Move constructors will only bind to non-const rvalues.

Beware of const! Const variables can not be moved from, since move constructors
take a non-const (rvalue) reference. However the compiler will silently
fall-back to the copy constructor.

```cpp
int main(int argc, char** argv) {
  const MyType type = MakeMyType(2);
  // std::move casts to an rvalue, but since this is const, it becomes an rvalue
  // to const, which uses the copy constructor, not the move constructor.
  ProcessMyType(std::move(type));
}
```

<http://pastebin.com/raw.php?i=5aVRz5SQ>

This type of error can only be caught by code inspection (and performance
testing). Always look at the type of the variable on which you call move().

This won’t bite you with function return rvalues, but requires caution when
using std::move().

### 9. Move constructors should be accompanied by move-assignment operators.

Just as a copy constructor MyType(const MyType& t) should be accompanied by a
copy-assignment operator operator=(const MyType& t), so should a move
constructor.

Move-assignment operators work and look the same as a Move constructor. Most
things we’ve discussed about move constructors (behaviour and performance) will
apply in all the same ways to move-assignment.

In following example, the class defines its move constructor in terms of
operator=, using a move assignment to construct itself.

```cpp
class MyType {
 public:
  MyType() {
    pointer_ = new int;
    *pointer_ = 1;
    memset(array_, 0, sizeof(array_));
    vector_.push_back(3.14);
  }

  MyType(MyType&& other) : pointer_(nullptr) {
    // Note that although the type of |other| is an rvalue reference, |other|
    // itself is an lvalue, since it is a named object. In order to ensure that
    // the move assignment is used, we have to explicitly specify
    // std::move(other).
    *this = std::move(other);
  }

  ~MyType() {
    delete pointer_;
  }

  MyType& operator=(MyType&& other) {
    // Steal the memory, null out |other|.
    delete pointer_;
    pointer_ = other.pointer_;
    other.pointer_ = nullptr;

    // Copy the contents of the array.
    memcpy(array_, other.array_, sizeof(array_));

    // Swap with our vector. Leaves the old contents in |other|.
    // We could destroy the old content instead, but this is equally
    // valid.
    vector_.swap(other.vector_);

    return *this;
  }

 private:
  int* pointer_;
  char array_[42];
  std::vector<float> vector_;
};
```

<http://pastebin.com/raw.php?i=WMZg2y3C>

### 10. Don’t go cray cray with move semantics.

Move-constructing is always as cheap or cheaper than copy-constructing. So you
could use it in every situation possible. But it may be adding very little value
(or making code misleading).

Almost always, just use a copy constructor. For example gfx::Point is 2
integers, a move constructor would do nothing different than a copy constructor.
There’s no reason to write a move constructor for this class, or to use
std::move() when using Points.

```cpp
class Point {

  int x_;
  int y_;

  Point(const Point& p) { x_ = p.x_; y_ = p.y_; }

  Point(Point&& p) { x_ = p.x_; y_ = p.y_; }
};
```

Move constructors are especially valuable for types that hold a pointer to
something large, since moving a pointer is much cheaper than copying everything
it points to. Other scenarios benefit less or not at all.

### 11. Don’t return references. Don’t use std::move() on rvalues.

Functions return an rvalue without you doing anything, they already do this (see
point #1). So you don’t need to do anything to make this happen. You should not
see functions that return a MyType&&. Simply return MyType.

```cpp
MyType&& MakeMyType() { // This is WRONG. The return type should not be

  // a reference.
  MyType type;
  return std::move(type); // This is usually WRONG. Returning will
  // already make it an rvalue.
}
```

Just as returning a MyType& would be wrong (returning a reference to a temporary
object), returning a MyType&& is wrong for the same reasons.

Putting std::move() on the return call actually makes things less efficient, it
interferes with the compiler doing RVO (Return Value Optimization). You can read
a previous chromium-dev thread about this in the context of scoped_ptr:
[link](https://groups.google.com/a/chromium.org/d/msg/chromium-dev/ICA3hiR4Z3A/Bi_ec46cAQAJ).

Similarly do not use move() on the return value from calling a function. You
already have an rvalue, so casting it there adds no value, only noise.

```cpp
// This is WRONG. It is casting an rvalue to an rvalue, which is not
// needed.
ProcessMyType(std::move(MakeMyType()));
```

### 12. std::move() is the same thing as our familiar Pass() function.

For some time, scoped_ptr::Pass() has been implemented as being the same thing
as std::move().

Places that previously called a.Pass() can be changed to call std::move(a)
instead, and they will be changed in the near future. It may help you to
consider this, if you are already familiar with Pass().

Using std::unique_ptr instead of scoped_ptr will require the use of std::move()
instead, as Pass() does not exist there (and is not needed). So code will be
change from:

```cpp
scoped_ptr<MyType> t = ...;
CallFunction(t.Pass());
```

To instead be:

```cpp
std::unique_ptr<MyType> t = ...;
CallFunction(std::move(t));
```

## Perfect forwarding

The [google style
guide](https://google.github.io/styleguide/cppguide.html#Rvalue_references)
allows the following: Use rvalue references only to define move constructors and
move assignment operators, or for perfect forwarding. We’ll talk about the
latter here.

So far, we’ve seen that we can write an rvalue reference in a move constructor,
or a move-assignment operator:

```cpp
MyType(MyType&& type); // Move constructor

operator=(MyType&& type); // Move-assignment operator
```

In both of these cases, the type MyType is fixed, so we know that we have a
reference to something that is definitely an rvalue. However when writing a
templated method, we may want to support an unknown type instead, at which point
we don’t know if it is an rvalue.

```cpp
template<typename T> 
void ProcessAndStore(T&& var) { ... }
```

Here the argument type may have been one of two things:

- Passed by move (rvalue reference) if you had an rvalue: MyType&&

- Passed by reference (aka lvalue reference) otherwise: MyType&

(You can read more about [reference
collapsing](http://stackoverflow.com/questions/13725747/concise-explanation-of-reference-collapsing-rules-requested-1-a-a-2)
if you want more details about how these are the only two possibilities.)

Arguments in templates can be written this way to implement perfect forwarding.
This allows for arguments to be passed through the template to another method
without introducing any additional copies of the object (ie no copy or move
constructors occur inside the template):

- If we had written the template as taking a parameter T, then a new variable
would have to be constructed inside the templated method. This could be done
with a copy constructor, or a move constructor (using std::move() inside the
template method), but it would require some extra work (which is why we normally
don’t pass-by-value of course).

- If we had written it instead as taking a const T&, then no copy takes place
entering the method, which is great. But when passing the variable to another
method, if it takes the argument by value, a copy occurs.

- Lastly, if we had written T&, then an rvalue would not bind to it, causing a
compiler error.

The most generally correct way to implement this is with
[std::forward()](http://en.cppreference.com/w/cpp/utility/forward), which will
always do the right thing for you.

```cpp
template<typename T>
void ProcessAndStore(T&& var) {
  // Process
  // ...

  // The type of |var| could be an rvalue reference, which means we should pass
  // an rvalue to Store. However, it could also be an lvalue reference, which
  // means we should pass an lvalue.
  // Note that just doing Store(var); will always pass an lvalue and doing
  // Store(std::move(var)) will always pass an rvalue. Forward does the right
  // thing by casting to rvalue only if var is an rvalue reference.
  Store(std::forward<T>(var));
}
```

<http://pastebin.com/raw.php?i=rVRebELt>

### std::forward() is a conditional cast to an rvalue.

This sounds lot like std::move(), which also casts to an rvalue. But
std::forward() will avoid casting to an rvalue if the argument is an lvalue
reference. It’s implementation is conceptually something like:

```cpp
if (is_lvalue_reference<T>::value) {
  return t;
}
return std::move(t);
```

That is, it preserves lvalue references. Why?

Because doing std::move() on an lvalue reference is bad! Code producing an
lvalue reference expects the object to remain valid. But code receiving an
rvalue reference expects to be able to steal from it. This leaves you with a
reference pointing to a potentially-invalid object.

```cpp
// Although Chromium disallows non-const lvalue references, this situation is
// still possible in templates.
void Oops(MyType& type) {
  // Move is destructive. It can change |type| which is an lvalue reference.
  MyType local_type = std::move(type);

  // Work with local_type.
  // ...
}

int main(int argc, char **argv) {
  MyType type;
  // Don't move type, just pass it.
  Oops(type);
  // Oops, this is a null pointer dereference.
  *type.pointer = 314;
}
```

<http://pastebin.com/raw.php?i=cPrYgGhr>

For perfect forwarding in templates (ie arguments of type T&&), use
std::forward() instead of std::move().
