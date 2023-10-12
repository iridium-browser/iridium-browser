use std::cell::UnsafeCell;
use std::cmp::Ordering;
use std::iter::FromIterator;
use std::ops::Index;

use stable_deref_trait::StableDeref;

/// Append-only version of `std::vec::Vec` where
/// insertion does not require mutable access
pub struct FrozenVec<T> {
    vec: UnsafeCell<Vec<T>>,
    // XXXManishearth do we need a reentrancy guard here as well?
    // StableDeref may not guarantee that there are no side effects
}

// safety: UnsafeCell implies !Sync

impl<T> FrozenVec<T> {
    /// Constructs a new, empty vector.
    pub fn new() -> Self {
        Self {
            vec: UnsafeCell::new(Default::default()),
        }
    }
}

impl<T> FrozenVec<T> {
    // these should never return &T
    // these should never delete any entries

    /// Appends an element to the back of the vector.
    pub fn push(&self, val: T) {
        unsafe {
            let vec = self.vec.get();
            (*vec).push(val)
        }
    }
}

impl<T: StableDeref> FrozenVec<T> {
    /// Push, immediately getting a reference to the element
    pub fn push_get(&self, val: T) -> &T::Target {
        unsafe {
            let vec = self.vec.get();
            (*vec).push(val);
            &*(&**(*vec).get_unchecked((*vec).len() - 1) as *const T::Target)
        }
    }

    /// Returns a reference to an element.
    pub fn get(&self, index: usize) -> Option<&T::Target> {
        unsafe {
            let vec = self.vec.get();
            (*vec).get(index).map(|x| &**x)
        }
    }

    /// Returns a reference to an element, without doing bounds checking.
    ///
    /// ## Safety
    ///
    /// `index` must be in bounds, i.e. it must be less than `self.len()`
    pub unsafe fn get_unchecked(&self, index: usize) -> &T::Target {
        let vec = self.vec.get();
        &**(*vec).get_unchecked(index)
    }
}

impl<T: Copy> FrozenVec<T> {
    /// Returns a copy of an element.
    pub fn get_copy(&self, index: usize) -> Option<T> {
        unsafe {
            let vec = self.vec.get();
            (*vec).get(index).copied()
        }
    }
}

impl<T> FrozenVec<T> {
    /// Returns the number of elements in the vector.
    pub fn len(&self) -> usize {
        unsafe {
            let vec = self.vec.get();
            (*vec).len()
        }
    }

    /// Returns `true` if the vector contains no elements.
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

impl<T: StableDeref> FrozenVec<T> {
    /// Returns the first element of the vector, or `None` if empty.
    pub fn first(&self) -> Option<&T::Target> {
        unsafe {
            let vec = self.vec.get();
            (*vec).first().map(|x| &**x)
        }
    }

    /// Returns the last element of the vector, or `None` if empty.
    pub fn last(&self) -> Option<&T::Target> {
        unsafe {
            let vec = self.vec.get();
            (*vec).last().map(|x| &**x)
        }
    }
    /// Returns an iterator over the vector.
    pub fn iter(&self) -> Iter<T> {
        self.into_iter()
    }
}

impl<T: StableDeref> FrozenVec<T> {
    /// Converts the frozen vector into a plain vector.
    pub fn into_vec(self) -> Vec<T> {
        self.vec.into_inner()
    }
}

impl<T: StableDeref> FrozenVec<T> {
    // binary search functions: they need to be reimplemented here to be safe (instead of calling
    // their equivalents directly on the underlying Vec), as they run user callbacks that could
    // reentrantly call other functions on this vector

    /// Binary searches this sorted vector for a given element, analogous to [slice::binary_search].
    pub fn binary_search(&self, x: &T::Target) -> Result<usize, usize>
    where
        T::Target: Ord,
    {
        self.binary_search_by(|p| p.cmp(x))
    }

    /// Binary searches this sorted vector with a comparator function, analogous to
    /// [slice::binary_search_by].
    pub fn binary_search_by<'a, F>(&'a self, mut f: F) -> Result<usize, usize>
    where
        F: FnMut(&'a T::Target) -> Ordering,
    {
        let mut size = self.len();
        let mut left = 0;
        let mut right = size;
        while left < right {
            let mid = left + size / 2;

            // safety: like the core algorithm, mid is always within original vector len; in
            // pathlogical cases, user could push to the vector in the meantime, but this can only
            // increase the length, keeping this safe
            let cmp = f(unsafe { self.get_unchecked(mid) });

            if cmp == Ordering::Less {
                left = mid + 1;
            } else if cmp == Ordering::Greater {
                right = mid;
            } else {
                return Ok(mid);
            }

            size = right - left;
        }
        Err(left)
    }

    /// Binary searches this sorted vector with a key extraction function, analogous to
    /// [slice::binary_search_by_key].
    pub fn binary_search_by_key<'a, B, F>(&'a self, b: &B, mut f: F) -> Result<usize, usize>
    where
        F: FnMut(&'a T::Target) -> B,
        B: Ord,
    {
        self.binary_search_by(|k| f(k).cmp(b))
    }

    /// Returns the index of the partition point according to the given predicate
    /// (the index of the first element of the second partition), analogous to
    /// [slice::partition_point].
    pub fn partition_point<P>(&self, mut pred: P) -> usize
    where
        P: FnMut(&T::Target) -> bool,
    {
        let mut left = 0;
        let mut right = self.len();

        while left != right {
            let mid = left + (right - left) / 2;
            // safety: like in binary_search_by
            let value = unsafe { self.get_unchecked(mid) };
            if pred(value) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }

        left
    }

    // TODO add more
}

impl<T> std::convert::AsMut<Vec<T>> for FrozenVec<T> {
    /// Get mutable access to the underlying vector.
    ///
    /// This is safe, as it requires a `&mut self`, ensuring nothing is using
    /// the 'frozen' contents.
    fn as_mut(&mut self) -> &mut Vec<T> {
        unsafe { &mut *self.vec.get() }
    }
}

impl<T> Default for FrozenVec<T> {
    fn default() -> Self {
        FrozenVec::new()
    }
}

impl<T> From<Vec<T>> for FrozenVec<T> {
    fn from(vec: Vec<T>) -> Self {
        Self {
            vec: UnsafeCell::new(vec),
        }
    }
}

impl<T: StableDeref> Index<usize> for FrozenVec<T> {
    type Output = T::Target;
    fn index(&self, idx: usize) -> &T::Target {
        self.get(idx).unwrap_or_else(|| {
            panic!(
                "index out of bounds: the len is {} but the index is {}",
                self.len(),
                idx
            )
        })
    }
}

impl<A> FromIterator<A> for FrozenVec<A> {
    fn from_iter<T>(iter: T) -> Self
    where
        T: IntoIterator<Item = A>,
    {
        let vec: Vec<_> = iter.into_iter().collect();
        vec.into()
    }
}

/// Iterator over FrozenVec, obtained via `.iter()`
///
/// It is safe to push to the vector during iteration
pub struct Iter<'a, T> {
    vec: &'a FrozenVec<T>,
    idx: usize,
}

impl<'a, T: StableDeref> Iterator for Iter<'a, T> {
    type Item = &'a T::Target;
    fn next(&mut self) -> Option<&'a T::Target> {
        if let Some(ret) = self.vec.get(self.idx) {
            self.idx += 1;
            Some(ret)
        } else {
            None
        }
    }
}

impl<'a, T: StableDeref> IntoIterator for &'a FrozenVec<T> {
    type Item = &'a T::Target;
    type IntoIter = Iter<'a, T>;
    fn into_iter(self) -> Iter<'a, T> {
        Iter { vec: self, idx: 0 }
    }
}

#[test]
fn test_iteration() {
    let vec = vec!["a", "b", "c", "d"];
    let frozen: FrozenVec<_> = vec.clone().into();

    assert_eq!(vec, frozen.iter().collect::<Vec<_>>());
    for (e1, e2) in vec.iter().zip(frozen.iter()) {
        assert_eq!(*e1, e2);
    }

    assert_eq!(vec.len(), frozen.iter().count())
}

#[test]
fn test_accessors() {
    let vec: FrozenVec<String> = FrozenVec::new();

    assert_eq!(vec.is_empty(), true);
    assert_eq!(vec.len(), 0);
    assert_eq!(vec.first(), None);
    assert_eq!(vec.last(), None);
    assert_eq!(vec.get(1), None);

    vec.push("a".to_string());
    vec.push("b".to_string());
    vec.push("c".to_string());

    assert_eq!(vec.is_empty(), false);
    assert_eq!(vec.len(), 3);
    assert_eq!(vec.first(), Some("a"));
    assert_eq!(vec.last(), Some("c"));
    assert_eq!(vec.get(1), Some("b"));
}

#[test]
fn test_non_stable_deref() {
    #[derive(Copy, Clone, Debug, PartialEq, Eq)]
    struct Moo(i32);
    let vec: FrozenVec<Moo> = FrozenVec::new();

    assert_eq!(vec.is_empty(), true);
    assert_eq!(vec.len(), 0);
    assert_eq!(vec.get_copy(1), None);

    vec.push(Moo(1));
    vec.push(Moo(2));
    vec.push(Moo(3));

    assert_eq!(vec.is_empty(), false);
    assert_eq!(vec.len(), 3);
    assert_eq!(vec.get_copy(1), Some(Moo(2)));
}

#[test]
fn test_binary_search() {
    let vec: FrozenVec<_> = vec!["ab", "cde", "fghij"].into();

    assert_eq!(vec.binary_search("cde"), Ok(1));
    assert_eq!(vec.binary_search("cdf"), Err(2));
    assert_eq!(vec.binary_search("a"), Err(0));
    assert_eq!(vec.binary_search("g"), Err(3));

    assert_eq!(vec.binary_search_by_key(&1, |x| x.len()), Err(0));
    assert_eq!(vec.binary_search_by_key(&3, |x| x.len()), Ok(1));
    assert_eq!(vec.binary_search_by_key(&4, |x| x.len()), Err(2));

    assert_eq!(vec.partition_point(|x| x.len() < 4), 2);
    assert_eq!(vec.partition_point(|_| false), 0);
    assert_eq!(vec.partition_point(|_| true), 3);
}
