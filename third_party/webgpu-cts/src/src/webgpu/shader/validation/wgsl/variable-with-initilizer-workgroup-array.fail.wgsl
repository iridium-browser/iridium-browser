// v-0032: variable 'w' has an initializer, however its storage class is 'workgroup'.

var<workgroup> w : array<i32, 4> = array<i32, 4>(0, 1, 0, 1);

@stage(fragment)
fn main() {
}

