//! Direct, unsafe bindings for Linux [`perf_event_open`][man] and friends.
//!
//! Linux's `perf_event_open` system call provides access to the processor's
//! performance measurement counters (things like instructions retired, cache
//! misses, and so on), kernel counters (context switches, page faults), and
//! many other sources of performance information.
//!
//! You can't get the `perf_event_open` function from the `libc` crate, as you
//! would any other system call. The Linux standard C library does not provide a
//! binding for this function or its associated types and constants.
//!
//! Rust analogs to the C types and constants from `<linux/perf_event.h>` and
//! `<linux/hw_breakpoint.h>`, generated with `bindgen`, are available in the
//! [`bindings`] module.
//!
//! There are several ioctls for use with `perf_event_open` file descriptors;
//! see the [`ioctls`] module for those.
//!
//! For a safe and convenient interface to this functionality, see the
//! [`perf_event`] crate.
//!
//! ## Using the raw API
//!
//! As the kernel interface evolves, the struct and union types from the
//! [`bindings`] module may acquire new fields. To ensure that your code will
//! continue to compile against newer versions of this crate, you should
//! construct values of these types by calling their `Default` implementations,
//! which return zero-filled values, and then assigning to the fields you care
//! about. For example:
//!
//! ```
//! use perf_event_open_sys as sys;
//!
//! // Construct a zero-filled `perf_event_attr`.
//! let mut attrs = sys::bindings::perf_event_attr::default();
//!
//! // Populate the fields we need.
//! attrs.size = std::mem::size_of::<sys::bindings::perf_event_attr>() as u32;
//! attrs.type_ = sys::bindings::perf_type_id_PERF_TYPE_HARDWARE;
//! attrs.config = sys::bindings::perf_hw_id_PERF_COUNT_HW_INSTRUCTIONS as u64;
//! attrs.set_disabled(1);
//! attrs.set_exclude_kernel(1);
//! attrs.set_exclude_hv(1);
//!
//! // Make the system call.
//! let result = unsafe {
//!     sys::perf_event_open(&mut attrs, 0, -1, -1, 0)
//! };
//!
//! if result < 0 {
//!     // ... handle error
//! }
//!
//! // ... use `result` as a raw file descriptor
//! ```
//!
//! It is not necessary to adjust `size` to what the running kernel expects:
//! older kernels can accept newer `perf_event_attr` structs, and vice versa. As
//! long as the `size` field was properly initialized, an error result of
//! `E2BIG` indicates that the `attrs` structure has requested behavior the
//! kernel is too old to support.
//!
//! When `E2BIG` is returned, the kernel writes the size it expected back to the
//! `size` field of the `attrs` struct. Again, if you want to retry the call, it
//! is not necessary to adjust the size you pass to match what the kernel passed
//! back. The size from the kernel just indicates which version of the API the
//! kernel supports; see the documentation for the `PERF_EVENT_ATTR_SIZE_VER...`
//! constants for details.
//!
//! ## Kernel versions
//!
//! The bindings in this crate are generated from the Linux kernel headers
//! packaged by Fedora as `kernel-headers-5.6.11-100.fc30.x86_64`, which
//! corresponds to `PERF_EVENT_ATTR_SIZE_VER6`.
//!
//! As explained above, bugs aside, it is not necessary to use the version of
//! these structures that matches the kernel you want to run under, so it should
//! always be acceptable to use the latest version of this crate, even if you
//! want to support older kernels.
//!
//! This crate's `README.md` file includes instructions on regenerating the
//! bindings from newer kernel headers. However, this can be a breaking change
//! for users that have not followed the advice above, so regeneration should
//! cause a major version increment.
//!
//! If you need features that are available only in a more recent version of the
//! types than this crate provides, please file an issue.
//!
//! ## Linux API Backward/Forward Compatibility Strategy
//!
//! (This is more detail than necessary if you just want to use the crate. I
//! want to write this down somewhere so that I have something to refer to when
//! I forget the details.)
//!
//! It is an important principle of Linux kernel development that new versions
//! of the kernel should not break userspace. If upgrading your kernel breaks a
//! user program, then that's a bug in the kernel. (This refers to the run-time
//! interface. I don't know what the stability rules are for the kernel headers:
//! can new headers cause old code to fail to compile? Anyway, run time is our
//! concern here.)
//!
//! But when you have an open-ended, complex system call like `perf_event_open`,
//! it's really important for the interface to be able to evolve. Certainly, old
//! programs must run properly on new kernels, but ideally, it should work the
//! other way, too: a program built against a newer version of the kernel
//! headers should run on an older kernel, as long as it only requests features
//! the old kernel actually supports. That is, simply compiling against newer
//! headers should not be disqualifying - only using those new headers to
//! request features the running kernel can't provide should cause an error.
//!
//! Consider the specific case of passing a struct like `perf_event_attr` to a
//! system call like `perf_event_open`. In general, there are two versions of
//! the struct in play: the version the user program was compiled against, and
//! the version the running kernel was compiled against. How can we let old
//! programs call `perf_event_open` on new kernels, and vice versa?
//!
//! Linux has a neat strategy for making this work. There are four rules:
//!
//! -   Every system call that passes a struct to the kernel includes some
//!     indication of how large userspace thinks that struct is. For
//!     `perf_event_open`, it's the `size` field of the `perf_event_attr`
//!     struct. For `ioctl`s that pass a struct, it's a bitfield of the
//!     `request` value.
//!
//! -   Fields are never deleted from structs. At most, newer kernel headers may
//!     rename them to '__reserved_foo' or something like that, but once a field
//!     has been placed, its layout in the struct never changes.
//!
//! -   New fields are added to the end of structs.
//!
//! -   New fields' semantics are chosen such that filling them with zeros
//!     preserves the old behavior. That is, turning an old struct into a new
//!     struct by extending it with zero bytes should always give you a new
//!     struct with the same meaning the old struct had.
//!
//! Then, the kernel's strategy for receiving structs from userspace (explained
//! by the kernel comments for `copy_struct_from_user` in
//! `include/linux/uaccess.h`) is as follows:
//!
//! -   If the kernel's struct is larger than the one passed from userspace,
//!     then that means the kernel is newer than the userspace program. The
//!     kernel copies the userspace data into the initial bytes of its own
//!     struct, and zeros the remaining bytes. Since zeroed fields have no
//!     effect, the resulting struct properly reflects the user's intent.
//!
//! -   If the kernel's struct is smaller than the one passed from userspace,
//!     then that means that a userspace program compiled against newer kernel
//!     headers is running on an older kernel. The kernel checks that the excess
//!     bytes in the userspace struct are all zero; if they are not, the system
//!     call returns `E2BIG`, indicating that userspace has requested a feature
//!     the kernel doesn't support. If they are all zero, then the kernel
//!     initializes its own struct with the bytes from the start of the
//!     userspace struct, and drops the rest. Since the dropped bytes were all
//!     zero, they did not affect the requested behavior, and the resulting
//!     struct reflects the user's intent.
//!
//! -   In either case, the kernel verifies that any `__reserved_foo` fields in
//!     its own version of the struct are zero.
//!
//! This covers both the old-on-new and new-on-old cases, and returns an error
//! only when the call requests functionality the kernel doesn't support.
//!
//! You can find one example of using `perf_event_open` in the [`perf_event`]
//! crate, which provides a safe interface to a subset of `perf_event_open`'s
//! functionality.
//!
//! [`bindings`]: bindings/index.html
//! [`ioctls`]: ioctls/index.html
//! [man]: http://man7.org/linux/man-pages/man2/perf_event_open.2.html
//! [`perf_event`]: https://crates.io/crates/perf_event

pub mod bindings;

use libc::pid_t;
use std::os::raw::{c_int, c_ulong};

/// The `perf_event_open` system call.
///
/// See the [`perf_event_open(2) man page`][man] for details.
///
/// On error, this returns a negated raw OS error value. The C `errno` value is
/// not changed.
///
/// Note: The `attrs` argument needs to be a `*mut` because if the `size` field
/// is too small or too large, the kernel writes the size it was expecing back
/// into that field. It might do other things as well.
///
/// [man]: http://man7.org/linux/man-pages/man2/perf_event_open.2.html
pub unsafe fn perf_event_open(
    attrs: *mut bindings::perf_event_attr,
    pid: pid_t,
    cpu: c_int,
    group_fd: c_int,
    flags: c_ulong,
) -> c_int {
    libc::syscall(
        bindings::__NR_perf_event_open as libc::c_long,
        attrs as *const bindings::perf_event_attr,
        pid,
        cpu,
        group_fd,
        flags,
    ) as c_int
}

#[allow(dead_code, non_snake_case)]
pub mod ioctls {
    //! Ioctls for use with `perf_event_open` file descriptors.
    //!
    //! See the [`perf_event_open(2)`][man] man page for details.
    //!
    //! On error, these return `-1` and set the C `errno` value.
    //!
    //! [man]: http://man7.org/linux/man-pages/man2/perf_event_open.2.html
    use crate::bindings::{self, perf_event_attr, perf_event_query_bpf};
    use std::os::raw::{c_char, c_int, c_uint, c_ulong};

    macro_rules! define_ioctls {
        ( $( $args:tt )* ) => {
            $(
                define_ioctl!($args);
            )*
        }
    }

    macro_rules! define_ioctl {
        ({ $name:ident, $ioctl:ident, $arg_type:ty }) => {
            pub unsafe fn $name(fd: c_int, arg: $arg_type) -> c_int {
                untyped_ioctl(fd, bindings::$ioctl, arg)
            }
        };
    }

    define_ioctls! {
        { ENABLE, perf_event_ioctls_ENABLE, c_uint }
        { DISABLE, perf_event_ioctls_DISABLE, c_uint }
        { REFRESH, perf_event_ioctls_REFRESH, c_int }
        { RESET, perf_event_ioctls_RESET, c_uint }
        { PERIOD, perf_event_ioctls_PERIOD, u64 }
        { SET_OUTPUT, perf_event_ioctls_SET_OUTPUT, c_int }
        { SET_FILTER, perf_event_ioctls_SET_FILTER, *mut c_char }
        { ID, perf_event_ioctls_ID, *mut u64 }
        { SET_BPF, perf_event_ioctls_SET_BPF, u32 }
        { PAUSE_OUTPUT, perf_event_ioctls_PAUSE_OUTPUT, u32 }
        { QUERY_BPF, perf_event_ioctls_QUERY_BPF, *mut perf_event_query_bpf }
        { MODIFY_ATTRIBUTES, perf_event_ioctls_MODIFY_ATTRIBUTES, *mut perf_event_attr }
    }

    unsafe fn untyped_ioctl<A>(
        fd: c_int,
        ioctl: bindings::perf_event_ioctls,
        arg: A,
    ) -> c_int {
        #[cfg(target_env = "musl")]
        return libc::ioctl(fd, ioctl as c_int, arg);

        #[cfg(not(target_env = "musl"))]
        libc::ioctl(fd, ioctl as c_ulong, arg)
    }
}
