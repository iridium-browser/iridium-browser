fn main() {
    println!("clicolor: {:?}", concolor_query::clicolor());
    println!("clicolor_force: {}", concolor_query::clicolor_force());
    println!("no_color: {}", concolor_query::no_color());
    println!(
        "term_supports_ansi_color: {}",
        concolor_query::term_supports_ansi_color()
    );
    println!(
        "term_supports_color: {}",
        concolor_query::term_supports_color()
    );
    println!("truecolor: {}", concolor_query::truecolor());
    println!(
        "enable_ansi_colors: {:?}",
        concolor_query::windows::enable_ansi_colors()
    );
    println!("is_ci: {:?}", concolor_query::is_ci());
}
