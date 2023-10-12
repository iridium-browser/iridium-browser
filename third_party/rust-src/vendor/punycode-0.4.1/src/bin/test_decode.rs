extern crate punycode;

use std::io;
use std::io::Write;

fn main() {
    let mut input = String::new();
    io::stdin().read_line(&mut input).unwrap();

    match punycode::decode(&input) {
        Ok(s) => { println!("{}", s); }
        Err(..) => { writeln!(&mut std::io::stderr(), "Error").unwrap(); }
    }
}
