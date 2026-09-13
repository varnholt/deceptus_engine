# tmx-verify

checks that catch the ways a level sync from `deceptus_game` silently breaks the
engine side. all read-only, all plain python, no dependencies. run from the repo
root.

`lab/tmx_lint` covers duplicate object *names*; these cover the rest.

## verify_lua_refs.py

every name a level script resolves, checked against the map, per layer.

```
python lab/tmx_verify/verify_lua_refs.py data/level-catacombs/catacombs.tmx data/level-catacombs/level.lua
```

the engine resolves mechanisms by TMX object name, so a rename fails silently:
`setMechanismEnabled`/`showDialogue` still runs, finds nothing, and the mechanism
never reacts. only `showDialogue` logs anything.

compare the unresolved count against `master` — it must *match*, not merely be
small. the catacombs map has 4 pre-existing dangling references, so a bare "4
unresolved" is the expected clean result, not a failure.

## verify_assets.py

every file the map references, checked against disk.

```
python lab/tmx_verify/verify_assets.py data/level-catacombs/catacombs.tmx .
```

resolution rules are per property and sometimes per layer, taken from the engine:

| property | resolves to | source |
|---|---|---|
| `script` | `data/scripts/enemies/` | `level.cpp:667` |
| `sample`, `filename` | `data/sounds/` | `audiobackenddesktop.cpp:17` |
| `texture` on `static_lights` | `data/light/` | `staticlight.cpp:153` |
| `texture` elsewhere | repo-relative as written | |
| tileset / imagelayer `source` | relative to the tmx | |

also follows images referenced from inside each `.tsx`. getting the per-layer
rule wrong produces false positives — `topdown.png` and friends look missing
until you know static lights prefix `data/light/`.

## tmx_compare2.py

diffs two versions of a map by *(layer, object id)* rather than by line.

```
python lab/tmx_verify/tmx_compare2.py data/level-catacombs/catacombs.tmx ../deceptus_game/levels/catacombs/catacombs.tmx
```

tiled object ids are stable, so keying on them turns a 2500-line textual diff
into a short list of renames, property changes, additions and deletions. reports
duplicate ids and newly introduced duplicate names too.

key on `(layer, id)` and not `id` alone: ids can collide across layers when
`nextobjectid` has drifted below the highest id in use, and keying on id alone
silently compares two unrelated objects.

## dig_locked_box.py

walks every revision of a map and reports how one object was wired over time.

```
python lab/tmx_verify/dig_locked_box.py . data/level-catacombs/catacombs.tmx
```

written to answer "was this chest ever linked to an extra?" across 200
revisions. adjust the object name and radius inside for other questions.
