[![Clippy Linting Result](https://clippy.bashy.io/github/oli-obk/quine-mc_cluskey/master/badge.svg)](https://clippy.bashy.io/github/oli-obk/quine-mc_cluskey/master/log)
[![Current Version](http://meritbadge.herokuapp.com/quine-mc_cluskey)](https://crates.io/crates/quine-mc_cluskey)
[![Build Status](https://travis-ci.org/oli-obk/quine-mc_cluskey.svg?branch=master)](https://travis-ci.org/oli-obk/quine-mc_cluskey)

An algorithm to automatically minimize boolean expressions.

# Example

```rust
extern crate quine_mc_cluskey;

use quine_mc_cluskey::*;
use quine_mc_cluskey::Bool::{And, Or, Not, True, False};

fn main() {
    // !false => true
    assert_eq!(
        Not(Box::new(False)).simplify(),
        vec![True]
    );

    // a && (b || a) => a
    assert_eq!(
        And(vec![Bool::Term(0),
        Or(vec![Bool::Term(1), Bool::Term(0)])]).simplify(), vec![Bool::Term(0)]
    );
}
```

# Obtaining a minimal "and of or" form

Sometimes an expression of the form `a && (b || c)` is shorter than the `a && b || a && c` form.
We can simply negate the original expression and negate all the resulting simplified expressions to obtain that form.

```rust
extern crate quine_mc_cluskey;
use quine_mc_cluskey::Bool;

fn main() {
    let a: Bool = Bool::And(vec![Bool::True, Bool::True]);
    let simplified: Vec<Bool> = Bool::Not(Box::new(a)).simplify()
        .iter().map(simple_negate).collect();
}

fn simple_negate(b: &Bool) -> Bool {
    use quine_mc_cluskey::Bool::*;
    let b = b.clone();

    match b {
        True => False,
        False => True,
        t @ Term(_) => Not(Box::new(t)),
        And(mut v) => {
            for el in &mut v {
                *el = simple_negate(el);
            }
            Or(v)
        },
        Or(mut v) => {
            for el in &mut v {
                *el = simple_negate(el);
            }
            And(v)
        },
        Not(inner) => *inner,
    }
}
```
