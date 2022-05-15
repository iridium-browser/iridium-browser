// v-0033: variable 'a' store type is 'f32', however its initializer type is 'i32'.

@stage(fragment)
fn main() {
  var<function> a : f32  = 1;
}
