# Fast iteration loop for the Switch port.
#
# The engine builds against its own fetched copies under build_switch_engine/_deps/, not
# against the working copies at D:/deceptus/sdl3_switch and D:/deceptus/vrsfml_switch, so
# editing a working copy alone changes nothing. This does both halves of the loop:
#
#   1. regenerates the patch under patches/ from the working copy, which is what the build
#      actually verifies -- CMake refuses to configure when a patch neither applies nor
#      reverse-applies, so a stale patch fails the build before anything compiles
#   2. mirrors the changed sources straight into build_switch_engine/_deps/ so the next
#      build picks them up without re-cloning the dependency
#
# The mirror is deliberately a copy rather than a git operation. Never git checkout or git
# clean a tree under _deps/ from Windows: core.autocrlf rewrites it CRLF and the LF patch
# then fails to apply inside the container while git apply --check still passes on the host.

param(
    [string]$RepositoryRoot = "D:\deceptus\deceptus_engine",
    [string]$SdlWorkingCopy = "D:\deceptus\sdl3_switch",
    [string]$VrsfmlWorkingCopy = "D:\deceptus\vrsfml_switch",
    [string]$BuildDirectory = "build_switch_engine"
)

$ErrorActionPreference = "Stop"

# the revisions the patches are generated against; they match the FetchContent pins in the
# root CMakeLists, and regenerating against anything else produces a patch that will not apply
$sdl_base_revision = "e205361fb"
$vrsfml_base_revision = "9c272d601"

function Sync-Dependency {
    param(
        [string]$WorkingCopy,
        [string]$BaseRevision,
        [string]$PatchPath,
        [string]$DependencyDirectory
    )

    if (-not (Test-Path $WorkingCopy)) {
        Write-Output "skipping $WorkingCopy (not present)"
        return
    }

    Push-Location $WorkingCopy
    try {
        git add -A
        # --cached against the base revision, so newly added files are included too. The
        # redirection goes through cmd on purpose: git writes LF and cmd passes the bytes
        # through untouched, while PowerShell's Set-Content would re-encode the line endings
        # and a CRLF patch does not apply inside the Linux container.
        cmd /c "git diff $BaseRevision --cached > `"$PatchPath`""
    } finally {
        Pop-Location
    }
    Write-Output "regenerated $PatchPath"

    if (-not (Test-Path $DependencyDirectory)) {
        Write-Output "  no fetched copy at $DependencyDirectory yet, nothing to mirror"
        return
    }

    # only the files the patch touches need mirroring; anything else is already identical
    Push-Location $WorkingCopy
    try {
        $changed_files = git diff $BaseRevision --cached --name-only
    } finally {
        Pop-Location
    }

    foreach ($relative_path in $changed_files) {
        $source = Join-Path $WorkingCopy $relative_path
        $destination = Join-Path $DependencyDirectory $relative_path

        if (-not (Test-Path $source)) {
            continue
        }

        $destination_directory = Split-Path $destination -Parent
        if (-not (Test-Path $destination_directory)) {
            New-Item -ItemType Directory -Force -Path $destination_directory | Out-Null
        }

        # Deliberately not Copy-Item. The working copies sit on a host with core.autocrlf=true,
        # so every file git has ever checked out there is CRLF, while the tree under _deps/ was
        # cloned inside the container and is LF. Copying verbatim would drag CRLF into _deps/,
        # and the LF patch would then fail to apply in the container -- the exact trap this
        # port has hit before. Normalise on the way in.
        $content = [System.IO.File]::ReadAllBytes($source)
        $text = [System.Text.Encoding]::UTF8.GetString($content).Replace("`r`n", "`n")
        [System.IO.File]::WriteAllText($destination, $text, (New-Object System.Text.UTF8Encoding($false)))
    }

    Write-Output "  mirrored $($changed_files.Count) file(s) into $DependencyDirectory"
}

Sync-Dependency -WorkingCopy $SdlWorkingCopy `
                -BaseRevision $sdl_base_revision `
                -PatchPath (Join-Path $RepositoryRoot "patches\switch-sdl3-backend.patch") `
                -DependencyDirectory (Join-Path $RepositoryRoot "$BuildDirectory\_deps\SDL")

Sync-Dependency -WorkingCopy $VrsfmlWorkingCopy `
                -BaseRevision $vrsfml_base_revision `
                -PatchPath (Join-Path $RepositoryRoot "patches\switch-vrsfml-backend.patch") `
                -DependencyDirectory (Join-Path $RepositoryRoot "$BuildDirectory\_deps\sfml-src")

Write-Output "done - now run: build_switch.bat . $BuildDirectory"
