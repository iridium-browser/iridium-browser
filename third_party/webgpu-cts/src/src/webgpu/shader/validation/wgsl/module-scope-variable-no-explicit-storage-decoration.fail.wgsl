// v-0037: 'f' is a module scope variable so it must be declared with an explicit storage class
// decoration.

var f : vec2<f32>;

@stage(fragment)
fn main() {
}
