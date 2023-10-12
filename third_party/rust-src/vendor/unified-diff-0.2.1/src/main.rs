// Sample program. Do not use.
use std::env;
use std::fs;
use std::io::{self, Write};
use std::process;
fn main() {
    let mut o = env::args_os();
    // parse CLI
    let exe = match o.next() {
        Some(from) => from,
        None => {
            eprintln!("Usage: [exe] [from] [to]");
            process::exit(1);
        }
    };
    let from = match o.next() {
        Some(from) => from,
        None => {
            eprintln!("Usage: {} [from] [to]", exe.to_string_lossy());
            process::exit(1);
        }
    };
    let to = match o.next() {
        Some(from) => from,
        None => {
            eprintln!("Usage: {} [from] [to]", exe.to_string_lossy());
            process::exit(1);
        }
    };
    // read files
    let from_content = match fs::read(&from) {
        Ok(from_content) => from_content,
        Err(e) => {
            eprintln!("Failed to read from-file: {}", e);
            process::exit(2);
        }
    };
    let to_content = match fs::read(&to) {
        Ok(to_content) => to_content,
        Err(e) => {
            eprintln!("Failed to read to-file: {}", e);
            process::exit(2);
        }
    };
    // run diff
    io::stdout()
        .write_all(&unified_diff::diff(
            &from_content,
            &from.to_string_lossy(),
            &to_content,
            &to.to_string_lossy(),
            1,
        ))
        .unwrap();
}
