// This fails because variable `i` is redeclared

@stage(fragment)
fn main() {
  for (var i: i32 = 0; i < 10; i = i + 1) {
    var i: i32 = 1;
  }
  return;
}
