// v-0007 -  This fails because `fn Foo()` returns `struct goo`, which does not
// have a member `s.z`.

struct goo {
  s : vec2<i32>
};

fn Foo() -> goo {
  var a : goo;
  a.s.x = 2;
  a.s.y = 3;
  return a;
}

@stage(fragment)
fn main() {
  var r : i32 = Foo().s.z;
  return;
}
