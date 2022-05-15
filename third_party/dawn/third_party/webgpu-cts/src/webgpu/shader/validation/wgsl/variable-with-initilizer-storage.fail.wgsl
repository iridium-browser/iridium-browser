// v-0032: variable 'u' has an initializer, however its storage class is 'storage'.

struct PositionBuffer {
  pos: vec2<f32>
};

@group(0) @binding(0)
var<storage> s : PositionBuffer = PositionBuffer(vec2<f32>(0.0, 0.0));

@stage(fragment)
fn main() {
}
