// v-0009 - This fails because the break is used outside a for or switch block.

@stage(fragment)
fn main() {
  if (true) {
    break;
  }
  return;
}

