#pragma once

#include "tmxchunk.h"
#include "tmxelement.h"

struct TmxProperties;

///
/// \brief Represents a TMX tile layer, including chunk merge output.
///
struct TmxLayer : TmxElement
{
   ///
   /// \brief Constructs a layer and sets the element type.
   ///
   TmxLayer();

   ///
   /// \brief Parses layer attributes and data (raw csv or chunked infinite data).
   /// \param element XML element for `<layer>`.
   /// \param parse_data Shared TMX parse context.
   ///
   void deserialize(tinyxml2::XMLElement* element, const std::shared_ptr<TmxParseData>& parse_data) override;

   uint32_t _width_tl = 0;
   uint32_t _height_tl = 0;
   float _opacity = 1.0f;
   bool _visible = true;
   std::shared_ptr<TmxProperties> _properties = nullptr;
   int32_t _z = 0;

   int32_t _offset_x_px = 0;
   int32_t _offset_y_px = 0;

   std::vector<int32_t> _data;
};
