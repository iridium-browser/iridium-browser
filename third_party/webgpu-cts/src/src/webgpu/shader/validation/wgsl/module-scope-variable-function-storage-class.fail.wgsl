// v-0036: 'f' is a module scope variable however its storage class is 'function'.

var<function> f : vec2<f32>;

@stage(fragment)
fn main() {
}
