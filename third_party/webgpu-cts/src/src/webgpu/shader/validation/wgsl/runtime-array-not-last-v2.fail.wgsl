// v-0015 - This fails because of the aliased runtime array is not last member of the struct.

type RTArr = array<vec4<f32>>;
struct S {
  data : RTArr,
  b : f32,
};

@stage(fragment)
fn main() {
}
