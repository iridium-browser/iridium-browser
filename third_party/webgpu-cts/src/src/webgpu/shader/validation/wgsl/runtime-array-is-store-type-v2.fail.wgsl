// v-0030 - This fails because 'RTArr' is a runtime array alias and it is used as store type.

type RTArr = array<f32>;
@group(0) @binding(1) var<storage> x : RTArr;

@stage(fragment)
fn main() {
  return;
}
