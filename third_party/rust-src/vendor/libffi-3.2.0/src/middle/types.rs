//! Representations of C types and arrays thereof.
//!
//! These are used to describe the types of the arguments and results of
//! functions. When we construct a [CIF](super::Cif) (“Call
//! Inter<span></span>Face”), we provide a sequence of argument types
//! and a result type, and libffi uses this to figure out how to set up
//! a call to a function with those types.

use libc;
use std::fmt;
use std::mem;
use std::ptr;

use crate::low;

use super::util::Unique;

// Internally we represent types and type arrays using raw pointers,
// since this is what libffi understands. Below we wrap them with
// types that implement Drop and Clone.

type Type_ = *mut low::ffi_type;
type TypeArray_ = *mut Type_;

// Informal indication that the object should be considered owned by
// the given reference.
type Owned<T> = T;

/// Represents a single C type.
///
/// # Example
///
/// Suppose we have a C struct:
///
/// ```c
/// struct my_struct {
///     uint16_t f1;
///     uint64_t f2;
/// };
/// ```
///
/// To pass the struct by value via libffi, we need to construct a
/// `Type` object describing its layout:
///
/// ```
/// use libffi::middle::Type;
///
/// let my_struct = Type::structure(vec![
///     Type::u64(),
///     Type::u16(),
/// ]);
/// ```
pub struct Type(Unique<low::ffi_type>);

/// Represents a sequence of C types.
///
/// This can be used to construct a struct type or as the arguments
/// when creating a [`Cif`].
pub struct TypeArray(Unique<*mut low::ffi_type>);

impl fmt::Debug for Type {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        formatter.write_fmt(format_args!("Type({:?})", *self.0))
    }
}

impl fmt::Debug for TypeArray {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        formatter.write_fmt(format_args!("TypeArray({:?})", *self.0))
    }
}

/// Computes the length of a raw `TypeArray_` by searching for the
/// null terminator.
unsafe fn ffi_type_array_len(mut array: TypeArray_) -> usize {
    let mut count = 0;
    while !(*array).is_null() {
        count += 1;
        array = array.offset(1);
    }
    count
}

/// Creates an empty `TypeArray_` with null terminator.
unsafe fn ffi_type_array_create_empty(len: usize) -> Owned<TypeArray_> {
    let array = libc::malloc((len + 1) * mem::size_of::<Type_>()) as TypeArray_;
    assert!(
        !array.is_null(),
        "ffi_type_array_create_empty: out of memory"
    );
    *array.add(len) = ptr::null_mut::<low::ffi_type>() as Type_;
    array
}

/// Creates a null-terminated array of Type_. Takes ownership of
/// the elements.
unsafe fn ffi_type_array_create<I>(elements: I) -> Owned<TypeArray_>
where
    I: ExactSizeIterator<Item = Type>,
{
    let size = elements.len();
    let new = ffi_type_array_create_empty(size);
    for (i, element) in elements.enumerate() {
        *new.add(i) = *element.0;
        mem::forget(element);
    }

    new
}

/// Creates a struct type from a raw array of element types.
unsafe fn ffi_type_struct_create_raw(
    elements: Owned<TypeArray_>,
    size: usize,
    alignment: u16,
) -> Owned<Type_> {
    let new = libc::malloc(mem::size_of::<low::ffi_type>()) as Type_;
    assert!(!new.is_null(), "ffi_type_struct_create_raw: out of memory");

    (*new).size = size;
    (*new).alignment = alignment;
    (*new).type_ = low::type_tag::STRUCT;
    (*new).elements = elements;

    new
}

/// Creates a struct `ffi_type` with the given elements. Takes ownership
/// of the elements.
unsafe fn ffi_type_struct_create<I>(elements: I) -> Owned<Type_>
where
    I: ExactSizeIterator<Item = Type>,
{
    ffi_type_struct_create_raw(ffi_type_array_create(elements), 0, 0)
}

/// Makes a copy of a type array.
unsafe fn ffi_type_array_clone(old: TypeArray_) -> Owned<TypeArray_> {
    let size = ffi_type_array_len(old);
    let new = ffi_type_array_create_empty(size);

    for i in 0..size {
        *new.add(i) = ffi_type_clone(*old.add(i));
    }

    new
}

/// Makes a copy of a type.
unsafe fn ffi_type_clone(old: Type_) -> Owned<Type_> {
    if (*old).type_ == low::type_tag::STRUCT {
        let low::ffi_type {
            alignment,
            elements,
            size,
            ..
        } = *old;
        let new = ffi_type_struct_create_raw(ffi_type_array_clone(elements), size, alignment);
        new
    } else {
        old
    }
}

/// Destroys a `TypeArray_` and all of its elements.
unsafe fn ffi_type_array_destroy(victim: Owned<TypeArray_>) {
    let mut current = victim;
    while !(*current).is_null() {
        ffi_type_destroy(*current);
        current = current.offset(1);
    }

    libc::free(victim as *mut libc::c_void);
}

/// Destroys a `Type_` if it was dynamically allocated.
unsafe fn ffi_type_destroy(victim: Owned<Type_>) {
    if (*victim).type_ == low::type_tag::STRUCT {
        ffi_type_array_destroy((*victim).elements);
        libc::free(victim as *mut libc::c_void);
    }
}

impl Drop for Type {
    fn drop(&mut self) {
        unsafe { ffi_type_destroy(*self.0) }
    }
}

impl Drop for TypeArray {
    fn drop(&mut self) {
        unsafe { ffi_type_array_destroy(*self.0) }
    }
}

impl Clone for Type {
    fn clone(&self) -> Self {
        Type(unsafe { Unique::new(ffi_type_clone(*self.0)) })
    }
}

impl Clone for TypeArray {
    fn clone(&self) -> Self {
        TypeArray(unsafe { Unique::new(ffi_type_array_clone(*self.0)) })
    }
}

macro_rules! match_size_signed {
    ( $name:ident ) => {
        match mem::size_of::<libc::$name>() {
            1 => Self::i8(),
            2 => Self::i16(),
            4 => Self::i32(),
            8 => Self::i64(),
            _ => panic!("Strange size for C type"),
        }
    };
}

macro_rules! match_size_unsigned {
    ( $name:ident ) => {
        match mem::size_of::<libc::$name>() {
            1 => Self::u8(),
            2 => Self::u16(),
            4 => Self::u32(),
            8 => Self::u64(),
            _ => panic!("Strange size for C type"),
        }
    };
}

impl Type {
    /// Returns the representation of the C `void` type.
    ///
    /// This is used only for the return type of a [CIF](super::Cif),
    /// not for an argument or struct member.
    pub fn void() -> Self {
        Type(unsafe { Unique::new(&mut low::types::void) })
    }

    /// Returns the unsigned 8-bit numeric type.
    pub fn u8() -> Self {
        Type(unsafe { Unique::new(&mut low::types::uint8) })
    }

    /// Returns the signed 8-bit numeric type.
    pub fn i8() -> Self {
        Type(unsafe { Unique::new(&mut low::types::sint8) })
    }

    /// Returns the unsigned 16-bit numeric type.
    pub fn u16() -> Self {
        Type(unsafe { Unique::new(&mut low::types::uint16) })
    }

    /// Returns the signed 16-bit numeric type.
    pub fn i16() -> Self {
        Type(unsafe { Unique::new(&mut low::types::sint16) })
    }

    /// Returns the unsigned 32-bit numeric type.
    pub fn u32() -> Self {
        Type(unsafe { Unique::new(&mut low::types::uint32) })
    }

    /// Returns the signed 32-bit numeric type.
    pub fn i32() -> Self {
        Type(unsafe { Unique::new(&mut low::types::sint32) })
    }

    /// Returns the unsigned 64-bit numeric type.
    pub fn u64() -> Self {
        Type(unsafe { Unique::new(&mut low::types::uint64) })
    }

    /// Returns the signed 64-bit numeric type.
    pub fn i64() -> Self {
        Type(unsafe { Unique::new(&mut low::types::sint64) })
    }

    #[cfg(target_pointer_width = "16")]
    /// Returns the C equivalent of Rust `usize` (`u16`).
    pub fn usize() -> Self {
        Self::u16()
    }

    #[cfg(target_pointer_width = "16")]
    /// Returns the C equivalent of Rust `isize` (`i16`).
    pub fn isize() -> Self {
        Self::i16()
    }

    #[cfg(target_pointer_width = "32")]
    /// Returns the C equivalent of Rust `usize` (`u32`).
    pub fn usize() -> Self {
        Self::u32()
    }

    #[cfg(target_pointer_width = "32")]
    /// Returns the C equivalent of Rust `isize` (`i32`).
    pub fn isize() -> Self {
        Self::i32()
    }

    #[cfg(target_pointer_width = "64")]
    /// Returns the C equivalent of Rust `usize` (`u64`).
    pub fn usize() -> Self {
        Self::u64()
    }

    #[cfg(target_pointer_width = "64")]
    /// Returns the C equivalent of Rust `isize` (`i64`).
    pub fn isize() -> Self {
        Self::i64()
    }

    /// Returns the C `signed char` type.
    pub fn c_schar() -> Self {
        match_size_signed!(c_schar)
    }

    /// Returns the C `unsigned char` type.
    pub fn c_uchar() -> Self {
        match_size_unsigned!(c_uchar)
    }

    /// Returns the C `short` type.
    pub fn c_short() -> Self {
        match_size_signed!(c_short)
    }

    /// Returns the C `unsigned short` type.
    pub fn c_ushort() -> Self {
        match_size_unsigned!(c_ushort)
    }

    /// Returns the C `int` type.
    pub fn c_int() -> Self {
        match_size_signed!(c_int)
    }

    /// Returns the C `unsigned int` type.
    pub fn c_uint() -> Self {
        match_size_unsigned!(c_uint)
    }

    /// Returns the C `long` type.
    pub fn c_long() -> Self {
        match_size_signed!(c_long)
    }

    /// Returns the C `unsigned long` type.
    pub fn c_ulong() -> Self {
        match_size_unsigned!(c_ulong)
    }

    /// Returns the C `longlong` type.
    pub fn c_longlong() -> Self {
        match_size_signed!(c_longlong)
    }

    /// Returns the C `unsigned longlong` type.
    pub fn c_ulonglong() -> Self {
        match_size_unsigned!(c_ulonglong)
    }

    /// Returns the C `float` (32-bit floating point) type.
    pub fn f32() -> Self {
        Type(unsafe { Unique::new(&mut low::types::float) })
    }

    /// Returns the C `double` (64-bit floating point) type.
    pub fn f64() -> Self {
        Type(unsafe { Unique::new(&mut low::types::double) })
    }

    /// Returns the C `void*` type, for passing any kind of pointer.
    pub fn pointer() -> Self {
        Type(unsafe { Unique::new(&mut low::types::pointer) })
    }

    /// Returns the C `long double` (extended-precision floating point) type.
    #[cfg(not(any(target_arch = "arm", target_arch = "aarch64")))]
    pub fn longdouble() -> Self {
        Type(unsafe { Unique::new(&mut low::types::longdouble) })
    }

    /// Returns the C `_Complex float` type.
    ///
    /// This item is enabled by `#[cfg(feature = "complex")]`.
    #[cfg(feature = "complex")]
    pub fn c32() -> Self {
        Type(unsafe { Unique::new(&mut low::types::complex_float) })
    }

    /// Returns the C `_Complex double` type.
    ///
    /// This item is enabled by `#[cfg(feature = "complex")]`.
    #[cfg(feature = "complex")]
    pub fn c64() -> Self {
        Type(unsafe { Unique::new(&mut low::types::complex_double) })
    }

    /// Returns the C `_Complex long double` type.
    ///
    /// This item is enabled by `#[cfg(feature = "complex")]`.
    #[cfg(feature = "complex")]
    #[cfg(not(all(target_arch = "arm")))]
    pub fn complex_longdouble() -> Self {
        Type(unsafe { Unique::new(&mut low::types::complex_longdouble) })
    }

    /// Constructs a structure type whose fields have the given types.
    pub fn structure<I>(fields: I) -> Self
    where
        I: IntoIterator<Item = Type>,
        I::IntoIter: ExactSizeIterator<Item = Type>,
    {
        Type(unsafe { Unique::new(ffi_type_struct_create(fields.into_iter())) })
    }

    /// Gets a raw pointer to the underlying [`low::ffi_type`].
    ///
    /// This method may be useful for interacting with the
    /// [`low`](crate::low) and [`raw`](crate::raw) layers.
    pub fn as_raw_ptr(&self) -> *mut low::ffi_type {
        *self.0
    }
}

impl TypeArray {
    /// Constructs an array the given `Type`s.
    pub fn new<I>(elements: I) -> Self
    where
        I: IntoIterator<Item = Type>,
        I::IntoIter: ExactSizeIterator<Item = Type>,
    {
        TypeArray(unsafe { Unique::new(ffi_type_array_create(elements.into_iter())) })
    }

    /// Gets a raw pointer to the underlying C array of
    /// [`low::ffi_type`]s.
    ///
    /// The C array is null-terminated.
    ///
    /// This method may be useful for interacting with the
    /// [`low`](crate::low) and [`raw`](crate::raw) layers.
    pub fn as_raw_ptr(&self) -> *mut *mut low::ffi_type {
        *self.0
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn create_u64() {
        Type::u64();
    }

    #[test]
    fn clone_u64() {
        let _ = Type::u64().clone().clone();
    }

    #[test]
    fn create_struct() {
        Type::structure(vec![Type::i64(), Type::i64(), Type::u64()]);
    }

    #[test]
    fn clone_struct() {
        let _ = Type::structure(vec![Type::i64(), Type::i64(), Type::u64()])
            .clone()
            .clone();
    }
}
