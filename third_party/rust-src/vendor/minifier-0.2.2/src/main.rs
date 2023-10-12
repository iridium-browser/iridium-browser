// Take a look at the license at the top of the repository in the LICENSE file.

extern crate minifier;

use std::env;
use std::ffi::OsStr;
use std::fs::{File, OpenOptions};
use std::io::{self, Read, Write};
use std::path::{Path, PathBuf};

use minifier::{css, js, json};

fn print_help() {
    println!(
        r##"For now, this minifier supports the following type of files:

 * .css
 * .js
 * .json"##
    );
}

pub fn get_all_data(file_path: &str) -> io::Result<String> {
    let mut file = File::open(file_path)?;
    let mut data = String::new();

    file.read_to_string(&mut data).unwrap();
    Ok(data)
}

fn call_minifier<F>(file_path: &str, func: F)
where
    F: Fn(&str) -> String,
{
    match get_all_data(file_path) {
        Ok(content) => {
            let mut out = PathBuf::from(file_path);
            let original_extension = out
                .extension()
                .unwrap_or_else(|| OsStr::new(""))
                .to_str()
                .unwrap_or("")
                .to_owned();
            out.set_extension(format!("min.{}", original_extension));
            if let Ok(mut file) = OpenOptions::new()
                .truncate(true)
                .write(true)
                .create(true)
                .open(out.clone())
            {
                if let Err(e) = write!(file, "{}", func(&content)) {
                    eprintln!("Impossible to write into {:?}: {}", out, e);
                } else {
                    println!("{:?}: done -> generated into {:?}", file_path, out);
                }
            } else {
                eprintln!("Impossible to create new file: {:?}", out);
            }
        }
        Err(e) => eprintln!("\"{}\": {}", file_path, e),
    }
}

fn main() {
    let args: Vec<_> = env::args().skip(1).collect();

    if args.is_empty() {
        println!("Missing files to work on...\nExample: ./minifier file.js\n");
        print_help();
        return;
    }
    for arg in &args {
        let p = Path::new(arg);

        if !p.is_file() {
            eprintln!("\"{}\" isn't a file", arg);
            continue;
        }
        match p
            .extension()
            .unwrap_or_else(|| OsStr::new(""))
            .to_str()
            .unwrap_or("")
        {
            "css" => call_minifier(arg, |s| {
                css::minify(s).expect("css minification failed").to_string()
            }),
            "js" => call_minifier(arg, |s| js::minify(s).to_string()),
            "json" => call_minifier(arg, |s| json::minify(s).to_string()),
            // "html" | "htm" => call_minifier(arg, html::minify),
            x => println!("\"{}\": this format isn't supported", x),
        }
    }
}
