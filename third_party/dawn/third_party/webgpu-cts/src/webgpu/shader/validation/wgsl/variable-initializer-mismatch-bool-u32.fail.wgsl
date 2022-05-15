// v-0033: variable 'flag' store type is 'bool' however the initializer type is 'u32'.

var<private> flag : bool  = 1u;

@stage(compute) @workgroup_size(1)
fn main() {
}
