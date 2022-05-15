// v-0006 - Fails because `a` calls `c` but `c` has not been defined.

fn a() -> i32 {
  return c();
}

fn b() -> i32 {
  return a();
}

fn c() -> i32 {
  return a();
}

@stage(fragment)
fn main() {
  return;
}
