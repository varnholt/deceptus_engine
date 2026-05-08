#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use eframe::egui;
use fuzzy_diff::{apply_patch_to_dir, parse_diff, HunkResult};
use std::fs;
use std::path::{Path, PathBuf};

fn main() -> Result<(), eframe::Error> {
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([720.0, 520.0])
            .with_title("Patch Tool"),
        ..Default::default()
    };
    eframe::run_native(
        "Patch Tool",
        options,
        Box::new(|_cc| Ok(Box::new(PatchApp::new()))),
    )
}

// ─── app state ───────────────────────────────────────────────────────────────

struct PatchApp {
    catacombs_dir: Option<PathBuf>,
    available_patches: Vec<PathBuf>,
    selected_index: usize,
    log: String,
}

impl PatchApp {
    fn new() -> Self {
        let catacombs_dir = locate_catacombs_dir();
        let available_patches = scan_for_patches();
        Self {
            catacombs_dir,
            available_patches,
            selected_index: 0,
            log: String::new(),
        }
    }

    fn refresh_patch_list(&mut self) {
        self.available_patches = scan_for_patches();
        self.selected_index = 0;
    }

    fn apply_selected_patch(&mut self) {
        let Some(patch_path) = self.available_patches.get(self.selected_index).cloned() else {
            self.append_log("No patch selected.\n");
            return;
        };

        let Some(ref catacombs_dir) = self.catacombs_dir.clone() else {
            self.append_log("catacombs directory not found.\n");
            return;
        };

        let patch_name =
            patch_path.file_name().unwrap_or_default().to_string_lossy().into_owned();

        let raw_content = match fs::read_to_string(&patch_path) {
            Ok(content) => content,
            Err(error) => {
                self.append_log(&format!("Error reading '{patch_name}': {error}\n"));
                return;
            }
        };

        let file_patches = parse_diff(&raw_content);
        if file_patches.is_empty() {
            self.append_log(&format!("No hunks found in '{patch_name}'.\n"));
            return;
        }

        self.append_log(&format!("─── Applying '{patch_name}' ───\n"));

        let mut overall_success = true;

        for file_patch in &file_patches {
            self.append_log(&format!("  File: {}\n", file_patch.filename));
            let report = apply_patch_to_dir(catacombs_dir, file_patch);
            for (hunk_index, result) in report.hunk_results.iter().enumerate() {
                match result {
                    HunkResult::Applied => {
                        self.append_log(&format!("    hunk {}: applied\n", hunk_index + 1));
                    }
                    HunkResult::AlreadyApplied => {
                        self.append_log(&format!(
                            "    hunk {}: already applied — skipped\n",
                            hunk_index + 1
                        ));
                    }
                    HunkResult::Failed(message) => {
                        self.append_log(&format!(
                            "    hunk {}: FAILED — {message}\n",
                            hunk_index + 1
                        ));
                        overall_success = false;
                    }
                }
            }
        }

        if overall_success {
            let applied_dir = patch_path
                .parent()
                .unwrap_or(Path::new("."))
                .join("applied_patches");
            let _ = fs::create_dir_all(&applied_dir);
            let destination = applied_dir.join(&patch_name);
            match fs::rename(&patch_path, &destination) {
                Ok(()) => {
                    self.append_log(&format!("Moved to applied_patches/{patch_name}\n"));
                }
                Err(error) => {
                    self.append_log(&format!("Warning: could not move patch file: {error}\n"));
                }
            }
            self.append_log("Done.\n\n");
            self.refresh_patch_list();
        } else {
            self.append_log(
                "One or more hunks failed — target file was not modified. \
                 Check that the context lines still match.\n\n",
            );
        }
    }

    fn append_log(&mut self, text: &str) {
        self.log.push_str(text);
    }
}

// ─── gui ─────────────────────────────────────────────────────────────────────

impl eframe::App for PatchApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("Patch Tool");
            ui.separator();

            // Catacombs folder row
            ui.horizontal(|ui| {
                ui.label("Catacombs:");
                match &self.catacombs_dir {
                    Some(directory) => {
                        ui.monospace(directory.to_string_lossy().as_ref());
                    }
                    None => {
                        ui.colored_label(egui::Color32::RED, "catacombs directory not found");
                    }
                }
            });

            ui.add_space(10.0);

            // Dropdown + refresh row
            ui.horizontal(|ui| {
                let selected_label = self
                    .available_patches
                    .get(self.selected_index)
                    .and_then(|path| path.file_name())
                    .map(|name| name.to_string_lossy().into_owned())
                    .unwrap_or_else(|| "— no .diff / .patch files found —".to_string());

                egui::ComboBox::from_id_source("patch_selector")
                    .selected_text(&selected_label)
                    .width(500.0)
                    .show_ui(ui, |ui| {
                        for (index, patch_path) in self.available_patches.iter().enumerate() {
                            let name = patch_path
                                .file_name()
                                .map(|name| name.to_string_lossy().into_owned())
                                .unwrap_or_default();
                            ui.selectable_value(&mut self.selected_index, index, name);
                        }
                    });

                if ui.button("⟳").on_hover_text("Re-scan folder").clicked() {
                    self.refresh_patch_list();
                }
            });

            ui.add_space(10.0);

            let can_apply =
                !self.available_patches.is_empty() && self.catacombs_dir.is_some();
            if ui
                .add_enabled(can_apply, egui::Button::new("  Apply Patch  "))
                .clicked()
            {
                self.apply_selected_patch();
            }

            ui.add_space(10.0);
            ui.separator();
            ui.label("Log:");
            ui.add_space(4.0);

            egui::ScrollArea::vertical()
                .stick_to_bottom(true)
                .show(ui, |ui| {
                    ui.add(
                        egui::Label::new(egui::RichText::new(&self.log).monospace().size(12.0))
                    );
                });
        });
    }
}

// ─── helpers ─────────────────────────────────────────────────────────────────

/// Walk up from the current working directory and the executable location,
/// looking for a `levels/catacombs/` or `data/catacombs/` subdirectory.
fn locate_catacombs_dir() -> Option<PathBuf> {
    let candidates = ["levels/catacombs", "data/catacombs"];

    if let Ok(working_dir) = std::env::current_dir() {
        for subpath in &candidates {
            let candidate = working_dir.join(subpath);
            if candidate.is_dir() {
                return Some(candidate);
            }
        }
    }

    let exe_path = std::env::current_exe().ok()?;
    let mut search_dir = exe_path.parent()?;
    for _ in 0..12 {
        for subpath in &candidates {
            let candidate = search_dir.join(subpath);
            if candidate.is_dir() {
                return Some(candidate);
            }
        }
        search_dir = search_dir.parent()?;
    }

    None
}

/// Collect all `.diff` and `.patch` files from the executable's directory and
/// the current working directory, sorted by name.
fn scan_for_patches() -> Vec<PathBuf> {
    let mut search_dirs: Vec<PathBuf> = Vec::new();

    if let Ok(working_dir) = std::env::current_dir() {
        search_dirs.push(working_dir);
    }
    if let Ok(exe_path) = std::env::current_exe() {
        if let Some(exe_dir) = exe_path.parent() {
            let exe_dir = exe_dir.to_path_buf();
            if !search_dirs.contains(&exe_dir) {
                search_dirs.push(exe_dir);
            }
        }
    }

    let mut patch_files: Vec<PathBuf> = Vec::new();
    for directory in &search_dirs {
        let Ok(read_dir) = fs::read_dir(directory) else {
            continue;
        };
        for entry in read_dir.filter_map(|entry| entry.ok()) {
            let path = entry.path();
            if !path.is_file() {
                continue;
            }
            let Some(extension) = path.extension().and_then(|ext| ext.to_str()) else {
                continue;
            };
            if (extension == "diff" || extension == "patch") && !patch_files.contains(&path) {
                patch_files.push(path);
            }
        }
    }

    patch_files.sort();
    patch_files
}
