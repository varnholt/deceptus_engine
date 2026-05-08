# Development Hotkeys

All hotkeys in this document are only active in `DEVELOPMENT_MODE` builds, except where noted.

## Overlays & UI

| Key | Action |
|-----|--------|
| `F1` | Toggle debug overlay |
| `F2` | Toggle controller overlay |
| `F3` | Toggle camera system config UI |
| `F4` | Toggle debug info / `Alt+F4` quits |
| `F5` | Toggle log viewer |
| `F6` | Toggle test scene |
| `F7` | Toggle physics config UI |
| `F12` | Toggle developer console |

## Player Event Recording

| Key | Action |
|-----|--------|
| `F8` | Start / stop recording player events |
| `F9` | Replay recorded player events |

## Level & World

| Key | Action |
|-----|--------|
| `L` | Reload current level |
| `N` | Load next level |
| `R` | Reset level |
| `Page Up` | Increase ambient light |
| `Page Down` | Decrease ambient light |
| `1` | Zoom in |
| `2` | Zoom out |
| `3` | Reset zoom |

## Player

| Key | Action |
|-----|--------|
| `G` | Flip player gravity |
| `V` | Toggle player visibility |
| `M` | Toggle recording mode |
| `Q` | Quit |

## Always Available

| Key | Action |
|-----|--------|
| `F` | Toggle fullscreen |
| `P` / `Escape` | Pause menu |

---

## Developer Console (`F12`)

### Navigation

| Command | Action |
|---------|--------|
| `tps` | Teleport to start |
| `tpp <x> <y>` | Teleport to tile position |
| `tpc <n>` | Teleport to checkpoint n |
| `tpr <name>` | Teleport to room by name |

### Player State

| Command | Action |
|---------|--------|
| `iddqd` | Toggle invulnerability |
| `damage <n>` | Deal n damage to player |
| `pgravity <value>` | Set gravity scale |

### Weapons & Abilities

| Command | Action |
|---------|--------|
| `weapon add sword/bow/gun` | Give weapon |
| `weapon clear` | Remove all weapons |
| `extra add climb/dash/walljump/doublejump/…` | Give ability |
| `extra add all` | Give all abilities |
| `extra clear` | Remove all abilities |

### Items

| Command | Action |
|---------|--------|
| `item add <name>` | Add item |
| `item remove <name>` | Remove item |
| `item list` | List player's items |
| `item listall` | List all available items |
| `item clear` | Remove all items |

### Misc

| Command | Action |
|---------|--------|
| `playerlight enable/disable` | Toggle player light |
| `playerlight alpha <0-255>` | Set player light intensity |
| `cpanlimitoff` | Disable camera pan distance limit |
| `ra` | Reload animation pool |
| `playback enable/save/load/replay/reset` | Player event playback |
