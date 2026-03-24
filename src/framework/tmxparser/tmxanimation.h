#pragma once

#include "tmxelement.h"

struct TmxFrame;

///
/// \brief Stores one TMX tile animation sequence made of frame entries.
///
struct TmxAnimation : TmxElement
{
   ///
   /// \brief Constructs an empty animation.
   ///
   TmxAnimation() = default;

   ///
   /// \brief Reads `<frame>` children and appends them to `_frames`.
   /// \param e XML element representing `<animation>`.
   /// \param parse_data Shared TMX parse context.
   ///
   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>& parse_data) override;
   std::vector<std::shared_ptr<TmxFrame>> _frames;
};
