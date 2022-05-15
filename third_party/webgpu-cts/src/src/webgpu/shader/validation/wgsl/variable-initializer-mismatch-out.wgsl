// v-0033: variable 'a' store type is 'i32' however the initializer type is 'f32'.

var<private> a : i32  = 1.0;

@stage(fragment)
fn main() {
}
