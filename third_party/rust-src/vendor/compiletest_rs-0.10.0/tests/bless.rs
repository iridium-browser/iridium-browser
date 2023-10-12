//! Tests for the `bless` option

extern crate compiletest_rs as compiletest;

mod test_support;
use test_support::{testsuite, TestsuiteBuilder, GLOBAL_ROOT};
use compiletest::Config;

fn setup(mode: &str) -> (Config, TestsuiteBuilder) {
    let builder = testsuite(mode);
    let mut config = Config::default();
    let cfg_mode = mode.parse().expect("Invalid mode");
    config.mode = cfg_mode;
    config.src_base = builder.root.clone();
    config.build_base = GLOBAL_ROOT.join("build_base");

    (config, builder)
}

#[test]
fn test_bless_new_file() {
    let (mut config, builder) = setup("ui");
    config.bless = true;

    builder.mk_file(
              "foobar.rs",
              r#"
                  #[warn(unused_variables)]
                  fn main() {
                      let abc = "foobar";
                  }
              "#,
          );
    compiletest::run_tests(&config);

    // Blessing should cause the stderr to be created directly
    assert!(builder.file_contents("foobar.stderr").contains("unused variable"));

    // And a second run of the tests, with blessing disabled should work just fine
    config.bless = false;
    compiletest::run_tests(&config);
}

#[test]
fn test_bless_update_file() {
    let (mut config, builder) = setup("ui");
    config.bless = true;

    builder.mk_file(
        "foobar2.rs",
        r#"
            #[warn(unused_variables)]
            fn main() {
                let abc = "foobar_update";
            }
        "#,
    );
    builder.mk_file(
        "foobar2.stderr",
        r#"
            warning: unused variable: `abc`
             --> $DIR/foobar2.rs:4:27
              |
            4 |                       let abc = "foobar";
              |                           ^^^ help: if this is intentional, prefix it with an underscore: `_abc`
              |
            note: the lint level is defined here
             --> $DIR/foobar2.rs:2:26
              |
            2 |                   #[warn(unused_variables)]
              |                          ^^^^^^^^^^^^^^^^

            warning: 1 warning emitted
        "#,
    );
    compiletest::run_tests(&config);

    // Blessing should cause the stderr to be created directly
    assert!(builder.file_contents("foobar2.stderr").contains("unused variable"));
    assert!(builder.file_contents("foobar2.stderr").contains("foobar_update"));

    // And a second run of the tests, with blessing disabled should work just fine
    config.bless = false;
    compiletest::run_tests(&config);
}
