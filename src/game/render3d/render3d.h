#pragma once

// Include all necessary components for 3D rendering functionality
#include "3dobject.h"
#include "texturedobject.h"
#include "texturedsphereobject.h"
#include "renderer3d.h"

// Define StarmapObject as a typedef to TexturedObject for consistency with our naming
namespace deceptus {
namespace render3d {
    using StarmapObject = TexturedObject;
}
}