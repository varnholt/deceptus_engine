#pragma once

#include "tmxelement.h"

///
/// \brief Represents one animation frame entry in a TMX tile animation.
///
struct TmxFrame : TmxElement
{
   ///
   /// \brief Constructs an empty frame.
   ///
   TmxFrame() = default;

   ///
   /// \brief Parses `tileid` and `duration` frame attributes.
   /// \param e XML element for `<frame>`.
   /// \param parse_data Shared TMX parse context.
   ///
   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>& parse_data) override;

   int32_t _tile_id = 0;
   int32_t _duration_ms = 0;
};
