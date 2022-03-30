// v-0008: line 6: switch statement must have exactly one default clause

@stage(fragment)
fn main() {
  var a: i32 = 2;
  switch (a) {
    case 2: {}
  }
  return;
}

