#pragma once

#include "tmxelement.h"

struct TmxImage;
struct TmxProperties;

///
/// \brief Represents a TMX image layer with optional properties.
///
struct TmxImageLayer : public TmxElement
{
   ///
   /// \brief Constructs an image layer and sets the element type.
   ///
   TmxImageLayer();

   float _offset_x_px = 0.0f;
   float _offset_y_px = 0.0f;
   float _opacity = 1.0f;
   std::shared_ptr<TmxImage> _image;
   std::shared_ptr<TmxProperties> _properties;
   int32_t _z = 0;

   ///
   /// \brief Deserializes image-layer attributes and child elements.
   /// \param e XML element for `<imagelayer>`.
   /// \param parse_data Shared TMX parse context.
   ///
   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>& parse_data) override;
};
