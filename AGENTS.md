# Project conventions

When generating code for this project, always follow these conventions:

## C++ Standard

- **Use C++23 features** wherever applicable
- Prefer modern C++ idioms (ranges, views, `std::format`, etc.)
- Avoid pre-C++20 constructs when C++23 alternatives exist

## C++ naming conventions

- Classes, structs and enum types use CamelCase: `SplashAnimation`, `SpriteLayer`.
- Functions and methods use camelCase: `getDuration()`, `drawFrame()`, `calculateDamage()`.
- Variables and parameters use snake_case: `config_path`, `player_health`.
- Member variables use a leading underscore and snake_case: `_player_position`.
- Constants and enumerators follow the surrounding module. Preserve names required by external APIs.
- Follow the surrounding module's file naming and layout. Do not apply Python naming rules to C++.

## C++ comments

- **All comments in lowercase** (except for proper nouns, acronyms, and code references)
- Example:
  ```cpp
  // initialize player body
  // box2d uses meters, not pixels
  ```

## C++ code style

- Follow existing project formatting (`_clang-format` and `.cmake-format.yaml` are provided)
- Always use curly braces for `if`, `else`, `for`, `while`, and `do` bodies, including single-statement early returns
- Run `clang-format -i` on changed C++ files and `cmake-format -i` on changed CMake files before committing
- Document public classes/functions and non-obvious contracts with concise Doxygen comments. Include parameters, ownership and side effects when useful; avoid boilerplate on trivial private helpers.
- Use `[[nodiscard]]` for functions that shouldn't ignore return values
- Prefer `const std::string&` over `std::string` for input parameters
- Use `std::string_view` where appropriate for read-only string access
- Prefer ranges and algorithms over manual loops

## C++ example

```cpp
#include "framework/tools/stringutils.h"

#include <ranges>
#include <string>

namespace
{
// helper function to normalize item names
std::string normalizeItemName(const std::string& name)
{
   return StringUtils::toLower(name);
}
}  // namespace
```

## Shared clean-code practices

- Inspect neighboring implementations and existing utilities before writing code. Match the established module's conventions; resolve contradictory instructions against the user's stated conventions.
- Give functions one clear purpose. Extract a helper when it makes intent clearer, not merely to hide a few lines. Keep public interfaces small and use meaningful domain names.
- Separate engine behavior, game-specific content, asset conversion and platform integration. Keep artwork, layout and timing choices in assets/configuration where appropriate.
- Prefer straightforward control flow and explicit ownership. Avoid speculative frameworks, unnecessary indirection, duplicated logic and hidden mutable state.
- Reuse established parsers, easing functions and graphics helpers. Validate external input at the boundary and report useful errors at an appropriate boundary rather than silently swallowing failures.
- Comments explain intent, constraints and non-obvious decisions. Do not narrate obvious code or add boilerplate documentation to trivial helpers.
- Preserve behavior during structural changes. Update callers, generators and tests together, and run checks appropriate to the affected platforms.

## Python

- Follow PEP 8: CapWords classes, snake_case functions/methods/variables and UPPER_SNAKE_CASE constants. Use four-space indentation.
- Follow the existing module's organization and imports. Keep Python readable and idiomatic; C/C++ naming and formatting rules do not apply to Python.
- Python bindings and generated C/C++ text must use the actual exported C/C++ symbol names, even when those differ from Python naming conventions.


## Local verification artifacts

- Newly created unit tests, integration checks, throwaway harnesses, test-only fixtures, recordings and build outputs are local verification artifacts by default. Do not commit or push them unless the user explicitly asks to version them.
- Keep local checks outside tracked source paths where practical, and stage publication files explicitly rather than adding all untracked files.
- Maintain existing versioned tests when an API change requires it, but do not treat that as permission to publish new throwaway checks.
