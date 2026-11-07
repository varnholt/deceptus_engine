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
| `F10` | Toggle profiling window |
| `F12` | Toggle developer console |

## Mechanism inspector and property editor

On desktop development builds, enter `inspector` in the developer console (`F12`), then close
that console to resume playing. The inspector is otherwise inactive and does not draw game overlays.

Press **Pick an object in the game**, then left-click a position in the scene. The list filters to
**every mechanism whose bounds contain that position**, including hidden objects and overlapping
triggers or effects. Nothing is automatically selected. Choose any object from that filtered list,
or clear the position filter to browse everything. Decorative tile graphics are not mechanism objects. Schema-backed mechanism tile layers are editable as a single source.

The details show live state, the last saved level snapshot, source properties, outgoing/incoming
Tiled references, and individual condition requirements. Follow a reference to inspect its target.
Script-created connections are not inferred. Duplicate named IDs are flagged.

Editing is disabled by default. Enable **Enable editing** to edit every property described by the
mechanism schema using boolean, integer, float, string, or enum controls. Fields marked **live**
preview through the mechanism's own runtime bindings; fields marked **reload** are authored changes
that require a later manual level reload. The editor does not reconstruct mechanisms implicitly.

Drafts remain attached to their objects when switching selections. **Save all property changes to TMX**
commits the edited fields across all objects in one map update, without capturing unrelated runtime
state. It writes source-level overrides for objects and mechanism tile layers, including for template instances, and leaves shared templates
and other objects unchanged. A `.inspector.bak` backup is retained. If the map changed externally,
saving is refused until the source is refreshed. Required values, numeric types, and enums are validated.

The editor's exact saved contents are accepted by the file watcher **without auto-reloading** the
level. Ordinary external edits in Tiled still trigger the normal reload. Saved properties marked
**reload** take effect on a manual level reload (`L`). Live edits already affect the current object.

Discarding restores unsaved live previews. Closing the inspector also discards unsaved previews in
the active level; saved changes remain. Refreshing discards drafts and reloads source metadata.
The inspector does not change pause settings, so normal focus-loss behavior still applies.

The inspector, source writer, live bindings, and watcher suppression are excluded when
`DECEPTUS_DEVELOPMENT_MODE=OFF`, and are not built for Switch or web targets.

### Adding live support to a mechanism

The inspector lists schema-backed authored sources, including deferred laser groups/paths and water
emitters. Exact TMX IDs associate runtime instances with their sources; names may be empty or duplicated.
Multiple runtime parts share one source entry, draft, and save. A live edit applies to all parts;
discard restores each part's previous value. Sources without runtime bindings remain editable as reload fields.
Shared-layer schemas can declare a selector property/value to identify their specialization.
The editor contains no mechanism-type-specific mutation code.

Common enabled/visible state, Z order, observation, and serialization flags use the existing
mechanism interface. A mechanism opts additional member fields into live preview with:

```cpp
#ifdef DECEPTUS_MECHANISM_INSPECTOR
   bindInspectorProperty("speed", _speed, [](const float& value) { return value >= 0.0f; });
#endif
```

Bind member fields whose lifetime matches the mechanism, and only when assigning the field is
sufficient to update behavior safely. Derived caches, physics fixtures, loaded assets, and references
need a dedicated setter/reconstruction path; leaving them unbound explicitly marks them **reload**.
Bindings are provided for fan speed, bouncer force, blade behavior, collectible behavior, damage,
ring shader tuning, lever handles, gate fade speed, conveyor velocity, shader UV/time settings,
and overlay timing. The same generic editor handles future bindings.

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

### Finding commands

The help panel on the right reacts to what you type, so it never lists more than you need:

| Input | Panel shows |
|-------|-------------|
| _(nothing typed)_ | The topic names only |
| any text | Every command whose description contains that text |
| `help <command>` | The matching commands together with their usage examples |

Examples are deliberately hidden until you ask for them with `help`, and the panel is capped to the
height of the screen; whatever does not fit is reported as `+n more, keep typing`.

### Navigation

| Command | Action |
|---------|--------|
| `tps` | Teleport to start |
| `tpp <x> <y>` | Teleport to tile position |
| `tpc <n>` | Teleport to checkpoint n |
| `tpr <name>` | Teleport to room by name |
| `pwatch <interval_ms\|off>` | Log the player position, health and dialogue state to stdout |

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
