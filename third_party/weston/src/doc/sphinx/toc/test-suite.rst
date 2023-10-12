Weston test suite
=================

Weston test suite aims to test features of the Weston compositor and libweston.
The automatic tests are executed as part of ``meson test`` and in the Gitlab CI.
In addition to automatic tests, there are few manual tests that have not been
automated, but being manual means they are also not routinely (or ever)
executed.


Test execution
--------------

The test execution hierarchy is:

* ``meson test``

  * a test program

    * a fixture setup

      * a test

        * a sub-test from a data array

When ``meson test`` is executed, it will run all defined *test programs*
potentially in parallel and collect their exit status. Therefore it is
important to design each test program to be executable in parallel with every
other test program.

A **test program** is essentially one ``.c`` source code file that is built into
one executable file (not a library, module, or plugin). Each test program is
possible to run manually without Meson straight from the build directory
without any environment or command line setup, e.g. with GDB or Valgrind.

A test program may define one **fixture setup** function. The function may be
defined alone or with a data array of an arbitrary data type. If an array is
defined, the fixture setup will be called and all the tests in the program
executed for each element in the array serially. Fixture setups are used for
setting up the Weston compositor for the tests that need it. The array is
useful for running the compositor with different settings for the same tests,
e.g. with Pixman-renderer and GL-renderer.

**A test** in a test program is defined with one of the macros :c:func:`TEST`,
:c:func:`TEST_P`, or :c:func:`PLUGIN_TEST`. :c:func:`TEST` defines a single
test with no sub-tests. :c:func:`TEST_P` defines a data-driven array of tests:
a set of sub-tests. :c:func:`PLUGIN_TEST` is used specifically by *plugin
tests* that require access to :type:`weston_compositor`.

All tests and sub-tests are executed serially in a test program. The test
harness does not ``fork()`` which means that any test that crashes or hits an
assert failure will quit the whole test program on the spot, leaving following
tests in that program not executed.

The test suite has no tests that are expected to fail in general. All tests
that test for a failure must check the exact error condition expected and
succeed if it is met or fail for any other or no error.


Types of tests
--------------

Aside from manual vs. automatic, there are three types of tests:

Standalone tests
   Standalone tests do not launch the full compositor.

Plugin tests
   Plugin tests launch the Weston compositor and execute the list of tests
   from an idle callback handler in the compositor context, blocking the
   compositor while they run.

Client tests
   Client tests launch the Weston compositor and execute the list of tests
   in a new thread that is created from an idle callback handler. This means
   the compositor runs independently from the tests and one can write a test
   like as a normal Wayland client.

The type of all the tests in a test program is defined by the fixture setup
function. A fixture setup function is any defined function with a specific
signature and registered with either :c:func:`DECLARE_FIXTURE_SETUP` or
:c:func:`DECLARE_FIXTURE_SETUP_WITH_ARG`.


.. _test-suite-standalone:

Standalone tests
^^^^^^^^^^^^^^^^

Standalone tests do not have a fixture setup function defined in the test
program or the fixture setup function calls
:func:`weston_test_harness_execute_standalone` explicitly. All test cases must
be defined with :c:func:`TEST` or :c:func:`TEST_P`.

This is the simplest possible test example:

.. code-block:: c

   TEST(always_success)
   {
   	/* true */
   }


.. _test-suite-plugin:

Plugin tests
^^^^^^^^^^^^

Plugin tests must have a fixture setup function that calls
:func:`weston_test_harness_execute_as_plugin`. All test cases must be defined
with :c:func:`PLUGIN_TEST` which declares an implicit function argument
:type:`weston_compositor` ``*compositor``.

The compositor fixture manufactures the necessary environment variables and the
command line argument array to launch Weston, and calls :func:`wet_main`
directly. An idle task handler is registered, which gets invoked when
initialization is done. All tests are executed from that idle handler, and then
the compositor exits.

This is an example of a plugin test that just logs a line:

.. code-block:: c

   static enum test_result_code
   fixture_setup(struct weston_test_harness *harness)
   {
   	struct compositor_setup setup;

   	compositor_setup_defaults(&setup);

   	return weston_test_harness_execute_as_plugin(harness, &setup);
   }
   DECLARE_FIXTURE_SETUP(fixture_setup);

   PLUGIN_TEST(plugin_registry_test)
   {
   	/* struct weston_compositor *compositor; */
   	testlog("Got compositor %p\n", compositor);
   }


.. _test-suite-client:

Client tests
^^^^^^^^^^^^

Plugin tests must have a fixture setup function that calls
:func:`weston_test_harness_execute_as_client`. All test cases must be
defined with :c:func:`TEST` or :c:func:`TEST_P`.

The compositor fixture manufactures the necessary environment variables and the
command line argument array to launch Weston, and calls :func:`wet_main`
directly. An idle task handler is registered, which gets invoked when
initialization is done. The idle handler creates a new thread and returns. The
new thread will execute all tests and then signal the compositor to exit.

This is an incomplete example of an array of sub-tests and another test as
clients:

.. code-block:: c

   static enum test_result_code
   fixture_setup(struct weston_test_harness *harness)
   {
   	struct compositor_setup setup;

   	compositor_setup_defaults(&setup);

   	return weston_test_harness_execute_as_client(harness, &setup);
   }
   DECLARE_FIXTURE_SETUP(fixture_setup);

   struct bad_source_rect_args {
   	int x, y, w, h;
   };

   static const struct bad_source_rect_args bad_source_rect_args[] = {
   	{ -5,  0,  20,  10 },
   	{  0, -5,  20,  10 },
   	{  5,  6,   0,  10 },
   	{  5,  6,  20,   0 },
   	{  5,  6, -20,  10 },
   	{  5,  6,  20, -10 },
   	{ -1, -1,  20,  10 },
   	{  5,  6,  -1,  -1 },
   };

   TEST_P(test_viewporter_bad_source_rect, bad_source_rect_args)
   {
   	const struct bad_source_rect_args *args = data;
   	struct client *client;
   	struct wp_viewport *vp;

   	client = create_client_and_test_surface(100, 50, 123, 77);

   	vp = create_viewport(client);

   	testlog("wp_viewport.set_source x=%d, y=%d, w=%d, h=%d\n",
   		args->x, args->y, args->w, args->h);
   	set_source(vp, args->x, args->y, args->w, args->h);

   	expect_protocol_error(client, &wp_viewport_interface,
   			      WP_VIEWPORT_ERROR_BAD_VALUE);
   }

   TEST(test_roundtrip)
   {
   	struct client *client;

   	client = create_client_and_test_surface(100, 50, 123, 77);
   	client_roundtrip(client);
   }


DRM-backend tests
-----------------

DRM-backend tests require a DRM device, so they are a special case. To select a
device the test suite will simply look at the environment variable
``WESTON_TEST_SUITE_DRM_DEVICE``. In Weston's CI, we set this variable to the
DRM node that VKMS takes (``cardX`` - X can change across each bot, as the order
in which devices are loaded is not predictable).

**IMPORTANT**: our DRM-backend tests are written specifically to run on top of
VKMS (KMS driver created to be used by headless machines in test suites, so it
aims to be more configurable and predictable than real hardware). We don't
guarantee that these tests will work on real hardware.

But if users want to run DRM-backend tests using real hardware anyway, the first
thing they need to do is to set this environment variable with the DRM node of
the card that should run the tests. For instance, in order to run DRM-backend
tests with ``card0`` we need to run ``export WESTON_TEST_SUITE_DRM_DEVICE=card0``.

Note that the card should not be in use by a desktop environment (or any other
program that requires master status), as there can only be one user at a time
with master status in a DRM device. Also, this is the reason why we can not run
two or more DRM-backend tests simultaneously. Since the test suite tries to run
the tests in parallel, we have a lock mechanism to enforce that DRM-backend
tests run sequentially, one at a time. Note that this will not avoid them to run
in parallel with other types of tests.

Another specificity of DRM-backend tests is that they run using the non-default
seat ``seat-weston-test``. This avoids unnecessarily opening input devices that
may be present in the default seat ``seat0``. On the other hand, this will make
``launcher-logind`` fail, as we are trying to use a seat that is different from
the one we are logged in. In the CI we do not rely on ``logind``, so it should
fallback to ``launcher-direct`` anyway. It requires root, but this is also not a
problem for the CI, as ``virtme`` starts as root. The problem is that to run
the tests locally with a real hardware the users need to run as root.


Writing tests
-------------

Test programs do not have a ``main()`` of their own. They all share the
``main()`` from the test harness and only define test cases and a fixture
setup.

It is recommended to have one test program (one ``.c`` file) contain only one
type of tests to keep the fixture setup simple. See
:ref:`test-suite-standalone`, :ref:`test-suite-plugin` and
:ref:`test-suite-client` how to set up each type in a test program.

.. note::

   **TODO:** Currently it is not possible to gracefully skip or fail a test.
   You can skip with ``exit(RESULT_SKIP)`` but that will quit the whole test
   program and all defined tests that were not ran yet will be counted as
   failed. You can fail a test by any means, e.g. ``exit(RESULT_FAIL)``, but
   the same caveat applies. Succeeded tests must simply return and not call any
   exit function.


.. toctree::
   :hidden:

   test-suite-api.rst
