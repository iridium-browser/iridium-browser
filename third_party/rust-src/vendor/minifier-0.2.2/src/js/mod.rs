// Take a look at the license at the top of the repository in the LICENSE file.

mod token;
mod tools;
mod utils;

pub use self::token::{tokenize, Condition, Keyword, Operation, ReservedChar, Token, Tokens};
pub use self::tools::{
    aggregate_strings, aggregate_strings_into_array, aggregate_strings_into_array_filter,
    aggregate_strings_into_array_with_separation,
    aggregate_strings_into_array_with_separation_filter, aggregate_strings_with_separation, minify,
    simple_minify,
};
pub use self::utils::{
    clean_token, clean_token_except, clean_tokens, clean_tokens_except,
    get_variable_name_and_value_positions, replace_token_with, replace_tokens_with,
};
