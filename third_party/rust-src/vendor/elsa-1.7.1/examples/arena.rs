use elsa::FrozenVec;

fn main() {
    let arena = Arena::new();
    let lonely = arena.add_thing("lonely", vec![]);
    let best_friend = arena.add_thing("best friend", vec![lonely]);
    let threes_a_crowd = arena.add_thing("threes a crowd", vec![lonely, best_friend]);
    let rando = arena.add_thing("rando", vec![]);
    let _facebook = arena.add_thing("facebook", vec![rando, threes_a_crowd, lonely, best_friend]);

    assert!(cmp_ref(lonely, best_friend.friends[0]));
    assert!(cmp_ref(best_friend, threes_a_crowd.friends[1]));
    arena.dump();
}

struct Arena<'arena> {
    things: FrozenVec<Box<Thing<'arena>>>,
}

struct Thing<'arena> {
    pub friends: Vec<ThingRef<'arena>>,
    pub name: &'static str,
}

type ThingRef<'arena> = &'arena Thing<'arena>;

impl<'arena> Arena<'arena> {
    fn new() -> Arena<'arena> {
        Arena {
            things: FrozenVec::new(),
        }
    }

    fn add_thing(
        &'arena self,
        name: &'static str,
        friends: Vec<ThingRef<'arena>>,
    ) -> ThingRef<'arena> {
        let idx = self.things.len();
        self.things.push(Box::new(Thing { name, friends }));
        &self.things[idx]
    }

    fn dump(&'arena self) {
        for thing in &self.things {
            println!("friends of {}:", thing.name);
            for friend in &thing.friends {
                println!("\t{}", friend.name);
            }
        }
    }
}

fn cmp_ref<T>(x: &T, y: &T) -> bool {
    x as *const T as usize == y as *const T as usize
}
