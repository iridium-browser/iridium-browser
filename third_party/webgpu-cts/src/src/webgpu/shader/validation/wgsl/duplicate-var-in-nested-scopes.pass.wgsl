// This passes because inter-scope shadowing is supported

@stage(fragment)
fn main() {
  var a : u32 = 1u;
  if (true) {
    var a : i32 = -1;
  }
  return;
}

