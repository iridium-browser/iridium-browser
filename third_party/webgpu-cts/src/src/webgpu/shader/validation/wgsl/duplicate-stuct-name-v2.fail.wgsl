// v-0012 - This fails because |struct Foo|, |fn Foo| and |var Foo| have
// the same name |Foo|.

struct Foo {
  b : f32
};

fn Foo() {
  return;
}

@stage(fragment)
fn main() {
  var Foo : f32;
  var f : Foo;
  return;
}
