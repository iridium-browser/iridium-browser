// Take a look at the license at the top of the repository in the LICENSE file.

extern crate minifier;

use std::ops::Range;

fn get_ranges(pos: usize, s: &str) -> Range<usize> {
    let mut start = pos;
    let mut end = pos;
    if start >= 20 {
        start -= 20;
    } else {
        start = 0;
    }
    if end < s.len() - 20 {
        end += 20;
    } else {
        end = s.len();
    }
    start..end
}

fn compare_strs(minified: &str, str2: &str) {
    let mut it1 = minified.char_indices();
    let mut it2 = str2.char_indices();

    loop {
        match (it1.next(), it2.next()) {
            (Some((pos, c1)), Some((_, c2))) => {
                if c1 != c2 {
                    println!(
                        "{}\n==== differs from: ====\n{}",
                        &minified[get_ranges(pos, minified)],
                        &str2[get_ranges(pos, str2)]
                    );
                    panic!("Chars differ");
                }
            }
            (None, Some((pos, _))) => {
                println!("missing: {}...", &str2[get_ranges(pos + 20, str2)]);
                panic!("Missing parts of minified content");
            }
            (Some((pos, _)), None) => {
                println!("extra: {}...", &minified[get_ranges(pos + 20, minified)]);
                panic!("Extra part in minified content");
            }
            (None, None) => {
                break;
            }
        }
    }
}

#[test]
fn test_minification() {
    use minifier::js::minify;

    let source = include_str!("./files/main.js");
    let expected_result = include_str!("./files/minified_main.js");
    compare_strs(&minify(source).to_string(), expected_result);
}
