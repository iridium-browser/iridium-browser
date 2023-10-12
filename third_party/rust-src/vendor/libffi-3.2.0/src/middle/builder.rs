use std::any::Any;

use super::types::Type;

/// Provides a builder-style API for constructing CIFs and closures.
///
/// To use a builder, first construct it using [`Builder::new`]. The
/// default calling convention is
/// [`ffi_abi_FFI_DEFAULT_ABI`](crate::low::ffi_abi_FFI_DEFAULT_ABI),
/// and the default function type is `extern "C" fn()` (or in C,
/// `void(*)()`). Add argument types to the function type with the
/// [`Builder::arg`] and [`args`](Builder::args) methods. Set the result type
/// with [`Builder::res`]. Change the calling convention, if necessary,
/// with [`Builder::abi`].
///
/// Once the builder is configured, construct a `Cif` with
/// [`Builder::into_cif`] or a closure with [`Builder::into_closure`],
/// [`into_closure_mut`](Builder::into_closure_mut), or
/// [`into_closure_once`](Builder::into_closure_once).
///
/// # Examples
///
/// ```
/// use std::mem;
/// use std::os::raw::c_void;
///
/// use libffi::middle::*;
/// use libffi::low;
///
/// unsafe extern "C" fn lambda_callback<F: Fn(u64, u64) -> u64>(
///     _cif: &low::ffi_cif,
///     result: &mut u64,
///     args: *const *const c_void,
///     userdata: &F)
/// {
///     let args: *const &u64 = mem::transmute(args);
///     let arg1 = **args.offset(0);
///     let arg2 = **args.offset(1);
///
///     *result = userdata(arg1, arg2);
/// }
///
/// let lambda = |x: u64, y: u64| x + y;
///
/// let closure = Builder::new()
///     .arg(Type::u64())
///     .arg(Type::u64())
///     .res(Type::u64())
///     .into_closure(lambda_callback, &lambda);
///
/// unsafe {
///     let fun: &unsafe extern "C" fn(u64, u64) -> u64
///         = mem::transmute(closure.code_ptr());
///
///     assert_eq!(11, fun(5, 6));
///     assert_eq!(12, fun(5, 7));
/// }
/// ```
#[derive(Clone, Debug)]
pub struct Builder {
    args: Vec<Type>,
    res: Type,
    abi: super::FfiAbi,
}

impl Default for Builder {
    fn default() -> Self {
        Builder::new()
    }
}

impl Builder {
    /// Constructs a `Builder`.
    pub fn new() -> Self {
        Builder {
            args: vec![],
            res: Type::void(),
            abi: super::ffi_abi_FFI_DEFAULT_ABI,
        }
    }

    /// Adds a type to the argument type list.
    pub fn arg(mut self, type_: Type) -> Self {
        self.args.push(type_);
        self
    }

    /// Adds several types to the argument type list.
    pub fn args<I>(mut self, types: I) -> Self
    where
        I: IntoIterator<Item = Type>,
    {
        self.args.extend(types.into_iter());
        self
    }

    /// Sets the result type.
    pub fn res(mut self, type_: Type) -> Self {
        self.res = type_;
        self
    }

    /// Sets the calling convention.
    pub fn abi(mut self, abi: super::FfiAbi) -> Self {
        self.abi = abi;
        self
    }

    /// Builds a CIF.
    pub fn into_cif(self) -> super::Cif {
        let mut result = super::Cif::new(self.args, self.res);
        result.set_abi(self.abi);
        result
    }

    /// Builds an immutable closure.
    ///
    /// # Arguments
    ///
    /// - `callback` — the function to call when the closure is invoked
    /// - `userdata` — the pointer to pass to `callback` along with the
    ///   arguments when the closure is called
    ///
    /// # Result
    ///
    /// The new closure.
    pub fn into_closure<U, R>(
        self,
        callback: super::Callback<U, R>,
        userdata: &U,
    ) -> super::Closure {
        super::Closure::new(self.into_cif(), callback, userdata)
    }

    /// Builds a mutable closure.
    ///
    /// # Arguments
    ///
    /// - `callback` — the function to call when the closure is invoked
    /// - `userdata` — the pointer to pass to `callback` along with the
    ///   arguments when the closure is called
    ///
    /// # Result
    ///
    /// The new closure.
    pub fn into_closure_mut<U, R>(
        self,
        callback: super::CallbackMut<U, R>,
        userdata: &mut U,
    ) -> super::Closure {
        super::Closure::new_mut(self.into_cif(), callback, userdata)
    }

    /// Builds a one-shot closure.
    ///
    /// # Arguments
    ///
    /// - `callback` — the function to call when the closure is invoked
    /// - `userdata` — the object to pass to `callback` along with the
    ///   arguments when the closure is called
    ///
    /// # Result
    ///
    /// The new closure.
    pub fn into_closure_once<U: Any, R>(
        self,
        callback: super::CallbackOnce<U, R>,
        userdata: U,
    ) -> super::ClosureOnce {
        super::ClosureOnce::new(self.into_cif(), callback, userdata)
    }
}
