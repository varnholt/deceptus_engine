use anyhow::{bail, Context, Result};
use serde::Deserialize;
use std::fs;
use std::path::{Path, PathBuf};

/// Same JSON shape as used by `Backup`:
/// { "files": ["deceptus.exe", "data/config.json", "plugins/example.dll"] }
#[derive(Debug, Deserialize)]
struct FileList {
    files: Vec<String>,
}

/// Tiny service that installs (replaces) the listed files from an extracted build tree
/// into your working install directory. It only touches files named in the JSON.
pub struct Installer;

impl Default for Installer {
    fn default() -> Self { Self }
}

impl Installer {
    /// `json_path`     – path to files.json
    /// `extracted_root`– directory where the ZIP was extracted (e.g., ./.tmp/deceptus_extract_*)
    /// `install_root`  – usually the current working dir; pass `std::env::current_dir()?` if you want
    pub fn install_list(&self, json_path: &Path, extracted_root: &Path, install_root: &Path) -> Result<()> {
        if !json_path.exists() {
            bail!("file list not found: {}", json_path.display());
        }
        let raw = fs::read_to_string(json_path)
            .with_context(|| format!("reading {}", json_path.display()))?;
        let list: FileList = serde_json::from_str(&raw)
            .with_context(|| "parsing JSON file list (expect { \"files\": [...] })")?;

        for rel in list.files {
            let rel_path = PathBuf::from(&rel);

            // source inside extracted tree
            let src = extracted_root.join(&rel_path);
            if !src.exists() {
                eprintln!("warn: source not found in extracted build: {}", src.display());
                continue;
            }

            // destination in live install
            let dst = install_root.join(&rel_path);
            if let Some(parent) = dst.parent() {
                fs::create_dir_all(parent)
                    .with_context(|| format!("creating {}", parent.display()))?;
            }

            // copy to a side-by-side temp, then swap in (reduces partial-write risks)
            let tmp_dst = dst.with_extension("tmp-installing");
            if tmp_dst.exists() {
                let _ = fs::remove_file(&tmp_dst);
            }
            fs::copy(&src, &tmp_dst)
                .with_context(|| format!("copy {} -> {}", src.display(), tmp_dst.display()))?;

            // remove old file if present (Windows rename won’t overwrite)
            if dst.exists() {
                fs::remove_file(&dst)
                    .with_context(|| format!("removing {}", dst.display()))?;
            }
            fs::rename(&tmp_dst, &dst)
                .with_context(|| format!("activating {}", dst.display()))?;

            println!("installed {}", rel_path.display());
        }

        Ok(())
    }
}
