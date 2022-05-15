// v-0032: variable 'w' has an initializer, however its storage class is 'workgroup'.

var<workgroup> w : vec3<i32> = vec3<i32>(0, 1, 0);

@stage(fragment)
fn main() {
}

