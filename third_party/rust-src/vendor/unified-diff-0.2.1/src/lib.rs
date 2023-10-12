use std::collections::VecDeque;
use std::io::Write;

#[derive(Debug, PartialEq)]
pub enum DiffLine {
    Context(Vec<u8>),
    Expected(Vec<u8>),
    Actual(Vec<u8>),
    MissingNL,
}

#[derive(Debug, PartialEq)]
struct Mismatch {
    pub line_number_expected: u32,
    pub line_number_actual: u32,
    pub lines: Vec<DiffLine>,
}

impl Mismatch {
    fn new(line_number_expected: u32, line_number_actual: u32) -> Mismatch {
        Mismatch {
            line_number_expected,
            line_number_actual,
            lines: Vec::new(),
        }
    }
}

// Produces a diff between the expected output and actual output.
fn make_diff(expected: &[u8], actual: &[u8], context_size: usize) -> Vec<Mismatch> {
    let mut line_number_expected = 1;
    let mut line_number_actual = 1;
    let mut context_queue: VecDeque<&[u8]> = VecDeque::with_capacity(context_size);
    let mut lines_since_mismatch = context_size + 1;
    let mut results = Vec::new();
    let mut mismatch = Mismatch::new(0, 0);

    let mut expected_lines: Vec<&[u8]> = expected.split(|&c| c == b'\n').collect();
    let mut actual_lines: Vec<&[u8]> = actual.split(|&c| c == b'\n').collect();

    debug_assert_eq!(b"".split(|&c| c == b'\n').count(), 1);
    // ^ means that underflow here is impossible
    let expected_lines_count = expected_lines.len() as u32 - 1;
    let actual_lines_count = actual_lines.len() as u32 - 1;

    if expected_lines.last() == Some(&&b""[..]) {
        expected_lines.pop();
    }

    if actual_lines.last() == Some(&&b""[..]) {
        actual_lines.pop();
    }

    for result in diff::slice(&expected_lines, &actual_lines) {
        match result {
            diff::Result::Left(str) => {
                if lines_since_mismatch >= context_size && lines_since_mismatch > 0 {
                    results.push(mismatch);
                    mismatch = Mismatch::new(
                        line_number_expected - context_queue.len() as u32,
                        line_number_actual - context_queue.len() as u32,
                    );
                }

                while let Some(line) = context_queue.pop_front() {
                    mismatch.lines.push(DiffLine::Context(line.to_vec()));
                }

                if mismatch.lines.last() == Some(&DiffLine::MissingNL) {
                    mismatch.lines.pop();
                    match mismatch.lines.pop() {
                        Some(DiffLine::Actual(res)) => {
                            // We have to make sure that Actual (the + lines)
                            // always come after Expected (the - lines)
                            mismatch.lines.push(DiffLine::Expected(str.to_vec()));
                            if line_number_expected > expected_lines_count {
                                mismatch.lines.push(DiffLine::MissingNL)
                            }
                            mismatch.lines.push(DiffLine::Actual(res));
                            mismatch.lines.push(DiffLine::MissingNL);
                        }
                        _ => unreachable!("unterminated Left and Common lines shouldn't be followed by more Left lines"),
                    }
                } else {
                    mismatch.lines.push(DiffLine::Expected(str.to_vec()));
                    if line_number_expected > expected_lines_count {
                        mismatch.lines.push(DiffLine::MissingNL)
                    }
                }
                line_number_expected += 1;
                lines_since_mismatch = 0;
            }
            diff::Result::Right(str) => {
                if lines_since_mismatch >= context_size && lines_since_mismatch > 0 {
                    results.push(mismatch);
                    mismatch = Mismatch::new(
                        line_number_expected - context_queue.len() as u32,
                        line_number_actual - context_queue.len() as u32,
                    );
                }

                while let Some(line) = context_queue.pop_front() {
                    debug_assert!(mismatch.lines.last() != Some(&DiffLine::MissingNL));
                    mismatch.lines.push(DiffLine::Context(line.to_vec()));
                }

                mismatch.lines.push(DiffLine::Actual(str.to_vec()));
                if line_number_actual > actual_lines_count {
                    mismatch.lines.push(DiffLine::MissingNL)
                }
                line_number_actual += 1;
                lines_since_mismatch = 0;
            }
            diff::Result::Both(str, _) => {
                // if one of them is missing a newline and the other isn't, then they don't actually match
                if (line_number_actual > actual_lines_count)
                    && (line_number_expected > expected_lines_count)
                {
                    if context_queue.len() < context_size {
                        while let Some(line) = context_queue.pop_front() {
                            debug_assert!(mismatch.lines.last() != Some(&DiffLine::MissingNL));
                            mismatch.lines.push(DiffLine::Context(line.to_vec()));
                        }
                        if lines_since_mismatch < context_size {
                            mismatch.lines.push(DiffLine::Context(str.to_vec()));
                            mismatch.lines.push(DiffLine::MissingNL);
                        }
                    }
                    lines_since_mismatch = 0;
                } else if line_number_actual > actual_lines_count {
                    if lines_since_mismatch >= context_size && lines_since_mismatch > 0 {
                        results.push(mismatch);
                        mismatch = Mismatch::new(
                            line_number_expected - context_queue.len() as u32,
                            line_number_actual - context_queue.len() as u32,
                        );
                    }
                    while let Some(line) = context_queue.pop_front() {
                        debug_assert!(mismatch.lines.last() != Some(&DiffLine::MissingNL));
                        mismatch.lines.push(DiffLine::Context(line.to_vec()));
                    }
                    mismatch.lines.push(DiffLine::Expected(str.to_vec()));
                    mismatch.lines.push(DiffLine::Actual(str.to_vec()));
                    mismatch.lines.push(DiffLine::MissingNL);
                    lines_since_mismatch = 0;
                } else if line_number_expected > expected_lines_count {
                    if lines_since_mismatch >= context_size && lines_since_mismatch > 0 {
                        results.push(mismatch);
                        mismatch = Mismatch::new(
                            line_number_expected - context_queue.len() as u32,
                            line_number_actual - context_queue.len() as u32,
                        );
                    }
                    while let Some(line) = context_queue.pop_front() {
                        debug_assert!(mismatch.lines.last() != Some(&DiffLine::MissingNL));
                        mismatch.lines.push(DiffLine::Context(line.to_vec()));
                    }
                    mismatch.lines.push(DiffLine::Expected(str.to_vec()));
                    mismatch.lines.push(DiffLine::MissingNL);
                    mismatch.lines.push(DiffLine::Actual(str.to_vec()));
                    lines_since_mismatch = 0;
                } else {
                    debug_assert!(context_queue.len() <= context_size);
                    if context_queue.len() >= context_size {
                        let _ = context_queue.pop_front();
                    }
                    if lines_since_mismatch < context_size {
                        mismatch.lines.push(DiffLine::Context(str.to_vec()));
                    } else if context_size > 0 {
                        context_queue.push_back(str);
                    }
                    lines_since_mismatch += 1;
                }
                line_number_expected += 1;
                line_number_actual += 1;
            }
        }
    }

    results.push(mismatch);
    results.remove(0);

    if results.len() == 0 && expected_lines_count != actual_lines_count {
        let mut mismatch = Mismatch::new(expected_lines.len() as u32, actual_lines.len() as u32);
        // empty diff and only expected lines has a missing line at end
        if expected_lines_count != expected_lines.len() as u32 {
            mismatch.lines.push(DiffLine::Expected(
                expected_lines
                    .pop()
                    .expect("can't be empty; produced by split()")
                    .to_vec(),
            ));
            mismatch.lines.push(DiffLine::MissingNL);
            mismatch.lines.push(DiffLine::Actual(
                actual_lines
                    .pop()
                    .expect("can't be empty; produced by split()")
                    .to_vec(),
            ));
            results.push(mismatch);
        } else if actual_lines_count != actual_lines.len() as u32 {
            mismatch.lines.push(DiffLine::Expected(
                expected_lines
                    .pop()
                    .expect("can't be empty; produced by split()")
                    .to_vec(),
            ));
            mismatch.lines.push(DiffLine::Actual(
                actual_lines
                    .pop()
                    .expect("can't be empty; produced by split()")
                    .to_vec(),
            ));
            mismatch.lines.push(DiffLine::MissingNL);
            results.push(mismatch);
        }
    }

    results
}

pub fn diff(
    expected: &[u8],
    expected_filename: &str,
    actual: &[u8],
    actual_filename: &str,
    context_size: usize,
) -> Vec<u8> {
    let mut output =
        format!("--- {}\t\n+++ {}\t\n", expected_filename, actual_filename).into_bytes();
    let diff_results = make_diff(expected, actual, context_size);
    if diff_results.len() == 0 {
        return Vec::new();
    };
    for result in diff_results {
        let mut line_number_expected = result.line_number_expected;
        let mut line_number_actual = result.line_number_actual;
        let mut expected_count = 0;
        let mut actual_count = 0;
        for line in &result.lines {
            match line {
                DiffLine::Expected(_) => {
                    expected_count += 1;
                }
                DiffLine::Context(_) => {
                    expected_count += 1;
                    actual_count += 1;
                }
                DiffLine::Actual(_) => {
                    actual_count += 1;
                }
                DiffLine::MissingNL => {}
            }
        }
        // Let's imagine this diff file
        //
        // --- a/something
        // +++ b/something
        // @@ -2,0 +3,1 @@
        // + x
        //
        // In the unified diff format as implemented by GNU diff and patch,
        // this is an instruction to insert the x *after* the preexisting line 2,
        // not before. You can demonstrate it this way:
        //
        // $ echo -ne '--- a/something\t\n+++ b/something\t\n@@ -2,0 +3,1 @@\n+ x\n' > diff
        // $ echo -ne 'a\nb\nc\nd\n' > something
        // $ patch -p1 < diff
        // patching file something
        // $ cat something
        // a
        // b
        //  x
        // c
        // d
        //
        // Notice how the x winds up at line 3, not line 2. This requires contortions to
        // work with our diffing algorithm, which keeps track of the "intended destination line",
        // not a line that things are supposed to be placed after. It's changing the first number,
        // not the second, that actually affects where the x goes.
        //
        // # change the first number from 2 to 3, and now the x is on line 4 (it's placed after line 3)
        // $ echo -ne '--- a/something\t\n+++ b/something\t\n@@ -3,0 +3,1 @@\n+ x\n' > diff
        // $ echo -ne 'a\nb\nc\nd\n' > something
        // $ patch -p1 < diff
        // patching file something
        // $ cat something
        // a
        // b
        // c
        //  x
        // d
        // # change the third number from 3 to 1000, and it's obvious that it's the first number that's
        // # actually being read
        // $ echo -ne '--- a/something\t\n+++ b/something\t\n@@ -2,0 +1000,1 @@\n+ x\n' > diff
        // $ echo -ne 'a\nb\nc\nd\n' > something
        // $ patch -p1 < diff
        // patching file something
        // $ cat something
        // a
        // b
        //  x
        // c
        // d
        //
        // Now watch what happens if I add a context line:
        //
        // $ echo -ne '--- a/something\t\n+++ b/something\t\n@@ -2,1 +3,2 @@\n+ x\n c\n' > diff
        // $ echo -ne 'a\nb\nc\nd\n' > something
        // $ patch -p1 < diff
        // patching file something
        // Hunk #1 succeeded at 3 (offset 1 line).
        //
        // It technically "succeeded", but this is a warning. We want to produce clean diffs.
        // Now that I have a context line, I'm supposed to say what line it's actually on, which is the
        // line that the x will wind up on, and not the line immediately before.
        //
        // $ echo -ne '--- a/something\t\n+++ b/something\t\n@@ -3,1 +3,2 @@\n+ x\n c\n' > diff
        // $ echo -ne 'a\nb\nc\nd\n' > something
        // $ patch -p1 < diff
        // patching file something
        // $ cat something
        // a
        // b
        //  x
        // c
        // d
        //
        // I made this comment because this stuff is not obvious from GNU's
        // documentation on the format at all.
        if expected_count == 0 {
            line_number_expected -= 1
        }
        if actual_count == 0 {
            line_number_actual -= 1
        }
        let exp_ct = if expected_count == 1 {
            String::new()
        } else {
            format!(",{}", expected_count)
        };
        let act_ct = if actual_count == 1 {
            String::new()
        } else {
            format!(",{}", actual_count)
        };
        writeln!(
            output,
            "@@ -{}{} +{}{} @@",
            line_number_expected, exp_ct, line_number_actual, act_ct
        )
        .expect("write to Vec is infallible");
        for line in result.lines {
            match line {
                DiffLine::Expected(e) => {
                    write!(output, "-").expect("write to Vec is infallible");
                    output.write_all(&e).expect("write to Vec is infallible");
                    writeln!(output).unwrap();
                }
                DiffLine::Context(c) => {
                    write!(output, " ").expect("write to Vec is infallible");
                    output.write_all(&c).expect("write to Vec is infallible");
                    writeln!(output).unwrap();
                }
                DiffLine::Actual(r) => {
                    write!(output, "+",).expect("write to Vec is infallible");
                    output.write_all(&r).expect("write to Vec is infallible");
                    writeln!(output).unwrap();
                }
                DiffLine::MissingNL => {
                    writeln!(output, r"\ No newline at end of file")
                        .expect("write to Vec is infallible");
                }
            }
        }
    }
    output
}

#[test]
fn test_permutations() {
    // test all possible six-line files.
    for &a in &[0, 1, 2] {
        for &b in &[0, 1, 2] {
            for &c in &[0, 1, 2] {
                for &d in &[0, 1, 2] {
                    for &e in &[0, 1, 2] {
                        for &f in &[0, 1, 2] {
                            use std::fs::{self, File};
                            use std::io::Write;
                            use std::process::Command;
                            let mut alef = Vec::new();
                            let mut bet = Vec::new();
                            alef.write_all(if a == 0 { b"a\n" } else { b"b\n" })
                                .unwrap();
                            if a != 2 {
                                bet.write_all(b"b\n").unwrap();
                            }
                            alef.write_all(if b == 0 { b"c\n" } else { b"d\n" })
                                .unwrap();
                            if b != 2 {
                                bet.write_all(b"d\n").unwrap();
                            }
                            alef.write_all(if c == 0 { b"e\n" } else { b"f\n" })
                                .unwrap();
                            if c != 2 {
                                bet.write_all(b"f\n").unwrap();
                            }
                            alef.write_all(if d == 0 { b"g\n" } else { b"h\n" })
                                .unwrap();
                            if d != 2 {
                                bet.write_all(b"h\n").unwrap();
                            }
                            alef.write_all(if e == 0 { b"i\n" } else { b"j\n" })
                                .unwrap();
                            if e != 2 {
                                bet.write_all(b"j\n").unwrap();
                            }
                            alef.write_all(if f == 0 { b"k\n" } else { b"l\n" })
                                .unwrap();
                            if f != 2 {
                                bet.write_all(b"l\n").unwrap();
                            }
                            // This test diff is intentionally reversed.
                            // We want it to turn the alef into bet.
                            let diff = diff(&alef, "a/alef", &bet, "target/alef", 2);
                            File::create("target/ab.diff")
                                .unwrap()
                                .write_all(&diff)
                                .unwrap();
                            let mut fa = File::create("target/alef").unwrap();
                            fa.write_all(&alef[..]).unwrap();
                            let mut fb = File::create("target/bet").unwrap();
                            fb.write_all(&bet[..]).unwrap();
                            let _ = fa;
                            let _ = fb;
                            let output = Command::new("patch")
                                .arg("-p0")
                                .stdin(File::open("target/ab.diff").unwrap())
                                .output()
                                .unwrap();
                            if !output.status.success() {
                                panic!("{:?}", output);
                            }
                            //println!("{}", String::from_utf8_lossy(&output.stdout));
                            //println!("{}", String::from_utf8_lossy(&output.stderr));
                            let alef = fs::read("target/alef").unwrap();
                            assert_eq!(alef, bet);
                        }
                    }
                }
            }
        }
    }
}

#[test]
fn test_permutations_missing_line_ending() {
    // test all possible six-line files with missing newlines.
    for &a in &[0, 1, 2] {
        for &b in &[0, 1, 2] {
            for &c in &[0, 1, 2] {
                for &d in &[0, 1, 2] {
                    for &e in &[0, 1, 2] {
                        for &f in &[0, 1, 2] {
                            for &g in &[0, 1, 2] {
                                use std::fs::{self, File};
                                use std::io::Write;
                                use std::process::Command;
                                let mut alef = Vec::new();
                                let mut bet = Vec::new();
                                alef.write_all(if a == 0 { b"a\n" } else { b"b\n" })
                                    .unwrap();
                                if a != 2 {
                                    bet.write_all(b"b\n").unwrap();
                                }
                                alef.write_all(if b == 0 { b"c\n" } else { b"d\n" })
                                    .unwrap();
                                if b != 2 {
                                    bet.write_all(b"d\n").unwrap();
                                }
                                alef.write_all(if c == 0 { b"e\n" } else { b"f\n" })
                                    .unwrap();
                                if c != 2 {
                                    bet.write_all(b"f\n").unwrap();
                                }
                                alef.write_all(if d == 0 { b"g\n" } else { b"h\n" })
                                    .unwrap();
                                if d != 2 {
                                    bet.write_all(b"h\n").unwrap();
                                }
                                alef.write_all(if e == 0 { b"i\n" } else { b"j\n" })
                                    .unwrap();
                                if e != 2 {
                                    bet.write_all(b"j\n").unwrap();
                                }
                                alef.write_all(if f == 0 { b"k\n" } else { b"l\n" })
                                    .unwrap();
                                if f != 2 {
                                    bet.write_all(b"l\n").unwrap();
                                }
                                match g {
                                    0 => {
                                        alef.pop();
                                    }
                                    1 => {
                                        bet.pop();
                                    }
                                    2 => {
                                        alef.pop();
                                        bet.pop();
                                    }
                                    _ => unreachable!(),
                                }
                                // This test diff is intentionally reversed.
                                // We want it to turn the alef into bet.
                                let diff = diff(&alef, "a/alefn", &bet, "target/alefn", 2);
                                File::create("target/abn.diff")
                                    .unwrap()
                                    .write_all(&diff)
                                    .unwrap();
                                let mut fa = File::create("target/alefn").unwrap();
                                fa.write_all(&alef[..]).unwrap();
                                let mut fb = File::create("target/betn").unwrap();
                                fb.write_all(&bet[..]).unwrap();
                                let _ = fa;
                                let _ = fb;
                                let output = Command::new("patch")
                                    .arg("-p0")
                                    .stdin(File::open("target/abn.diff").unwrap())
                                    .output()
                                    .unwrap();
                                if !output.status.success() {
                                    panic!("{:?}", output);
                                }
                                //println!("{}", String::from_utf8_lossy(&output.stdout));
                                //println!("{}", String::from_utf8_lossy(&output.stderr));
                                let alef = fs::read("target/alefn").unwrap();
                                assert_eq!(alef, bet);
                            }
                        }
                    }
                }
            }
        }
    }
}

#[test]
fn test_permutations_empty_lines() {
    // test all possible six-line files with missing newlines.
    for &a in &[0, 1, 2] {
        for &b in &[0, 1, 2] {
            for &c in &[0, 1, 2] {
                for &d in &[0, 1, 2] {
                    for &e in &[0, 1, 2] {
                        for &f in &[0, 1, 2] {
                            for &g in &[0, 1, 2, 3] {
                                use std::fs::{self, File};
                                use std::io::Write;
                                use std::process::Command;
                                let mut alef = Vec::new();
                                let mut bet = Vec::new();
                                alef.write_all(if a == 0 { b"\n" } else { b"b\n" }).unwrap();
                                if a != 2 {
                                    bet.write_all(b"b\n").unwrap();
                                }
                                alef.write_all(if b == 0 { b"\n" } else { b"d\n" }).unwrap();
                                if b != 2 {
                                    bet.write_all(b"d\n").unwrap();
                                }
                                alef.write_all(if c == 0 { b"\n" } else { b"f\n" }).unwrap();
                                if c != 2 {
                                    bet.write_all(b"f\n").unwrap();
                                }
                                alef.write_all(if d == 0 { b"\n" } else { b"h\n" }).unwrap();
                                if d != 2 {
                                    bet.write_all(b"h\n").unwrap();
                                }
                                alef.write_all(if e == 0 { b"\n" } else { b"j\n" }).unwrap();
                                if e != 2 {
                                    bet.write_all(b"j\n").unwrap();
                                }
                                alef.write_all(if f == 0 { b"\n" } else { b"l\n" }).unwrap();
                                if f != 2 {
                                    bet.write_all(b"l\n").unwrap();
                                }
                                match g {
                                    0 => {
                                        alef.pop();
                                    }
                                    1 => {
                                        bet.pop();
                                    }
                                    2 => {
                                        alef.pop();
                                        bet.pop();
                                    }
                                    3 => {}
                                    _ => unreachable!(),
                                }
                                // This test diff is intentionally reversed.
                                // We want it to turn the alef into bet.
                                let diff = diff(&alef, "a/alef_", &bet, "target/alef_", 2);
                                File::create("target/ab_.diff")
                                    .unwrap()
                                    .write_all(&diff)
                                    .unwrap();
                                let mut fa = File::create("target/alef_").unwrap();
                                fa.write_all(&alef[..]).unwrap();
                                let mut fb = File::create("target/bet_").unwrap();
                                fb.write_all(&bet[..]).unwrap();
                                let _ = fa;
                                let _ = fb;
                                let output = Command::new("patch")
                                    .arg("-p0")
                                    .stdin(File::open("target/ab_.diff").unwrap())
                                    .output()
                                    .unwrap();
                                if !output.status.success() {
                                    panic!("{:?}", output);
                                }
                                //println!("{}", String::from_utf8_lossy(&output.stdout));
                                //println!("{}", String::from_utf8_lossy(&output.stderr));
                                let alef = fs::read("target/alef_").unwrap();
                                assert_eq!(alef, bet);
                            }
                        }
                    }
                }
            }
        }
    }
}

#[test]
fn test_permutations_missing_lines() {
    // test all possible six-line files.
    for &a in &[0, 1, 2] {
        for &b in &[0, 1, 2] {
            for &c in &[0, 1, 2] {
                for &d in &[0, 1, 2] {
                    for &e in &[0, 1, 2] {
                        for &f in &[0, 1, 2] {
                            use std::fs::{self, File};
                            use std::io::Write;
                            use std::process::Command;
                            let mut alef = Vec::new();
                            let mut bet = Vec::new();
                            alef.write_all(if a == 0 { b"a\n" } else { b"" }).unwrap();
                            if a != 2 {
                                bet.write_all(b"b\n").unwrap();
                            }
                            alef.write_all(if b == 0 { b"c\n" } else { b"" }).unwrap();
                            if b != 2 {
                                bet.write_all(b"d\n").unwrap();
                            }
                            alef.write_all(if c == 0 { b"e\n" } else { b"" }).unwrap();
                            if c != 2 {
                                bet.write_all(b"f\n").unwrap();
                            }
                            alef.write_all(if d == 0 { b"g\n" } else { b"" }).unwrap();
                            if d != 2 {
                                bet.write_all(b"h\n").unwrap();
                            }
                            alef.write_all(if e == 0 { b"i\n" } else { b"" }).unwrap();
                            if e != 2 {
                                bet.write_all(b"j\n").unwrap();
                            }
                            alef.write_all(if f == 0 { b"k\n" } else { b"" }).unwrap();
                            if f != 2 {
                                bet.write_all(b"l\n").unwrap();
                            }
                            // This test diff is intentionally reversed.
                            // We want it to turn the alef into bet.
                            let diff = diff(&alef, "a/alefx", &bet, "target/alefx", 2);
                            File::create("target/abx.diff")
                                .unwrap()
                                .write_all(&diff)
                                .unwrap();
                            let mut fa = File::create("target/alefx").unwrap();
                            fa.write_all(&alef[..]).unwrap();
                            let mut fb = File::create("target/betx").unwrap();
                            fb.write_all(&bet[..]).unwrap();
                            let _ = fa;
                            let _ = fb;
                            let output = Command::new("patch")
                                .arg("-p0")
                                .stdin(File::open("target/abx.diff").unwrap())
                                .output()
                                .unwrap();
                            if !output.status.success() {
                                panic!("{:?}", output);
                            }
                            //println!("{}", String::from_utf8_lossy(&output.stdout));
                            //println!("{}", String::from_utf8_lossy(&output.stderr));
                            let alef = fs::read("target/alefx").unwrap();
                            assert_eq!(alef, bet);
                        }
                    }
                }
            }
        }
    }
}

#[test]
fn test_permutations_reverse() {
    // test all possible six-line files.
    for &a in &[0, 1, 2] {
        for &b in &[0, 1, 2] {
            for &c in &[0, 1, 2] {
                for &d in &[0, 1, 2] {
                    for &e in &[0, 1, 2] {
                        for &f in &[0, 1, 2] {
                            use std::fs::{self, File};
                            use std::io::Write;
                            use std::process::Command;
                            let mut alef = Vec::new();
                            let mut bet = Vec::new();
                            alef.write_all(if a == 0 { b"a\n" } else { b"f\n" })
                                .unwrap();
                            if a != 2 {
                                bet.write_all(b"a\n").unwrap();
                            }
                            alef.write_all(if b == 0 { b"b\n" } else { b"e\n" })
                                .unwrap();
                            if b != 2 {
                                bet.write_all(b"b\n").unwrap();
                            }
                            alef.write_all(if c == 0 { b"c\n" } else { b"d\n" })
                                .unwrap();
                            if c != 2 {
                                bet.write_all(b"c\n").unwrap();
                            }
                            alef.write_all(if d == 0 { b"d\n" } else { b"c\n" })
                                .unwrap();
                            if d != 2 {
                                bet.write_all(b"d\n").unwrap();
                            }
                            alef.write_all(if e == 0 { b"e\n" } else { b"b\n" })
                                .unwrap();
                            if e != 2 {
                                bet.write_all(b"e\n").unwrap();
                            }
                            alef.write_all(if f == 0 { b"f\n" } else { b"a\n" })
                                .unwrap();
                            if f != 2 {
                                bet.write_all(b"f\n").unwrap();
                            }
                            // This test diff is intentionally reversed.
                            // We want it to turn the alef into bet.
                            let diff = diff(&alef, "a/alefr", &bet, "target/alefr", 2);
                            File::create("target/abr.diff")
                                .unwrap()
                                .write_all(&diff)
                                .unwrap();
                            let mut fa = File::create("target/alefr").unwrap();
                            fa.write_all(&alef[..]).unwrap();
                            let mut fb = File::create("target/betr").unwrap();
                            fb.write_all(&bet[..]).unwrap();
                            let _ = fa;
                            let _ = fb;
                            let output = Command::new("patch")
                                .arg("-p0")
                                .stdin(File::open("target/abr.diff").unwrap())
                                .output()
                                .unwrap();
                            if !output.status.success() {
                                panic!("{:?}", output);
                            }
                            //println!("{}", String::from_utf8_lossy(&output.stdout));
                            //println!("{}", String::from_utf8_lossy(&output.stderr));
                            let alef = fs::read("target/alefr").unwrap();
                            assert_eq!(alef, bet);
                        }
                    }
                }
            }
        }
    }
}
