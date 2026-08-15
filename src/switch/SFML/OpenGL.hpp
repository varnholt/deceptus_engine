#pragma once
// Nintendo Switch counterpart to src/wasm/SFML/OpenGL.hpp.
//
// The WASM shim pulls in <GLES3/gl3.h>, which is the wrong header here: switch-mesa
// runs on nouveau and exposes desktop OpenGL 4.3 core. Entry points come from the glad
// loader bundled inside VRSFML's sfml-glutils -- see src/switch/opengl/glew.h for why
// devkitPro's switch-glad portlib is deliberately not used.
//
// Because CMakeLists puts src/switch ahead of src/wasm on the include path, this file
// shadows the GLES one for the Switch target.

#include <glad/gl.h>
