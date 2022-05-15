// v-0015: variable 's' store type is struct 'SArr' that has a runtime-sized member but its storage class is not 'storage'.

type RTArr = array<vec4<f32>>;
struct SArr {
  data : RTArr
};

@stage(fragment)
fn main() {
  var s : SArr;
}
