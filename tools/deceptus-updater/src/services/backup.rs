use anyhow::{bail, Context, Result};
use chrono::{Datelike, Local};
use serde::Deserialize;
use std::fs;
use std::path::{Path, PathBuf};

#[derive(Debug, Deserialize)]
struct FileList {
    files: Vec<String>,
}

pub struct Backup {
    root: PathBuf,
}

impl Backup {
    pub fn new<P: Into<PathBuf>>(root: P) -> Self {
        Self { root: root.into() }
    }

    /// JSON format:
    /// { "files": ["deceptus.exe", "data/config.json", "plugin.dll"] }
    pub fn backup_from_json(&self, json_path: PathBuf) -> Result<PathBuf> {
        if !json_path.exists() {
            bail!("file list not found: {}", json_path.display());
        }
        let raw = fs::read_to_string(&json_path)
            .with_context(|| format!("reading {}", json_path.display()))?;
        let list: FileList = serde_json::from_str(&raw)
            .with_context(|| "parsing JSON file list (expect { \"files\": [...] })")?;

        let backup_dir = self.make_dated_folder()?;
        for item in list.files {
            let src = PathBuf::from(&item);
            if !src.exists() {
                eprintln!("warn: skipping missing file {}", src.display());
                continue;
            }
            self.move_into(&src, &backup_dir)
                .with_context(|| format!("moving {}", src.display()))?;
        }
        Ok(backup_dir)
    }

    fn make_dated_folder(&self) -> Result<PathBuf> {
        let now = Local::now();
        let date = format!("{:04}{:02}{:02}", now.year(), now.month(), now.day());
        let dir = self.root.join(format!("backup-{date}"));
        fs::create_dir_all(&dir).with_context(|| format!("creating {}", dir.display()))?;
        Ok(dir)
    }

    fn move_into(&self, src: &Path, backup_dir: &Path) -> Result<()> {
        let file_name = src.file_name().ok_or_else(|| anyhow::anyhow!("invalid filename"))?;
        let mut dest = backup_dir.join(file_name);

        // avoid overwrite: append -N if needed
        let mut counter = 1;
        while dest.exists() {
            let stem = dest.file_stem().and_then(|s| s.to_str()).unwrap_or("file");
            let ext = dest.extension().and_then(|e| e.to_str()).unwrap_or("");
            let candidate = if ext.is_empty() {
                format!("{stem}-{counter}")
            } else {
                format!("{stem}-{counter}.{ext}")
            };
            dest = backup_dir.join(candidate);
            counter += 1;
        }

        if let Some(parent) = dest.parent() { fs::create_dir_all(parent)?; }

        // try rename; fall back to copy+remove for cross-device moves
        fs::rename(src, &dest).or_else(|_| {
            fs::copy(src, &dest).and_then(|_| fs::remove_file(src))
        }).with_context(|| format!("moving into {}", dest.display()))?;

        Ok(())
    }
}
