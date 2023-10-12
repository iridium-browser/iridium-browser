mod common;
#[cfg(target_env = "msvc")]
mod msvc;
#[cfg(not(target_env = "msvc"))]
mod not_msvc;

#[cfg(target_env = "msvc")]
use msvc::*;
#[cfg(not(target_env = "msvc"))]
use not_msvc::*;

fn main() {
    if cfg!(feature = "system") {
        probe_and_link();
    } else {
        build_and_link();
    }
}
