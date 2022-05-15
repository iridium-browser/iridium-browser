// v-0032: variable 'u' has an initializer, however its storage class is 'uniform'.

struct Params {
  count: i32
};

@group(0) @binding(0)
var<uniform> u : Params = Params(1);

@stage(fragment)
fn main() {
}
