// This fails because variable `a` is redeclared.

fn func(a: i32) -> i32 {
  var a : u32 = 1u;
  return 0;
}

@stage(fragment)
fn main() {
  return;
}
