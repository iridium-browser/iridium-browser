---
breadcrumbs:
- - /nativeclient
  - Native Client
page_name: styleguide
title: Native Client Coding Style Guidelines
---

The purpose of this document is to codify our existing coding style guidelines
for new Native Client project members, including external committers. Some of
the recommendations address security concerns, e.g., making the code easier to
audit. A core principle is that our coding style should encourage writing
correct code. Moreover, it should encourage writing obviously correct, easily
audited code.

Generally, we follow the published Google Coding Style guidelines. However,
those guidelines were written primarily for C++ and python code, and a portion
of NaCl code is in C (esp kernel-like code); furthermore, we use
scons/gyp/subversion rather than gconfig/perforce, so build system issues are
also addressed. Additionally, we incorporate/use bodies of third party code or
their interfaces, most obvious of which is NPAPI, and stylistic differences
occur, especially among major modules. In such cases, it is generally a good
idea to adhere to the existing style -- for example, the Google Coding Style
expressly does not endorse one position of the space in declarations such as
char \*p; versus char\* p; over another -- and in such cases maintaining
consistency with the prevalent style in the module would enhance readability.

The Google C++ coding style guide can be found here:
<http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml>; the python
version can be found here:
<http://google-styleguide.googlecode.com/svn/trunk/pyguide.html>.

**C/C++/Build Style Rules**

*   In a file that implements an interface declared in a header file,
            always include that header file. Rationale: this prevents interface
            / implementation mismatches, since C does not do C++ style name
            mangling to encode the parameter types in the linker-level name.
            Even in C++, since the return type is not part of the type
            signature, return type mismatches would not get detected as a
            side-effect of name mangling.
*   Header files have guards against multiple inclusion. Rationale:
            unlike traditional Unix system header file style, the user of a
            header file is not responsible for knowing what type declarations
            etc that the header file may transitively require and to put include
            directives for transitively dependent header files in the C/C++ file
            ahead of the desired header file. Instead, the Google style is that
            if a header file A depends on something in header file B, then A
            should include B. If a source file also needs something from B, it
            will explicitly include B again. Do *not* depend on knowledge about
            A's implementation -- i.e., that it would include B -- in the source
            file, since that's an implementation detail that may change. Always
            directly include all header files that are needed by a file, but do
            not "look inside" files that you include: do not depend on
            transitive inclusions.
*   At the same time try to reduce the number of other header files your
            header depends on. If you only use a class/struct via
            pointers/references a simple forward declaration will do. Similarly
            you may want think twice before making a class contain another class
            as a member. Often using a pointer/reference will provide better
            information hiding at the price of an extra indirection (along with
            memory allocation and associated error handling).
*   Don't turn off compiler warning flags. If you have to, e.g., because
            we absolutely have to include a third party file, clone the scons
            build environment and remove the compiler warning flag only for that
            cloned environment, and use that cloned environment only for those
            files that includes the third party header file. Be judicious and
            limit the amount of taint as much as possible.
*   When importing third party code, rather than turning off warnings
            broadly (e.g., removing -pedantic or -Wall), try to turn off only
            the warning that the third party code triggers, e.g.,
            -Wno-long-long. Limit the scope.
*   Do not declare function prototypes in .c or .cc files -- include the
            corresponding header file.

**C Style Rules**

*   We implement objects in C via explicit ctor and virtual function
            tables. Unlike C++, the ctor function returns an int -- conceptually
            a bool -- indicating success with a non-zero value. In C++ this
            would be split into a simple ctor that fills in fields with default
            values so dtor cleanup can happen, and an explicit Init function
            that handles the more complex initialization that may return an
            error.
*   In ctors, we always start by setting the vtbl pointer to NULL. This
            is a defense-in-depth mechanism. Only when the ctor is about to
            return success is the vtbl pointer set to point at a valid virtual
            function table. This means that should the caller forget to check
            the return value of the ctor, the next use of a virtual function
            will cause the program to abort due to the NULL pointer
            de-reference. In C++ code this would have to be emulated with a
            boolean flag that is set only when the Init member function returns
            success, and where every virtual function explicitly checks the flag
            upon entry. We generally use placement-new style ctors, where the
            caller allocates the memory (either stack or heap) and pass the
            memory to the ctor. Note that in ctors of derived classes, we may
            have to explicitly reset vtbl pointers if the ctor of the base class
            succeeds but later initialization of (new) members fail -- in order
            for the derived class ctor to fail cleanly, it should destroy any
            already initialized new members, then call the base class dtor --
            which should reset the vtbl pointer anyway -- as the last operation
            before returning failure (0).
*   In base class dtors, we always end with setting the vtbl pointer to
            NULL. It doesn't hurt to also explicitly set them to NULL as the
            last operation in derived class dtors either; depending on the
            frequency of object construction/destruction, it may even make sense
            to explicitly clear the entire object's memory. Rationale: this
            prevents uses -- via virtual functions -- of objects that have been
            deleted (object reuse).
*   Use standard safety idioms. The principle here is that it should be
            obvious that the code is correct. A code auditor/reviewer should not
            have to hunt far away for type declarations or for constants to
            determine the correctness of the code. Sometimes it's better to have
            redundant checks, i.e., integer overflow checks just above the
            allocation, even though the value was checked earlier against
            another value so that the earlier check's post-condition would
            entail that the overflow check is unnecessary.
    *   Integer overflow checks: use SIZE_T_MAX, which is (~((size_t)
                0)), to verify that allocating several objects will not cause an
                overflow:

> > > /\* num_elt is untrusted \*/

> > > if (num_elt &gt; SIZE_T_MAX / sizeof \*ptr) {

> > > /\* error handling / abort \*/

> > > }

> > > ptr = malloc(num_elt \* sizeof \*ptr);

> > > if (NULL == ptr) {

> > > /\* error handling \*/

> > > }

> > similarly, even assignments can cause problems:

> > > int32_t len = strlen(some_c_str); /\* on 64-bit systems, a problem also
> > > with int64_t \*/

> > here, since strlen returns a value of type size_t, it may be a value that
> > cannot be represented as an int32_t -- according to the C (and C++)
> > standard, when such an assignment occurs and the expression value is not
> > representable in the destination type, the result is implementation
> > dependent. And when this occurs, compilers are free to generate code that
> > delete all of the users' files (though we know of no such compiler!).
> > Instead, check:

> > > size_t len_sz = strlen(some_c_str);

> > > if (len_sz &gt; INT32_MAX) { /\* from stdint.h \*/

> > > /\* error handling \*/

> > > }

> > > len = (int32_t) len_sz; /\* len_sz unused henceforth, so this is just
> > > renaming a register / memory location \*/

> > to ensure that overflows won't cause problems. Similarly, even additions can
> > overflow:

> > > int32_t foo;
> > > int32_t bar;
> > > ...
> > > foo = bar + 1;

> > should be

> > > int32_t foo;

> > > int32_t bar;

> > > ...

> > > if (bar &gt; INT32_MAX - 1) {

> > > /\* error handling \*/

> > > }

> > > foo = bar + 1;

    *   See the malloc idiom above. The amount of memory to be allocated
                should always use a dereference of the left-hand-side of the
                assignment, since the lhs must be of the right type. Rationale:
                if for some reason we later change the type of a variable, there
                is no need to track down occurrences of sizeof(Typename)
                elsewhere, since the use of the lhs in the sizeof will make it
                automatically correct.

> > Comparisons involving a constant should keep the constant on the lhs of the
> > comparison operator. This avoids accidents like

> > > if (ptr = NULL) {

> > > /\* oops \*/

> > > }

> > where a comparison gets turned into an assignment (followed by implicit
> > comparison to 0) due to a typographical error, changing the meaning of the
> > code. Instead, use the idom from the larger code snippet above. Since the
> > lhs is not an lvalue, this cannot be accidentally turned into an assignment.
> > While not a strict requirement, constants should be used on the lhs even for
> > inequality tests, since in the future somebody else might change things
> > around and turn it (or via cut-n-paste) into an intended equality test.

> > Once we have a test to ensure that we do not accidentally turn off
> > compiler-specific flags that warn when constructs like if (lvalue =
> > expression) is used in code, we can relax this rule.

    *   Rather than refering to a distantly defined value/variable such
                as a constant defined in a header or the top of the file

> > > #define BUFFER_SIZE 1000

> > > /\* ... \*/

> > > char buffer\[BUFFER_SIZE\];

> > > /\* ... \*/

> > > memset(buffer, 0, BUFFER_SIZE); /\* bad \*/

> > instead, do

> > > memset(buffer, 0, sizeof buffer); /\* better (Yes, the parentheses are not
> > > required around buffer here. It surprised cbiffle and sehr too.) \*/

> > The latter is more obviously correct. (But not if buffer is actually a char
> > \* to malloc'd memory, so auditors still have to do some looking around.)

    *   add more lines...
*   add more lines...

### **C++ Style Rules**

*   We generally match the Google style rules. Differences are generally
            due to our code not being standard Google C++ code and not running
            in the production environment found in Google datacenters.
*   Exceptions:
    *   Note that Chromium's default gyp file setup compiles with
                -fno-exceptions so catch statements have no effect -- the catch
                block is just dead code, and the C++ runtime provided top-level
                exception handler will be the only exception handler that's
                active.
    *   If for some reason we must use exceptions or deal with
                exceptions (e.g., because we are building a library/module that
                will be generally useful and is not NaCl specific or must use an
                underlying library that throws exceptions as part of its API),
                do not throw exceptions across API boundaries (except for fatal
                errors). The library/module *must* be compiled with exceptions
                enabled, but the relying code might not, and we should not, in
                general, force relying code to enable exceptions. This usually
                means that the library must catch all (non-fatal) exceptions at
                API boundaries and convert them into error codes. Use idioms
                that encourage exception safety, particularly when interacting
                with STL. See Effective C++ item #29.
    *   Remember that new can throw. You must either (1) assume that an
                exception can cause a jump at any invocation of new -- and cause
                the application to abort -- or (2) use new(std::nothrow) and
                check its result! Unlike production code where failing fast is
                generally a good idea to either failover to another machine or
                to restart the local server process, this is sometimes not
                appropriate in client-side code.
        *   Note that neither of these is inherently safer than the
                    other. While nothrow may seem safer because of the lack of
                    surprise control flow, at least an exception causes
                    destructors in the current scope to be run, while failing to
                    check for a null pointer will simply segfault down the line
                    (at best).
        *   When allocating a series of objects in the heap, use
                    scoped_ptr or auto_ptr to cleanly deallocate them if one new
                    or constructor throws. This is also good RAII practice.
*   If a data structure's size may be determined by a malicious author —
            say, a set describing all the validation errors encountered in a
            NaCl module — expect exceptions like bad_alloc that indicate you are
            being DoS'd. To avoid having to reason about exception safety and
            failure modes in these cases, consider capping the size of such
            structures.
*   Beware of integral type conversions, especially when dealing with
            inputs from external sources that might be under an attacker's
            control. Use assert_cast&lt;T&gt;(expr) and
            saturate_cast&lt;T&gt;(expr) from
            native_client/src/include/check_cast.h when appropriate. For
            example:

> > bool SanityCheckerApi(SomeType obj, char\* data, int32_t nbytes);

> > bool SomeClass::SomeMethod(std::string s) {

> > if (SanityCheckerApi(obj_, s.c_str(), static_cast&lt;int32_t&gt;(s.size())))
> > {

> > // Compilers correctly warn that precision is lost without a cast,

> > // since std::string::size() is size_t, and on a 64-bit machine is

> > // large than int32_t. However, just using a static_cast&lt;int32_t&gt; to

> > // get rid of the compiler warning messages is wrong.

> > return false; // error

> > }

> > /\* Use all of s \*/

> > return true;

> > }

> > can cause a security problem: if the caller-supplied data s is under the
> > adversary's control and could be more than 4GB in size, then
> > SanityCheckerApi will only check the first s.size() mod 2\*\*32 bytes of
> > data, and the use of s after the check will (presumably) consume all of s
> > and do the wrong thing.

*   add more here...
