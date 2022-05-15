// v-0004 - Self recursion is dis-allowed and `other` calls itself.

@stage(fragment)
fn other() -> i32 {
  return other();
}
