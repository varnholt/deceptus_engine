# License scope

| Material | Terms |
| --- | --- |
| Original engine code and associated engine documentation owned by Matthias Varnholt, in the paths listed in the root LICENSE.md | [MIT](ENGINE-MIT.txt); commercial engine reuse is allowed |
| Deceptus game content, including artwork by dstar | [Rights reserved](GAME-CONTENT.txt), subject to separately licensed assets and valid earlier permissions |
| Third-party libraries, generated bindings, separately licensed assets and contributions whose rights are held by others | Their respective existing terms; not relicensed by this notice |
| Material previously validly offered under CC BY-NC 4.0 | [Earlier terms preserved](LEGACY-CC-BY-NC-4.0.md) |

`src/opengl/glm/`, GLEW headers and generated OpenGL bindings are examples of
bundled third-party material; retain their notices. Dependencies fetched by
CMake also retain their upstream licenses. Check each dependency and asset
before redistributing a product; the root MIT grant does not replace those terms.

Repository history also includes contributions by Andre/André Wösten, including
commits `743535f2`, `b087b6de`, `db5f3647`, `980e65bc` and `a83b51f9`. No claim
is made to license rights he may retain in that material under MIT. Any surviving
copyrightable contributions remain subject to their existing permissions unless
separate authorization applies. This is a specific boundary to review for a
commercial redistribution, not a blanket statement that all historical changes
are separately copyrightable or still present.

For an independent commercial game, use the reusable engine with your own or
separately licensed content. The Deceptus campaign and its assets are not an
MIT-licensed sample asset pack. Unclassified game data defaults to the reserved
content policy rather than the engine grant; `data/shaders/` is the stated
engine-code exception, subject to third-party notices.

Packagers must include the root LICENSE.md and this LICENSES directory, alongside
all applicable dependency and asset notices. A packed asset archive is a delivery
format, not a change to copyright permissions or a guarantee against extraction.
