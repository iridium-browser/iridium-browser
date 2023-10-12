//! Representations of C types for the high layer.

use std::marker::PhantomData;

use super::super::low;
use super::super::middle;

/// Represents a C type statically associated with a Rust type.
///
/// In particular, the run-time value describes a particular C type,
/// while the type parameter `T` is the equivalent Rust type.
/// Instances of this type are created via the [`CType`] trait.
#[derive(Clone, Debug)]
pub struct Type<T> {
    untyped: middle::Type,
    _marker: PhantomData<*mut T>,
}

impl<T> Type<T> {
    fn make(untyped: middle::Type) -> Self {
        Type {
            untyped,
            _marker: PhantomData,
        }
    }

    /// Gets the underlying representation as used by the
    /// [`mod@middle`] layer.
    pub fn into_middle(self) -> middle::Type {
        self.untyped
    }
}

/// Types that we can automatically marshall to/from C.
///
/// In particular, for any type `T` that implements `CType`, we can
/// get a `Type<T>` for describing that type.
/// This trait is unsafe to implement because if the libffi type
/// associated with a Rust type doesn’t match then we get
/// undefined behavior.
pub unsafe trait CType: Copy {
    /// Creates or retrieves a `Type<T>` for any type `T: CType`.
    ///
    /// We can use the resulting object to assemble a CIF to set up
    /// a call that uses type `T`.
    fn reify() -> Type<Self>;
    /// The low-level libffi library implicitly extends small integer
    /// return values to `ffi_arg` or `ffi_sarg`.  Track the possibly
    /// extended variant of `T` as an associated type here.
    type RetType: std::convert::From<Self> + std::convert::TryInto<Self>;
}

macro_rules! impl_ffi_type {
    ($type_:ty, $ret_:ty, $cons:ident) => {
        unsafe impl CType for $type_ {
            fn reify() -> Type<Self> {
                Type::make(middle::Type::$cons())
            }
            type RetType = $ret_;
        }
    };
    ($type_:ident, $ret_:ty) => {
        impl_ffi_type!($type_, $ret_, $type_);
    };
    ($type_:ident) => {
        impl_ffi_type!($type_, $type_, $type_);
    };
}

// We assume that `ffi_arg` and `ffi_sarg` are either 32-bit or 64-bit
// integer types on all supported platforms here.
impl_ffi_type!(u8, low::ffi_arg);
impl_ffi_type!(i8, low::ffi_sarg);
impl_ffi_type!(u16, low::ffi_arg);
impl_ffi_type!(i16, low::ffi_sarg);
impl_ffi_type!(u32, low::ffi_arg);
impl_ffi_type!(i32, low::ffi_sarg);
impl_ffi_type!(u64);
impl_ffi_type!(i64);
impl_ffi_type!(f32);
impl_ffi_type!(f64);
impl_ffi_type!(usize);
impl_ffi_type!(isize);
impl_ffi_type!((), (), void);

// Why is the complex stuff even here? It doesn’t work yet because
// libffi doesn’t support it, so it should probably go away and come
// back when it’s actually useful. Also, the definitions for c_c32 and
// c_c64 should come from elsewhere (the num package?), but that
// elsewhere doesn’t seem to exist yet.

/// Laid out the same as C11 `float complex` and C++11
/// `std::complex<float>`.
///
/// This item is enabled by `#[cfg(feature = "complex")]`.
///
/// # Warning
///
/// This type does not obey the ABI, and as such should not be passed by
/// value to or from a C or C++ function. Passing it via a pointer is
/// okay. Theoretically, passing it via libffi is okay, but libffi
/// doesn’t have complex support on most platforms yet.
#[allow(non_camel_case_types)]
#[cfg(feature = "complex")]
pub type c_c32 = [f32; 2];

/// Laid out the same as C11 `double complex` and C++11
/// `std::complex<double>`.
///
/// This item is enabled by `#[cfg(feature = "complex")]`.
///
/// # Warning
///
/// This type does not obey the ABI, and as such should not be passed by
/// value to or from a C or C++ function. Passing it via a pointer is
/// okay. Theoretically, passing it via libffi is okay, but libffi
/// doesn’t have complex support on most platforms yet.
#[allow(non_camel_case_types)]
#[cfg(feature = "complex")]
pub type c_c64 = [f64; 2];

#[cfg(feature = "complex")]
impl_ffi_type!(c_c32, c32);

#[cfg(feature = "complex")]
impl_ffi_type!(c_c64, c64);

unsafe impl<T> CType for *const T {
    fn reify() -> Type<Self> {
        Type::make(middle::Type::pointer())
    }
    type RetType = *const T;
}

unsafe impl<T> CType for *mut T {
    fn reify() -> Type<Self> {
        Type::make(middle::Type::pointer())
    }
    type RetType = *mut T;
}
