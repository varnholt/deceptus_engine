# Patch Tool

A small Windows GUI tool for applying patches to files in `data/catacombs/`.
Designed for the workflow where an artist makes TMX edits on a separate branch
and wants to hand those changes to someone on a different branch without a full merge.

---

## Creating a patch

### From a git branch (most common)

You have your changes on a branch and want to produce a patch that can be applied
on top of whatever is currently on `master`:

```
git diff master -- data/catacombs/catacombs.tmx > data/catacombs/my_changes.diff
```

To include all changed files under `data/catacombs/` at once:

```
git diff master -- data/catacombs/ > data/catacombs/my_changes.diff
```

Give the `.diff` file a descriptive name so the recipient knows what it does,
e.g. `add_chest_room_props.diff`.

### From two files (no git)

If you just have an original and a modified copy side by side:

```
diff -u original/catacombs.tmx modified/catacombs.tmx > my_changes.diff
```

On Windows you can also use WinMerge: **Tools → Generate Patch** produces a
unified diff in the same format.

---

## Applying a patch

1. Copy the `.diff` or `.patch` file into `data/catacombs/`.
2. Run `tmx_patcher.exe` (from `lab/patch_tool/target/release/`).
3. Select the patch from the dropdown (click **⟳** to refresh if you just copied it in).
4. Click **Apply Patch**.
5. Check the log — each hunk reports `applied`, `already applied`, or `FAILED`.

On success the original file is backed up as `catacombs.tmx.bak` and the patch
file is moved to `data/catacombs/applied_patches/` so it won't show up in the
dropdown again.

---

## How it handles a moving target

The tool ignores the line numbers in the `@@ -N,M @@` headers entirely.
Instead it searches for the exact block of context + changed lines anywhere
in the file. This means a patch created against an older version of the TMX
will still apply correctly as long as the surrounding lines haven't changed —
even if hundreds of lines were added or removed elsewhere in the file.

If a hunk can't be found the tool reports `FAILED` and leaves the file
untouched, so it is always safe to run.
