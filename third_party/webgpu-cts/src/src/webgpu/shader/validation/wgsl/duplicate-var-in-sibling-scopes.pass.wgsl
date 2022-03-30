// This passes because variable `a` is redeclared in a sibling scope.

@stage(fragment)
fn main() {
  if (true) {
    var a : i32 = -1;
  }
  if (true) {
    var a : i32 = -2;
  }
  return;
}

