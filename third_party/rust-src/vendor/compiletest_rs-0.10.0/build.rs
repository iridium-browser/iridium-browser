use std::env;

pub fn main() {
    if env::var("CARGO_FEATURE_RUSTC").is_err() {
        println!("cargo:rustc-env=TARGET={}", env::var("TARGET").unwrap());
        println!("cargo:rustc-env=HOST={}", env::var("HOST").unwrap());
    }
}
