// v-0031: in 'y = x', x is a runtime array and it's used as an expression.

type RTArr = array<i32>;
struct S {
  data : RTArr
};

@stage(fragment)
fn main() {
  var <storage> x : S;
  var y : array<i32,2>;
  y = x;
}
