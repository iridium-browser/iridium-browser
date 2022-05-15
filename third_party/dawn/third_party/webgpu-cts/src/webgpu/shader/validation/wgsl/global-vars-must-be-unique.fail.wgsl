// v-0011 - This fails because of the duplicated `a` variable.

let a : vec2<f32> = vec2<f32>(0.1, 1.0);
var<private> a : vec4<f32>;

@stage(compute) @workgroup_size(1)
fn main() {
  return;
}
