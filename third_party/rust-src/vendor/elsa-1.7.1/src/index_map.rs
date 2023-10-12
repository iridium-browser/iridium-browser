use std::borrow::Borrow;
use std::cell::{Cell, UnsafeCell};
use std::collections::hash_map::RandomState;
use std::hash::{BuildHasher, Hash};
use std::iter::FromIterator;
use std::ops::Index;

use indexmap::IndexMap;
use stable_deref_trait::StableDeref;

/// Append-only version of `indexmap::IndexMap` where
/// insertion does not require mutable access
pub struct FrozenIndexMap<K, V, S = RandomState> {
    map: UnsafeCell<IndexMap<K, V, S>>,
    /// Eq/Hash implementations can have side-effects, and using Rc it is possible
    /// for FrozenIndexMap::insert to be called on a key that itself contains the same
    /// `FrozenIndexMap`, whose `eq` implementation also calls FrozenIndexMap::insert
    ///
    /// We use this `in_use` flag to guard against any reentrancy.
    in_use: Cell<bool>,
}

// safety: UnsafeCell implies !Sync

impl<K: Eq + Hash, V> FrozenIndexMap<K, V> {
    pub fn new() -> Self {
        Self {
            map: UnsafeCell::new(Default::default()),
            in_use: Cell::new(false),
        }
    }
}

impl<K: Eq + Hash, V: StableDeref, S: BuildHasher> FrozenIndexMap<K, V, S> {
    // these should never return &K or &V
    // these should never delete any entries
    pub fn insert(&self, k: K, v: V) -> &V::Target {
        assert!(!self.in_use.get());
        self.in_use.set(true);
        let ret = unsafe {
            let map = self.map.get();
            &*(*map).entry(k).or_insert(v)
        };
        self.in_use.set(false);
        ret
    }

    // these should never return &K or &V
    // these should never delete any entries
    pub fn insert_full(&self, k: K, v: V) -> (usize, &V::Target) {
        assert!(!self.in_use.get());
        self.in_use.set(true);
        let ret = unsafe {
            let map = self.map.get();
            let entry = (*map).entry(k);
            let index = entry.index();
            (index, &**entry.or_insert(v))
        };
        self.in_use.set(false);
        ret
    }

    /// Returns a reference to the value corresponding to the key.
    ///
    /// The key may be any borrowed form of the map's key type, but
    /// [`Hash`] and [`Eq`] on the borrowed form *must* match those for
    /// the key type.
    ///
    /// # Examples
    ///
    /// ```
    /// use elsa::FrozenIndexMap;
    ///
    /// let map = FrozenIndexMap::new();
    /// map.insert(1, Box::new("a"));
    /// assert_eq!(map.get(&1), Some(&"a"));
    /// assert_eq!(map.get(&2), None);
    /// ```
    pub fn get<Q: ?Sized>(&self, k: &Q) -> Option<&V::Target>
    where
        K: Borrow<Q>,
        Q: Hash + Eq,
    {
        assert!(!self.in_use.get());
        self.in_use.set(true);
        let ret = unsafe {
            let map = self.map.get();
            (*map).get(k).map(|x| &**x)
        };
        self.in_use.set(false);
        ret
    }

    pub fn get_index(&self, index: usize) -> Option<(&K::Target, &V::Target)>
    where
        K: StableDeref,
    {
        assert!(!self.in_use.get());
        self.in_use.set(true);
        let ret = unsafe {
            let map = self.map.get();
            (*map).get_index(index).map(|(k, v)| (&**k, &**v))
        };
        self.in_use.set(false);
        ret
    }

    /// Applies a function to the owner of the value corresponding to the key (if any).
    ///
    /// The key may be any borrowed form of the map's key type, but
    /// [`Hash`] and [`Eq`] on the borrowed form *must* match those for
    /// the key type.
    ///
    /// # Examples
    ///
    /// ```
    /// use elsa::FrozenIndexMap;
    ///
    /// let map = FrozenIndexMap::new();
    /// map.insert(1, Box::new("a"));
    /// assert_eq!(map.map_get(&1, Clone::clone), Some(Box::new("a")));
    /// assert_eq!(map.map_get(&2, Clone::clone), None);
    /// ```
    pub fn map_get<Q: ?Sized, T, F>(&self, k: &Q, f: F) -> Option<T>
    where
        K: Borrow<Q>,
        Q: Hash + Eq,
        F: FnOnce(&V) -> T,
    {
        assert!(!self.in_use.get());
        self.in_use.set(true);
        let ret = unsafe {
            let map = self.map.get();
            (*map).get(k).map(f)
        };
        self.in_use.set(false);
        ret
    }

    pub fn into_map(self) -> IndexMap<K, V, S> {
        self.map.into_inner()
    }

    /// Get mutable access to the underlying [`IndexMap`].
    ///
    /// This is safe, as it requires a `&mut self`, ensuring nothing is using
    /// the 'frozen' contents.
    pub fn as_mut(&mut self) -> &mut IndexMap<K, V, S> {
        unsafe { &mut *self.map.get() }
    }

    /// Returns true if the map contains no elements.
    pub fn is_empty(&self) -> bool {
        assert!(!self.in_use.get());
        self.in_use.set(true);
        let ret = unsafe {
            let map = self.map.get();
            (*map).is_empty()
        };
        self.in_use.set(false);
        ret
    }
}

impl<K, V, S> From<IndexMap<K, V, S>> for FrozenIndexMap<K, V, S> {
    fn from(map: IndexMap<K, V, S>) -> Self {
        Self {
            map: UnsafeCell::new(map),
            in_use: Cell::new(false),
        }
    }
}

impl<Q: ?Sized, K: Eq + Hash, V: StableDeref, S: BuildHasher> Index<&Q> for FrozenIndexMap<K, V, S>
    where
        Q: Eq + Hash,
        K: Eq + Hash + Borrow<Q>,
        V: StableDeref,
        S: BuildHasher
{
    type Output = V::Target;

    /// # Examples
    ///
    /// ```
    /// use elsa::FrozenIndexMap;
    ///
    /// let map = FrozenIndexMap::new();
    /// map.insert(1, Box::new("a"));
    /// assert_eq!(map[&1], "a");
    /// ```
    fn index(&self, idx: &Q) -> &V::Target {
        self.get(&idx)
            .expect("attempted to index FrozenIndexMap with unknown key")
    }
}

impl<K: Eq + Hash, V, S: BuildHasher + Default> FromIterator<(K, V)> for FrozenIndexMap<K, V, S> {
    fn from_iter<T>(iter: T) -> Self
    where
        T: IntoIterator<Item = (K, V)>,
    {
        let map: IndexMap<_, _, _> = iter.into_iter().collect();
        map.into()
    }
}

impl<K: Eq + Hash, V, S: Default> Default for FrozenIndexMap<K, V, S> {
    fn default() -> Self {
        Self {
            map: UnsafeCell::new(Default::default()),
            in_use: Cell::new(false),
        }
    }
}
