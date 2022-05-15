# v-0039: variable 's' is in storage storage class so it must be declared with group and binding
# decoration.

struct PositionBuffer {
  pos: vec2<f32>
};

var<storage> s : PositionBuffer;

@stage(fragment)
fn main() {
}
