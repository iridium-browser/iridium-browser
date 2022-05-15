// v-0031: struct 'S' has runtime-sized member but it's storage class is not 'storage'.

type RTArr = array<vec4<f32>>;
struct S {
  data : RTArr
};

@stage(fragment)
fn main() {
}
