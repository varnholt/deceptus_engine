# Mechanism Editor

The mechanism editor lets level designers change mechanism properties while playing, preview live
behavior, and explicitly save property changes to the level's TMX map. Use Tiled for map
layout, tile graphics, creating objects, and editing geometry or paths.

- [Quick start](#quick-start)
- [Saving, discarding, and reloading](#saving-discarding-and-reloading)
- [Supported types and live behavior](#supported-types-and-live-behavior)
- [Developer reference](#developer-reference)

## Quick start

1. Run a desktop development build and load a level.
2. Press **F6** to open the mechanism editor. Properties are editable immediately; there is no
   separate editing-enable checkbox and no console command is needed.
3. Click **Pick an object in the game**, then left-click the desired position inside the game image.
4. Choose an entry from the filtered list. Picking includes every matching mechanism, including
   overlapping effects and hidden trigger volumes, without automatically choosing the topmost one.
5. Change a property. **[live]** fields preview immediately; **[reload]** fields are drafts until
   saved and applied by a level reload. An **asterisk** marks a changed property.
6. Choose **Save all property changes to TMX** to keep the changes, or **Discard all previews**
   to restore unsaved property values.

**F6** closes the editor from either the game or editor window. The window's close button has the
same effect. Closing restores unsaved previews in the active level; saved changes remain.
The editor does not change the game's pause-on-focus-loss setting.

### Finding objects

While the editor is open, hovering inside the game outlines matching mechanisms and shows their
names, types, and TMX source IDs beside the cursor. Overlapping matches are listed together.
Hovering does not change the selection or position filter. The overlay disappears when the cursor leaves the game image or the editor closes.

Picking is armed for one valid click. Press the picking button again for another position, or
**Cancel picking** before clicking the game. Click inside the game image rather than a letterbox bar.

The position filter includes rendered pixels outside a mechanism's interaction rectangle, so a
swinging lamp can be selected at its visible position. Each runtime instance is checked separately,
so other objects cannot hide it from selection. Interaction/source bounds also include hidden
mechanisms and trigger volumes. Rope and hanging-lamp highlights follow their current rendered
geometry, including interpolated movement and lamp rotation, independently of their gameplay
interaction area. Decorative tile graphics are not mechanism objects.

Use **Clear position filter** to browse elsewhere, and the search field to filter by object name,
group, type, or numeric TMX source ID. Drafts remain attached to their sources when selecting
another object. **Save all** includes drafts across all edited sources, not just the visible selection.

One source can produce multiple runtime parts, such as a laser group or mechanism tile layer.
They share one editor entry and draft. **[mixed]** means their runtime values differ: editing that
field applies the new value to every part; discarding restores each part's individual baseline.

### Example: change a hanging lamp's color

Pick the visible lamp, select its `RopeWithLight` entry, and find **color [live]** under the schema
properties. Open the color swatch and adjust RGB and alpha. The shared light component updates
immediately. Choose **Save all property changes to TMX** to keep the color, or discard/close to
restore the previous value.

A color field on another source may be marked **[reload]**. The color picker is available for both
modes; the label describes whether that loaded object has a live runtime binding.

## Saving, discarding, and reloading

| Action | Runtime effect | TMX effect |
|---|---|---|
| Edit **[live]** | Applies accepted values immediately | None until Save |
| Edit **[reload]** | Requires a later level reload | None until Save |
| **Default** | Previews the schema default when live | Draft until Save |
| **Save all property changes to TMX** | Does not reload; reload fields still need reload | Writes all changed property overrides |
| **Discard this object's property edits** | Restores this source's unsaved previews | None |
| **Discard all previews** | Restores all unsaved previews and clears drafts | None |
| **Refresh (discard previews)** | Discards drafts and rereads source metadata | Reads the current map; does not save drafts |
| Close / **F6** | Restores unsaved previews in the active level | Saved changes remain |
| **L** in the game | Reconstructs the level and restores applicable saved game state | Reads the map |

**Default** uses the value declared in the mechanism schema. It can differ from a template's suggested
value for newly inserted objects. Required values, enum choices, finite numbers, and runtime
constraints are validated. Correct rejected values or discard the affected edits before saving.

Save changes only the edited properties of the exact TMX source objects/layers. It preserves
unrelated sources and shared template files; edits to a template instance become overrides in the
map. It does not save all current runtime state or alter tile data.

Each successful save keeps the pre-save map at **`<map>.inspector.bak`**. Later saves reuse that
backup file; it is not version history. To undo a saved map change, restore that backup before
another save overwrites it, or use version control. Discard only undoes unsaved drafts.

### Reload fields and external Tiled edits

For a **[reload]** field: edit it, Save, focus the game, then press **L**. Closing the editor alone
does not apply reload fields. Reloading or changing levels discards unsaved drafts.

The editor's own Save is recognized by the file watcher and does **not** automatically reload the
level. Ordinary external changes in Tiled still use the normal automatic reload behavior.

If the map changed externally before Save, the editor refuses to overwrite it. An external reload
can also replace the current level and clear drafts. Use **Refresh (discard previews)** to read the
new metadata, then reapply the desired edits. A failed validation or conflict does not partially
write a batch of property changes.

### Map defaults and checkpoint state

TMX values are authored level defaults. The game's runtime/checkpoint save state is separate and
can override serialized mechanism state when loading a level. Saving a property to TMX does not
create a checkpoint or save the player's progress.

Game logic continues to run and can subsequently change a live value again. Saving writes the
edited defaults rather than freezing mechanism behavior.

Live previews are real runtime changes: checkpoints may capture serialized effects of those
changes. Discard restores property values, not their gameplay consequences, such as script events,
collected items, spawned objects, or physics motion.

The **Live serialized state** and **Last level snapshot** panels help explain the loaded state.
The latter is an engine level-state snapshot, not proof that a preview was saved to TMX or disk.

## Supported types and live behavior

| Tiled property type | Editor control |
|---|---|
| `bool` | Checkbox |
| `int` | Integer input |
| `float` | Numeric input |
| `string` | Text input |
| `color` | RGBA color picker with alpha |
| Constrained string enum | Choice list |

Colors are saved in [Tiled's alpha-first hex format](https://doc.mapeditor.org/en/stable/reference/tmx-map-format/#property),
with alpha retained across editing and reload. Native **file**, **object-reference**, and nested
**class** property types are not yet supported. Named mechanism links stored as strings can still
be followed in the details panel.

The labels on the loaded source are the current support list: a schema describes which properties
exist, while a runtime binding determines whether each one is **[live]** or **[reload]**. Support for
editing/saving a type does not imply every mechanism can apply it immediately. Sources without a
standalone runtime instance, such as water emitters and laser paths, can still expose reload fields.

The details also show explicit Tiled target/reference links and condition requirements. Script-created
connections are not inferred. Duplicate names are flagged because script and saved-state lookups
can be ambiguous; map writes use numeric TMX source IDs.

## Developer reference

The editor is included only in native desktop development builds. The existing CMake option
`DECEPTUS_DEVELOPMENT_MODE` controls `DEVELOPMENT_MODE`; `DECEPTUS_VRSFML` excludes Switch/web.
There is no separate editor feature define.

The editor starts closed. Property bindings are initialized lazily when an instance is first viewed,
after construction. Constructors and gameplay updates do not initialize or consult the binding
registry, and ordinary runtime fields remain unchanged. Visual pixel checks run during an explicit pick and at most ten times per second for hover
feedback while the editor is open and the cursor is inside the game image. The editor, adapters, and watcher suppression are
excluded from non-development builds.

### Describe properties and bind live behavior

Declare names, supported Tiled types, defaults, and optional enum choices in `MechanismSchema` /
`PropertyInfo`. Defaults should match the engine's actual loading fallback; `template_value`
provides a separate insertion suggestion. Schemas sharing a layer can use a selector property/value
to distinguish specializations. Starting a desktop development build from the repository root
regenerates `data/templates`, `data/schemas/mechanisms.json`, and `deceptus.tiled-project`.

All mechanisms use [MechanismPropertyBindings](../src/game/debug/mechanism_property_bindings.h).
Common properties are registered automatically.
For fields where direct assignment safely changes behavior, declare the guarded override in the
class header and implement the development-only hook:

```cpp
#if defined(DEVELOPMENT_MODE) && !defined(DECEPTUS_VRSFML)
void Fan::configureInspectorProperties(MechanismPropertyBindings& properties)
{
   properties.bind("speed", _speed);
}
#endif
```

For derived/cached state, bind the existing setter instead of writing a member directly:

```cpp
properties.bind<float>("velocity", [this] { return _velocity; }, [this](float value)
{
   setVelocity(value);
   return true;
});
```

Typed adapters support `bool`, `int32_t`, `float`, `std::string`, and `sf::Color`. The shared color
codec handles the difference between RGBA fields and TMX encoding. Extend a specialized parent's
hook when its additional bindings are needed; common bindings do not need a manual base call.

An optional typed validator runs before applying a value. Live edits and TMX writes share schema
validation, and grouped edits validate every part before invoking any setter. Validators must have
no side effects; a setter rejecting an edit must leave its target unchanged.

Member references must remain valid for the mechanism's lifetime. Shorter-lived components should
use weak ownership and the binding's availability callback. A single owned light contributes its
color through the shared light system; removed components become unavailable, and multiple owned
lights require explicit mapping. The editor contains no mechanism-type-specific widget or setter
cases. Leave structural, asset, physics, or reference properties as reload fields until they have a
safe apply/reconstruction path.

The [source writer](../src/game/debug/mechanismsourceeditor.h) returns a
`[[nodiscard]] std::expected<WriteResult, WriteError>`. On success, callers receive the exact saved
map contents and backup path; on failure, they receive a displayable error. The editor acknowledges
successful contents with the watcher before advancing its source baseline. Validation predicates
continue to return `bool`; the persistence API does not use boolean results with output parameters.
