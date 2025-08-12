mod services; // your Backup / ArtifactDownloader / Extractor / Installer
mod gui;      // egui app below

use crate::services::{ArtifactDownloader, Backup, Extractor, Installer};
use std::path::PathBuf;

/// Usage:
///   updater.exe --silent [files.json]
///   updater.exe [files.json]        # GUI
fn main() {
    let args: Vec<String> = std::env::args().skip(1).collect();
    let silent = args.iter().any(|a| a == "--silent");
    let files_json = args
        .iter()
        .find(|s| !s.starts_with("--"))
        .cloned()
        .unwrap_or_else(|| "files.json".into());

    if silent {
        if let Err(err) = run_cli(&files_json) {
            eprintln!("ERROR: {err:#}");
            std::process::exit(1);
        }
        return;
    }

    if let Err(err) = gui::run_gui(files_json) {
        eprintln!("GUI failed: {err}");
        std::process::exit(1);
    }
}

/// Headless pipeline with verbose println! output.
fn run_cli(files_json: &str) -> anyhow::Result<()> {
    let json_path = PathBuf::from(files_json);

    println!("[1/4] Backing up listed files… ({})", json_path.display());
    let moved_dir = Backup::new("backups").backup_from_json(json_path.clone())?;
    println!("      Backup complete → {}", moved_dir.display());

    println!("[2/4] Downloading latest artifact…");
    let zip = ArtifactDownloader::default().fetch_zip("ignored")?;
    println!("      Downloaded → {}", zip.display());

    println!("[3/4] Extracting artifact…");
    let extracted = Extractor::default().extract_zip(&zip)?;
    println!("      Extracted → {}", extracted.display());

    println!("[4/4] Installing files into working directory…");
    let install_root = std::env::current_dir()?;
    Installer::default().install_list(&json_path, &extracted, &install_root)?;
    println!("✔ Update complete.");
    Ok(())
}
