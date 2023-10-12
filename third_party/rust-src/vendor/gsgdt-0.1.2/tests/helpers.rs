use gsgdt;
use serde_json;

use std::fs::File;
use std::io::prelude::*;

use gsgdt::*;

pub fn read_graph_from_file(file: &str) -> Graph{
    let mut file = File::open(file).unwrap();
    let mut contents = String::new();
    file.read_to_string(&mut contents).unwrap();
    serde_json::from_str(&contents).unwrap()
}
