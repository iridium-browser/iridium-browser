// Example calling out to libc qsort.

use libffi::high::Closure2;

mod c {
    use libffi::high::FnPtr2;
    use std::os::raw::{c_int, c_void};

    pub type Callback<'a> = FnPtr2<'a, *const c_void, *const c_void, c_int>;

    extern "C" {
        pub fn qsort(base: *const c_void, nel: usize, width: usize, compar: Callback);
    }
}

fn qsort<T: Ord>(array: &mut [T]) {
    use std::cmp::Ordering::*;
    use std::mem;
    use std::os::raw::c_void;

    let lambda = |x: *const c_void, y: *const c_void| {
        let x = unsafe { &*(x as *const T) };
        let y = unsafe { &*(y as *const T) };
        match x.cmp(y) {
            Less => -1,
            Equal => 0,
            Greater => 1,
        }
    };
    let compare = Closure2::new(&lambda);

    unsafe {
        c::qsort(
            array.as_ptr() as *const _,
            array.len(),
            mem::size_of::<T>(),
            *compare.code_ptr(),
        )
    }
}

fn main() {
    let mut v = vec![3, 4, 8, 1, 2, 0, 9];
    qsort(&mut v);

    assert_eq!(vec![0, 1, 2, 3, 4, 8, 9], v);
}
