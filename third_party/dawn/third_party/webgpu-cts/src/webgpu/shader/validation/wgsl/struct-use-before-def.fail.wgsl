// v-0007 - This fails because the structure is used before definition.

let a : Foo;

struct Foo {
  a : i32
};

@stage(fragment)
fn main() {
  return;
}
