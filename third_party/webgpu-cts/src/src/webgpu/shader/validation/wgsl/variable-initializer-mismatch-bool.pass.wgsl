// pass v-0033: variable 'a' and its initilizer have the same store type, 'bool'.

var<private> flag : bool  = true;

@stage(compute) @workgroup_size(1)
fn main() {
}
