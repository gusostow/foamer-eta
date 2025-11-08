fn slice(s: &str, range: std::ops::Range<usize>) -> Option<String> {
    let out: String = s
        .chars()
        .skip(range.start)
        .take(range.end - range.start)
        .collect();
    if out.is_empty() { None } else { Some(out) }
}

/// Implement line wrapping by word on the server-side to avoid cpp segfaults on the device.
pub fn lines(s: String, max_width: usize) -> Vec<String> {
    // Written the old-fashioned way: by a human, poorly
    let mut lines = Vec::new();
    let mut current = String::new();
    for word in s.split_whitespace() {
        let word_len = word.chars().count();
        if word_len <= max_width {
            if current.chars().count() + word_len < max_width {
                if current.chars().count() != 0 {
                    current.push(' ');
                }
                current.push_str(word);
            } else {
                lines.push(current);
                current = String::from(word);
            }
        } else {
            if current.chars().count() > max_width - 1 {
                lines.push(current);
                current = String::new();
            } else if !current.is_empty() {
                current.push(' ');
            }
            let mut start: usize = 0;
            let mut stop: usize = max_width - current.chars().count();
            while let Some(chunk) = slice(word, start..stop) {
                start = stop;
                stop = start + max_width;
                current.push_str(&chunk);
                if current.chars().count() == max_width {
                    lines.push(current);
                    current = String::new();
                }
            }
        }
    }
    if !current.is_empty() {
        lines.push(current)
    }
    lines
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_lines() {
        assert_eq!(
            lines("This is a multiline string".to_string(), 16),
            Vec::from(["This is a", "multiline string"])
        );
        assert_eq!(
            lines("a 0123456789012345678 b".to_string(), 16),
            Vec::from(["a 01234567890123", "45678 b"])
        );
        assert_eq!(lines("1234".to_string(), 3), Vec::from(["123", "4"]));
        assert_eq!(
            lines("012345678901234 a".to_string(), 16),
            Vec::from(["012345678901234", "a"])
        );
    }
}
