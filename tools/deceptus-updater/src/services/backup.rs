use anyhow::{bail, Context, Result};
use serde::Deserialize;
use std::collections::BTreeSet;
use std::fs;
use std::path::{Path, PathBuf};

/// Accept both the new unified manifest and the legacy split form.
#[derive(Debug, Deserialize)]
#[serde(untagged)]
enum Manifest {
    Unified { entries: Vec<String> },
    Split {
        #[serde(default)]
        files: Vec<String>,
        #[serde(default)]
        data_folders: Vec<String>,
    },
}

impl Manifest {
    fn into_entries(self) -> Vec<String> {
        match self {
            Manifest::Unified { entries } => entries,
            Manifest::Split { files, data_folders } => {
                let mut v = files;
                for df in data_folders {
                    // legacy data_folders were relative to data/, normalize to a single list
                    v.push(format!("data/{}", df.trim_matches('/').trim_matches('\\')));
                }
                v
            }
        }
    }
}

pub struct Backup {
    root: PathBuf, // backups root (e.g., "backups")
}

impl Backup {
    pub fn new(root: impl Into<PathBuf>) -> Self {
        Self { root: root.into() }
    }

    /// Back up all entries listed in the manifest JSON.
    /// Compatible with both the old and new manifest structures.
    pub fn backup_from_json(&self, json_path: PathBuf) -> Result<PathBuf> {
        if !json_path.exists() {
            bail!("file list not found: {}", json_path.display());
        }
        let raw = fs::read_to_string(&json_path)
            .with_context(|| format!("reading {}", json_path.display()))?;
        let manifest: Manifest = serde_json::from_str(&raw)
            .with_context(|| "parsing JSON file list (accepts {\"entries\": [...]} or legacy {\"files\":[],\"data_folders\":[]})")?;

        // Normalize + dedupe
        let mut set = BTreeSet::<String>::new();
        for s in manifest.into_entries() {
            let n = normalize_rel(&s);
            if !n.is_empty() {
                set.insert(n);
            }
        }
        if set.is_empty() {
            eprintln!("warn: manifest has no entries after normalization/dedup.");
            // Still create an empty dated folder so the caller sees a path
            let empty_dir = self.make_dated_folder()?;
            return Ok(empty_dir);
        }

        let backup_dir = self.make_dated_folder()?;

        for rel in set {
            let rel_path = PathBuf::from(&rel);
            let src = PathBuf::from(&rel_path); // relative to current working dir
            if !src.exists() {
                eprintln!("warn: skipping missing path: {}", src.display());
                continue;
            }
            let dst = backup_dir.join(&rel_path);

            if src.is_dir() {
                backup_dir_move(&src, &dst)
                    .with_context(|| format!("backing up dir {}", src.display()))?;
            } else {
                backup_file_move(&src, &dst)
                    .with_context(|| format!("backing up file {}", src.display()))?;
            }
        }

        Ok(backup_dir)
    }

    fn make_dated_folder(&self) -> Result<PathBuf> {
        use chrono::{Datelike, Local};
        let now = Local::now();
        let date = format!("{:04}{:02}{:02}", now.year(), now.month(), now.day());
        let dir = self.root.join(format!("backup-{}", date));
        fs::create_dir_all(&dir)
            .with_context(|| format!("creating backup dir {}", dir.display()))?;
        Ok(dir)
    }
}

/* ---------- helpers ---------- */

/// Normalize a user-specified relative path:
/// - trim whitespace
/// - drop leading "./"
/// - convert '\' -> '/'
/// - trim trailing '/' (but keep root names)
fn normalize_rel(s: &str) -> String {
    let mut t = s.trim().replace('\\', "/");
    if t.starts_with("./") {
        t = t.trim_start_matches("./").to_string();
    }
    if t.ends_with('/') && t.len() > 1 {
        t.pop();
    }
    t
}

/// Move a single file into backup, creating parent dirs.
/// Prefer rename; if it fails (e.g., cross-device), copy then remove.
fn backup_file_move(src: &Path, dst: &Path) -> Result<()> {
    if let Some(parent) = dst.parent() {
        fs::create_dir_all(parent)?;
    }
    match fs::rename(src, dst) {
        Ok(()) => Ok(()),
        Err(_) => {
            fs::copy(src, dst)
                .with_context(|| format!("copy {} -> {}", src.display(), dst.display()))?;
            fs::remove_file(src)
                .with_context(|| format!("remove {}", src.display()))?;
            Ok(())
        }
    }
}

/// Move a directory tree into backup.
/// Try rename; if that fails, copy recursively then remove the source.
fn backup_dir_move(src_dir: &Path, dst_dir: &Path) -> Result<()> {
    if let Some(parent) = dst_dir.parent() {
        fs::create_dir_all(parent)?;
    }
    match fs::rename(src_dir, dst_dir) {
        Ok(()) => Ok(()),
        Err(_) => {
            copy_dir_all(src_dir, dst_dir)?;
            fs::remove_dir_all(src_dir)
                .with_context(|| format!("remove {}", src_dir.display()))?;
            Ok(())
        }
    }
}

/// Recursive copy preserving structure.
fn copy_dir_all(src: &Path, dst: &Path) -> Result<()> {
    fs::create_dir_all(dst)?;
    for entry in fs::read_dir(src)? {
        let entry = entry?;
        let ty = entry.file_type()?;
        let from = entry.path();
        let to = dst.join(entry.file_name());
        if ty.is_dir() {
            copy_dir_all(&from, &to)?;
        } else {
            if let Some(parent) = to.parent() {
                fs::create_dir_all(parent)?;
            }
            fs::copy(&from, &to)
                .with_context(|| format!("copy {} -> {}", from.display(), to.display()))?;
        }
    }
    Ok(())
}
