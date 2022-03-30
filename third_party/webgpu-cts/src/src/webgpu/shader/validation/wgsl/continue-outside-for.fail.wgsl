// v-0010 - This fails because the continue is used outside of a for block.

@stage(fragment)
fn main() {
  continue;
  return;
}
