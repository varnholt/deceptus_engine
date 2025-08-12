use eframe::egui;
use std::path::PathBuf;
use std::sync::mpsc::{self, Receiver, Sender};
use std::thread;
use std::time::{Duration, Instant};

use crate::services::{ArtifactDownloader, Backup, Extractor, Installer};

/* ---------------- App entry ---------------- */

pub fn run_gui(files_json: String) -> eframe::Result<()> {
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_title("Deceptus Updater")
            .with_inner_size([640.0, 520.0]),
        ..Default::default()
    };
    let app = UpdaterApp::new(Some(files_json));
    eframe::run_native("Deceptus Updater", options, Box::new(|_| Box::new(app)))
}

/* ---------------- UI model ---------------- */

#[derive(Clone, Default)]
struct UiState {
    running: bool,
    step: String,
    percent: f32,        // 0.0 ..= 1.0
    logs: Vec<String>,
    done: bool,
    error: Option<String>,
}

/* ---------------- Messages from worker → UI ---------------- */

enum Event {
    Started,
    Step { label: &'static str, bump: f32 },
    Log(String),
    Done,
    Error(String),
}

/* ---------------- eframe App ---------------- */

struct UpdaterApp {
    ui: UiState,
    files_json_flag: Option<String>,
    rx: Option<Receiver<Event>>,
    last_poll: Instant,
}

impl UpdaterApp {
    fn new(files_json_flag: Option<String>) -> Self {
        Self {
            ui: UiState::default(),
            files_json_flag: files_json_flag.filter(|s| !s.is_empty()),
            rx: None,
            last_poll: Instant::now(),
        }
    }
}

impl eframe::App for UpdaterApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        // Poll worker messages ~10Hz
        if self.last_poll.elapsed() >= Duration::from_millis(100) {
            if let Some(rx) = &self.rx {
                while let Ok(ev) = rx.try_recv() {
                    match ev {
                        Event::Started => {
                            self.ui = UiState::default();
                            self.ui.running = true;
                            self.ui.step = "Starting…".into();
                            self.ui.percent = 0.01;
                            self.ui.logs.push("Starting update…".into());
                        }
                        Event::Step { label, bump } => {
                            self.ui.step = label.to_string();
                            self.ui.percent = (self.ui.percent + bump).clamp(0.0, 1.0);
                        }
                        Event::Log(line) => {
                            self.ui.logs.push(line);
                            if self.ui.logs.len() > 500 {
                                let overflow = self.ui.logs.len() - 500;
                                self.ui.logs.drain(0..overflow);
                            }
                        }
                        Event::Done => {
                            self.ui.running = false;
                            self.ui.done = true;
                            self.ui.percent = 1.0;
                        }
                        Event::Error(e) => {
                            self.ui.running = false;
                            self.ui.done = false;
                            self.ui.error = Some(e);
                        }
                    }
                }
            }
            self.last_poll = Instant::now();
        }
        ctx.request_repaint_after(Duration::from_millis(100));

        // ---- UI ----
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("Deceptus Updater");

            // status line
            if let Some(err) = &self.ui.error {
                ui.colored_label(egui::Color32::RED, format!("✘ {err}"));
            } else if self.ui.done {
                ui.colored_label(egui::Color32::GREEN, "✔ All done.");
            } else if self.ui.running {
                ui.label(format!("{} ({:.0}%)", self.ui.step, self.ui.percent * 100.0));
            } else {
                ui.label("Idle");
            }

            // progress
            ui.add(egui::ProgressBar::new(self.ui.percent).show_percentage());

            // start button
            let running = self.ui.running;
            if ui.add_enabled(!running, egui::Button::new("Update now")).clicked() {
                let (tx, rx) = mpsc::channel::<Event>();
                self.rx = Some(rx);
                self.ui = UiState::default(); // clear log immediately
                spawn_worker(tx, self.files_json_flag.clone());
            }

            ui.separator();
            ui.label("Log:");
            egui::ScrollArea::vertical()
                .auto_shrink([false, false])
                .stick_to_bottom(true)
                .show(ui, |ui| {
                    for line in &self.ui.logs {
                        ui.monospace(line);
                    }
                });
        });
    }
}

/* ---------------- Worker ---------------- */

fn spawn_worker(tx: Sender<Event>, files_json_flag: Option<String>) {
    let _ = tx.send(Event::Started);
    thread::spawn(move || {
        // Resolve files.json (flag → argv → default)
        let files_json = files_json_flag
            .filter(|s| !s.is_empty())
            .or_else(|| std::env::args().nth(1))
            .unwrap_or_else(|| "files.json".into());
        let json_path = PathBuf::from(&files_json);

        // Helpers
        let step = |label: &'static str, bump: f32| { let _ = tx.send(Event::Step { label, bump }); };
        let log  = |s: String| { let _ = tx.send(Event::Log(s)); };
        let fail = |e: String| { let _ = tx.send(Event::Error(e)); };

        // 1) Backup
        step("Backing up listed files…", 0.10);
        log(format!("Backing up using {}", json_path.display()));
        if let Err(e) = Backup::new("backups").backup_from_json(json_path.clone()) {
            fail(format!("Backup failed: {e:#}"));
            return;
        }
        log("Backup complete.".into());

        // 2) Download
        step("Downloading artifact…", 0.25);
        let zip_path = match ArtifactDownloader::default().fetch_zip("ignored") {
            Ok(p) => { log(format!("Downloaded → {}", p.display())); p }
            Err(e) => { fail(format!("Download failed: {e:#}")); return; }
        };

        // 3) Extract
        step("Extracting…", 0.35);
        let extracted_dir = match Extractor::default().extract_zip(&zip_path) {
            Ok(p) => { log(format!("Extracted → {}", p.display())); p }
            Err(e) => { fail(format!("Extract failed: {e:#}")); return; }
        };

        // 4) Install
        step("Installing files…", 0.25);
        let install_root = match std::env::current_dir() {
            Ok(d) => d,
            Err(e) => { fail(format!("Cannot read current dir: {e:#}")); return; }
        };
        if let Err(e) = Installer::default().install_list(&json_path, &extracted_dir, &install_root) {
            fail(format!("Install failed: {e:#}"));
            return;
        }
        log("Installation complete.".into());

        let _ = tx.send(Event::Done);
    });
}
