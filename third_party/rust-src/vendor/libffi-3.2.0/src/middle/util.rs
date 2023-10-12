use std::marker::PhantomData;
use std::ops::Deref;

pub struct Unique<T> {
    contents: *mut T,
    _marker: PhantomData<T>,
}

impl<T> Deref for Unique<T> {
    type Target = *mut T;
    fn deref(&self) -> &Self::Target {
        &self.contents
    }
}

impl<T> Unique<T> {
    pub unsafe fn new(ptr: *mut T) -> Self {
        Unique {
            contents: ptr,
            _marker: PhantomData,
        }
    }
}
