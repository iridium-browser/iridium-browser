use alloc::string::String;
use core::mem::transmute;
use core::panic::BoxMeUp;
use core::ptr::copy_nonoverlapping;

// Forward the abort message to libc's android_set_abort_message. We try our best to populate the
// message but as this function may already be called as part of a failed allocation, it might not be
// possible to do so.
//
// Some methods of core are on purpose avoided (such as try_reserve) as these rely on the correct
// resolution of rust_eh_personality which is loosely defined in panic_abort.
pub(crate) unsafe fn android_set_abort_message(payload: *mut &mut dyn BoxMeUp) {
    let payload = (*payload).get();
    let msg = match payload.downcast_ref::<&'static str>() {
        Some(msg) => msg.as_bytes(),
        None => match payload.downcast_ref::<String>() {
            Some(msg) => msg.as_bytes(),
            None => &[],
        },
    };
    if msg.is_empty() {
        return;
    }

    // Allocate a new buffer to append the null byte.
    let size = msg.len() + 1usize;
    let buf = libc::malloc(size) as *mut libc::c_char;
    if buf.is_null() {
        return; // allocation failure
    }
    copy_nonoverlapping(msg.as_ptr(), buf as *mut u8, msg.len());
    buf.offset(msg.len() as isize).write(0);

    libc::android_set_abort_message(buf);
}
