use elsa::FrozenVec;

fn main() {
    let arena = Arena::new();
    let lonely = arena.add_person("lonely", vec![]);
    let best_friend = arena.add_person("best friend", vec![lonely]);
    let threes_a_crowd = arena.add_person("threes a crowd", vec![lonely, best_friend]);
    let rando = arena.add_person("rando", vec![]);
    let _everyone = arena.add_person(
        "follows everyone",
        vec![rando, threes_a_crowd, lonely, best_friend],
    );
    arena.dump();
}

struct Arena<'arena> {
    people: FrozenVec<Box<Person<'arena>>>,
}

struct Person<'arena> {
    pub follows: FrozenVec<PersonRef<'arena>>,
    pub reverse_follows: FrozenVec<PersonRef<'arena>>,
    pub name: &'static str,
}

type PersonRef<'arena> = &'arena Person<'arena>;

impl<'arena> Arena<'arena> {
    fn new() -> Arena<'arena> {
        Arena {
            people: FrozenVec::new(),
        }
    }

    fn add_person(
        &'arena self,
        name: &'static str,
        follows: Vec<PersonRef<'arena>>,
    ) -> PersonRef<'arena> {
        let idx = self.people.len();
        self.people.push(Box::new(Person {
            name,
            follows: follows.into(),
            reverse_follows: Default::default(),
        }));
        let me = &self.people[idx];
        for friend in &me.follows {
            friend.reverse_follows.push(me)
        }
        me
    }

    fn dump(&'arena self) {
        for thing in &self.people {
            println!("{} network:", thing.name);
            println!("\tfollowing:");
            for friend in &thing.follows {
                println!("\t\t{}", friend.name);
            }
            println!("\tfollowers:");
            for friend in &thing.reverse_follows {
                println!("\t\t{}", friend.name);
            }
        }
    }
}

// Note that the following will cause the above code to stop compiling
// since non-eyepatched custom destructors can potentially
// read deallocated data.
//
// impl<'arena> Drop for Person<'arena> {
//     fn drop(&mut self) {
//         println!("goodbye {:?}", self.name);
//         for friend in &self.follows {
//             println!("\t\t{}", friend.name);
//         }
//     }
// }
