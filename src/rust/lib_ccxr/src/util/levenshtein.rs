use std::cmp::min;

/// Calculates the levenshtein distance between two slices.
///
/// # Examples
/// ```rust
/// # use lib_ccxr::util::levenshtein;
/// assert_eq!(levenshtein(&[1,2,3,4,5], &[1,3,2,4,5,6]), 3);
/// ```
pub fn levenshtein<T: Copy + PartialEq>(a: &[T], b: &[T]) -> usize {
    let mut column: Vec<usize> = (0..).take(a.len() + 1).collect();

    for x in 1..=b.len() {
        column[0] = x;
        let mut lastdiag = x - 1;
        for y in 1..=a.len() {
            let olddiag = column[y];
            column[y] = min(
                min(column[y] + 1, column[y - 1] + 1),
                lastdiag + (if a[y - 1] == b[x - 1] { 0 } else { 1 }),
            );
            lastdiag = olddiag;
        }
    }

    column[a.len()]
}
