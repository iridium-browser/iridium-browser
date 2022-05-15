// v-0002 - This program fails due to the missing return at the end of the
// function.

fn func() -> i32 {
  var a : i32 = 0;
}

@stage(fragment)
fn main() {
  func();
}
