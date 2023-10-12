use std::borrow::Borrow;
use std::cell::{Cell, UnsafeCell};
use std::collections::hash_map::RandomState;
use std::collections::BTreeMap;
use std::collections::HashMap;
use std::hash::{BuildHasher, Hash};
use std::iter::FromIterator;
use std::ops::Index;

use stable_deref_trait::StableDeref;

/// Append-only version of `std::collections::HashMap` where
/// insertion does not require mutable access
pub struct FrozenMap<K, V, S = RandomState> {
    map: UnsafeCell<HashMap<K, V, S>>,
    /// Eq/Hash implementations can have side-effects, and using Rc it is possible
    /// for FrozenMap::insert to be called on a key that itself contains the same
    /// `FrozenMap`, whose `eq` implementation also calls FrozenMap::insert
    ///
    /// We use this `in_use` flag to guard against any reentrancy.
    in_use: Cell<bool>,
}

// safety: UnsafeCell implies !Sync

impl<K: Eq + Hash, V> FrozenMap<K, V> {
    pub fn new() -> Self {
        Self {
            map: UnsafeCell::new(Default::default()),
            in_use: Cell::new(false),
        }
    }

    /// # Examples
    ///
    /// ```
    /// use elsa::FrozenMap;
    ///
    /// let map = FrozenMap::new();
    /// assert_eq!(map.len(), 0);
    /// map.insert(1, Box::new("a"));
    /// assert_eq!(map.len(), 1);
    /// ```
    pub fn len(&self) -> usize {
        assert!(!self.in_use.get());
        self.in_use.set(true);
        let len = unsafe {
            let map = self.map.get();
            (*map).len()
        };
        self.in_use.set(false);
        len
    }

    /// # Examples
    ///
    /// ```
    /// use elsa::FrozenMap;
    ///
    /// let map = FrozenMap::new();
    /// assert_eq!(map.is_empty(), true);
    /// map.insert(1, Box::new("a"));
    /// assert_eq!(map.is_empty(), false);
    /// ```
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

impl<K: Eq + Hash, V: StableDeref, S: BuildHasher> FrozenMap<K, V, S> {
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

    /// Returns a reference to the value corresponding to the key.
    ///
    /// The key may be any borrowed form of the map's key type, but
    /// [`Hash`] and [`Eq`] on the borrowed form *must* match those for
    /// the key type.
    ///
    /// # Examples
    ///
    /// ```
    /// use elsa::FrozenMap;
    ///
    /// let map = FrozenMap::new();
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

    /// Applies a function to the owner of the value corresponding to the key (if any).
    ///
    /// The key may be any borrowed form of the map's key type, but
    /// [`Hash`] and [`Eq`] on the borrowed form *must* match those for
    /// the key type.
    ///
    /// # Examples
    ///
    /// ```
    /// use elsa::FrozenMap;
    ///
    /// let map = FrozenMap::new();
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

    pub fn into_map(self) -> HashMap<K, V, S> {
        self.map.into_inner()
    }

    // TODO add more
}

impl<K: Eq + Hash + StableDeref, V: StableDeref, S: BuildHasher> FrozenMap<K, V, S> {
    /// Returns a reference to the key and value matching a borrowed
    /// key.
    ///
    /// The key argument may be any borrowed form of the map's key type,
    /// but [`Hash`] and [`Eq`] on the borrowed form *must* match those
    /// for the key type.
    ///
    /// # Examples
    ///
    /// ```
    /// use elsa::FrozenMap;
    ///
    /// let map = FrozenMap::new();
    /// map.insert(Box::new("1"), Box::new("a"));
    /// assert_eq!(map.get_key_value(&"1"), Some((&"1", &"a")));
    /// assert_eq!(map.get_key_value(&"2"), None);
    /// ```
    pub fn get_key_value<Q: ?Sized>(&self, k: &Q) -> Option<(&K::Target, &V::Target)>
    where
        K: Borrow<Q>,
        Q: Hash + Eq,
    {
        assert!(!self.in_use.get());
        self.in_use.set(true);
        let ret = unsafe {
            let map = self.map.get();
            (*map).get_key_value(k).map(|(k, v)| (&**k, &**v))
        };
        self.in_use.set(false);
        ret
    }
}

impl<K, V, S> std::convert::AsMut<HashMap<K, V, S>> for FrozenMap<K, V, S> {
    /// Get mutable access to the underlying [`HashMap`].
    ///
    /// This is safe, as it requires a `&mut self`, ensuring nothing is using
    /// the 'frozen' contents.
    fn as_mut(&mut self) -> &mut HashMap<K, V, S> {
        unsafe { &mut *self.map.get() }
    }
}

impl<K, V, S> From<HashMap<K, V, S>> for FrozenMap<K, V, S> {
    fn from(map: HashMap<K, V, S>) -> Self {
        Self {
            map: UnsafeCell::new(map),
            in_use: Cell::new(false),
        }
    }
}

impl<Q: ?Sized, K, V, S> Index<&Q> for FrozenMap<K, V, S>
where
    Q: Eq + Hash,
    K: Eq + Hash + Borrow<Q>,
    V: StableDeref,
    S: BuildHasher,
{
    type Output = V::Target;

    /// # Examples
    ///
    /// ```
    /// use elsa::FrozenMap;
    ///
    /// let map = FrozenMap::new();
    /// map.insert(1, Box::new("a"));
    /// assert_eq!(map[&1], "a");
    /// ```
    fn index(&self, idx: &Q) -> &V::Target {
        self.get(idx)
            .expect("attempted to index FrozenMap with unknown key")
    }
}

impl<K: Eq + Hash, V, S: BuildHasher + Default> FromIterator<(K, V)> for FrozenMap<K, V, S> {
    fn from_iter<T>(iter: T) -> Self
    where
        T: IntoIterator<Item = (K, V)>,
    {
        let map: HashMap<_, _, _> = iter.into_iter().collect();
        map.into()
    }
}

impl<K: Eq + Hash, V, S: Default> Default for FrozenMap<K, V, S> {
    fn default() -> Self {
        Self {
            map: UnsafeCell::new(Default::default()),
            in_use: Cell::new(false),
        }
    }
}

/// Append-only version of `std::collections::BTreeMap` where
/// insertion does not require mutable access
pub struct FrozenBTreeMap<K, V> {
    map: UnsafeCell<BTreeMap<K, V>>,
    /// Eq/Hash implementations can have side-effects, and using Rc it is possible
    /// for FrozenBTreeMap::insert to be called on a key that itself contains the same
    /// `FrozenBTreeMap`, whose `eq` implementation also calls FrozenBTreeMap::insert
    ///
    /// We use this `in_use` flag to guard against any reentrancy.
    in_use: Cell<bool>,
}

// safety: UnsafeCell implies !Sync

impl<K: Clone + Ord, V: StableDeref> FrozenBTreeMap<K, V> {
    pub fn new() -> Self {
        Self {
            map: UnsafeCell::new(Default::default()),
            in_use: Cell::new(false),
        }
    }

    /// # Examples
    ///
    /// ```
    /// use elsa::FrozenBTreeMap;
    ///
    /// let map = FrozenBTreeMap::new();
    /// assert_eq!(map.len(), 0);
    /// map.insert(1, Box::new("a"));
    /// assert_eq!(map.len(), 1);
    /// ```
    pub fn len(&self) -> usize {
        assert!(!self.in_use.get());
        self.in_use.set(true);
        let len = unsafe {
            let map = self.map.get();
            (*map).len()
        };
        self.in_use.set(false);
        len
    }

    /// # Examples
    ///
    /// ```
    /// use elsa::FrozenBTreeMap;
    ///
    /// let map = FrozenBTreeMap::new();
    /// assert_eq!(map.is_empty(), true);
    /// map.insert(1, Box::new("a"));
    /// assert_eq!(map.is_empty(), false);
    /// ```
    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

impl<K: Clone + Ord, V: StableDeref> FrozenBTreeMap<K, V> {
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

    /// Returns a reference to the value corresponding to the key.
    ///
    /// The key may be any borrowed form of the map's key type, but
    /// [`Hash`] and [`Eq`] on the borrowed form *must* match those for
    /// the key type.
    ///
    /// # Examples
    ///
    /// ```
    /// use elsa::FrozenBTreeMap;
    ///
    /// let map = FrozenBTreeMap::new();
    /// map.insert(1, Box::new("a"));
    /// assert_eq!(map.get(&1), Some(&"a"));
    /// assert_eq!(map.get(&2), None);
    /// ```
    pub fn get<Q: ?Sized>(&self, k: &Q) -> Option<&V::Target>
    where
        K: Borrow<Q>,
        Q: Ord,
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

    /// Applies a function to the owner of the value corresponding to the key (if any).
    ///
    /// The key may be any borrowed form of the map's key type, but
    /// [`Hash`] and [`Eq`] on the borrowed form *must* match those for
    /// the key type.
    ///
    /// # Examples
    ///
    /// ```
    /// use elsa::FrozenBTreeMap;
    ///
    /// let map = FrozenBTreeMap::new();
    /// map.insert(1, Box::new("a"));
    /// assert_eq!(map.map_get(&1, Clone::clone), Some(Box::new("a")));
    /// assert_eq!(map.map_get(&2, Clone::clone), None);
    /// ```
    pub fn map_get<Q: ?Sized, T, F>(&self, k: &Q, f: F) -> Option<T>
    where
        K: Borrow<Q>,
        Q: Ord,
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

    pub fn into_map(self) -> BTreeMap<K, V> {
        self.map.into_inner()
    }

    // TODO add more
}

impl<K, V> std::convert::AsMut<BTreeMap<K, V>> for FrozenBTreeMap<K, V> {
    /// Get mutable access to the underlying [`HashMap`].
    ///
    /// This is safe, as it requires a `&mut self`, ensuring nothing is using
    /// the 'frozen' contents.
    fn as_mut(&mut self) -> &mut BTreeMap<K, V> {
        unsafe { &mut *self.map.get() }
    }
}

impl<K: Clone + Ord, V: StableDeref> From<BTreeMap<K, V>> for FrozenBTreeMap<K, V> {
    fn from(map: BTreeMap<K, V>) -> Self {
        Self {
            map: UnsafeCell::new(map),
            in_use: Cell::new(false),
        }
    }
}

impl<Q: ?Sized, K, V> Index<&Q> for FrozenBTreeMap<K, V>
where
    Q: Ord,
    K: Clone + Ord + Borrow<Q>,
    V: StableDeref,
{
    type Output = V::Target;

    /// # Examples
    ///
    /// ```
    /// use elsa::FrozenBTreeMap;
    ///
    /// let map = FrozenBTreeMap::new();
    /// map.insert(1, Box::new("a"));
    /// assert_eq!(map[&1], "a");
    /// ```
    fn index(&self, idx: &Q) -> &V::Target {
        self.get(idx)
            .expect("attempted to index FrozenBTreeMap with unknown key")
    }
}

impl<K: Clone + Ord, V: StableDeref> FromIterator<(K, V)> for FrozenBTreeMap<K, V> {
    fn from_iter<T>(iter: T) -> Self
    where
        T: IntoIterator<Item = (K, V)>,
    {
        let map: BTreeMap<_, _> = iter.into_iter().collect();
        map.into()
    }
}

impl<K: Clone + Ord, V: StableDeref> Default for FrozenBTreeMap<K, V> {
    fn default() -> Self {
        Self {
            map: UnsafeCell::new(Default::default()),
            in_use: Cell::new(false),
        }
    }
}
