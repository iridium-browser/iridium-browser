// v-0033: variable 'a' store type is 'i32' however the initializer type is 'u32'.

var<private> a : i32  = 1u;

@stage(fragment)
fn main() {
}
