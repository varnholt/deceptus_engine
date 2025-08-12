use anyhow::{Context, Result};
use std::fs::{self, File};
use std::path::{Path, PathBuf};
use tempfile::tempdir;
use zip::ZipArchive;

pub struct Extractor;

impl Default for Extractor {
    fn default() -> Self { Self }
}

impl Extractor {
    /// Extracts zip to a temp folder and returns the path.
    pub fn extract_zip(&self, zip_path: &Path) -> Result<PathBuf> {
        let file = File::open(zip_path)
            .with_context(|| format!("opening {}", zip_path.display()))?;
        let mut archive = ZipArchive::new(file).context("reading zip")?;

        let tmpdir = tempdir()?;
        let out_dir = tmpdir.path().join("extracted");
        fs::create_dir_all(&out_dir)?;

        for i in 0..archive.len() {
            let mut entry = archive.by_index(i)?;
            let rel = entry.mangled_name(); // sanitized path
            let out_path = out_dir.join(rel);

            if entry.is_dir() {
                fs::create_dir_all(&out_path)?;
            } else {
                if let Some(parent) = out_path.parent() {
                    fs::create_dir_all(parent)?;
                }
                let mut outfile = File::create(&out_path)?;
                std::io::copy(&mut entry, &mut outfile)?;
            }
        }

        // persist outside the tempdir (which would auto-delete)
        let final_dir = std::env::current_dir()?
            .join(".tmp")
            .join(format!("deceptus_extract_{}", nano_ts()));
        std::fs::create_dir_all(&final_dir)?;

        copy_dir_all(&out_dir, &final_dir)?;
        Ok(final_dir)
    }
}

fn copy_dir_all(src: &Path, dst: &Path) -> Result<()> {
    if !dst.exists() { fs::create_dir_all(dst)?; }
    for entry in fs::read_dir(src)? {
        let entry = entry?;
        let ty = entry.file_type()?;
        let target = dst.join(entry.file_name());
        if ty.is_dir() {
            copy_dir_all(&entry.path(), &target)?;
        } else {
            if let Some(parent) = target.parent() { fs::create_dir_all(parent)?; }
            fs::copy(entry.path(), &target)?;
        }
    }
    Ok(())
}

fn nano_ts() -> u128 {
    use std::time::{SystemTime, UNIX_EPOCH};
    SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_nanos()
}
