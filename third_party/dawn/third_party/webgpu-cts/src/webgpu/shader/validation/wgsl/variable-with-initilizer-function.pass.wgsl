// pass v-0032: variable 'a' has an initializer and its storage class is 'function'.

@stage(fragment)
fn main() {
  var<function> a : i32  = 1;
}
