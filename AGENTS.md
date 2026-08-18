# AI Agent Instructions

When generating code for this project, always follow these conventions:

## C++ Standard

- **Use C++23 features** wherever applicable
- Prefer modern C++ idioms (ranges, views, `std::format`, etc.)
- Avoid pre-C++20 constructs when C++23 alternatives exist

## Naming Conventions

- **snake_case** for all code:
  - Variables: `player_health`, `max_velocity`
  - Functions: `calculate_damage`, `get_player_info`
  - Member variables: `_player_position`, `current_state`
  - File names: `player_controls.cpp`, `item_factory.h`

## Comments

- **All comments in lowercase** (except for proper nouns, acronyms, and code references)
- Example:
  ```cpp
  // initialize player body
  // box2d uses meters, not pixels
  ```

## Code Style

- Follow existing project formatting (`.clang-format` is provided)
- Use `[[nodiscard]]` for functions that shouldn't ignore return values
- Prefer `const std::string&` over `std::string` for input parameters
- Use `std::string_view` where appropriate for read-only string access
- Prefer ranges and algorithms over manual loops

## Example

```cpp
#include "framework/tools/stringutils.h"

#include <ranges>
#include <string>

namespace
{
// helper function to normalize item names
std::string normalize_item_name(const std::string& name)
{
   return StringUtils::toLower(name);
}
}  // namespace
```
