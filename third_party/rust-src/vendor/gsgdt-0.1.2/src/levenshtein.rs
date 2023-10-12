use std::cmp::min;

/// Calculate the levenshtein distance between two strings.
pub(crate) fn distance(s1: &str, s2: &str) -> usize {
    let v1: Vec<char> = s1.chars().collect();
    let v2: Vec<char> = s2.chars().collect();

    let l_v1 = v1.len();
    let l_v2 = v2.len();

    if l_v1 == 0 {
        return l_v2;
    }
    if l_v2 == 0 {
        return l_v1;
    }
    if l_v1 > l_v2 {
        return distance(s2, s1);
    }

    let mut col: Vec<usize> = (0..=l_v1).collect();

    for i in 1..=l_v2 {
        let mut last_diag = col[0];
        col[0] += 1;
        for j in 1..=l_v1 {
            let last_diag_temp = col[j];
            if v1[j-1] == v2[i-1] {
                col[j] = last_diag;
            } else {
                col[j] = min(last_diag, min(col[j-1], col[j])) + 1;
            }
            last_diag = last_diag_temp;
        }
    }

    col[l_v1]
}


#[cfg(test)]
mod tests {
    use crate::levenshtein::*;
    #[test]
    fn test1() {
        assert_eq!(distance("kitten", "sitting"), 3);
    }

    #[test]
    fn test_diff_len() {
        assert_eq!(distance("kit", "kitten"), 3);
    }

    #[test]
    fn test_equal() {
        assert_eq!(distance("test", "test"), 0);
        assert_eq!(distance("", ""), 0);
        assert_eq!(distance("long string with space", "long string with space"), 0);
        assert_eq!(distance("unicode 😀", "unicode 😀"), 0);
    }

    #[test]
    fn test_one_empty() {
        assert_eq!(distance("test", ""), 4);
        assert_eq!(distance("", "test"), 4);
        assert_eq!(distance("", ""), 0);
        assert_eq!(distance("long string", ""), 11);
        assert_eq!(distance("😀", ""), 1);
    }
}
