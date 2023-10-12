extern crate array_tool;

#[test]
fn it_implements_individual_uniq_on_vec() {
  use array_tool::vec::Uniq;
  assert_eq!(vec![1,2,3,4,5,6].uniq(vec![1,2,5,7,9]),vec![3,4,6]);
  assert_eq!(vec![1,2,3,1,3,2,1,3,1,2,3,1,2,3,3,1,2,3,3,1,2,3,1,2,3,3,4,1,5,4,6].uniq(vec![3,5]),
             vec![1,2,4,6]
            );
}

#[test]
fn it_can_return_its_own_unique() {
  use array_tool::vec::Uniq;
  assert_eq!(vec![1,2,1,3,4,3,4,5,6].unique(),vec![1,2,3,4,5,6]);
  assert_eq!(vec![1,2,3,1,3,2,1,3,1,2,3,1,2,3,3,1,2,3,3,1,2,3,1,2,3,3,4,1,5,4,6].unique(),
             vec![1,2,3,4,5,6]
            );
}

#[test]
fn it_answers_about_uniqueness() {
  use array_tool::vec::Uniq;
  assert_eq!(vec![1,2,1,3,4,3,4,5,6].is_unique(), false);
  assert_eq!(vec![1,2,3,4,5,6].is_unique(), true);
}

#[test]
fn it_implements_individual_uniq_on_vec_via() {
  use array_tool::vec::Uniq;
  assert_eq!(vec![1.1,2.6,3.7,4.7,5.4,6.6].uniq_via(vec![1.5,2.7,5.0,7.1,9.4], |l: &f64, r: &f64| l.floor() == r.floor()),vec![3.7,4.7,6.6]);
  assert_eq!(vec![1.2,2.5,3.4,1.2,3.8,2.9,1.0,3.2,1.2,2.5,3.7,1.7,2.9,3.1,3.5,1.6,2.7,3.9,3.1,1.5,2.6,3.8,1.2,2.6,3.7,3.8,4.9,1.0,5.1,4.4,6.6]
                  .uniq_via(vec![3.5,5.1], |l: &f64, r: &f64| l.floor() == r.floor()),
             vec![1.2,2.5,2.9,1.0,1.7,1.6,2.7,1.5,2.6,4.9,4.4, 6.6]
            );
}

#[test]
fn it_can_return_its_own_unique_via() {
  use array_tool::vec::Uniq;
  assert_eq!(vec![1.2,2.5,1.4,3.2,4.8,3.9,4.0,5.2,6.2].unique_via(|l: &f64, r: &f64| l.floor() == r.floor()),vec![1.2,2.5,3.2,4.8,5.2,6.2]);
  assert_eq!(vec![1.2,2.5,3.4,1.2,3.8,2.9,1.0,3.2,1.2,2.5,3.7,1.7,2.9,3.1,3.5,1.6,2.7,3.9,3.1,1.5,2.6,3.8,1.2,2.6,3.7,3.8,4.9,1.0,5.1,4.4,6.6]
                  .unique_via(|l: &f64, r: &f64| l.floor() == r.floor()),
             vec![1.2,2.5,3.4,4.9,5.1,6.6]
            );
}

#[test]
fn it_answers_about_uniqueness_via() {
  use array_tool::vec::Uniq;
  assert_eq!(vec![1.2,2.4,1.5,3.6,4.1,3.5,4.7,5.9,6.5].is_unique_via(|l: &f64, r: &f64| l.floor() == r.floor()), false);
  assert_eq!(vec![1.2,2.4,3.5,4.6,5.1,6.5].is_unique_via(|l: &f64, r: &f64| l.floor() == r.floor()), true);
}

#[test]
fn it_shifts() {
  use array_tool::vec::Shift;
  let mut x = vec![1,2,3];
  x.unshift(0);
  assert_eq!(x, vec![0,1,2,3]);
  assert_eq!(x.shift(), Some(0));
  assert_eq!(x, vec![1,2,3]);
}

#[test]
fn it_handles_empty_shift() {
  use array_tool::vec::Shift;
  let mut x: Vec<u8> = vec![];
  assert_eq!(x.shift(), None);
}

#[test]
fn it_intersects() {
  use array_tool::vec::Intersect;
  assert_eq!(vec![1,1,3,5].intersect(vec![1,2,3]), vec![1,3])
}

#[allow(unused_imports)]
#[test]
fn it_intersects_if() {
  use array_tool::vec::Intersect;
  use std::ascii::AsciiExt;
  assert_eq!(vec!['a','a','c','e'].intersect_if(vec!['A','B','C'], |l, r| l.eq_ignore_ascii_case(r)), vec!['a','c']);
}

#[test]
fn it_multiplies(){
  use array_tool::vec::Times;
  assert_eq!(vec![1,2,3].times(3), vec![1,2,3,1,2,3,1,2,3]);

  // Empty collection
  let vec1: Vec<i32> = Vec::new();
  let vec2: Vec<i32> = Vec::new();
  assert_eq!(vec1.times(4), vec2)
}

#[test]
fn it_joins(){
  use array_tool::vec::Join;
  assert_eq!(vec![1,2,3].join(","), "1,2,3")
}

#[test]
fn it_creates_union() {
  use array_tool::vec::Union;
  assert_eq!(vec!["a","b","c"].union(vec!["c","d","a"]), vec![ "a", "b", "c", "d" ]);
  assert_eq!(vec![1,2,3,1,3,2,1,3,1,2,3,1,2,3,3,1,2,3,3,1,2,3,1,2,3,3,4,1,4,6].union(vec![3,5,7,8,0]),
             vec![1,2,3,4,6,5,7,8,0]
            );
}

