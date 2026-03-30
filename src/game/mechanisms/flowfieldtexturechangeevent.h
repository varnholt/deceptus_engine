#ifndef FLOWFIELDTEXTURECHANGEEVENT_H
#define FLOWFIELDTEXTURECHANGEEVENT_H

#include <string>

/// \brief requests that a dust mechanism swaps to a different flow-field texture.
struct FlowFieldTextureChangeEvent
{
   std::string _object_id;
   std::string _texture_id;
};

#endif  // FLOWFIELDTEXTURECHANGEEVENT_H
