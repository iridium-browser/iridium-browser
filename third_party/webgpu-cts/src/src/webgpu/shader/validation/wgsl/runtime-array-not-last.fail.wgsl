// v-0015 - This fails because of the runtime array is not last member of the struct.

struct Foo {
  a : @stride(16) array<f32>;
  b : f32;
};

@stage(fragment)
fn main() {
  return;
}
