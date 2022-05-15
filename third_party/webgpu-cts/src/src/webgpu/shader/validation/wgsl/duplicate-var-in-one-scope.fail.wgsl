// This fails because variable `a` is redeclared.

@stage(fragment)
fn main() {
  var a : u32 = 1u;
  var a : u32 = 2u;
  return;
}
