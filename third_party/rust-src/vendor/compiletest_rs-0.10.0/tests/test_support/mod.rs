//! Provides a simple way to set up compiletest sample testsuites used in testing.
//!
//! Inspired by cargo's `cargo-test-support` crate:
//! https://github.com/rust-lang/cargo/tree/master/crates/cargo-test-support
use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::cell::RefCell;
use std::sync::atomic::{AtomicUsize, Ordering};


static COMPILETEST_INTEGRATION_TEST_DIR: &str = "cit";

thread_local! {
    static TEST_ID: RefCell<Option<usize>> = RefCell::new(None);
}

lazy_static::lazy_static! {
    pub static ref GLOBAL_ROOT: PathBuf = {
        let mut path = env::current_exe().unwrap();
        path.pop(); // chop off exe name
        path.pop(); // chop off 'deps' part
        path.pop(); // chop off 'debug'

        path.push(COMPILETEST_INTEGRATION_TEST_DIR);
        path.mkdir_p();
        path
    };
}

pub fn testsuite(mode: &str) -> TestsuiteBuilder {
    let builder = TestsuiteBuilder::new(mode);
    builder.build();
    builder
}

pub struct TestsuiteBuilder {
    pub root: PathBuf,
}

impl TestsuiteBuilder {
    pub fn new(mode: &str) -> Self {
        static NEXT_ID: AtomicUsize = AtomicUsize::new(0);

        let id = NEXT_ID.fetch_add(1, Ordering::Relaxed);
        TEST_ID.with(|n| *n.borrow_mut() = Some(id));
        let root = GLOBAL_ROOT.join(format!("id{}", TEST_ID.with(|n|n.borrow().unwrap()))).join(mode);
        root.mkdir_p();

        Self {
            root,
        }
    }


    /// Creates a new file to be used for the integration test
    pub fn mk_file(&self, path: &str, body: &str) {
        self.root.mkdir_p();
        fs::write(self.root.join(&path), &body)
            .unwrap_or_else(|e| panic!("could not create file {}: {}", path, e));
    }

    /// Returns the contents of the file
    pub fn file_contents(&self, name: &str) -> String {
        fs::read_to_string(self.root.join(name)).expect("Unable to read file")
    }

    // Sets up a new testsuite root directory
    fn build(&self) {
        // Cleanup before we run the next test
        self.rm_root();

        // Create the new directory
        self.root.mkdir_p();
    }

    /// Deletes the root directory and all its contents
    fn rm_root(&self) {
        self.root.rm_rf();
    }
}

pub trait PathExt {
    fn rm_rf(&self);
    fn mkdir_p(&self);
}

impl PathExt for Path {
    fn rm_rf(&self) {
        if self.is_dir() {
            if let Err(e) = fs::remove_dir_all(self) {
                panic!("failed to remove {:?}: {:?}", self, e)
            }
        } else {
            if let Err(e) = fs::remove_file(self) {
                panic!("failed to remove {:?}: {:?}", self, e)
            }
        }
    }

    fn mkdir_p(&self) {
        fs::create_dir_all(self)
            .unwrap_or_else(|e| panic!("failed to mkdir_p {}: {}", self.display(), e))
    }
}
