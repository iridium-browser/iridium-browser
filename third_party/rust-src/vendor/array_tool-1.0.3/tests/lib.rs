extern crate array_tool;

#[test]
fn it_implements_uniques() {
  assert_eq!(array_tool::uniques(vec![1,2,3,4,5,6],vec![1,2]), vec![vec![3,4,5,6],vec![]]);
  assert_eq!(array_tool::uniques(vec![1,2,3,4,5,6],vec![1,2,3,4]), vec![vec![5,6], vec![]]);
  assert_eq!(array_tool::uniques(vec![1,2,3],vec![1,2,3,4,5]), vec![vec![], vec![4,5]]);
  assert_eq!(array_tool::uniques(vec![1,2,9],vec![1,2,3,4,5]), vec![vec![9], vec![3,4,5]]);
}

