cfg_if::cfg_if! {
    if #[cfg(all(
        target_feature = "sse2",
        any(target_arch = "x86", target_arch = "x86_64"),
        not(miri),
        not(feature = "no_simd"),
    ))] {
        mod sse2;
        use sse2 as imp;
    } else {
        mod no_simd;
        use no_simd as imp;
    }
}

pub use imp::GroupQuery;
pub use imp::GROUP_SIZE;

// While GROUP_SIZE is target-dependent for performance reasons,
// we always do probing as if it was the same as REFERENCE_GROUP_SIZE.
// This way the same slot indices will be assigned to the same
// entries no matter the underlying target. This allows the
// binary format of the table to be portable.
pub const REFERENCE_GROUP_SIZE: usize = 16;

#[cfg(test)]
mod tests {
    use super::*;

    const EMPTY_GROUP: [u8; GROUP_SIZE] = [255; GROUP_SIZE];

    fn full_group() -> [u8; GROUP_SIZE] {
        let mut group = [0; GROUP_SIZE];
        for i in 0..group.len() {
            group[i] = i as u8;
        }
        group
    }

    #[test]
    fn full() {
        let mut q = GroupQuery::from(&full_group(), 42);

        assert_eq!(Iterator::count(&mut q), 0);
        assert!(!q.any_empty());
        assert_eq!(q.first_empty(), None);
    }

    #[test]
    fn all_empty() {
        let mut q = GroupQuery::from(&EMPTY_GROUP, 31);

        assert_eq!(Iterator::count(&mut q), 0);
        assert!(q.any_empty());
        assert_eq!(q.first_empty(), Some(0));
    }

    #[test]
    fn partially_filled() {
        for filled_up_to_index in 0..=(GROUP_SIZE - 2) {
            let mut group = EMPTY_GROUP;

            for i in 0..=filled_up_to_index {
                group[i] = 42;
            }

            let mut q = GroupQuery::from(&group, 77);

            assert_eq!(Iterator::count(&mut q), 0);
            assert!(q.any_empty());
            assert_eq!(q.first_empty(), Some(filled_up_to_index + 1));
        }
    }

    #[test]
    fn match_iter() {
        let expected: Vec<_> = (0..GROUP_SIZE).filter(|x| x % 3 == 0).collect();

        let mut group = full_group();

        for i in &expected {
            group[*i] = 103;
        }

        let mut q = GroupQuery::from(&group, 103);

        let matches: Vec<usize> = (&mut q).collect();

        assert_eq!(matches, expected);
        assert!(!q.any_empty());
        assert_eq!(q.first_empty(), None);
    }

    #[test]
    fn match_iter_with_empty() {
        let expected: Vec<_> = (0..GROUP_SIZE).filter(|x| x % 3 == 2).collect();

        let mut group = full_group();

        for i in &expected {
            group[*i] = 99;
        }

        // Clear a few slots
        group[3] = 255;
        group[4] = 255;
        group[GROUP_SIZE - 1] = 255;

        let mut q = GroupQuery::from(&group, 99);

        let matches: Vec<usize> = (&mut q).collect();

        assert_eq!(matches, expected);
        assert!(q.any_empty());
        assert_eq!(q.first_empty(), Some(3));
    }
}
