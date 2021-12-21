// v-0015: variable 's' store type is struct 'SArr' that has a runtime-sized member but its storage class is not 'storage'.

type RTArr = [[stride (16)]] array<vec4<f32>>;
[[block]]
struct SArr{
  data : RTArr;
};

[[stage(fragment)]]
fn main() {
  var s : SArr;
}
