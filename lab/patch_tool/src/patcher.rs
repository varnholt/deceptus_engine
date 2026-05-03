use std::path::Path;

// ─── types ───────────────────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct FilePatch {
    pub filename: String,
    pub hunks: Vec<Hunk>,
}

#[derive(Debug, Clone)]
pub struct Hunk {
    lines: Vec<HunkLine>,
}

#[derive(Debug, Clone)]
enum HunkLine {
    Context(String),
    Removal(String),
    Addition(String),
}

#[derive(Debug, Clone, PartialEq)]
pub enum HunkResult {
    Applied,
    AlreadyApplied,
    Failed(String),
}

#[derive(Debug)]
pub struct ApplyReport {
    pub filename: String,
    pub hunk_results: Vec<HunkResult>,
}

// ─── parsing ─────────────────────────────────────────────────────────────────

pub fn parse_diff(content: &str) -> Vec<FilePatch> {
    let mut patches: Vec<FilePatch> = Vec::new();
    let mut current_file: Option<FilePatch> = None;
    let mut current_hunk: Option<Hunk> = None;
    let mut saw_minus_header = false;

    for line in content.lines() {
        if line.starts_with("--- ") {
            flush_hunk(&mut current_hunk, &mut current_file);
            flush_file(&mut current_file, &mut patches);
            saw_minus_header = true;
        } else if line.starts_with("+++ ") && saw_minus_header {
            saw_minus_header = false;
            current_file = Some(FilePatch {
                filename: extract_filename(line),
                hunks: Vec::new(),
            });
        } else if line.starts_with("@@ ") {
            saw_minus_header = false;
            flush_hunk(&mut current_hunk, &mut current_file);
            current_hunk = Some(Hunk { lines: Vec::new() });
        } else if let Some(ref mut hunk) = current_hunk {
            saw_minus_header = false;
            // a bare empty line in a hunk is a context line with empty content
            if line.starts_with('-') {
                hunk.lines.push(HunkLine::Removal(line[1..].to_string()));
            } else if line.starts_with('+') {
                hunk.lines.push(HunkLine::Addition(line[1..].to_string()));
            } else if line.starts_with(' ') || line.is_empty() {
                hunk.lines.push(HunkLine::Context(line[1..].to_string()));
            }
            // "\ No newline at end of file" and other special markers are ignored
        } else {
            saw_minus_header = false;
        }
    }

    flush_hunk(&mut current_hunk, &mut current_file);
    flush_file(&mut current_file, &mut patches);
    patches
}

fn flush_hunk(current_hunk: &mut Option<Hunk>, current_file: &mut Option<FilePatch>) {
    if let (Some(hunk), Some(file)) = (current_hunk.take(), current_file.as_mut()) {
        file.hunks.push(hunk);
    }
}

fn flush_file(current_file: &mut Option<FilePatch>, patches: &mut Vec<FilePatch>) {
    if let Some(file) = current_file.take() {
        if !file.hunks.is_empty() {
            patches.push(file);
        }
    }
}

fn extract_filename(line: &str) -> String {
    // "+++ b/data/catacombs/foo.tmx\t..." → "foo.tmx"
    let without_prefix = line.trim_start_matches('+').trim_start_matches('-').trim();
    // strip the git a/ b/ prefix
    let without_ab = if without_prefix.starts_with("a/") || without_prefix.starts_with("b/") {
        &without_prefix[2..]
    } else {
        without_prefix
    };
    // strip optional tab + timestamp that some diff tools append
    let path_str = without_ab.split('\t').next().unwrap_or(without_ab);
    Path::new(path_str)
        .file_name()
        .and_then(|name| name.to_str())
        .unwrap_or(path_str)
        .to_string()
}

// ─── application ─────────────────────────────────────────────────────────────

pub fn apply_patch_to_dir(target_dir: &Path, patch: &FilePatch) -> ApplyReport {
    let file_path = target_dir.join(&patch.filename);

    let content = match std::fs::read_to_string(&file_path) {
        Ok(content) => content,
        Err(error) => {
            return ApplyReport {
                filename: patch.filename.clone(),
                hunk_results: vec![HunkResult::Failed(format!(
                    "Cannot read '{}': {error}",
                    patch.filename
                ))],
            };
        }
    };

    let line_ending = if content.contains("\r\n") { "\r\n" } else { "\n" };
    let had_trailing_newline = content.ends_with('\n');

    let mut file_lines: Vec<String> = content.lines().map(String::from).collect();
    let mut hunk_results: Vec<HunkResult> = Vec::new();

    for hunk in &patch.hunks {
        hunk_results.push(apply_hunk_fuzzy(&mut file_lines, hunk));
    }

    let any_applied = hunk_results.iter().any(|result| *result == HunkResult::Applied);
    let all_ok = hunk_results
        .iter()
        .all(|result| matches!(result, HunkResult::Applied | HunkResult::AlreadyApplied));

    if all_ok && any_applied {
        let backup_path = format!("{}.bak", file_path.to_string_lossy());
        let _ = std::fs::copy(&file_path, &backup_path);

        let mut new_content = file_lines.join(line_ending);
        if had_trailing_newline {
            new_content.push_str(line_ending);
        }

        if let Err(error) = std::fs::write(&file_path, new_content) {
            return ApplyReport {
                filename: patch.filename.clone(),
                hunk_results: vec![HunkResult::Failed(format!("Write failed: {error}"))],
            };
        }
    }

    ApplyReport { filename: patch.filename.clone(), hunk_results }
}

// Fuzzy apply: ignore line numbers entirely, search by content.
//
// search      = all Context + Removal lines in hunk order  (what the file should contain now)
// replacement = all Context + Addition lines in hunk order (what it should contain after)
//
// Find search sequence anywhere in the file, splice in replacement.
// If not found, check whether replacement is already present → already applied.
fn apply_hunk_fuzzy(file_lines: &mut Vec<String>, hunk: &Hunk) -> HunkResult {
    let search: Vec<String> = hunk
        .lines
        .iter()
        .filter_map(|hunk_line| match hunk_line {
            HunkLine::Context(content) | HunkLine::Removal(content) => Some(content.clone()),
            HunkLine::Addition(_) => None,
        })
        .collect();

    let replacement: Vec<String> = hunk
        .lines
        .iter()
        .filter_map(|hunk_line| match hunk_line {
            HunkLine::Context(content) | HunkLine::Addition(content) => Some(content.clone()),
            HunkLine::Removal(_) => None,
        })
        .collect();

    if search.is_empty() {
        return HunkResult::Failed("Hunk has no context or removal lines to search for".into());
    }

    if let Some(match_start) = find_sequence(file_lines, &search) {
        file_lines.splice(match_start..match_start + search.len(), replacement);
        return HunkResult::Applied;
    }

    // Not found as "before" state — check if the "after" state is already there.
    if !replacement.is_empty() && find_sequence(file_lines, &replacement).is_some() {
        return HunkResult::AlreadyApplied;
    }

    HunkResult::Failed(format!(
        "Context not found in file (first search line: {:?})",
        search.first().map(String::as_str).unwrap_or("")
    ))
}

fn find_sequence(haystack: &[String], needle: &[String]) -> Option<usize> {
    if needle.is_empty() || needle.len() > haystack.len() {
        return None;
    }
    'candidate: for start in 0..=(haystack.len() - needle.len()) {
        for (offset, needle_line) in needle.iter().enumerate() {
            if haystack[start + offset] != *needle_line {
                continue 'candidate;
            }
        }
        return Some(start);
    }
    None
}

// ─── tests ────────────────────────────────────────────────────────────────────

#[cfg(test)]
mod tests {
    use super::*;

    fn make_lines(text: &str) -> Vec<String> {
        text.lines().map(String::from).collect()
    }

    // Use concat! so that context lines keep their leading space.
    // The \n\ continuation idiom eats that space, which breaks parsing.

    #[test]
    fn test_addition_only_hunk() {
        let diff = concat!(
            "--- a/foo.txt\n",
            "+++ b/foo.txt\n",
            "@@ -1,3 +1,4 @@\n",
            " line_a\n",
            " line_b\n",
            "+new_line\n",
            " line_c\n",
        );

        let patches = parse_diff(diff);
        assert_eq!(patches.len(), 1);

        let mut lines = make_lines("line_a\nline_b\nline_c\n");
        let result = apply_hunk_fuzzy(&mut lines, &patches[0].hunks[0]);
        assert_eq!(result, HunkResult::Applied);
        assert_eq!(lines, make_lines("line_a\nline_b\nnew_line\nline_c\n"));
    }

    #[test]
    fn test_removal_only_hunk() {
        let diff = concat!(
            "--- a/foo.txt\n",
            "+++ b/foo.txt\n",
            "@@ -1,4 +1,3 @@\n",
            " line_a\n",
            " line_b\n",
            "-old_line\n",
            " line_c\n",
        );

        let patches = parse_diff(diff);
        let mut lines = make_lines("line_a\nline_b\nold_line\nline_c\n");
        let result = apply_hunk_fuzzy(&mut lines, &patches[0].hunks[0]);
        assert_eq!(result, HunkResult::Applied);
        assert_eq!(lines, make_lines("line_a\nline_b\nline_c\n"));
    }

    #[test]
    fn test_already_applied() {
        let diff = concat!(
            "--- a/foo.txt\n",
            "+++ b/foo.txt\n",
            "@@ -1,3 +1,4 @@\n",
            " line_a\n",
            " line_b\n",
            "+new_line\n",
            " line_c\n",
        );

        let patches = parse_diff(diff);
        let mut lines = make_lines("line_a\nline_b\nnew_line\nline_c\n");
        let result = apply_hunk_fuzzy(&mut lines, &patches[0].hunks[0]);
        assert_eq!(result, HunkResult::AlreadyApplied);
    }

    #[test]
    fn test_hunk_found_at_shifted_position() {
        let diff = concat!(
            "--- a/foo.txt\n",
            "+++ b/foo.txt\n",
            "@@ -1,3 +1,4 @@\n",
            " line_a\n",
            "-old\n",
            "+new\n",
            " line_b\n",
        );

        let patches = parse_diff(diff);
        // The block has moved: extra line prepended
        let mut lines = make_lines("EXTRA\nline_a\nold\nline_b\n");
        let result = apply_hunk_fuzzy(&mut lines, &patches[0].hunks[0]);
        assert_eq!(result, HunkResult::Applied);
        assert_eq!(lines, make_lines("EXTRA\nline_a\nnew\nline_b\n"));
    }
}
