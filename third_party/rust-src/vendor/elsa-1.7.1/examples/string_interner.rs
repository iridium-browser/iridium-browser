use std::collections::BTreeSet;
use std::convert::AsRef;

use elsa::FrozenIndexSet;

struct StringInterner {
    set: FrozenIndexSet<String>,
}

impl StringInterner {
    fn new() -> Self {
        StringInterner {
            set: FrozenIndexSet::new(),
        }
    }

    fn get_or_intern<T>(&self, value: T) -> usize
    where
        T: AsRef<str>,
    {
        // TODO use Entry in case the standard Entry API gets improved
        // (here to avoid premature allocation or double lookup)
        self.set.insert_full(value.as_ref().to_string()).0
    }

    fn get<T>(&self, value: T) -> Option<usize>
    where
        T: AsRef<str>,
    {
        self.set.get_full(value.as_ref()).map(|(i, _r)| i)
    }

    fn resolve(&self, index: usize) -> Option<&str> {
        self.set.get_index(index)
    }
}

fn main() {
    let interner = StringInterner::new();
    let lonely = interner.get_or_intern("lonely");
    let best_friend = interner.get_or_intern("best friend");
    let threes_a_crowd = interner.get_or_intern("threes a crowd");
    let rando = interner.get_or_intern("rando");
    let _facebook = interner.get_or_intern("facebook");

    let best_friend_2 = interner.get_or_intern("best friend");
    let best_friend_3 = interner.get("best friend").unwrap();

    let best_friend_ref = interner.resolve(best_friend).unwrap();

    let mut set = BTreeSet::new();
    set.insert(lonely);
    set.insert(best_friend);
    set.insert(threes_a_crowd);
    set.insert(rando);
    set.insert(best_friend_2);
    assert_eq!(set.len(), 4);
    assert_eq!(best_friend, best_friend_2);
    assert_eq!(best_friend_2, best_friend_3);
    assert_eq!(best_friend_ref, "best friend");
}
