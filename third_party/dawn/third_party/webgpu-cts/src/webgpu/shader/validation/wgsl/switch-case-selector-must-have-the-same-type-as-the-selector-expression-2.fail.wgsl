// v-0026: line 7: the case selector values must have the same type as the selector expression

@stage(fragment)
fn main() {
  var a: i32 = -2;
  switch (a) {
    case 2u:{}
    default: {}
  }
  return;
}

