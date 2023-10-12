#![deny(
    missing_copy_implementations,
    missing_debug_implementations,
    missing_docs,
    trivial_casts,
    trivial_numeric_casts,
    unsafe_code,
    unused_import_braces,
    unused_qualifications,
)]

#![cfg_attr(feature = "dev", feature(plugin))]
#![cfg_attr(feature = "dev", plugin(clippy))]
#![cfg_attr(feature = "dev", deny(clippy))]

//! Fonctions to decode and encode [RFC-3492 Punycode](https://tools.ietf.org/html/rfc3492).

// See [RFC-3492, section 4](https://tools.ietf.org/html/rfc3492#section-4).
const BASE         : u32 = 36;
const TMIN         : u32 = 1;
const TMAX         : u32 = 26;
const SKEW         : u32 = 38;
const DAMP         : u32 = 700;
const INITIAL_BIAS : u32 = 72;
const INITIAL_N    : u32 = 128;
const DELIMITER    : char = '-';

/// Decode the string as Punycode. The string should not contain the initial `xn--` and must
/// contain only ASCII characters.
/// # Example
/// ```
/// assert_eq!(
///     punycode::decode("acadmie-franaise-npb1a").unwrap(),
///     "académie-française"
/// );
/// ```
pub fn decode(input: &str) -> Result<String, ()> {
    if !input.is_ascii() {
        return Err(());
    }

    let mut n = INITIAL_N;
    let mut i = 0;
    let mut bias = INITIAL_BIAS;

    let (mut output, input) = if let Some(i) = input.rfind(DELIMITER) {
        (input[0..i].chars().collect(), &input[i+1..])
    }
    else {
        (vec![], &input[..])
    };

    let mut it = input.chars().peekable();
    while it.peek() != None {
        let oldi = i;
        let mut w = 1;

        for k in 1.. {
            let c = if let Some(c) = it.next() {
                c
            }
            else {
                return Err(());
            };

            let k = k*BASE;

            let digit = decode_digit(c);

            if digit == BASE {
                return Err(());
            }

            // overflow check
            if digit > (std::u32::MAX - i) / w {
                return Err(());
            }
            i += digit * w;

            let t = clamped_sub(TMIN, k, bias, TMAX);
            if digit < t {
                break;
            }

            // overflow check
            if BASE > (std::u32::MAX - t) / w {
                return Err(());
            }
            w *= BASE - t;
        }

        let len = (output.len() + 1) as u32;
        bias = adapt(i - oldi, len, oldi == 0);

        let il = i / len;
        // overflow check
        if n > std::u32::MAX - il {
            return Err(());
        }
        n += il;
        i %= len;

        if let Some(c) = std::char::from_u32(n) {
            output.insert(i as usize, c);
        }
        else {
            return Err(());
        }

        i += 1;
    }

    Ok(output.iter().cloned().collect())
}

/// Encode a string as punycode. The result string will contain only ASCII characters. The result
/// string does not start with `xn--`.
/// # Example
/// ```
/// assert_eq!(
///     punycode::encode("académie-française").unwrap(),
///     "acadmie-franaise-npb1a"
/// );
/// ```
pub fn encode(input: &str) -> Result<String, ()> {
    encode_slice(&input.chars().collect::<Vec<char>>())
}

fn encode_slice(input: &[char]) -> Result<String, ()> {
    let mut n = INITIAL_N;
    let mut delta = 0;
    let mut bias = INITIAL_BIAS;

    let mut output : String = input.iter().filter(|&&c| c.is_ascii()).cloned().collect();
    let mut h = output.len() as u32;
    let b = h;

    if b > 0 {
        output.push(DELIMITER)
    }

    while h < input.len() as u32 {
        let m = *input.iter().filter(|&&c| (c as u32) >= n).min().unwrap() as u32;

        if m - n > (std::u32::MAX - delta) / (h + 1) {
            return Err(());
        }
        delta += (m - n) * (h + 1);

        n = m;

        for c in input {
            let c = *c as u32;
            if c < n {
                delta += 1;
            }
            else if c == n {
                let mut q = delta;

                for k in 1.. {
                    let k = k*BASE;

                    let t = clamped_sub(TMIN, k, bias, TMAX);

                    if q < t {
                        break;
                    }

                    output.push(encode_digit(t + (q - t) % (BASE - t)));

                    q = (q - t) / (BASE - t);
                }

                output.push(encode_digit(q));

                bias = adapt(delta, h+1, h == b);
                delta = 0;
                h += 1;
            }
        }

        delta += 1;
        n += 1;
    }

    Ok(output)
}

fn adapt(delta: u32, numpoint: u32, firsttime: bool) -> u32 {
    let mut delta = if firsttime {
        delta / DAMP
    }
    else {
        delta / 2
    };

    delta += delta / numpoint;
    let mut k = 0;

    while delta > (BASE - TMIN) * TMAX / 2 {
        delta /= BASE - TMIN;
        k += BASE
    }

    k + (BASE - TMIN + 1) * delta / (delta + SKEW)
}

/// Compute `lhs-rhs`. Result will be clamped in [min, max].
fn clamped_sub<T>(min: T, lhs: T, rhs: T, max: T) -> T
where T : Ord
        + std::ops::Add<Output=T>
        + std::ops::Sub<Output=T>
        + Copy
{
    if min + rhs >= lhs { min }
    else if max + rhs <= lhs { max }
    else { lhs - rhs }
}

fn decode_digit(c: char) -> u32 {
    let cp = c as u32;

    match c {
        '0' ... '9' => cp - ('0' as u32) + 26,
        'A' ... 'Z' => cp - ('A' as u32),
        'a' ... 'z' => cp - ('a' as u32),
        _ => BASE,
    }
}

fn encode_digit(d: u32) -> char {
    let r = (d + 22 + (if d < 26 { 75 } else { 0 })) as u8 as char;

    assert!(('0' <= r && r <= '9') || ('a' <= r && r <= 'z'), "r = {}", r);

    r
}

#[cfg(test)]
static TESTS: &'static [(&'static str, &'static str)] = &[
    // examples taken from [RCF-3492, section 7.1](https://tools.ietf.org/html/rfc3492#section-7.1)
    (&"\u{0644}\u{064A}\u{0647}\u{0645}\u{0627}\u{0628}\u{062A}\u{0643}\u{0644}\
       \u{0645}\u{0648}\u{0634}\u{0639}\u{0631}\u{0628}\u{064A}\u{061F}",
     &"egbpdaj6bu4bxfgehfvwxn"),

    (&"\u{4ED6}\u{4EEC}\u{4E3A}\u{4EC0}\u{4E48}\u{4E0D}\u{8BF4}\u{4E2D}\u{6587}",
     &"ihqwcrb4cv8a8dqg056pqjye"),

    (&"\u{4ED6}\u{5011}\u{7232}\u{4EC0}\u{9EBD}\u{4E0D}\u{8AAA}\u{4E2D}\u{6587}",
     &"ihqwctvzc91f659drss3x8bo0yb"),

    (&"\u{0050}\u{0072}\u{006F}\u{010D}\u{0070}\u{0072}\u{006F}\u{0073}\u{0074}\
       \u{011B}\u{006E}\u{0065}\u{006D}\u{006C}\u{0075}\u{0076}\u{00ED}\u{010D}\
       \u{0065}\u{0073}\u{006B}\u{0079}",
     &"Proprostnemluvesky-uyb24dma41a"),

    (&"\u{05DC}\u{05DE}\u{05D4}\u{05D4}\u{05DD}\u{05E4}\u{05E9}\u{05D5}\u{05D8}\
       \u{05DC}\u{05D0}\u{05DE}\u{05D3}\u{05D1}\u{05E8}\u{05D9}\u{05DD}\u{05E2}\
       \u{05D1}\u{05E8}\u{05D9}\u{05EA}",
     &"4dbcagdahymbxekheh6e0a7fei0b"),

    (&"\u{092F}\u{0939}\u{0932}\u{094B}\u{0917}\u{0939}\u{093F}\u{0928}\u{094D}\
       \u{0926}\u{0940}\u{0915}\u{094D}\u{092F}\u{094B}\u{0902}\u{0928}\u{0939}\
       \u{0940}\u{0902}\u{092C}\u{094B}\u{0932}\u{0938}\u{0915}\u{0924}\u{0947}\
       \u{0939}\u{0948}\u{0902}",
     &"i1baa7eci9glrd9b2ae1bj0hfcgg6iyaf8o0a1dig0cd"),

    (&"\u{306A}\u{305C}\u{307F}\u{3093}\u{306A}\u{65E5}\u{672C}\u{8A9E}\u{3092}\
       \u{8A71}\u{3057}\u{3066}\u{304F}\u{308C}\u{306A}\u{3044}\u{306E}\u{304B}",
     &"n8jok5ay5dzabd5bym9f0cm5685rrjetr6pdxa"),

    (&"\u{C138}\u{ACC4}\u{C758}\u{BAA8}\u{B4E0}\u{C0AC}\u{B78C}\u{B4E4}\u{C774}\
       \u{D55C}\u{AD6D}\u{C5B4}\u{B97C}\u{C774}\u{D574}\u{D55C}\u{B2E4}\u{BA74}\
       \u{C5BC}\u{B9C8}\u{B098}\u{C88B}\u{C744}\u{AE4C}",
     &"989aomsvi5e83db1d2a355cv1e0vak1dwrv93d5xbh15a0dt30a5jpsd879ccm6fea98c"),

    (&"\u{043F}\u{043E}\u{0447}\u{0435}\u{043C}\u{0443}\u{0436}\u{0435}\u{043E}\
       \u{043D}\u{0438}\u{043D}\u{0435}\u{0433}\u{043E}\u{0432}\u{043E}\u{0440}\
       \u{044F}\u{0442}\u{043F}\u{043E}\u{0440}\u{0443}\u{0441}\u{0441}\u{043A}\
       \u{0438}",
     &"b1abfaaepdrnnbgefbaDotcwatmq2g4l"),

    (&"\u{0050}\u{006F}\u{0072}\u{0071}\u{0075}\u{00E9}\u{006E}\u{006F}\u{0070}\
       \u{0075}\u{0065}\u{0064}\u{0065}\u{006E}\u{0073}\u{0069}\u{006D}\u{0070}\
       \u{006C}\u{0065}\u{006D}\u{0065}\u{006E}\u{0074}\u{0065}\u{0068}\u{0061}\
       \u{0062}\u{006C}\u{0061}\u{0072}\u{0065}\u{006E}\u{0045}\u{0073}\u{0070}\
       \u{0061}\u{00F1}\u{006F}\u{006C}",
     &"PorqunopuedensimplementehablarenEspaol-fmd56a"),

    (&"\u{0054}\u{1EA1}\u{0069}\u{0073}\u{0061}\u{006F}\u{0068}\u{1ECD}\u{006B}\
       \u{0068}\u{00F4}\u{006E}\u{0067}\u{0074}\u{0068}\u{1EC3}\u{0063}\u{0068}\
       \u{1EC9}\u{006E}\u{00F3}\u{0069}\u{0074}\u{0069}\u{1EBF}\u{006E}\u{0067}\
       \u{0056}\u{0069}\u{1EC7}\u{0074}",
     &"TisaohkhngthchnitingVit-kjcr8268qyxafd2f1b9g"),

    (&"\u{0033}\u{5E74}\u{0042}\u{7D44}\u{91D1}\u{516B}\u{5148}\u{751F}",
     &"3B-ww4c5e180e575a65lsy2b"),

    (&"\u{5B89}\u{5BA4}\u{5948}\u{7F8E}\u{6075}\u{002D}\u{0077}\u{0069}\u{0074}\
       \u{0068}\u{002D}\u{0053}\u{0055}\u{0050}\u{0045}\u{0052}\u{002D}\u{004D}\
       \u{004F}\u{004E}\u{004B}\u{0045}\u{0059}\u{0053}",
     &"-with-SUPER-MONKEYS-pc58ag80a8qai00g7n9n"),

    (&"\u{0048}\u{0065}\u{006C}\u{006C}\u{006F}\u{002D}\u{0041}\u{006E}\u{006F}\
       \u{0074}\u{0068}\u{0065}\u{0072}\u{002D}\u{0057}\u{0061}\u{0079}\u{002D}\
       \u{305D}\u{308C}\u{305E}\u{308C}\u{306E}\u{5834}\u{6240}",
     &"Hello-Another-Way--fc4qua05auwb3674vfr0b"),

    (&"\u{3072}\u{3068}\u{3064}\u{5C4B}\u{6839}\u{306E}\u{4E0B}\u{0032}",
     &"2-u9tlzr9756bt3uc0v"),

    (&"\u{004D}\u{0061}\u{006A}\u{0069}\u{3067}\u{004B}\u{006F}\u{0069}\u{3059}\
       \u{308B}\u{0035}\u{79D2}\u{524D}",
     &"MajiKoi5-783gue6qz075azm5e"),

    (&"\u{30D1}\u{30D5}\u{30A3}\u{30FC}\u{0064}\u{0065}\u{30EB}\u{30F3}\u{30D0}",
     &"de-jg4avhby1noc0d"),

    (&"\u{305D}\u{306E}\u{30B9}\u{30D4}\u{30FC}\u{30C9}\u{3067}",
     &"d9juau41awczczp"),

    (&"\u{002D}\u{003E}\u{0020}\u{0024}\u{0031}\u{002E}\u{0030}\u{0030}\u{0020}\
       \u{003C}\u{002D}",
     &"-> $1.00 <--"),

     // some real-life examples
     (&"académie-française", &"acadmie-franaise-npb1a"),
     (&"bücher", &"bcher-kva"),
     (&"république-numérique", &"rpublique-numrique-bwbm"),

     // some real-life TLD
     (&"бг",       &"90ae"),
     (&"рф",       &"p1ai"),
     (&"укр",      &"j1amh"),
     (&"السعودية", &"mgberp4a5d4ar"),
     (&"امارات",   &"mgbaam7a8h"),
     (&"مصر",      &"wgbh1c"),
     (&"中国",     &"fiqs8s"),
     (&"中國",     &"fiqz9s"),
     (&"台湾",     &"kprw13d"),
     (&"台灣",     &"kpry57d"),
     (&"香港",     &"j6w193g"),

     // other
     (&"", &""),
     (&"a", &"a-"),
     (&"0", &"0-"),
     (&"A", &"A-"),
     (&"é", &"9ca"),
     (&"\n", &"\n-"),
];

#[test]
fn test_decode() {
    for t in TESTS {
        assert_eq!(decode(&t.1), Ok(t.0.into()));
    }
}

#[test]
fn test_encode() {
    for t in TESTS {
        assert_eq!(encode(t.0).unwrap().to_lowercase(), t.1.to_lowercase());
    }
}

#[test]
fn test_fail_decode() {
    assert_eq!(decode(&"bcher-kva.ch"), Err(()));
    assert_eq!(decode(&"+"), Err(()));
    assert_eq!(decode(&"\\"), Err(()));
    assert_eq!(decode(&"é"), Err(()));
    assert_eq!(decode(&"99999999"), Err(()));
}
