// src/services/artifact_downloader.rs (no-token version)
use anyhow::{Context, Result};
use reqwest::blocking::Client;
use serde::Deserialize;
use std::{fs::File, io::Write, path::PathBuf};

const API_BASE: &str = "https://ci.appveyor.com/api";

#[derive(Deserialize)]
struct BuildJob { jobId: String }
#[derive(Deserialize)]
struct BuildInfo { status: String, version: String, jobs: Vec<BuildJob> }
#[derive(Deserialize)]
struct ProjectResponse { build: Option<BuildInfo> }
#[derive(Deserialize)]
struct Artifact { fileName: String }

pub struct ArtifactDownloader {
    client: Client,
    account: String,
    project: String,
    branch: String,
}

impl ArtifactDownloader {
    pub fn new(account: impl Into<String>, project: impl Into<String>, branch: impl Into<String>) -> Self {
        let client = Client::builder().user_agent("deceptus-updater/0.1").build().unwrap();
        Self { client, account: account.into(), project: project.into(), branch: branch.into() }
    }
}

impl Default for ArtifactDownloader {
    fn default() -> Self { Self::new("varnholt", "deceptus-engine", "master") }
}

impl ArtifactDownloader {
    pub fn fetch_zip(&self, _ignored: &str) -> Result<PathBuf> {
        // 1) latest branch build (public projects usually work without auth)
        let proj_url = format!("{API_BASE}/projects/{}/{}/branch/{}", self.account, self.project, self.branch);
        let resp = self.client.get(&proj_url).send()?;
        if resp.status().as_u16() == 401 {
            anyhow::bail!("appveyor says 401 Unauthorized (project may be private). scraping won't help; consider a token.");
        }
        let project: ProjectResponse = resp.error_for_status()?.json()?;
        let build = project.build.context("no build info for branch")?;
        if build.status != "success" {
            anyhow::bail!("latest '{}' build not successful: {}", self.branch, build.status);
        }
        let job_id = build.jobs.first().context("no jobId on latest build")?.jobId.clone();

        // 2) list artifacts
        let arts_url = format!("{API_BASE}/buildjobs/{job_id}/artifacts");
        let resp = self.client.get(&arts_url).send()?;
        if resp.status().as_u16() == 401 {
            anyhow::bail!("401 on artifacts list. project likely requires auth; token needed.");
        }
        let artifacts: Vec<Artifact> = resp.error_for_status()?.json()?;
        let art = artifacts.into_iter().find(|a| a.fileName.ends_with(".zip"))
            .context("no .zip artifact found")?;

        // 3) download zip (bearer header NOT required here for public; also don't set Content-Type)
        let dl_url = format!("{API_BASE}/buildjobs/{job_id}/artifacts/{}", art.fileName);
        let mut resp = self.client.get(&dl_url).send()?.error_for_status()?;

        let out = std::env::current_dir()?  // current working dir
            .join(".tmp")
            .join(format!("deceptus_artifact_{}.zip", timestamp_nanos()));
        std::fs::create_dir_all(out.parent().unwrap())?;

        let mut file = File::create(&out)?;
        let bytes = resp.bytes()?;
        file.write_all(&bytes)?;
        Ok(out)
    }
}

fn timestamp_nanos() -> u128 {
    use std::time::{SystemTime, UNIX_EPOCH};
    SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_nanos()
}
