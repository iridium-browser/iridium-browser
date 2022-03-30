// v-0011 - This fails because of the duplicated `a` variable.

let a : vec2<f32> = vec2<f32>(0.1, 1.0);

@stage(fragment)
fn main() {
  var a: u32;
  return;
}
