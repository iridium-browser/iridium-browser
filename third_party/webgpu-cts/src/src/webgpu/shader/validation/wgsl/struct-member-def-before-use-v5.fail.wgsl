// v-0007 - Fails because struct `foo` does not have a member `b`however `f.b` is
// used.

struct goo {
  b : f32
};

struct foo {
  a : f32
};

@stage(fragment)
fn main() {
  var f : foo;
  f.a = 2.0;
  f.b = 5.0;
  return;
}
