use assert_cmd::prelude::*;

use std::process::Command;

// Timings are flaky, so tests would spuriously fail.
// Thus we replace all `/([0-9]+)ms/` with underscores
fn replace_ms(data: &[u8]) -> Vec<u8> {
    let mut skip = false;
    let mut seen_s = false;
    let mut v: Vec<u8> = data
        .iter()
        .rev()
        .filter_map(|&b| match (b, skip, seen_s) {
            (b'0'..=b'9', true, _) => None,
            (_, true, _) => {
                skip = false;
                Some(b)
            }
            (b's', _, _) => {
                seen_s = true;
                Some(b)
            }
            (b'm', _, true) => {
                seen_s = false;
                skip = true;
                Some(b)
            }
            _ => Some(b),
        })
        .collect();
    v.reverse();
    v
}

fn main() {
    for entry in glob::glob("examples/*.rs").expect("Failed to read glob pattern") {
        let entry = entry.unwrap();
        let mut cmd = Command::cargo_bin(entry.with_extension("").to_str().unwrap()).unwrap();
        let output = cmd.unwrap();
        let stderr = entry.with_extension("stderr");
        let stdout = entry.with_extension("stdout");

        if std::env::args().any(|arg| arg == "--bless") {
            if output.stderr.is_empty() {
                let _ = std::fs::remove_file(stderr);
            } else {
                std::fs::write(stderr, replace_ms(&output.stderr)).unwrap();
            }
            if output.stdout.is_empty() {
                let _ = std::fs::remove_file(stdout);
            } else {
                std::fs::write(stdout, replace_ms(&output.stdout)).unwrap();
            }
        } else {
            if output.stderr.is_empty() {
                assert!(
                    !stderr.exists(),
                    "{} exists but there was no stderr output",
                    stderr.display()
                );
            } else {
                assert!(
                    std::fs::read(&stderr).unwrap() == replace_ms(&output.stderr),
                    "{} is not the expected output, rerun the test with `cargo test -- -- --bless`",
                    stderr.display()
                );
            }
            if output.stdout.is_empty() {
                assert!(!stdout.exists());
            } else {
                assert!(
                    std::fs::read(&stdout).unwrap() == replace_ms(&output.stdout),
                    "{} is not the expected output, rerun the test with `cargo test -- -- --bless`",
                    stdout.display()
                );
            }
        }
    }
}
