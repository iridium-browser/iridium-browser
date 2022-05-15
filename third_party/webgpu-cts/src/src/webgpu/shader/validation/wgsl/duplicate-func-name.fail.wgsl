// v-0016 - This fails because of the duplicate function `my_func`.

fn my_func() {
  return;
}

fn my_func() {
  return;
}

@stage(fragment)
fn main() {
  return;
}

