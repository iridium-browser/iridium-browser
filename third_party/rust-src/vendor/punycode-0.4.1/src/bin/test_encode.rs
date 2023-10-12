extern crate punycode;

use std::io;

fn main() {
    let mut input = String::new();
    io::stdin().read_line(&mut input).unwrap();

    let s = punycode::encode(&input[0..input.len()-1]);
    println!("{}", s.unwrap());
}
