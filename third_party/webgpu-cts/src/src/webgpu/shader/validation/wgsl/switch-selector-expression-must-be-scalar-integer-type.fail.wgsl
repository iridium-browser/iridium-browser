// v-0025: line 6: switch statement selector expression must be of a scalar integer type

@stage(fragment)
fn main() {
  var a: f32 = 3.14;
  switch (a) {
    default: {}
  }
  return;
}

