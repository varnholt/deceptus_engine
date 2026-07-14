# Documentation

- Cutscene system: `data/scripts/cutscene.md`
- Easing functions: `data/scripts/easings.lua`

# Code Style

- Always use curly braces for `if`, `else`, `for`, `while` bodies — even single-line ones.
- Doxygen comments for member variables use trailing `//!<` style, not leading `/// \brief` blocks.
- Keep changes minimal and non-invasive: only touch what the task requires.
- Do not add error handling, comments, or refactoring beyond what was asked.
- Never remove or strip existing comments — not ASCII diagrams, explanatory blocks, commented-out code, or any other comment. Ever.
- Never use short or abbreviated variable names (`val`, `tex`, `map`, `rgba`, `it`, `os`, `i`, `n`). Always use fully descriptive names.
