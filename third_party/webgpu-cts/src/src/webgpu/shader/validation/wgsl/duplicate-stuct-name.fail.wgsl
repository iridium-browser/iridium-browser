// v-0012 - This fails because of the duplicated `foo` structure.

struct foo {
  a : i32
};

struct foo {
  b : f32
};

@stage(fragment)
fn main() {
  return;
}

