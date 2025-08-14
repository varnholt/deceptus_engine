use anyhow::{bail, Context, Result};
use serde::Deserialize;
use std::collections::BTreeSet;
use std::fs;
use std::path::{Path, PathBuf};

/// Unified manifest: one list of relative paths, each either a file or a directory.
/// Directories are always copied recursively.
///
/// Back-compat: we also accept the older split form:
/// { "files": [...], "data_folders": [...] }  -> merged into one list.
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
                // convert split style into paths relative to root: data/<folder>
                for df in data_folders {
                    v.push(format!("data/{}", df.trim_matches('/').trim_matches('\\')));
                }
                v
            }
        }
    }
}

/// Installs (replaces) paths from an extracted build tree into the working directory:
/// - files are atomically replaced (copy to temp then rename)
/// - directories are copied recursively to a temp dir then swapped in
pub struct Installer;

impl Default for Installer {
    fn default() -> Self { Self }
}

impl Installer {
    /// `json_path`      – manifest json with either { entries:[...]} or the legacy split form
    /// `extracted_root` – e.g., ./.tmp/deceptus_extract_*
    /// `install_root`   – usually current working directory
    pub fn install_list(&self, json_path: &Path, extracted_root: &Path, install_root: &Path) -> Result<()> {
        if !json_path.exists() {
            bail!("manifest not found: {}", json_path.display());
        }
        let raw = fs::read_to_string(json_path)
            .with_context(|| format!("reading {}", json_path.display()))?;
        let manifest: Manifest = serde_json::from_str(&raw)
            .with_context(|| "parsing manifest")?;

        // 1) load, normalize, and deduplicate entries
        let mut set = BTreeSet::<String>::new();
        for raw in manifest.into_entries() {
            let norm = normalize_rel(&raw);
            if !norm.is_empty() {
                set.insert(norm);
            }
        }
        if set.is_empty() {
            eprintln!("warn: manifest has no entries after normalization/dedup.");
            return Ok(());
        }

        // 2) install each entry
        for rel in set {
            let rel_path = PathBuf::from(&rel);
            let src = extracted_root.join(&rel_path);
            if !src.exists() {
                eprintln!("warn: missing source path in extracted build: {}", src.display());
                continue;
            }
            let dst = install_root.join(&rel_path);

            if src.is_dir() {
                self.atomic_dir_replace(&src, &dst)
                    .with_context(|| format!("installing directory {}", rel_path.display()))?;
                println!("installed dir {}", rel_path.display());
            } else {
                self.atomic_file_replace(&src, &dst)
                    .with_context(|| format!("installing file {}", rel_path.display()))?;
                println!("installed file {}", rel_path.display());
            }
        }

        Ok(())
    }

    /// Copy to `<dst>.tmp-installing`, then remove old and rename into place.
    fn atomic_file_replace(&self, src: &Path, dst: &Path) -> Result<()> {
        if let Some(parent) = dst.parent() {
            fs::create_dir_all(parent)?;
        }
        let tmp = dst.with_extension("tmp-installing");
        if tmp.exists() { let _ = fs::remove_file(&tmp); }
        fs::copy(src, &tmp).with_context(|| format!("copy {} -> {}", src.display(), tmp.display()))?;

        if dst.exists() {
            fs::remove_file(dst).with_context(|| format!("remove old {}", dst.display()))?;
        }
        fs::rename(&tmp, dst).with_context(|| format!("activate {}", dst.display()))?;
        Ok(())
    }

    /// Copy directory to `<dst>.tmp-installing-dir`, then remove old dir and rename.
    fn atomic_dir_replace(&self, src_dir: &Path, dst_dir: &Path) -> Result<()> {
        if let Some(parent) = dst_dir.parent() {
            fs::create_dir_all(parent)?;
        }
        let tmp_dir = dst_dir.with_extension("tmp-installing-dir");
        if tmp_dir.exists() {
            fs::remove_dir_all(&tmp_dir).with_context(|| format!("remove {}", tmp_dir.display()))?;
        }
        copy_dir_all(src_dir, &tmp_dir)
            .with_context(|| format!("copy dir {} -> {}", src_dir.display(), tmp_dir.display()))?;

        if dst_dir.exists() {
            fs::remove_dir_all(dst_dir)
                .with_context(|| format!("remove old {}", dst_dir.display()))?;
        }
        fs::rename(&tmp_dir, dst_dir)
            .with_context(|| format!("activate {}", dst_dir.display()))?;
        Ok(())
    }
}

/// Recursive copy preserving tree. (Basic; extend if you need perms/timestamps.)
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
            if let Some(parent) = to.parent() { fs::create_dir_all(parent)?; }
            fs::copy(&from, &to)
                .with_context(|| format!("copy {} -> {}", from.display(), to.display()))?;
        }
    }
    Ok(())
}

/// Normalize a user-specified relative path:
/// - trim whitespace
/// - drop leading "./"
/// - convert '\' -> '/'
/// - trim trailing '/' (but keep root names)
fn normalize_rel(s: &str) -> String {
    let mut t = s.trim().replace('\\', "/");
    if t.starts_with("./") { t = t.trim_start_matches("./").to_string(); }
    if t.ends_with('/') && t.len() > 1 { t.pop(); }
    t
}
