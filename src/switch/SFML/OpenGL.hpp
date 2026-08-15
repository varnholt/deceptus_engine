#pragma once
// Nintendo Switch counterpart to src/wasm/SFML/OpenGL.hpp.
//
// The WASM shim pulls in <GLES3/gl3.h>, which is the wrong header here: switch-mesa
// runs on nouveau and exposes desktop OpenGL 4.3 core, loaded through glad. Because
// CMakeLists puts src/switch ahead of src/wasm on the include path, this file shadows
// the GLES one for the Switch target.

#include <glad/glad.h>
