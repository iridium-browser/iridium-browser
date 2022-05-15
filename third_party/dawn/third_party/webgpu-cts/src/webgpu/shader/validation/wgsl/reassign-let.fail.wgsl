// v-0021 - Fails because `c` is a const and can not be re-assigned.

@stage(fragment)
fn main() {
  let c : f32 = 0.1;
  c = 0.2;
  return;
}
