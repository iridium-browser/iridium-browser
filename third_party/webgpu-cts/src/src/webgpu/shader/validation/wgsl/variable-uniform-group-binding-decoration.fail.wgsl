# v-0040: variable 'u' is in uniform storage class so it must be declared with group and binding
# decoration.

struct Params {
  count: i32
};

var<uniform> u : Params;

@stage(fragment)
fn main() {
}
