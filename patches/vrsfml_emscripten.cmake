# Patch VRSFML 3.1.0 to add Emscripten/WASM support.
# Fixes cmake/Config.cmake (OS detection) and include/SFML/Config.hpp (header guard).
# Run via FetchContent PATCH_COMMAND:
#   PATCH_COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=<SOURCE_DIR> -P patches/vrsfml_emscripten.cmake

# --- cmake/Config.cmake ---
set(cmake_config "${SOURCE_DIR}/cmake/Config.cmake")
file(READ "${cmake_config}" _content)
if(NOT _content MATCHES "SFML_OS_EMSCRIPTEN")
    string(REPLACE
        "else()\n    message(FATAL_ERROR \"Unsupported operating system or environment\")"
        "elseif(\${CMAKE_SYSTEM_NAME} STREQUAL \"Emscripten\")\n    set(SFML_OS_UNIX 1)\n    set(OPENGL_ES 1)\n    set(SFML_OS_EMSCRIPTEN 1)\nelse()\n    message(FATAL_ERROR \"Unsupported operating system or environment\")"
        _content "${_content}")
    file(WRITE "${cmake_config}" "${_content}")
endif()

# --- include/SFML/Config.hpp ---
set(sfml_config "${SOURCE_DIR}/include/SFML/Config.hpp")
file(READ "${sfml_config}" _content)
if(NOT _content MATCHES "__EMSCRIPTEN__")
    string(REPLACE
        "#else\n\n// Unsupported UNIX system\n#error This UNIX operating system is not supported by SFML library"
        "#elif defined(__EMSCRIPTEN__)\n\n// Emscripten / WebAssembly\n#define SFML_SYSTEM_LINUX\n\n#else\n\n// Unsupported UNIX system\n#error This UNIX operating system is not supported by SFML library"
        _content "${_content}")
    file(WRITE "${sfml_config}" "${_content}")
endif()
