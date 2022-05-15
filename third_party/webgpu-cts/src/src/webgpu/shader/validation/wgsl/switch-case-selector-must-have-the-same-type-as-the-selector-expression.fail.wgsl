// v-0026: line 7: the case selector values must have the same type as the selector expression

@stage(fragment)
fn main() {
  var a: u32 = 2;
  switch (a) {
    case -1:{}
    default: {}
  }
  return;
}

