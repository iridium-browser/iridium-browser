// v-0011 - This fails because of the duplicate name `a`.

let a : vec2<f32> = vec2<f32>(0.1, 1.0);

fn a() {
  return;
}

@stage(fragment)
fn main() {
  return;
}
