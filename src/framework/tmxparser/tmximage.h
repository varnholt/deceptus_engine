#pragma once

#include "tmxelement.h"

///
/// \brief Represents image metadata used by TMX layers or tilesets.
///
struct TmxImage : TmxElement
{
   ///
   /// \brief Constructs an empty image descriptor.
   ///
   TmxImage() = default;

   ///
   /// \brief Parses TMX image attributes (`source`, `width`, `height`).
   /// \param e XML element for `<image>`.
   /// \param parse_data Shared TMX parse context.
   ///
   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>& parse_data) override;

   std::string _source;
   int _width_px = 0;
   int _height_px = 0;
};
